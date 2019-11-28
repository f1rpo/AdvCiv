#pragma once

#ifndef WAR_UTILITY_ASPECT_H
#define WAR_UTILITY_ASPECT_H

#include "WarAndPeaceAI.h"

class MilitaryAnalyst;
class WarEvalParameters;
class WarAndPeaceReport;
class WarAndPeaceAI::Civ;
class WarAndPeaceAI::Team;
class WarAndPeaceCache;


/*  advc.104: New class. An aspect of war evaluation.
	Few const functions in this class because the 'log' function needs to be
	able to write to the report object. */
class WarUtilityAspect {

public:
	/*  Returns the computed utility (same as calling utility(void)).
		Sets some protected data members that subclasses should find useful.
		Concrete subclasses should therefore overwrite evaluate(void) instead. */
	virtual int evaluate(MilitaryAnalyst const& m);
	virtual char const* aspectName() const=0;
	// Needs to correspond to the call order in WarAndPeaceAI::cacheXML
	virtual int xmlId() const=0;
	/*  Caller needs to call evaluate first, which computes war utility. This is
		just a getter. */
	int utility() const { return u; }

protected:
	WarUtilityAspect(WarEvalParameters const& params);
	// Just for convenience (replacing report->log)
	void log(char const* fmt, ...);
	/*  What can m->ourId gain from or lose to theyId (both set by
		evaluate(MilitaryAnalyst&)). From the pov of m->ourId.
		After computing the aspect utility, subclasses should add (not assign!)
		their result to WarUtilityAspect::u */
	virtual void evaluate()=0;
	/*  If war utility is computed in this function, it should be added to
		WarUtilityAspect::u. */
	/*  Pre-computations not specific to a given civ. Most subclasses shouldn't
		need this.
		If (partial) war utility is computed, it should be returned (otherwise 0).
		This function should not modify WarUtilityAspect::u. */
	virtual int preEvaluate();
	// See WarUtilityBroaderAspect. Concrete subclasses shouldn't overwrite this.
	virtual bool concernsOnlyWarParties() const;

	/* Everything and the kitchen sink. Lots of code in this
	   class hierarchy; don't want to call getters all the time.
	   Caveat: The order of declaration here determines the order
	   of initialization in the constructor. Improper order will
	   result in faulty initialization. The Visual Studio compiler
	   doesn't warn about this either (lacks a WReorder option).
	   This also affects a few other classes that have references
	   as members, in particular WarEvaluator and MilitaryAnalyst.*/
	WarEvalParameters const& params;
	TeamTypes agentId;
	CvTeamAI& agent;
	WarAndPeaceAI::Team& agentAI;
	int u;
	WarAndPeaceReport& report;
	// So that subclasses don't need to call GC.getGame().getCurrentEra() repeatedly:
	EraTypes gameEra;

	/*  Subclasses must not access these members until evaluate(m) has been called.
		Initialization is then guaranteed although they're not references.
		This is obviously not an ideal class design. A separate class
		WarUtilityAspect::Civ would be even more unwieldy I think. */
	MilitaryAnalyst const* m;
	PlayerTypes weId;
	CvPlayerAI* we;
	WarAndPeaceAI::Civ const* weAI;
	WarAndPeaceCache const* ourCache;
	/*  'they' are not necessarily the team targeted by the DoW. Can be any rival
		that we might directly or indirectly gain sth. from (or lose sth. to). */
	PlayerTypes theyId;
	CvPlayerAI const* they;
	WarAndPeaceAI::Civ const* theyAI;
	std::vector<int> weConquerFromThem;
	// Our current attitude towards them and their current attitude towards us
	AttitudeTypes towardsThem, towardsUs;
	int valTowardsThem, valTowardsUs; // Relations values

	// To be called by subclasses only from evaluate or preEvaluate.
	 /* Score for assets lost by them to a given civ, or to any civ
		(first parameter NO_PLAYER).
		Computed based on our knowledge (i.e. not necessarily from the pov of 'to').
		The score total for all their present assets (lost or not) can be obtained
		by passing a double pointer.
		(Shouldn't use theyAI->getCache().totalAssetScore() instead b/c that
		includes data about cities we may not know of.)
		Update: Now also subtracts score for assets conquered by them from 'to'
		(or from any civ); i.e. scores net loss (or gain) of assets.
		Gains from team ignoreGains aren't counted. */
	 double lostAssetScore(PlayerTypes to = NO_PLAYER, double* returnTotal = NULL,
			TeamTypes ignoreGains = NO_TEAM);
	 double lossesFromBlockade(PlayerTypes victimId, PlayerTypes to);
	 double lossesFromNukes(PlayerTypes victimId, PlayerTypes sourceId);
	 // advc.035:
	 double lossesFromFlippedTiles(PlayerTypes victimId, PlayerTypes sourceId = NO_PLAYER);
	 /* Score for assets conquered by us from them (as set by evaluate(void)).
		'mute' disables logging within the function body. */
	 double conqAssetScore(bool mute = true);
	 // Portion of cities of civId that aren't lost in the war
	 double cityRatio(PlayerTypes civId) const;
	 // Between our team and 'other', or their team if none given
	 double normalizeUtility(double utilityTeamOnTeam, TeamTypes other = NO_TEAM);
	 template<bool bCHECK_HAS_MET> int countRivals() const {
		 return PlayerIter<FREE_MAJOR_CIV, bCHECK_HAS_MET ?
				KNOWN_POTENTIAL_ENEMY_OF : POTENTIAL_ENEMY_OF>::count(agentId);
	 }
	 /*  Evaluation of their usefulness as our trade partner. Would prefer this
		 to be computed just once by WarAndPeaceCache (the computations aren't
		 totally cheap), but I also want the log output. They're not called
		 frequently. */
	 double partnerUtilFromTech();
	 double partnerUtilFromTrade();
	 double partnerUtilFromMilitary();
	 static int const partnerUtilFromOB = 8;

private:
	// In between calls to evaluate
	 void reset();
	 void resetCivOnCiv();
	int evaluate(PlayerTypes theyId);
	AttitudeTypes techRefuseThresh(PlayerTypes civId);
};

/*  Not a nice name. Derive from this class rather than WarUtilityAspect
	if evaluate(void) should be called also for parties that aren't part of the
	military analysis. */
class WarUtilityBroaderAspect : public WarUtilityAspect {
protected:
	WarUtilityBroaderAspect(WarEvalParameters& params);
	bool concernsOnlyWarParties() const;
};

class GreedForAssets : public WarUtilityAspect {
public:
	GreedForAssets(WarEvalParameters& params);
	void evaluate();
	char const* aspectName() const;
	int xmlId() const;
private:
	void initCitiesPerArea();
	void freeCitiesPerArea();
	double overextensionCost();
	double defensibilityCost();
	double medianDistFromOurConquests(PlayerTypes civId);
	double threatToCities(PlayerTypes civId);
	double competitionMultiplier();
	double teamSizeMultiplier();
	std::vector<std::map<int,int>*> citiesPerArea;
	double ourDist;
};


class GreedForVassals : public WarUtilityAspect {
public:
	GreedForVassals(WarEvalParameters& params);
	void evaluate();
	char const* aspectName() const;
	int xmlId() const;
};


class GreedForSpace : public WarUtilityAspect {
public:
	GreedForSpace(WarEvalParameters& params);
	void evaluate();
	char const* aspectName() const;
	int xmlId() const;
};


class GreedForCash : public WarUtilityAspect {
public:
	GreedForCash(WarEvalParameters& params);
	void evaluate();
	char const* aspectName() const;
	int xmlId() const;
};


class Loathing : public WarUtilityAspect {
public:
	Loathing(WarEvalParameters& params);
	void evaluate();
	char const* aspectName() const;
	int xmlId() const;
private:
	double lossRating();
};


class MilitaryVictory : public WarUtilityAspect {
public:
	MilitaryVictory(WarEvalParameters& params);
	int preEvaluate();
	void evaluate();
	char const* aspectName() const;
	int xmlId() const;
private:
	double progressRatingConquest();
	double progressRatingDomination();
	double progressRatingDiplomacy();
	void addConquestsByPartner(std::map<int,double>& r, AttitudeTypes attThresh,
			double weight);
	double votesToGo;
	bool enoughVotes;
};


class Assistance : public WarUtilityAspect {
public:
	Assistance(WarEvalParameters& params);
	void evaluate();
	char const* aspectName() const;
	int xmlId() const;
private:
	double assistanceRatio();
};


class Reconquista : public WarUtilityAspect {
public:
	Reconquista(WarEvalParameters& params);
	void evaluate();
	char const* aspectName() const;
	int xmlId() const;
};


class Rebuke : public WarUtilityAspect {
public:
	Rebuke(WarEvalParameters& params);
	void evaluate();
	char const* aspectName() const;
	int xmlId() const;
};


class Fidelity : public WarUtilityAspect {
public:
	Fidelity(WarEvalParameters& params);
	void evaluate();
	char const* aspectName() const;
	int xmlId() const;
};


class HiredHand : public WarUtilityAspect {
public:
	HiredHand(WarEvalParameters& params);
	void evaluate();
	char const* aspectName() const;
	int xmlId() const;
private:
	double eval(PlayerTypes allyId, int originalUtility, int obligationThresh);
};


class BorderDisputes : public WarUtilityAspect {
public:
	BorderDisputes(WarEvalParameters& params);
	void evaluate();
	char const* aspectName() const;
	int xmlId() const;
};


class SuckingUp : public WarUtilityAspect {
public:
	SuckingUp(WarEvalParameters& params);
	void evaluate();
	char const* aspectName() const;
	int xmlId() const;
};


class PreEmptiveWar : public WarUtilityBroaderAspect {
public:
	PreEmptiveWar(WarEvalParameters& params);
	void evaluate();
	char const* aspectName() const;
	int xmlId() const;
};


class KingMaking : public WarUtilityBroaderAspect {
public:
	KingMaking(WarEvalParameters& params);
	int preEvaluate();
	void evaluate();
	char const* aspectName() const;
	int xmlId() const;
private:
	void addWinning(std::set<PlayerTypes>& r, bool bPredict);
	bool anyVictory(PlayerTypes civId, int iVictoryFlags, int stage,
			bool bPredict = true) const;
	void addLeadingCivs(std::set<PlayerTypes>& r, double margin,
			bool bPredict = true) const;
	double theirRelativeLoss();
	std::set<PlayerTypes> winningFuture;
	std::set<PlayerTypes> winningPresent;
	static double const scoreMargin;
};


class Effort : public WarUtilityAspect {
public:
	Effort(WarEvalParameters& params);
	int preEvaluate();
	void evaluate();
	char const* aspectName() const;
	int xmlId() const;
};


class Risk : public WarUtilityAspect {
public:
	Risk(WarEvalParameters& params);
	int preEvaluate();
	void evaluate();
	char const* aspectName() const;
	int xmlId() const;
};


class IllWill : public WarUtilityBroaderAspect {
public:
	IllWill(WarEvalParameters& params);
	int preEvaluate();
	void evaluate();
	char const* aspectName() const;
	int xmlId() const;
private:
	void evalLostPartner();
	void evalRevenge();
	double powerRatio();
	void evalAngeredPartners();
	double nukeCost(double nukes);
	double uMinus;
	double altPartnerFactor;
};


class Affection : public WarUtilityAspect {
public:
	Affection(WarEvalParameters& params);
	void evaluate();
	char const* aspectName() const;
	int xmlId() const;
private:
	double gameProgressFactor;
};


class Distraction : public WarUtilityAspect {
public:
	Distraction(WarEvalParameters& params);
	void evaluate();
	char const* aspectName() const;
	int xmlId() const;
};


class PublicOpposition : public WarUtilityAspect {
public:
	PublicOpposition(WarEvalParameters& params);
	void evaluate();
	char const* aspectName() const;
	int xmlId() const;
};


class Revolts : public WarUtilityAspect {
public:
	Revolts(WarEvalParameters& params);
	void evaluate();
	char const* aspectName() const;
	int xmlId() const;
private:
	std::set<int> countedCities;
};


class UlteriorMotives : public WarUtilityBroaderAspect {
public:
	UlteriorMotives(WarEvalParameters& params);
	void evaluate();
	char const* aspectName() const;
	int xmlId() const;
};


class FairPlay : public WarUtilityAspect {
public:
	FairPlay(WarEvalParameters& params);
	void evaluate();
	char const* aspectName() const;
	int xmlId() const;
private:
	int initialMilitaryUnits(PlayerTypes civId);
};


class Bellicosity : public WarUtilityAspect {
public:
	Bellicosity(WarEvalParameters& params);
	void evaluate();
	char const* aspectName() const;
	int xmlId() const;
};


class TacticalSituation : public WarUtilityAspect {
public:
	TacticalSituation(WarEvalParameters& params);
	void evaluate();
	char const* aspectName() const;
	int xmlId() const;
private:
	void evalEngagement();
	void evalOperational();
	int evacPop(PlayerTypes ownerId, PlayerTypes invaderId);
};

#endif
