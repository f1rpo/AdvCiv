#pragma once

#ifndef MILITARY_ANALYST_H
#define MILITARY_ANALYST_H

#include "WarEvalParameters.h"
#include "InvasionGraph.h"

class InvasionGraph;


/*  <advc.104>: New class. Handles the military assessment of a war between
	two teams, from the pov of a particular member of one of those teams.
	The pov civ is 'we', its team is the 'agent' team, and the enemy team is
	'they'.
	Considers the present situation, but, more so, the medium-term prospects
	(~25 turns). The computations happen in the constructor. The various results
	can then be accessed through (fairly) quick functions.
	Depending on the peaceScenario parameter, simulates a "war" scenario in which
	war against 'they' is assumed to be declared immediately, after some
	preparation time or continued (if already at war), - or - a "peace" scenario
	in which no war is declared or the war against 'they' is ended immediately
	if ongoing. */
class MilitaryAnalyst {

public:

	MilitaryAnalyst(PlayerTypes weId, WarEvalParameters& warEvalParams,
			bool peaceScenario);
	~MilitaryAnalyst();
	WarEvalParameters& evaluationParameters();
	PlayerTypes ourId() const;
	bool isOnOurSide(TeamTypes tId) const;
	/*  The parameter only refers to proper defensive pacts; vassal-master is always
		checked. */
	bool isOnTheirSide(TeamTypes tId, bool defensivePacts = false) const;
	bool isPeaceScenario() const;
	/*  City plot indexes. For lostCities, civId is the civ that loses the cities;
		for conqueredCities, civId is the civ that conquers the cities. */
	 std::set<int> const& lostCities(PlayerTypes civId) const;
	 std::set<int> const& conqueredCities(PlayerTypes civId) const;
	 std::set<TeamTypes> const& getCapitulationsAccepted(TeamTypes masterId) const;
	bool isEliminated(PlayerTypes civId) const;
	bool hasCapitulated(TeamTypes civId) const;
	// From units lost at war
	double lostPower(PlayerTypes civId, MilitaryBranchTypes mb) const;
    // Net gain, i.e. build-up minus losses; can be negative
	double gainedPower(PlayerTypes civId, MilitaryBranchTypes mb) const;
	// Production invested in military build-up
	double militaryProduction(PlayerTypes civId) const;
	/*  Only for the war scenario. No DoW are anticipated in the peace scenario;
		will return empty set. */
	std::set<PlayerTypes> const& getWarsDeclaredBy(PlayerTypes civId) const;
	std::set<PlayerTypes> const& getWarsDeclaredOn(PlayerTypes civId) const;
	// Based on conquered and lost cities
	double predictedGameScore(PlayerTypes civId) const;
	// Empty unless considering peace
	std::set<PlayerTypes> const& getWarsContinued(PlayerTypes civId) const;
	bool isWar(PlayerTypes c1, PlayerTypes c2) const; // By the end of the simulation
	bool isWar(TeamTypes t1, TeamTypes t2) const;
	// Prep. time, if any, plus time horizon
	int turnsSimulated() const;
	// Does civId have a node in the InvasionGraph?
	bool isPartOfAnalysis(PlayerTypes civId) const;
	// Fired by someone (shouldn't matter who); interception factored in
	double getNukesSufferedBy(PlayerTypes civId) const;
	// Fired on someone; interception not factored in
	double getNukesFiredBy(PlayerTypes civId) const;
	// Interception factored in
	double getNukedCities(PlayerTypes byId, PlayerTypes ownerId) const;
	// For debugging
	void logResults(PlayerTypes civId);

private:
	// True if we are preparing war against civId or considering a war plan
	bool doWePlanToDeclWar(PlayerTypes civId) const;
	void prepareResults();
	void simulateNuclearWar();
	double interceptionMultiplier(TeamTypes tId);
	// Logs either conquests or losses
	void logCities(PlayerTypes civId, bool conquests);
	// Logs either gained or lost power
	void logPower(PlayerTypes civId, bool gained);
	void logCapitulations(PlayerTypes civId);
	// Wars declared on and by (the team of) civId
	void logDoW(PlayerTypes civId);

	PlayerTypes weId;
	WarEvalParameters& warEvalParams;
	TeamTypes theyId;
	WarAndPeaceReport& report;
	bool peaceScenario;

	InvasionGraph* ig;
	int turnsSim;
	double gameScore[MAX_CIV_PLAYERS];
	double nukesSufferedBy[MAX_CIV_PLAYERS];
	double nukesFiredBy[MAX_CIV_PLAYERS];
	double nukedCities[MAX_CIV_PLAYERS][MAX_CIV_PLAYERS];
	std::set<PlayerTypes> partOfAnalysis;
	std::set<PlayerTypes>* DoWOn[MAX_CIV_PLAYERS];
	std::set<PlayerTypes>* DoWBy[MAX_CIV_PLAYERS];
	std::set<PlayerTypes>* warsCont[MAX_CIV_PLAYERS];
	 std::set<int>* lostCitiesPerCiv[MAX_CIV_PLAYERS];
	 std::set<int>* conqueredCitiesPerCiv[MAX_CIV_PLAYERS];
	 std::set<TeamTypes>* capitulationsAcceptedPerTeam[MAX_CIV_TEAMS];
	/*  Needed as return values (for queries about civs that weren't part of the
		simulation -- too tedious for the caller to test this each time) */
	std::set<int> emptyIntSet;
	std::set<TeamTypes> emptyTeamSet;
	std::set<PlayerTypes> emptyCivSet;
	bool isWarTable[MAX_CIV_PLAYERS][MAX_CIV_PLAYERS];
};

// </advc.104>

#endif
