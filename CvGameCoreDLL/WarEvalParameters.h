#pragma once

#ifndef WAR_EVAL_PARAMETERS_H
#define WAR_EVAL_PARAMETERS_H

class WarUtilityAspect;
class UWAIReport;

/* advc.104: New class. Parameters that enter into the computation of
   war utility. On the level of teams (not individual civs).
   These parameters need to be passed around a bit, ultimately to the
   objects that evaluate individual war utility aspects; therefore encapsulated
   in a class. */
class WarEvalParameters {

public:

	WarEvalParameters(TeamTypes agentId, TeamTypes targetId,
			UWAIReport& report,
			bool ignoreDistraction = false,
			PlayerTypes sponsor = NO_PLAYER,
			TeamTypes capitulationTeam = NO_TEAM);
	TeamTypes agentId() const;
	TeamTypes targetId() const;
	UWAIReport& getReport() const;
	bool isConsideringPeace() const;
	/*  For evaluating joint wars when the agent is already at war, but the ally
		is not. The agent does then not consider itself (and its vassals) to be at
		peace with the target in the peace scenario. */
	void setNotConsideringPeace();
	bool isIgnoreDistraction() const;
	// For joint wars
	void addWarAlly(TeamTypes tId);
	bool isWarAlly(TeamTypes tId) const;
	bool isAnyWarAlly() const;
	// For peace votes (peace will be assumed in the peace scenario)
	void addExtraTarget(TeamTypes tId);
	bool isExtraTarget(TeamTypes tId) const;
	/*  If this returns true, then, in the peace scenario, war should be assumed
		vs. the main target and peace with the extra targets. Will return true
		only if the ConsideringPeace flag is not set. */
	bool isNoWarVsExtra() const;
	void setSponsor(PlayerTypes civId); // Alternative to constructor arg
	PlayerTypes getSponsor() const;
	bool isTotal() const;
	bool isNaval() const;
	int getPreparationTime() const;
	// Set to true automatically when a sponsor is set
	void setImmediateDoW(bool b);
	bool isImmediateDoW() const;
	/*	Team we're considering to capitulate to (would-be master);
		NO_TEAM if we're not considering to capitulate. */
	TeamTypes getCapitulationTeam() const;
	// For WarEvaluator cache
	int id() const;
	// To be filled in by WarEvaluator
	  void setTotal(bool b);
	  void setNaval(bool b);
	  void setPreparationTime(int t);

private:

	UWAIReport& report;
	TeamTypes _targetId, _agentId;
	bool consideringPeace;
	bool ignoreDistraction;
	std::set<TeamTypes> warAllies;
	std::set<TeamTypes> extraTargets;
	bool total, naval;
	int preparationTime;
	bool immediateDoW;
	PlayerTypes sponsor;
	TeamTypes capitulationTeam;
	// Data members added to this class will have to be factored into the id function!
};

#endif
