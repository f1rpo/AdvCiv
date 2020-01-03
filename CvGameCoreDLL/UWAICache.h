#pragma once

#ifndef UWAI_CACHE_H
#define UWAI_CACHE_H

class UWAICache;
class MilitaryBranch;
class CvCity;
class FDataStreamBase;

/* advc.104: Cached data used by the war-and-peace AI. Each civ has its own
   cache. That said, each civ's cache also contains certain (subjective)
   information about all other civs, and (not nice) also a bit of team-level data.
   Also handles the updating of cached values, i.e. many of the AI's
   heuristic functions belong to this class. (Maybe it would be cleaner
   if the heuristics were moved to UWAI::Civ? Will have to split
   it up a bit more at some point b/c this class is getting too large.) */
class UWAICache {

public:

	class City;

	UWAICache();
	~UWAICache();
	/* Call order during initialization and clean-up:
	   + When starting a new game directly after starting Civ 4:
		 Constructors (for CvTeam and all related classes) are called, then init.
		 Actually, the CvTeam constructor is already called when starting up Civ.
	   + When starting a new game after returning to the main menu
		 from another game: Constructors are _not_ called; objects need
		 to be reset in init. Error-prone, but there's no changing it.
		 clear(bool) handles the reset.
	   + When loading a savegame right after starting Civ:
		 Constructors are called (some already when starting Civ), read is called
		 while loading the savegame.
	   + When loading a savegame after returning to the main menu:
		 Only read is called. Will have to reset the object.
	   + When returning to the main menu, nothing special happens.
	   + Only when exiting Civ, destructors are called. They might be called on
		 other occasions, but, on exit, it's guaranteeed.
	   + When saving a game, write is called. */
	void init(PlayerTypes ownerId);
	void uninit(); // Called when the owner is defeated (to free memory)
	void update();
	void write(FDataStreamBase* stream);
	void read(FDataStreamBase* stream);
	int numReachableCities(PlayerTypes civId) const { return nReachableCities.get(civId); }
	int size() const;
		private: /* These shouldn't be used anymore. Always keep the cache
					sorted by AttackPriority instead. */
	void sortCitiesByOwnerAndDistance();
	void sortCitiesByOwnerAndTargetValue();
	void sortCitiesByDistance();
	void sortCitiesByTargetValue();
		public:
	void sortCitiesByAttackPriority();
	inline City* getCity(int index) const {
		// Verify that the city still exists
		if(v[index] == NULL || v[index]->city() == NULL)
			return NULL;
		return v[index];
	};
	// Use CvCity::plotNum for the plotIndex of a given CvCity
	City* lookupCity(int plotIndex) const;
	City* lookupCity(CvCity const& cvCity) const;
	// Referring to cache owner
	 /*	Any trait that gives free Combat I (or any other promotion that grants
		an unconditional combat bonus). */
	 bool hasAggressiveTrait() const { return bHasAggressiveTrait; }
	 /* Any trait that gives free Garrison I (or any other promotion that
		boosts city defense). */
	 bool hasProtectiveTrait() const { return bHasProtectiveTrait; }
	 bool canScrubFallout() const { return canScrub; }
	// Ongoing combat missions against civId
	int targetMissionCount(PlayerTypes civId) const { return targetMissionCounts.get(civId); }
	/*  Long-term military threat by civId between 0 and 1. Can see it as the
		probability of them hurting us substantially with an invasion sometime
		within the next 50 to 100 turns. */
	double threatRating(PlayerTypes civId) const { return threatRatings.get(civId); }
	// If civId were to capitulate to the cache owner
	 int vassalTechScore(PlayerTypes civId) const { return vassalTechScores.get(civId); }
	 int vassalResourceScore(PlayerTypes civId) const { return vassalResourceScores.get(civId); }
	int numAdjacentLandPlots(PlayerTypes civId) const { return adjacentLand.get(civId); }
	int numLostTilesAtWar(TeamTypes tId) const { return lostTilesAtWar.get(tId); } // advc.035
	double relativeNavyPower(PlayerTypes civId) const { return relativeNavyPow.get(civId); }
	int pastWarScore(TeamTypes tId) const { return pastWarScores.get(tId); }
	// Trade value paid to us for declaring war against tId
	int sponsorshipAgainst(TeamTypes tId) const { return sponsorshipsAgainst.get(tId); }
	// Identity of the sponsor who made the above payment
	PlayerTypes sponsorAgainst(TeamTypes tId) const { return sponsorsAgainst.get(tId); }
	/*  Other classes should base the actual war utility computations on this
		preliminary result */
	int warUtilityIgnoringDistraction(TeamTypes tId) const { return leaderCache().warUtilityIgnDistraction.get(tId); }
	// Not a _sufficient_ condition for agreeing to a joint war
	bool canBeHiredAgainst(TeamTypes tId) const { return leaderCache().hireAgainst.get(tId); }
	void setCanBeHiredAgainst(TeamTypes tId, bool b);
	void updateCanBeHiredAgainst(TeamTypes tId, int u, int thresh);
	bool canTrainDeepSeaCargo() const { return trainDeepSeaCargo; }
	bool canTrainAnyCargo() const { return trainAnyCargo; }
	bool isFocusOnPeacefulVictory() const { return focusOnPeacefulVictory; }

	/* Caching of power values. Military planning must not add the power
	   of hypothetical units to the vector; need to make a copy for that. */
	inline std::vector<MilitaryBranch*> const& getPowerValues() const { return militaryPower; }
	// Counts only combatants
	int numNonNavyUnits() const { return nNonNavyUnits; }
	// Includes national wonders (which City::updateAssetScore does not count)
	double totalAssetScore() const { return totalAssets; }
	/*  Number of citizens that are angry now and wouldn't be if it weren't for
		the war weariness against civId. */
	double angerFromWarWeariness(PlayerTypes civId) const { return wwAnger.get(civId); }
	double goldValueOfProduction() const { return goldPerProduction; }
	void reportUnitCreated(CvUnitInfo const& u);
	void reportUnitDestroyed(CvUnitInfo const& u);
	void reportWarEnding(TeamTypes enemyId, CLinkList<TradeData> const* weReceive = NULL,
			CLinkList<TradeData> const* wePay = NULL);
	void reportCityOwnerChanged(CvCity* c, PlayerTypes oldOwnerId);
	/*  Would prefer to pass a CvDeal object, but no suitable one is available
		at the call location */
	void reportSponsoredWar(CLinkList<TradeData> const& sponsorship,
			PlayerTypes sponsorId, TeamTypes targetId);
	bool isReadyToCapitulate(TeamTypes masterId) const;
	void setReadyToCapitulate(TeamTypes masterId, bool b);
	 // When forming a Permanent Alliance
	void addTeam(PlayerTypes otherLeaderId);
	// Moves data that is stored only at the team leader
	void onTeamLeaderChanged(PlayerTypes formerLeaderId);
	/*  public b/c this needs to be done ahead of the normal update when a
		colony is created (bootstrapping problem) */
	void updateTypicalUnits();

private:
	// beforeUpdated: Only clear data that is recomputed in 'update'
	void clear(bool beforeUpdate = false);
	void updateCities(PlayerTypes civId);
	void updateLatestTurnReachableBySea();
	void updateTraits();
	void updateTargetMissionCounts();
	void updateThreatRatings();
	void updateVassalScores();
	void updateAdjacentLand();
	void updateLostTilesAtWar(); // advc.035
	void updateRelativeNavyPower();
	void updateTargetMissionCount(PlayerTypes civId);
	double calculateThreatRating(PlayerTypes civId) const;
	double teamThreat(TeamTypes tId) const;
	double longTermPower(TeamTypes tId, bool defensive = false) const;
	void updateVassalScore(PlayerTypes civId);
	void updateMilitaryPower(CvUnitInfo const& u, bool add);
	void updateTotalAssetScore();
	void updateWarAnger();
	void updateGoldPerProduction();
	 double goldPerProdBuildings();
	 double goldPerProdSites();
	 double goldPerProdVictory();
	void updateWarUtility();
	void updateWarUtilityIgnDistraction(TeamTypes targetId);
	void updateCanScrub();
	void updateTrainCargo();
	bool calculateFocusOnPeacefulVictory();
	// To supply team-on-team data
	UWAICache const& leaderCache() const;
	UWAICache& leaderCache();

	PlayerTypes ownerId;
	std::vector<City*> v;
	// I've tried stdext::hash_map for both of these. That was a little bit slower.
	std::map<int,City*> cityMap;
	std::map<int,std::pair<int,int> > latestTurnReachableBySea;
	std::vector<MilitaryBranch*> militaryPower;

	int nNonNavyUnits;
	double totalAssets;
	double goldPerProduction;
	bool bHasAggressiveTrait, bHasProtectiveTrait;
	bool canScrub;
	bool trainDeepSeaCargo, trainAnyCargo;
	bool focusOnPeacefulVictory;
	std::set<TeamTypes> readyToCapitulate;
	static double const goldPerProdUpperLimit;
  
	CivPlayerMap<float> wwAnger;
	CivPlayerMap<int> nReachableCities;
	CivPlayerMap<int> targetMissionCounts;
	CivPlayerMap<float> threatRatings;
	CivPlayerMap<int> vassalTechScores;
	CivPlayerMap<int> vassalResourceScores;
	CivPlayerMap<bool> located;
	 // (CvTeamAI::AI_calculateAdjacentLandPlots is too slow and per team)
	 CivPlayerMap<int> adjacentLand;
	 CivPlayerMap<float> relativeNavyPow;

	CivTeamMap<int> lostTilesAtWar; // advc.035
	CivTeamMap<int> pastWarScores;
	// Value of the sponsorship
	CivTeamMap<int> sponsorshipsAgainst;
	// Identity of the sponsor (PlayerTypes)
	CivTeamMap<PlayerTypes,NO_PLAYER> sponsorsAgainst;
	CivTeamMap<int,MIN_INT> warUtilityIgnDistraction;
	CivTeamMap<bool,true> hireAgainst;

public:
	/* Information to be cached about a CvCity and scoring functions useful
	   for computing war utility. */
	class City {
	public:
		City(PlayerTypes cacheOwnerId, CvCity const& c);
		City(PlayerTypes cacheOwnerId); // for reading from savegame
		PlayerTypes cityOwner() const;
		bool isOwnCity() const;
		int getAssetScore() const;
		bool canReach() const;
		/*  -1 if unreachable
			forceCurrentVal: Doesn't fall back on latestTurnReachable if not
			currently reachable. */
		int getDistance(bool forceCurrentVal = false) const;
		int getTargetValue() const;
		/* A mix of target value and distance. Target value alone would
		   ignore opportunistic attacks. */
		double attackPriority() const;
		bool canReachByLand() const;
		// Use canReachByLand instead for military analysis
		bool canCurrentlyReachBySea() const;
		/* CvCity doesn't have a proper ID. Use the plot number (index of
		   the linearized map) as an ID. It's unique (b/c there is at most
		   one city per plot), but not consecutive (most plots don't have a city). */
		int id() const;
		// NULL if the city no longer exists at the time of retrieval
		CvCity* city() const;
		void write(FDataStreamBase* stream);
		void read(FDataStreamBase* stream);
		static CvCity* cityById(int id);
		// Wrapper for CvUnit::generatePath
		static bool measureDistance(PlayerTypes civId, DomainTypes dom,
				CvPlot const& start, CvPlot const& dest, int* r);
		static double estimateMovementSpeed(PlayerTypes civId, DomainTypes dom, int dist);
		/* For sorting cities. None of these are currently used, and I'm not
			sure if they handle -1 distance/attackPriority/targetValue correctly. */
		 /*	The ordering of owners is arbitrary, just ensures that each civ's cities
			are ordered consecutively. For cities of the same owner: true if 'one'
			is closer to us than 'two' in terms of getDistance. */
		 static bool byOwnerAndDistance(City* one, City* two);
		 static bool byDistance(City* one, City* two);
		// Unreachable cities are treated as having targetValue -1
		 static bool byOwnerAndTargetValue(City* one, City* two);
		 static bool byTargetValue(City* one, City* two);
		 static bool byAttackPriority(City* one, City* two);

	private:
		/* Auxiliary function for sorting. -1 means one < two, +1 two < one and 0
		   neither. */
		static int byOwner(City* one, City* two);
		void updateDistance(CvCity const& targetCity);
		void updateAssetScore();

		int distance, targetValue, assetScore;
		bool reachByLand;
		bool reachBySea;
		bool canDeduce;
		int plotIndex;
		PlayerTypes cacheOwnerId;
	};
};

#endif
