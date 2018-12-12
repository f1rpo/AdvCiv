#pragma once

#ifndef CVDEFINES_H
#define CVDEFINES_H

// defines.h

#define _USRDLL /* advc.make: Normally set by the makefile. However, then the
                   Visual Studio editor can't find dozens of inlined functions,
                   and puts a red line under each call to them. */

// The following #defines should not be moddable...

#define MOVE_IGNORE_DANGER									(0x00000001)
#define MOVE_SAFE_TERRITORY									(0x00000002)
#define MOVE_NO_ENEMY_TERRITORY								(0x00000004)
#define MOVE_DECLARE_WAR									(0x00000008)
#define MOVE_DIRECT_ATTACK									(0x00000010)
#define MOVE_THROUGH_ENEMY									(0x00000020)
#define MOVE_MAX_MOVES										(0x00000040)
/************************************************************************************************/
/* BETTER_BTS_AI_MOD                      01/01/09                                jdog5000      */
/*                                                                                              */
/* General AI                                                                                   */
/************************************************************************************************/
// These two flags signal to weight the cost of moving through or adjacent to enemy territory higher
// Used to reduce exposure to attack for approaching enemy cities
#define MOVE_AVOID_ENEMY_WEIGHT_2							(0x00000080)
#define MOVE_AVOID_ENEMY_WEIGHT_3							(0x00000100)
/************************************************************************************************/
/* BETTER_BTS_AI_MOD                       END                                                  */
/************************************************************************************************/
#define MOVE_ATTACK_STACK									(0x00000200) // K-Mod.. allow the path to fight through enemy defences, but prefer not to.
#define MOVE_SINGLE_ATTACK									(0x00000400) // K-Mod. only attack with one unit, not the whole stack
#define MOVE_NO_ATTACK										(0x00000800) // K-Mod. used to prevent humans from accidentally attacking unseen units
#define MOVE_HAS_STEPPED									(0x00001000) // K-Mod. used to signal that at least one step has been taken for this move command
#define MOVE_ASSUME_VISIBLE									(0x00002000) // K-Mod. With this flag, the pathfinder will plan around enemy units even if they are not visible. (Note: AI units do this regardless of the flag.)
#define MOVE_ROUTE_TO										(0x00004000) // advc.049

/*
** K-Mod, 18/dec/10, karadoc
** global warming pollution flags for use in the CvPlayer::calculateGwPollution function
*/
#define POLLUTION_POPULATION	1
#define POLLUTION_BUILDINGS		2
#define POLLUTION_BONUSES		4
#define POLLUTION_POWER			8
#define POLLUTION_ALL			15
/*
** K-Mod end
*/


#define RANDPLOT_LAND													(0x00000001)
#define RANDPLOT_UNOWNED											(0x00000002)
#define RANDPLOT_ADJACENT_UNOWNED							(0x00000004)
#define RANDPLOT_ADJACENT_LAND								(0x00000008)
#define RANDPLOT_PASSABLE											(0x00000010)
#define RANDPLOT_NOT_VISIBLE_TO_CIV						(0x00000020)
#define RANDPLOT_NOT_CITY											(0x00000040)
// <advc.300>
#define RANDPLOT_HABITABLE (0x00000080)
#define RANDPLOT_WATERSOURCE (0x00000100) // </advc.300>

#ifdef _USRDLL
#define MAX_CIV_PLAYERS												(18) // note: default is 18, some people like 48. They are not compatible
#else
#define MAX_CIV_PLAYERS												(CvGlobals::getInstance().getMaxCivPlayers())
#endif

#define MAX_CIV_TEAMS													(MAX_CIV_PLAYERS)
#define MAX_PLAYERS														(MAX_CIV_PLAYERS + 1)
#define MAX_TEAMS															(MAX_PLAYERS)
#define BARBARIAN_PLAYER											((PlayerTypes)MAX_CIV_PLAYERS)
#define BARBARIAN_TEAM												((TeamTypes)MAX_CIV_TEAMS)

// Char Count limit for edit boxes
#define PREFERRED_EDIT_CHAR_COUNT							(15)
#define MAX_GAMENAME_CHAR_COUNT								(32)
#define MAX_PLAYERINFO_CHAR_COUNT							(32)
#define MAX_PLAYEREMAIL_CHAR_COUNT						(64)
#define MAX_PASSWORD_CHAR_COUNT								(32)
#define MAX_GSLOGIN_CHAR_COUNT								(17)
#define MAX_GSEMAIL_CHAR_COUNT								(50)
#define MAX_GSPASSWORD_CHAR_COUNT							(30)
#define MAX_CHAT_CHAR_COUNT										(256)
#define MAX_ADDRESS_CHAR_COUNT								(64)

#define INVALID_PLOT_COORD										(-(MAX_INT))	// don't use -1 since that is a valid wrap coordinate
#define DIRECTION_RADIUS											(1)
#define DIRECTION_DIAMETER										((DIRECTION_RADIUS * 2) + 1)
#define NUM_CITY_PLOTS												(21)
#define CITY_HOME_PLOT												(0)
#define CITY_PLOTS_RADIUS											(2)
#define CITY_PLOTS_DIAMETER										((CITY_PLOTS_RADIUS*2) + 1)

#define GAME_NAME															("Game")

#define LANDSCAPE_FOW_RESOLUTION							(4)
															
#define Z_ORDER_LAYER													(-0.1f)
#define Z_ORDER_LEVEL													(-0.3f)

#define CIV4_GUID															"civ4bts"
#define CIV4_PRODUCT_ID												11081
#define CIV4_NAMESPACE_ID											17
#define CIV4_NAMESPACE_EXT										"-tk"

#define MAP_TRANSFER_EXT											"_t"

#define USER_CHANNEL_PREFIX										"#civ4buser!"

#define SETCOLR																L"<color=%d,%d,%d,%d>"
#define ENDCOLR																L"</color>"
#define NEWLINE																L"\n"
#define SEPARATOR															L"\n-----------------------"
// BUG - start
#define DOUBLE_SEPARATOR													L"\n======================="
// BUG - end
#define TEXT_COLOR(szColor)										((int)(GC.getColorInfo((ColorTypes)GC.getInfoTypeForString(szColor)).getColor().r * 255)), ((int)(GC.getColorInfo((ColorTypes)GC.getInfoTypeForString(szColor)).getColor().g * 255)), ((int)(GC.getColorInfo((ColorTypes)GC.getInfoTypeForString(szColor)).getColor().b * 255)), ((int)(GC.getColorInfo((ColorTypes)GC.getInfoTypeForString(szColor)).getColor().a * 255))

// Version Verification files and folders
#ifdef _DEBUG
#define CIV4_EXE_FILE													".\\Civ4BeyondSword_DEBUG.exe"
#define CIV4_DLL_FILE													".\\Assets\\CvGameCoreDLL_DEBUG.dll"
#else
#define CIV4_EXE_FILE													".\\Civ4BeyondSword.exe"
#define CIV4_DLL_FILE													".\\Assets\\CvGameCoreDLL.dll"
#endif
#define CIV4_SHADERS													".\\Shaders\\FXO"
#define CIV4_ASSETS_PYTHON										".\\Assets\\Python"
#define CIV4_ASSETS_XML												".\\Assets\\XML"

#define MAX_PLAYER_NAME_LEN										(64)
#define MAX_VOTE_CHOICES											(8)
#define VOTE_TIMEOUT													(600000)	// 10 minute vote timeout - temporary

#define ANIMATION_DEFAULT											(1)			// Default idle animation

// python module names
#define PYDebugToolModule			"CvDebugInterface"
#define PYScreensModule				"CvScreensInterface"
#define PYCivModule						"CvAppInterface"
#define PYWorldBuilderModule	"CvWBInterface"
#define PYPopupModule					"CvPopupInterface"
#define PYDiplomacyModule			"CvDiplomacyInterface"
#define PYUnitControlModule		"CvUnitControlInterface"
#define PYTextMgrModule				"CvTextMgrInterface"
#define PYPerfTestModule			"CvPerfTest"
#define PYDebugScriptsModule	"DebugScripts"
#define PYPitBossModule				"PbMain"
#define PYTranslatorModule		"CvTranslator"
#define PYGameModule					"CvGameInterface"
#define PYEventModule					"CvEventInterface"
#define PYRandomEventModule					"CvRandomEventInterface"

#define DANGER_RANGE						(4)
#define BORDER_DANGER_RANGE					(2) // K-Mod

#define SERIALIZE_CVINFOS 0 // advc.003i

#endif	// CVDEFINES_H
