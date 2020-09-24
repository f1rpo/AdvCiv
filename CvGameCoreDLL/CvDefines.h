#pragma once

#ifndef CVDEFINES_H
#define CVDEFINES_H

/*	advc.enum: MAX_CIV_PLAYERS, MAX_PLAYERS etc.
	and city plot constants moved to CvEnums.h */
/*	advc: K-Mod pollution flags moved to CvPlayer.h.
	RANDPLOT flags moved to the end of CvPlot.h.
	(Border) danger range moved to CvPlayerAI.h. */

// The following #defines should not be moddable...
/*	advc: Which is to say, I think, that the defines have been compiled into
	the EXE and changing them will either have no effect or result in
	inconsistencies between EXE and DLL. Of course this doesn't apply
	to defines added by mods. Imo mods should only add defines here
	when it's important to group them with the original defines for context. */

// advc.enum (tbd.): Use OVERRIDE_BITMASK_OPERATORS to turn this into an enum
#define MOVE_IGNORE_DANGER									(0x00000001)
#define MOVE_SAFE_TERRITORY									(0x00000002)
#define MOVE_NO_ENEMY_TERRITORY								(0x00000004)
#define MOVE_DECLARE_WAR									(0x00000008)
#define MOVE_DIRECT_ATTACK									(0x00000010)
#define MOVE_THROUGH_ENEMY									(0x00000020)
#define MOVE_MAX_MOVES										(0x00000040)
// BETTER_BTS_AI_MOD, General AI, 01/01/09, jdog5000: START
// These two flags signal to weight the cost of moving through or adjacent to enemy territory higher
// Used to reduce exposure to attack for approaching enemy cities
#define MOVE_AVOID_ENEMY_WEIGHT_2							(0x00000080)
#define MOVE_AVOID_ENEMY_WEIGHT_3							(0x00000100)
// BETTER_BTS_AI_MOD: END
// <K-Mod>
// allow the path to fight through enemy defences, but prefer not to.
#define MOVE_ATTACK_STACK									(0x00000200)
// only attack with one unit, not the whole stack
#define MOVE_SINGLE_ATTACK									(0x00000400)
// to prevent humans from accidentally attacking unseen units
#define MOVE_NO_ATTACK										(0x00000800)
// to signal that at least one step has been taken for this move command
#define MOVE_HAS_STEPPED									(0x00001000)
/*	With this flag, the pathfinder will plan around enemy units
	even if they are not visible. (Note: AI units do this regardless of the flag.) */
#define MOVE_ASSUME_VISIBLE									(0x00002000)
// </K-Mod>
#define MOVE_ROUTE_TO										(0x00004000) // advc.pf

/*	Char Count limit for edit boxes ...
	advc (note): The DLL uses only some of these and only in CvDLLButtonPopup,
	but I expect that the EXE uses some or all, so I'm keeping the definitions here. */
#define PREFERRED_EDIT_CHAR_COUNT							(15)
#define MAX_GAMENAME_CHAR_COUNT								(32)
#define MAX_PLAYERINFO_CHAR_COUNT							(32)
#define MAX_PLAYEREMAIL_CHAR_COUNT							(64)
#define MAX_PASSWORD_CHAR_COUNT								(32)
#define MAX_GSLOGIN_CHAR_COUNT								(17)
#define MAX_GSEMAIL_CHAR_COUNT								(50)
#define MAX_GSPASSWORD_CHAR_COUNT							(30)
#define MAX_CHAT_CHAR_COUNT									(256)
#define MAX_ADDRESS_CHAR_COUNT								(64)

#define INVALID_PLOT_COORD									(-MAX_INT) // NB: -1 is a valid wrap coordinate
#define DIRECTION_RADIUS									(1)
#define DIRECTION_DIAMETER									(DIRECTION_RADIUS * 2 + 1)

#ifndef _USRDLL // advc: Unused (at least in the DLL); should probably keep it that way.
	#define GAME_NAME ("Game")
	#define Z_ORDER_LAYER									(-0.1f)
	#define Z_ORDER_LEVEL									(-0.3f)

	#define CIV4_GUID										"civ4bts"
	#define CIV4_PRODUCT_ID									11081
	#define CIV4_NAMESPACE_ID								17
	#define CIV4_NAMESPACE_EXT								"-tk"

	#define USER_CHANNEL_PREFIX								"#civ4buser!"
	// Version Verification files and folders
	#ifdef _DEBUG
		#define CIV4_EXE_FILE								".\\Civ4BeyondSword_DEBUG.exe"
		#define CIV4_DLL_FILE								".\\Assets\\CvGameCoreDLL_DEBUG.dll"
	#else
		#define CIV4_EXE_FILE								".\\Civ4BeyondSword.exe"
		#define CIV4_DLL_FILE								".\\Assets\\CvGameCoreDLL.dll"
	#endif
	#define CIV4_SHADERS									".\\Shaders\\FXO"
	#define CIV4_ASSETS_PYTHON								".\\Assets\\Python"
	#define CIV4_ASSETS_XML									".\\Assets\\XML"
#endif
// advc (note): These four are also unused in the DLL, but might be useful(?).
#define MAX_PLAYER_NAME_LEN									(64)
#define MAX_VOTE_CHOICES									(8)
#define VOTE_TIMEOUT										(600000) // 10 minute vote timeout - temporary
#define ANIMATION_DEFAULT									(1) // Default idle animation

#define MAP_TRANSFER_EXT									"_t"
#define LANDSCAPE_FOW_RESOLUTION							(4)

#define SETCOLR												L"<color=%d,%d,%d,%d>"
#define ENDCOLR												L"</color>"
#define NEWLINE												L"\n"
#define SEPARATOR											L"\n-----------------------"
// BUG - start
#define DOUBLE_SEPARATOR									L"\n======================="
// BUG - end
#define TEXT_COLOR(szColor) \
		(int)GC.getInfo((ColorTypes)GC.getInfoTypeForString(szColor)).getColor().r * 255, \
		(int)GC.getInfo((ColorTypes)GC.getInfoTypeForString(szColor)).getColor().g * 255, \
		(int)GC.getInfo((ColorTypes)GC.getInfoTypeForString(szColor)).getColor().b * 255, \
		(int)GC.getInfo((ColorTypes)GC.getInfoTypeForString(szColor)).getColor().a * 255
// advc:  (uses of this macro aren't tagged with "advc")
#define PLAYER_TEXT_COLOR(kPlayer) \
		kPlayer.getPlayerTextColorR(), kPlayer.getPlayerTextColorG(), \
		kPlayer.getPlayerTextColorB(), kPlayer.getPlayerTextColorA()

// python module names
#define PYDebugToolModule			"CvDebugInterface"
#define PYScreensModule				"CvScreensInterface"
#define PYCivModule					"CvAppInterface"
#define PYWorldBuilderModule		"CvWBInterface"
#define PYPopupModule				"CvPopupInterface"
#define PYDiplomacyModule			"CvDiplomacyInterface"
#define PYUnitControlModule			"CvUnitControlInterface"
#define PYTextMgrModule				"CvTextMgrInterface"
#define PYPerfTestModule			"CvPerfTest"
#define PYDebugScriptsModule		"DebugScripts"
#define PYPitBossModule				"PbMain"
#define PYTranslatorModule			"CvTranslator"
#define PYGameModule				"CvGameInterface"
#define PYEventModule				"CvEventInterface"
#define PYRandomEventModule			"CvRandomEventInterface"

#endif	// CVDEFINES_H
