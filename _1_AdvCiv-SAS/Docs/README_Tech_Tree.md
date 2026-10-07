# README_Tech_Tree.md

Below are the details on how the tech tree was made in/for AdvCiv-SAS as well as links to other related docs in AdvCiv-SAS or external sources as well if any.

If any images below in this readme mention a google drive link, you can access it from main readme's entire google drive folder link, or for convenience access directly closest google drive folder at least as of now here in [Misc 0.x file images google drive folder link](https://drive.google.com/drive/folders/1-s26vjr5m9J9vPTIH-kSwpgkoqjmMjl2?usp=sharing).

## Menu

[Current Tech Tree ingame](/_1_AdvCiv-SAS/Docs/README_Tech_Tree.md#current-tech-tree-ingame)\
[Additional info about the tech tree](/_1_AdvCiv-SAS/Docs/README_Tech_Tree.md#additional-info-about-the-tech-tree)\
[More info on notes](/_1_AdvCiv-SAS/Docs/README_Tech_Tree.md#more-info-on-notes)\
[Abstract timeline tech tree](/_1_AdvCiv-SAS/Docs/README_Tech_Tree.md#abstract-timeline-tech-tree)\
[Earlier prototype / alternative version](/_1_AdvCiv-SAS/Docs/README_Tech_Tree.md#earlier-prototype--alternative-version)\
[Earlier but much later version](/_1_AdvCiv-SAS/Docs/README_Tech_Tree.md#earlier-but-much-later-version)\
[Starting techs rework](/_1_AdvCiv-SAS/Docs/README_Tech_Tree.md#starting-techs-rework)\
[Recent tech rework](/_1_AdvCiv-SAS/Docs/README_Tech_Tree.md#recent-tech-rework)\

## Current Tech Tree ingame

<img src="../Images/advisors/0.5160_tech_tree_merged.png" alt="0.5160_tech_tree_merged.png" width="250"></img>

## Additional info about the tech tree

In AdvCiv-SAS, history starts at -50 000 BCE (may not be accurate if not latest updated, view `<DefineName>START_YEAR</DefineName>` in (adjust to your mod path) `C:\Program Files (x86)\Steam\steamapps\common\Sid Meier's Civilization IV Beyond the Sword\Beyond the Sword\Mods\AdvCiv-SAS\Assets\XML\GlobalDefines_advc.xml`, and the code comment just before `<GameTurnInfos>` (or the code in this xml tag too that this is indeed an xml tag) in (adjust to your mod path similarly) `C:\Program Files (x86)\Steam\steamapps\common\Sid Meier's Civilization IV Beyond the Sword\Beyond the Sword\Mods\AdvCiv-SAS\Assets\XML\GameInfo\CIV4GameSpeedInfo.xml`) and quickly spans until bronze age, then gradually slowing down in the process (as in civ4 (base advciv too?)), perhaps a bit more gradual and less very slow endgame perhaps todo or would not be done.

At first and before doing a/the (more) simplified tech tree above, the original tech tree approach i had in mind for AdvCiv-SAS was a more complicated or rather complex one (see [README_Tech_Tree.md#abstract-timeline-tech-tree](/_1_AdvCiv-SAS/Docs/README_Tech_Tree.md#abstract-timeline-tech-tree)).

But while refining it and drafting several versions (see also the [misc](/_1_AdvCiv-SAS/Images/misc/) for details or other alternative versions) or the google drive of the mod (link in [the main README.md](/README.md)), i found it more desirable/suitable for AdvCiv-SAS, at least would be how i want to do it rather, to have less inefficient techs (alphabet, paper, that slow the tech progress for no real key tech progress, hindering the feeling of progression for the player perhaps at least for me and making it a drag, making it also less likely to reach end game (modern and such era/techs as is too tedious and long)), repurposing them where i want more relevant techs (consitution, medicine in early eras for example, naturalism in renaissance), AI tech ideally too if we do in modern or future/robotic era if we implement it partly or entirely even if just the techs or maybe an actual era too but in allc cases.

So i hope this more efficient tech tree helps have a smoother game experience for players at least me, and make it more likely and less tedious to reach endgame in an enjoyable way at least to me if not other shopefully or maybe (they) would not, among with other AI changes or other changes as well in AdvCiv-SAS that maybe help (achieve) that or get closer to that goal

## More info on notes

View also notes files that have tech in their name, or and other notes files that while not directly named with "tech" in their filename, may also contain directly or indirectly related tech information (ideally i should have (more maybe separated the info, but sometimes also the info is intertwined so not possible to separate unless duplicating the info, so or i hope there is still enough info in these docs hopefully or not))

These files include (may not be exhaustive in case i forgot some or maybe i didn't) (note: see them in the mod's google drive to view the file and for details, some text in them is not updated or not relevant anymore, some of it should be informative about some technological choices in advciv-sas):

- notes_about_tech_design_choices.txt
- notes_about_tech_design_indirect_associations.txt
- notes_about_tech_design_swapped_simplified_tree.txt
- notes_about_techs_civ4_to_remove_or_replace_or_add.txt

## Abstract timeline tech tree

(click on google drive link at top of this readme to view it on google drive rather at full size)

![0.33_Techtree_modified.png](https://drive.google.com/thumbnail?id=1XZqhjw0PFoBR2YY0M7uhtddSeo2kinWo&sz=w4000)

As explained in the [README_Tech_Tree.md#more-info-on-notes](/_1_AdvCiv-SAS/Docs/README_Tech_Tree.md#more-info-on-notes) section just above, there are more details there (i.e. in these notes about why and how the tech tree was made as such)

## Earlier prototype / alternative version

Very early before starting to develop this advciv-sas mod (not long before it), i had made this prototype reworked tech tree that is done only by swapping techs (click on google drive link at top of this readme to view it on google drive rather at full size)

(click on google drive link at top of this readme to view it on google drive rather at full size)

![0.30_tech_tree_mini.jpg](https://drive.google.com/thumbnail?id=1ySmVauqYXmBKJhHKwLMFGvnoVaVkmpXL&sz=w4000)

It is actually what motivated me to start the mod, at least one of the reasons, along with sadly not being able to change easily enough base advciv with my contributions/suggestions (see [README.md#authors](/README.md#authors) for bit more details) and realizing with my exchanges or not with @f1rpo main advciv maintainer that indeed i could do it in my own mod rather and not be limited rather than force myself to align with base advciv enough, and also being dependent on them/the base advciv mod.

It is also painful times as this project could not be implemented and i had to abandon it, but it was not lost if i may say as a lot of the new reworked tech tree took inspiration on this original idea even though i changed it quite a bit since then, it was also a lot of fun to do and very challenging as i did it all in one day with no other modification allowed than swapping and replacing techs (total num the same). More details as well here in [AdvCiv/pull/10](https://github.com/f1rpo/AdvCiv/pull/10) even though is bit painful to remember sharing for exhaustiveness and as something to look back on for me at least and in this case.

Actually quite lto painful not sure if i'll keep it longterm in the readme, maybe will or maybe not if too tedious to remember to remove will see, but for now i want to add this, is also good opportunity to make mod lighter by removing such heavy unnecessary images since we dont use them as much but helpful to reference to everywhile if i am reworking the tech tree maybe or not if i may say  (i could have directly removed them entirely but useful at least for now to have them here if not for always or maybe not).

## Earlier but much later version

Then much later i did this swapping based version but that was based or closer (i don't remember exactly but i think it was as such) on the abstract time line that is shown in this section as well,, i thought i could gain some benefits from representing the tech tree in real civ4 design before reworking it, which i think helped to more easily visualize how techs could fit and their effects and such (even though it's essentially the same info dispalyed in another way, it helped to organize it and such as well and reflect on it if i remember or guess how it went correctly, it may have also helped to visualize a bit too even if less and was not too long considering it was mostly matching previous prototype and abstract timeline from my memories of it), before we used the real ingame reworked tech tree as in docs, this is how it went and why, not 100% sure but quite close.

(click on google drive link at top of this readme to view it on google drive rather at full size)

![0.30b_tech_tree_mini_temp.png](https://drive.google.com/thumbnail?id=1e42RDsufVEuBpY9ZVii2uFC65wXP-wnt&sz=w4000)

## Starting techs rework

After changing the tech tree, starting techs had to be adjusted. See [README_Assets_Rebalancing.md (Civs' Starting techs rework)](/_1_AdvCiv-SAS/Docs/README_Assets_Rebalancing.md#civs-starting-techs-rework) for details/results.

## Recent tech rework

Much later, it now appears better not to be strictly historically accurate when gameplay-relevant, for example techs appearing slightly earlier or later than they would when practical (e.g. Metal Casting early so Bronze Working can be soon enough (in a fantasy setting it can be imagined early civs started tinkering with metals about when they discovered Agriculture or started to settle down as civilizations or such)).

Or for example when balanced or practical in terms of gameplay (e.g. religions appearing too late or too few or not varied enough to be gameplay relevant or effective at all) and strategic value (e.g. if all religions beeline from same tech path it would not be too balanced or strategically diverse or valuable).

This new vision happens to fit my original wish more, of Civ being a boardgame like strategy simulator, rather than a history reenactment which i don't find as appealing, so although interesting to some extent, it is also constraining, and generally and regardless i think i value gameability and choosing strongest strategies much more than carefull replay the known history timeline, which i don't care as much if at all about in retrospect i think.
