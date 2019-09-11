#pragma once

#ifndef CIV4_GAME_PLAY_H
#define CIV4_GAME_PLAY_H

/*  advc.make: Wrapper header to reduce the number of include statements and
	a place for the team accessor macros. (However, the one that returns a CvTeamAI
	reference is defined in CvAI.h.) For inline definitions, both CvPlayer.h
	and CvTeam.h are needed. */

#include "CvGame.h"
#include "CvTeam.h"
#include "CvPlayer.h"
#include "CvCivilization.h" // advc.003w
/*  Not: CvCity.h - not much use without CvCityList, which, in turn, includes
	CvCityAI.h. Same problem with CvUnit.h, CvSelectionGroup.h.
	To amend this, the constructor and destructor calls would have to be moved out of
	FFreeListTrashArray. */

// <advc.003u>
#ifndef GET_TEAM // Prefer the definition in CvAI.h
#define GET_TEAM(x) CvGamePlay::getTeam(x)
#endif
#define TEAMID(x) GET_PLAYER(x).getTeam()

namespace CvGamePlay
{
	__forceinline CvTeam& getTeam(TeamTypes eTeam)
	{
		return CvTeam::getTeam(eTeam);
	}
	__forceinline CvTeam& getTeam(PlayerTypes ePlayer)
	{
		return CvTeam::getTeam(TEAMID(ePlayer));
	}
} // </advc.003u>

#endif
