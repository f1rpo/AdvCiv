#pragma once

#ifndef WAR_AND_PEACE_AI_H
#define WAR_AND_PEACE_AI_H

class FDataStreamBase;

/*  <advc.104> AI functionality for decisions on war and peace. Main class of the
	Utility-Based War AI mod component (UWAI).
	(Tbd.: Change everything that's named "WPAI" or "WarAndPeace" to "UWAI".)
	Instead of making lots of additions to CvTeamAI and CvPlayerAI, I've put the
	new functions in classes WarAndPeaceAI::Team and WarAndPeaceAI::Civ, which are
	defined in WarAndPeaceAgent.h. (I use the term "civ" instead of "player" when
	Barbarians are excluded. The actual CvCivilization class added by advc.003w
	isn't used much in the WarAndPeace code.) The outer class WarAndPeaceAI shared by
	Team and Civ is for overarching stuff that would otherwise fit into CvGameAI or
	CvGameCoreUtils. An instance is accessible through the macro "getWPAI".

	The main method for war planning is WarAndPeaceAI::Team::doWar. */

#define getWPAI GC.getGame().warAndPeaceAI()

class WarAndPeaceAI /* advc.003e: */ : private boost::noncopyable {

public:

	// Inner classes; defined in WarAndPeaceAgent.h.
	class Civ;
	class Team;

	WarAndPeaceAI();
	void invalidateUICache();
	// excluded: Barbarians, Minor civs, dead civs
	 std::vector<PlayerTypes>& properCivs();
	 std::vector<TeamTypes>& properTeams();
	void update();
	// When a colony is created
	void processNewCivInGame(PlayerTypes newCivId);
	/*  true if UWAI fully enabled, making all decisions, otherwise false.
		If inBackground is set, then true if UWAI is running only in the background,
		but false if UWAI fully enabled or fully disabled. */
	bool isEnabled(bool inBackground = false) const;
	void setUseKModAI(bool b);
	void setInBackground(bool b);
	void read(FDataStreamBase* stream);
	void write(FDataStreamBase* stream);
	int maxLandDist() const;
	int maxSeaDist() const;
	bool isUpdated() const;
	void doXML(); // Can't do this in constructor b/c not yet loaded
	double aspectWeight(int xmlId) const;
	static int const preparationTimeLimited = 8;
	static int const preparationTimeLimitedNaval = 10;
	static int const preparationTimeTotal = 15;
	static int const preparationTimeTotalNaval = 20;
	// Modifier for all AI payments for peace
	static int const reparationsAIPercent = 50;
	/*  Modifier for human payments for peace, i.e what the AI asks a human to pay
		(no modifier for brokering, i.e. 100%) */
	static int const reparationsHumanPercent = 75;
	static int const dwtUtilityThresh = -35;

private:
	std::vector<PlayerTypes> _properCivs;
	std::vector<TeamTypes> _properTeams;
	std::vector<int> xmlWeights;
	bool enabled; // true iff K-Mod AI disabled through Game Options
	bool inBackgr; // status of the XML flag

	void applyPersonalityWeight();
};

#endif // </advc.104>
