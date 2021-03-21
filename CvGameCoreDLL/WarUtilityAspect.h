#pragma once

#ifndef WAR_UTILITY_ASPECT_H
#define WAR_UTILITY_ASPECT_H

#include "UWAI.h"
#include "AIStrategies.h"

class MilitaryAnalyst;
class WarEvalParameters;
class UWAIReport;
class UWAI::Civ;
class UWAI::Team;
class UWAICache;


/*  advc.104: New class. An aspect of war evaluation.
	Few const functions in this class because the 'log' function needs to be
	able to write to the report object. */
class WarUtilityAspect {

public:
	/*  Returns the computed utility (same as calling utility(void)).
		Sets some protected data members that subclasses should find useful.
		Concrete subclasses should therefore overwrite evaluate(void) instead. */
	virtual int evaluate(MilitaryAnalyst const& m);
	char const* aspectName() const;
	// Needs to correspond to the call order in UWAI::cacheXML
	virtual UWAI::AspectTypes xmlId() const=0;
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
	TeamTypes const agentId;
	CvTeamAI const& agent;
	UWAI::Team const& agentAI;
	int u;
	UWAIReport& report;
	CvGameAI const& game;
	EraTypes const gameEra;
	scaled const gameEraAIFactor;
	CvGameSpeedInfo const& speed;

	/*  Subclasses must not access these members until evaluate(m) has been called.
		Initialization is then guaranteed although they're not references.
		This is obviously not an ideal class design. A separate class
		WarUtilityAspect::Civ would be even more unwieldy I think. */
	MilitaryAnalyst const* m;
	PlayerTypes weId;
	CvPlayerAI* we;
	UWAI::Civ const* weAI;
	UWAICache const* ourCache;
	/*  'they' are not necessarily the team targeted by the DoW. Can be any rival
		that we might directly or indirectly gain sth. from (or lose sth. to). */
	PlayerTypes theyId;
	CvPlayerAI const* they;
	UWAI::Civ const* theyAI;
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
		 to be computed just once by UWAICache (the computations aren't
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
	bool concernsOnlyWarParties() const; // override
};

class GreedForAssets : public WarUtilityAspect {
public:
	GreedForAssets(WarEvalParameters& params) :
			WarUtilityAspect(params), ourDist(-1) {}
	void evaluate();
	UWAI::AspectTypes xmlId() const { return UWAI::GREED_FOR_ASSETS; }
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
	GreedForVassals(WarEvalParameters& params)
			: WarUtilityAspect(params) {}
	void evaluate();
	UWAI::AspectTypes xmlId() const { return UWAI::GREED_FOR_VASSALS; }
};


class GreedForSpace : public WarUtilityAspect {
public:
	GreedForSpace(WarEvalParameters& params)
			: WarUtilityAspect(params) {}
	void evaluate();
	UWAI::AspectTypes xmlId() const { return UWAI::GREED_FOR_SPACE; }
};


class GreedForCash : public WarUtilityAspect {
public:
	GreedForCash(WarEvalParameters& params)
			: WarUtilityAspect(params) {}
	void evaluate();
	UWAI::AspectTypes xmlId() const { return UWAI::GREED_FOR_CASH; }
};


class Loathing : public WarUtilityAspect {
public:
	Loathing(WarEvalParameters& params)
			: WarUtilityAspect(params) {}
	void evaluate();
	UWAI::AspectTypes xmlId() const { return UWAI::LOATHING; }
private:
	double lossRating();
};


class MilitaryVictory : public WarUtilityAspect {
public:
	MilitaryVictory(WarEvalParameters& params)
			: WarUtilityAspect(params), votesToGo(-1), enoughVotes(false) {}
	int preEvaluate();
	void evaluate();
	UWAI::AspectTypes xmlId() const { return UWAI::MILITARY_VICTORY; }
private:
	double progressRatingConquest();
	double progressRatingDomination();
	double progressRatingDiplomacy();
	void addConquestsByPartner(std::map<int,double>& r, AttitudeTypes attThresh,
			double weight);
	int votesToGo;
	bool enoughVotes;
};


class Assistance : public WarUtilityAspect {
public:
	Assistance(WarEvalParameters& params)
			: WarUtilityAspect(params) {}
	void evaluate();
	UWAI::AspectTypes xmlId() const { return UWAI::PRESERVATION_OF_PARTNERS; }
private:
	double assistanceRatio();
};


class Reconquista : public WarUtilityAspect {
public:
	Reconquista(WarEvalParameters& params);
	void evaluate();
	UWAI::AspectTypes xmlId() const { return UWAI::RECONQUISTA; }
};


class Rebuke : public WarUtilityAspect {
public:
	Rebuke(WarEvalParameters& params)
			: WarUtilityAspect(params) {}
	void evaluate();
	UWAI::AspectTypes xmlId() const { return UWAI::REBUKE; }
};


class Fidelity : public WarUtilityAspect {
public:
	Fidelity(WarEvalParameters& params)
			: WarUtilityAspect(params) {}
	void evaluate();
	UWAI::AspectTypes xmlId() const { return UWAI::FIDELITY; }
};


class HiredHand : public WarUtilityAspect {
public:
	HiredHand(WarEvalParameters& params)
			: WarUtilityAspect(params) {}
	void evaluate();
	UWAI::AspectTypes xmlId() const { return UWAI::HIRED_HAND; }
private:
	double eval(PlayerTypes allyId, int originalUtility, int obligationThresh);
};


class BorderDisputes : public WarUtilityAspect {
public:
	BorderDisputes(WarEvalParameters& params)
			: WarUtilityAspect(params) {}
	void evaluate();
	UWAI::AspectTypes xmlId() const { return UWAI::BORDER_DISPUTES; }
};


class SuckingUp : public WarUtilityAspect {
public:
	SuckingUp(WarEvalParameters& params)
			: WarUtilityAspect(params) {}
	void evaluate();
	UWAI::AspectTypes xmlId() const { return UWAI::SUCKING_UP; }
};


class PreEmptiveWar : public WarUtilityBroaderAspect {
public:
	PreEmptiveWar(WarEvalParameters& params)
			: WarUtilityBroaderAspect(params) {}
	void evaluate();
	UWAI::AspectTypes xmlId() const { return UWAI::PREEMPTIVE_WAR; }
};


class KingMaking : public WarUtilityBroaderAspect {
public:
	KingMaking(WarEvalParameters& params)
			: WarUtilityBroaderAspect(params) {}
	int preEvaluate();
	void evaluate();
	UWAI::AspectTypes xmlId() const { return UWAI::KING_MAKING; }
private:
	void addWinning(std::set<PlayerTypes>& r, bool bPredict);
	bool anyVictory(PlayerTypes civId, AIVictoryStage flags, int stage,
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
	Effort(WarEvalParameters& params)
			: WarUtilityAspect(params) {}
	int preEvaluate();
	void evaluate();
	UWAI::AspectTypes xmlId() const { return UWAI::EFFORT; }
};


class Risk : public WarUtilityAspect {
public:
	Risk(WarEvalParameters& params)
			: WarUtilityAspect(params) {}
	int preEvaluate();
	void evaluate();
	UWAI::AspectTypes xmlId() const { return UWAI::RISK; }
};


class IllWill : public WarUtilityBroaderAspect {
public:
	IllWill(WarEvalParameters& params)
		: WarUtilityBroaderAspect(params), uMinus(-1), altPartnerFactor(-1) {}
	int preEvaluate();
	void evaluate();
	UWAI::AspectTypes xmlId() const { return UWAI::ILL_WILL; }
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
	UWAI::AspectTypes xmlId() const { return UWAI::AFFECTION; }
private:
	double gameProgressFactor;
};


class Distraction : public WarUtilityAspect {
public:
	Distraction(WarEvalParameters& params)
			: WarUtilityAspect(params) {}
	void evaluate();
	UWAI::AspectTypes xmlId() const { return UWAI::DISTRACTION; }
};


class PublicOpposition : public WarUtilityAspect {
public:
	PublicOpposition(WarEvalParameters& params)
			: WarUtilityAspect(params) {}
	void evaluate();
	UWAI::AspectTypes xmlId() const { return UWAI::PUBLIC_OPPOSITION; }
};


class Revolts : public WarUtilityAspect {
public:
	Revolts(WarEvalParameters& params)
			: WarUtilityAspect(params) {}
	void evaluate();
	UWAI::AspectTypes xmlId() const { return UWAI::REVOLTS; }
private:
	std::set<int> countedCities;
};

// BroaderAspect: need to evaluate for theyId==sponsorId
class UlteriorMotives : public WarUtilityBroaderAspect {
public:
	UlteriorMotives(WarEvalParameters& params)
			: WarUtilityBroaderAspect(params) {}
	void evaluate();
	UWAI::AspectTypes xmlId() const { return UWAI::ULTERIOR_MOTIVES; }
};


class FairPlay : public WarUtilityAspect {
public:
	FairPlay(WarEvalParameters& params)
			: WarUtilityAspect(params) {}
	void evaluate();
	UWAI::AspectTypes xmlId() const { return UWAI::FAIR_PLAY; }
private:
	int initialMilitaryUnits(PlayerTypes civId);
};


class Bellicosity : public WarUtilityAspect {
public:
	Bellicosity(WarEvalParameters& params)
			: WarUtilityAspect(params) {}
	void evaluate();
	UWAI::AspectTypes xmlId() const { return UWAI::BELLICOSITY; }
};


class TacticalSituation : public WarUtilityAspect {
public:
	TacticalSituation(WarEvalParameters& params)
			: WarUtilityAspect(params) {}
	void evaluate();
	UWAI::AspectTypes xmlId() const { return UWAI::TACTICAL_SITUATION; }
private:
	void evalEngagement();
	void evalOperational();
	int evacPop(PlayerTypes ownerId, PlayerTypes invaderId);
};


class LoveOfPeace : public WarUtilityAspect {
public:
	LoveOfPeace(WarEvalParameters& params)
			: WarUtilityAspect(params) {}
	void evaluate();
	UWAI::AspectTypes xmlId() const { return UWAI::LOVE_OF_PEACE; }
};

#endif
