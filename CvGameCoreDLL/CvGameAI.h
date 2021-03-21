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
  void AI_reset(bool bConstructor = false);

  void AI_makeAssignWorkDirty();
  void AI_updateAssignWork();

  int AI_combatValue(UnitTypes eUnit) const;

  int AI_turnsPercent(int iTurns, int iPercent);
  // <advc.erai>
  scaled AI_getCurrEraFactor() const;
  inline int AI_getCurrEra() const { return AI_getCurrEraFactor().round(); }
  EraTypes AI_getVoteSourceEra(VoteSourceTypes eVS = NO_VOTESOURCE) const;
  // </advc.erai>
  scaled AI_exclusiveRadiusWeight(int iDist = -1) const; // advc.099b

  void read(FDataStreamBase* pStream);
  void write(FDataStreamBase* pStream);

  inline UWAI& uwai() { return m_uwai; } // advc.104

protected:

  int m_iPad;
  // <advc.104>
  UWAI m_uwai;
  void AI_sortOutUWAIOptions(bool bFromSaveGame);
  // </advc.104>
  EnumMap<VoteSourceTypes,EraTypes> m_aeVoteSourceEras; // advc.erai
  std::vector<scaled> m_arExclusiveRadiusWeight; // advc.099b

  void AI_updateExclusiveRadiusWeight(); // advc.009b
  void AI_updateVoteSourceEras(); // advc.erai
};

#endif
