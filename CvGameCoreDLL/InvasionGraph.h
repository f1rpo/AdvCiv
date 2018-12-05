#pragma once

#ifndef INVASION_GRAPH_H
#define INVASION_GRAPH_H

#include "MilitaryAnalyst.h"
#include "MilitaryBranch.h"
#include <set>

class MilitaryAnalyst;
class SimulationStep;

/* <advc.104>: New class. The invasion graph says who tries to invade whom.
   It's an analysis from the perspective of one civ ("we", the owner of
   the associated MilitaryAnalyst) planning war (WarAndPeaceAI::Team::doWar).
   There's a node in the graph for each civ that is presently at war or
   is soon going to be. Projected wars are based on the knowledge of "we",
   i.e. our war plans in preparation and under consideration, and wars linked
   to these war plans.
   NB: This class does not _decide_ who targets whom. That happens in CvPlayerAI.
   The invasion graph is a prediction on which our team bases its decisions
   on war and peace.
   The graph can also run a simulation of who might conquer which cities.
   Cf. simulate(int).
   Fixme: This class does too much and has too many LoC. If everything triggered
   by simulate(int) could be encapsulated in a new class (and file), that would
   be best. Else, at least the step function should be moved into SimulationStep,
   and SimulationStep into a new file. */
class InvasionGraph {

public:

// Nested class with full access to outer class
friend class Node;

	/* A Node in an invasion graph with its edges representing a single
      (primary) target and the war opponents the Node is targeted by.
	  "Targets" in this context are civs assumed to be (soon/ in the
	  medium term) attacked by invading units. The "target team"
	  of an agent in war evaluation is a different concept. In particular,
	  a Node can be at war with many opponents, but is assumed to focus its
	  attacks on one opponent at a time.
	  It's important to distinguish the civ represented by a Node ("id")
	  from the civ from whose perspective the graph is computed ("weId").
	  Only one node in the graph represents "weId". */
	class Node {

	public:
		Node(PlayerTypes civId, InvasionGraph& outer);
		~Node();
		// Selects primary target.
		void findAndLinkTarget();
		void addWarOpponents(std::set<PlayerTypes> const& wo);
		// If they aren't war opponents: no effect
		void removeWarOpponents(std::set<PlayerTypes> const& wo);
		PlayerTypes getId() const;
		bool hasTarget() const;
		bool isIsolated() const;

		/* Methods for simulation. Not to be called until the graph is constructed.
		   Might be better to move them elsewhere, e.g. a wrapper. */
		 void prepareForSimulation();
		 void logTypicalUnits();
		 void predictArmament(int duration, bool noUpgrading = false);
		 /* Follows primaryTarget links backwards recursively and resolves
		    (simulated) losses (by calling resolveLosses). Won't terminate when
			called on a Node that's part of a cycle. */
		 void resolveLossesRec();
		 /* Follows primaryTarget links until encountering a cycle.
		    The traversed path is stored in 'path'. Returns the (path-) index of
		    the first Node encountered that is part of the cycle. A return value
		    of path.size indicates that there is no reachable cycle. */
		 size_t findCycle(std::vector<Node*>& path);
		 /* Won't terminate when called on a Node that's part of a cycle. */
		 Node& findSink();
		 bool hasLost(int cityId) const;
		 // Updates adjacency lists. NULL deletes an edge.
		 void changePrimaryTarget(Node* newTarget);
		 InvasionGraph::Node* getPrimaryTarget() const;
		 std::set<PlayerTypes> const& getTargetedBy() const;
		 /* To be called on the attacker. Caller needs to delete the result.
		    Last two parameters only for clash steps */
		 SimulationStep* step(double armyPortionDefender = 0,
				double armyPortionAttacker = 1, bool clashOnly = false) const;
		 // To be called on the defender.
		 void applyStep(SimulationStep const& step);
		 void setEliminated(bool b);
		 bool isEliminated() const;
		 void resolveLosses();
		 /* Simulates a clash of armies of two Nodes targeting each other.
		    This node clashes with its target. armyPortion1 says which
		    portion of its army this Node commits to the clash; armyPortion2
		    works likewise (for the target). */
		 void clash(double armyPortion1, double armyPortion2);
		 // For iterating over connected components
		  bool isComponentDone() const;
		  void setComponentDone(bool b);
		 // Once simulation is finished:
		  // Lost power minus shifted power
		  double getLostPower(MilitaryBranchTypes mb) const;
		  // Power after simulation minus initial power
		  double getGainedPower(MilitaryBranchTypes mb) const;
		  // Invested during the build-up phases
		  double getProductionInvested() const;
		  // These two add the respective city ids (plot indexes) to 'r'
		   void getConquests(std::set<int>& r) const;
		   void getLosses(std::set<int>& r) const;
		  /* Not ideal to store this team-level info at nodes representing civs.
		     Implemented such that all members of a master team (callees) write
			 the same vassal ids into 'r'. */
		  void getCapitulationsAccepted(std::set<TeamTypes>& r) const;
		  // Only true if capitulated during the simulation.
		  bool hasCapitulated() const;

	private:
		void initMilitary();
		void logPower(char const* msg) const;
		int countUnitsWithAI(std::vector<UnitAITypes> aiTypes) const;
		/* param: In addition to warOpponents. Also includes the vassals
		  of that team. */
		PlayerTypes findTarget(TeamTypes include = NO_TEAM) const;
		bool isValidTarget(WarAndPeaceCache::City const& c,
				TeamTypes include = NO_TEAM) const;
		static std::vector<UnitAITypes> garrisonTypes();

		InvasionGraph& outer;
		WarAndPeaceReport& report;
		PlayerTypes id, weId;
		std::set<PlayerTypes> warOpponents;
		bool isWarOpponent[MAX_CIV_PLAYERS];
		std::vector<MilitaryBranch*> military;
		std::vector<double> currentPow;
		double productionInvested;
		bool eliminated;
		bool capitulated;
		bool hasClashed;
	    // Adjacency lists
		 Node* primaryTarget;
		 std::set<PlayerTypes> targetedBy;

		static double powerCorrect(double multiplier);
		// Remaining production capacity after losses
		double productionPortion() const;

		// For simulation; call prepareForSimulation first
		 WarAndPeaceCache::City const* targetCity(
			    // Default: based on primaryTarget
				PlayerTypes owner = NO_PLAYER) const;
		 void addConquest(WarAndPeaceCache::City const& c);
		 void addLoss(WarAndPeaceCache::City const& c);
		 // Vassals that break free are currently not modeled.
		 void setCapitulated(TeamTypes masterId);
		 double clashDistance(Node const& other) const;
		 bool isSneakAttack(Node const& other, bool bClash) const;
		 bool isContinuedWar(Node const& other) const;
		 bool canReachByLand(int cityId) const;
		 CvArea* clashArea(PlayerTypes otherId) const;

		 std::vector<WarAndPeaceCache::City const*> conquests;
		 std::set<int> losses; // WarAndPeaceCache::City IDs
		 std::set<TeamTypes> capitulationsAccepted;
		 WarAndPeaceCache& cache;
		 int cacheIndex;
		 double lostPower[NUM_BRANCHES];
		 /* Units shifted from e.g. army to guard are counted as losses.
		    For war utility, may want to count them differently. To allow this,
			shifts are also tracked separately. Only records the branch that
			power is shifted away from. */
		 double shiftedPower[NUM_BRANCHES];
		 double emergencyDefPow;
		 bool componentDone;
		 /* Measure of (temporary) distraction: defense less effective while
		    trying to conquer cities of a third civ, and conquest less effective
			while trying to fend off a third civ. */
		 double distractionByConquest,
			    distractionByDefense;
		 int warTimeSimulated; // For resolveLosses; currently not used
		 double tempArmyLosses;

		 /* Should create a small class for this stuff. Related:
		    Losses from city attack. */
		  static std::pair<double,double> clashLossesWinnerLoser(double powAtt,
		      double powDef, bool att = true, bool naval = false);
		  static double clashLossesTemporary(double powAtt, double powDef);
		  static double stake(double powAtt, double powDef);
		  static double powRatio(double pow1, double pow2);
		  static double const clashPortion;
	};

public:
	InvasionGraph(MilitaryAnalyst& m, std::set<PlayerTypes> const& warParties,
			bool peaceScenario = false);
	~InvasionGraph();
	Node* getNode(PlayerTypes civId) const;// not yet used
	Node& owner() const;
	WarAndPeaceReport& getReport();
	/* No military build-up is estimated by simulate(int) until
	   this function is called. Intended to be called exactly once. */
	void addFutureWarParties(std::set<PlayerTypes> const& ourSide,
			std::set<PlayerTypes> const& ourFutureOpponents);
	// Call this before a simulation that assumes a peace treaty with the target
	void removeWar(std::set<PlayerTypes> const& ourSide,
			std::set<PlayerTypes> const& theirSide);
	// No need to call after addFutureWarParties or removeWar (done internally)
	void updateTargets();
	/* Makes sure these (possibly uninvolved) parties have nodes in the graph
	   (and ArmamentForecasts). */
	void addUninvolvedParties(std::set<PlayerTypes> const& parties);
	/*  Duration: Time horizon of the simulation. Affects the estimated military
		armament (if addFutureWarParties is called beforehand). Not suitable
		for long-term predictions (e.g. > 50 turns).
		No military build-up is assumed for the first phase.
		Simulates conquests and unit losses (expressed as a loss of power).
		These results are stored in the affected Nodes. */
	void simulate(int duration);

private:
	std::set<PlayerTypes> const& warParties;
	Node* nodeMap[MAX_PLAYERS];
	MilitaryAnalyst& m;
	PlayerTypes weId;
	WarAndPeaceReport& report;
	bool allWarPartiesKnown;
	int timeLimit; // for simulateLosses
	bool isPeaceScenario;
	bool lossesDone; // for deciding on recentlyAttacked
	bool firstSimulateCall;

	void simulateArmament(int duration, bool noUpgrading = false);
	void simulateLosses();
	/* Simulation of the connected component containing 'start'. This function
	   will mark all encountered nodes as 'componentDone' so that the caller
	   can avoid simulating the same component twice. */
	void simulateComponent(Node& start);
	// cyc isn't actually modified. The Nodes contained are.
	void breakCycle(std::vector<Node*> const& cyc);
	double willingness(PlayerTypes agg, PlayerTypes def) const;
};

/* The results of a simulation step. A simulation step is either a clash step
   or a city-attack step. The former is about two armies meeting in the field,
   the latter about an attacking army trying to conquer a city (which may also
   involve an open battle).
   The calculation happens in InvasionGraph::Node (with access to
   private members). The results are stored in instances of this class.
   So, it's more like a struct.
   Not handled by this class:
   If a city is contested between more than two war parties, then there are
   several SimulationStep objects, and only one of them is going to be applied
   (by changing the two respective Nodes). */
class SimulationStep {

public:
	SimulationStep(PlayerTypes attacker,
			WarAndPeaceCache::City const* contestedCity = NULL);
	void setDuration(int duration);
	/* id should be attacker or the defender (attacker's target); other ids are
	   treated as the defender. */
	void reducePower(PlayerTypes id, MilitaryBranchTypes mb, double subtrahend);
	void setSuccess(bool b);
	void setThreat(double d);
	/* The defending army is assumed to be split among parallel attacks
	   based on the attacks' threat values. */
	double getThreat() const;
	int getDuration() const;
	// See reducePower
	double getLostPower(PlayerTypes id, MilitaryBranchTypes mb) const;
	// Temporary losses of attacking army
	double getTempLosses() const;
	void setTempLosses(double d);
	bool isAttackerSuccessful() const;
	bool isClashOnly() const;
	PlayerTypes getAttacker() const;
	WarAndPeaceCache::City const* getCity() const;

private:
	int duration;
	double threat;
	double lostPowerAttacker[NUM_BRANCHES];
	double lostPowerDefender[NUM_BRANCHES];
	PlayerTypes attacker; // defender is attacker.primaryTarget
	WarAndPeaceCache::City const* contestedCity;
	bool success;
	double tempLosses;
};
// </advc.104>

#endif
