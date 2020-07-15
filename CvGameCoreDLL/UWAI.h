#pragma once

#ifndef UWAI_H
#define UWAI_H

class FDataStreamBase;

/*  AI functionality for decisions on war and peace. Main class of the
	Utility-Based War AI mod component (UWAI).
	Instead of making lots of additions to CvTeamAI and CvPlayerAI, I've put the
	new functions in classes UWAI::Team and UWAI::Civ, which are
	defined in UWAIAgent.h. (I use the term "civ" instead of "player" when
	Barbarians are excluded. The actual CvCivilization class added by advc.003w
	isn't used much in the UWAI code.) The outer class UWAI shared by
	Team and Civ is for overarching stuff that would otherwise fit into CvGameAI or
	CvGameCoreUtils. An instance is accessible through the macro "getUWAI".

	The main method for war planning is UWAI::Team::doWar. */

#define getUWAI GC.AI_getGame().uwai()

class UWAI /* advc.003e: */ : private boost::noncopyable {

public:

	// Inner classes; defined in UWAIAgent.h.
	class Civ;
	class Team;

	UWAI();
	void invalidateUICache();
	// When a colony is created
	void initNewCivInGame(PlayerTypes newCivId);
	// When the colony has received a capital and tech
	void processNewCivInGame(PlayerTypes newCivId);
	/*  true if UWAI fully enabled, making all decisions, otherwise false.
		If inBackground is set, then true if UWAI is running only in the background,
		but false if UWAI fully enabled or fully disabled. */
	bool isEnabled(bool inBackground = false) const; // Exposed to Python via CyGame::useKModAI
	void setUseKModAI(bool b);
	void setInBackground(bool b);
	void read(FDataStreamBase* stream);
	void write(FDataStreamBase* stream);
	int maxLandDist() const;
	int maxSeaDist() const;
	bool isUpdated() const;

	static int const preparationTimeLimited = 8;
	static int const preparationTimeLimitedNaval = 10;
	static int const preparationTimeTotal = 15;
	static int const preparationTimeTotalNaval = 20;
	// Modifier for any AI payments for peace
	static int const reparationsAIPercent = 50;
	/*  Modifier for human payments for peace, i.e what the AI asks a human to pay
		(no modifier for brokering, i.e. 100%) */
	static int const reparationsHumanPercent = 75;
	static int const dwtUtilityThresh = -35;

	void doXML();
	#define DO_FOR_EACH_WAR_UTILITY_ASPECT(DO)\
		DO(GREED_FOR_ASSETS) \
		DO(GREED_FOR_VASSALS) \
		DO(GREED_FOR_SPACE) \
		DO(GREED_FOR_CASH) \
		DO(LOATHING) \
		DO(MILITARY_VICTORY) \
		DO(PRESERVATION_OF_PARTNERS) \
		DO(RECONQUISTA) \
		DO(REBUKE) \
		DO(FIDELITY) \
		DO(HIRED_HAND) \
		DO(BORDER_DISPUTES) \
		DO(SUCKING_UP) \
		DO(PREEMPTIVE_WAR) \
		DO(KING_MAKING) \
		DO(EFFORT) \
		DO(RISK) \
		DO(ILL_WILL) \
		DO(AFFECTION) \
		DO(DISTRACTION) \
		DO(PUBLIC_OPPOSITION) \
		DO(REVOLTS) \
		DO(ULTERIOR_MOTIVES) \
		DO(FAIR_PLAY) \
		DO(BELLICOSITY) \
		DO(TACTICAL_SITUATION) \
		DO(LOVE_OF_PEACE)
	enum AspectTypes {
		DO_FOR_EACH_WAR_UTILITY_ASPECT(MAKE_ENUMERATOR)
		NUM_ASPECTS
	};
	double aspectWeight(AspectTypes eAspect) const {
		FAssertBounds(0, NUM_ASPECTS, eAspect);
		return xmlWeights[eAspect] / 100.0;
	}
	char const* aspectName(AspectTypes eAspect) const {
		FAssertBounds(0, NUM_ASPECTS, eAspect);
		return aszAspectNames[eAspect];
	}

private:
	std::vector<int> xmlWeights;
	std::vector<char const*> aszAspectNames;
	bool enabled; // true iff K-Mod AI disabled through Game Options
	bool inBackgr; // status of the XML flag

	void applyPersonalityWeight();
};

#endif
