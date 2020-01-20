#pragma once

#ifndef CIV4_AI_H
#define CIV4_AI_H

// advc.make: Wrapper header to reduce the number of include directives

#include "CvGamePlay.h" // The AI headers aren't much good w/o the core gameplay headers
#include "CvGameAI.h"
#include "CvTeamAI.h"
#include "CvPlayerAI.h"

// <advc.003u>
#undef GET_TEAM // Overwrite definition in CvGamePlay.h
#define GET_TEAM(x) CoreAI::getTeam(x)

namespace CoreAI
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
