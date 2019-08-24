#pragma once

#ifndef CIV4_GAME_PLAY_H
#define CIV4_GAME_PLAY_H

/*  advc.make: I've created this wrapper header file to improve the readability of
	the ca. 40 .cpp files that require CvGameAI.h, CvPlayerAI.h and CvTeamAI.h.
	Note that CvGame.h, CvPlayer.h, CvTeam.h and several other frequently used
	headers are recursively included (for better or worse).
	Most of the client code has nothing to do with the AI; it's just that the
	very commonly used GET_PLAYER and GET_TEAM macros and the getGame inline
	function are defined in the ...AI header files and can't be easily moved
	from there b/c they're tied to exported functions that the EXE calls. */

#include "CvGameAI.h"
#include "CvPlayerAI.h"
#include "CvCivilization.h" // advc.003w
#include "CvTeamAI.h"

#endif
