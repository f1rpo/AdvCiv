#pragma once

#ifndef WAR_AND_PEACE_CACHE_H
#define WAR_AND_PEACE_CACHE_H

class WarAndPeaceCache;
class MilitaryBranch;
class CvCity;
class FDataStreamBase;

/* <advc.104>: Cached data used by the war-and-peace AI. Each civ has its own
   cache. That said, each civ's cache also contains certain (subjective)
   information about all other civs, and (not nice) also a bit of team-level data.
   Also handles the updating of cached values, i.e. many of the AI's
   heuristic functions belong to this class. (Maybe it would be cleaner
   if the heuristics were moved to WarAndPeaceAI::Civ? Will have to split
   it up a bit more at some point b/c this class is getting too large.) */
class WarAndPeaceCache {

public:

	class City;

	WarAndPeaceCache();
	~WarAndPeaceCache();
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
	void update();
	void write(FDataStreamBase* stream);
	void read(FDataStreamBase* stream);
	int numReachableCities(PlayerTypes civId) const ;
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
	 bool hasAggressiveTrait() const;
	 bool hasProtectiveTrait() const;
	 bool canScrubFallout() const;
	// Ongoing combat missions against civId
	int targetMissionCount(PlayerTypes civId) const;
	/*  Long-term military threat by civId between 0 and 1. Can see it as the
		probability of them hurting us substantially with an invasion sometime
		within the next 50 to 100 turns. */
	double threatRating(PlayerTypes civId) const;
	// If civId were to capitulate to the cache owner
	 int vassalTechScore(PlayerTypes civId) const;
	 int vassalResourceScore(PlayerTypes civId) const;
	int numAdjacentLandPlots(PlayerTypes civId) const;
	int numLostTilesAtWar(TeamTypes tId) const; // advc.035
	double relativeNavyPower(PlayerTypes civId) const;
	int pastWarScore(TeamTypes tId) const;
	// Trade value paid to us for declaring war against tId
	int sponsorshipAgainst(TeamTypes tId) const;
	// Identity of the sponsor who made the above payment
	PlayerTypes sponsorAgainst(TeamTypes tId) const;
	/*  Other classes should base the actual war utility computations on this
		preliminary result */
	int warUtilityIgnoringDistraction(TeamTypes tId) const;
	// Not a _sufficient_ condition for agreeing to a joint war
	bool canBeHiredAgainst(TeamTypes tId) const;
	void setCanBeHiredAgainst(TeamTypes tId, bool b);
	void updateCanBeHiredAgainst(TeamTypes tId, int u, int thresh);
	bool canTrainDeepSeaCargo() const;
	bool canTrainAnyCargo() const;
	bool isFocusOnPeacefulVictory() const;

	/* Caching of power values. Military planning must not add the power
	   of hypothetical units to the vector; need to make a copy for that. */
	std::vector<MilitaryBranch*> const& getPowerValues() const;
	// Counts only combatants
	int numNonNavyUnits() const;
	// Includes national wonders (which City::updateAssetScore does not count)
	double totalAssetScore() const;
	/*  Number of citizens that are angry now and wouldn't be if it weren't for
		the war weariness against civId. */
	double angerFromWarWeariness(PlayerTypes civId) const;
	double goldValueOfProduction() const;
	void reportUnitCreated(CvUnitInfo const& u);
	void reportUnitDestroyed(CvUnitInfo const& u);
	void reportWarEnding(TeamTypes enemyId, CLinkList<TradeData>* weReceive = NULL,
			CLinkList<TradeData>* wePay = NULL);
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
	void updateHasAggressiveTrait();
	void updateHasProtectiveTrait();
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
	WarAndPeaceCache const& leaderCache() const;
	WarAndPeaceCache& leaderCache();

	PlayerTypes ownerId;
	std::vector<City*> v;
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
   // Serializable arrays
	// per civ
	 double wwAnger[MAX_CIV_PLAYERS];
	 int nReachableCities[MAX_CIV_PLAYERS];
	 int targetMissionCounts[MAX_CIV_PLAYERS];
	 double threatRatings[MAX_CIV_PLAYERS];
	 int vassalTechScores[MAX_CIV_PLAYERS];
	 int vassalResourceScores[MAX_CIV_PLAYERS];
	 bool located[MAX_CIV_PLAYERS];
	 // (CvTeamAI::AI_calculateAdjacentLandPlots is too slow and per team)
	 int adjacentLand[MAX_CIV_PLAYERS];
	 double relativeNavyPow[MAX_CIV_PLAYERS];
	// per team
	 int lostTilesAtWar[MAX_CIV_TEAMS]; // advc.035
	 int pastWarScores[MAX_CIV_TEAMS];
	 // Value of the sponsorship
	 int sponsorshipsAgainst[MAX_CIV_TEAMS];
	 // Identity of the sponsor (PlayerTypes)
	 int sponsorsAgainst[MAX_CIV_TEAMS];
	 int warUtilityIgnDistraction[MAX_CIV_TEAMS];
	 bool hireAgainst[MAX_CIV_TEAMS];

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
		/*  See ::cityCross in CvGameCoreUtils. If the underlying CvCity
			no longer exists, all entries are NULL. */
		void cityCross(std::vector<CvPlot*>& r);
		void write(FDataStreamBase* stream);
		void read(FDataStreamBase* stream);
		static CvCity* cityById(int id);
		// Wrapper for CvUnit::generatePath
		static bool measureDistance(PlayerTypes civId, DomainTypes dom,
				CvPlot* start, CvPlot* dest, int* r);
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

// </advc.104>

#endif
