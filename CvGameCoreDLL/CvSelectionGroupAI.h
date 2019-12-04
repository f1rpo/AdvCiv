#pragma once

// selectionGroupAI.h

#ifndef CIV4_SELECTION_GROUP_AI_H
#define CIV4_SELECTION_GROUP_AI_H

#include "CvSelectionGroup.h"


class CvSelectionGroupAI : public CvSelectionGroup
{
public:

	DllExport CvSelectionGroupAI();
	DllExport ~CvSelectionGroupAI();

	void AI_init();
	void AI_uninit();
	void AI_reset();

	void AI_separate();
	void AI_separateNonAI(UnitAITypes eUnitAI);
	void AI_separateAI(UnitAITypes eUnitAI);
	// BETTER_BTS_AI_MOD, General AI, 06/02/09, jdog5000, START
	bool AI_separateImpassable(); // K-Mod added bool return value.
	bool AI_separateEmptyTransports();
	// BETTER_BTS_AI_MOD: END

	bool AI_update();

	int AI_attackOdds(const CvPlot* pPlot, bool bPotentialEnemy) const;
	int AI_getWeightedOdds(CvPlot* pPlot, bool bPotentialEnemy = false); // K-Mod
	// advc.003u: These two had returned CvUnit*
	CvUnitAI* AI_getBestGroupAttacker(const CvPlot* pPlot, bool bPotentialEnemy,
			int& iUnitOdds, bool bForce = false, bool bNoBlitz = false,
			bool bSacrifice = false, bool bMaxSurvival = false) const; // advc.048
	CvUnitAI* AI_getBestGroupSacrifice(const CvPlot* pPlot, bool bPotentialEnemy, bool bForce = false, bool bNoBlitz = false) const;
	// K-Mod has removed bCheckCanMove param from these two and bPotentialEnemy from AI_compareStacks
	int AI_compareStacks(const CvPlot* pPlot, bool bCheckCanAttack = false) const;
	int AI_sumStrength(const CvPlot* pAttackedPlot = NULL, DomainTypes eDomainType = NO_DOMAIN, bool bCheckCanAttack = false) const;

	void AI_queueGroupAttack(int iX, int iY);
	inline void AI_cancelGroupAttack() { m_bGroupAttack = false; } // K-Mod (made inline)
	inline bool AI_isGroupAttack() const { return m_bGroupAttack; } // K-Mod (made inline)

	bool AI_isControlled() /* advc: */ const;
	bool AI_isDeclareWar(const CvPlot* pPlot = NULL) const;

	CvPlot* AI_getMissionAIPlot() const;

	bool AI_isForceSeparate() const;
	//void AI_makeForceSeparate();
	inline void AI_setForceSeparate(bool bNewValue = true) { m_bForceSeparate = bNewValue; } // K-Mod

	inline MissionAITypes AI_getMissionAIType() /* K-Mod: */ const
	{
		return m_eMissionAIType; // advc.inl: inline (now that it's no longer virtual)
	}
	void AI_setMissionAI(MissionAITypes eNewMissionAI, CvPlot const* pNewPlot, CvUnit const* pNewUnit);
	// advc.003u: These two had returned CvUnit*
	CvUnitAI* AI_ejectBestDefender(CvPlot* pTargetPlot);
	CvUnitAI* AI_getMissionAIUnit() const;
	CvUnitAI* AI_getHeadUnit() const; // advc.003u: Counterpart to CvSelectionGroup::getHeadUnit

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
