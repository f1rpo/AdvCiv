## Sid Meier's Civilization 4
## Copyright Firaxis Games 2005
from CvPythonExtensions import *
import CvUtil
import PyHelpers

PyPlayer = PyHelpers.PyPlayer

DebugLogging = False

gc = CyGlobalContext()

class CvDiplomacy:
	"Code used by Civ Diplomacy interface"
	
	def __init__(self):
		"constructor - set up class vars, AI and User strings"
		if DebugLogging:
			print "Launching Diplomacy"
		
		self.iLastResponseID = -1

		self.diploScreen = CyDiplomacy()
			
	def setDebugLogging(self, bDebugLogging):
		global DebugLogging
		DebugLogging = bDebugLogging
	
	def determineResponses (self, eComment):
		"Will determine the user responses given an AI comment"
		if DebugLogging:
			print "CvDiplomacy.determineResponses: %s" %(eComment,)
		
		# Eliminate previous comments
		self.diploScreen.clearUserComments()
		
		# If the AI is declaring war
		if (self.isComment(eComment, "AI_DIPLOCOMMENT_DECLARE_WAR") ):

			# We respond to their declaration
			self.addUserComment("USER_DIPLOCOMMENT_WAR_RESPONSE", -1, -1)
			self.diploScreen.endTrade()

		# If this is the first time we are being contacted by the AI
		# Or if the AI is no longer a vassal
		elif (self.isComment(eComment, "AI_DIPLOCOMMENT_FIRST_CONTACT") or self.isComment(eComment, "AI_DIPLOCOMMENT_NO_VASSAL")):

			# if you are on different teams and NOT at war, give the user the option to declare war
			if (gc.getTeam(gc.getGame().getActiveTeam()).canDeclareWar(gc.getPlayer(self.diploScreen.getWhoTradingWith()).getTeam())):
				self.addUserComment("USER_DIPLOCOMMENT_WAR", -1, -1)

			# We say hi and begin our peace
			self.addUserComment("USER_DIPLOCOMMENT_PEACE", -1, -1)

			self.diploScreen.endTrade()

		# The AI refuses to talk
		elif (self.isComment(eComment, "AI_DIPLOCOMMENT_REFUSE_TO_TALK") ):
	
			# K-Mod: give the option to declare war!
			if (gc.getTeam(gc.getGame().getActiveTeam()).canDeclareWar(gc.getPlayer(self.diploScreen.getWhoTradingWith()).getTeam())):
				self.addUserComment("USER_DIPLOCOMMENT_WAR", -1, -1)
			# K-Mod end

			# Give the option to exit
			self.addUserComment("USER_DIPLOCOMMENT_EXIT", -1, -1)
			self.diploScreen.endTrade();

		# If the AI is offering a city oo
		# If the AI is giving help
		elif (self.isComment(eComment, "AI_DIPLOCOMMENT_OFFER_CITY") or
					self.isComment(eComment, "AI_DIPLOCOMMENT_GIVE_HELP")):

			# We can accept their offer
			self.addUserComment("USER_DIPLOCOMMENT_ACCEPT_OFFER", -1, -1)
			# Or reject it...
			self.addUserComment("USER_DIPLOCOMMENT_REJECT_OFFER", -1, -1)

		# If the AI is offering a deal
		elif (self.isComment(eComment, "AI_DIPLOCOMMENT_OFFER_PEACE") or
					self.isComment(eComment, "AI_DIPLOCOMMENT_OFFER_DEAL") or
					self.isComment(eComment, "AI_DIPLOCOMMENT_OFFER_VASSAL")):

			# We can accept their offer
			self.addUserComment("USER_DIPLOCOMMENT_ACCEPT_OFFER", -1, -1)
			# Or we can try to negotiate
			self.addUserComment("USER_DIPLOCOMMENT_RENEGOTIATE", -1, -1)
			# Or reject it...
			self.addUserComment("USER_DIPLOCOMMENT_REJECT_OFFER", -1, -1)

		# If the AI is cancelling a deal
		elif (self.isComment(eComment, "AI_DIPLOCOMMENT_CANCEL_DEAL")):

			# We can try to renegotiate
			self.addUserComment("USER_DIPLOCOMMENT_RENEGOTIATE", -1, -1)
			# Or just exit...
			self.addUserComment("USER_DIPLOCOMMENT_NO_RENEGOTIATE", -1, -1)

		# If the AI is demanding tribute
		elif (self.isComment(eComment, "AI_DIPLOCOMMENT_ASK_FOR_HELP")):

			# We can give them help
			self.addUserComment("USER_DIPLOCOMMENT_GIVE_HELP", -1, -1)
			# Or refuse...
			self.addUserComment("USER_DIPLOCOMMENT_REFUSE_HELP", -1, -1)

		# If the AI is demanding tribute
		elif (self.isComment(eComment, "AI_DIPLOCOMMENT_DEMAND_TRIBUTE")):
	
			# We can accept their demands
			self.addUserComment("USER_DIPLOCOMMENT_ACCEPT_DEMAND", -1, -1)
			# Or reject them...
			self.addUserComment("USER_DIPLOCOMMENT_REJECT_DEMAND", -1, -1)

		# If the AI is pressuring us to convert to their religion
		elif (self.isComment(eComment, "AI_DIPLOCOMMENT_RELIGION_PRESSURE")):
	
			# We can accept their demands
			self.addUserComment("USER_DIPLOCOMMENT_CONVERT", -1, -1)
			# Or reject them...
			self.addUserComment("USER_DIPLOCOMMENT_NO_CONVERT", -1, -1)
	
		# If the AI is pressuring us to switch to their favorite civic
		elif (self.isComment(eComment, "AI_DIPLOCOMMENT_CIVIC_PRESSURE")):
	
			# We can accept their demands
			self.addUserComment("USER_DIPLOCOMMENT_REVOLUTION", -1, -1)
			# Or reject them...
			self.addUserComment("USER_DIPLOCOMMENT_NO_REVOLUTION", -1, -1)
	
		# If the AI is pressuring us to join their war
		elif (self.isComment(eComment, "AI_DIPLOCOMMENT_JOIN_WAR")):
	
			# We can accept their demands
			self.addUserComment("USER_DIPLOCOMMENT_JOIN_WAR", -1, -1)
			# Or reject them...
			self.addUserComment("USER_DIPLOCOMMENT_NO_JOIN_WAR", -1, -1)
	
		# If the AI is pressuring us to stop trading with their enemy
		elif (self.isComment(eComment, "AI_DIPLOCOMMENT_STOP_TRADING")):
	
			# We can accept their demands
			self.addUserComment("USER_DIPLOCOMMENT_STOP_TRADING", -1, -1)
			# Or reject them...
			self.addUserComment("USER_DIPLOCOMMENT_NO_STOP_TRADING", -1, -1)

		# If we are viewing our current deals or
		elif (self.isComment(eComment, "AI_DIPLOCOMMENT_CURRENT_DEALS")):

			# Exit option
			self.addUserComment("USER_DIPLOCOMMENT_NEVERMIND", -1, -1)
			self.addUserComment("USER_DIPLOCOMMENT_EXIT", -1, -1)
			self.diploScreen.startTrade( eComment, true)

		# If we are trading or
		# If we are trying another proposal or
		# If they reject our offer or
		# If they reject our demand
		elif (self.isComment(eComment, "AI_DIPLOCOMMENT_TRADING") or 
					self.isComment(eComment, "AI_DIPLOCOMMENT_REJECT") or
					self.isComment(eComment, "AI_DIPLOCOMMENT_SORRY") or
					self.isComment(eComment, "AI_DIPLOCOMMENT_TRY_THIS_DEAL") or 
					self.isComment(eComment, "AI_DIPLOCOMMENT_NO_DEAL") or
					self.isComment(eComment, "AI_DIPLOCOMMENT_REJECT_ASK") or
					self.isComment(eComment, "AI_DIPLOCOMMENT_REJECT_DEMAND")):
	
			# If no one is currently offering anything
			if (self.diploScreen.ourOfferEmpty() == 1 and self.diploScreen.theirOfferEmpty() == 1):
	
				# If we are at war, allow us to suggest a peace treaty
				if (self.diploScreen.atWar()):
					self.addUserComment("USER_DIPLOCOMMENT_PROPOSE", -1, -1)
					self.addUserComment("USER_DIPLOCOMMENT_OFFER_PEACE", -1, -1)
	
			# If one of us has something on the table
			# <advc.ctr>
			bOurOfferEmpty = self.diploScreen.ourOfferEmpty()
			bTheirOfferEmpty = self.diploScreen.theirOfferEmpty()
			# </advc.ctr>
			if not bOurOfferEmpty or not bTheirOfferEmpty:
	
				# If the offer is from the AI
				if (self.diploScreen.isAIOffer()):
	
					# We can accept or reject the offer
					self.addUserComment("USER_DIPLOCOMMENT_ACCEPT", -1, -1)
					self.addUserComment("USER_DIPLOCOMMENT_REJECT", -1, -1)

				# Otherwise this is a player offer to the AI
				else:
					# <advc.ctr>
					# Treat offer as empty if it's only a peace treaty at peacetime. Since such peace treaties are always appended at the end of the offer list (by CvPlayder::updateTradeList), it's sufficient to check iIndex=0. (I'm not sure if any other position can be safely checked as the interface of CyDiplomacy doesn't have getters for the list lengths.)
					if not self.diploScreen.atWar():
						if not bOurOfferEmpty and self.diploScreen.getPlayerTradeOffer(0).ItemType == TradeableItems.TRADE_PEACE_TREATY:
							bOurOfferEmpty = True
						if not bTheirOfferEmpty and self.diploScreen.getTheirTradeOffer(0).ItemType == TradeableItems.TRADE_PEACE_TREATY:
							bTheirOfferEmpty = True
					# </advc.ctr>
					# This is a two way deal
					if not bOurOfferEmpty and not bTheirOfferEmpty:
					
						# Insert the propose trade button
						self.addUserComment("USER_DIPLOCOMMENT_PROPOSE", -1, -1)

						# During peace, see what we can get for these items
						if (not self.diploScreen.atWar()):
							if (gc.getGame().getActiveTeam() != gc.getPlayer(self.diploScreen.getWhoTradingWith()).getTeam()):
								self.addUserComment("USER_DIPLOCOMMENT_COMPLETE_DEAL", -1, -1)

					# Otherwise they have something on the table and we dont
					elif not bTheirOfferEmpty:

						# If we are at war, demand the items for peace or ask what they want
						if (self.diploScreen.atWar()):
							self.addUserComment("USER_DIPLOCOMMENT_PROPOSE", -1, -1)

						# Otherwise (during peacetime) ask what they want for our item or demand they give it to us
						else: # <advc.155> Rearranged so that OFFER always appears
							self.addUserComment("USER_DIPLOCOMMENT_OFFER", -1, -1)
							if (gc.getGame().getActiveTeam() == gc.getPlayer(self.diploScreen.getWhoTradingWith()).getTeam()):
								self.addUserComment("USER_DIPLOCOMMENT_DEMAND_TEAM", -1, -1)
							else: # </advc.155>
								if (gc.getTeam(gc.getPlayer(self.diploScreen.getWhoTradingWith()).getTeam()).isVassal(gc.getGame().getActiveTeam()) and self.diploScreen.theirVassalTribute()):
									self.addUserComment("USER_DIPLOCOMMENT_VASSAL_TRIBUTE", -1, -1)
								elif (gc.getPlayer(self.diploScreen.getWhoTradingWith()).AI_getAttitude(gc.getGame().getActivePlayer()) >= AttitudeTypes.ATTITUDE_PLEASED):
									self.addUserComment("USER_DIPLOCOMMENT_ASK", -1, -1)
								elif (gc.getTeam(gc.getPlayer(self.diploScreen.getWhoTradingWith()).getTeam()).isVassal(gc.getGame().getActiveTeam()) or gc.getTeam(gc.getGame().getActiveTeam()).canDeclareWar(gc.getPlayer(self.diploScreen.getWhoTradingWith()).getTeam())):
									self.addUserComment("USER_DIPLOCOMMENT_DEMAND", -1, -1)
	
					# Otherwise we have something on the table and they dont
					else:

						# If we are at war, use this item to fish for peace or propose peace with the items
						if (self.diploScreen.atWar()):
							self.addUserComment("USER_DIPLOCOMMENT_PROPOSE", -1, -1)

						# During peace, see what we can get for these items or simply gift them to the AI
						else: # advc.155: Commented out
							#if (gc.getGame().getActiveTeam() != gc.getPlayer(self.diploScreen.getWhoTradingWith()).getTeam()):
							self.addUserComment("USER_DIPLOCOMMENT_FISH_FOR_DEAL", -1, -1)

							self.addUserComment("USER_DIPLOCOMMENT_GIFT", -1, -1)

			# Exit option
			self.addUserComment("USER_DIPLOCOMMENT_NEVERMIND", -1, -1)
			self.addUserComment("USER_DIPLOCOMMENT_EXIT", -1, -1)
			self.diploScreen.startTrade( eComment, false )

		elif (self.isComment(eComment, "AI_DIPLOCOMMENT_SOMETHING_ELSE")):
			if (gc.getTeam(gc.getGame().getActiveTeam()).canDeclareWar(gc.getPlayer(self.diploScreen.getWhoTradingWith()).getTeam())):
				self.addUserComment("USER_DIPLOCOMMENT_WAR", -1, -1)

			self.addUserComment("USER_DIPLOCOMMENT_ATTITUDE", -1, -1)

			if (gc.getGame().getActiveTeam() == gc.getPlayer(self.diploScreen.getWhoTradingWith()).getTeam() or gc.getTeam(gc.getPlayer(self.diploScreen.getWhoTradingWith()).getTeam()).isVassal(gc.getGame().getActiveTeam())):
				self.addUserComment("USER_DIPLOCOMMENT_RESEARCH", -1, -1)

			#if (gc.getTeam(gc.getGame().getActiveTeam()).AI_shareWar(gc.getPlayer(self.diploScreen.getWhoTradingWith()).getTeam())):
			# K-Mod. The AI should not just accept target requests from anyone...
			if (gc.getTeam(gc.getGame().getActiveTeam()).AI_shareWar(gc.getPlayer(self.diploScreen.getWhoTradingWith()).getTeam()) and
				(gc.getPlayer(self.diploScreen.getWhoTradingWith()).AI_getAttitude(gc.getGame().getActivePlayer()) >= AttitudeTypes.ATTITUDE_PLEASED or
				gc.getPlayer(self.diploScreen.getWhoTradingWith()).getTeam() == gc.getGame().getActiveTeam() or
				gc.getTeam(gc.getPlayer(self.diploScreen.getWhoTradingWith()).getTeam()).isVassal(gc.getGame().getActiveTeam()))):
			# K-Mod end
				self.addUserComment("USER_DIPLOCOMMENT_TARGET", -1, -1)

			# K-Mod. Allow masters to tell their vassals to prepare for war
			if (gc.getTeam(gc.getPlayer(self.diploScreen.getWhoTradingWith()).getTeam()).getAtWarCount(true) == 0 and
			    (gc.getTeam(gc.getPlayer(self.diploScreen.getWhoTradingWith()).getTeam()).isVassal(gc.getGame().getActiveTeam()) or
			     gc.getPlayer(self.diploScreen.getWhoTradingWith()).getTeam() == gc.getGame().getActiveTeam())):
				if (gc.getTeam(gc.getPlayer(self.diploScreen.getWhoTradingWith()).getTeam()).getAnyWarPlanCount(true) > 0):
					self.addUserComment("USER_DIPLOCOMMENT_WARPLAN_CANCEL", -1, -1)
				else:
					self.addUserComment("USER_DIPLOCOMMENT_WARPLAN", -1, -1)
			# K-Mod end

			self.addUserComment("USER_DIPLOCOMMENT_NEVERMIND", -1, -1)
			self.addUserComment("USER_DIPLOCOMMENT_EXIT", -1, -1)

		elif (self.isComment(eComment, "AI_DIPLOCOMMENT_RESEARCH")):
			for i in range(gc.getNumTechInfos()):
				if (gc.getPlayer(self.diploScreen.getWhoTradingWith()).canResearch(i, False)):
					self.addUserComment("USER_DIPLOCOMMENT_RESEARCH_TECH", i, -1, gc.getTechInfo(i).getTextKey())

			self.addUserComment("USER_DIPLOCOMMENT_SOMETHING_ELSE", -1, -1)
			self.addUserComment("USER_DIPLOCOMMENT_EXIT", -1, -1)

		elif (self.isComment(eComment, "AI_DIPLOCOMMENT_ATTITUDE") or
					self.isComment(eComment, "AI_DIPLOCOMMENT_ATTITUDE_PLAYER_FURIOUS") or 
					self.isComment(eComment, "AI_DIPLOCOMMENT_ATTITUDE_PLAYER_ANNOYED") or
					self.isComment(eComment, "AI_DIPLOCOMMENT_ATTITUDE_PLAYER_CAUTIOUS") or
					self.isComment(eComment, "AI_DIPLOCOMMENT_ATTITUDE_PLAYER_PLEASED") or 
					self.isComment(eComment, "AI_DIPLOCOMMENT_ATTITUDE_PLAYER_FRIENDLY")):
			for i in range(gc.getMAX_CIV_PLAYERS()):
				if (gc.getPlayer(i).isAlive()):
					if ((i != gc.getGame().getActivePlayer()) and (i != self.diploScreen.getWhoTradingWith())):
						if (gc.getTeam(gc.getGame().getActiveTeam()).isHasMet(gc.getPlayer(i).getTeam()) and gc.getTeam(gc.getPlayer(self.diploScreen.getWhoTradingWith()).getTeam()).isHasMet(gc.getPlayer(i).getTeam())):
							self.addUserComment("USER_DIPLOCOMMENT_ATTITUDE_PLAYER", i, -1, gc.getPlayer(i).getNameKey())

			self.addUserComment("USER_DIPLOCOMMENT_SOMETHING_ELSE", -1, -1)
			self.addUserComment("USER_DIPLOCOMMENT_EXIT", -1, -1)

		elif (self.isComment(eComment, "AI_DIPLOCOMMENT_TARGET")):
			for i in range(gc.getMAX_CIV_PLAYERS()):
				if (gc.getPlayer(i).isAlive()):
					#if (gc.getTeam(gc.getGame().getActiveTeam()).isAtWar(gc.getPlayer(i).getTeam())):
					if gc.getTeam(gc.getGame().getActiveTeam()).isAtWar(gc.getPlayer(i).getTeam()) and gc.getTeam(gc.getPlayer(self.diploScreen.getWhoTradingWith()).getTeam()).isAtWar(gc.getPlayer(i).getTeam()): # K-Mod
						player = PyPlayer(i)
						cityList = player.getCityList()
						for city in cityList:
							if (city.isRevealed(gc.getGame().getActiveTeam())):
								self.addUserComment("USER_DIPLOCOMMENT_TARGET_CITY", i, city.getID(), city.getNameKey())

			self.addUserComment("USER_DIPLOCOMMENT_SOMETHING_ELSE", -1, -1)
			self.addUserComment("USER_DIPLOCOMMENT_EXIT", -1, -1)

		# K-Mod
		elif (self.isComment(eComment, "AI_DIPLOCOMMENT_WARPLAN")):
			for i in range(gc.getMAX_CIV_TEAMS()):
				#if (gc.getTeam(i).isAlive() and i != gc.getPlayer(self.diploScreen.getWhoTradingWith()).getTeam() and gc.getTeam(gc.getGame().getActiveTeam()).isHasMet(i) and gc.getTeam(gc.getPlayer(self.diploScreen.getWhoTradingWith()).getTeam()).isHasMet(i) and not gc.getTeam(i).isAtWar(gc.getGame().getActiveTeam()) and gc.getTeam(gc.getGame().getActiveTeam()).canEventuallyDeclareWar()):
				if (gc.getTeam(i).isAlive() and gc.getTeam(gc.getGame().getActiveTeam()).isHasMet(i) and gc.getTeam(gc.getPlayer(self.diploScreen.getWhoTradingWith()).getTeam()).isHasMet(i)
					and not gc.getTeam(i).isAtWar(gc.getGame().getActiveTeam()) and gc.getTeam(gc.getGame().getActiveTeam()).canEventuallyDeclareWar(i)):
						self.addUserComment("USER_DIPLOCOMMENT_WARPLAN_TARGET", i, WarPlanTypes.WARPLAN_PREPARING_TOTAL, gc.getTeam(i).getName())

			self.addUserComment("USER_DIPLOCOMMENT_SOMETHING_ELSE", -1, -1)
			self.addUserComment("USER_DIPLOCOMMENT_EXIT", -1, -1)
		# K-Mod end

		# The default...
		else:

			if (gc.getPlayer(gc.getGame().getActivePlayer()).canTradeWith(self.diploScreen.getWhoTradingWith())):
				# Allow us to begin another proposal
				self.addUserComment("USER_DIPLOCOMMENT_PROPOSAL", -1, -1)

			if (self.isComment(eComment, "AI_DIPLOCOMMENT_GREETINGS") or
					self.isComment(eComment, "AI_DIPLOCOMMENT_UNIT_BRAG") or
					self.isComment(eComment, "AI_DIPLOCOMMENT_NUKES") or
					self.isComment(eComment, "AI_DIPLOCOMMENT_WORST_ENEMY") or 
					self.isComment(eComment, "AI_DIPLOCOMMENT_WORST_ENEMY_TRADING")):
				# If we are at war, allow to suggest peace
				if (self.diploScreen.atWar()):
					self.addUserComment("USER_DIPLOCOMMENT_SUGGEST_PEACE", -1, -1)

			# If we have a current deal, allow us to see the deals
			if (self.diploScreen.hasAnnualDeal()):
				self.addUserComment("USER_DIPLOCOMMENT_CURRENT_DEALS", -1, -1)

			self.addUserComment("USER_DIPLOCOMMENT_SOMETHING_ELSE", -1, -1)

			# Exit potential
			self.addUserComment("USER_DIPLOCOMMENT_EXIT", -1, -1)
			self.diploScreen.endTrade();

	def addUserComment(self, eComment, iData1, iData2, *args):
		" Helper for adding User Comments "
		iComment = self.getCommentID( eComment )
		self.diploScreen.addUserComment( iComment, iData1, iData2, self.getDiplomacyComment(iComment), args)
		
	def setAIComment (self, eComment, *args):
		" Handles the determining the AI comments"
		AIString = self.getDiplomacyComment(eComment)

		if DebugLogging:
			print "CvDiplomacy.setAIComment: %s" %(eComment,)
			if (len(args)):
				print "args", args
			AIString = "(%d) - %s" %(self.getLastResponseID(), AIString)
		
		self.diploScreen.setAIString(AIString, args)
		self.diploScreen.setAIComment(eComment)
		self.determineResponses(eComment)
		self.performHeadAction(eComment)

	def performHeadAction( self, eComment ):
	
		if ( eComment == self.getCommentID("AI_DIPLOCOMMENT_NO_PEACE") or
		     eComment == self.getCommentID("AI_DIPLOCOMMENT_REJECT") or
		     eComment == self.getCommentID("AI_DIPLOCOMMENT_NO_DEAL") or
		     eComment == self.getCommentID("AI_DIPLOCOMMENT_CANCEL_DEAL") or
		     eComment == self.getCommentID("AI_DIPLOCOMMENT_REJECT_ASK") or
			 eComment == self.getCommentID("AI_DIPLOCOMMENT_WORST_ENEMY_TRADING") or # K-Mod. (not really "disagreeing" so much as expressing discouragement)
		     eComment == self.getCommentID("AI_DIPLOCOMMENT_REJECT_DEMAND") ):
			self.diploScreen.performHeadAction( LeaderheadAction.LEADERANIM_DISAGREE ) 
		elif ( eComment == self.getCommentID("AI_DIPLOCOMMENT_ACCEPT") or
		       eComment == self.getCommentID("AI_DIPLOCOMMENT_TRY_THIS_DEAL") ):
			self.diploScreen.performHeadAction( LeaderheadAction.LEADERANIM_AGREE ) 
		elif ( eComment == self.getCommentID("AI_DIPLOCOMMENT_FIRST_CONTACT") or
		       eComment == self.getCommentID("AI_DIPLOCOMMENT_GREETINGS") or
		       eComment == self.getCommentID("AI_DIPLOCOMMENT_WORST_ENEMY") or
		       eComment == self.getCommentID("AI_DIPLOCOMMENT_UNIT_BRAG") or
		       eComment == self.getCommentID("AI_DIPLOCOMMENT_NUKES") or
		       eComment == self.getCommentID("AI_DIPLOCOMMENT_OFFER_PEACE") or
		       eComment == self.getCommentID("AI_DIPLOCOMMENT_OFFER_VASSAL") or
		       eComment == self.getCommentID("AI_DIPLOCOMMENT_OFFER_CITY") or
		       eComment == self.getCommentID("AI_DIPLOCOMMENT_OFFER_DEAL") or
		       eComment == self.getCommentID("AI_DIPLOCOMMENT_GIVE_HELP") or
		       eComment == self.getCommentID("AI_DIPLOCOMMENT_RELIGION_PRESSURE") or
		       eComment == self.getCommentID("AI_DIPLOCOMMENT_CIVIC_PRESSURE") or
		       eComment == self.getCommentID("AI_DIPLOCOMMENT_JOIN_WAR") or
		       eComment == self.getCommentID("AI_DIPLOCOMMENT_STOP_TRADING") or
		       eComment == self.getCommentID("AI_DIPLOCOMMENT_ASK_FOR_HELP") or
		       eComment == self.getCommentID("AI_DIPLOCOMMENT_DEMAND_TRIBUTE") ):
			self.diploScreen.performHeadAction( LeaderheadAction.LEADERANIM_GREETING ) 
		elif ( eComment == self.getCommentID("AI_DIPLOCOMMENT_ATTITUDE_PLAYER_FURIOUS") or
		       #eComment == self.getCommentID("AI_DIPLOCOMMENT_WORST_ENEMY_TRADING") or # (moved by K-mod)
		       eComment == self.getCommentID("AI_DIPLOCOMMENT_HELP_REFUSED") or
		       eComment == self.getCommentID("AI_DIPLOCOMMENT_DEMAND_REJECTED") or
		       eComment == self.getCommentID("AI_DIPLOCOMMENT_RELIGION_DENIED") or
		       eComment == self.getCommentID("AI_DIPLOCOMMENT_CIVIC_DENIED") or
		       eComment == self.getCommentID("AI_DIPLOCOMMENT_JOIN_DENIED") or
		       eComment == self.getCommentID("AI_DIPLOCOMMENT_STOP_DENIED") ):
			self.diploScreen.performHeadAction( LeaderheadAction.LEADERANIM_FURIOUS ) 
		elif ( eComment == self.getCommentID("AI_DIPLOCOMMENT_ATTITUDE_PLAYER_ANNOYED") ):
			self.diploScreen.performHeadAction( LeaderheadAction.LEADERANIM_ANNOYED ) 
		elif ( eComment == self.getCommentID("AI_DIPLOCOMMENT_ATTITUDE_PLAYER_CAUTIOUS") ):
			self.diploScreen.performHeadAction( LeaderheadAction.LEADERANIM_CAUTIOUS ) 
		elif ( eComment == self.getCommentID("AI_DIPLOCOMMENT_ATTITUDE_PLAYER_PLEASED") or
		       eComment == self.getCommentID("AI_DIPLOCOMMENT_SORRY") ):
			self.diploScreen.performHeadAction( LeaderheadAction.LEADERANIM_PLEASED ) 
		elif ( eComment == self.getCommentID("AI_DIPLOCOMMENT_ATTITUDE_PLAYER_FRIENDLY") or
		       eComment == self.getCommentID("AI_DIPLOCOMMENT_GLAD") or
		       eComment == self.getCommentID("AI_DIPLOCOMMENT_THANKS") ):
			self.diploScreen.performHeadAction( LeaderheadAction.LEADERANIM_FRIENDLY ) 
	
		return

	def getDiplomacyComment (self, eComment):
		"Function to get the user String"
		debugString = "CvDiplomacy.getDiplomacyComment: %s" %(eComment,)
		eComment = int(eComment)
		if DebugLogging:
			print debugString, eComment
		
		szString = ""
		szFailString = "Error***: No string found for eComment: %s"
		
		if ( gc.getDiplomacyInfo(eComment) ):
			DiplomacyTextInfo = gc.getDiplomacyInfo(eComment)
			if ( not DiplomacyTextInfo ):
				print "%s IS AN INVALID DIPLOCOMMENT" %(eComment,)
				CvUtil.pyAssert(True, "CvDiplomacy.getDiplomacyComment: %s does not have a DiplomacyTextInfo" %(eComment,))
				return szFailString %(eComment,)
			
			szString = self.filterUserResponse(DiplomacyTextInfo)
			
		else:
			szString = szFailString %(eComment,)
		
		return szString
	
	def setLastResponseID(self, iResponse):
		self.iLastResponseID = iResponse
	
	def getLastResponseID(self):
		return self.iLastResponseID

	def isUsed(self, var, i, num):
		"returns true if any element in the var list is true"
		for j in range(num):
			if (var(i, j)):
				return true
		return false
		
	def filterUserResponse(self, diploInfo):
		"pick the user's response from a CvDiplomacyTextInfo, based on response conditions"
		if (self.diploScreen.getWhoTradingWith() == -1):
			return ""
			
		theirPlayer = gc.getPlayer(self.diploScreen.getWhoTradingWith())
		ourPlayer = gc.getActivePlayer()
		responses = []
		
		for i in range(diploInfo.getNumResponses()):	
			
			# check attitude of other player towards me
			if (self.isUsed(diploInfo.getAttitudeTypes, i, AttitudeTypes.NUM_ATTITUDE_TYPES)):
				att = theirPlayer.AI_getAttitude(CyGame().getActivePlayer())
				if (not diploInfo.getAttitudeTypes(i, att)):
					continue
			
			# check civ type
			if (self.isUsed(diploInfo.getCivilizationTypes, i, gc.getNumCivilizationInfos()) and
				not diploInfo.getCivilizationTypes(i, theirPlayer.getCivilizationType())):
				continue
				
			# check leader type
			if (self.isUsed(diploInfo.getLeaderHeadTypes, i, gc.getNumLeaderHeadInfos()) and
				not diploInfo.getLeaderHeadTypes(i, theirPlayer.getLeaderType())):
				continue

			# check power type
			if (self.isUsed(diploInfo.getDiplomacyPowerTypes, i, DiplomacyPowerTypes.NUM_DIPLOMACYPOWER_TYPES)):
				theirPower = theirPlayer.getPower()
				ourPower = ourPlayer.getPower()
				
				if (ourPower < (theirPower / 2)):
					if not diploInfo.getDiplomacyPowerTypes(i, DiplomacyPowerTypes.DIPLOMACYPOWER_STRONGER):
						continue
						
				elif (ourPower > (theirPower * 2)):
					if not diploInfo.getDiplomacyPowerTypes(i, DiplomacyPowerTypes.DIPLOMACYPOWER_WEAKER):
						continue
						
				else:
					if not diploInfo.getDiplomacyPowerTypes(i, DiplomacyPowerTypes.DIPLOMACYPOWER_EQUAL):
						continue
				
			# passed all tests, so add to response list
			for j in range(diploInfo.getNumDiplomacyText(i)):
				responses.append(diploInfo.getDiplomacyText(i, j))
					
		# pick a random response
		numResponses = len(responses)
		if (numResponses>0):
			iResponse = gc.getASyncRand().get(numResponses, "Python Diplomacy ASYNC")
			self.setLastResponseID(iResponse)
			return responses[iResponse]
		
		return ""	# no responses matched
			
	def handleUserResponse(self, eComment, iData1, iData2):
		if DebugLogging:
			print "CvDiplomacy.handleUserResponse: %s" %(eComment,)
			
		diploScreen = CyDiplomacy()

		# If we accept peace
		if (self.isComment(eComment, "USER_DIPLOCOMMENT_PEACE")):
			self.setAIComment(self.getCommentID("AI_DIPLOCOMMENT_PEACE"))
	
		# If we choose war
		elif (self.isComment(eComment, "USER_DIPLOCOMMENT_WAR")):
			diploScreen.declareWar()
			#diploScreen.closeScreen()

		# If we wish to make a trade proposal or try to renegotiate
		elif (self.isComment(eComment, "USER_DIPLOCOMMENT_PROPOSAL") or
					self.isComment(eComment, "USER_DIPLOCOMMENT_RENEGOTIATE")):
			self.setAIComment(self.getCommentID("AI_DIPLOCOMMENT_TRADING"))
			diploScreen.showAllTrade(True)

		# If we want to propose a trade
		elif(self.isComment(eComment, "USER_DIPLOCOMMENT_PROPOSE")):
			if (diploScreen.offerDeal() == 1):
				self.setAIComment(self.getCommentID("AI_DIPLOCOMMENT_ACCEPT"))
			else:
				self.setAIComment(self.getCommentID("AI_DIPLOCOMMENT_REJECT"))
	
		# If we ask for peace
# K-Mod
		elif (self.isComment(eComment, "USER_DIPLOCOMMENT_SUGGEST_PEACE")):
			diploScreen.startTrade(eComment, False)
			if (diploScreen.counterPropose() == 1):
				self.setAIComment(self.getCommentID("AI_DIPLOCOMMENT_TRY_THIS_DEAL"))
			else:
				self.setAIComment(self.getCommentID("AI_DIPLOCOMMENT_NO_PEACE"))
# K-Mod end
# original bts code.
#		elif (self.isComment(eComment, "USER_DIPLOCOMMENT_SUGGEST_PEACE")):
#			if (diploScreen.offerDeal() == 1):
#				self.setAIComment(self.getCommentID("AI_DIPLOCOMMENT_PEACE"))
#			else:
#				self.setAIComment(self.getCommentID("AI_DIPLOCOMMENT_NO_PEACE"))
	
		# If we accept a trade
		elif (self.isComment(eComment, "USER_DIPLOCOMMENT_ACCEPT")):
			diploScreen.implementDeal()
			diploScreen.setAIOffer(0)
			self.setAIComment(self.getCommentID("AI_DIPLOCOMMENT_GLAD"))
	
		# If we reject a trade
		elif (self.isComment(eComment, "USER_DIPLOCOMMENT_REJECT")):
			diploScreen.setAIOffer(0)
			self.setAIComment(self.getCommentID("AI_DIPLOCOMMENT_SORRY"))

		# If we offer a deal, or is we are fishing for a deal, or if we are offering peace or fishing for peace
		elif (self.isComment(eComment, "USER_DIPLOCOMMENT_OFFER") or
					self.isComment(eComment, "USER_DIPLOCOMMENT_COMPLETE_DEAL") or
					self.isComment(eComment, "USER_DIPLOCOMMENT_FISH_FOR_DEAL") or
					self.isComment(eComment, "USER_DIPLOCOMMENT_OFFER_PEACE")):
			if (diploScreen.counterPropose() == 1):
				self.setAIComment(self.getCommentID("AI_DIPLOCOMMENT_TRY_THIS_DEAL"))
			else:
				self.setAIComment(self.getCommentID("AI_DIPLOCOMMENT_NO_DEAL"))

		# if we are asking for something
		# advc.155: Treat DEMAND_TEAM here as well
		elif (self.isComment(eComment, "USER_DIPLOCOMMENT_ASK") or self.isComment(eComment, "USER_DIPLOCOMMENT_DEMAND_TEAM")):
			bAccept = diploScreen.offerDeal() # advc.144: Put acceptance flag in iData1
			diploScreen.diploEvent(DiploEventTypes.DIPLOEVENT_ASK_HELP, bAccept, -1)
			if bAccept:
				self.setAIComment(self.getCommentID("AI_DIPLOCOMMENT_ACCEPT_ASK"))
			else:
				self.setAIComment(self.getCommentID("AI_DIPLOCOMMENT_REJECT_ASK"))

		# if we are demanding something
		elif (self.isComment(eComment, "USER_DIPLOCOMMENT_DEMAND")):
			bAccept = diploScreen.offerDeal() # advc.144: Put acceptance flag in iData1
			diploScreen.diploEvent(DiploEventTypes.DIPLOEVENT_MADE_DEMAND, bAccept, -1)
			if bAccept:
				self.setAIComment(self.getCommentID("AI_DIPLOCOMMENT_ACCEPT_DEMAND"))
			else:
				self.setAIComment(self.getCommentID("AI_DIPLOCOMMENT_REJECT_DEMAND"))

		# if we are demanding something with the threat of war
		elif (self.isComment(eComment, "USER_DIPLOCOMMENT_VASSAL_TRIBUTE")):
			diploScreen.diploEvent(DiploEventTypes.DIPLOEVENT_MADE_DEMAND_VASSAL, -1, -1)
			if (diploScreen.offerDeal()):
				self.setAIComment(self.getCommentID("AI_DIPLOCOMMENT_ACCEPT_DEMAND"))
			else:
				self.setAIComment(self.getCommentID("AI_DIPLOCOMMENT_DECLARE_WAR"))
				diploScreen.diploEvent(DiploEventTypes.DIPLOEVENT_DEMAND_WAR, -1, -1)


		# if we are demanding something from our teammate
		# advc.155: Commented out
		#elif (self.isComment(eComment, "USER_DIPLOCOMMENT_DEMAND_TEAM")):
			#diploScreen.offerDeal()
			#self.setAIComment(self.getCommentID("AI_DIPLOCOMMENT_ACCEPT_DEMAND_TEAM"))

		# If we are giving a gift
		elif (self.isComment(eComment, "USER_DIPLOCOMMENT_GIFT")):
			diploScreen.offerDeal()
			self.setAIComment(self.getCommentID("AI_DIPLOCOMMENT_THANKS"))

		# If we decide to view current deals
		elif (self.isComment(eComment, "USER_DIPLOCOMMENT_CURRENT_DEALS")):
			self.setAIComment(self.getCommentID("AI_DIPLOCOMMENT_CURRENT_DEALS"))
			diploScreen.showAllTrade(False)

		# If we give help
		elif (self.isComment(eComment, "USER_DIPLOCOMMENT_GIVE_HELP")):
			diploScreen.diploEvent(DiploEventTypes.DIPLOEVENT_GIVE_HELP, -1, -1)
			diploScreen.implementDeal()
			diploScreen.setAIOffer(0)
			self.setAIComment(self.getCommentID("AI_DIPLOCOMMENT_THANKS"))

		# If we accept their demand
		elif (self.isComment(eComment, "USER_DIPLOCOMMENT_ACCEPT_DEMAND")):
			diploScreen.diploEvent(DiploEventTypes.DIPLOEVENT_ACCEPT_DEMAND, -1, -1)
			diploScreen.implementDeal()
			diploScreen.setAIOffer(0)
			self.setAIComment(self.getCommentID("AI_DIPLOCOMMENT_THANKS"))

		# If we accept the offer
		elif (self.isComment(eComment, "USER_DIPLOCOMMENT_ACCEPT_OFFER")):
			diploScreen.implementDeal()
			diploScreen.setAIOffer(0)
			self.setAIComment(self.getCommentID("AI_DIPLOCOMMENT_THANKS"))

		# If we refuse to help
		elif (self.isComment(eComment, "USER_DIPLOCOMMENT_REFUSE_HELP")):
			diploScreen.diploEvent(DiploEventTypes.DIPLOEVENT_REFUSED_HELP, -1, -1)
			self.setAIComment(self.getCommentID("AI_DIPLOCOMMENT_HELP_REFUSED"))

		# If we reject their demand
		elif (self.isComment(eComment, "USER_DIPLOCOMMENT_REJECT_DEMAND")):
			diploScreen.diploEvent(DiploEventTypes.DIPLOEVENT_REJECTED_DEMAND, -1, -1)
			
			if (gc.getPlayer(self.diploScreen.getWhoTradingWith()).AI_demandRebukedWar(gc.getGame().getActivePlayer())):
				self.setAIComment(self.getCommentID("AI_DIPLOCOMMENT_DECLARE_WAR"))
				diploScreen.diploEvent(DiploEventTypes.DIPLOEVENT_DEMAND_WAR, -1, -1)
			else:
				self.setAIComment(self.getCommentID("AI_DIPLOCOMMENT_DEMAND_REJECTED"))

		# If we convert to their state religion
		elif (self.isComment(eComment, "USER_DIPLOCOMMENT_CONVERT")):
			diploScreen.diploEvent(DiploEventTypes.DIPLOEVENT_CONVERT, -1, -1)
			self.setAIComment(self.getCommentID("AI_DIPLOCOMMENT_THANKS"))

		# If we refuse to convert to their state religion
		elif (self.isComment(eComment, "USER_DIPLOCOMMENT_NO_CONVERT")):
			diploScreen.diploEvent(DiploEventTypes.DIPLOEVENT_NO_CONVERT, -1, -1)
			self.setAIComment(self.getCommentID("AI_DIPLOCOMMENT_RELIGION_DENIED"))

		# If we adopt their favorite civic
		elif (self.isComment(eComment, "USER_DIPLOCOMMENT_REVOLUTION")):
			diploScreen.diploEvent(DiploEventTypes.DIPLOEVENT_REVOLUTION, -1, -1)
			self.setAIComment(self.getCommentID("AI_DIPLOCOMMENT_THANKS"))

		# If we refuse to adopt their favorite civic
		elif (self.isComment(eComment, "USER_DIPLOCOMMENT_NO_REVOLUTION")):
			diploScreen.diploEvent(DiploEventTypes.DIPLOEVENT_NO_REVOLUTION, -1, -1)
			self.setAIComment(self.getCommentID("AI_DIPLOCOMMENT_CIVIC_DENIED"))

		# If we join their war
		elif (self.isComment(eComment, "USER_DIPLOCOMMENT_JOIN_WAR")):
			diploScreen.diploEvent(DiploEventTypes.DIPLOEVENT_JOIN_WAR, diploScreen.getData(), -1)
			self.setAIComment(self.getCommentID("AI_DIPLOCOMMENT_THANKS"))

		# If we refuse to join their war
		elif (self.isComment(eComment, "USER_DIPLOCOMMENT_NO_JOIN_WAR")):
			diploScreen.diploEvent(DiploEventTypes.DIPLOEVENT_NO_JOIN_WAR, -1, -1)
			self.setAIComment(self.getCommentID("AI_DIPLOCOMMENT_JOIN_DENIED"))

		# If we stop the trading
		elif (self.isComment(eComment, "USER_DIPLOCOMMENT_STOP_TRADING")):
			diploScreen.diploEvent(DiploEventTypes.DIPLOEVENT_STOP_TRADING, diploScreen.getData(), -1)
			self.setAIComment(self.getCommentID("AI_DIPLOCOMMENT_THANKS"))

		# If we refuse to stop the trading
		elif (self.isComment(eComment, "USER_DIPLOCOMMENT_NO_STOP_TRADING")):
			diploScreen.diploEvent(DiploEventTypes.DIPLOEVENT_NO_STOP_TRADING, -1, -1)
			self.setAIComment(self.getCommentID("AI_DIPLOCOMMENT_STOP_DENIED"))

		# If we want to go back to first screen
		elif (self.isComment(eComment, "USER_DIPLOCOMMENT_NEVERMIND")):
			self.setAIComment(self.getCommentID("AI_DIPLOCOMMENT_WELL"))

		# If we want to discuss something else
		elif (self.isComment(eComment, "USER_DIPLOCOMMENT_SOMETHING_ELSE")):
			self.setAIComment(self.getCommentID("AI_DIPLOCOMMENT_SOMETHING_ELSE"))

		# If we want to ask them to change their research
		elif (self.isComment(eComment, "USER_DIPLOCOMMENT_RESEARCH")):
			self.setAIComment(self.getCommentID("AI_DIPLOCOMMENT_RESEARCH"))

		# If we want to ask them to change their research to a specific tech
		elif (self.isComment(eComment, "USER_DIPLOCOMMENT_RESEARCH_TECH")):
			diploScreen.diploEvent(DiploEventTypes.DIPLOEVENT_RESEARCH_TECH, iData1, -1)
			self.setAIComment(self.getCommentID("AI_DIPLOCOMMENT_RESEARCH_TECH"))

		# If we want to ask them to what their attitude is
		elif (self.isComment(eComment, "USER_DIPLOCOMMENT_ATTITUDE")):
			self.setAIComment(self.getCommentID("AI_DIPLOCOMMENT_ATTITUDE"))

		# If we want to ask them to what their attitude is on a specific player
		elif (self.isComment(eComment, "USER_DIPLOCOMMENT_ATTITUDE_PLAYER")):
			eAttitude = gc.getPlayer(self.diploScreen.getWhoTradingWith()).AI_getAttitude(iData1)
			
			if (eAttitude == AttitudeTypes.ATTITUDE_FURIOUS):
				self.setAIComment(self.getCommentID("AI_DIPLOCOMMENT_ATTITUDE_PLAYER_FURIOUS"), gc.getPlayer(iData1).getNameKey())
			elif (eAttitude == AttitudeTypes.ATTITUDE_ANNOYED):
				self.setAIComment(self.getCommentID("AI_DIPLOCOMMENT_ATTITUDE_PLAYER_ANNOYED"), gc.getPlayer(iData1).getNameKey())
			elif (eAttitude == AttitudeTypes.ATTITUDE_CAUTIOUS):
				self.setAIComment(self.getCommentID("AI_DIPLOCOMMENT_ATTITUDE_PLAYER_CAUTIOUS"), gc.getPlayer(iData1).getNameKey())
			elif (eAttitude == AttitudeTypes.ATTITUDE_PLEASED):
				self.setAIComment(self.getCommentID("AI_DIPLOCOMMENT_ATTITUDE_PLAYER_PLEASED"), gc.getPlayer(iData1).getNameKey())
			else:
				self.setAIComment(self.getCommentID("AI_DIPLOCOMMENT_ATTITUDE_PLAYER_FRIENDLY"), gc.getPlayer(iData1).getNameKey())

		# If we want to ask them to change their target
		elif (self.isComment(eComment, "USER_DIPLOCOMMENT_TARGET")):
			self.setAIComment(self.getCommentID("AI_DIPLOCOMMENT_TARGET"))

		# If we want to ask them to change their target to a specific city
		elif (self.isComment(eComment, "USER_DIPLOCOMMENT_TARGET_CITY")):
			diploScreen.diploEvent(DiploEventTypes.DIPLOEVENT_TARGET_CITY, iData1, iData2)
			self.setAIComment(self.getCommentID("AI_DIPLOCOMMENT_TARGET_CITY"))

		# K-Mod
		elif (self.isComment(eComment, "USER_DIPLOCOMMENT_WARPLAN")):
			self.setAIComment(self.getCommentID("AI_DIPLOCOMMENT_WARPLAN"))

		elif (self.isComment(eComment, "USER_DIPLOCOMMENT_WARPLAN_TARGET")):
			diploScreen.diploEvent(DiploEventTypes.DIPLOEVENT_SET_WARPLAN, iData1, iData2)
			self.setAIComment(self.getCommentID("AI_DIPLOCOMMENT_WARPLAN_ACKNOWLEDGE"))

		elif (self.isComment(eComment, "USER_DIPLOCOMMENT_WARPLAN_CANCEL")):
			diploScreen.diploEvent(DiploEventTypes.DIPLOEVENT_SET_WARPLAN, TeamTypes.NO_TEAM, WarPlanTypes.NO_WARPLAN)
			self.setAIComment(self.getCommentID("AI_DIPLOCOMMENT_WARPLAN_CANCEL"))
		# K-Mod end

		else:
			diploScreen.closeScreen()

	def dealCanceled( self ):
	
		self.setAIComment(self.getCommentID("AI_DIPLOCOMMENT_TRADING"))
		
		return

	def isComment(self, eComment, strComment):
		'bool - comment matching'
		if ( gc.getDiplomacyInfo(eComment).getType() == strComment ):
			return True
		return False
	
	def getCommentID(self, strComment):
		'int - ID for DiploCommentType'
		for i in range(gc.getNumDiplomacyInfos()):
			if ( gc.getDiplomacyInfo(i).getType() == strComment ):
				return i
		return -1
