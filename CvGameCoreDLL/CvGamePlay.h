#pragma once

#ifndef CIV4_GAME_PLAY_H
#define CIV4_GAME_PLAY_H

/*  advc.make: I've created this wrapper header file to improve the readability of
	the ca. 40 .cpp files that require CvGameAI.h, CvPlayerAI.h and CvTeamAI.h.
	Note that CvPlayer.h, CvTeam.h are recursively included.
	Most of the client code has nothing to do with the AI; it's just that the
	very commonly used GET_PLAYER and GET_TEAM macros are defined in the
	...AI header files. (I may soon move them elsewhere though.) */

#include "CvGame.h"
#include "CvPlayerAI.h"
#include "CvCivilization.h" // advc.003w
#include "CvTeamAI.h"

#endif
