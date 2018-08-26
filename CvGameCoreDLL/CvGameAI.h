#pragma once

// gameAI.h

#ifndef CIV4_GAME_AI_H
#define CIV4_GAME_AI_H

#include "CvGame.h"
#include "WarAndPeaceAI.h" // advc.104

class CvGameAI : public CvGame
{

public:

  CvGameAI();
  virtual ~CvGameAI();

  void AI_init();
  void AI_initScenario(); // advc.104u
  void AI_uninit();
  void AI_reset();

  void AI_makeAssignWorkDirty();
  void AI_updateAssignWork();

  int AI_combatValue(UnitTypes eUnit) const;

  int AI_turnsPercent(int iTurns, int iPercent);

  virtual void read(FDataStreamBase* pStream);
  virtual void write(FDataStreamBase* pStream);

  WarAndPeaceAI& warAndPeaceAI(); // advc.104

protected:

  int m_iPad;

// <advc.104>
private:
	/* I'm repurposing the Aggressive AI option
	   so that it enables the war-and-peace AI from K-Mod in addition to
	   the option's normal effect. A bit of a hack, but less invasive
	   than changing all the isOption(AGGRESSIVE_AI) checks. And I don't want
	   two separate options because my war-and-peace AI implies Aggressive AI. */
	void sortOutWPAIOptions(bool fromSaveGame);
	WarAndPeaceAI wpai;
// </advri.104>


};

#endif
