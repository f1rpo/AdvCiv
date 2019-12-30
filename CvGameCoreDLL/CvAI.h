#pragma once

#ifndef CIV4_AI_H
#define CIV4_AI_H

/*  advc.make: Wrapper header to improve the readability of classes that need
	all or most of the AI headers. */

#include "CvGamePlay.h"
// Note: Most of the headers in CvGamePlay.h are included by the ...AI.h headers as well
#include "CvGameAI.h"
#include "CvTeamAI.h"
#include "CvPlayerAI.h" // Includes the remaining AI headers (except the UWAI headers)
#include "AIStrategies.h" // Not needed that frequently, but might as well include it.

// <advc.003u>
#undef GET_TEAM // Overwrite definition in CvGamePlay.h
#define GET_TEAM(x) CvAI::getTeam(x)

namespace CvAI
{
	__forceinline CvTeamAI& getTeam(TeamTypes eTeam)
	{
		return CvTeamAI::AI_getTeam(eTeam);
	}
	__forceinline CvTeamAI& getTeam(PlayerTypes ePlayer)
	{
		return CvTeamAI::AI_getTeam(TEAMID(ePlayer));
	}
} // </advc.003u>

#endif
