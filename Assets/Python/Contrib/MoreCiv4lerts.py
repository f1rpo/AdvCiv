## MoreCiv4lerts
## From HOF MOD V1.61.001
## Based upon Gillmer J. Derge's Civ4lerts.py

from CvPythonExtensions import *
import PyHelpers
import BugCore
import PlayerUtil
import TradeUtil
import CityUtil # advc

# BUG - Mac Support - start
import BugUtil
BugUtil.fixSets(globals())
# BUG - Mac Support - end

gc = CyGlobalContext()
localText = CyTranslator()
PyGame = PyHelpers.PyGame()
PyPlayer = PyHelpers.PyPlayer
PyCity = PyHelpers.PyCity
PyInfo = PyHelpers.PyInfo

PEACE_TREATY_LENGTH = gc.getDefineINT("PEACE_TREATY_LENGTH")

class MoreCiv4lerts:

	def __init__(self, eventManager):
		## Init event handlers
		# <advc.135b> One instance per player
		for iPlayer in range(gc.getMAX_PLAYERS()):
			# advc.706: Not just for humans
			if gc.getPlayer(iPlayer).isHuman() or gc.getGame().isOption(GameOptionTypes.GAMEOPTION_RISE_FALL):
				MoreCiv4lertsEvent(eventManager, iPlayer)
		# <advc.135b>

class AbstractMoreCiv4lertsEvent(object):
	
	def __init__(self, eventManager, iPlayer, *args, **kwargs):
			super( AbstractMoreCiv4lertsEvent, self).__init__(*args, **kwargs)
			# advc.135b: Added attribute iOwner
			self.iOwner = iPlayer

	def _addMessageNoIcon(self, iPlayer, message, iColor=-1):
			#Displays an on-screen message with no popup icon.
			self._addMessage(iPlayer, message, None, -1, -1, False, False, iColor)

	def _addMessageAtCity(self, iPlayer, message, icon, city, iColor=-1):
			#Displays an on-screen message with a popup icon that zooms to the given city.
			self._addMessage(iPlayer, message, icon, city.getX(), city.getY(), True, True, iColor)

	def _addMessageAtPlot(self, iPlayer, message, icon, plot, iColor=-1):
			#Displays an on-screen message with a popup icon that zooms to the given plot.
			self._addMessage(iPlayer, message, icon, plot.getX(), plot.getY(), True, True, iColor)

	def _addMessage(self, iPlayer, szString, szIcon, iFlashX, iFlashY, bOffArrow, bOnArrow, iColor):
			#Displays an on-screen message.
			# <advc.706>
			# advc.127: No alerts during or right after Auto Play
			if gc.getGame().isRFBlockPopups() or gc.getPlayer(self.iOwner).isHumanDisabled() or gc.getPlayer(self.iOwner).isAutoPlayJustEnded():
				return # </advc.706>
			# advc.106c: Reduced time from LONG to normal
			eventMessageTime = gc.getDefineINT("EVENT_MESSAGE_TIME")
			# advc.135b: Ignore iPlayer. Shouldn't be necessary b/c
			# the iActivePlayer (on the caller side) should always
			# be iOwner. Tbd.: Remove the iPlayer attribute from all
			# _addMessage... functions.
			# advc.106: Set bForce to False
			CyInterface().addMessage(self.iOwner, False, eventMessageTime, szString, None, 0, szIcon, ColorTypes(iColor), iFlashX, iFlashY, bOffArrow, bOnArrow)

class MoreCiv4lertsEvent( AbstractMoreCiv4lertsEvent):

	# advc.135b: Param iPlayer (owner) added
	def __init__(self, eventManager, iPlayer, *args, **kwargs):
		super(MoreCiv4lertsEvent, self).__init__(eventManager, iPlayer, *args, **kwargs)

		eventManager.addEventHandler("BeginActivePlayerTurn", self.onBeginActivePlayerTurn)
		eventManager.addEventHandler("cityAcquired", self.OnCityAcquired)
		eventManager.addEventHandler("cityBuilt", self.OnCityBuilt)
		eventManager.addEventHandler("cityRazed", self.OnCityRazed)
		eventManager.addEventHandler("cityLost", self.OnCityLost)
		
		eventManager.addEventHandler("GameStart", self.reset)
		eventManager.addEventHandler("OnLoad", self.reset)

		self.eventMgr = eventManager
		self.options = BugCore.game.MoreCiv4lerts
		self.reset()
	
	def reset(self, argsList=None):
		# <advc.106c><advc.135b>
		# Should perhaps just call checkAlerts with
		# a silent=true parameter.
		# advc.135b: owner instead of activePlayer
		currentTurn = gc.getGame().getGameTurn()
		owner = gc.getPlayer(self.iOwner)
		iTeam = owner.getTeam()
		self.PrevAvailTechTrades = self.getTechTrades(owner, iTeam)
		# (self.CurrAvailTechTrades was never used)
		self.PrevAvailBonusTrades = self.getBonusTrades(owner, iTeam)
		# advc.210e:
		self.PrevAvailBonusSales = self.getBonusSales(owner)
		self.PrevAvailOpenBordersTrades = self.getOpenBordersTrades(owner, iTeam)
		self.PrevAvailMapTrades = self.getMapTrades(owner, iTeam)
		self.PrevAvailDefensivePactTrades = self.getDefensivePactTrades(owner, iTeam)
		self.PrevAvailPermanentAllianceTrades = self.getPermanentAllianceTrades(owner, iTeam)
		self.PrevAvailVassalTrades = self.getVassalTrades(owner, iTeam)
		self.PrevAvailSurrenderTrades = self.getSurrenderTrades(owner, iTeam)
		# advc.210: Disabled
		#self.PrevAvailPeaceTrades = self.getPeaceTrades(owner, iTeam)
		# </advc.135b>
		self.lastDomLimitMsgTurn = currentTurn
		# I don't think the domination alert is an important one, and the
		# computation of last[Pop|Land]Count isn't in a separate function,
		# so I'm leaving the initial/ reset values as they are.
		# </advc.106c>
		self.lastPopCount = 0
		self.lastLandCount = 0

	def getCheckForDomPopVictory(self):
		return False # advc.210
		#return self.options.isShowDomPopAlert()

	def getCheckForDomLandVictory(self):
		return False # advc.210
		#return self.options.isShowDomLandAlert()

	def getPopThreshold(self):
		return self.options.getDomPopThreshold()

	def getLandThreshold(self):
		return self.options.getDomLandThreshold()

	def getCheckForCityBorderExpansion(self):
		return False # advc.210
		#return self.options.isShowCityPendingExpandBorderAlert()

	def getCheckForTechs(self):
		return self.options.isShowTechTradeAlert()
	
	def getCheckForBonuses(self):
		return self.options.isShowBonusTradeAlert()
	
	def getCheckForMap(self):
		return self.options.isShowMapTradeAlert()

	def getCheckForOpenBorders(self):
		return self.options.isShowOpenBordersTradeAlert()

	def getCheckForDefensivePact(self):
		return self.options.isShowDefensivePactTradeAlert()

	def getCheckForPermanentAlliance(self):
		return self.options.isShowPermanentAllianceTradeAlert()
	
	def getCheckForVassal(self):
		return self.options.isShowVassalTradeAlert()
	
	def getCheckForSurrender(self):
		return self.options.isShowSurrenderTradeAlert()
	
	def getCheckForPeace(self):
		return False # advc.210
		#return self.options.isShowPeaceTradeAlert()

	def getCheckForDomVictory(self):
		return False # advc.210
		#return self.getCheckForDomPopVictory() or self.getCheckForDomLandVictory()
	
	def getCheckForForeignCities(self):
		return False # advc.210c
		#return self.options.isShowCityFoundedAlert()

	def onBeginActivePlayerTurn(self, argsList):
		"Called when the active player can start making their moves."
		#iGameTurn = argsList[0] # advc: Unused
		# <advc.127>
		if not gc.getPlayer(self.iOwner).isHuman():
			return # </advc.127>
		iPlayer = gc.getGame().getActivePlayer()
		if iPlayer == self.iOwner: # advc.135b
			self.CheckForAlerts(iPlayer, PyPlayer(iPlayer).getTeam(), True)

	def OnCityAcquired(self, argsList):
		owner, playerType, city, bConquest, bTrade = argsList
		# <advc.127>
		if not gc.getPlayer(self.iOwner).isHuman():
			return # </advc.127>
		iPlayer = city.getOwner()
		if (not self.getCheckForDomVictory()): return
		if (iPlayer == self.iOwner): # advc.135b
			self.CheckForAlerts(iPlayer, PyPlayer(iPlayer).getTeam(), False)

	def OnCityBuilt(self, argsList):
		city = argsList[0]
		# <advc.127>
		if not gc.getPlayer(self.iOwner).isHuman():
			return # </advc.127>
		iPlayer = city.getOwner()
		# advc.135b: All uses replaced with self.iOwner
		#iActivePlayer = gc.getGame().getActivePlayer()
		if (self.getCheckForDomVictory()):
			if (iPlayer == self.iOwner):
				self.CheckForAlerts(iPlayer, PyPlayer(iPlayer).getTeam(), False)
		if (self.getCheckForForeignCities()):
			if (iPlayer != self.iOwner):
				owner = gc.getPlayer(self.iOwner)
				bRevealed = city.isRevealed(owner.getTeam(), False)
				# advc.135b: canSeeCityList checks if ActivePlayer
				# can see the list of iPlayer - should be self.iOwner
				# instead of ActivePlayer. Doesn't matter though b/c
				# K-Mod disables public city lists.
				# advc.210c (comment): The city-founded alert is now entirely disabled.
				if (bRevealed or PlayerUtil.canSeeCityList(iPlayer)):
					player = gc.getPlayer(iPlayer)
					#iColor = gc.getPlayerColorInfo(player.getPlayerColor()).getColorTypePrimary()
					iColor = gc.getInfoTypeForString("COLOR_MAGENTA")
					if (bRevealed):
						message = localText.getText("TXT_KEY_MORECIV4LERTS_CITY_FOUNDED", (player.getName(), city.getName()))
						self._addMessageAtCity(self.iOwner, message, "Art/Interface/Buttons/Actions/foundcity.dds", city, iColor)
					else:
						message = localText.getText("TXT_KEY_MORECIV4LERTS_CITY_FOUNDED_UNSEEN", (player.getName(), city.getName()))
						self._addMessageNoIcon(self.iOwner, message, iColor)

	def OnCityRazed(self, argsList):
		city, iPlayer = argsList
		if (not self.getCheckForDomVictory()): return
		# <advc.127>
		if not gc.getPlayer(self.iOwner).isHuman():
			return # </advc.127>
		if (iPlayer == self.iOwner): # advc.135b
			self.CheckForAlerts(iPlayer, PyPlayer(iPlayer).getTeam(), False)

	def OnCityLost(self, argsList):
		city = argsList[0]
		# <advc.127>
		if not gc.getPlayer(self.iOwner).isHuman():
			return # </advc.127>
		iPlayer = city.getOwner()
		if (not self.getCheckForDomVictory()): return
		if (iPlayer == self.iOwner): # advc.135b
			self.CheckForAlerts(iPlayer, PyPlayer(iPlayer).getTeam(), False)

	def CheckForAlerts(self, iActivePlayer, activeTeam, BeginTurn):
	##Added "else: pass" code to diagnose strange results - might be related to indent issues
		ourPop = 0
		ourLand = 0
		totalPop = 0
		totalLand = 0
		LimitPop =0
		LimitLand = 0
		DomVictory = 3
		popGrowthCount = 0
		currentTurn = gc.getGame().getGameTurn()
		activePlayer = gc.getPlayer(iActivePlayer)

		if (self.getCheckForDomPopVictory() or (BeginTurn and self.getCheckForCityBorderExpansion())):
			# Check for cultural expansion and population growth
			teamPlayerList = []
			teamPlayerList = PyGame.getCivTeamList(PyGame.getActiveTeam())
			teamPlayerList.append(PyPlayer(iActivePlayer))
			for loopPlayer in range(len(teamPlayerList)):
				lCity = []
				# EF: This looks very wrong. Above the list of players will not be 0, 1, ...
				#     but here it uses loopPlayer which is 0, 1, ...
				lCity = PyPlayer(loopPlayer).getCityList()
				for loopCity in range(len(lCity)):
					city = gc.getPlayer(loopPlayer).getCity(loopCity)
					#if (city.getFoodTurnsLeft() == 1 and not city.isFoodProduction()) and not city.AI_isEmphasize(5):
					if CityUtil.willGrowThisTurn(city): # advc
						popGrowthCount = popGrowthCount + 1
					if (BeginTurn and self.getCheckForCityBorderExpansion()):
						if (city.getCultureLevel() != gc.getNumCultureLevelInfos() - 1):
							if ((city.getCulture(loopPlayer) + city.getCommerceRate(CommerceTypes.COMMERCE_CULTURE)) >= city.getCultureThreshold()):
								message = localText.getText("TXT_KEY_MORECIV4LERTS_CITY_TO_EXPAND",(city.getName(),))
								icon = "Art/Interface/Buttons/General/Warning_popup.dds"
								self._addMessageAtCity(loopPlayer, message, icon, city)
							else: pass
						else: pass #expand check
					else: pass #message check
				else: pass #end city loop
			else: pass #end activePlayer loop
		else: pass # end expansion check / pop count

		# Check Domination Limit
		if (self.getCheckForDomVictory() and gc.getGame().isVictoryValid(DomVictory)):
			
			# Population Limit
			if (self.getCheckForDomPopVictory()):
				VictoryPopPercent = 0.0
				VictoryPopPercent = gc.getGame().getAdjustedPopulationPercent(DomVictory) * 1.0
				totalPop = gc.getGame().getTotalPopulation()
				LimitPop = int((totalPop * VictoryPopPercent) / 100.0)
				ourPop = activeTeam.getTotalPopulation()
				if (totalPop > 0):
					popPercent = (ourPop * 100.0) / totalPop
					NextpopPercent = ((ourPop + popGrowthCount) * 100.0) / totalPop
				else:
					popPercent = 0.0
					NextpopPercent = 0.0

				if (totalPop > 1 and (currentTurn <> self.lastDomLimitMsgTurn or (ourPop + popGrowthCount) <> self.lastPopCount)):
					self.lastPopCount = ourPop + popGrowthCount
					if (popPercent >= VictoryPopPercent):
						message = localText.getText("TXT_KEY_MORECIV4LERTS_POP_EXCEEDS_LIMIT",
						   		(ourPop, (u"%.2f%%" % popPercent), LimitPop, (u"%.2f%%" % VictoryPopPercent)))
						self._addMessageNoIcon(iActivePlayer, message)

					elif (popGrowthCount > 0 and NextpopPercent >= VictoryPopPercent):
						message = localText.getText("TXT_KEY_MORECIV4LERTS_POP_GROWTH_EXCEEDS_LIMIT",
						   		(ourPop, popGrowthCount, (u"%.2f%%" % NextpopPercent), LimitPop, (u"%.2f%%" % VictoryPopPercent)))
						self._addMessageNoIcon(iActivePlayer, message)

					elif (popGrowthCount > 0 and (VictoryPopPercent - NextpopPercent < self.getPopThreshold())):
						message = localText.getText("TXT_KEY_MORECIV4LERTS_POP_GROWTH_CLOSE_TO_LIMIT",
						   		(ourPop, popGrowthCount, (u"%.2f%%" % NextpopPercent), LimitPop, (u"%.2f%%" % VictoryPopPercent)))
						self._addMessageNoIcon(iActivePlayer, message)

## .005 			elif (VictoryPopPercent - popPercent < self.getPopThreshold()):
					elif (popGrowthCount > 0 and (VictoryPopPercent - popPercent < self.getPopThreshold())):
						message = localText.getText("TXT_KEY_MORECIV4LERTS_POP_CLOSE_TO_LIMIT",
						   		(ourPop, (u"%.2f%%" % popPercent), LimitPop, (u"%.2f%%" % VictoryPopPercent)))
						self._addMessageNoIcon(iActivePlayer, message)
					else: pass #end elif
				else: pass #end totalPop if
			else: pass #end pop limit if

			# Land Limit
			if (self.getCheckForDomLandVictory()):
				VictoryLandPercent = 0.0
				VictoryLandPercent = gc.getGame().getAdjustedLandPercent(DomVictory) * 1.0
				totalLand = gc.getMap().getLandPlots()
				LimitLand = int((totalLand * VictoryLandPercent) / 100.0)
				ourLand = activeTeam.getTotalLand()
				if (totalLand > 0):
					landPercent = (ourLand * 100.0) / totalLand
				else:
					landPercent = 0.0
				if (currentTurn <> self.lastDomLimitMsgTurn or ourLand <> self.lastLandCount):
					self.lastLandCount = ourLand
					if (landPercent > VictoryLandPercent):
						message = localText.getText("TXT_KEY_MORECIV4LERTS_LAND_EXCEEDS_LIMIT",
								(ourLand, (u"%.2f%%" % landPercent), LimitLand, (u"%.2f%%" % VictoryLandPercent)))
						self._addMessageNoIcon(iActivePlayer, message)
					elif (VictoryLandPercent - landPercent < self.getLandThreshold()):
						message = localText.getText("TXT_KEY_MORECIV4LERTS_LAND_CLOSE_TO_LIMIT",
								(ourLand, (u"%.2f%%" % landPercent), LimitLand, (u"%.2f%%" % VictoryLandPercent)))
						self._addMessageNoIcon(iActivePlayer, message)
					else: pass #end elif
				else: pass #end currentTurn if
			else: pass #end land limit if
		else: pass #end dom limt if
	
		#save turn num
		if (self.getCheckForDomVictory()):
			self.lastDomLimitMsgTurn = currentTurn

		# tech trades
		if (BeginTurn and self.getCheckForTechs()):
			researchTechs = set()
			for iTech in range(gc.getNumTechInfos()):
				if (activePlayer.canResearch(iTech, True)):
					researchTechs.add(iTech)
			techsByPlayer = self.getTechTrades(activePlayer, activeTeam)
			for iLoopPlayer, currentTechs in techsByPlayer.iteritems():

				#Did he have trades avail last turn
				if (self.PrevAvailTechTrades.has_key(iLoopPlayer)):
					previousTechs = self.PrevAvailTechTrades[iLoopPlayer]
				else:
					previousTechs = set()
					
				#Determine new techs
				newTechs = currentTechs.difference(previousTechs).intersection(researchTechs)
				if (newTechs):
					szNewTechs = self.buildTechString(newTechs)
					message = localText.getText("TXT_KEY_MORECIV4LERTS_NEW_TECH_AVAIL",	
												(gc.getPlayer(iLoopPlayer).getName(), szNewTechs))
					self._addMessageNoIcon(iActivePlayer, message)
				
				#Determine removed techs
				removedTechs = previousTechs.difference(currentTechs).intersection(researchTechs)
				if (removedTechs):
					szRemovedTechs = self.buildTechString(removedTechs)
					message = localText.getText("TXT_KEY_MORECIV4LERTS_TECH_NOT_AVAIL",	
												(gc.getPlayer(iLoopPlayer).getName(), szRemovedTechs))
					self._addMessageNoIcon(iActivePlayer, message)
				
			else: pass #end activePlayer loop

			#save curr trades for next time
			self.PrevAvailTechTrades = techsByPlayer

		else: pass #end new trades if
		
		# bonus trades
		if (BeginTurn and self.getCheckForBonuses()):
			desiredBonuses = TradeUtil.getDesiredBonuses(activePlayer)
			tradesByPlayer = self.getBonusTrades(activePlayer, activeTeam)
			for iLoopPlayer, currentTrades in tradesByPlayer.iteritems():

				#Did he have trades avail last turn
				if (self.PrevAvailBonusTrades.has_key(iLoopPlayer)):
					previousTrades = self.PrevAvailBonusTrades[iLoopPlayer]
				else:
					previousTrades = set()
					
				#Determine new bonuses
				newTrades = currentTrades.difference(previousTrades).intersection(desiredBonuses)
				if (newTrades):
					szNewTrades = self.buildBonusString(newTrades)
					message = localText.getText("TXT_KEY_MORECIV4LERTS_NEW_BONUS_AVAIL",	
												(gc.getPlayer(iLoopPlayer).getName(), szNewTrades))
					self._addMessageNoIcon(iActivePlayer, message)
					# advc.106: Moved here to avoid messages about "flickering" offers
					self.PrevAvailBonusTrades = tradesByPlayer
				
				#Determine removed bonuses
				# <advc.106> This is rarely relevant (resources being no
				# longer available).
				#removedTrades = previousTrades.difference(currentTrades).intersection(desiredBonuses)
				#if (removedTrades):
				#	szRemovedTrades = self.buildBonusString(removedTrades)
				#	message = #localText.getText("TXT_KEY_MORECIV4LERTS_BONUS_NOT_AVAIL",	
				#								(gc.getPlayer(iLoopPlayer).getName(), szRemovedTrades))
				#	self._addMessageNoIcon(iActivePlayer, message)
				# </advc.106>

			#save curr trades for next time
			# self.PrevAvailBonusTrades = tradesByPlayer # advc.106: moved up
			self.checkForExports(activePlayer) # advc.210e
		
		if (BeginTurn and self.getCheckForMap()):
			currentTrades = self.getMapTrades(activePlayer, activeTeam)
			newTrades = currentTrades.difference(self.PrevAvailMapTrades)
			self.PrevAvailMapTrades = currentTrades
			if (newTrades):
				players = self.buildPlayerString(newTrades)
				message = localText.getText("TXT_KEY_MORECIV4LERTS_MAP", (players,))
				self._addMessageNoIcon(iActivePlayer, message)
		
		if (BeginTurn and self.getCheckForOpenBorders()):
			currentTrades = self.getOpenBordersTrades(activePlayer, activeTeam)
			newTrades = currentTrades.difference(self.PrevAvailOpenBordersTrades)
			self.PrevAvailOpenBordersTrades = currentTrades
			if (newTrades):
				players = self.buildPlayerString(newTrades)
				message = localText.getText("TXT_KEY_MORECIV4LERTS_OPEN_BORDERS", (players,))
				self._addMessageNoIcon(iActivePlayer, message)
		
		if (BeginTurn and self.getCheckForDefensivePact()):
			currentTrades = self.getDefensivePactTrades(activePlayer, activeTeam)
			newTrades = currentTrades.difference(self.PrevAvailDefensivePactTrades)
			self.PrevAvailDefensivePactTrades = currentTrades
			if (newTrades):
				players = self.buildPlayerString(newTrades)
				message = localText.getText("TXT_KEY_MORECIV4LERTS_DEFENSIVE_PACT", (players,))
				self._addMessageNoIcon(iActivePlayer, message)
		
		if (BeginTurn and self.getCheckForPermanentAlliance()):
			currentTrades = self.getPermanentAllianceTrades(activePlayer, activeTeam)
			newTrades = currentTrades.difference(self.PrevAvailPermanentAllianceTrades)
			self.PrevAvailPermanentAllianceTrades = currentTrades
			if (newTrades):
				players = self.buildPlayerString(newTrades)
				message = localText.getText("TXT_KEY_MORECIV4LERTS_PERMANENT_ALLIANCE", (players,))
				self._addMessageNoIcon(iActivePlayer, message)
		
		if (BeginTurn and self.getCheckForVassal()):
			currentTrades = self.getVassalTrades(activePlayer, activeTeam)
			newTrades = currentTrades.difference(self.PrevAvailVassalTrades)
			self.PrevAvailVassalTrades = currentTrades
			if (newTrades):
				players = self.buildPlayerString(newTrades)
				message = localText.getText("TXT_KEY_MORECIV4LERTS_VASSAL", (players,))
				self._addMessageNoIcon(iActivePlayer, message)
		
		if (BeginTurn and self.getCheckForSurrender()):
			currentTrades = self.getSurrenderTrades(activePlayer, activeTeam)
			newTrades = currentTrades.difference(self.PrevAvailSurrenderTrades)
			self.PrevAvailSurrenderTrades = currentTrades
			if (newTrades):
				players = self.buildPlayerString(newTrades)
				message = localText.getText("TXT_KEY_MORECIV4LERTS_SURRENDER", (players,))
				self._addMessageNoIcon(iActivePlayer, message)

		if (BeginTurn and self.getCheckForPeace()):
			currentTrades = self.getPeaceTrades(activePlayer, activeTeam)
			newTrades = currentTrades.difference(self.PrevAvailPeaceTrades)
			self.PrevAvailPeaceTrades = currentTrades
			if (newTrades):
				players = self.buildPlayerString(newTrades)
				message = localText.getText("TXT_KEY_MORECIV4LERTS_PEACE_TREATY", (players,))
				self._addMessageNoIcon(iActivePlayer, message)
	
	# <advc.210e> Based on 'bonus trades' code
	def checkForExports(self, player):
		tradesByPlayer = self.getBonusSales(player)
		for iLoopPlayer, currentTrades in tradesByPlayer.iteritems():
			if (self.PrevAvailBonusSales.has_key(iLoopPlayer)):
				previousTrades = self.PrevAvailBonusSales[iLoopPlayer]
			else:
				previousTrades = set()
			newTrades = currentTrades.difference(previousTrades)
			if (newTrades):
				szNewTrades = self.buildBonusString(newTrades)
				message = localText.getText("TXT_KEY_MORECIV4LERTS_NEW_SALE", (gc.getPlayer(iLoopPlayer).getName(), szNewTrades))
				self._addMessageNoIcon(player.getID(), message)
				self.PrevAvailBonusSales = tradesByPlayer
	# </advc.210e>

	def getTechTrades(self, player, team):
		iPlayerID = player.getID()
		tradeData = TradeData()
		tradeData.ItemType = TradeableItems.TRADE_TECHNOLOGIES
		techsByPlayer = {}
		for loopPlayer in TradeUtil.getTechTradePartners(player):
			techsToTrade = set()
			for iLoopTech in range(gc.getNumTechInfos()):
				tradeData.iData = iLoopTech
				if (loopPlayer.canTradeItem(iPlayerID, tradeData, False)):
					if (loopPlayer.getTradeDenial(iPlayerID, tradeData) == DenialTypes.NO_DENIAL): # will trade
						techsToTrade.add(iLoopTech)
			techsByPlayer[loopPlayer.getID()] = techsToTrade
		return techsByPlayer

	def getBonusTrades(self, player, team):
		bonusesByPlayer = {}
		for loopPlayer in TradeUtil.getBonusTradePartners(player):
			will, wont = TradeUtil.getTradeableBonuses(loopPlayer, player)
			bonusesByPlayer[loopPlayer.getID()] = will
		return bonusesByPlayer
	# <advc.210e>
	def getBonusSales(self, player):
		r = {}
		playerGoldTr = gc.getTeam(player.getTeam()).isGoldTrading()
		for other in TradeUtil.getBonusTradePartners(player):
			if other.isHuman():
				continue
			if not playerGoldTr and not gc.getTeam(other.getTeam()).isGoldTrading():
				continue
			if other.AI_maxGoldPerTurnTrade(player.getID()) <= 2:
				continue
			# As getBonusTrades, but switch the parameters in this call:
			will, wont = TradeUtil.getTradeableBonuses(player, other)
			r[other.getID()] = set()
			for iBonus in will.intersection(TradeUtil.getDesiredBonuses(other)):
				if player.getNumTradeableBonuses(iBonus) > 1 and player.AI_corporationBonusVal(iBonus) <= 0:
					r[other.getID()].add(iBonus)
		return r # </advc.210e>

	def getMapTrades(self, player, team):
		iPlayerID = player.getID()
		tradeData = TradeData()
		tradeData.ItemType = TradeableItems.TRADE_MAPS
		currentTrades = set()
		for loopPlayer in TradeUtil.getMapTradePartners(player):
			#tradeData.iData = None
			if (loopPlayer.canTradeItem(iPlayerID, tradeData, False)):
				if (loopPlayer.getTradeDenial(iPlayerID, tradeData) == DenialTypes.NO_DENIAL): # will trade
					currentTrades.add(loopPlayer.getID())
		return currentTrades

	def getOpenBordersTrades(self, player, team):
		iPlayerID = player.getID()
		tradeData = TradeData()
		tradeData.ItemType = TradeableItems.TRADE_OPEN_BORDERS
		currentTrades = set()
		for loopPlayer in TradeUtil.getOpenBordersTradePartners(player):
			#tradeData.iData = None
			if (loopPlayer.canTradeItem(iPlayerID, tradeData, False)):
				if (loopPlayer.getTradeDenial(iPlayerID, tradeData) == DenialTypes.NO_DENIAL): # will trade
					currentTrades.add(loopPlayer.getID())
		return currentTrades

	def getDefensivePactTrades(self, player, team):
		iPlayerID = player.getID()
		tradeData = TradeData()
		tradeData.ItemType = TradeableItems.TRADE_DEFENSIVE_PACT
		currentTrades = set()
		for loopPlayer in TradeUtil.getDefensivePactTradePartners(player):
			#tradeData.iData = None
			if (loopPlayer.canTradeItem(iPlayerID, tradeData, False)):
				if (loopPlayer.getTradeDenial(iPlayerID, tradeData) == DenialTypes.NO_DENIAL): # will trade
					currentTrades.add(loopPlayer.getID())
		return currentTrades

	def getPermanentAllianceTrades(self, player, team):
		iPlayerID = player.getID()
		tradeData = TradeData()
		tradeData.ItemType = TradeableItems.TRADE_PERMANENT_ALLIANCE
		currentTrades = set()
		for loopPlayer in TradeUtil.getPermanentAllianceTradePartners(player):
			#tradeData.iData = None
			if (loopPlayer.canTradeItem(iPlayerID, tradeData, False)):
				if (loopPlayer.getTradeDenial(iPlayerID, tradeData) == DenialTypes.NO_DENIAL): # will trade
					currentTrades.add(loopPlayer.getID())
		return currentTrades

	def getVassalTrades(self, player, team):
		iPlayerID = player.getID()
		tradeData = TradeData()
		tradeData.ItemType = TradeableItems.TRADE_VASSAL
		currentTrades = set()
		for loopPlayer in TradeUtil.getVassalTradePartners(player):
			# <advc.135>
			if gc.getTeam(loopPlayer.getTeam()).isHuman():
				continue # </advc.135>
			#tradeData.iData = None
			if (loopPlayer.canTradeItem(iPlayerID, tradeData, False)):
				if (loopPlayer.getTradeDenial(iPlayerID, tradeData) == DenialTypes.NO_DENIAL): # will trade
					currentTrades.add(loopPlayer.getID())
		return currentTrades

	def getSurrenderTrades(self, player, team):
		iPlayerID = player.getID()
		tradeData = TradeData()
		tradeData.ItemType = TradeableItems.TRADE_SURRENDER
		currentTrades = set()
		for loopPlayer in TradeUtil.getCapitulationTradePartners(player):
			# <advc.135>
			if gc.getTeam(loopPlayer.getTeam()).isHuman():
				continue # </advc.135>
			#tradeData.iData = None
			if (loopPlayer.canTradeItem(iPlayerID, tradeData, False)):
				if (loopPlayer.getTradeDenial(iPlayerID, tradeData) == DenialTypes.NO_DENIAL): # will trade
					currentTrades.add(loopPlayer.getID())
		return currentTrades

	def getPeaceTrades(self, player, team):
		iPlayerID = player.getID()
		tradeData = TradeData()
		tradeData.ItemType = TradeableItems.TRADE_PEACE_TREATY
		tradeData.iData = PEACE_TREATY_LENGTH
		currentTrades = set()
		for loopPlayer in TradeUtil.getPeaceTradePartners(player):
			if (loopPlayer.canTradeItem(iPlayerID, tradeData, False)):
				# advc.104i: Added call to AI_isWillingToTalk; the crucial UWAI check is in there.
				if (loopPlayer.getTradeDenial(iPlayerID, tradeData) == DenialTypes.NO_DENIAL and loopPlayer.AI_isWillingToTalk(iPlayerID)): 
					currentTrades.add(loopPlayer.getID())
		return currentTrades
	
	def buildTechString(self, techs):
		return self.buildItemString(techs, gc.getTechInfo, CvTechInfo.getDescription)
	
	def buildBonusString(self, bonuses):
		return self.buildItemString(bonuses, gc.getBonusInfo, CvBonusInfo.getDescription)

	def buildPlayerString(self, players):
		return self.buildItemString(players, gc.getPlayer, CyPlayer.getName)
	
	def buildItemString(self, items, getItemFunc, getNameFunc):
		names = [getNameFunc(getItemFunc(eItem)) for eItem in items]
		names.sort()
		return u", ".join(names)
