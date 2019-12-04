#pragma once

#ifndef CIV4_GAME_AI_H
#define CIV4_GAME_AI_H

#include "CvGame.h"
#include "UWAI.h" // advc.104


class CvGameAI : public CvGame
{
public:

  CvGameAI();
  ~CvGameAI();

  void AI_init();
  void AI_initScenario(); // advc.104u
  void AI_uninit();
  void AI_reset();

  void AI_makeAssignWorkDirty();
  void AI_updateAssignWork();

  int AI_combatValue(UnitTypes eUnit) const;

  int AI_turnsPercent(int iTurns, int iPercent);

  void read(FDataStreamBase* pStream);
  void write(FDataStreamBase* pStream);

  inline UWAI& uwai() { return m_uwai; } // advc.104

protected:

  int m_iPad;
  // <advc.104>
  void AI_sortOutUWAIOptions(bool bFromSaveGame);
  UWAI m_uwai; // </advc.104>
};

#endif
