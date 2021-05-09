advc.009: This folder contains the documentation of BBAI 1.01 (minus the installation instructions). Much of this has changed in K-Mod and AdvCiv. I'm including it mostly in order to credit the minor contributors, i.e. EmperorFool, Afforess, Fuyu, LunarMongoose and others (search changelog.txt for the word "thanks"). Note that there is also an SVN revision history:
https://sourceforge.net/p/civ4betterai/code/



[B]Save Compatibility[/B]
_________________________

Better BTS AI will always maintain backwards save game compatibility, meaning that you will always be able to load a save from either plain BTS or an earlier version of the mod.  (Note:  If you installed as a mod, then you will be able to load saves with the same mod name.  If you replaced default files, then you'll be able to load games from plain BTS)  However, saves from some versions of the mod cannot be opened in plain BTS or in earlier versions of the mod.  Save-game compatibility (save version:  opens with):

0.90 and higher: Only 0.90 and higher
0.80 - 0.84:  0.80 and higher
0.01 - 0.78:  Any BBAI
Plain BTS:  BTS, any BBAI


[B]New User Interface Features[/B]
_________________________

- Holding down SHIFT+ALT and clicking on a leader in the scoreboard toggles your civ's warplan between WARPLAN_PREPARING_TOTAL and NO_WARPLAN.  This feature can be used to signal to your vassals that they should begin preparing for war with a particular player as well, allowing them to be much better prepared when you declare.  Any warning you can give your vassals will help, but enough time to build one to two units in a city is best.  WARNING: Use of this feature is not multiplayer compatible, WARPLANs are not sent across the network since they're otherwise only used by the AI and so it will lead to OOS errors if you have vassals.
- The scoreboard will show WAR in yellow instead of red when you have declared you are planning a war using the above feature.
- Added line to contact help text explaining that SHIFT+ALT clicking toggles war preparation plans.
- Air units can now be set to explore, they use the same explore logic as AI planes and then have additional logic if that doesn't push a mission.  Note that planes on auto explore always move at the very beginning of your turn!


[B]Customization[/B]
_________________________

There are now several XML global define files controlling different aspects of the mod.  See the particular file for more information.

[I]BBAI_Game_Options_GlobalDefines.xml[/I]: Controls options for new features, including options to change how defensive pacts work, disable victory strategy system, and allow the human player to become a vassal of an AI.

[I]TechDiffusion_GlobalDefines.xml[/I]: Enable and set up the new tech diffusion code which changes how BTS lowers research costs of techs known to many other players.

[I]LeadFromBehind_GlobalDefines.xml[/I]: Controls for the Lead From Behind mod component used in BBAI to help AI make better decisions with its units in stack v stack combat.

[I]BBAI_AI_Variables_GlobalDefines.xml[/I]: Some controls on AI in game decision making at various levels, including stack attacks of cities.

The new AI victory strategy system is also customizable.  There are five new settings for each leader in CIV4LeaderHeadInfos.xml controlling the odds they have of starting the game looking to win a particular type of victory.  The early stages of victory strategy system depend on these odds and have small effects on AI decisions all game long.  Later stages are independent of these odds and depend mainly on the AI detecting it is close to winning in a particular way.

Finally, BBAI includes a new system for easily scaling tech costs by era.  The idea is that tech rates can be easily scaled for all later era techs to adjust for the AI's better handling of its economy.  In CIV4EraInfos.xml, adjust the values for iTechCostModifier.


[B]Merge instructions[/B]
_________________________

DLL:  If the other mod has a custom DLL, you will need to merge the source code and compile a new DLL.  If you don't know what this means, ask for help from the community.  All of the changes in this mod are marked with one of the following tags:  BETTER_BTS_AI_MOD, AI_AUTO_PLAY_MOD, CHANGE_PLAYER, or UNOFFICIAL_PATCH.

Python:  Only the file in Assets\Python\Screens\ contains a fix, the other Python files are only to facilitate testing.  AIAutoPlay and ChangePlayer are very useful for general testing, so consider including them.  Tester contains some test popups specific to this mod.  These components use DrElmerGiggles custom event manager to manage their subscriptions to different Python events.

XML:  You will need to merge over all of the XML files for the mod to work properly, particularly the _GlobalDefines.xml files (which has updated AI variables), the new leader XML files, the new CIV4EraInfos.xml, and the text files Text\CIV4GameText_BetterBTSAI.xml and Text\CIV4GameText_AIAutoPlay.xml (for AI Autoplay python component).  The unit and building XML files included with the mod are from the unofficial patch and are not strictly necessary.


[B]Debug Keyboard Commands[/B]
_________________________

This mod includes the AIAutoPlay and ChangePlayer debug suites from the Revolution mod.  These commands are intended to help debug the game plus are also kind of fun (if you're not playing for real):

Ctrl+Shift+X    AIAutoPlay      Opens popup to start automation for any number of turns.  Pressing while in automation cancels automation.
Ctrl+Shift+M    AIAutoPlay      Automate the rest of the current turn only.
Ctrl+Shift+P    ChangePlayer    Open popup to change the civ and leader type for any player in the game.
Ctrl+Shift+L    ChangePlayer    Open popup to switch which player the human controls.


[B]Changelog[/B]
_________________________

The full change log from plain BTS is in changelog.txt, only the most recent changes are listed below.  There are hundreds of places where AI logic has been overhauled, tweaked, or better integrated with other pieces.

New in Better BTS AI 1.02

Bugfixes
- Fixed several uses of maxMoves where baseMoves was intended in CvUnitAI, code should be a bit faster now (thanks denev)

Victory strategy
- 

War strategy
- Tweaks to AI city razing decisions, including higher weight on wonders and culture
