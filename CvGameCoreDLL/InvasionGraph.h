#pragma once

#ifndef INVASION_GRAPH_H
#define INVASION_GRAPH_H

#include "MilitaryBranch.h"
#include "UWAICache.h"
#include "UWAIReport.h"
#include "UWAISets.h"

class MilitaryAnalyst;
class SimulationStep;
class CvArea;

/* advc.104: New class. The invasion graph says who tries to invade whom.
   It's an analysis from the perspective of one civ ("we", the owner of
   the associated MilitaryAnalyst) planning war (UWAI::Team::doWar).
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
		void addWarOpponents(PlyrSet const& wo);
		// If they aren't war opponents: no effect
		void removeWarOpponents(PlyrSet const& wo);
		inline PlayerTypes getId() const { return id; }
		bool hasTarget() const { return primaryTarget != NULL; }
		bool isIsolated() const { return !hasTarget() && targetedBy.empty(); }

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
		 bool hasLost(int cityId) const { return (losses.count(cityId) > 0); }
		 // Updates adjacency lists. NULL deletes an edge.
		 void changePrimaryTarget(Node* newTarget);
		 InvasionGraph::Node* getPrimaryTarget() const { return primaryTarget; }
		 PlyrSet const& getTargetedBy() const { return targetedBy; }
		 /* To be called on the attacker. Caller needs to delete the result.
		    Last two parameters only for clash steps */
		 SimulationStep* step(double armyPortionDefender = 0,
				double armyPortionAttacker = 1, bool clashOnly = false) const;
		 // To be called on the defender.
		 void applyStep(SimulationStep const& step);
		 void setEliminated(bool b) { eliminated = b; }
		 bool isEliminated() const { return eliminated; }
		 void resolveLosses();
		 /* Simulates a clash of armies of two Nodes targeting each other.
		    This node clashes with its target. armyPortion1 says which
		    portion of its army this Node commits to the clash; armyPortion2
		    works likewise (for the target). */
		 void clash(double armyPortion1, double armyPortion2);
		 // For iterating over connected components
		  bool isComponentDone() const { return componentDone; }
		 // Once simulation is finished:
		  // Lost power minus shifted power
		  double getLostPower(MilitaryBranchTypes mb) const {
			  return lostPower[mb] - shiftedPower[mb];
		  }
		  // Power after simulation minus initial power
		  double getGainedPower(MilitaryBranchTypes mb) const {
			  return military[mb]->power() - currentPow[mb];
		  }
		  // Invested during the build-up phases
		  double getProductionInvested() const { return productionInvested; }
		  // These two add the respective city ids (plot indexes) to 'r'
		   void getConquests(CitySet& r) const;
		   void getLosses(CitySet& r) const;
		   bool anyConquests() const { return !conquests.empty(); }
		   bool anyLosses() const { return !losses.empty(); }
		  /* Not ideal to store this team-level info at nodes representing civs.
		     Implemented such that all members of a master team (callees) write
			 the same vassal ids into 'r'. */
		  void getCapitulationsAccepted(TeamSet& r) const;
		  // Only true if capitulated during the simulation.
		  bool hasCapitulated() const { return capitulated; }

	private:
		void initMilitary();
		void logPower(char const* msg) const;
		int countUnitsWithAI(std::vector<UnitAITypes> aiTypes) const;
		/* param: In addition to warOpponents. Also includes the vassals
		  of that team. */
		PlayerTypes findTarget(TeamTypes include = NO_TEAM) const;
		PlayerTypes findBestTarget(TeamTypes include) const;
		bool isValidTarget(UWAICache::City const& c, TeamTypes include = NO_TEAM) const;
		bool isValidTarget(PlayerTypes target, TeamTypes include) const;
		static std::vector<UnitAITypes> garrisonTypes();

		InvasionGraph& outer;
		UWAIReport& report;
		PlayerTypes id, weId;
		PlyrSet warOpponents;
		std::vector<bool> isWarOpponent;
		std::vector<MilitaryBranch*> military;
		std::vector<double> currentPow;
		double productionInvested;
		bool eliminated;
		bool capitulated;
		bool hasClashed;
	    // Adjacency lists
		 Node* primaryTarget;
		 PlyrSet targetedBy;

		// Remaining production capacity after losses
		double productionPortion() const;

		// For simulation; call prepareForSimulation first
		 UWAICache::City const* targetCity(
			    // Default: based on primaryTarget
				PlayerTypes owner = NO_PLAYER) const;
		 void addConquest(UWAICache::City const& c);
		 void addLoss(UWAICache::City const& c) { losses.insert(c.id()); }
		 // Vassals that break free are currently not modeled.
		 void setCapitulated(TeamTypes masterId);
		 double clashDistance(Node const& other) const;
		 bool isSneakAttack(Node const& other, bool bClash) const;
		 bool isContinuedWar(Node const& other) const;
		 bool canReachByLand(int cityId, bool fromCapital) const;
		 CvArea const* clashArea(PlayerTypes otherId) const;

		 std::vector<UWAICache::City const*> conquests;
		 CitySet losses;
		 TeamSet capitulationsAccepted;
		 UWAICache& cache;
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
		  static double clashLossesTemporary(double powAtt, double powDef) {
			  return clashPortion * std::min(powAtt, powDef) / 3;
		  }
		  static double stake(double powAtt, double powDef) {
			  return clashPortion * std::min(1.0, 1.6 * powRatio(powAtt, powDef));
		  }
		  static double powRatio(double pow1, double pow2) {
			  return std::min(pow1, pow2) / (std::max(pow1, pow2) + 0.001);
		  }
		  static double const clashPortion;
	};

public:
	InvasionGraph(MilitaryAnalyst& m, PlyrSet const& warParties,
			bool peaceScenario = false);
	~InvasionGraph();
	Node* getNode(PlayerTypes civId) const {
		FAssertBounds(0, MAX_PLAYERS, civId);
		return nodeMap[civId];
	}
	/* No military build-up is estimated by simulate(int) until
	   this function is called. Intended to be called exactly once. */
	void addFutureWarParties(PlyrSet const& ourSide, PlyrSet const& ourFutureOpponents);
	// Call this before a simulation that assumes a peace treaty with the target
	void removeWar(PlyrSet const& ourSide, PlyrSet const& theirSide);
	// No need to call after addFutureWarParties or removeWar (done internally)
	void updateTargets();
	/* Makes sure these (possibly uninvolved) parties have nodes in the graph
	   (and ArmamentForecasts). */
	void addUninvolvedParties(PlyrSet const& parties);
	/*  Duration: Time horizon of the simulation. Affects the estimated military
		armament (if addFutureWarParties is called beforehand). Not suitable
		for long-term predictions (e.g. > 50 turns).
		No military build-up is assumed for the first phase.
		Simulates conquests and unit losses (expressed as a loss of power).
		These results are stored in the affected Nodes. */
	void simulate(int duration);

private:
	PlyrSet const& warParties;
	std::vector<Node*> nodeMap;
	MilitaryAnalyst& m;
	PlayerTypes weId;
	UWAIReport& report;
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
	SimulationStep(PlayerTypes attacker, UWAICache::City const* contestedCity = NULL);
	void setDuration(int duration) { this->duration = duration; }
	/* id should be attacker or the defender (attacker's target); other ids are
	   treated as the defender. */
	void reducePower(PlayerTypes id, MilitaryBranchTypes mb, double subtrahend);
	void setSuccess(bool b) { success = b; }
	void setThreat(double threat) { this->threat = threat; }
	/* The defending army is assumed to be split among parallel attacks
	   based on the attacks' threat values. */
	double getThreat() const { return threat; }
	int getDuration() const { return duration; }
	// See reducePower
	double getLostPower(PlayerTypes id, MilitaryBranchTypes mb) const;
	// Temporary losses of attacking army
	double getTempLosses() const { return tempLosses; }
	void setTempLosses(double d) { tempLosses = d; }
	bool isAttackerSuccessful() const { return success; }
	bool isClashOnly() const { return (contestedCity == NULL); }
	PlayerTypes getAttacker() const { return attacker; }
	UWAICache::City const* getCity() const { return contestedCity; }

private:
	int duration;
	double threat;
	double lostPowerAttacker[NUM_BRANCHES];
	double lostPowerDefender[NUM_BRANCHES];
	PlayerTypes attacker; // defender is attacker.primaryTarget
	UWAICache::City const* contestedCity;
	bool success;
	double tempLosses;
};

#endif
