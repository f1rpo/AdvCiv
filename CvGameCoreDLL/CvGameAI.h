#pragma once

#ifndef CIV4_GAME_AI_H
#define CIV4_GAME_AI_H

#include "CvGame.h"
#include "UWAI.h" // advc.104
#include "ScaledInt.h" // advc.tmp (Move to precompiled header eventually)


class CvGameAI : public CvGame
{
public:

  CvGameAI();
  ~CvGameAI();

  void AI_init();
  void AI_initScenario(); // advc.104u
  void AI_uninit();
  void AI_reset(bool bConstructor = false);

  void AI_makeAssignWorkDirty();
  void AI_updateAssignWork();

  int AI_combatValue(UnitTypes eUnit) const;

  int AI_turnsPercent(int iTurns, int iPercent);
  scaled AI_exclusiveRadiusWeight(int iDist = -1) const; // advc.099b

  void read(FDataStreamBase* pStream);
  void write(FDataStreamBase* pStream);

  inline UWAI& uwai() { return m_uwai; } // advc.104

protected:

  int m_iPad;
  // <advc.104>
  void AI_sortOutUWAIOptions(bool bFromSaveGame);
  UWAI m_uwai; // </advc.104>
  std::vector<scaled> m_arExclusiveRadiusWeight; // advc.099b

  void AI_updateExclusiveRadiusWeight(); // advc.009b
};

#endif
