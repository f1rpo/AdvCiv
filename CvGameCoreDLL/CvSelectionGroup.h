#pragma once

// selectionGroup.h

#ifndef CIV4_GROUP_H
#define CIV4_GROUP_H

//#include "CvStructs.h"
#include "LinkedList.h"
#include "KmodPathFinder.h"

class CvPlot;
class CvArea;
class FAStarNode;

class CvSelectionGroup
{

public:

	CvSelectionGroup();
	virtual ~CvSelectionGroup();

	DllExport void init(int iID, PlayerTypes eOwner);
	DllExport void uninit();			
	DllExport void reset(int iID = 0, PlayerTypes eOwner = NO_PLAYER, bool bConstructorCall = false);

	void kill();

	void doTurn();
	void doTurnPost(); // advc.004l
	bool showMoves( // <advc.102>
			CvPlot const& fromPlot) const;
	void setInitiallyVisible(bool b); // </advc.102>

	void updateTimers();
	bool doDelayedDeath();

	void playActionSound();

	void pushMission(MissionTypes eMission, int iData1 = -1, int iData2 = -1, int iFlags = 0, bool bAppend = false, bool bManual = false, MissionAITypes eMissionAI = NO_MISSIONAI, CvPlot* pMissionAIPlot = NULL, CvUnit* pMissionAIUnit = NULL,		// Exposed to Python
			bool bModified = false); // advc.011b
	void popMission();																																										// Exposed to Python
	//DllExport void autoMission();
	bool autoMission(); // K-Mod. (No 'DllExport'? Are you serious!?)
	void updateMission();
	DllExport CvPlot* lastMissionPlot();																																					// Exposed to Python

	bool canStartMission(int iMission, int iData1, int iData2, CvPlot* pPlot = NULL, bool bTestVisible = false, bool bUseCache = false);		// Exposed to Python
	void startMission();
	//void continueMission(int iSteps = 0);
	// K-Mod. Split continueMission into two functions to remove the recursion.
	void continueMission();
protected:
	bool continueMission_bulk(int iSteps);
public:
	// K-Mod end

	DllExport bool canDoInterfaceMode(InterfaceModeTypes eInterfaceMode);													// Exposed to Python
	DllExport bool canDoInterfaceModeAt(InterfaceModeTypes eInterfaceMode, CvPlot* pPlot);				// Exposed to Python

	bool canDoCommand(CommandTypes eCommand, int iData1, int iData2, bool bTestVisible = false, bool bUseCache = false);		// Exposed to Python
	bool canEverDoCommand(CommandTypes eCommand, int iData1, int iData2, bool bTestVisible, bool bUseCache);
	void setupActionCache();

	bool isHuman() const; // advc.002i: const																																									// Exposed to Python
	DllExport bool isBusy();
	bool isCargoBusy();
	int baseMoves() const;																																										// Exposed to Python 
	int maxMoves() const; // K-Mod
	int movesLeft() const; // K-Mod
	bool isWaiting() const;																																							// Exposed to Python
	bool isFull();																																											// Exposed to Python
	bool hasCargo();																																										// Exposed to Python
	int getCargo() const;
	int cargoSpaceAvailable(SpecialUnitTypes eSpecialCargo = NO_SPECIALUNIT, DomainTypes eDomainCargo = NO_DOMAIN) const; // K-Mod
	DllExport bool canAllMove();																																				// Exposed to Python
	bool canAnyMove() const; // Exposed to Python
	bool canCargoAllMove() const; // K-Mod (moved from CvUnit)
	bool hasMoved() const; // Exposed to Python
	bool canEnterTerritory(TeamTypes eTeam, bool bIgnoreRightOfPassage = false) const;									// Exposed to Python
	bool canEnterArea(TeamTypes eTeam, const CvArea* pArea, bool bIgnoreRightOfPassage = false) const;									// Exposed to Python
	DllExport bool canMoveInto(CvPlot* pPlot, bool bAttack = false);																		// Exposed to Python
	DllExport bool canMoveOrAttackInto(CvPlot* pPlot, bool bDeclareWar = false) {					 // Exposed to Python
		return canMoveOrAttackInto(pPlot, bDeclareWar, false);
	} // K-Mod. (hack to avoid breaking the DllExport)			advc.003: const
	bool canMoveOrAttackInto(CvPlot* pPlot, bool bDeclareWar, bool bCheckMoves/* = false (see above) */, bool bAssumeVisible = true) const;
	bool canMoveThrough(CvPlot* pPlot, bool bDeclareWar = false, bool bAssumeVisible = true) const; // Exposed to Python, K-Mod added bDeclareWar and bAssumeVisible
	bool canFight();																																										// Exposed to Python 
	bool canDefend();																																										// Exposed to Python
	bool canBombard(const CvPlot* pPlot);
	bool visibilityRange();
/************************************************************************************************/
/* BETTER_BTS_AI_MOD                      08/19/09                                jdog5000      */
/*                                                                                              */
/* General AI                                                                                   */
/************************************************************************************************/
	int getBombardTurns( CvCity* pCity );
	bool isHasPathToAreaPlayerCity( PlayerTypes ePlayer, int iFlags = 0, int iMaxPathTurns = -1 );
	bool isHasPathToAreaEnemyCity( bool bIgnoreMinors = true, int iFlags = 0, int iMaxPathTurns = -1 );
	bool isStranded() const; // Note: K-Mod no longer uses the stranded cache. I have a new system.
	//void invalidateIsStrandedCache(); // deleted by K-Mod
	//bool calculateIsStranded();
	bool canMoveAllTerrain() const;
/************************************************************************************************/
/* BETTER_BTS_AI_MOD                       END                                                  */
/************************************************************************************************/

	void unloadAll();
	bool alwaysInvisible() const;																																							// Exposed to Python
	bool isInvisible(TeamTypes eTeam) const;																								// Exposed to Python
	int countNumUnitAIType(UnitAITypes eUnitAI);																												// Exposed to Python
	bool hasWorker();																																										// Exposed to Python
	bool IsSelected();
	DllExport void NotifyEntity(MissionTypes eMission);
	void airCircle(bool bStart);
	void setBlockading(bool bStart);

	int getX() const;
	int getY() const;
	bool at(int iX, int iY) const;																																								// Exposed to Python
	bool atPlot(const CvPlot* pPlot) const;																																				// Exposed to Python
	DllExport CvPlot* plot() const;																																								// Exposed to Python
	int getArea() const;
	CvArea* area() const;																																													// Exposed to Python
	DomainTypes getDomainType() const;

	RouteTypes getBestBuildRoute(CvPlot* pPlot, BuildTypes* peBestBuild = NULL) const;	// Exposed to Python

	bool groupAttack(int iX, int iY, int iFlags, bool& bFailedAlreadyFighting);
	void groupMove(CvPlot* pPlot, bool bCombat, CvUnit* pCombatUnit = NULL, bool bEndMove = false);
	bool groupPathTo(int iX, int iY, int iFlags);
	bool groupRoadTo(int iX, int iY, int iFlags);
	bool groupBuild(BuildTypes eBuild,
			bool bFinish = true); // advc.011b

	void setTransportUnit(CvUnit* pTransportUnit, CvSelectionGroup** pOtherGroup = NULL); // bbai added pOtherGroup

	bool isAmphibPlot(const CvPlot* pPlot) const;																																		// Exposed to Python
	bool groupAmphibMove(CvPlot* pPlot, int iFlags);

	DllExport bool readyToSelect(bool bAny = false);																										// Exposed to Python
	bool readyToMove(bool bAny = false); // Exposed to Python
	bool readyToAuto(); // Exposed to Python
	// K-Mod. (note: I'd make these function const, but it would conflict with some dllexport functions)
	bool readyForMission();
	bool canDoMission(int iMission, int iData1, int iData2, CvPlot* pPlot,
			bool bTestVisible, bool bCheckMoves) const; // advc.002i: const
	// K-Mod end

	int getID() const;																																												// Exposed to Python
	void setID(int iID);																			

	int getMissionTimer() const;
	void setMissionTimer(int iNewValue);
	void changeMissionTimer(int iChange);
	void updateMissionTimer(int iSteps = 0,
		CvPlot* fromPlot = NULL); // advc.102

	inline bool isForceUpdate() { return m_bForceUpdate; } // K-Mod made inline
	inline void setForceUpdate(bool bNewValue) { m_bForceUpdate = bNewValue; } // K-Mod made inline
	// void doForceUpdate(); // K-Mod. (disabled. force update doesn't work the same way anymore.)

	PlayerTypes getOwner() const;																															// Exposed to Python
#ifdef _USRDLL
	inline PlayerTypes getOwnerINLINE() const
	{
		return m_eOwner;
	}
#endif
	TeamTypes getTeam() const;																																					// Exposed to Python

	ActivityTypes getActivityType() const;																															// Exposed to Python
	void setActivityType(ActivityTypes eNewValue);																											// Exposed to Python

	AutomateTypes getAutomateType() const;																																		// Exposed to Python
	bool isAutomated() const; // Exposed to Python
	void setAutomateType(AutomateTypes eNewValue);																											// Exposed to Python

	// FAStarNode* getPathLastNode() const; // disabled by K-Mod. Use path_finder methods instead.
	CvPlot* getPathFirstPlot() const;																																		// Exposed to Python
	CvPlot* getPathEndTurnPlot() const;																																	// Exposed to Python
	bool generatePath(const CvPlot* pFromPlot, const CvPlot* pToPlot, int iFlags = 0, bool bReuse = false, int* piPathTurns = NULL, int iMaxPath = -1) const; // Exposed to Python (K-mod added iMaxPath)
	// void resetPath() const; // disabled by K-mod. Use path_finder.Reset instead. (was exposed to Python)

	DllExport void clearUnits();
	DllExport bool addUnit(CvUnit* pUnit, bool bMinimalChange);
	void removeUnit(CvUnit* pUnit);
	void mergeIntoGroup(CvSelectionGroup* pSelectionGroup);
	CvSelectionGroup* splitGroup(int iSplitSize, CvUnit* pNewHeadUnit = NULL, CvSelectionGroup** ppOtherGroup = NULL);
	void regroupSeparatedUnits(); // K-Mod

	DllExport CLLNode<IDInfo>* deleteUnitNode(CLLNode<IDInfo>* pNode);
	DllExport CLLNode<IDInfo>* nextUnitNode(CLLNode<IDInfo>* pNode) const;
	DllExport int getNumUnits() const;																												// Exposed to Python
	DllExport int getUnitIndex(CvUnit* pUnit, int maxIndex = -1) const;
	DllExport CLLNode<IDInfo>* headUnitNode() const;
	DllExport CvUnit* getHeadUnit() const;
	CvUnit* getUnitAt(int index) const;
	UnitAITypes getHeadUnitAI() const;
	PlayerTypes getHeadOwner() const;
	TeamTypes getHeadTeam() const;

	void clearMissionQueue();																																	// Exposed to Python
	int getLengthMissionQueue() const;																											// Exposed to Python
	MissionData* getMissionFromQueue(int iIndex) const;																							// Exposed to Python
	void insertAtEndMissionQueue(MissionData mission, bool bStart = true);
	CLLNode<MissionData>* deleteMissionQueueNode(CLLNode<MissionData>* pNode);
	DllExport CLLNode<MissionData>* nextMissionQueueNode(CLLNode<MissionData>* pNode) const;
	CLLNode<MissionData>* prevMissionQueueNode(CLLNode<MissionData>* pNode) const;
	DllExport CLLNode<MissionData>* headMissionQueueNode() const;
	CLLNode<MissionData>* tailMissionQueueNode() const;
	int getMissionType(int iNode) const;																														// Exposed to Python
	int getMissionData1(int iNode) const;																														// Exposed to Python
	int getMissionData2(int iNode) const;																														// Exposed to Python

	// for serialization
	virtual void read(FDataStreamBase* pStream);
	virtual void write(FDataStreamBase* pStream);

	virtual void AI_init() = 0;
	virtual void AI_reset() = 0;
	virtual void AI_separate() = 0;
	virtual bool AI_update() = 0;
	virtual int AI_attackOdds(const CvPlot* pPlot, bool bPotentialEnemy) const = 0;
	virtual CvUnit* AI_getBestGroupAttacker(const CvPlot* pPlot, bool bPotentialEnemy, int& iUnitOdds, bool bForce = false, bool bNoBlitz = false) const = 0;
	virtual CvUnit* AI_getBestGroupSacrifice(const CvPlot* pPlot, bool bPotentialEnemy, bool bForce = false, bool bNoBlitz = false) const = 0;
	//virtual int AI_compareStacks(const CvPlot* pPlot, bool bPotentialEnemy, bool bCheckCanAttack = false, bool bCheckCanMove = false) const = 0;
	//virtual int AI_sumStrength(const CvPlot* pAttackedPlot = NULL, DomainTypes eDomainType = NO_DOMAIN, bool bCheckCanAttack = false, bool bCheckCanMove = false) const = 0;
	// K-Mod. (hopefully the core game doesn't rely on these function signatures.)
	virtual int AI_compareStacks(const CvPlot* pPlot, bool bCheckCanAttack = false) const = 0;
	virtual int AI_sumStrength(const CvPlot* pAttackedPlot = NULL, DomainTypes eDomainType = NO_DOMAIN, bool bCheckCanAttack = false) const = 0;
	// K-Mod end
	virtual void AI_queueGroupAttack(int iX, int iY) = 0;
	virtual void AI_cancelGroupAttack() = 0;
	virtual bool AI_isGroupAttack() const = 0; // K-Mod made const

	virtual bool AI_isControlled() = 0;
	virtual bool AI_isDeclareWar(const CvPlot* pPlot = NULL) = 0;
	virtual CvPlot* AI_getMissionAIPlot() = 0;
	virtual bool AI_isForceSeparate() = 0;
	//virtual void AI_makeForceSeparate() = 0;
	virtual void AI_setForceSeparate(bool bNewValue = true) = 0; // K-Mod
	//virtual MissionAITypes AI_getMissionAIType() = 0;
	virtual MissionAITypes AI_getMissionAIType() const = 0; // K-Mod
	virtual void AI_setMissionAI(MissionAITypes eNewMissionAI, CvPlot* pNewPlot, CvUnit* pNewUnit) = 0;
	virtual CvUnit* AI_getMissionAIUnit() = 0;
	virtual CvUnit* AI_ejectBestDefender(CvPlot* pTargetPlot) = 0;
	virtual void AI_separateNonAI(UnitAITypes eUnitAI) = 0;
	virtual void AI_separateAI(UnitAITypes eUnitAI) = 0;
/************************************************************************************************/
/* BETTER_BTS_AI_MOD                      06/02/09                                jdog5000      */
/*                                                                                              */
/* General AI                                                                                   */
/************************************************************************************************/
	virtual bool AI_separateImpassable() = 0; // K-Mod added bool return value.
	virtual bool AI_separateEmptyTransports() = 0; // same
/************************************************************************************************/
/* BETTER_BTS_AI_MOD                       END                                                  */
/************************************************************************************************/

	virtual bool AI_isFull() = 0;

protected:
	// WARNING: adding to this class will cause the civ4 exe to crash

	// K-Mod: I've done some basic tests of the above warning.
	// I've found that it does indeed crash during startup if I add int[30]
	// but it does not crash if I only add int[2]. (I haven't tested inbetween.)
	// The game also crashes if I add int[30] to CvSelectionGroupAI.

	// ... I see that BBAI ignored the warning. They added some stuff below.
	// Removing the BBAI bools from below does not change the size 80. Neither does removing the BBAI virtual functions.
	// but adding another int increases the size to 84. Which is a shame, because I really want to add one more int...
	// Although a single int doesn't cause a startup crash, I'd rather not risk instability.

	// <advc.003k> Pointer to additional data members
	class Data;
	CvSelectionGroup::Data* m; // dial m for members
	// </advc.003k>

	int m_iID;
	int m_iMissionTimer;

	bool m_bForceUpdate;

	PlayerTypes m_eOwner;
	ActivityTypes m_eActivityType;

	CLinkList<IDInfo> m_units;
	CLinkList<MissionData> m_missionQueue;
	std::vector<CvUnit*> m_aDifferentUnitCache;
	bool m_bIsBusyCache;

	void activateHeadMission();
	void deactivateHeadMission();
	// <advc.075>
	void handleBoarded();
	bool canDisembark() const;
	void resetBoarded();
	void getLandCargoGroups(std::vector<CvSelectionGroup*>& r);
	// </advc.075>
	bool sentryAlert(
			bool bUpdateKnownEnemies = false); // advc.004l

	// <advc.003k>
	class Data {
		CLinkList<IDInfo> knownEnemies; // advc.004l
		bool bInitiallyVisible; // advc.102
		// Moved here in order to bring sizeof down to 80
		AutomateTypes eAutomateType;
		friend CvSelectionGroup;
	}; // </advc.003k>

public:
	static KmodPathFinder path_finder; // K-Mod! I'd rather this not be static, but I can't do that here.
};
/*  advc.003k: A trick from
https://stackoverflow.com/questions/19401887/how-to-check-the-size-of-a-structure-at-compile-time/19402038
	to verify that the class has a safe size. If this won't compile, then you've
	probably added a data member (directly) to CvSelectionGroup. */
typedef char assertSizeOfSelectionGroup[(sizeof(CvSelectionGroup)==80)*2-1];

#endif
