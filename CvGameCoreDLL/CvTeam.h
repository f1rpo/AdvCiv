#pragma once

#ifndef CIV4_TEAM_H
#define CIV4_TEAM_H

class CvArea;

// <advc.003u> Let the more powerful macros take precedence
#if !defined(CIV4_GAME_PLAY_H) && !defined(COREAI_H) && !defined(CIV4_TEAM_AI_H)
	#undef GET_TEAM // </advc.003u>
	#define GET_TEAM(x) CvTeam::getTeam(x)
#endif

class CvTeam /* advc.003e: */ : private boost::noncopyable
{
public:
	// <advc.003u>
#ifdef _DEBUG
	__forceinline // Annoying to step into by accident
#endif
	static CvTeam& getTeam(TeamTypes eTeam)
	{
		FAssertBounds(0, MAX_TEAMS, eTeam);
		// Needs to be inline and I don't want to include CvTeamAI.h here
		return *reinterpret_cast<CvTeam*>(m_aTeams[eTeam]);
	}
	// static functions moved from CvTeamAI
	static void initStatics();
	static void freeStatics(); // </advc.003u>

	// <kekm.26>
	// <!-- custom: Preserve why queued wars begin so SASGameRecord can distinguish direct declarations from alliance, defensive-pact, vassal, vote, and nuclear cascades. (GPT-5.6-Sol) -->
	static void queueWar(TeamTypes eAttackingTeam, TeamTypes eDefendingTeam, bool bNewDiplo, WarPlanTypes eWarPlan, bool bPrimaryDOW = true, WarDeclarationCause eCause = WAR_DECLARATION_DIRECT);
	static void triggerWars(/* advc: */ bool bForceUpdateAttitude = false);
	// </kekm.26>

	/*	advc (comment): Call order during (de-)initialization of CvTeam, CvPlayer:
		+	When Civ 4 is launched, constructors are called.
		+	When Civ 4 is exited, destructors are called.
		+	When starting a new game, init is called.
		+	When returning to the opening menu, reset is called.
		+	When saving a game, write is called.
		+	When loading a saved game, read is called.
		read, init and the constructor use reset to clear the data.
		(Reusing classes like this is error-prone and seems ill-advised in this case,
		but there's probably no changing it because the EXE is involved.) */
	explicit CvTeam(TeamTypes eID);
	virtual ~CvTeam();

	DllExport void init(TeamTypes eID);
	DllExport void reset(TeamTypes eID = NO_TEAM, bool bConstructorCall = false);

	void resetPlotAndCityData(); // BETTER_BTS_AI_MOD, 12/30/08, jdog5000
	// <!-- custom: Permanent Alliance absorption preserves the original survivor-member UWAI merge boundary and also migrates outsiders' persistent state keyed to the absorbed target team. See KI#539. See KI#551. (ChatGPT-5.6-Sol + GPT-5.6-Sol) -->
	void addTeam(TeamTypes eTeam);																								// Exposed to Python
	void shareItems(TeamTypes eTeam);
	void shareCounters(TeamTypes eTeam);
	void processBuilding(BuildingTypes eBuilding, int iChange);

	void doTurn();
	// <!-- custom: Dead teams receive no ordinary team turn, so the global-turn boundary invokes only their retained war-weariness aging. See KI#415. See KI#1055. (GPT-5.6-Sol) -->
	void doWarWeariness();

	void updateYield();
	void updatePowerHealth();
	void updateCommerce();

	bool canChangeWarPeace(TeamTypes eTeam, bool bAllowVassal = false) const;																			// Exposed to Python
	DllExport bool canDeclareWar(TeamTypes eTeam) const;																// Exposed to Python
	bool canEventuallyDeclareWar(TeamTypes eTeam) const; // bbai, Exposed to Python
	// advc.100
	// <!-- custom: hoisted from multiline signature between `eSponsor` and `bRandomEvent` by collapse_cpp_signatures.py. (GPT-5.5 (reviewed script output)) -->
	// <!-- custom: Added eCause so direct and queued war declarations retain their origin for SASGameRecord; defaults preserve all existing callers and Python exposure. (GPT-5.6-Sol) -->
	void declareWar(TeamTypes eTeam, bool bNewDiplo, WarPlanTypes eWarPlan, bool bPrimaryDoW = true, PlayerTypes eSponsor = NO_PLAYER, bool bRandomEvent = false, WarDeclarationCause eCause = WAR_DECLARATION_DIRECT); // advc.106g; K-Mod added bPrimaryDoW, Exposed to Python
	// advc.100b
	// <!-- custom: hoisted from multiline signature between `eBroker` and `bCapitulate` by collapse_cpp_signatures.py. (GPT-5.5 (reviewed script output)) -->
	// advc.034
	// <!-- custom: hoisted from multiline signature between `bCapitulate` and `pReparations` by collapse_cpp_signatures.py. (GPT-5.5 (reviewed script output)) -->
	// advc.039
	// <!-- custom: hoisted from multiline signature between `pReparations` and `bRandomEvent` by collapse_cpp_signatures.py. (GPT-5.5 (reviewed script output)) -->
	// <!-- custom: Carry the actual reparations-giving player so announcement formatting can resolve player-local city IDs on multi-member teams. See KI#412. (ChatGPT-5.6-Sol + GPT-5.6-Sol) -->
	void makePeace(TeamTypes eTarget, bool bBumpUnits = true, TeamTypes eBroker = NO_TEAM, bool bCapitulate = false, CLinkList<TradeData> const* pReparations = NULL, bool bRandomEvent = false, PlayerTypes eReparationsFrom = NO_PLAYER); // advc.106g; Exposed to Python
	bool canContact(TeamTypes eTeam, bool bCheckWillingness = false) const; // K-Mod, Exposed to Python
	void meet(TeamTypes eTeam, bool bNewDiplo, FirstContactData* pData = NULL); // advc.071; Exposed to Python
	void signPeaceTreaty(TeamTypes eTeam, bool bForce = false); // K-Mod (advc: bForce)
	void signOpenBorders(TeamTypes eTeam, /* advc.032: */ bool bProlong = false);																				// Exposed to Python
	void signDisengage(TeamTypes otherId); // advc.034
	void signDefensivePact(TeamTypes eTeam, /* advc.032: */ bool bProlong = false);																			// Exposed to Python
	bool canSignDefensivePact(TeamTypes eTeam) /* advc: */ const;

	int getAssets() const;																															// Exposed to Python
	int getPower(bool bIncludeVassals) const;																																// Exposed to Python
	int getDefensivePower(TeamTypes eExcludeTeam = NO_TEAM) const;						// Exposed to Python
	int getEnemyPower() const;
	int getNumNukeUnits() const;																												// Exposed to Python
	int getVotes(VoteTypes eVote, VoteSourceTypes eVoteSource) const;
	bool isVotingMember(VoteSourceTypes eVoteSource) const;
	bool isFullMember(VoteSourceTypes eVoteSource) const;

	/*  advc: Added a default value for bIgnoreMinors to all WarPlan functions.
		getAtWarCount renamed to getNumWars to avoid confusion with CvTeamAI::AI_getAtWarCounter. */
	// BETTER_BTS_AI_MOD, 01/10/09, jdog5000: bIgnoreVassals added
	int getNumWars(bool bIgnoreMinors = true, bool bIgnoreVassals = false) const;						// Exposed to Python
	// advc: Replaced by CvTeamAI::AI_countWarPlans
	//int getWarPlanCount(WarPlanTypes eWarPlan, bool bIgnoreMinors = true) const;
	int getHasMetCivCount(bool bIgnoreMinors = true) const;												// Exposed to Python

	// kekm.3
	// <!-- custom: hoisted from multiline signature between `eOther` and `bCheckBothWays` by collapse_cpp_signatures.py. (GPT-5.5 (reviewed script output)) -->
	// advc.130f: If false, check only if the war enemies of this team are included in those of otherId (set inclusion).
	// <!-- custom: hoisted from multiline signature between `eOther` and `bCheckBothWays` by collapse_cpp_signatures.py. (GPT-5.5 (reviewed script output)) -->
	bool allWarsShared(TeamTypes eOther, bool bCheckBothWays = true) const;
	bool hasMetHuman() const;																			// Exposed to Python
	bool isInContactWithBarbarians() const; // advc.302
	int getDefensivePactCount(TeamTypes eObs = NO_TEAM) const;											// Exposed to Python
	int getVassalCount(TeamTypes eObs = NO_TEAM) const;
	// advc.opt:
	bool isAVassal() const { return (m_eMaster != NO_TEAM); }											// Exposed to Python
	bool canVassalRevolt(TeamTypes eMaster, bool bCheckLosses = true, int iExtraLand = 0, int iExtraPop = 0) const; // advc.ctr
	bool isLossesAllowRevolt(TeamTypes eMaster) const; // advc.112
	int getUnitClassMaking(UnitClassTypes eUnitClass) const;											// Exposed to Python
	int getUnitClassCountPlusMaking(UnitClassTypes eIndex) const { return getUnitClassCount(eIndex) + getUnitClassMaking(eIndex); } // Exposed to Python
	int getBuildingClassMaking(BuildingClassTypes eBuildingClass) const;								// Exposed to Python
	int getBuildingClassCountPlusMaking(BuildingClassTypes eIndex) const { return getBuildingClassCount(eIndex) + getBuildingClassMaking(eIndex); } // Exposed to Python
	int getHasReligionCount(ReligionTypes eReligion) const;												// Exposed to Python
	int getHasCorporationCount(CorporationTypes eCorporation) const;									// Exposed to Python

	int countTotalCulture() const;																		// Exposed to Python

	int countNumUnitsByArea(CvArea const& kArea) const;													// Exposed to Python
	int countNumCitiesByArea(CvArea const& kArea) const;												// Exposed to Python
	int countTotalPopulationByArea(CvArea const& kArea) const;											// Exposed to Python
	int countPowerByArea(CvArea const& kArea) const;													// Exposed to Python
	int countNumAIUnitsByArea(CvArea const& kArea, UnitAITypes eUnitAI) const;							// Exposed to Python
	// (advc: countEnemyDangerByArea moved to CvTeamAI)
	EraTypes getCurrentEra() const; // advc.112b
	// K-Mod:
	int getTypicalUnitValue(UnitAITypes eUnitAI, DomainTypes eDomain = NO_DOMAIN) const;

	// bool bGlobalModifiers = true, // K-Mod
	// <!-- custom: hoisted from multiline signature between `eTech` and `bFreeBarbarianResearch` by collapse_cpp_signatures.py. (GPT-5.5 (reviewed script output)) -->
	// advc.301
	// <!-- custom: hoisted from multiline signature between `bFreeBarbarianResearch` and `bTeamSizeModifiers` by collapse_cpp_signatures.py. (GPT-5.5 (reviewed script output)) -->
	int getResearchCost(TechTypes eTech, bool bFreeBarbarianResearch = false, bool bTeamSizeModifiers = true) const; // K-Mod; Exposed to Python
	int getResearchLeft(TechTypes eTech) const;																// Exposed to Python

	bool hasHolyCity(ReligionTypes eReligion) const;																		// Exposed to Python
	bool hasHeadquarters(CorporationTypes eCorporation) const;																		// Exposed to Python
	bool hasBonus(BonusTypes eBonus) const;
	bool isBonusObsolete(BonusTypes eBonus) const;

	bool isHuman() const;													// Exposed to Python
	// advc: see CvPlayer::isActive
	bool isActive() const { return (GC.getInitCore().getActiveTeam() == getID()); }
	// advc: (The Barbarians aren't a proper civ)
	bool isMajorCiv() const { return (!isBarbarian() && !isMinorCiv()); }
	bool isBarbarian() const { return (m_eID == BARBARIAN_TEAM); }								// Exposed to Python
	// <advc.003m> cached
	bool isMinorCiv() const { return m_bMinorTeam; }											// Exposed to Python
	void updateMinorCiv() { m_bMinorTeam = checkMinorCiv(); }
	// </advc.003m>  <advc.opt> This gets called a lot. Now precomputed.
	PlayerTypes getLeaderID() const { return m_eLeader; }																					// Exposed to Python
	void updateLeaderID(); // </advc.opt>
	bool isAlwaysWar() const; // advc.127

	PlayerTypes getSecretaryID() const;																									// Exposed to Python
	HandicapTypes getHandicapType() const;																							// Exposed to Python
	CvWString getName() const;																								// Exposed to Python
	CvWString getReplayName() const; // K-Mod

	DllExport int getNumMembers() const { return m_iNumMembers; }										// Exposed to Python
	void changeNumMembers(int iChange);

	// advc.inl: In-line definitions for most of the get..Count and is... functions below
	int getAliveCount() const { return m_iAliveCount; } // advc.155: Exposed to Python
	bool isAlive() const { return (m_iAliveCount > 0); }														// Exposed to Python
	void changeAliveCount(int iChange);
	PlayerTypes getRandomMemberAlive(bool bHuman) const; // advc.104

	int getEverAliveCount() const { return m_iEverAliveCount; }
	bool isEverAlive() const { return (getEverAliveCount() > 0); } // advc: return type was int					// Exposed to Python
	void changeEverAliveCount(int iChange);

	int getNumCities() const { return m_iNumCities; }															// Exposed to Python
	void changeNumCities(int iChange);
	// <!-- custom: Refresh every master member's maintenance only after this vassal's authoritative player-city container reaches its final state. See KI#789. See KI#1035. (GPT-5.6-Sol) -->
	void updateMasterMaintenance() const;

	int getTotalPopulation(bool bCheckVassals = true) const;																											// Exposed to Python
	void changeTotalPopulation(int iChange);

	int getTotalLand(bool bCheckVassals = true) const;																														// Exposed to Python
	void changeTotalLand(int iChange);

	int getNukeInterception() const;																										// Exposed to Python
	void changeNukeInterception(int iChange);																			// Exposed to Python

	int getForceTeamVoteEligibilityCount(VoteSourceTypes eVoteSource) const;																				// Exposed to Python
	bool isForceTeamVoteEligible(VoteSourceTypes eVoteSource) const;																								// Exposed to Python
	void changeForceTeamVoteEligibilityCount(VoteSourceTypes eVoteSource, int iChange);												// Exposed to Python

	int getExtraWaterSeeFromCount() const { return m_iExtraWaterSeeFromCount; }																		// Exposed to Python
	bool isExtraWaterSeeFrom() const { return (getExtraWaterSeeFromCount() > 0); }																						// Exposed to Python
	void changeExtraWaterSeeFromCount(int iChange);																// Exposed to Python

	int getMapTradingCount() const { return m_iMapTradingCount; }																					// Exposed to Python
	bool isMapTrading() const { return (getMapTradingCount() > 0); }																					// Exposed to Python
	void changeMapTradingCount(int iChange);																			// Exposed to Python

	int getTechTradingCount() const { return m_iTechTradingCount; }																	// Exposed to Python
	bool isTechTrading() const { return (getTechTradingCount() > 0); }																	// Exposed to Python
	void changeTechTradingCount(int iChange);																			// Exposed to Python

	int getGoldTradingCount() const { return m_iGoldTradingCount; }																// Exposed to Python
	bool isGoldTrading() const { return (getGoldTradingCount() > 0); }																					// Exposed to Python
	void changeGoldTradingCount(int iChange);																			// Exposed to Python

	int getOpenBordersTradingCount() const { return m_iOpenBordersTradingCount; }															// Exposed to Python
	bool isOpenBordersTrading() const { return (getOpenBordersTradingCount() > 0); }										// Exposed to Python
	void changeOpenBordersTradingCount(int iChange);															// Exposed to Python

	int getDefensivePactTradingCount() const { return m_iDefensivePactTradingCount; }										// Exposed to Python
	bool isDefensivePactTrading() const { return (getDefensivePactTradingCount() > 0); }															// Exposed to Python
	void changeDefensivePactTradingCount(int iChange);														// Exposed to Python

	int getPermanentAllianceTradingCount() const { return m_iPermanentAllianceTradingCount; }										// Exposed to Python
	bool isPermanentAllianceTrading() const;																						// Exposed to Python
	void changePermanentAllianceTradingCount(int iChange);												// Exposed to Python

	int getVassalTradingCount() const { return m_iVassalTradingCount; }														// Exposed to Python
	bool isVassalStateTrading() const;																						// Exposed to Python
	void changeVassalTradingCount(int iChange);												// Exposed to Python

	int getBridgeBuildingCount() const { return m_iBridgeBuildingCount; }																	// Exposed to Python
	bool isBridgeBuilding() const { return (getBridgeBuildingCount() > 0); }												// Exposed to Python
	void changeBridgeBuildingCount(int iChange);																			// Exposed to Python

	int getIrrigationCount() const { return m_iIrrigationCount; }																			// Exposed to Python
	bool isIrrigation() const { return (getIrrigationCount() > 0); }														// Exposed to Python
	void changeIrrigationCount(int iChange);																			// Exposed to Python

	int getIgnoreIrrigationCount() const { return m_iIgnoreIrrigationCount; }																// Exposed to Python
	bool isIgnoreIrrigation() const { return (getIgnoreIrrigationCount() > 0); }														// Exposed to Python
	void changeIgnoreIrrigationCount(int iChange);																// Exposed to Python

	int getWaterWorkCount() const { return m_iWaterWorkCount; }																	// Exposed to Python
	bool isWaterWork() const { return (getWaterWorkCount() > 0); }																		// Exposed to Python
	void changeWaterWorkCount(int iChange);																				// Exposed to Python

	int getVassalPower() const { return m_iVassalPower; }																// Exposed to Python
	void setVassalPower(int iPower) { m_iVassalPower = iPower; }														// Exposed to Python
	int getMasterPower() const { return m_iMasterPower; }																// Exposed to Python
	void setMasterPower(int iPower) { m_iMasterPower = iPower; }														// Exposed to Python

	int getEnemyWarWearinessModifier() const { return m_iEnemyWarWearinessModifier; }																								// Exposed to Python
	void changeEnemyWarWearinessModifier(int iChange);																	// Exposed to Python
	void changeWarWeariness(TeamTypes eOtherTeam, const CvPlot& kPlot, int iFactor);

	bool isMapCentering() const { return m_bMapCentering; }																	// Exposed to Python
	void setMapCentering(bool bNewValue);																					// Exposed to Python

	TeamTypes getID() const { return m_eID; }														// Exposed to Python

	int getStolenVisibilityTimer(TeamTypes eIndex) const { return m_aiStolenVisibilityTimer.get(eIndex); }
	bool isStolenVisibility(TeamTypes eIndex) const { return (getStolenVisibilityTimer(eIndex) > 0); }																		// Exposed to Python
	void setStolenVisibilityTimer(TeamTypes eIndex, int iNewValue);
	void changeStolenVisibilityTimer(TeamTypes eIndex, int iChange);

	int getWarWeariness(TeamTypes eIndex, bool bUseEnemyModifer = false) const; // K-Mod; Exposed to Python
	void setWarWeariness(TeamTypes eIndex, int iNewValue);												// Exposed to Python
	void changeWarWeariness(TeamTypes eIndex, int iChange);												// Exposed to Python
	/*	<advc> (for kekm.38) Params changed to PlayerTypes; were named "eIndex".
		NB: This function says how many team members have a tech share effect
		that gets triggered when eSharePlayers other players know a tech. */
	int getTechShareCount(PlayerTypes eSharePlayers) const { return m_aiTechShareCount.get(eSharePlayers); } // Exposed to Python
	bool isTechShare(PlayerTypes eSharePlayers) const { return (getTechShareCount(eSharePlayers) > 0); } // Exposed to Python
	bool isAnyTechShare() const { return m_aiTechShareCount.isAnyNonDefault(); } // advc.opt
	void changeTechShareCount(PlayerTypes eSharePlayers, int iChange); // </advc>						// Exposed to Python

	int getCommerceFlexibleCount(CommerceTypes eIndex) const;														// Exposed to Python
	bool isCommerceFlexible(CommerceTypes eIndex) const;																// Exposed to Python
	void changeCommerceFlexibleCount(CommerceTypes eIndex, int iChange);					// Exposed to Python

	int getExtraMoves(DomainTypes eIndex) const;																				// Exposed to Python
	void changeExtraMoves(DomainTypes eIndex, int iChange);								// Exposed to Python

	// advc.091
	bool isHasMet(TeamTypes eOther) const { return (m_aiHasMetTurn.get(eOther) >= 0); } // Exposed to Python
	int getHasMetTurn(TeamTypes eOther) const { return m_aiHasMetTurn.get(eOther); } // advc.091  (exposed to Python)
	// advc.071: Return value, 2nd param added.
	CvPlot* makeHasMet(TeamTypes eOther, bool bNewDiplo, FirstContactData* pData = NULL);
	bool isHasSeen(TeamTypes eOther) const { return m_abHasSeen.get(eOther); }; // K-Mod
	void makeHasSeen(TeamTypes eOther) { m_abHasSeen.set(eOther, true); }; // K-Mod
	// <advc.134a>
	bool isAtWarExternal(TeamTypes eIndex) const; // Exported through .def file
	bool isAtWar(TeamTypes eIndex) const { return m_abAtWar.get(eIndex); } // Exposed to Python // </advc.134a>
	void setAtWar(TeamTypes eIndex, bool bNewValue);
	/*  advc.162: "Just" meaning on the current turn. Don't want to rely on
		AI code (AI_getWarPlanStateCounter) for this.
		Also used for advc.010. */
	bool hasJustDeclaredWar(TeamTypes eIndex) const { return m_abJustDeclaredWar.get(eIndex); }
	// <advc.130k> Replacing CvTeamAI::m_aiAtPeaceCounter, which is now randomized.
	int getTurnsAtPeace(TeamTypes eTeam) const { return m_aiTurnsAtPeace.get(eTeam); }
	void changeTurnsAtPeace(TeamTypes eTeam, int iChange) { setTurnsAtPeace(eTeam, getTurnsAtPeace(eTeam) + iChange); }
	void setTurnsAtPeace(TeamTypes eTeam, int iTurns); // </advc.130k>

	bool isPermanentWarPeace(TeamTypes eIndex) const { return m_abPermanentWarPeace.get(eIndex); } // Exposed to Python
	void setPermanentWarPeace(TeamTypes eIndex, bool bNewValue);									// Exposed to Python

	bool canTradeWith(TeamTypes eWhoTo) const; // advc
	bool isFreeTrade(TeamTypes eIndex) const;																	// Exposed to Python
	bool isOpenBorders(TeamTypes eIndex) const { return m_abOpenBorders.get(eIndex); } // Exposed to Python
	void setOpenBorders(TeamTypes eIndex, bool bNewValue);
	// <advc.034>
	bool isDisengage(TeamTypes eIndex) const { return m_abDisengage.get(eIndex); }
	void setDisengage(TeamTypes eIndex, bool bNewValue);
	void cancelDisengage(TeamTypes otherId);
	// </advc.034>
	bool isDefensivePact(TeamTypes eIndex) const { return m_abDefensivePact.get(eIndex); }						// Exposed to Python
	void setDefensivePact(TeamTypes eIndex, bool bNewValue);

	bool isForcePeace(TeamTypes eIndex) const { return m_abForcePeace.get(eIndex); }							// Exposed to Python
	void setForcePeace(TeamTypes eIndex, bool bNewValue);
	int turnsOfForcedPeaceRemaining(TeamTypes eOther) const; // advc.104

	// advc.opt
	bool isVassal(TeamTypes eMaster) const { return (m_eMaster == eMaster); } // Exposed to Python
	void setVassal(TeamTypes eMaster, bool bNewValue, bool bCapitulated);
	// K-Mod. Return the team which is the master of this team. (if this team is free, return getID())
	// advc.opt
	TeamTypes getMasterTeam() const { return (m_eMaster == NO_TEAM ? getID() : m_eMaster); } // advc.155: Exposed to Python
	void assignVassal(TeamTypes eVassal, bool bSurrender) const;											// Exposed to Python
	void freeVassal(TeamTypes eVassal) const;																// Exposed to Python

	bool isCapitulated() const { return m_bCapitulated; } // advc.130v: Exposed to Python // <advc>
	bool isCapitulated(TeamTypes eMaster) { return (isCapitulated() && isVassal(eMaster)); } // </advc>
	int getCapitulationTurn() const; // advc.130w
	int getRouteChange(RouteTypes eIndex) const { return m_aiRouteChange.get(eIndex); } // Exposed to Python
	void changeRouteChange(RouteTypes eIndex, int iChange);												// Exposed to Python

	int getProjectCount(ProjectTypes eIndex) const { return m_aiProjectCount.get(eIndex); } // Exposed to Python
	DllExport int getProjectDefaultArtType(ProjectTypes eIndex) const;
	DllExport void setProjectDefaultArtType(ProjectTypes eIndex, int iValue);
	DllExport int getProjectArtType(ProjectTypes eProject, int iIndex) const;
	DllExport void setProjectArtType(ProjectTypes eProject, int iIndex, int iValue);
	bool isProjectMaxedOut(ProjectTypes eProject, int iExtra = 0) const;								// Exposed to Python
	DllExport bool isProjectAndArtMaxedOut(ProjectTypes eProject) const;
	// <!-- custom: Allow Permanent-Alliance inheritance to apply project mechanics without announcing an already-completed project as new. See KI#422. (ChatGPT-5.6-Sol + GPT-5.6-Sol) -->
	void changeProjectCount(ProjectTypes eProject, int iChange, bool bAnnounceCompletion = true);			// Exposed to Python
	DllExport void finalizeProjectArtTypes();
	int getProjectMaking(ProjectTypes eProject) const { return m_aiProjectMaking.get(eProject); } // Exposed to Python
	void changeProjectMaking(ProjectTypes eProject, int iChange);

	int getUnitClassCount(UnitClassTypes eIndex) const { return m_aiUnitClassCount.get(eIndex); } // Exposed to Python
	bool isUnitClassMaxedOut(UnitClassTypes eIndex, int iExtra = 0) const;							// Exposed to Python
	void changeUnitClassCount(UnitClassTypes eIndex, int iChange);

	int getBuildingClassCount(BuildingClassTypes eIndex) const { return m_aiBuildingClassCount.get(eIndex); } // Exposed to Python
	bool isBuildingClassMaxedOut(BuildingClassTypes eIndex, int iExtra = 0) const;			// Exposed to Python
	void changeBuildingClassCount(BuildingClassTypes eIndex, int iChange);

	int getObsoleteBuildingCount(BuildingTypes eIndex) const { return m_aiObsoleteBuildingCount.get(eIndex); }
	bool isObsoleteBuilding(BuildingTypes eIndex) const { return (getObsoleteBuildingCount(eIndex) > 0); } // Exposed to Python
	void changeObsoleteBuildingCount(BuildingTypes eIndex, int iChange);

	int getResearchProgress(TechTypes eIndex) const;																						// Exposed to Python
	// <!-- custom: Added eCause to setResearchProgress, changeResearchProgress and changeResearchProgressPercent.
	// Carry the final source through research-progress completion so GameRecord does not mislabel Great-Person, event, Barbarian, or scripted progress as ordinary research; defaults preserve existing Python/script calls as UNKNOWN. (GPT-5.6-Sol + GPT-5.6 Thinking) -->
	void setResearchProgress(TechTypes eIndex, int iNewValue, PlayerTypes ePlayer, TechAcquisitionCause eCause = TECH_ACQUISITION_UNKNOWN); // Exposed to Python
	void changeResearchProgress(TechTypes eIndex, int iChange, PlayerTypes ePlayer, TechAcquisitionCause eCause = TECH_ACQUISITION_UNKNOWN); // Exposed to Python
	int changeResearchProgressPercent(TechTypes eIndex, int iPercent, PlayerTypes ePlayer, TechAcquisitionCause eCause = TECH_ACQUISITION_UNKNOWN);

	int getTechCount(TechTypes eIndex) const;																										// Exposed to Python
	// BETTER_BTS_AI_MOD, General AI, 07/27/09, jdog5000:
	int getBestKnownTechScorePercent() const;

	int getTerrainTradeCount(TerrainTypes eIndex) const { return m_aiTerrainTradeCount.get(eIndex); }
	bool isTerrainTrade(TerrainTypes eIndex) const { return (getTerrainTradeCount(eIndex) > 0); } // Exposed to Python
	void changeTerrainTradeCount(TerrainTypes eIndex, int iChange);

	int getRiverTradeCount() const { return m_iRiverTradeCount; }
	//return (getRiverTradeCount() > 0);
	// advc.124
	bool isRiverTrade() const { return true; } // Exposed to Python
	void changeRiverTradeCount(int iChange);
	// advc.500c:
	int getNoMilitaryAnger() const { return (m_iNoFearForSafetyCount > 0 ? 0 : GC.getDefineINT(CvGlobals::NO_MILITARY_PERCENT_ANGER)); }

	int getVictoryCountdown(VictoryTypes eVictory) const { return m_aiVictoryCountdown.get(eVictory); } // Exposed to Python
	void setVictoryCountdown(VictoryTypes eVictory, int iTurnsLeft);
	void changeVictoryCountdown(VictoryTypes eVictory, int iChange);
	bool isAnyVictoryCountdown() const { return m_aiVictoryCountdown.isAnyNonDefault(); } // advc.opt
	int getVictoryDelay(VictoryTypes eVictory) const;
	bool canLaunch(VictoryTypes eVictory) const;									// Exposed to Python
	void setCanLaunch(VictoryTypes eVictory, bool bCan);
	int getLaunchSuccessRate(VictoryTypes eVictory) const;								// Exposed to Python
	void resetVictoryProgress();
	bool hasSpaceshipArrived() const; // K-Mod, Exposed to Python

	bool isParent(TeamTypes eChildTeam) const;

	bool isHasTech(TechTypes eIndex) const;																																			// Exposed to Python
	// <!-- custom: Added eCause to setHasTech for exact GameRecord provenance; it does not affect technology rules, and legacy/Python callers default safely to UNKNOWN. (GPT-5.6-Sol + GPT-5.6 Thinking) -->
	void setHasTech(TechTypes eTech, bool bNewValue, PlayerTypes ePlayer, bool bFirst, bool bAnnounce, /* advc.121: */ bool bEndOfTurn = false, TechAcquisitionCause eCause = TECH_ACQUISITION_UNKNOWN); // Exposed to Python
	/* advc.004a: A hack that allows other classes to pretend that a team knows
	   a tech for some computation. Should be toggled back afterwards. */
	void setHasTechTemporarily(TechTypes eTech, bool b) { m_abHasTech.set(eTech, b); }
	int getTechCount() const { return m_iTechCount; } // advc.101
	// <advc.134a>
	void advancePeaceOfferStage(TeamTypes eAITeam = NO_TEAM);
	bool isPeaceOfferStage(int iStage, TeamTypes eOffering) const;
	// </advc.134a>
	bool isNoTradeTech(TechTypes eIndex) const;																														// Exposed to Python
	void setNoTradeTech(TechTypes eIndex, bool bNewValue);																					// Exposed to Python

	int getImprovementYieldChange(ImprovementTypes eIndex1, YieldTypes eIndex2) const { return m_aaiImprovementYieldChange.get(eIndex1, eIndex2); } // Exposed to Python
	void changeImprovementYieldChange(ImprovementTypes eIndex1, YieldTypes eIndex2, int iChange);		// Exposed to Python

	bool doesImprovementConnectBonus(ImprovementTypes eImprovement, BonusTypes eBonus) const; // K-Mod
	// advc.opt:
	bool canPeacefullyEnter(TeamTypes eTerritoryOwner) const
	{
		return (isOpenBorders(eTerritoryOwner) || //isFriendlyTerritory(eTerritoryOwner)
				// (The above checks too much stuff that we don't need)
				getID() == eTerritoryOwner || getTeam(eTerritoryOwner).isVassal(getID()));
	}
	bool isFriendlyTerritory(TeamTypes eTerritoryOwner) const;
	bool isAlliedTerritory(TeamTypes eTerritoryOwner, TeamTypes eEnemy) const; // advc.183
	void updateActivePaths(TeamTypes eOtherTeam = NO_TEAM); // advc.001w
	// <advc> Same as isRevealedBase (but doesn't have to be)
	bool isRevealedAirBase(CvPlot const& kPlot) const { return isRevealedBase(kPlot); }
	bool isRevealedCityHeal(CvPlot const& kPlot) const { return isRevealedBase(kPlot); }
	bool isRevealedCityTrade(CvPlot const& kPlot) const { return isRevealedBase(kPlot); }
	bool isRevealedBase(CvPlot const& kPlot) const; 
	// Same as isBase (but doesn't have to be)
	bool isAirBase(CvPlot const& kPlot) const { return isBase(kPlot); }
	bool isCityHeal(CvPlot const& kPlot) const { return isBase(kPlot); }
	bool isBase(CvPlot const& kPlot) const;
	bool isCityDefense(CvPlot const& kPlot, TeamTypes eAttacker = NO_TEAM) const; // </advc>
	bool canAccessHappyHealth(CvPlot const& kPlot, int iHealthOrHappy) const; // advc.901

	int getEspionageModifier(TeamTypes eTarget) const;								// Exposed to Python (though CyGameCoreUtils)
	int getEspionagePointsAgainstTeam(TeamTypes eIndex) const;																							// Exposed to Python
	void setEspionagePointsAgainstTeam(TeamTypes eIndex, int iValue);																							// Exposed to Python
	void changeEspionagePointsAgainstTeam(TeamTypes eIndex, int iChange);																				// Exposed to Python
	bool canSeeTech(TeamTypes eOther) const; // advc.120d

	int getTotalUnspentEspionage() const; // K-Mod

	int getEspionagePointsEver() const;																							// Exposed to Python
	void setEspionagePointsEver(int iValue);																							// Exposed to Python
	void changeEspionagePointsEver(int iChange);																				// Exposed to Python

	int getCounterespionageTurnsLeftAgainstTeam(TeamTypes eIndex) const;																							// Exposed to Python
	void setCounterespionageTurnsLeftAgainstTeam(TeamTypes eIndex, int iValue);																		// Exposed to Python
	void changeCounterespionageTurnsLeftAgainstTeam(TeamTypes eIndex, int iChange);																// Exposed to Python

	int getCounterespionageModAgainstTeam(TeamTypes eIndex) const;																							// Exposed to Python
	void setCounterespionageModAgainstTeam(TeamTypes eIndex, int iValue);																		// Exposed to Python
	void changeCounterespionageModAgainstTeam(TeamTypes eIndex, int iChange);																// Exposed to Python

	void verifySpyUnitsValidPlot();

	void setForceRevealedBonus(BonusTypes eBonus, bool bRevealed);
	bool isForceRevealedBonus(BonusTypes eBonus) const { return m_abRevealedBonuses.get(eBonus); }
	bool isBonusRevealed(BonusTypes eBonus) const; // K-Mod. (the definitive answer)
	bool canDiscoverBonus(BonusTypes eBonus) const; // advc

	void revealSurroundingPlots(CvPlot const& kCenter, int iRange) const; // advc.108

	int countNumHumanGameTurnActive() const;
	void setTurnActive(bool bNewValue, bool bTurn = true);
	bool isTurnActive() const;

	bool hasShrine(ReligionTypes eReligion) const;

	DllExport void getCompletedSpaceshipProjects(std::map<ProjectTypes, int>& mapProjects) const;
	DllExport int getProjectPartNumber(ProjectTypes projectType, bool bAssert) const;
	DllExport bool hasLaunched() const;
	// advc:
	bool hasTechToClear(FeatureTypes eFeature, TechTypes eCurrentResearch = NO_TECH) const;
	void testCircumnavigated(); // advc.136a: Made public
	/*  <advc.127b> Both return -1 if no team member has a capital or
		(eObserver!=NO_TEAM) if none is revealed to eObserver. */
	int getCapitalX(TeamTypes eObserver, bool bDebug = false) const;
	int getCapitalY(TeamTypes eObserver, bool bDebug = false) const;
	CvCity* getLeaderCapital(TeamTypes eObserver, bool bDebug = false) const;
	// </advc.127b>
	void finalizeInit(); // advc.003m
	// <advc.003u>
	//return *static_cast<CvTeamAI*>(const_cast<CvTeam*>(this));
	/* The above won't work in an inline function b/c the compiler doesn't know that CvTeamAI is derived from CvTeam */
	CvTeamAI& AI() { return *reinterpret_cast<CvTeamAI*>(this); }
	//return *static_cast<CvTeamAI const*>(this);
	CvTeamAI const& AI() const { return *reinterpret_cast<CvTeamAI const*>(this); } // </advc.003u>

protected:
	virtual void read(FDataStreamBase* pStream);
	virtual void write(FDataStreamBase* pStream);
	// advc.003u: Keep one pure virtual function so that this class is abstract
	virtual void AI_makeAssignWorkDirty() = 0;
	// advc.003u: See the comments in the private section of CvPlayer.h before adding any virtual functions!

	TeamTypes m_eID; // advc: Moved up for easier access in the debugger

	static CvTeamAI** m_aTeams; // advc.003u: Moved from CvTeamAI.h; and store only pointers.

	int m_iNumMembers;
	int m_iAliveCount;
	int m_iEverAliveCount;
	int m_iNumCities;
	int m_iTotalPopulation;
	int m_iTotalLand;
	int m_iNukeInterception;
	int m_iExtraWaterSeeFromCount;
	int m_iMapTradingCount;
	int m_iTechTradingCount;
	int m_iGoldTradingCount;
	int m_iOpenBordersTradingCount;
	int m_iDefensivePactTradingCount;
	int m_iPermanentAllianceTradingCount;
	int m_iVassalTradingCount;
	int m_iBridgeBuildingCount;
	int m_iIrrigationCount;
	int m_iIgnoreIrrigationCount;
	int m_iWaterWorkCount;
	int m_iVassalPower;
	int m_iMasterPower;
	int m_iEnemyWarWearinessModifier;
	int m_iRiverTradeCount;
	int m_iNoFearForSafetyCount; // advc.500c
	int m_iEspionagePointsEver;
	// <advc.003m>
	int m_iMajorWarEnemies; // incl. vassals
	int m_iMinorWarEnemies;
	int m_iVassalWarEnemies;
	// <advc.opt>
	TeamTypes m_eMaster;
	PlayerTypes m_eLeader;
	// </advc.opt>
	short m_iTechCount; // advc.101
	bool m_bMinorTeam;
	// </advc.003m>
	bool m_bMapCentering;
	bool m_bCapitulated;

	/*	<advc.enum> (See comment in CvPlayer header about lazy allocation.) */
	// (This is only used for the obsolete steal-plans spy mission)
	ArrayEnumMap<TeamTypes,int,char> m_aiStolenVisibilityTimer;
	ArrayEnumMap<TeamTypes,int> m_aiWarWeariness;
	ArrayEnumMap<TeamTypes,int> m_aiEspionagePointsAgainstTeam;
	ListEnumMap<TeamTypes,int,short> m_aiCounterespionageTurnsLeftAgainstTeam;
	ListEnumMap<TeamTypes,int,short> m_aiCounterespionageModAgainstTeam;
	ArrayEnumMap<TeamTypes,int,short> m_aiTurnsAtPeace; // advc.130k
	ArrayEnumMap<PlayerTypes,int,char> m_aiTechShareCount;

	CommerceChangeMap m_aiCommerceFlexibleCount;
	ArrayEnumMap<DomainTypes,int,char> m_aiExtraMoves;
	ArrayEnumMap<VoteSourceTypes,int,char> m_aiForceTeamVoteEligibilityCount;
	ArrayEnumMap<RouteTypes,int,char> m_aiRouteChange;

	ArrayEnumMap<ProjectTypes,int,short> m_aiProjectCount;
	ArrayEnumMap<ProjectTypes,int,short> m_aiProjectMaking;
	/*	Could create an enum ProjectArtTypes { NO_PROJECTART=-1 } for this,
		but DLLExports make that more trouble than it's worth. */
	ArrayEnumMap<ProjectTypes,int> m_aiProjectDefaultArtTypes;
	ArrayEnumMap<UnitClassTypes,int,short> m_aiUnitClassCount;
	ArrayEnumMap<BuildingClassTypes,int,short> m_aiBuildingClassCount;
	ArrayEnumMap<BuildingTypes,int,char> m_aiObsoleteBuildingCount;
	ArrayEnumMap<TechTypes,int> m_aiResearchProgress;
	ArrayEnumMap<TechTypes,int,char> m_aiTechCount;
	ArrayEnumMap<TerrainTypes,int,char> m_aiTerrainTradeCount;
	ArrayEnumMap<VictoryTypes,int,short,-1> m_aiVictoryCountdown;
	ArrayEnumMap<TeamTypes,int,short,-1> m_aiHasMetTurn; // advc.091

	Enum2IntEncMap<ArrayEnumMap<ImprovementTypes,YieldChangeMap::enc_t>,
			YieldChangeMap> m_aaiImprovementYieldChange;

	ArrayEnumMap<TeamTypes,bool> m_abAtWar;
	ArrayEnumMap<TeamTypes,bool> m_abJustDeclaredWar; // advc.162
	ArrayEnumMap<TeamTypes,bool> m_abHasSeen; // K-Mod
	ArrayEnumMap<TeamTypes,bool> m_abPermanentWarPeace;
	ArrayEnumMap<TeamTypes,bool> m_abOpenBorders;
	ArrayEnumMap<TeamTypes,bool> m_abDisengage; // advc.034
	ArrayEnumMap<TeamTypes,bool> m_abDefensivePact;
	ArrayEnumMap<TeamTypes,bool> m_abForcePeace;
	//EnumMap<TeamTypes,bool> m_abVassal; // advc.opt: Replaced by m_eMaster
	ArrayEnumMap<VictoryTypes,bool> m_abCanLaunch;
	ArrayEnumMap<TechTypes,bool> m_abHasTech;
	ArrayEnumMap<TechTypes,bool> m_abNoTradeTech;
	// a vector for each type of project (advc: was an array of vectors)
	std::vector<std::vector<int> > m_aaiProjectArtTypes;
	ArrayEnumMap<BonusTypes,bool> m_abRevealedBonuses; // was vector<BonusTypes>
	// </advc.enum>
	// <advc.134a>
	mutable TeamTypes m_eOfferingPeace;
	mutable int m_iPeaceOfferStage;
	// </advc.134a>

	// <kekm.26>
	static std::queue<TeamTypes> attacking_queue;
	static std::queue<TeamTypes> defending_queue;
	static std::queue<bool> newdiplo_queue;
	static std::queue<WarPlanTypes> warplan_queue;
	static std::queue<bool> primarydow_queue;
	static std::queue<WarDeclarationCause> warcause_queue;
	static bool bTriggeringWars;
	// </kekm.26>

	void doBarbarianResearch(); // advc
	void updateTechShare(TechTypes eTech, /* advc.opt: */ int iOtherKnownThreshold = -1);
	void updateTechShare();
	void updateMilitaryHappinessUnits(); // advc.184
	int calculateBestTechShare() const; // advc.opt
	void updatePlotGroupBonus(TechTypes eTech, bool bAdd); // advc

	void processTech(TechTypes eTech, int iChange, /* advc.121: */ bool bEndOfTurn);
	void changeNoFearForSafetyCount(int iChange); // advc.500c

	void triggerDefensivePacts(TeamTypes eTarget, bool bNewDiplo, bool bPrimary); // advc
	void cancelDefensivePacts();
	void allowDefensivePactsToBeCanceled(); // kekm.3
	// <advc.003m>
	// New name for BBAI's getAtWarCount
	int countWarEnemies(bool bIgnoreMinors = true, bool bIgnoreVassals = false) const;
	void changeAtWarCount(int iChange, bool bMinorTeam, bool bVassal);
	// New name for isMinorCiv (uncached)
	bool checkMinorCiv() const; // </advc.003m>
	// <advc.039>
	// <!-- custom: Trade-item formatting needs the giving player for player-local city IDs; team-global items derive the team from that player. See KI#412. (ChatGPT-5.6-Sol + GPT-5.6-Sol) -->
	CvWString const tradeItemString(TradeableItems eItem, int iData, PlayerTypes eFrom) const; // </advc.039>
	bool isTechSplash() const; // advc
	// advc.156
	// <!-- custom: hoisted from multiline signature between `eDiscoverPlayer` and `bPartial` by collapse_cpp_signatures.py. (GPT-5.5 (reviewed script output)) -->
	void announceTechToPlayers(TechTypes eIndex, PlayerTypes eDiscoverPlayer, bool bPartial = false);
	// <advc>
	void announceWar(TeamTypes eTarget, bool bPrimaryDoW, PlayerTypes eSponsor = NO_PLAYER, bool bRandomEvent = false);
	// <!-- custom: Retain the actual reparations-giving player through announcement formatting. See KI#412. (ChatGPT-5.6-Sol + GPT-5.6-Sol) -->
	void announcePeace(TeamTypes eTarget, TeamTypes eBroker = NO_TEAM, CLinkList<TradeData> const* pReparations = NULL, bool bRandomEvent = false, PlayerTypes eReparationsFrom = NO_PLAYER);
	// </advc>  <advc.106o>
	void setWarPeacePartyStrings(TeamTypes eAgent, TeamTypes eTarget, CvWString& szAgents, CvWString& szTargets, bool bReplay = false);
	void setWarPeacePartyStrings(TeamTypes eTeam, CvWString& szTeams, bool bReplay, bool bCapitalize);
	// </advc.106o>

private: // advc.003u: (See comments in the private section of CvPlayer.h)
	/*virtual void AI_initExternal();
	virtual void AI_resetExternal(bool bConstructor);
	virtual void AI_doTurnPreExternal();*/
	virtual void AI_doTurnPostExternal();
	virtual void AI_makeAssignWorkDirtyExternal();
	virtual void AI_updateAreaStrategiesExternal(bool bTargets = true);
	virtual bool AI_shareWarExternal(TeamTypes eTeam);
	virtual void AI_updateWorstEnemyExternal();
	virtual int AI_getAtWarCounterExternal(TeamTypes eIndex);
	virtual void AI_setAtWarCounterExternal(TeamTypes eIndex, int iNewValue);
	virtual int AI_getAtPeaceCounterExternal(TeamTypes eIndex);
	virtual void AI_setAtPeaceCounterExternal(TeamTypes eIndex, int iNewValue);
	virtual int AI_getHasMetCounterExternal(TeamTypes eIndex);
	virtual void AI_setHasMetCounterExternal(TeamTypes eIndex, int iNewValue);
	virtual int AI_getOpenBordersCounterExternal(TeamTypes eIndex);
	virtual void AI_setOpenBordersCounterExternal(TeamTypes eIndex, int iNewValue);
	virtual int AI_getDefensivePactCounterExternal(TeamTypes eIndex);
	virtual void AI_setDefensivePactCounterExternal(TeamTypes eIndex, int iNewValue);
	virtual int AI_getShareWarCounterExternal(TeamTypes eIndex);
	virtual void AI_setShareWarCounterExternal(TeamTypes eIndex, int iNewValue);
	virtual int AI_getWarSuccessExternal(TeamTypes eIndex);
	virtual void AI_setWarSuccessExternal(TeamTypes eIndex, int iNewValue);
	virtual void AI_changeWarSuccessExternal(TeamTypes eIndex, int iChange);
	virtual int AI_getEnemyPeacetimeTradeValueExternal(TeamTypes eIndex);
	virtual void AI_setEnemyPeacetimeTradeValueExternal(TeamTypes eIndex, int iNewValue);
	virtual int AI_getEnemyPeacetimeGrantValueExternal(TeamTypes eIndex);
	virtual void AI_setEnemyPeacetimeGrantValueExternal(TeamTypes eIndex, int iNewValue);
	virtual WarPlanTypes AI_getWarPlanExternal(TeamTypes eIndex);
	virtual bool AI_isChosenWarExternal(TeamTypes eIndex);
	virtual bool AI_isSneakAttackPreparingExternal(TeamTypes eIndex);
	virtual bool AI_isSneakAttackReadyExternal(TeamTypes eIndex);
	virtual void AI_setWarPlanExternal(TeamTypes eIndex, WarPlanTypes eNewValue, bool bWar = true);
	virtual void readExternal(FDataStreamBase* pStream);
	virtual void writeExternal(FDataStreamBase* pStream);
};

#endif
