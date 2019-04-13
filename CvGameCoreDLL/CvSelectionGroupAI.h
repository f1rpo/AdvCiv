#pragma once

// selectionGroupAI.h

#ifndef CIV4_SELECTION_GROUP_AI_H
#define CIV4_SELECTION_GROUP_AI_H

#include "CvSelectionGroup.h"

class CvSelectionGroupAI : public CvSelectionGroup
{

public:

	DllExport CvSelectionGroupAI();
	DllExport virtual ~CvSelectionGroupAI();

	void AI_init();
	void AI_uninit();
	void AI_reset();

	void AI_separate();
	void AI_separateNonAI(UnitAITypes eUnitAI);
	void AI_separateAI(UnitAITypes eUnitAI);
	// BETTER_BTS_AI_MOD, General AI, 06/02/09, jdog5000, START
	bool AI_separateImpassable();
	bool AI_separateEmptyTransports();
	// BETTER_BTS_AI_MOD: END

	bool AI_update();

	int AI_attackOdds(const CvPlot* pPlot, bool bPotentialEnemy) const;
	CvUnit* AI_getBestGroupAttacker(const CvPlot* pPlot, bool bPotentialEnemy, int& iUnitOdds, bool bForce = false, bool bNoBlitz = false) const {
		// <advc.048> Pure virtual in CvSelectionGroup, so preserve the signature to be on the safe side.
		return AI_getBestGroupAttacker(pPlot, bPotentialEnemy, iUnitOdds, bForce, bNoBlitz,
				false, false);
	}
	CvUnit* AI_getBestGroupAttacker(const CvPlot* pPlot, bool bPotentialEnemy, int& iUnitOdds,
			bool bForce, bool bNoBlitz, bool bSacrifice, bool bMaxSurvival = false) const;
	// </advc.048>
	CvUnit* AI_getBestGroupSacrifice(const CvPlot* pPlot, bool bPotentialEnemy, bool bForce = false, bool bNoBlitz = false) const;
	//int AI_compareStacks(const CvPlot* pPlot, bool bPotentialEnemy, bool bCheckCanAttack = false, bool bCheckCanMove = false) const;
	//int AI_sumStrength(const CvPlot* pAttackedPlot = NULL, DomainTypes eDomainType = NO_DOMAIN, bool bCheckCanAttack = false, bool bCheckCanMove = false) const;
	// K-Mod
	int AI_compareStacks(const CvPlot* pPlot, bool bCheckCanAttack = false) const;
	int AI_sumStrength(const CvPlot* pAttackedPlot = NULL, DomainTypes eDomainType = NO_DOMAIN, bool bCheckCanAttack = false) const;
	// K-Mod end
	void AI_queueGroupAttack(int iX, int iY);
	inline void AI_cancelGroupAttack() { m_bGroupAttack = false; } // K-Mod (made inline)
	inline bool AI_isGroupAttack() const { return m_bGroupAttack; } // K-Mod (made inline)

	bool AI_isControlled();
	bool AI_isDeclareWar(const CvPlot* pPlot = NULL);

	CvPlot* AI_getMissionAIPlot();

	bool AI_isForceSeparate();
	//void AI_makeForceSeparate();
	inline void AI_setForceSeparate(bool bNewValue = true) { m_bForceSeparate = bNewValue; } // K-Mod

	MissionAITypes AI_getMissionAIType() const;
	void AI_setMissionAI(MissionAITypes eNewMissionAI, CvPlot* pNewPlot, CvUnit* pNewUnit);
	CvUnit* AI_ejectBestDefender(CvPlot* pTargetPlot);

	CvUnit* AI_getMissionAIUnit();

	bool AI_isFull();

	void read(FDataStreamBase* pStream);
	void write(FDataStreamBase* pStream);

protected:
	// K-Mod note: the game will crash if too much data is added here. See CvSelectionGroup.h.

	int m_iMissionAIX;
	int m_iMissionAIY;

	bool m_bForceSeparate;

	MissionAITypes m_eMissionAIType;

	IDInfo m_missionAIUnit;

	bool m_bGroupAttack;
	int m_iGroupAttackX;
	int m_iGroupAttackY;
};

#endif
