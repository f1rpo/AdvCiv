# Install Civilization 4 Beyond the Sword and the AdvCiv-SAS mod

To install and play this mod AdvCiv-SAS, you can follow the steps in this document.

## Menu

[Install Civilization 4 Beyond the Sword (skip if already done)](/_1_AdvCiv-SAS/Docs/README_Quick_Install_Setup_Guide.md#install-civilization-4-beyond-the-sword-skip-if-already-done)\
[Download this mod AdvCiv-SAS](/_1_AdvCiv-SAS/Docs/README_Quick_Install_Setup_Guide.md#download-this-mod-advciv-sas)\
&emsp;[Stable Version](/_1_AdvCiv-SAS/Docs/README_Quick_Install_Setup_Guide.md#stable-version)\
&emsp;[Development version](/_1_AdvCiv-SAS/Docs/README_Quick_Install_Setup_Guide.md#development-version)\
&emsp;[Any version](/_1_AdvCiv-SAS/Docs/README_Quick_Install_Setup_Guide.md#any-version)\
[Extract the mod to your civ4 BTS/BTS/Mods folder](/_1_AdvCiv-SAS/Docs/README_Quick_Install_Setup_Guide.md#extract-the-mod-to-your-civ4-btsbtsmods-folder)\
[Place a shortcut of the mod on your desktop](/_1_AdvCiv-SAS/Docs/README_Quick_Install_Setup_Guide.md#place-a-shortcut-of-the-mod-on-your-desktop)\
[If nothing happens when launching the mod](/_1_AdvCiv-SAS/Docs/README_Quick_Install_Setup_Guide.md#if-nothing-happens-when-launching-the-mod)\
[Configure the mod](/_1_AdvCiv-SAS/Docs/README_Quick_Install_Setup_Guide.md#configure-the-mod)\
&emsp;[General ideas](/_1_AdvCiv-SAS/Docs/README_Quick_Install_Setup_Guide.md#general-ideas)\
&emsp;[Disable the Steam overlay](/_1_AdvCiv-SAS/Docs/README_Quick_Install_Setup_Guide.md#disable-the-steam-overlay)\
&emsp;[Settings if you want to develop/modify the mod or try some autoplay or such (skip this section if only playing without wanting extra details)](/_1_AdvCiv-SAS/Docs/README_Quick_Install_Setup_Guide.md#settings-if-you-want-to-developmodify-the-mod-or-try-some-autoplay-or-such-skip-this-section-if-only-playing-without-wanting-extra-details)\
[Mod Version](/_1_AdvCiv-SAS/Docs/README_Quick_Install_Setup_Guide.md#mod-version)\
&emsp;[Version number](#version-number)\
&emsp;[Upgrading/Downgrading version of the mod](/_1_AdvCiv-SAS/Docs/README_Quick_Install_Setup_Guide.md#upgradingdowngrading-version-of-the-mod)\
&emsp;[If you have trouble downloading/installing/using/upgrading/downgrading the mod](/_1_AdvCiv-SAS/Docs/README_Quick_Install_Setup_Guide.md#if-you-have-trouble-downloadinginstallingusingupgradingdowngrading-the-mod)\
[Modify the mod (its development version)](/_1_AdvCiv-SAS/Docs/README_Quick_Install_Setup_Guide.md#modify-the-mod-its-development-version)\
[Where to download more/other mods](/_1_AdvCiv-SAS/Docs/README_Quick_Install_Setup_Guide.md#where-to-download-moreother-mods)

## Install Civilization 4 Beyond the Sword (skip if already done)

First you need to install (or have it installed already) Civilization 4 Beyond the Sword. (skip if already done)

If you don't have the game, i recommend buying it from GOG rather than Steam (i have no affiliation with either xd, although i have the steam version myself for convenience and such, as the executable or whatever they call it is cleaner and closer if not entirely unmodified unlike the Steam one, but check to be sure).

It should work fine with the Steam one too, but given the choice again i may have bought it from GOG rather hehe, although it is not certain as Steam one is convenient too for library access of other games or centralized games lbirary for me i mean or such, but i hope this information helps you decide, check i mean too or for extra or updated info if any change has been made since maybe.

## Download this mod AdvCiv-SAS

### Stable Version

You can download the latest stable version from:

- the [CFC Modpacks downloads page](https://forums.civfanatics.com/resources/advciv-sas-simple-advanced-strategy.32513/)
- the [ModDB website](https://www.moddb.com/mods/advciv-sas-simple-advanced-strategy)
- or alternatively from GitHub (you can get it from the [github tags](https://github.com/wonderingabout/AdvCiv-SAS/tags) page (that also has links to cumulative changes since last version in git history format for each version), or clone from github if you know how as you prefer).

### Development version

If you want the latest functionnalities even if it might be buggy (generally not, but saying this just in case it might be), consider also testing the development version! It should be further enhanced with possibly a stronger AI or other changes if any (if you want to see the list of changes, see the [git commit history, as of now here](https://github.com/wonderingabout/AdvCiv-SAS/commits/main/)).

To download the development version, go to the [mod's github main page](https://github.com/wonderingabout/AdvCiv-SAS), and click on the green rectangle on the top right, then click "download zip".

(example of how to do it in the screenshot below, click to view it full screen)

<img src="../Images/tools/0.250_download-zip-github.png" alt="0.250_download-zip-github.png" width="250"></img>

Note: if you want to modify AdvCiv-SAS, this player development version is missing some files, see [Modify the mod (its development version)](/_1_AdvCiv-SAS/Docs/README_Quick_Install_Setup_Guide.md#modify-the-mod-its-development-version) for details.

Update: now that i now seemingly know how to use .pdb files with WindDbg to inspect crashes or code issues precisely (it tells the line that crashed it seems), as was used to successfully resolve [KI#475.2](/_1_AdvCiv-SAS/Docs/README_Known_Issues.md#ki-475.2) and [KI#475.3](/_1_AdvCiv-SAS/Docs/README_Known_Issues.md#ki-475.3), i may not update as often the Release DLL if it is too tedious, as i may have an already compiled locally Debug-opt DLL, but that seems inconvenient to share for legal or sth reason (check if accurate as i don't know much about these) according to ChatGPT or Codex so i may not share it, meaning i may not update the shipped Release DLL as often as well.

### Any version

With github you can actually download the mod at any version/commit if i'm not mistaken, but it is a bit less intuitive how to do it if you don't know how, todo write.

## Extract the mod to your civ4 BTS/BTS/Mods folder

Extract the archive in the Mods folder of your civ4 BTS/BTS folder (be careful twice BTS (i.e. "Beyond The Sword")), for example, using Steam the path of AdvCiv-SAS should be **(remove version name such as "-5860" or any name like "-main" (git branch name, remove when extracting to your civ4 mods folder)** or similar or anything else xd in your folder destination name so it is **strictly "AdvCiv-SAS"**):

```C:\Program Files (x86)\Steam\steamapps\common\Sid Meier's Civilization IV Beyond the Sword\Beyond the Sword\Mods\AdvCiv-SAS\```

If you don't use Steam you can still play this AdvCiv-SAS mod, you'll just have to change the path to where civ4 was installed (and might have less DRMs at the same time so may be even better as you prefer).

Note: be careful while extracting, a common error (which i made too having no clue why it didn't work...) is to extract with an extra subfolder, then i don't think the game would function. So for example for the steam version of the game, one of the mod files such as AdvCiv-SAS.ini should be found in `C:\Program Files (x86)\Steam\steamapps\common\Sid Meier's Civilization IV Beyond the Sword\Beyond the Sword\Mods\AdvCiv-SAS\AdvCiv-SAS.ini` and NOT in `C:\Program Files (x86)\Steam\steamapps\common\Sid Meier's Civilization IV Beyond the Sword\Beyond the Sword\Mods\AdvCiv-SAS\AdvCiv-SAS\AdvCiv-SAS.ini` (i.e. don't do this `\AdvCiv-SAS\AdvCiv-SAS\`).

## Place a shortcut of the mod on your desktop

Finally, add a shortcut to your desktop which link is, for example for Steam:

```"C:\Program Files (x86)\Steam\steamapps\common\Sid Meier's Civilization IV Beyond the Sword\Beyond the Sword\Civ4BeyondSword.exe" mod=\AdvCiv-SAS```

For convenience i have also provided a steam shortcut of this same link just above (since this is the version i have i.e. the steam one of the game), ready to use, (AdvCiv-SAS - Steam shortcut.lnk: see [Windows_shortcuts](/_1_AdvCiv-SAS/Windows_shortcuts/)). You can just move it to the desktop for your convenience.

Note: Make sure you run AdvCiv-SAS from the windows shortcut and not another way.

<img src="../Images/tools/0.260_windows_shortcut_example.PNG" alt="0.260_windows_shortcut_example.PNG" width="250"></img>

If you don't see the same main menu wallpaper as below when opening AdvCiv-SAS, then maybe other things could be missing as well, which could lead to unexpected errors or missing features. So it is recommended to run AdvCiv-SAS from a windows shortcut.

<img src="../Images/main_menu/0.300_main_menu_home (1).JPG" alt="0.300_main_menu_home (1).JPG" width="250"></img>

Note 2: you can do this same process (download and install a Civ 4 mod(s), then place a shortcut of it to desktop or wherever you want/prefer) for any number of mods you want, just extract it in same Mods folder but with a different folder name, for example you could play Cavemen2Cosmos, Middle-earth mod, even AdvCiv alongside AdvCiv-SAS as long they are in different folders, they are/should be totally indepedent in theory if i'm not mistaken, the settings or files of one will/should not override or affect the other mods, thanks to the very awesome design of Civ 4, thanks!

## If nothing happens when launching the mod

Disclaimer: try at your own risk.

Sometimes, rarely, when launching the mod nothing happens (i.e. the mod does not start as usual). I don't know the cause, but empirically i have found that going to Windows Task Manager, right click on civ4 process, kill it, then launch the mod again fixes it.

<img src="../Images/tools/0.270_kill_civ4_process.png" alt="0.270_kill_civ4_process.png" width="250"></img>

## Configure the mod

### General ideas

Recommended, change your config as you prefer. For example as for me, i play at 1920x1080 (seems to display best ingame, even though i have a larger screen actually (4K) i also optimized Sevopedia features like the AI personality panel for 1920x1080 (but may work as well for higher resolutions although i didn't test it)) in full screen, but when i need to debug or test the windowed mode is so much more practical.

There are some options i use personally xdin ingame settings then like (from memory since i can't find the txt files where i stored it if i ever did (which i think i did)), so do as you prefer, but as for me i use these civ4 options for example if not already enabled before (non-exhaustive in case i forgot / didn't show some):

- wait at end of turn
- quick attack
- quick defense
- quick moves
- single unit graphics
- numbers on city bar

Then in BUG Menu options i also use this for example hehe but do as you prefer as well (non-exhaustive in case i forgot / didn't show some):

- wide city bars

### Disable the Steam overlay

Optionally, for steam users, consider disabling the Steam Overlay if you don't use it in Civ4; allows access to the F12 key that won't screenshot anymore (as of now default In-game is Open Sevopedia).

### Settings if you want to develop/modify the mod or try some autoplay or such (skip this section if only playing without wanting extra details)

(skip this section if only playing without wanting extra details)

If you're developping a mod, or simply want to run some autoplays or such yourself, you'd most likely want to enable debug mode as well as do autoplay runs yourself. See [Modding_Ressources/README.md#how-to-autoplay-let-the-ai-play-for-you-super-fast-gameplay--testing-tool-in-map-loaded-save-file-new-game-etc-view](/_1_AdvCiv-SAS/Docs/Modding_Ressources/README.md#how-to-autoplay-let-the-ai-play-for-you-super-fast-gameplay--testing-tool-in-map-loaded-save-file-new-game-etc-view) for details on how to do it and such.

Also, if you're developping/modding or doing autoplays or such, i highly recommend "windowed" rather than fullscreen (`fullscreen = 0` in CivilizationIV.ini file (or maybe in ingame settings although i didn't test it)), as this is so much more convenient in order to do screenshots and browse them easily, as well as go back and forth to your windows folders without having a lag of several seconds everytime you click outside of the game. Otherwise fullscreen is nice for playing i think and much more immersive at least i'd prefer it i think, but fullscreen is way too tedious when modding and testing.

## Mod Version

### Version number

See [/README.md (Version number)](/README.md#version-number).

### Upgrading/Downgrading version of the mod

If you want to change the version of AdvCiv-SAS (be it an upgrade to a newer version, or a downgrade to an older version, process is the same), delete the entire mod folder and put instead your new version of AdvCiv-SAS (deleting it does not delete savegames as they are not located in this mod folder anyway if i'm not mistaken) (or make a backup if you want to be safe of the old AdvCiv-SAS folder until you see all works well or want to keep it longer in case, but it shouldn't be useful unless the development version crashes and you want to switch back (even then you could just redownload it if you want to)).

### If you have trouble downloading/installing/using/upgrading/downgrading the mod

Consider asking the question in [the AdvCiv-SAS CFC forum's discussion thread here](https://forums.civfanatics.com/threads/advciv-sas-simple-advanced-strategy.699716/) rather than messaging me privately (although i don't mind, just it would be more useful if other people see the question (and does some publicity for me xd at the same time if i may say shamelessly xd, although it is a non-financial profit mod if i may say too) and reply in case they encounter same issue or such, but do as you prefer and hopefully i or others can provide some help).

## Modify the mod (its development version)

If you want to modify AdvCiv-SAS (or whatever this mod is named), the player "download ZIP" options do not include all files (images, some texts, etc.) as they are not needed just to play yet take significant size needlessly.

You'd need to git clone this repo or some similar method so that the exclusion list at [`.gitattributes`](../../.gitattributes) is not applied.

If you rebrand/fork AdvCiv-SAS, the runtime display/project name is centralized through `SAS_MOD_DISPLAY_NAME` in [`GlobalDefines_advciv_sas.xml`](/Assets/XML/GlobalDefines_advciv_sas.xml). Some startup/theme filesystem paths cannot use that runtime define and must still be changed manually; see the checklist next to the define and [Main Changes Guide: Mod Name/Version](/_1_AdvCiv-SAS/Docs/README_Main_Changes_Guide.md#mod-nameversion).

## Where to download more/other mods

If you want to try/play other mods than AdvCiv-SAS or browse them (but you may already know if you downloaded this, still, or if not, maybe this can help you too), consider visiting, among other possible sources/websites:

- CivFanatics Center (also known as CFC) 's forums -> Civ 4 forum -> Modpacks forum (is a forum of a forum called a forum(?)): [civfanatics website's civ4 modpacks forum](https://forums.civfanatics.com/forums/civ4-modpacks.171/) or the [civfanatics website's civ4 modpacks downloads section](https://forums.civfanatics.com/resources/categories/civ4-modpacks.2/)
- ModDB (but does not have as much mods i think, AdvCiv (base mod) is not listed for example (which is the most interesting of the AdvCiv mods i think, except my awesome (xd) mod maybe as interesting maybe as AdvCiv)): [ModDB website (Civilization 4 Mods)](https://www.moddb.com/games/civilization-iv/mods).
