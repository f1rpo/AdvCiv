#pragma once

#ifndef MILITARY_ANALYST_H
#define MILITARY_ANALYST_H

#include "MilitaryBranch.h"
#include "UWAISets.h"

class InvasionGraph;
class WarEvalParameters;
class UWAIReport;
class InvasionGraph;


/*  advc.104: New class. Handles the military assessment of a war between
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
	WarEvalParameters& evaluationParameters() { return warEvalParams; }
	WarEvalParameters const& evaluationParameters() const { return warEvalParams; }
	PlayerTypes ourId() const { return weId; }
	bool isPeaceScenario() const { return peaceScenario; }
	// For debugging
	void logResults(PlayerTypes civId);
	bool isOnOurSide(TeamTypes tId) const;
	/*  The parameter only refers to proper defensive pacts; vassal-master is always
		checked. */
	bool isOnTheirSide(TeamTypes tId, bool defensivePacts = false) const;
	bool isEliminated(PlayerTypes civId) const;
	bool hasCapitulated(TeamTypes civId) const;
	// From units lost at war
	double lostPower(PlayerTypes civId, MilitaryBranchTypes mb) const;
	// Net gain, i.e. build-up minus losses; can be negative
	double gainedPower(PlayerTypes civId, MilitaryBranchTypes mb) const;
	// Production invested in military build-up
	double militaryProduction(PlayerTypes civId) const;
	TeamSet const& getCapitulationsAccepted(TeamTypes masterId) const {
		FAssertBounds(0, MAX_CIV_TEAMS, masterId);
		return capitulationsAcceptedPerTeam[masterId];
	}
	double getNukedCities(PlayerTypes byId, PlayerTypes ownerId) const {
		FAssertBounds(0, MAX_CIV_PLAYERS, byId);
		FAssertBounds(0, MAX_CIV_PLAYERS, ownerId);
		return nukedCities[byId][ownerId];
	}
	bool isWar(PlayerTypes p1, PlayerTypes p2) const { // By the end of the simulation
		FAssertBounds(0, MAX_CIV_PLAYERS, p1);
		FAssertBounds(0, MAX_CIV_PLAYERS, p2);
		return warTable[p1][p2];
	}
	// (there's another public section below)
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
	UWAIReport& report;
	bool peaceScenario;

	InvasionGraph* ig;
	int turnsSim;

	std::vector<TeamSet> capitulationsAcceptedPerTeam;
	PlyrSet partOfAnalysis;
	std::vector<std::vector<bool> > warTable;
	std::vector<std::vector<double> > nukedCities;

	class PlayerResult { // Per-player results of analysis
	public:
		PlayerResult() : gameScore(-1), nukesSuffered(0), nukesFired(0) {}
		void setGameScore(double score) { gameScore = score; }
		inline double getGameScore() const { return gameScore; }
		void addNukesSuffered(double nukes) { nukesSuffered += nukes; }
		inline double getNukesSuffered() const { return nukesSuffered; }
		void addNukesFired(double nukes) { nukesFired += nukes; }
		inline double getNukesFired() const { return nukesFired; }
		void setDoWOn(PlayerTypes aggressorId) { DoWOn.insert(aggressorId); }
		void setDoWOn(PlyrSetIter const& first, PlyrSetIter const& last) { DoWOn.insert(first, last); }
		inline PlyrSet const& getDoWOn() const { return DoWOn; }
		void setDoWBy(PlayerTypes targetId) { DoWBy.insert(targetId); }
		void setDoWBy(PlyrSetIter const& first, PlyrSetIter const& last) { DoWBy.insert(first, last); }
		inline PlyrSet const& getDoWBy() const { return DoWBy; }
		void setWarContinued(PlayerTypes enemyId) { warsCont.insert(enemyId); }
		inline PlyrSet const& getWarsContinued() const { return warsCont; }
		void setCityLost(int cityPlotNum) { lostCities.insert(cityPlotNum); }
		inline CitySet const& getLostCities() const { return lostCities; }
		inline CitySet& getLostCities() { return lostCities; }
		void setCityConquered(int cityPlotNum) { conqueredCities.insert(cityPlotNum); }
		inline CitySet const& getConqueredCities() const { return conqueredCities; }
		inline CitySet& getConqueredCities() { return conqueredCities; }
	private:
		double gameScore;
		double nukesSuffered;
		double nukesFired;
		PlyrSet DoWOn;
		PlyrSet DoWBy;
		PlyrSet warsCont;
		CitySet lostCities;
		CitySet conqueredCities;
	};
	std::vector<PlayerResult*> playerResults;
	static CitySet emptyCitySet;
	static PlyrSet emptyPlayerSet;
	PlayerResult& playerResult(PlayerTypes civId);

public:
	CitySet const& lostCities(PlayerTypes oldOwnerId) const {
		FAssertBounds(0, MAX_CIV_PLAYERS, oldOwnerId);
		PlayerResult* r = playerResults[oldOwnerId];
		return (r == NULL ? emptyCitySet : r->getLostCities());
	}
	CitySet const& conqueredCities(PlayerTypes newOwnerId) const {
		FAssertBounds(0, MAX_CIV_PLAYERS, newOwnerId);
		PlayerResult* r = playerResults[newOwnerId];
		return (r == NULL ? emptyCitySet : r->getConqueredCities());
	}
	double predictedGameScore(PlayerTypes civId) const; // Based on conquered and lost cities
	/*  Only for the war scenario. No DoW are anticipated in the peace scenario;
		will return false then. */
	PlyrSet const& getWarsDeclaredBy(PlayerTypes aggressorId) const {
		 FAssertBounds(0, MAX_CIV_PLAYERS, aggressorId);
		 PlayerResult* r = playerResults[aggressorId];
		 return (r == NULL ? emptyPlayerSet : r->getDoWBy());
	}
	PlyrSet const& getWarsDeclaredOn(PlayerTypes targetId) const {
		 FAssertBounds(0, MAX_CIV_PLAYERS, targetId);
		 PlayerResult* r = playerResults[targetId];
		 return (r == NULL ? emptyPlayerSet : r->getDoWOn());
	}
	// Empty unless considering peace
	PlyrSet const& getWarsContinued(PlayerTypes civId) const {
		FAssertBounds(0, MAX_CIV_PLAYERS, civId);
		PlayerResult* r = playerResults[civId];
		return (r == NULL ? emptyPlayerSet : r->getWarsContinued());
	}
	bool isWar(TeamTypes t1, TeamTypes t2) const;
	// Prep. time, if any, plus time horizon
	int turnsSimulated() const { return turnsSim; }
	// Does civId have a node in the InvasionGraph?
	bool isPartOfAnalysis(PlayerTypes civId) const {
		return (partOfAnalysis.count(civId) > 0);
	}
	// Only those that aren't expected to be intercepted. Fired by someone (shouldn't matter who).
	double getNukesSufferedBy(PlayerTypes civId) const {
		FAssertBounds(0, MAX_CIV_PLAYERS, civId);
		PlayerResult* r = playerResults[civId];
		return (r == NULL ? 0.0 : r->getNukesSuffered());
	}
	double getNukesFiredBy(PlayerTypes civId) const {
		FAssertBounds(0, MAX_CIV_PLAYERS, civId);
		PlayerResult* r = playerResults[civId];
		return (r == NULL ? 0.0 : r->getNukesFired());
	}
};

#endif
