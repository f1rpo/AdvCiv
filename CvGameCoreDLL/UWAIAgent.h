#pragma once

#ifndef UWAI_AGENT_H
#define UWAI_AGENT_H

// advc.104: See comment at the start of UWAI.h

#include "UWAI.h"
#include "UWAICache.h"

class FDataStreamBase;
class UWAIReport;

// This class handles war and peace at the level of CvTeams
class UWAI::Team {
public:
	Team();
	~Team();
	// See UWAICache.h about the call order during initialization
	// (Tbd.: Make the UWAI classes non-reusable, i.e. initialize in the ctor.)
	void init(TeamTypes agentId);
	void turnPre();
	void doWar(); // replacement for CvTeamAI::doWar
	void read(FDataStreamBase* stream);
	void write(FDataStreamBase* stream);
	// Replacing parts of CvTeamAI::AI_declareWarTrade
	DenialTypes declareWarTrade(TeamTypes targetId, TeamTypes sponsorId) const;
	/*  Replacing CvTeamAI::AI_declareWarTradeVal. However, that function is
		called on the sponsor, whereas this one is called on the team that gets
		payed for war (which is also the case for declareWarTrade and
		CvTeamAI::AI_declareWarTrade). */
	int declareWarTradeVal(TeamTypes targetId, TeamTypes sponsorId) const;
	// Replacing parts of CvTeamAI::AI_makePeaceTrade
	DenialTypes makePeaceTrade(TeamTypes enemyId, TeamTypes brokerId) const;
	/*  Replacing CvTeamAI::AI_makePeaceTradeVal. However, that function is
		called on the broker, whereas this one is called on the team that gets
		payed for peace (which is also the case for makePeaceTrade and
		CvTeamAI::AI_makePeaceTrade). */
	int makePeaceTradeVal(TeamTypes enemyId, TeamTypes brokerId) const;
	/*  Replacing some calls to CvTeamAI::AI_endWarVal.
		How much we value ending the war with enemyId. */
	int endWarVal(TeamTypes enemyId) const;
	/*  Utility of ending all our wars (through diplo vote).
		Positive if we want to end the war.
		If vs is set, only wars with members of the VoteSource are considered. */
	int uEndAllWars(VoteSourceTypes vs = NO_VOTESOURCE) const;
	// Utility of ending the war against enemyId
	int uEndWar(TeamTypes enemyId) const;
	// Joint war through a diplo vote. We mustn't be at war yet.
	int uJointWar(TeamTypes targetId, VoteSourceTypes vs) const;
	/*  If ourTeam is at war with targetId, this computes and returns our
		utility of convincing allyId to join the war against targetId
		(compared with a scenario where we continue the war by ourselves).
		Otherwise, it's our utility of a joint war together with allyId
		against targetId (compared with a scenario where neither
		we nor allyId declare war on targetId). */
	int uJointWar(TeamTypes targetId, TeamTypes allyId) const;
	// How much we're willing to pay to allyId for declaring war on targetd
	int tradeValJointWar(TeamTypes targetId, TeamTypes allyId) const;
	/*  Based on war utility and the utility threshold for peace.
		At least 0 unless nonNegative=false, in which case a negative
		return value indicates a willingness for peace. */
	int reluctanceToPeace(TeamTypes otherId, bool nonNegative = true) const;
	/*  Trade value that we're willing to pay to any human civ for peace
		if peace has a utility of u for us. */
	double reparationsToHuman(double u) const;
	void respondToRebuke(TeamTypes targetId, bool prepare);
	/*  Taking over parts of CvTeamAI::AI_vassalTrade; only called when
		accepting vassalId puts us (the would-be master) into a war. */
	DenialTypes acceptVassal(TeamTypes vassalId) const;
	bool isLandTarget(TeamTypes theyId) const;

	/*  The remaining functions are only to be called while doWar
		is being evaluated. */
	// Known to be at war with anyone. If NO_TEAM, all wars are checked.
	bool isKnownToBeAtWar(TeamTypes observer = NO_TEAM) const;
	/*  Also checks vassal agreements, including other vassals of
		the same master. */
	bool hasDefactoDefensivePact(TeamTypes allyId) const;
	/*  Whether this team can reach any city of 'targetId' with military units;
		based on cached info. */
	bool canReach(TeamTypes targetId) const;
	/* Confidence based on experience in the current war with targetId.
		Between 0.5 (low confidence) and 1.5 (high confidence); negative
		if not at war. */
	double confidenceFromWarSuccess(TeamTypes targetId) const;
	void reportWarEnding(TeamTypes enemyId, CLinkList<TradeData> const* weReceive = NULL,
			CLinkList<TradeData> const* wePay = NULL);
	int countNonMembers(VoteSourceTypes voteSource) const;
	// Like canSchemeAgainst, but also true if currently at war (unless vassal).
	bool isPotentialWarEnemy(TeamTypes tId) const;
	bool isFastRoads() const;
	UWAI::Civ const& leaderUWAI() const;
	UWAI::Civ& leaderUWAI();
	// When forming a Permanent Alliance
	void addTeam(PlayerTypes otherLeaderId);
	double utilityToTradeVal(double u) const;
	/*  tradeVal should roughly correspond to gold per turn; converted into
		war utility based on our current commerce rate. */
	double tradeValToUtility(double tradeVal) const;
	/*  Runs 'scheme' as if UWAI was running in the background and writes a
		report file regardless of the REPORT_INTERVAL set in XML.
		Intended for debugging. Could insert a call like
		GET_TEAM((TeamTypes)1).uwai().doWarReport()
		in e.g. CvUnit::kill, load the savegame to be debugged, disband a unit,
		read the report and remove the doWarReport call again. */
	void doWarReport();

private:
	void reset();
	/*  Abandon wars in preparation, switch plans or targets and consider peace.
		Returns false if scheming should be skipped this turn. */
	bool reviewWarPlans();
	/*  Review plan vs. targetId. Returns true if plan continues unchanged,
		false if any change (abandoned, peace made, target changed). */
	bool reviewPlan(TeamTypes targetId, int u, int prepTime);
	void alignAreaAI(bool isNaval);
	int peaceThreshold(TeamTypes targetId) const;
	// All these return true if the war plan remains unchanged, false otherwise
	 bool considerPeace(TeamTypes targetId, int u);
	 bool considerCapitulation(TeamTypes masterId, int ourWarUtility,
			int masterReluctancePeace);
	 bool tryFindingMaster(TeamTypes enemyId);
	 bool considerPlanTypeChange(TeamTypes targetId, int u);
	 bool considerAbandonPreparations(TeamTypes targetId, int u, int timeRemaining);
	 bool considerSwitchTarget(TeamTypes targetId, int u, int timeRemaining);
	 bool considerConcludePreparations(TeamTypes targetId, int u, int timeRemaining);
	void scheme(); // Consider new war plans
	bool canSchemeAgainst(TeamTypes targetId, bool assumeNoWarPlan) const;
	double limitedWarWeight() const;
	void startReport();
	void closeReport();
	void setForceReport(bool b);
	bool isReportTurn() const;
	void showWarPrepStartedMsg(TeamTypes targetId);
	void showWarPlanAbandonedMsg(TeamTypes targetId);
	void showWarPlanMsg(TeamTypes targetId, char const* txtKey);
	UWAICache& leaderCache();
	UWAICache const& leaderCache() const;
	/*  Not in UWAI::Civ b/c I want these to be private. They're
		only auxiliary functions for their team-level counterparts, and should
		not be used for any other computations.
		Instead, I'm placing conversion functions in UWAI::Civ
		that simply call the team versions. */
	  double utilityToTradeVal(double u, PlayerTypes memberId) const;
	  double tradeValToUtility(double tradeVal, PlayerTypes memberId) const;

	TeamTypes agentId;
	bool inBackgr;
	bool bForceReport;
	UWAIReport* report; // Only to be used in doWar and its subroutines
};

// This class handles war and peace on the level of CvPlayers
class UWAI::Civ {

public:
	Civ();
	// See UWAICache.h about when init is called.
	void init(PlayerTypes we);
	void uninit();
	void turnPre();
	// 'cache' handles all the persistent data, these two only relay the calls.
	 void write(FDataStreamBase* stream);
	 void read(FDataStreamBase* stream);
	inline UWAICache const& getCache() const { return cache; }
	inline UWAICache& getCache() { return cache; }
	// Request and demands. BtS handles these in CvPlayerAI::AI_considerOffer.
	bool considerDemand(PlayerTypes theyId, int tradeVal) const;
	bool considerGiftRequest(PlayerTypes theyId, int tradeVal) const;
	bool amendTensions(PlayerTypes humanId) const;
	/*  Returns -1 if unwilling, 1 if willing and 0 to leave the decision to the
		BtS AI */
	int willTalk(PlayerTypes theyId, int atWarCounter, bool useCache) const;
	// False if all assets of the human civ wouldn't nearly be enough
	bool isPeaceDealPossible(PlayerTypes humanId) const;
	/*  Can humanId trade assets to us with a total value of at least
		targetTradeVal? */
	bool canTradeAssets(int targetTradeVal, PlayerTypes humanId,
			/*  If this is not NULL, it is used to return the trade value of
				all assets that the human can trade, but only up to
				targetTradeVal. */
			int* r = NULL, bool ignoreCities = false) const;
	double amortizationMultiplier() const;
	bool isNearMilitaryVictory(int stage) const;
	int getConquestStage() const;
	int getDominationStage() const;
	/*  This function isn't specific to a given civ. Should perhaps
		be in a wrapper/ subclass of CvUnitInfo. Leaving it here for now.
		At least it's easily accessible this way.
		If a 'baseValue' is given, that value replaces the power value defined in Unit.xml. */
	double militaryPower(CvUnitInfo const& u, double baseValue = -1) const;
	// Can this civ hurry production somehow? (Slavery, Univ. Suffrage)
	bool canHurry() const;
	double buildUnitProb() const;
	int shipSpeed() const;
	/*  period: Build-up over how many turns? Will be adjusted to game speed
		by this function! */
	double estimateBuildUpRate(PlayerTypes civId, int period = 10) const;
	double tradeValUtilityConversionRate() const;
	double utilityToTradeVal(double u) const;
	double tradeValToUtility(double tradeVal) const;
	/*  Confidence based on experience from past wars with targetId.
		1 if none, otherwise between 0.5 and 1.5. */
	double confidenceFromPastWars(TeamTypes targetId) const;
  // Personality values that aren't cached. More in UWAICache.
	/*  A measure of how paranoid our leader is, based on EspionageWeight and
		protective trait. EspionageWeight is between 50 (Gandhi) and 150 (Stalin).
		Return value is between 0.5 and 1.8.
		"Paranoia" would be a better name, but that already means sth. else
		(related to the Alert AI strategy). */
	double distrustRating() const;
	/*  A measure of optimism (above 1) or pessimism (between 0 and 1) of our
		leader about conducting war against 'vs'. (It's not clear if 'isTotal'
		should matter.) */
	double warConfidencePersonal(bool isNaval, bool isTotal, PlayerTypes vs) const;
	/*  Confidence based on experience in the current war or past wars.
		Between 0.5 (low confidence) and 1.5 (high confidence).
		ignoreDefOnly: Don't count factors that only make us confident about
		defending ourselves. */
	double warConfidenceLearned(PlayerTypes targetId, bool ignoreDefOnly) const;
	/*  How much our leader is (generally) willing to rely on war allies.
		0 to 1 means that the leader is rather self-reliant, above means
		he or she likes dogpile wars. */
	double warConfidenceAllies() const;
	double confidenceAgainstHuman() const;
	// How willing our leader is to go after civs he really dislikes
	int vengefulness() const;
	/*  Willingness to come to the aid of partners. "Interventionism" might
		also fit. Between 1.3 (Roosevelt) and 0.7 (Qin). */
	double protectiveInstinct() const;
	/*  Between 0.25 (Tokugawa) and 1.75 (Mansa Musa, Zara Yaqob). A measure
		of how much a leader cares about being generally liked in the world. */
	double diploWeight() const;
	/*  Between 0 (Pacal and several others) and 1 (Sitting Bull). How much a
		leader insists on reparations to end a war. Based on MakePeaceRand. */
	double prideRating() const;

private:
	// Probability assumed by the AI if this civ is human
	double humanBuildUnitProb() const;
	int willTalk(PlayerTypes theyId, int atWarCounter) const;

	PlayerTypes weId;
	UWAICache cache;
};

/*	advc.test: Disables the checks for DP, the PA prereq. tech and the game option;
	refusal based on attitude only if worse than Cautious.
	(Could define this in any header included by both CvTeamAI.cpp and CvPlayer.cpp.
	This one isn't included very frequently elsewhere, i.e. not too slow to recompile.) */
//#define TEST_PERMANENT_ALLIANCES

#endif
