# README (Modding Ressources)

This is a non-exhaustive page containing some ressources i found helpful and general help on how i modded AdvCiv-SAS.

## Menu

[Modding disclaimer and general information](/_1_AdvCiv-SAS/Docs/Modding_Ressources/README.md#modding-disclaimer-and-general-information)\
[DLL compilation guide (command-line workflow primarily for AI/LLM helpers)](/_1_AdvCiv-SAS/Docs/Modding_Ressources/README_DLL_Compilation.md)\
[Release preparation/checklist (for user + AI/LLM helpers)](/_1_AdvCiv-SAS/Docs/Modding_Ressources/README_Release_Process.md)\
[LLM-assisted source analysis](/_1_AdvCiv-SAS/Docs/Modding_Ressources/README.md#llm-assisted-source-analysis)\
&emsp;[Light-source analysis](#light-source-analysis)\
&emsp;[Historical source archaeology](#historical-source-archaeology)\
&emsp;[C++ File Audit Album](#c-file-audit-album)\
[A few useful tips](/_1_AdvCiv-SAS/Docs/Modding_Ressources/README.md#a-few-useful-tips)\
&emsp;[Insert a google drive image link in an html tag](/_1_AdvCiv-SAS/Docs/Modding_Ressources/README.md#insert-a-google-drive-image-link-in-an-html-tag)\
&emsp;[Download media assets for example on youtube](/_1_AdvCiv-SAS/Docs/Modding_Ressources/README.md#download-media-assets-for-example-on-youtube)\
&emsp;[add media assets](/_1_AdvCiv-SAS/Docs/Modding_Ressources/README.md#add-media-assets)\
&emsp;[trick to capture fast actions in same turn: press print screen super fast many times if you don't want to bother recording a video](#trick-to-capture-fast-actions-in-same-turn-press-print-screen-super-fast-many-times-if-you-dont-want-to-bother-recording-a-video)\
&emsp;[write git commit message gradually as you do changes before committing them, and keep notes of ideas](/_1_AdvCiv-SAS/Docs/Modding_Ressources/README.md#write-git-commit-message-gradually-as-you-do-changes-before-committing-them-and-keep-notes-of-ideas)\
&emsp;[how to create/edit git tags at specific commits with a tag message](/_1_AdvCiv-SAS/Docs/Modding_Ressources/README.md#how-to-createedit-git-tags-at-specific-commits-with-a-tag-message)\
&emsp;&emsp;[simple version(s)](#simple-versions)\
&emsp;&emsp;[More advanced version to also set the date of the tag to last commit and not time of tagging, plus using shell variables so less tedious](#more-advanced-version-to-also-set-the-date-of-the-tag-to-last-commit-and-not-time-of-tagging-plus-using-shell-variables-so-less-tedious)\
&emsp;&emsp;&emsp;[example with more advanced version (totally not shameless (or yes xd)) (but it might also help as well)](#example-with-more-advanced-version-totally-not-shameless-or-yes-xd-but-it-might-also-help-as-well)\
&emsp;[git log with anonymized email in a .txt](/_1_AdvCiv-SAS/Docs/Modding_Ressources/README.md#git-log-with-anonymized-email-in-a-txt)\
&emsp;[published web release changelogs in .txt](/_1_AdvCiv-SAS/Docs/Modding_Ressources/README.md#published-web-release-changelogs-in-txt)\
&emsp;[manual(s) and docs in .txt](/_1_AdvCiv-SAS/Docs/Modding_Ressources/README.md#manuals-and-docs-in-txt)\
&emsp;&emsp;[advciv id changes manual.txt results](/_1_AdvCiv-SAS/Docs/Modding_Ressources/README.md#advciv-id-changes-manualtxt-results)\
&emsp;[Sources about XML AI Attributes and their meaning](/_1_AdvCiv-SAS/Docs/Modding_Ressources/README.md#sources-about-xml-ai-attributes-and-their-meaning)\
[A few useful other links](/_1_AdvCiv-SAS/Docs/Modding_Ressources/README.md#a-few-useful-other-links)\
[In case UnitAIs info from link above is deleted someday, here is a copy here in case](/_1_AdvCiv-SAS/Docs/Modding_Ressources/README.md#in-case-unitais-info-from-link-above-is-deleted-someday-here-is-a-copy-here-in-case)\
[Mapscripts](/_1_AdvCiv-SAS/Docs/Modding_Ressources/README.md#mapscripts)\
&emsp;[Locations](#locations)\
&emsp;[Map types](#map-types)\
&emsp;[old info](#old-info)\
[Where to find Python errors or debug (so no need to copy them either if you need to ask chatgpt or such or a forum)](/_1_AdvCiv-SAS/Docs/Modding_Ressources/README.md#where-to-find-python-errors-or-debug-so-no-need-to-copy-them-either-if-you-need-to-ask-chatgpt-or-such-or-a-forum)\
[Python 2.4 compatibility for modding](#python-24-compatibility-for-modding)\
[How to check with ruff installed locally a folder for py linter errors candidates to fix with text report](/_1_AdvCiv-SAS/Docs/Modding_Ressources/README.md#how-to-check-with-ruff-installed-locally-a-folder-for-py-linter-errors-candidates-to-fix-with-text-report)\
[How to enable .dmp files so for some crashes that don't immediately exit you get a "splash screen" (whatever it is called) and can dmp and see turn at crash](/_1_AdvCiv-SAS/Docs/Modding_Ressources/README.md#how-to-enable-dmp-files-so-for-some-crashes-that-dont-immediately-exit-you-get-a-splash-screen-whatever-it-is-called-and-can-dmp-and-see-turn-at-crash)\
[Another example of how to use VS Code global search (also shows an example of how to also browse the CIV4 BUG_Doc (copy included in our mod))](/_1_AdvCiv-SAS/Docs/Modding_Ressources/README.md#another-example-of-how-to-use-vs-code-global-search-also-shows-an-example-of-how-to-also-browse-the-civ4-bug_doc-copy-included-in-our-mod)\
[Example of how to fast replace many redundant entries (e.g. 400+ strategy entries to obsolete) using regexp (with chatgpt 5's help) and vs code](/_1_AdvCiv-SAS/Docs/Modding_Ressources/README.md#example-of-how-to-fast-replace-many-redundant-entries-eg-400-strategy-entries-to-obsolete-using-regexp-with-chatgpt-5s-help-and-vs-code)\
[Which XML version file to choose to import in your mod when there are multiple ones (for example one in base Civ4 (vanilla is it called?) folder and one in BTS folder)](/_1_AdvCiv-SAS/Docs/Modding_Ressources/README.md#which-xml-version-file-to-choose-to-import-in-your-mod-when-there-are-multiple-ones-for-example-one-in-base-civ4-vanilla-is-it-called-folder-and-one-in-bts-folder)\
[Civ4 ProcessInfos XML info](/_1_AdvCiv-SAS/Docs/Modding_Ressources/README.md#civ4-processinfos-xml-info)\
[ICONS code XML names (with some color changing code as bonus or to be extra exhaustive a bit more than needed but is here if needed maybe)](/_1_AdvCiv-SAS/Docs/Modding_Ressources/README.md#icons-code-xml-names-with-some-color-changing-code-as-bonus-or-to-be-extra-exhaustive-a-bit-more-than-needed-but-is-here-if-needed-maybe)\
[Insert an img in a text label in Sevopedia](/_1_AdvCiv-SAS/Docs/Modding_Ressources/README.md#insert-an-img-in-a-text-label-in-sevopedia)\
[Import a nif art asset from another mod (example with how we added the impluvium's nif as an import from FFH2 mod's Adventurer's guild building)](/_1_AdvCiv-SAS/Docs/Modding_Ressources/README.md#import-a-nif-art-asset-from-another-mod-example-with-how-we-added-the-impluviums-nif-as-an-import-from-ffh2-mods-adventurers-guild-building)\
[(Not used anymore in advciv-sas) Example of how to add a static image file (such as .PNG, .JPG, etc) as a leader portrait](/_1_AdvCiv-SAS/Docs/Modding_Ressources/README.md#not-used-anymore-in-advciv-sas-example-of-how-to-add-a-static-image-file-such-as-png-jpg-etc-as-a-leader-portrait)\
[(Not used anymore in advciv-sas) Example of how to create a leader head .dds button from a static image file (such as .PNG, .JPG, etc) (ideally from the leader head image portrait if any, but if i may say)](/_1_AdvCiv-SAS/Docs/Modding_Ressources/README.md#not-used-anymore-in-advciv-sas-example-of-how-to-create-a-leader-head-dds-button-from-a-static-image-file-such-as-png-jpg-etc-ideally-from-the-leader-head-image-portrait-if-any-but-if-i-may-say)\
[.dds (button) size comparison analysis - (Trying to find) optimal .dds button size for a 164px (+ 2(px?) due to small mistake) icon frame in Sevopedia unit's unit pane('s icon frame)](/_1_AdvCiv-SAS/Docs/Modding_Ressources/README.md#dds-button-size-comparison-analysis---trying-to-find-optimal-dds-button-size-for-a-164px--2px-due-to-small-mistake-icon-frame-in-sevopedia-units-unit-panes-icon-frame)\
[How to change the mod's main menu background image (for example to add a static image instead of default base advciv +/- civ4 image)](/_1_AdvCiv-SAS/Docs/Modding_Ressources/README.md#how-to-change-the-mods-main-menu-background-image-for-example-to-add-a-static-image-instead-of-default-base-advciv---civ4-image)\
[How to remove the blue rectangle in main menu screen, change text color for example to black, etc. (in .thm files)](/_1_AdvCiv-SAS/Docs/Modding_Ressources/README.md#how-to-remove-the-blue-rectangle-in-main-menu-screen-change-text-color-for-example-to-black-etc-in-thm-files)\
[How to autoplay (let the AI play for you (super fast gameplay / testing tool)) in map (loaded save file, new game, etc.) view](/_1_AdvCiv-SAS/Docs/Modding_Ressources/README.md#how-to-autoplay-let-the-ai-play-for-you-super-fast-gameplay--testing-tool-in-map-loaded-save-file-new-game-etc-view)\
[Example of DLL modification of CvGameTextMgr.cpp and other related file(s) to add the new "This technology cannot be traded" flag in Sevopedia tech 's placeSpecial and in tech tree view (technology advisor)](/_1_AdvCiv-SAS/Docs/Modding_Ressources/README.md#example-of-dll-modification-of-cvgametextmgrcpp-and-other-related-files-to-add-the-new-this-technology-cannot-be-traded-flag-in-sevopedia-tech-s-placespecial-and-in-tech-tree-view-technology-advisor)\
[Example of DLL modification 2: missing BBAI getters expose them to Sevopedia leader info in gc too for display](/_1_AdvCiv-SAS/Docs/Modding_Ressources/README.md#example-of-dll-modification-2-missing-bbai-getters-expose-them-to-sevopedia-leader-info-in-gc-too-for-display)\
[Example of performance optimization of python loading time (as in when we load the code) and loading times](/_1_AdvCiv-SAS/Docs/Modding_Ressources/README.md#example-of-performance-optimization-of-python-loading-time-as-in-when-we-load-the-code-and-loading-times)\
[Drive](/_1_AdvCiv-SAS/Docs/Modding_Ressources/README.md#drive)

## Modding disclaimer and general information

I don't know a lot about XML, C++, or Python, and (only) got familiar with these languages recently as i started developping for Civ4 this AdvCiv-SAS mod.

I did have previous knowledge though.

I cannot be assumed or expected to be available or responsible to provide any coding help or feedback (i'd prefer to have my peace of mind rather, even though i may occasionnaly or more often, but not guaranteed and maybe not if i don't want or do so or any other reason or not, but i might or might not, just this is a fair warning/caution point about that, anyways), however i hope these few ressources i have accumulated may help those who wish to start their own mod.

I can also not be held responsible for any damage resulting of following these instructions, whether these are followed correctly or incorrectly. I am only providing them as general information, and while i hope it helps, you are responsible for checking it, verifying it, and using it, and any consequence that may happen as a result.

Hopefully you'll find them helpful though, but in case not or something happens, this is just to protect myself and talk to you in all fairness.

Else or additionally, you may find more help asking your question(s) directly on [CivFanaticsCenter's Civ4 Forum](https://forums.civfanatics.com/categories/civilization-iv.143/) rather maybe. Hopefully this data i provided is also helpful though.

## LLM-assisted source analysis

### Light-source analysis

[`LLM_Helpers/make_light_source_zip.py`](/LLM_Helpers/make_light_source_zip.py) packages current source, relevant docs/data, repository state, tracked history context, map references and selected screenshots for external LLM review without copying the full installation or `.git`. Current source and Git remain authoritative; the archive is a compact handoff rather than a separate source tree.

### Historical source archaeology

AdvCiv-SAS keeps a resumable source-archeology investigation for bugs, regressions, suspicious behavior changes, and useful fixes that may have survived through the Civ4 BtS -> K-Mod -> AdvCiv -> AdvCiv-SAS lineage.

The core method is not simply to search the current source or trust commit messages. The investigation systematically reviews historical K-Mod and AdvCiv commit diffs and then AdvCiv-SAS runtime-source history, comparing the actual before/after code and tracing suspicious changes across lineage boundaries when needed.

The local light-source tooling makes this practical by keeping greppable commit-diff mirrors, Git logs, map/source references, and exact practical/SHA anchors together; large EOL/whitespace-only churn can be normalized so the substantive code change is still reviewed. Commit messages are useful clues, but the diff and resulting source behavior are the main evidence.

The primary investigation phase was carried out with ChatGPT-5.6-Sol and is now complete for its defined runtime-source scope. At the current completed checkpoint, the tracker records **131 confirmed findings: 43 inherited/pre-SAS and 88 SAS-specific**.

Of those, **84 are fixed**, **46 surgical repairs are intentionally deferred**, and **KI#213** is the one broader repair still pending. This does not mean every deferred fix must be implemented before a release: the investigation and identification work can be complete while implementation continues incrementally.

The fixes are then implemented/reviewed separately, primarily by the project author with GPT-5.6-Sol (Codex) assistance and normal compile/runtime/autoplay validation. Some SAS-origin fixes have also flowed back into base AdvCiv 1.14, and a related SAS tiny-island map-script bugfix was also adopted into Taurus, so the archaeology can improve more than only AdvCiv-SAS.

See the detailed [AdvCiv/K-Mod historical bug-archeology progress tracker](/_1_AdvCiv-SAS/Docs/Source_Analysis/historical_source_archaeology.txt) for the exact scope, counters, review checkpoints, findings, lineage traces, and implementation status.

### C++ File Audit Album

The active [C++ File Audit Album](/_1_AdvCiv-SAS/Docs/Source_Analysis/cpp_file_audit_album.txt) complements commit-oriented archaeology by reviewing every current `.cpp` file from longest to shortest with the associated headers, data and neighboring contracts needed to judge it.

It is especially useful for cross-file and cross-commit interactions that survive only in the final combined tree, and records clean files and false positives alongside findings.

Future Python, XML or other file-oriented analyses belong in [`Docs/Source_Analysis/`](/_1_AdvCiv-SAS/Docs/Source_Analysis/).

## A few useful tips

### Insert a google drive image link in an html tag

Was not easy to find how, but [this stackoverflow answer](https://stackoverflow.com/a/52067077) shows how to do it successfully, ai-generated images as of now in our advciv-sas mod use this successfully as well, see also an implementation example of this in the [README_AI_Generated_Images.md](/_1_AdvCiv-SAS/Docs/README_AI_Generated_Images.md) .md file's `<img>` links for example.

To get the id from the url, what i did and that works as of now is to click on the image in the drive folder containing the image, then the sandwich menu whatever they call it xd, then "open in new window", then the following url such (for example in `https://drive.google.com/file/d/1_CcxxS36yJmp779-oxuQD3QSWI0eBubC/view`) has the id `1_CcxxS36yJmp779-oxuQD3QSWI0eBubC` that i can insert in the src field following the stackoverflow answer instructions/example linked and that as of now seems to work-function fine for all the images we inserted in the git readme as such.

(note: i didn't add too many images as it would be tedious to manage in the drive vs git, but for non-essential ones and especially heavy ones atleast in advciv-sas, moved them to the drive or fetched them for there directly if already there)

Was not easy to find how so i am glad i found how so hopefully these instructions help maybe.

### Download media assets for example on youtube

Disclaimer/note: for copyright disclaimer and such, please read this first [/README.md#copyright-and-disclaimer](/README.md#copyright-and-disclaimer). Note also it is up to you to download or use copyright-safe assets, at least it is your responsibility to do so, these are only provided as general instructions: i have to cover myself xd.

Disclaimer 2: Specifically about downloading from youtube, it seems from chatgpt's explanation and what i understand of it that even if a media file is public domain, downloading it from youtube may violate a license for example if the recording is not public domain, it is your responsibility to verify and make sure you use a copyright-compliant media file for the general instructions below that i provide in hope they help and without any warranty, in addition to any disclaimers in main readme's copyright section and and in this modding_ressources section as well

Moving forward, i found this very nice reddit post on [how to download youtube videos and parts of youtube videos](https://www.reddit.com/r/youtubedl/wiki/howdoidownloadpartsofavideo/) with yt-dlp, i also found this very nice reddit post too i mean very helpfulon [how to download any media file from youtube to mp3 using yt-dlp](https://www.reddit.com/r/youtubedl/comments/130i9og/ytdlp_how_to_automatically_convert_all_audio/)

In case the information of this second link disappears someday, here is a copy of it:

>What I actually ended up doing was adding this to the configuration file:
>
>--audio-format mp3 --embed-thumbnail --add-metadata

Thanks for help thanks

Here is now (I like this theatrical performance of an announcer i am performing xd and very excited to do so in my mind) an example of how to download one, for an audio asset for example here !

(,) As civ4 expects .mp3 files of a music without copyright claim on it, or "royalty free music" as it seems they call it (note: to be honest i think it's kinda shit (the music) but it is helpful and maybe not too bad, but thanks for that), for/to download(ing/) the entire media file as mp3 in this case if i'm not mistaken.

But while writing this, it now played this very nice or cool/relaxing from [this youtube playlist](https://www.youtube.com/watch?v=MM2-z8inpY8&list=PLfP6i5T0-DkLlj5LDluZcpP9n6YlATpSG&index=3) for example; also adding `-x` in yt-dlp.

(note: personally i also removed the thumbnail as it makes file needlessly heavier, plus may cause bugs (untested), also so i don't have to worry needlessly about whether/if the image itself is or may be copyrighted xd)

```bash
yt-dlp -x https://www.youtube.com/watch?v=MM2-z8inpY8 --audio-format mp3 --add-metadata
```

Please note that it is also possible to set a start time and end time, like i mean you don't have to have the whole file but say from 00:30 to 00:55 section/part of the media file if you want, see the first reddit link in this readme.md section for details still available.

Here is an example of how to download same video/media file but from seconds 15 to seconds 45 (i.e. from 00:15 to 00:45 of the video/media file), not sure is best way to do it, but at least seems to work; but since i couldn't find how exactly to download not best audio AND only a part of the video, then i asked chatgpt who told me how to do it so:

```bash
yt-dlp -x --audio-format mp3 --postprocessor-args "-ss 00:00:15.000 -to 00:00:45.000" "https://www.youtube.com/watch?v=MM2-z8inpY8"
```

To download from a specified time to end, use, as chatgpt advsied/told me when i asked it thanks but anwyays etc (thanks me (i.e. / to me too)):

```bash
yt-dlp -x --audio-format mp3 --postprocessor-args "-ss 00:00:15.000" "https://www.youtube.com/watch?v=MM2-z8inpY8"
```

Then untested but to download from beginning to an end time, use similarly as advised/told by chatgpt:

```bash
yt-dlp -x --audio-format mp3 --postprocessor-args "-to 00:00:45.000" "https://www.youtube.com/watch?v=MM2-z8inpY8"
```

Note: if you don't know where/when exactly to cut, see note about Audacity in [/README.md#some-useful-tools-while-doing-this](/README.md#some-useful-tools-while-doing-this).

Note 2: there are other websites than youtube to download or find music, including royalty-free one ideally or free for use or such, see [/README.md#some-useful-tools-while-doing-this](/README.md#some-useful-tools-while-doing-this) for example of an example of tool(s) i mention there to not redundantly repeat myself here and in case it changes too and slightly bit tedious.

### add media assets

Disclaimer/note: for copyright disclaimer and such, please read this first [/README.md#copyright-and-disclaimer](/README.md#copyright-and-disclaimer). Note also it is up to you to download or use copyright-safe assets, at least it is your responsibility to do so, these are only provided as general instructions; i have to cover myself xd.

To add music to a leader in advciv-sas for example (i assume would be the same or very similar in an advciv or civ4 based mod too), do a vs code global search for example of "ewuare_" (new leader we added in advciv-sas for the kingdom of benin) and see in audio xml files (and also a bit in leaderhead too if you track where the corresponding assets such as for example "DIPLO_EWUARE_EARLY" lead to in the leader head info xml file too) for an example of how we added his music.

Note: be very careful! Need to remove the `.mp3` in filename else file is unreadable.

### trick to capture fast actions in same turn: press print screen super fast many times if you don't want to bother recording a video

If you have an issue that happens only for split seconds or even shorter and hard to view in one screenshot, and if recording a video is too much trouble, you can try pressing print screen many times super fast while the behaviour you want to capture in screenshot happens.

I noticed this accidentally or rather unconsciously xd, and it helped me identify an issue that we can only for like 0.2 seconds or such, but that can be captured in a screenshot this way.

See an example of this in the update note in [KI#51](/_1_AdvCiv-SAS/Docs/README_Known_Issues.md#ki-51).

### write git commit message gradually as you do changes before committing them, and keep notes of ideas

I have noticed while developing this advciv-sas mod at some point it is is so helpful to write bit by bit and gradually the commit message rather than do all changes then try to guess/remember which. Not necessarily listing all changes, but the main ones or most relevant or important ones in importance i mean (redundant to say important importance) as bullet points or such as you prefer.

Personally i do so in Notepad++ where i also store notes of ideas of things to add for example. I may often not look at it for a long time, and spend bit of time then sorting the ideas and organizing them but is very helpful so i would recommend this as well as part or and on top of in this case writing commit message notes in a .txt, i'd also recommend writing your ideas in another or same .txt if tidy or readable to you or some similar or as you prefer solution/idea.

I started this habit somewhere in this advciv-sas mod development i mean but is very helpful, so adding mention of it if helps.

### how to create/edit git tags at specific commits with a tag message

Thanks to the help of chatgpt 5, and me noticing how based on past experience or such or current observation, what i mean is [git/github tags](https://github.com/wonderingabout/AdvCiv-SAS/tags) can be very useful to manage versions, provide download links, possibly also use as compare points to remember which commit they were at precisely and fast without needing to remember it yourself.

For example, AdvCiv-SAS 4986 is available at the [4986 git/github tag](https://github.com/wonderingabout/AdvCiv-SAS/releases/tag/4986)

#### simple version(s)

Here is how to do it with the help of chatgpt 5, which i adjusted a bit at some parts (check if accurate too, and make backups of your git repo before trying this just in case even though seems to function fine but is to be safe i mean) (for example i run this from git bash on windows):

If the tag doesn’t exist yet:

```shell
git fetch origin
git tag -a 4986 8d6bf6e0f61004e415aa3584e906fdb77275c1f8 \
  -m "AdvCiv-SAS 4986" \
  -m "Changelog from base AdvCiv 1.12 to version 4986 is here: https://github.com/wonderingabout/AdvCiv-SAS/compare/96790915238bdaf7d1ef38658e69fe1035129687...8d6bf6e0f61004e415aa3584e906fdb77275c1f8"
git push origin 4986
```

If the tag exists but points to the wrong commit:

```shell
git fetch origin
git tag -fa 4986 8d6bf6e0f61004e415aa3584e906fdb77275c1f8 \
  -m "AdvCiv-SAS 4986" \
  -m "Changelog from base AdvCiv 1.12 to version 4986 is here: https://github.com/wonderingabout/AdvCiv-SAS/compare/96790915238bdaf7d1ef38658e69fe1035129687...8d6bf6e0f61004e415aa3584e906fdb77275c1f8"
git push --force origin 4986
```

#### More advanced version to also set the date of the tag to last commit and not time of tagging, plus using shell variables so less tedious

A more advanced version would allow to better control the date we set to the tag. For example, i committed version 4986 as of now more or less 2 weeks ago, however if i create a tag now as above, i generally get the tag dated to now, which doesn't reflect the date when 4986 was made that i would want to show as date of the tag as well, not "now".

To do that, with bit of back and forth wit chatgpt 5 and trial and error (but check if accurate), i have found this very nice version that allows to do that (be bit more careful using it as i don't know if it will always be functionnal (the simple one too but this one in particular xd as it's more technical and might change in the future or apply differently to your use case or something so check if accurate and make a backup of your repo maybe/ideally if i may say just in case))

Result ([for example for tag version 4986 here](https://github.com/wonderingabout/AdvCiv-SAS/releases/tag/4986) as linked before but added here again for exhaustiveness) is great, now our tag is shown "2 weeks ago" (date of the commit of version 4986), and not "now" anymore (date of when i am tagging it as of now today) as we wanted, but again check if accurate and make a backup of your repo just in case, hopefully helpful.

##### example with more advanced version (totally not shameless (or yes xd)) (but it might also help as well)

As of now, this is the command we use to make each new git tag release.

For stable release updates, the update tag/label is the new download for that same stable release, not a separate parallel stable release. For example, `AdvCiv-SAS 5860 Update 1 (2 days after release)` replaces the first `AdvCiv-SAS 5860` release.

```shell
cd "C:\Program Files (x86)\Steam\steamapps\common\Sid Meier's Civilization IV Beyond the Sword\Beyond the Sword\Mods\AdvCiv-SAS"
git fetch origin

# choose names
TAG=6200
NEWNAME="AdvCiv-SAS $TAG"
NEWSHA=9887bed3ce4addb2a03d5c21deba3f63a2b32852

OLDNAME="AdvCiv-SAS 6020"
OLDSHA=feddabdf89f022dcf6412aab0cab414a54b4610c

WHEN=$(git show -s --format=%cI "$NEWSHA") # or %aI if you prefer author date
COMPARE_URL="https://github.com/wonderingabout/AdvCiv-SAS/compare/$OLDSHA...$NEWSHA"

# replace/create the tag locally with correct date + message
GIT_COMMITTER_DATE="$WHEN" GIT_AUTHOR_DATE="$WHEN" \
git tag -fa "$TAG" "$NEWSHA" \
  -m "$NEWNAME" \
  -m "Full cumulative file changes and git commit history from $OLDNAME to $NEWNAME viewable at $COMPARE_URL"

# force-push because we may be updating an existing remote tag object.
# <!-- custom: verify TAG first because this overwrites an existing remote tag when one exists. (GPT-5.5) -->
# <!-- custom: convenient for both new tag releases and replacing an old tag with a new commit, so as of now always using force regardless of which case. (GPT-5.5) -->
git push --force origin "refs/tags/$TAG:refs/tags/$TAG"
```

### git log with anonymized email in a .txt

It can be useful to keep a .txt copy of the full git log of your project, for grep with a global search (for example with VS Code) or such. You can gain while doing so precious information about some features you want to know more about.

To create a git log based on past commits using a comand line, plus also anonymizing the email with a `<hidden>` instead for all authors, as provided to me by chatgpt, for example in git bash for windows, for Steam users (adjust paths or such similar things anyways if not steam user).

Note: although redundant, we as of now keep in code comment the version where it ended, so it is easier to remember where we stopped last and to append only what is new and not everything from scratch again each time (which would lose our manual edits to remove churn/typoes etc).

```shell
# 1. Set your commit hashes here
NEWSHA=9887bed3ce4addb2a03d5c21deba3f63a2b32852 # new (included), e.g. AdvCiv-SAS 6200
OLDSHA=feddabdf89f022dcf6412aab0cab414a54b4610c # old (excluded, which is fine since was part of last batch as the former NEWSHA), e.g. AdvCiv-SAS 6020

# 2. Run the command (uses the variables above)
cd "C:/Program Files (x86)/Steam/steamapps/common/Sid Meier's Civilization IV Beyond the Sword/Beyond the Sword/Mods/AdvCiv-SAS/" && 
git log --pretty=format:"commit %H%nAuthor: %an <hidden>%nDate:   %ad%n%n%B" --date=iso "$OLDSHA".."$NEWSHA" > git_log_anonymized_email.txt
```

Note 2: since as of now git log text docs are updated incrementally unlike releases where we always like to compare vs last stable instead of vs last update of a stable (e.g., 5860 updated vs last 5500 for full git diff, but 5860 update 4 vs last 5860 which is update 3 for git log incremental last addition), the command may use different old and new tags than the git tag release one.

Note 3: it seems that due to this, some commits are not ordered chronologically, for example `24dbe0256ce974067b99f4d545fa8ca23596c344` (2026-04-25 00:18:51 +0200) is part of the AdvCiv-SAS 6020 release batch's changelog, so it is more recent in our git log than the newer `82f042a44f4cd4bbc2b687444356d220b83131c1` 2026-06-04 19:20:56 +0200, but as of now keep it as such for no tedium and clearer/more reliable version boundaries.

For example, i put thes git log files in the [/_0_Common_Docs/git_logs/](/_0_Common_Docs/git_logs/) and [/_1_AdvCiv-SAS/Docs/git_logs/](/_1_AdvCiv-SAS/Docs/git_logs/) folders.

### published web release changelogs in .txt

Full AdvCiv-SAS release/update text manually copied from CivFanatics and/or ModDB is archived in [Docs/changelogs_web/](/_1_AdvCiv-SAS/Docs/changelogs_web/) as version-named plain-text files.

These are historical source copies and complete release-writing examples that complement Git history; preserve their source/date provenance and keep distinct publication texts separate when one release has multiple web sources.

Routine working drafts/checkpoints do not need to be tracked because the repository, Git history, temporary development notes, and complete published examples already provide the useful inputs for the next release.

### manual(s) and docs in .txt

Similarly, i have found it very useful to keep a .txt copy of the docs i use, in particular technical ones, and such as the [AdvCiv_Base_Docs](/_0_Common_Docs/AdvCiv_Base_Doc/) in particular the [manual in .txt](/_0_Common_Docs/AdvCiv_Base_Doc/manual.txt)

When doing a global search with VS Code for anything, as long as this manual.txt for example are in the mods folders (anywhere, this/her is an example, anyways), then i can and may often (not guaranteed but maybe) also find useful information as part of my global search results.

Not necessarily for all docs, or may if you want, but sure is very convenient, hopefully helpful tip or maybe not etc anyways.

#### advciv id changes manual.txt results

note: about the below example, a global search on "HandicapIncrementTurns" directly rather may be more useful (see civ4handicapinfo xml 's code comment(s) for details), but this shows how to navigate the manual.txt and global search features for example how to use them.

VS Code's (for example) global search can be useful in particular but not only, for example, to try to understand what advciv id changes do.

For example while trying to tweak difficulty settings after having added a script for handicap info, i was trying, for example in this case, to understand what this unknown (to me at least) `iAIHandicapIncrementTurns` field does, no info in usual websites like kujira or and such

Output is as follows for a search on "251" and right click "copy" on the global search results (part of them that are in manual.txt):

```log
C:\Program Files (x86)\Steam\steamapps\common\Sid Meier's Civilization IV Beyond the Sword\Beyond the Sword\Mods\AdvCiv-SAS\_0_Common_Docs\AdvCiv_Base_Doc\manual.txt
  172,273: To balance out the remaining changes (especially to Free Speech), the threshold for Legendary city culture gets reduced on the low and medium difficulty levels. (On Monarch, the threshold begins to increase, matching the increasing tech costs relevant for Space victory.) 251
  252,229:     • No impact of global research on inflation; instead, adjusted tech costs based on difficulty and the years-per-turn progression for a more historically accurate tech pace, and Immortal and Deity games now start on turn 10. 251, 910
  297,419:     • On Emperor difficulty and above, some of the AI discounts have been reduced, but human civs face increased production costs, research costs, city growth thresholds and Great Person thresholds. The AI also starts with fewer freebies, in particular, without Agriculture on Immortal and Deity and without a second free Settler on Deity. Progres­sive AI bonuses are now based on the game turn instead of the AI era. 251/ 250e
  2243,1: 251 shows the start turn on the Settings tab when it isn't turn 0.
  4821,1: 251, 910: Tech cost adjustments.
  6489,132: Rationale	An extra defender everywhere is a big production sink now that AI production discounts have been reduced (through change 251), especially in the early game. That forum post probably describes an AI enemy in the Classical era; so I could address that situation (to an extent) by adding the extra defender already in the Classical era on Deity. However, I don't really think that this would make the AI more difficult to beat overall as it's not all that often possible to sneak up on the AI through the territory of a third civ that the AI isn't afraid of. It's also a pretty clever stratagem that I don't mind being rewarded a bit. (The reward isn't going to be that great because the conquered cities won't be connected to the player's core territory.)
  6802,10: See also	251 reduces the AI work rate bonuses from the difficulty setting.
  7811,10: See also	251 adjusts the culture level thresholds to the game difficulty level.
  9721,1: 251 increases tech costs for the above-average difficulty levels.
  9913,1: 251 may show the game start turn.
  9980,1: 251: Removes the culture rate modifier that K-Mod adds to Colosseum. And is concerned with the balancing Culture and Space victory.
  10199,221: The AI advantages that are the same for all difficulty settings, namely the discounts on unit supply and upgrade cost, are unaffected by the AI game progress modifier (per-era modifier in BtS, per game turn since change 251).	The per-era modifier increases most of the AI advantages with each passing era. 
  10203,10: See also	251 exempts all AI gold costs from game progress adjustments.
  10221,266: See also	I guess changes to map sizes (137, 165), starting positions (027) and AI behavior have made it considerably more difficult to establish at least four decent cities in the early game. Converting AI worker speed increases into a human worker speed decrease (251) may also have had the (unintended) effect of hampering the early development of human civs more than that of AI civs.
  10224,41: See also	The Deity research rate set by 251 is adjusted to this change.
  10230,1: 251	Pacing adjustments for difficulty levels
  10343,5: See 251 about Marathon culture level thresholds.
  10359,1: 251 uncouples AI unit expenses from the progressive per-era modifier, meaning that AI unit expenses don't depend on the difficulty level in AdvCiv (always halved).
  10449,10: See also	251 shows the game start turn on the Settings tab if it isn't turn 0.
  10556,55: See also	To match the increased production costs that 251 imposes on the civs.
  11371,10: See also	251 increases some expenses for the medium difficulty levels and reduced the number of free wins against Barbarians.
  11854,89: See also	200 reverts the K-Mod changes to Colosseum (25% culture, +20 production cost); 251 lowers the Legendary culture threshold instead.
  11878,10: See also	251 adjusts the AI research speed based on difficulty.
```

While there is some unrelated info of it/about it, some results may also help us also (in (trying to?)?) understanding what this 251 id change, or maybe rather for our need specifically what the `iAIHandicapIncrementTurns` tag/field does.

One of the main points of this is also to show a manual.txt is much easier to navigate and search on that the .pdf version of it, with VS Code global search or even with a normal text editor or such when simply looking for fast (access) info rather than detailed and nicely displayed one.

### Sources about XML AI Attributes and their meaning

You can read these docs if you want to know more about AI attributes:

- [kujira's website](https://gforestshade.github.io/kujira/post/civ4leaderheadinfos/#iclosebordersattitudechange) or for example list of unitAIs [kujira](https://gforestshade.github.io/kujira/post/keyichiran/#%E3%83%A6%E3%83%8B%E3%83%83%E3%83%88) and
[cfc forum with what they do](https://forums.civfanatics.com/threads/list-of-unitais-and-what-they-do.353919/post-8910602), see also [translate to english using web browser trick/technique for/in kujira's website](https://github.com/wonderingabout/kujira)
- [modiki civfanatics website](https://modiki.civfanatics.com/index.php/Civ4LeaderHeadInfos) or [modiki's website too](https://modiki.civfanatics.com/index.php/Civ4TraitInfos) for example
- [wikidot website for example for bonusinfos](http://civ4.wikidot.com/xml:civ4bonusinfos)

## A few useful other links

- [Local Python 2.4 compatibility checks and setup](#python-24-compatibility-for-modding), including the official Windows installer link and an example of syntax that modern Python accepts but Civ4 cannot parse.
- many links in the main README.md first for example maybe indeed in the [main README.md 's page link](/README.md)
- todo: tutorial on how to compile the DLL or files for Advciv (but should not be needed if you only modify XML files)
- [Python Class Reference (civ4bug.sourceforge.net)](https://civ4bug.sourceforge.net/PythonAPI/) List of all or many(?) python class detailed info, can be useful for example it helped me implement the Sevopedia religion's (working and finalized at least as of now) right panel version of leaders panel, telling [Authors for details (Claude AI's section) in the main README.md](/_1_AdvCiv-SAS/Docs/README_Authors.md#claude-web-chat) to adjust its code for the `screen.addMultiListControlGFC` to display leaders in a multi line button panel, see Sevopedia religion's code and code comments for details.
  - I have also made a copy of it (this website's content that is helpful) here in advciv-sas, in the file [civ4bug_pythonAPI_AllClasses_html.txt](/_0_Common_Docs/CIV4BUG_Sourceforge_net_All_Classes_Doc/civ4bug_pythonAPI_AllClasses_html.txt), may help us also for global search (with vs code for example).
- inspecting an object, see for example, one way i found it to work is using `printObjAttrs` in [_sevopedia_debuggers.py](/Assets/Python/Contrib/Sevopedia/_sevopedia_debuggers.py) and [Sevopedia/Debug/](/Assets/Python/Contrib/Sevopedia/Debug/) which helped me adjust Claude AI's (see [Authors for details (Claude AI's section) in the main README.md](/_1_AdvCiv-SAS/Docs/README_Authors.md#claude-web-chat)) code of the new placeTerrainFeatureCity or similarly named Sevopedia unit function/method, not using the inaccurate methods Claude AI provided, at least in advciv and advciv-sas by extension since we use same code as in advciv, `getHillsAttackPercent()` and `getHillsDefensePercent()` but instead the real at least as appears in our inspect of the unitinfo as linked just above in same bulelt point indeed with `getHillsAttackModifier()` and `getHillsDefenseModifier()` of an otherwise very great code that worked at first try!!!
  - (see also [this screenshot (1) (google drive image link)](https://drive.google.com/file/d/1oE0tG3VjCY7a5ABwlV0MYDhA3ox3Sbe4/view?usp=sharing) and [this screenshot (2) (google drive image link)](https://drive.google.com/file/d/1Yn7dIxzDpuJ2f8wbpXmhn2W0qZ42ADOe/view?usp=sharing) for example of how i solved it, with the help of Claude AI thanks

## In case UnitAIs info from link above is deleted someday, here is a copy here in case

quote from website linked before, and adjusted for AdvCiv-SAS:

```text
This is based on BBAI, so default BtS may be slightly different:

- UNITAI_ATTACK: General purpose; unit prioritizes joining an attack stack, but also may wander off on search and destroy/explore, sit in a city and defend it, etc.
UNITAI_ATTACK_CITY: Join an attack stack -> If in an attack stack, lead assault on a city once the AI decides the stack should attack a city
- UNITAI_COLLATERAL: Similar to UNITAI_ATTACK_CITY, but may also attack enemy stacks in the field
- UNITAI_PILLAGE: Causes unit to wander off by itself into enemy territory and pillage stuff
- UNITAI_RESERVE: Primary use - Floating defenders to shuffle around between threatened cities. Also is a priority UNITAI type for the AI to change if it needs units of another type (the AI will frequently change reserve units to attack units once it starts warplans, for instance)
- UNITAI_COUNTER: Join an attack stack -> if in stack may leave stack to attack adjacent tiles with enemy units
- UNITAI_CITY_COUNTER: Same as UNITAI_COUNTER, but for cities instead of stacks
- UNITAI_PARADROP: Hold in reserve and drop into enemy territory; very similar to UNITAI_PILLAGE, except units drop deep into enemy territory
- UNITAI_CITY_DEFENSE: Defend a city, never leave the city
- UNITAI_CITY_SPECIAL: Basically the same as UNITAI_CITY_DEFENSE, except the AI deprioritizes this type if a city already has a defender of this AI type, meaning that the AI in unmodded BtS, the AI only ever builds a single machine gun in it's cities
- UNITAI_EXPLORE: Wander around prioritizing terrain that hasn't been revealed until unit is killed
- UNITAI_ATTACK_AIR: Air Unit used for bombing
- UNITAI_CARRIER_AIR: Similar to UNITAI_ATTACK_AIR, but prioritizes filling up carriers
- UNITAI_ATTACK_CITY_LEMMING: beeline enemy cities and Suicide against enemy units
- UNITAI_ICBM: Hold unit, and then launch when at war
- UNITAI_SPY: Basic spy AI (prioritize going into enemy territory and running spy missions - In base BtS this is purely random, with the spys basically wandering around and rolling dice, in BBAI the spys prioritize high value missions and improvements)
- UNITAI_WORKER_SEA: Build sea improvement, if none is available explore
- UNITAI_ATTACK_SEA: Basic sea AI, wander around the oceans in search and destroy mode, join a stack, etc
- UNITAI_RESERVE_SEA: Similar to UNITAI_RESERVE except for sea units
- UNITAI_ESCORT_SEA: Join a stack
- UNITAI_EXPLORE_SEA: Wander around the oceans prioritizing moving into unexplored territory until unit dies
- UNITAI_ASSAULT_SEA: Transport land units for sea assault, launch when a sufficient assault stack is ready; forms core of water domain stacks
- UNITAI_SETTLER_SEA: Ferry settlers and workers over water tiles
- UNITAI_MISSIONARY_SEA: Ferry missionaries over water tiles
- UNITAI_SPY_SEA: Ferry spys over water tiles
- UNITAI_CARRIER_SEA: Be a mobile air base for UNITAI_CARRIER_AIR units
- UNITAI_MISSILE_CARRIER_SEA: similar to UNITAI_CARRIER_SEA but for missiles
- UNITAI_PIRATE_SEA: Wander around pointlessly, sometimes run a blockade if the die roll tells you to and you are in enemy territory

Great people AIs:
- UNITAI_GREAT_PROPHET: Prioritize Building a Shrine and prioritize the religion of which there are the most cities of if multiple shrines can be built; if no shrine can be built, roll dice to decide whether to start a golden age or settle in a high value city (probably the capital)
- UNITAI_GREAT_ARTIST: Not sure what the behavior of this type is actually
- UNITAI_GREAT_SCIENTIST: Build an academy in a high priority city (most likely capital); if no good cities are available for this mission, roll dice to decide whether to settle in a high priority city or start a golden age
- UNITAI_GREAT_GENERAL: Roll dice to decide whether to join a unit or not, if not (most likely scenario), Build a military academy in a high priority city; if no acceptable city is available to build an MA in, join a high priority city (almost always the capital)
- UNITAI_GREAT_MERCHANT: Roll dice to decide if unit should Find best city to conduct trade mission in and do so, or start a golden age, or join a high priority city
- UNITAI_GREAT_ENGINEER: Build an available wonder. If no wonder is available to rush, roll dice to decide whether to start a golden age, or join high priority city

For unit AIs that are correct in the OP, I didn't write anything.
```

## Mapscripts

### Locations

Adjust to your mod path:

- AdvCiv-SAS: `C:\Program Files (x86)\Steam\steamapps\common\Sid Meier's Civilization IV Beyond the Sword\Beyond the Sword\Mods\AdvCiv-SAS\PrivateMaps\`
- BTS: `C:\Program Files (x86)\Steam\steamapps\common\Sid Meier's Civilization IV Beyond the Sword\Beyond the Sword\PublicMaps\`
- Warlords: `C:\Program Files (x86)\Steam\steamapps\common\Sid Meier's Civilization IV Beyond the Sword\Warlords\PublicMaps\`
- Base Civ 4: `C:\Program Files (x86)\Steam\steamapps\common\Sid Meier's Civilization IV Beyond the Sword\PublicMaps\`

### Map types

Done with the help of GPT-5.2 Thinking thanks:

| Map script name         | Description                                                                                                                                                                                       | Map Type                                                                           |
| ----------------------- | ------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- | ---------------------------------------------------------------------------------- |
| `aDebugMap`             | Approximative: Debug/testing script; not meant for normal balanced play (layout can be artificial); `// <!-- custom: empirically and based on code comments looks landHeavy -->`                  | Other (debug/test map); `<!-- custom: Land, see description -->`                   |
| `Arboria`               | Forest-heavy/“lush" world variant (terrain emphasis more than geometry).                                                                                                                          | Land                                                                               |
| `Archipelago`           | Many small islands; heavy naval emphasis early.                                                                                                                                                   | Water                                                                              |
| `Balanced`              | Land/sea like standard scripts but biased toward fairer starts.                                                                                                                                   | Land                                                                               |
| `Big_and_Small`         | Mix of a few big landmasses plus many smaller islands; both land and naval matter.                                                                                                                | Other (hybrid land + islands)                                                      |
| `Boreal`                | Cold/taiga-biome emphasis (forests/tundra feel).                                                                                                                                                  | Land                                                                               |
| `Continents`            | Several large continents separated by ocean; naval matters for contact, but landmasses are large.                                                                                                 | Land                                                                               |
| `Custom_Continents`     | Continents-style with extra customization knobs; still “big continent" gameplay.                                                                                                                  | Land                                                                               |
| `Donut`                 | Donut-shaped land ring with central water (distinct navigation); `<!-- custom: empirically looks landHeavy -->`                                                                                   | Other (donut/ring geometry); `<!-- custom: Land, see description -->`              |
| `Earth2`                | Approximative: Earth-like distribution feel (not necessarily a fixed real Earth map).                                                                                                             | Land                                                                               |
| `Fantasy_Realm`         | Approximative: “Fantasy/exotic" generation (more extreme/unusual geography than standard scripts); `<!-- custom: empirically looks landHeavy -->`                                                 | Other (script-specific exotic geometry); `<!-- custom: Land, see description -->`  |
| `Fractal`               | Approximative: Standard random world generator; usually produces a few sizable landmasses with varied coastlines.                                                                                 | Land                                                                               |
| `Global_Highlands`      | Highlands-like ruggedness applied broadly (more hills/peaks overall).                                                                                                                             | Land                                                                               |
| `Great_Plains`          | Very land-dominant, open terrain focus (fewer big natural barriers).                                                                                                                              | Land                                                                               |
| `Hemispheres`           | Two (or few) major landmasses separated by ocean; tends to split starts across them.                                                                                                              | Land                                                                               |
| `Highlands`             | Rugged, hill/mountain-heavy world with more chokepoints.                                                                                                                                          | Land                                                                               |
| `Hub`                   | Structured hub-and-spokes style connectivity (nonstandard geometry).                                                                                                                              | Other (structured hub geometry)                                                    |
| `Ice_Age`               | Cold-biome leaning world (more tundra/snow feel than default).                                                                                                                                    | Land                                                                               |
| `Inland_Sea`            | Central inland sea with surrounding land; coastal play is central but not an island world; `<!-- custom: empirically looks landHeavy -->`                                                         | Other (central inland sea layout); `<!-- custom: Land, see description -->`        |
| `Islands`               | Island-heavy map (often fewer/larger islands than Archipelago, depending on script).                                                                                                              | Water                                                                              |
| `Lakes`                 | Land-heavy with many internal lakes and inland coastlines.                                                                                                                                        | Land                                                                               |
| `Maze`                  | Labyrinth corridors and chokepoints (movement constrained by channels); `<!-- custom: empirically looks landHeavy -->`                                                                            | Other (maze/chokepoint geometry); `<!-- custom: Land, see description -->`         |
| `Medium_and_Small`      | Mix of medium landmasses plus smaller islands; more hybrid than pure continents.                                                                                                                  | Other (hybrid land + islands)                                                      |
| `Mixed_Continents`      | Hybrid continents + smaller land/island pieces; not purely “big continents" nor purely islands. `<!-- custom: empirically looks navalHeavy -->`                                                   | Other (hybrid land + islands) `<!-- custom: Land, see description -->`             |
| `Mirror`                | Symmetrical/mirrored layout for fairness (often MP-friendly).                                                                                                                                     | Other (symmetry-based layout)                                                      |
| `not_too_Big_or_Small`  | Hybrid landmass sizing tuned to avoid extremes (not one supercontinent, not tiny-islands-only).                                                                                                   | Other (hybrid land + islands)                                                      |
| `Oasis`                 | Desert-heavy with fertile pockets; expansion routes revolve around green clusters.                                                                                                                | Land                                                                               |
| `Pangaea`               | One massive primary continent (most civs share early land contact).                                                                                                                               | Land                                                                               |
| `PerfectMongoose`       | Approximative: Third-party “planet" style generator; typically aims for more natural-looking geography (still mostly standard land/sea gameplay). `<!-- custom: empirically looks navalHeavy -->` | Land `<!-- custom: empirically looks Water -->`                                    |
| `Rainforest`            | Jungle-heavy/wet-biome emphasis (terrain/feature density changes).                                                                                                                                | Land                                                                               |
| `RandomScriptMap`       | Meta-script that selects one of the installed scripts at random (often from a curated set).                                                                                                       | Random                                                                             |
| `Ring`                  | Ring-shaped primary land distribution (distinct wraparound feel); `<!-- custom: empirically looks landHeavy -->`                                                                                  | Other (ring geometry); `<!-- custom: Land, see description -->`                    |
| `SAS_Simple_Flat_Grass` | SAS simple flat-grass sandbox map: almost all land with mostly flat grass tiles.                                                                                                                  | Land                                                                               |
| `Shuffle`               | Randomly selects among multiple generator styles each game.                                                                                                                                       | Random                                                                             |
| `Tectonics`             | Plate-tectonics style continents with strong mountain chains and structured geography.                                                                                                            | Land                                                                               |
| `Team_Battleground`     | Team-oriented, typically symmetric starts/front lines (purpose-built for teams); `<!-- custom: empirically looks landHeavy -->`                                                                   | Other (team/symmetry layout); `<!-- custom: Land, see description -->`             |
| `Terra`                 | Old World starts with a separate “New World" to discover/colonize later; `<!-- custom: empirically and based on map descriptions looks landHeavy -->`                                             | Other (old world/new world colonization); `<!-- custom: Land, see description -->` |
| `Tilted_Axis`           | Standard landmasses, but climate bands are “tilted" vs normal north-south; `<!-- custom: empirically looks navalHeavy -->`                                                                        | Land; `<!-- custom: empirically looks Water -->`                                   |
| `Wheel`                 | Wheel/spokes-style structure (highly nonstandard geometry); `<!-- custom: empirically looks landHeavy -->`                                                                                        | Other (wheel/spokes geometry); `<!-- custom: Land, see description -->`            |

Sources (GPT-5.2 Thinking):

- [1: The Complete Guide to Map Creation (image heavy) - CivFanatics](https://civfanatics.com/civ4/strategy/references/the-complete-guide-to-map-creation-image-heavy/)
- [2: scribd.com/document/597223574/AdvCiv-manual](https://www.scribd.com/document/597223574/AdvCiv-manual)
- [3: map-script-which-one-do-you-usually-play.471862](https://forums.civfanatics.com/threads/map-script-which-one-do-you-usually-play.471862/)
- [4: https://forums.civfanatics.com/threads/odd-scenarios.363992/](https://forums.civfanatics.com/threads/odd-scenarios.363992/)

### old info

[KI#42](/_1_AdvCiv-SAS/Docs/README_Known_Issues.md#ki-42).

## Where to find Python errors or debug (so no need to copy them either if you need to ask chatgpt or such or a forum)

Go and look in (adjust to your mod path):

- C:\Users\PC\Documents\My Games\beyond the sword\Logs\PythonDbg.log
- C:\Users\PC\Documents\My Games\beyond the sword\Logs\PythonErr.log
- C:\Users\PC\Documents\My Games\beyond the sword\Logs\PythonErr2.log

You'll have errors in err log (the same errors you see ingame, but no need to manually rewrite them, just copy paste these) such as for example:

```log
Traceback (most recent call last):

  File "CvScreensInterface", line 487, in pediaJumpToImprovement

  File "SevoPediaMain", line 327, in pediaJump

  File "SevoPediaImprovement", line 103, in interfaceScreen

  File "SevoPediaImprovement", line 300, in placeTerrainMakesValid

AttributeError: 'CvImprovementInfo' object has no attribute 'isTerrainMakesValid'
ERR: Python function pediaJumpToImprovement failed, module CvScreensInterface
```

Which would help you solve them, in my case by doing a global search, finding the correct makesvalid method or such (much easier with the [civ4bug_pythonAPI_AllClasses_html.txt](/_0_Common_Docs/CIV4BUG_Sourceforge_net_All_Classes_Doc/civ4bug_pythonAPI_AllClasses_html.txt) file we added in advciv-sas mod doc files (that i found on the internet i mean i didn't write it myself but it helps a lot, see below how to use vs code global search for example for details)

## Python 2.4 compatibility for modding

Note: Players do not need to do anything regarding this section. This is information for modders only.

As of now, it seems that Civ4 runtime Python must remain compatible with Python 2.4.

For modders, it is therefore recommended to install a local interpreter, so syntax errors can be caught before pushing or starting Civ4; modern Python and Ruff do not replace this check.

As of now, for example, the [official Python 2.4.4 release page](https://www.python.org/downloads/release/python-244/) provides the Windows x86 `python-2.4.4.msi` installer. Choose this for ordinary Intel/AMD Windows PCs, including 64-bit Windows; the `ia64` installer is for the different Itanium processor architecture, not ordinary 64-bit PCs.

Keep it separate from modern Python and preserve your existing file associations. This old interpreter is for local Civ4 compatibility checks, not general-purpose modern tooling.

After installing Python 2.4, open PowerShell in your AdvCiv-SAS folder and run this command. If you installed Python somewhere other than `C:\Python24`, change that part of the command:

```powershell
& 'C:\Python24\python.exe' .github/workflows/build/python/python24_compile.py --repo-root .
```

This checks every `.py` file in this mod's `Assets/Python` and `PrivateMaps` folders using the real Python 2.4 interpreter. A PASS means those files use syntax Python 2.4 accepts. It does not build the DLL or start Civ4, and it leaves the source files unchanged; temporary compiled files are cleaned up afterward.

On 2026-10-15, all 240 Python files in those two folders passed this local check. We also confirmed that it rejects a deliberately broken temporary copy: `",".join(missing) if missing else "-"` works in modern Python but causes `SyntaxError` in Python 2.4. Replacing that expression with an ordinary `if/else` fixed it.

Use this check after editing the mod's Python files, then test the affected feature in Civ4: valid syntax alone does not prove the code behaves correctly or uses functions available in the game.

GitHub also runs the check automatically for relevant pushes and pull-request updates through our [Python 2.4 compile workflow](/.github/workflows/python24-compile.yml), using Python 2.4.6 from [our Docker image project](https://github.com/wonderingabout/python-2.4-docker).

## How to check with ruff installed locally a folder for py linter errors candidates to fix with text report

Additionally to ruff VS Code extension "Problems" section, running ruff checks locally helps identify more broadly errors without manually checking files.

Done with the help/information of GPT-5.5-Thinking thanks.

Note: Treat Ruff output as candidate cleanup only: Civ4 uses Python 2.4, and some imports, indentation, or compatibility patterns can look wrong to modern Python tooling.

Example:

In powershell on windows:

```powershell
py -m pip install ruff
py -m ruff --version
cd "C:\Program Files (x86)\Steam\steamapps\common\Sid Meier's Civilization IV Beyond the Sword\Beyond the Sword\Mods\AdvCiv-SAS"
py -m ruff check PrivateMaps/ --output-format=grouped > ruff_privatemaps_all_grouped.txt
```

Example of how i used it:

```powershell
Windows PowerShell
Copyright (C) Microsoft Corporation. Tous droits réservés.

Testez le nouveau système multiplateforme PowerShell https://aka.ms/pscore6

PS C:\Users\PC> py -m pip install ruff
Collecting ruff
  Downloading ruff-0.15.15-py3-none-win_amd64.whl.metadata (27 kB)
Downloading ruff-0.15.15-py3-none-win_amd64.whl (11.9 MB)
   ---------------------------------------- 11.9/11.9 MB 77.1 MB/s eta 0:00:00
Installing collected packages: ruff
Successfully installed ruff-0.15.15

[notice] A new release of pip is available: 25.0.1 -> 26.1.1
[notice] To update, run: C:\Users\PC\AppData\Local\Programs\Python\Python313\python.exe -m pip install --upgrade pip
PS C:\Users\PC> py -m ruff --version
ruff 0.15.15
PS C:\Users\PC> cd "C:\Program Files (x86)\Steam\steamapps\common\Sid Meier's Civilization IV Beyond the Sword\Beyond the Sword\Mods\AdvCiv-SAS"
PS C:\Program Files (x86)\Steam\steamapps\common\Sid Meier's Civilization IV Beyond the Sword\Beyond the Sword\Mods\AdvCiv-SAS> py -m ruff check PrivateMaps/ --output-format=grouped > ruff_privatemaps_all_grouped.txt
warning: The top-level linter settings are deprecated in favour of their counterparts in the `lint` section. Please update the following options in `ruff.toml`:
  - 'ignore' -> 'lint.ignore'
  - 'select' -> 'lint.select'
PS C:\Program Files (x86)\Steam\steamapps\common\Sid Meier's Civilization IV Beyond the Sword\Beyond the Sword\Mods\AdvCiv-SAS> py -m ruff check PrivateMaps/ --select E701,E702,E703,E713 --output-format=grouped > ruff_privatemaps_easy_grouped.txt
warning: The top-level linter settings are deprecated in favour of their counterparts in the `lint` section. Please update the following options in `ruff.toml`:
  - 'ignore' -> 'lint.ignore'
  - 'select' -> 'lint.select'
PS C:\Program Files (x86)\Steam\steamapps\common\Sid Meier's Civilization IV Beyond the Sword\Beyond the Sword\Mods\AdvCiv-SAS> py -m ruff check PrivateMaps/ --select E101 --output-format=grouped > ruff_privatemaps_indent_grouped.txt
warning: The top-level linter settings are deprecated in favour of their counterparts in the `lint` section. Please update the following options in `ruff.toml`:
  - 'ignore' -> 'lint.ignore'
  - 'select' -> 'lint.select'
PS C:\Program Files (x86)\Steam\steamapps\common\Sid Meier's Civilization IV Beyond the Sword\Beyond the Sword\Mods\AdvCiv-SAS>
```

Then gave the .txt reports to GPT-5.5-Thinking, which could find likel next fixes to try to address, for example it said:

From the full report, the broader categories are:

```md
E101: 3800  -> skip / separate indentation archaeology
F841: 130   -> maybe later, manual only
F401: 115   -> maybe later, but imports in Civ4 scripts can be weird
E731: 60    -> Mirror.py only, skip for now
E712: 40    -> maybe later, because true/false aliases and truthiness can matter
E701: 8     -> good, selected files first
E713: 2     -> best immediate target
E741: 2     -> manual only, Planet_Generator local rename
E402: 2     -> skip unless we inspect context
```

Some errors may be tricky or solving them may break the code, so check the output with LLM or such.

For example, to solve E101, an easy and clean way we used is to collapse those lines rather than fix their indentation (since it is helpful to code for our needs anyway (see [collapse_multiline_calls.py](/LLM_Helpers/collapse_multiline_calls.py))).

Example:

```powershell
Windows PowerShell
Copyright (C) Microsoft Corporation. Tous droits réservés.

Testez le nouveau système multiplateforme PowerShell https://aka.ms/pscore6

PS C:\Users\PC> cd "C:\Program Files (x86)\Steam\steamapps\common\Sid Meier's Civilization IV Beyond the Sword\Beyond the Sword\Mods\AdvCiv-SAS"; New-Item -ItemType Directory -Force "LLM_Helpers\outputs" | Out-Null; $ts = Get-Date -Format "yyyyMMdd_HHmmss"; py -m ruff check PrivateMaps/ --output-format=grouped | Out-File -Encoding utf8 "LLM_Helpers\outputs\ruff_privatemaps_all_grouped_$ts.txt"; py -m ruff check PrivateMaps/ --select E701,E702,E703,E713 --output-format=grouped | Out-File -Encoding utf8 "LLM_Helpers\outputs\ruff_privatemaps_easy_grouped_$ts.txt"; py -m ruff check PrivateMaps/ --select E101 --output-format=grouped | Out-File -Encoding utf8 "LLM_Helpers\outputs\ruff_privatemaps_indent_grouped_$ts.txt"; Write-Host "wrote Ruff reports with timestamp $ts"
warning: The top-level linter settings are deprecated in favour of their counterparts in the `lint` section. Please update the following options in `ruff.toml`:
  - 'ignore' -> 'lint.ignore'
  - 'select' -> 'lint.select'
warning: The top-level linter settings are deprecated in favour of their counterparts in the `lint` section. Please update the following options in `ruff.toml`:
  - 'ignore' -> 'lint.ignore'
  - 'select' -> 'lint.select'
warning: The top-level linter settings are deprecated in favour of their counterparts in the `lint` section. Please update the following options in `ruff.toml`:
  - 'ignore' -> 'lint.ignore'
  - 'select' -> 'lint.select'
wrote Ruff reports with timestamp 20260530_090009
PS C:\Program Files (x86)\Steam\steamapps\common\Sid Meier's Civilization IV Beyond the Sword\Beyond the Sword\Mods\AdvCiv-SAS>
```

| Category                         | Before collapse pass | After collapse pass | Change | Takeaway                                                                                                          |
| -------------------------------- | -------------------: | ------------------: | -----: | ----------------------------------------------------------------------------------------------------------------- |
| `E101` mixed indentation         |                ~3800 |                3467 |  ~-333 | Good reduction, but most remaining cases are true indentation/table archaeology.                                  |
| `invalid-syntax`                 |                   89 |                  89 |      0 | Unchanged; mostly parser fallout from old/messy indentation.                                                      |
| `F841` unused local variable     |                  130 |                 130 |      0 | Not affected by multiline collapse. Manual only later.                                                            |
| `F401` unused import             |                  115 |                 115 |      0 | Not affected; risky in Civ4 scripts because imports can be engine/side-effect-ish.                                |
| `E731` lambda assignment         |                   60 |                  60 |      0 | Still `Mirror.py`-style cleanup; skip for now.                                                                    |
| `E712` true/false comparison     |                   40 |                  40 |      0 | Later/manual due to truthiness and `true`/`false` alias weirdness.                                                |
| `E701` one-line colon statements |                    8 |                   8 |      0 | Still good targeted cleanup.                                                                                      |
| `E713` `not in` style            |                    2 |                   2 |      0 | Still only two exact candidates.                                                                                  |
| `E741` ambiguous variable name   |                    2 |                   2 |      0 | Manual only.                                                                                                      |
| `E402` import position           |                    2 |                   2 |      0 | Skip unless inspecting context.                                                                                   |
| **Total Ruff findings**          |                ~4248 |                3915 |  ~-333 | The helper did exactly its narrow job: reduce continuation-style indentation noise without touching logic/tables. |

Or to catch them all, it seems this can be done on Git Bash for example (and according to GPT-5.5-Thinking):

```bash
cd "/c/Program Files (x86)/Steam/steamapps/common/Sid Meier's Civilization IV Beyond the Sword/Beyond the Sword/Mods/AdvCiv-SAS" && mkdir -p LLM_Helpers/outputs && ts=$(date +%Y%m%d_%H%M%S) && out="LLM_Helpers/outputs/ruff_all_py_${ts}.txt" && { py -m ruff check . --output-format=grouped > "$out" 2>&1 || true; } && echo "wrote $out"
```

## How to enable .dmp files so for some crashes that don't immediately exit you get a "splash screen" (whatever it is called) and can dmp and see turn at crash

In CivilizationIV.ini (for example as for me in `C:\Users\PC\Documents\My Games\beyond the sword\CivilizationIV.ini` path, update to your own windows user account path), there is an option that you can set to `generatecrashdumps = 1`.

It is useful in issues such as for example [57 - (Not reproducible) Unknown game crash at turn 94](/_1_AdvCiv-SAS/Docs/README_Known_Issues.md#ki-57) (with a screenshot), or in previous related crashes in known issues as of now 56 and 55 too with such screenshots.

I don't know too much about these nor can really read these .dmp files, but Visual 2010 C++ Express seemingly can, and so can WinDbg although that may be bit trickier/harder (but check if accurate again as i don't know too much about this).

The exception/error info as well as possibly some other things i don't know about could help pinpoint the crash possible type of cause or try to solve it or such.

I don't know too much about these but i thought i'd compile (no pun) this info quick if helps.

To read the output .dmp files (i do normal dump most times to have the vague info of why it crashed xd and don't know too much about these), see an example in this known issue's documentation (although we reverted it eventually, it documents an example of showing/reading .dmp files very grossly as in broadly i.e. just a glance/general view as i don't know too much about these but it gives some indication hopefully helpful) here for example [60 - (Fixed) Seemingly rare reproducible crash at turn 283 in CvUnitAI::AI_nextCityToImprove, by adding a bunch of seemingly conservative safeties (as chatgpt 5 did, check if accurate)](/_1_AdvCiv-SAS/Docs/README_Known_Issues.md#ki-60).

## Another example of how to use VS Code global search (also shows an example of how to also browse the CIV4 BUG_Doc (copy included in our mod))

I call it "global search" but i don't know how it is exactly called, it's the feature that allows you in VS Code to search in your entire opened folder (all files) rather than just the current file (not ctrl+f, but instead the "zoom object looking icon" on left panel as of now in vs code)

result is for example this (you can right click -> "copy" too so no need to rewrite them)

```txt
C:\Program Files (x86)\Steam\steamapps\common\Sid Meier's Civilization IV Beyond the Sword\Beyond the Sword\Mods\AdvCiv-SAS\_0_Common_Docs\CIV4BUG_Sourceforge_net_All_Classes_Doc\civ4bug_pythonAPI_AllClasses_html.txt
  4173,13: BOOL isBonusMakesValid ()
  4232,16: BOOL getFeatureMakesValid (INT i)
  4274,16: BOOL getTerrainMakesValid (INT i)
  4292,18: BOOL isFreshWaterMakesValid ()
  4298,13: BOOL isHillsMakesValid ()
  4301,24: BOOL isImprovementBonusMakesValid (INT i)
  4328,17: BOOL isRiverSideMakesValid ()
```

Then feed it to chatgpt with the makes valid entire method (a dozen or 2 dozen lines hehe as of now, which i only display for concision these fewer lines here) so it can help me fix it thanks chatgpt:

```python
		for iTerrain in xrange(gc.getNumTerrainInfos()):
			if gc.getImprovementInfo(self.iImprovement).isTerrainMakesValid(iTerrain):
```

that chatgpt fixed as (slightly adjusted its code by merging 2 lines (it created a bValid boolean we otherwise don't use)), after a bit of back and forth unusually, as usually it would fix it at first try when giving it BUG_Doc sample, but it thought our code was right despite the error. Still, (or i could ask other ais like gemini or claude or deepseek maybe too or and such but ):

```python
		for iTerrain in xrange(gc.getNumTerrainInfos()):
			if gc.getImprovementInfo(self.iImprovement).getTerrainMakesValid(iTerrain):
```

Which helped fix our issue successfully now if i may say thankfully:)

note: many many many other things global search can be useful for, for example this includes how i made suepr fast as of now all the readme md files's menus with chatgpt 5's help, bug hunting, and many other uses hehe.

## Example of how to fast replace many redundant entries (e.g. 400+ strategy entries to obsolete) using regexp (with chatgpt 5's help) and vs code

This is a very nice idea i got since i knew thanks to those who taught me about regexp, but i was doing this task of replacing the old strategy entries now not used in our mod for most if not all as tedious and outdated for most if not all, but there are 400+ entries (found using global search of `<Strategy>`)!

And it's always the same task, i started to do it manually for the first few dozen entries but it's extremely tiring xd.

Entries are like this

```xml
<Strategy>TXT_KEY_BUILDING_JAIL_STRATEGY</Strategy>
```

to all convert to the same xml field to obsolete them:

```xml
<Strategy>TXT_KEY_DUMMY_EMPTY_CUSTOM_TXTKEY_STRATEGY</Strategy>
```

So then i got the nice idea to ask chatgpt 5 since i don't know too much about regexp and tedious to do so, and it provided me this search regexp code if i'm not mistaken:

```xml
(<Strategy>)(?!\s*TXT_KEY_DUMMY_EMPTY_CUSTOM_TXTKEY_STRATEGY\s*)([\s\S]*?)(</Strategy>)
```

Which works perfectly to selectively search strategy xml fields that are not txt key dummy yet, so now i can blindly find and replace file per file for those i want to, saves me a lot of time and headache, i so love ai xd it will really save humanity at least me xd maybe and thanks a lot chatgpt 5, hopefully helpful.

## Which XML version file to choose to import in your mod when there are multiple ones (for example one in base Civ4 (vanilla is it called?) folder and one in BTS folder)

See screenshots or files about this in this [google drive folder link](https://drive.google.com/drive/folders/17FJs8tJy6g7SOsvtaRd5wVuX96mbmR_u?usp=sharing)

This is something that may seem obvious but i didn't know it until i noticed someone explaining something to me (thanks xd hehe) mention it.

For example, say you want to add or change some code related to basicinfos xml file in your mod.

If you do a global search with vs code at root of civ4 folder (as for me for example it is `C:\Program Files (x86)\Steam\steamapps\common\Sid Meier's Civilization IV Beyond the Sword\`) (note: not to be confounded with the BTS folder that is a subfolder of the former civ4 vanilla one at least in the steam version of civ4 bts it seems, and so that is in instead : `C:\Program Files (x86)\Steam\steamapps\common\Sid Meier's Civilization IV Beyond the Sword\Beyond the Sword\`), you'll find results in many files and folders related to such code samples you looked for.

Or easier, if you use a tool like wiztree to search for said files (e.g. basicinfos xml file in this example) in root of civ4 folder (which as for me is the vanilla steam civ4 folder mentioned before), you see something as in the screenshot, so many basicinfos xml files:

- one in `C:\Program Files (x86)\Steam\steamapps\common\Sid Meier's Civilization IV Beyond the Sword\Assets\XML\BasicInfos\CIV4BasicInfos.xml` (civ 4 vanilla version of it) (+/- 800 lines)
- and one in `C:\Program Files (x86)\Steam\steamapps\common\Sid Meier's Civilization IV Beyond the Sword\Warlords\Assets\XML\BasicInfos\CIV4BasicInfos.xml` (civ 4 warlords version of it i assume) (+/- 843 lines if i'm not mistaken)
- one in `C:\Program Files (x86)\Steam\steamapps\common\Sid Meier's Civilization IV Beyond the Sword\Beyond the Sword\Assets\XML\BasicInfos\CIV4BasicInfos.xml` (civ 4 bts version of it i assume) (+/- 260 lines)

I don't know too much about these, but it seems to me that these are different versions of same file, and only most recent one relevant to your mod is applied (i.e. if you play vanilla the vanilla one applies i guess, but if you play bts the bts one applies rather i guess) but this is just a guess and i could or not be mistaken, check if accurate (for example ask chatgpt 5 or whichever recent or relevant version of it or some other ai or ask in some forum or such alternatively).

Sometimes the differences are small, sometimes the differences are huge as here, so which version to pick? Generally if you play bts i'd say pick latest, but again i don't know for sure, so i compiled this info since it would have been nice to know when i started modding, hopefully helpful and helps you if you want to start your mod.

In this case i guess they changed the way old code worked or split it among other files or did some massive change that would explain the difference, but check if accurate as these are also just guesses of mine that may be accurate but i don't know for sure.

When importing assets or code from another mod, check that project's applicable rights and reuse terms rather than assuming permission. For AdvCiv-SAS content, the canonical and controlling project policy is [License and reuse](/LICENSE.md#license-and-reuse); this modding guide does not grant additional permission.

Hopefully helpful.

## Civ4 ProcessInfos XML info

From: [modiki's website, wayback archive version latest i found](https://web.archive.org/web/20230330010924/https://modiki.civfanatics.com/index.php?title=Civ4ProcessInfo) since was/is down the website as of/for now

adjusted for advciv-sas or not:

```text
Civ4ProcessInfo

The Civ4ProcessInfo file defines what commerce types can be created from production in a city (by default, 100% of production to wealth, research, and culture). This can also be used to create an option to have a city build nothing (by setting all of the commerce types so that no production is applied to them).

All tags must be opened and closed; the first is the "open", the second the "close" tag. If nothing goes inside a "list tag", then it should just be the opening tag with a "/" before the closing bracket. The following tables contain all available tags, as well as their purpose and accepted values.




Contents
1	Tags
1.1	Headers
1.2	Text
1.3	Lists (Multi-line)
1.4	Art
2	Example

Tags

Headers

These tags typically bracket other tags, sometimes the entire file, and are generally used to specify more than one piece of data.

Tag Name						Description
ProcessInfos					This tag surrounds all entries in the file
ProcessInfo						Main bracket for each entry


Text

Tag Name						Description
Type							How the process is referred to in other files
Description						Text for the process in the game
Strategy						Strategy text for the process in the game
TechPrereq						Tech required to build the process in the city


Lists (Multi-line)

All List tags consist of an opening/closing tag, which is shown here, and then each entry within it is another tag with the same name as the parent tag, minus the "s" (i.e. singular, rather than plural).

Tag Name						Description
ProductionToCommerceModifiers	The percent of the city's production applied to each commerce type; top is gold, then research, then culture, then espionage <iProductionToCommerceModifier>100</iProductionToCommerceModifier>



Art

These tags are directly related to the rendering of art for the entry.

Tag Name						Description
Button							The button for the process in the city screen and city production popup


Example

In the following example of code, please note that there is a specific order of all of the tags. You must list the tags in this order for the game to properly interpret your file.

<ProcessInfo>
 <Type>PROCESS_WEALTH</Type>
 <Description>TXT_KEY_PROCESS_WEALTH</Description>
 <Strategy>TXT_KEY_PROCESS_WEALTH_STRATEGY</Strategy>
 <TechPrereq>TECH_CURRENCY</TechPrereq>
 <ProductionToCommerceModifiers>
  <iProductionToCommerceModifier>100</iProductionToCommerceModifier>
  <iProductionToCommerceModifier>0</iProductionToCommerceModifier>
  <iProductionToCommerceModifier>0</iProductionToCommerceModifier>
 </ProductionToCommerceModifiers>
 <Button>Art/Interface/Buttons/Process/ProcessWealth.dds</Button>
</ProcessInfo>
```

## ICONS code XML names (with some color changing code as bonus or to be extra exhaustive a bit more than needed but is here if needed maybe)

from: `C:\Program Files (x86)\Steam\steamapps\common\Sid Meier's Civilization IV Beyond the Sword\Beyond the Sword\Mods\AdvCiv-SAS\CvGameCoreDLL\CvDllTranslator.cpp`

As follows:

```cpp
#include "CvGameCoreDLL.h"
#include "CvDllTranslator.h"
#include "CvGame.h"
#include "CvPlayer.h"

void CvDllTranslator::initializeTags(CvWString& szTagStartIcon, CvWString& szTagStartOur, CvWString& szTagStartCT, CvWString& szTagStartColor, CvWString& szTagStartLink, CvWString& szTagEndLink, CvWString& szEndLinkReplacement, std::map<std::wstring, CvWString>& aIconMap, std::map<std::wstring, CvWString>& aColorMap)
{
	szTagStartIcon = L"[ICON_";
	szTagStartOur = L"[OUR_";
	szTagStartCT = L"[CT_";
	szTagStartColor = L"[COLOR_";
	szTagStartLink = L"[LINK";
	szTagEndLink = L"[\\LINK";
	szEndLinkReplacement = L"</link>";

	//create icons map
	aIconMap[L"[ICON_BULLET]"] = std::wstring(1, (wchar)gDLL->getSymbolID(BULLET_CHAR));
	aIconMap[L"[ICON_HAPPY]"] = std::wstring(1, (wchar)gDLL->getSymbolID(HAPPY_CHAR));
	aIconMap[L"[ICON_UNHAPPY]"] = std::wstring(1, (wchar)gDLL->getSymbolID(UNHAPPY_CHAR));
	aIconMap[L"[ICON_HEALTHY]"] = std::wstring(1, (wchar)gDLL->getSymbolID(HEALTHY_CHAR));
	aIconMap[L"[ICON_UNHEALTHY]"] = std::wstring(1, (wchar)gDLL->getSymbolID(UNHEALTHY_CHAR));
	aIconMap[L"[ICON_STRENGTH]"] = std::wstring(1, (wchar)gDLL->getSymbolID(STRENGTH_CHAR));
	aIconMap[L"[ICON_MOVES]"] = std::wstring(1, (wchar)gDLL->getSymbolID(MOVES_CHAR));
	aIconMap[L"[ICON_RELIGION]"] = std::wstring(1, (wchar)gDLL->getSymbolID(RELIGION_CHAR));
	aIconMap[L"[ICON_STAR]"] = std::wstring(1, (wchar)gDLL->getSymbolID(STAR_CHAR));
	aIconMap[L"[ICON_SILVER_STAR]"] = std::wstring(1, (wchar)gDLL->getSymbolID(SILVER_STAR_CHAR));
	aIconMap[L"[ICON_TRADE]"] = std::wstring(1, (wchar)gDLL->getSymbolID(TRADE_CHAR));
	aIconMap[L"[ICON_DEFENSE]"] = std::wstring(1, (wchar)gDLL->getSymbolID(DEFENSE_CHAR));
	aIconMap[L"[ICON_GREATPEOPLE]"] = std::wstring(1, (wchar)gDLL->getSymbolID(GREAT_PEOPLE_CHAR));
	aIconMap[L"[ICON_BAD_GOLD]"] = std::wstring(1, (wchar)gDLL->getSymbolID(BAD_GOLD_CHAR));
	aIconMap[L"[ICON_BAD_FOOD]"] = std::wstring(1, (wchar)gDLL->getSymbolID(BAD_FOOD_CHAR));
	aIconMap[L"[ICON_EATENFOOD]"] = std::wstring(1, (wchar)gDLL->getSymbolID(EATEN_FOOD_CHAR));
	aIconMap[L"[ICON_GOLDENAGE]"] = std::wstring(1, (wchar)gDLL->getSymbolID(GOLDEN_AGE_CHAR));
	aIconMap[L"[ICON_ANGRYPOP]"] = std::wstring(1, (wchar)gDLL->getSymbolID(ANGRY_POP_CHAR));
	aIconMap[L"[ICON_OPENBORDERS]"] = std::wstring(1, (wchar)gDLL->getSymbolID(OPEN_BORDERS_CHAR));
	aIconMap[L"[ICON_DEFENSIVEPACT]"] = std::wstring(1, (wchar)gDLL->getSymbolID(DEFENSIVE_PACT_CHAR));
	aIconMap[L"[ICON_MAP]"] = std::wstring(1, (wchar)gDLL->getSymbolID(MAP_CHAR));
	aIconMap[L"[ICON_OCCUPATION]"] = std::wstring(1, (wchar)gDLL->getSymbolID(OCCUPATION_CHAR));
	aIconMap[L"[ICON_POWER]"] = std::wstring(1, (wchar)gDLL->getSymbolID(POWER_CHAR));

	aIconMap[L"[ICON_GOLD]"] = std::wstring(1, (wchar)GC.getInfo(COMMERCE_GOLD).getChar());
	aIconMap[L"[ICON_RESEARCH]"] = std::wstring(1, (wchar)GC.getInfo(COMMERCE_RESEARCH).getChar());
	aIconMap[L"[ICON_CULTURE]"] = std::wstring(1, (wchar)GC.getInfo(COMMERCE_CULTURE).getChar());
	aIconMap[L"[ICON_ESPIONAGE]"] = std::wstring(1, (wchar)GC.getInfo(COMMERCE_ESPIONAGE).getChar());

	aIconMap[L"[ICON_FOOD]"] = std::wstring(1, (wchar)GC.getInfo(YIELD_FOOD).getChar());
	aIconMap[L"[ICON_PRODUCTION]"] = std::wstring(1, (wchar)GC.getInfo(YIELD_PRODUCTION).getChar());
	aIconMap[L"[ICON_COMMERCE]"] = std::wstring(1, (wchar)GC.getInfo(YIELD_COMMERCE).getChar());
	// advc.064:
	aIconMap[L"[ICON_CITIZEN]"] = std::wstring(1, (wchar)gDLL->getSymbolID(CITIZEN_CHAR));
	// <advc.002f>
	aIconMap[L"[ICON_GREATGENERAL]"] = std::wstring(1, (wchar)gDLL->getSymbolID(GREAT_GENERAL_CHAR));
	aIconMap[L"[ICON_AIRPORT]"] = std::wstring(1, (wchar)gDLL->getSymbolID(AIRPORT_CHAR));
	// </advc.002f>
	//create color map
	aColorMap[L"[COLOR_REVERT]"] = CvWString(L"</color>");
	for(int i=0; i < GC.getNumColorInfos(); i++)
	{
		const NiColorA& color = GC.getInfo((ColorTypes) i).getColor();
		CvWString colorType(GC.getInfo((ColorTypes) i).getType());
		CvWString wideColorType;
		wideColorType.Format(L"[%s]", colorType.GetCString());
		CvWString colorOut;
		colorOut.Format(L"<color=%i,%i,%i,%i>", (int) (color.r * 255), (int) (color.g * 255), (int) (color.b * 255), (int) (color.a * 255));
		aColorMap[wideColorType.GetCString()] = colorOut;
	}
}
```

## Insert an img in a text label in Sevopedia

Code sample provided by Claude AI in one of my prompts to add YieldChanges to placeStats in Sevopedia building, but the code sample surprisingly and happily, or maybe rather plesantly or both or notworks, so sharing it in case it helps others or me who rereads it or and need or want to reuse it some day, or not, or etc.

Output looks like this for example: [(google drive folder screenshots link)](https://drive.google.com/drive/folders/1ZY8gTziXBsHoCjjxYRJ4DoJwTf5VHvsy?usp=sharing)

note: modified button path in code below to a button path known to exist in base advciv (civ4 too?), for example the jewish temple

```python
			# <!-- custom: 2.1: Yield Modifiers (Food +x%, Production +x%, Commerce +x%) with power breakdown -->
			for k in range(YieldTypes.NUM_YIELD_TYPES):
				iYieldModifier = buildingInfo.getYieldModifier(k)
				iPowerYieldModifier = buildingInfo.getPowerYieldModifier(k)

				# Total modifier (regular + power)
				iTotalYieldModifier = iYieldModifier + iPowerYieldModifier

				if (iTotalYieldModifier != 0):
					szText1 = ""

					# Base modifier part
					if (iYieldModifier != 0):
						if (iYieldModifier > 0):
							szSign = "+"
						else:
							szSign = ""
						szText1 = szSign + str(iYieldModifier) + "%"

					# Power modifier part (optional, only if exists)
					if (iPowerYieldModifier != 0):
						if (len(szText1) > 0):
							szText1 += ", and "
						if (iPowerYieldModifier > 0):
							szPowerSign = "+"
						else:
							szPowerSign = ""
						szText1 += szPowerSign + str(iPowerYieldModifier) + "% with "

						# Add power button (using fictional path - replace with actual if found)
						szPowerButton = u"<img=Art/Interface/Buttons/Buildings/Temple_Jewish.dds size=16></img>"
						szText1 += szPowerButton

					szText2 = u"%c  %s" % (gc.getYieldInfo(k).getChar(), szText1)
					fillCell(screen, szText2, x, y)
					x, y, rowItemId = getNextItemCoordinates(x, y, rowItemId, columnWidth)
```

note 2: i have tested with a .jpg (source [Stonehenge wallpaper from https://www.wallpaperflare.com/ Current photo size: 4272 x 2848 px • Resolution:4K)](https://www.wallpaperflare.com/stonehenge-england-stone-henge-united-kingdom-grass-sky-wallpaper-ahjky/download) (drives me to quite nostalgic times, even though is in the past now) (`szPowerButton = u"<img=Art/AdvCiv_SAS/Wallpapers/Stonehenge/test.dds size=16></img>"`), also appended in the drive folder, but display doesn't work, however converting it to .dds (DXT1 with Paint.NET for example, full resolution) is now displayed accurately and successfully (.dds also appended in drive link mentioned above))

In the note 2 example, increasing size to for example 1024 also works, we get a very nice Sevopedia background image hehe, i suspect using a lower resolution base image, perhaps 1080p, or 1440p may produce even better results (less pixelated one (based on the  [past .dds experiment/experience](/_1_AdvCiv-SAS/Docs/Modding_Ressources/README.md#dds-button-size-comparison-analysis---trying-to-find-optimal-dds-button-size-for-a-164px--2px-due-to-small-mistake-icon-frame-in-sevopedia-units-unit-panes-icon-frame) where optimal size seemed to be a bit higher than max icon_size (256px on a 164px if i remember correctly, so maybe here a 1440p native image may work best?

But not sure and would need to test, and in all cases,)), perhaps using another DXT conversion too or not,)

note 3: atlas linking buttons such as `szPowerButton = u"<img=,Art/Interface/Buttons/TechTree/Physics.dds,Art/Interface/Buttons/Warlords_Atlas_1.dds,1,15 size=32></img>"` (as part of changing tech_physics 's button as part of our tech rework in advciv-sas) also seems to work quite well, we see the button successfully even though a bit excentered if that is a word in english (not centered around the text), but hopefully good enoughat least as proof of concept further, see the great person button code in place Stats of Sevopedia building as well for a cleaner or rather more centered implementation, is just to show/test how it successfully works here; also for our needs this seems to work very well (whoaaa!!!!

If i may say indeed too), see screenshot(s) in drive link above for details

note 4: we later also used this approach for emojis in our Sevopedia leader's ai personality panel category headers, with the revived chatgpt's inputs as well, reusing old emojis in our code with the help of AI/LLM helpers. See as of now screenshot 5846 in same google drive or ingame in Sevopedia leader for an example.

## Import a nif art asset from another mod (example with how we added the impluvium's nif as an import from FFH2 mod's Adventurer's guild building)

See steps in screenshots in [this google drive folder (impluvium imported as nif)](https://drive.google.com/drive/folders/1Hx-bvRy7joM54S0Vnmva6HX1h8ZZ-LAh?usp=sharing)

In this example the nif was in a .fpk, but i assume it would work similarly if files were scattered in a folder raw.

Be careful to import all nif related files in the same folder in your mod (or in advciv-sas in this example), i am not too technical nor knowledgeable about these but it seems to work/function well for this nif import, hopefully this is helpful.

Note: be careful, in some mods/cases the assets such as nif may be scattered across several different fkps, like in ri mod smokehouse1 's nif, scattered across RI_37_Structures2.FPK (iroquoislh.nif and asian_building.dds) and RI_37_Structures9.FPK (asian_building_shadow.dds) so need to gather them all in one folder if not doing an FPK approach (we don't do FPKs in AdvCiv-SAS at least as of now, see screenshots in the drive link above for details/example), in such cases, i would recommend rather to use PakBuild, see [README.md#some-useful-tools-while-doing-this](/README.md#some-useful-tools-while-doing-this) explanation and drive link at PakBuild there for details; note 2 about this: in some mods like c2c for example, unpacking in a fodler where you unpacked before seems to create some elements to be missing, 13 by unpacking all fpks in same folder one by one vs 17 files by doing it manually with dragon unpacker, adjust as you see fit the note in this paragraph

note 2: sometimes the .kfm is base civ4 for example of the of arabian old camel archer so can't find it in the ri mod, a hint to that may be if a ctrl+f of the filename for example "ArabiaCamelArcher.kfm" with one result in our art assets xml before we added it from another mod in the xml, or 2+ results after adding the .kfm in xml too (before finding the actual .kfm file in the mod, then it i very likely the .kfm is a base civ4 file and thus can't be found in any mod unless they especially copy it again in exact same path or path specified which there should be no strong reason to do ince the file already exists in base civ4), so in short if can't find the .kfm or maybe perhaps even the .nif in the mod, in any .fpk or such mod files too perhaps, then maybe the file is a base civ4 one so just leave path as it is without changing path to your mod specific paths, for example for the camel archer `Art/Units/Unique/Arabia/CamelArcher/ArabiaCamelArcher.kfm` (default keep as is) not `Art/AdvCiv_SAS/Units/Camel_Archer/nif/ArabiaCamelArcher.kfm` while desperately trying to find a file that maybe doesn't exist, but many assets have their own ;kfm though so make sure to search too, but generally they should be in path specified in one of the .fpks of the mod or raw path if they don't use fpk for this asset, hopefully helpful, if still no luck try to find the asset in another mod perhaps, hopefully i found the trick before ditching the ri mod files i had already downloaded, so all works.

Like this for example if it helps too. (not saying this is a standard or ideal to follow, just how i do it so far)

```xml
			<NIF>Art/AdvCiv_SAS/Units/Camel_Archer/nif/BerberCamelRiderFinalFinal.nif</NIF>
			<KFM>Art/Units/Unique/Arabia/CamelArcher/ArabiaCamelArcher.kfm</KFM>
			<SHADERNIF>Art/AdvCiv_SAS/Units/Camel_Archer/nif/BerberCamelRiderFinalFinal_fx.nif</SHADERNIF>
```

note3: in some cases it's even harder than that as the nif expects some assets not in the other mod where you're importing it from's folder, so with nifskope (see screenshots in drive link in this section of the readme), for example for the usa_patriot of/in advciv-sas, open the trees and such until you find one or many of the assets the nif requires, in this case it was just brownbesstexture.dds, that was not in ri mod's li_fus_early_americawhere the remaining and what i expected to be allunit's art were.

So to solve this, i did a search with dragon unpacker in the .fpk of civilization america where i took this unit from, and luckily for us this brownbesstexture.dds was there, so i extracted, added it, verified path in nif is relative to current folder as was and not in some other folder parent or child to ours, and since all was good by adding this .dds as well in Mods\AdvCiv-SAS\Assets\Art\AdvCiv_SAS\Units\USA_Patriot\nif\ then finally the gun was not purple again of missing art but was the real gun same as in ri mod yohoo!!.

Since it was not luckily too hard to solve unlike harder times i had for example with c2c camel units with many missing assets or and with a path outside of the unit's folder if i remember correctly, i thought it was a good time to add this example to the tutorial since easier and such and i could solve it, while also remembering chatgpt's instructions at the time for c2c even though i didn't succeed at it at the time now solved with the usa patriot that has a nice brown gun in the game (or white in nifskope as in empty in this case). hopefully helpful, see drive screenshots as well for examples of how i solved it in main steps hopefully helpful

note 3.2 / update: i have found that as of now for ri mod in particular, downloading the SVN version (for example with TortoiseSVN as they recommend it on their website) allows to have directly all the asset files, which is extremely convenient, no need to browse through fpks painfully and very inefficiently!!!. Used for the mongol khishigten art assets!

Thanks for assets to ri mod and for providing this version that happens to have no fpk to save my sanity and time or whatnot.

## (Not used anymore in advciv-sas) Example of how to add a static image file (such as .PNG, .JPG, etc) as a leader portrait

Old information from my notes, now we use leader portraits (nif) with an animation, but an earlier version used static images if i may say and am not mistaken in saying so. But these instructions may be helpful so adding them for reference.

Note: if you want to add leaders or other kinds of button art assets or such, below instructions aim to help at that even though they may not be 100% accurate or best practive, but may be maybe or not; however it should also be noted that it may be easier to rather import existing art assets from many mods, but if you want to import your own manual art asset, perhaps as static images .dds files, this is how i do it, not guaranteed is best way to do it, may or may not be, but it worked and hopefully helpful enough if you want to do so, otherwise i would recommend in general or all cases to maybe import directy already refined art assets from other mods, adjust as you see fit.

Leader dimensions obtained using Paint.NET manual selection (approximately but should be quite precise) on a 4K screen windowed mode.

First, AdvCiv (+/- / AdvCiv-SAS at least for now) base art dimensions:

- in Sevopedia (before my fix): 421 x 488 	(ratio: 0,8627)   ;    (reverse-ratio: 1,1591)
- ingame diplomacy: 709 x 866 				(ratio: 0,8187)   ;    (reverse-ratio: 1,1214)

Secondly, Realism invictus, used as reference:

- in Sevopedia: 510 x 620     (ratio: 0,8226)   ;    (reverse-ratio: 1,2157)
- ingame diplomacy: 708 x 866 (ratio: 0,8175)   ;    (reverse-ratio: 1,2231)

Thirdly, example of use:

Kingdom of Benin:

- image: 900 x 610 (.PNG)
- it is landscape, so convert=crop here to portrait, reduce to 610 x 610 then think (is closer to portrait), using Paint.NET, image: 610 x 610

- convert to civ4 ratio: converted image = 610 x ? (no need to reduce, 610 < 866 of AdvCiv diplomacy, even if wasn't, is maybe not necessary for higher quality? But since we don't have this problem here, just continue)

- reduce to civ4 dimension (maintain ratio):

? / 610 = 709 / 866

=>

? = 709 / 866 * 610

=>

? = 499,41

=>

? = 499

=>

image: 499 x 610

- crop to this selection size (of 499 x 610) in this example, using Paint.NET for this image (note: this respects image ratio as in advciv diplomacy ingame)

- after it is cropped to 499 x 610, now resize image using Paint.NET to ? x 512 (Let Paint.NET autofill values and choose to maintain aspect ratio). In this example Paint.NET did it to 419 x 512

- after it is resized, now resize again but to 512 x 512 (without maintaining aspect ratio), using Paint.NET

(note: I think it is the same technique or similar in Realism Invictus, as their .dds files are enlarged in display, but ingame they seem to have a fine ratio.)

(note 2: it is also Realism Invictus who gave me indirectly the idea to do this non ratio (2nd) resize, now my image as theirs do not maintain aspect ratio)

- finally, save the image as a .dds file using Paint.NET still (for example), (may use for example maybeDXT1 with mip maps (Bicubic) and keep gamma correction enabled, or DXT5 with mip maps (not tried so far) for example if you need/use transparency (i don't know a lot about this, is according to ChatGPT's info, which seems accurate about this at least ) and i don't know if gamma would be needed or best desired here too maybe, refer to ChatGPT or maybe other specialized places/sources if you want a more specific or detailed or accurate info maybe)

- ingame check result, in my case looks very nice or as nice as could i think with starting image, displays well in (ingame) Sevopedia leader view and ingame diplomacy

This is just an example, adapt as you see fit

## (Not used anymore in advciv-sas) Example of how to create a leader head .dds button from a static image file (such as .PNG, .JPG, etc) (ideally from the leader head image portrait if any, but if i may say)

Old information from my notes, as we now we use leader portraits (nif) with an animation, and while doing so also imported the corresponding .dds button that the mod where we imported the .nif animation and such from also had (this .dds button), so we don't make leader .dds buttons from our own leader static images anymore as of now at least in advciv-sas, but an earlier version used static images if i may say and am not mistaken in saying so. But these instructions may be helpful so adding them for reference.

Fourthly, then for the leaderhead button (mini icon of the leader in the Sevopedia), since now the final dimension displayed is no longer a portrait (no 256 x 512 stretched to 512 x 512 if am not mistaken, so we can directly crop select at 64 x 64 for such a button (leader icon without any resize (stretching or compression of ratio or anything that alters ratio this time)) from the native image (for example .png, any size should do fine too so ideally take the original image if you still have it, else maybe the .dds (unstretched)) should maybe do fine anyways)

So for example you can first make a 96 x 96 crop selection around his head with Paint.NET, then fine tune it (i.e. redo a crop selection on this 96 x 96 (for example .png maybe image) to 64 x 64 in a similar manner to the area you prefer to appear in the leader's final icon/button), and save it as a different file for example Ogiso_Igodo_button.dds.

Note: if at 64x64 your leader head is too big or maybe too small, you may try instead to make a bigger selection of say for example 96 x 96 or maybe even 128 x 128 (or any number inbetwwen like 105 x 105, 99 x 99, etc should maybe work too? (to try i didn't but if you want to try it)) and then resize it to 64x64, this way leader head would be smaller relatively to the 64 x 64 button.

Then regenerate the .dds in a similar manner than for the full portrait, but no restretching from say 256x512 to 512x512 since 64x64 is alreayd square so just pick same format maybe not sure.

And finally implement it (the .dds) in XML, looks very very good, at least for me i think might do further beautification such as rounded edges or other things, but more than good enough for my purpose, hope this helps.

(note: again check all this to make sure my intution or if no or such is accurate or updated, hopefully helpful)

## .dds (button) size comparison analysis - (Trying to find) optimal .dds button size for a 164px (+ 2(px?) due to small mistake) icon frame in Sevopedia unit's unit pane('s icon frame)

Trying to implement the new AI-generated images of ChatGPT (see [Authors](/README.md#authors) for details), and while doing the Sevopedia Building todo add link (some time) after the [sevopedia units rework](/_1_AdvCiv-SAS/Docs/README_Sevopedia_Reworks.md#example-7-units-category) i first tried to assess the size at which they display optimally.

Lower size may result in smaller file size, perhaps even performance increase (but not sure i don't know), but may reduce image quality if too low.

Similarly, too high image size such as 1024 seems to result in a too pixelated image.

All image and .dds files at different sizes used in the below experiment/analysis are available in this [.dds comparison and analysis Google Drive folder link](https://drive.google.com/drive/folders/1_ACul0v892_wqODLBhjYdEoSobhvGiHZ?usp=sharing)

I have taken this 1024 .png generated image for the 164px icon frame (unit pane) in Sevopediaunit. But i made a small mistake, which hopefully doesn't alter too much the interpretation of the results, is that i added a + 2 to icon size, because i thought this is how borders (the blue edges are made), but now i understand they are part of the image indeed and not generated by civ4 during rendering, even though i saw such edges before, i didn't know that these are those.

So i have converted it to various sizes with Paint.NET from .png to .dds, always exiting the program (Paint.NET) and reopening the original 1024 x 1024 generated .png of ChatGPT at each new size conversion, if size is smaller than original (1024), i would thus (to explain and be exhaustive) resize image, then save as .dds, then exit.

The lines are for example like this:

```xml
		<UnitArtInfo>
			<Type>ART_DEF_UNIT_ARCHER</Type>
			<Button>Art/AdvCiv_SAS/Units/Archer_Bow_Short_Bronze/Archer_Bow_Short_Bronze_Button_84.dds</Button>
			<fScale>0.44</fScale>
			<fInterfaceScale>1.0</fInterfaceScale>
			<bActAsLand>0</bActAsLand>
			<bActAsAir>0</bActAsAir>
			<NIF>Art/Units/Archer/Archer.nif</NIF>
			<KFM>Art/Units/Archer/Archer.kfm</KFM>
			<SHADERNIF>Art/Units/Archer/Archer_FX.nif</SHADERNIF>
			<ShadowDef>
				<ShadowNIF>Art/Units/01_UnitShadows/UnitShadow.nif</ShadowNIF>
				<ShadowAttachNode>BIP Pelvis</ShadowAttachNode>
				<fShadowScale>0.85</fShadowScale>
			</ShadowDef>
			<fBattleDistance>0.35</fBattleDistance>
			<fRangedDeathTime>0.31</fRangedDeathTime>
			<bActAsRanged>1</bActAsRanged>
			<TrainSound>AS2D_UNIT_BUILD_UNIT</TrainSound>
			<AudioRunSounds>
				<AudioRunTypeLoop/>
				<AudioRunTypeEnd/>
			</AudioRunSounds>
		</UnitArtInfo>
```

The results, minus the small and ideally not too altering + 2 to icon size i made mistake or maybe not small or yes, that i would ideally redo cleanly but a bit tedious, and hopefully interpretable if that is a word as in similar enough or maybe rather accurate enough or and other or and not, is that:

- 1024 seems too pixelated on this 164px icon unit pane's frame
- gradually as i reduce size to 512, then 256, quality seems to improve, file size is also much smaller which is nice if high enough and best looking
- then at size 164 which should be a 1:1 ratio with the pane (minus my small mistake that ohpefully didn't alter it too much), it seems that image is a bit blurrier and less nice(ly?) if that is a word looking than 256 x 256 dds one, so 256 seems better and best so far for .dds size on this
- then the lower the size, from 164, the worse it gets, more and more blurry on our 164px + 2 due to my mistake
- finally, at 64px size, so a 1:1 ratio with other buttons (or maybe i am, it may actually be 84px or something similar, seeing how the button is smaller than other civ4 ones, so added a 84px image in a similar manner than other ones (with the + 2 mistake), but it is a bit too big, maybe due to not rounded edges, or maybe it is slightly smaller):

(In all cases) my general conclusion would be that (it seems that) a ratio slightly bigger than 1 (256:(162(+2))) seems to give best results, at least for the 256px on 164px ratio image example for .dds files.

The need is to display buttons at same size than others in the end (else the "overflow" out of the button box like for the 84px button or 128px or 96px or bigger ones too, but they also do not overflow in the unit pane even if 1024 or 256 is way bigger than 512px, todo find how/why, and todo look ingame if images fit in the button box or "overflow" out of it too)

Ideally i would redo the experiment without the uneeded and inaccurate due to my misunderstanding + 2, but is tedious.

Note: ingame (map and playing an actual game), the game crashes after a few turn with a 1024px, freezes without end seemingly at 256px so exiting it after a short time of a few seconds or such, and similarly at 128px when trying to enter debug mode just like in 128px, at least (all this happens) with default game settings which i don't know a lot about at least not now or and always or and not but in all cases, (so) not using them for now (or maybe a smaller size that doesn't crash or displays accurately) if not always

Note 2: Among all those sizes, only 64px doesn't crash with default settings it seemst), they seem a bit slower to load even at 64px ingame as they sometimes do not show but there is a default archer button instead the first times, so for now until i investigate more about it/this and know better how it works, not using them

## How to change the mod's main menu background image (for example to add a static image instead of default base advciv +/- civ4 image)

Used on the middle-earth mod's civ4mainmenubg.nif thanks to have a static image instead of a nif animation, and then just replaced their RingMainMenu.dds with our AdvCiv-SAS's wallpaper dds, using the same file name.

Then updated in XML the path to where our nif is, see `MAINMENU_SCENE_CLASSICAL` (main menu background) and `MAINMENU_LOAD_CLASSICAL` (loading screen (e.g. loading a save file, loading a new map, etc.) background) in [/Assets/XML/Art/CIV4ArtDefines_Interface.xml](/Assets/XML/Art/CIV4ArtDefines_Interface.xml).

Note: added top and bottom empty bands with Paint.NET to convert it ingame to 16:9 (else image was trimmed vertically).

Note 2: it looks like a small part of the image is trimmed horizontally, i could add small side empty spaces to the image too but i guess but this is much smaller than the verticaly trimming was so left as such.

## How to remove the blue rectangle in main menu screen, change text color for example to black, etc. (in .thm files)

If you want to know how i removed the blue rectangle color, it is (thanks to chatgpt 5) done by replacing, in [/Resource/Civ4Theme_Custom.thm](/Resource/Civ4Theme_Custom.thm) the `style SF_CtrlTheme_Civ4_Control_Panel_MainStart_Style` 's image lines such as `Image			= 	GBitmap("../../../../Resource/Civ4/Custom/main_start.tga", HVGrid, 1, 2, 0, 0);` to `Image			= 	GBitmap();`

Note 2: then in `style SF_CtrlTheme_Civ4_Control_Button_MainMenuSelection_Style` in same file you can also change text color for example at lines such as (thanks to chatgpt 5's help) `.Default						=	GFC_Control_Color_Text_Default;` (seemingly white at least before the change) to `.Default						=	GColor(0,0,0,255);` (as of now black much more readable now that we don't have the old blue rectangle as a background), white was too bright so now is black, screenshots are visible in the google drive folder corresponding to known issue above, or other text beautification(s) (see code comments there for details).

## How to autoplay (let the AI play for you (super fast gameplay / testing tool)) in map (loaded save file, new game, etc.) view

This feature is extremely useful to see how changes feel ingame or how AI players handle them.

I don't know the specifics, but in most mods you need to enable chipotle cheat code in CivilizationIV.ini (applies once for all mods and seemingly base bts too but i don't know for sure so check).

Then ingame (save file load, new map, etc), press ctrl+z if your mod has this shortcut and you enter debug mode, then press ctrl+shift+x you can specify the number of turns to autoplay, say 50 or 100 turns or any number (toggle (i.e. press this again) during autoplay to stop it before it completes if you want to do so).

I don't know if autoplay is a feature of base civ4 or bts or some mods that then most mods implemented check to know more (you could ask chatbots like chatgpt or such if you don't know, but their answers may be inaccurate or lead you astray, so double check or such even though they could be accurate or do web search for you or such but check to be sure) (or your best bet may be alternatively to check in cfc or such forums somewhere or google it xd)

Important note: For some behaviours to reliably show ingame by the autoplay AI you are playing, it strongly seems you need at least 2 2 autoplay turns IN ONE GO (sorry for caps but it's to insist), not just 2 turn buffer, but really playing at least 2 autoplay turns in one go.

For example in [62 - (Extremely better/stronger) AI almost not evacuating at all doomed cities (2 swordsmen in an 11 unit defending stack vs a 22+ attacking unit stack), fixed by always and 100% evacuating city doomed city regardless of land unit type, in CvUnitAI::AI_evacuateCity](/_1_AdvCiv-SAS/Docs/README_Known_Issues.md#ki-62), if we play instead from 2 turns before 1 autplay turn, then get the hand back and replay again 1 autoplay turn, units don't retreat, however if we do it 2 autoplay turns in one go then it works (rather than 1 autoplay turn then 1 autoplay turn again which doesn't), so make sure to test some behaviours at least 2 turns before and at least with 2 if not more autoplay turns.

## Example of DLL modification of CvGameTextMgr.cpp and other related file(s) to add the new "This technology cannot be traded" flag in Sevopedia tech 's placeSpecial and in tech tree view (technology advisor)

See screenshots of how this was implemented (not fully exhaustive but hopefully quite a bit exhaustive enough) in this [google drive folder](https://drive.google.com/drive/folders/176fGLxIWwOTRYAjafi2OGhe8VuVBZLMT?usp=sharing)

Small sample below /example too:

![0.5000_bTrade_bRepeat.JPG](/_1_AdvCiv-SAS/Images/advisors/0.5000_bTrade_bRepeat.JPG)
![0.710_sevopedia_techs (1).JPG](/_1_AdvCiv-SAS/Images/sevopedia/0.710_sevopedia_techs%20(1).JPG)

This was done by adding a new `buildBTradeString` function in (adjust to your mod path) C:\Program Files (x86)\Steam\steamapps\common\Sid Meier's Civilization IV Beyond the Sword\Beyond the Sword\Mods\AdvCiv-SAS\CvGameCoreDLL\CvGameTextMgr.cpp and (adjust to your mod path too) C:\Program Files (x86)\Steam\steamapps\common\Sid Meier's Civilization IV Beyond the Sword\Beyond the Sword\Mods\AdvCiv-SAS\CvGameCoreDLL\CvGameTextMgr.h

And with the big help or support for ideas i found myself too of chatgpt thanks

Since i didn't know how to do it, what helped me, with past CvGameTextMgr.cpp modifying experiences i successfully did such as for the power clean/dirty messages, or removing terrain modifiers since they are shown as buttons for clarity in Sevopedia unit's experiences in mind, was to search for one message that is displayed seemingly specifically in Sevopedia tech's placeSpecial.

But before that, i also had the idea to make sure the bTrade or something similar related to "trade" or "trad" (ctrl+f) exists in Sevopedia tech for example's in gc tech info by inspecting the gc tech info as in [Sevopedia/Debug/](/Assets/Python/Contrib/Sevopedia/Debug/). While not directly useful, it still helps to know.

The info is in `.isTrade()` method. I thought at first to hardcode it in placeSpecial's code, but that would be a bit ugly and not cover the tech advisor case where i wanted to show it there while not knowing how.

I asked chatgpt and it suggested me indeed this Sevopedia tech approach among other several ideas. But then i also had the idea as it suggested too but in another way to modify/check more the .cpp.

So back to CvGameTextMgr.cpp, i found this "Enables Bridge Building".

Then i could search which TXT_KEY uses this, and it is only once and specifically in CvGameTextMgr.cpp.

So by mirroring the functionning of it, but with the new `<bTrade>` (see [kujira's website bTrade info for details](https://gforestshade.github.io/kujira/post/civ4techinfos/#btrade) (translate to english with google chrome or your web browser or such or similar))

For example kujira's website says this (translated as such as explained before in previous paragraph/sentence just above  as of now in case paragraph(s)'s/sentence(s)'s order changed) provided for convenience:

```text
Sets whether this technology is tradable.
If you set this to 1, this technology will be tradable in diplomatic trades.
In BtS, this applies to all technologies except future technologies 1.

Value: 0 or 1

Example:
<bTrade>1</bTrade>
```

So then with the big help of chatgpt and my own ideas too and digging, but and also its support as i may have as well given up on it soon if not for its persistence and encouragement, and finally before i did it just worked luckily, so i am glad this feature (display bTrade info in Sevopedia tech and in tech advisor) is in the game now.

I also modified related files to the .cpp file(s), such as .h file(s) (see screenshots for details as i didn't recheck since then hence the plural singular but from my memory they should only be a very few/low number of files but adding this to be safe and cause bit lazy if i may say to recheck but hopefully helpful).

After all done, recompile the DLL, fix errors if any such as i had forgotten to add the .h  during compile, then again recompile (no fastdep as they seem to cause errors as well sometimes) cleanly, delete the old DLL rather than overwrite, in case we need to revert to old DLL or such, we have it as backup rather than it being lost if we simply copy pasted and overwrote old DLL.

What is very nice is that this same code change also displays it in tech advisor (tech tree view, F6 key ingame) which i also wanted to do and didn't know how, so this is also my first successful modification of the ingame behaviour of the tech tree view (minus the iGridX and iGridY i did before as well as part of modding)

This bTrade feature being displayed in placeSpecial should ideally have been part of civ4, so i hope players can now see this info if some techs use it (even though it seems in civ4 only future tech uses it, some mods seem to use it for several techs such as middle-earth mod but i only glanced quick, as for us in advciv-sas i intend to use it or at least try to use it if not more in a few key techs i want to use it at to prevent aggressive tech whoreism that is too advantageous to human players, while not forbidding tech trading altogether, hopefully more balanced and interesting this way)

I also added the info about the full list of which techs are not tradeable in placeSpecial, hopefully not too redundant or spammy this way in this case of the placeSpecial of Sevopedia tech and helpful maybe.

Note: later we also similarly changed the c++ so that we now also display the bRepeat info in tech advisor and Sevopedia tech, see also similarly buildBRepeatString

## Example of DLL modification 2: missing BBAI getters expose them to Sevopedia leader info in gc too for display

See [README_Known_Issues.md#ki-17](/_1_AdvCiv-SAS/Docs/README_Known_Issues.md#ki-17) on how this was implemented (google drive with screenshots link there too hopefully helpful)

## Example of performance optimization of python loading time (as in when we load the code) and loading times

The AI Personality feature in/of Sevopedia leader has a very or at least quite optimized precaching system, i think it is a good example or maybe at least time to show how i did it if it helps.

Even though i am not too knowledgeable, i hope this info and general process of how i did this can be informative or pleasant/enjoyable maybe too, see this doc section for details with google drive link with (= that has) screenshots and such: [README_AI_Personality_Panel.md#notes-about-performance-optimization-of-the-ai-personality-panel-caching](/_1_AdvCiv-SAS/Docs/README_AI_Personality_Panel.md#notes-about-performance-optimization-of-the-ai-personality-panel-caching)

## Drive

See also [the Modding_Ressources Google Drive](https://drive.google.com/drive/folders/1Hx-bvRy7joM54S0Vnmva6HX1h8ZZ-LAh?usp=sharing).
