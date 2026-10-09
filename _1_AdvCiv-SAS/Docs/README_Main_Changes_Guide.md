# Main Changes Guide

This guide highlights key differences between AdvCiv-SAS and base AdvCiv.

It’s intentionally practical, mostly player-facing, and a bit verbose so newer players can follow the "what changed" and "why it matters."

For technical details, see the main [README](/README.md) and the docs under [/_1_AdvCiv-SAS/](/_1_AdvCiv-SAS/).

Many of these changes are partially or entirely tunable via [`GlobalDefines_advciv_sas.xml`](/Assets/XML/GlobalDefines_advciv_sas.xml).

- **Player-tunable options (SAS defines):**
  - When this guide mentions DLL code locations (for example `CvUnitInfo::getTerrainAttackModifier`), that is just to document where the behavior comes from.
  - **Players do not need to modify or recompile the DLL** to change the behavior; you can instead change/tune the corresponding **SAS defines** (in the above mentioned **XML** file): for example the `SAS_CV_UNIT_INFO_ENABLE_UNIT_TERRAIN_ATTACK_MODIFIERS` SAS define can be set to `0` to disable or `1` to enable this option.
  - Note: after modifying them, restart Civ4 to apply changes.
- **Scope note:** The long-term goal is to make more gameplay options player-tunable via SAS defines when it is practical. Some changes are still hardcoded (or not worth exposing) due to complexity, risk, or limited value, so not everything is configurable.

Note: The main changes guide serves as an index that covers concisely most AdvCiv-SAS changes from base AdvCiv, but it may not be exhaustive, see the other AdvCiv-SAS supporting docs or files for further information.

## Menu

[Full code diff (very long)](/_1_AdvCiv-SAS/Docs/README_Main_Changes_Guide.md#full-code-diff-very-long)\
[Main Changes](/_1_AdvCiv-SAS/Docs/README_Main_Changes_Guide.md#main-changes)\
&emsp;[Not supported in AdvCiv-SAS](/_1_AdvCiv-SAS/Docs/README_Main_Changes_Guide.md#not-supported-in-advciv-sas)\
&emsp;[Renaming](/_1_AdvCiv-SAS/Docs/README_Main_Changes_Guide.md#renaming)\
&emsp;[Mod Name/Version](/_1_AdvCiv-SAS/Docs/README_Main_Changes_Guide.md#mod-nameversion)\
&emsp;[Tooling](/_1_AdvCiv-SAS/Docs/README_Main_Changes_Guide.md#tooling)\
&emsp;[System - Misc](/_1_AdvCiv-SAS/Docs/README_Main_Changes_Guide.md#system---misc)\
&emsp;[Code/Performance optimizations](/_1_AdvCiv-SAS/Docs/README_Main_Changes_Guide.md#codeperformance-optimizations)\
&emsp;[Logging](/_1_AdvCiv-SAS/Docs/README_Main_Changes_Guide.md#logging)\
&emsp;&emsp;[Logging (Common)](/_1_AdvCiv-SAS/Docs/README_Main_Changes_Guide.md#logging-common)\
&emsp;&emsp;[BBAI log](/_1_AdvCiv-SAS/Docs/README_Main_Changes_Guide.md#bbai-log)\
&emsp;&emsp;[SASGameRecord log](/_1_AdvCiv-SAS/Docs/README_Main_Changes_Guide.md#sasgamerecord-log)\
&emsp;[48 Civs DLL](/_1_AdvCiv-SAS/Docs/README_Main_Changes_Guide.md#48-civs-dll)\
[AI](/_1_AdvCiv-SAS/Docs/README_Main_Changes_Guide.md#ai)\
&emsp;[AI — General behaviour](#ai--general-behaviour)\
&emsp;&emsp;[General changes (AI)](/_1_AdvCiv-SAS/Docs/README_Main_Changes_Guide.md#general-changes-ai)\
&emsp;&emsp;[Victories (AI)](/_1_AdvCiv-SAS/Docs/README_Main_Changes_Guide.md#victories-ai)\
&emsp;&emsp;[Bonuses (AI)](/_1_AdvCiv-SAS/Docs/README_Main_Changes_Guide.md#bonuses-ai)\
&emsp;&emsp;[City Plots (AI)](/_1_AdvCiv-SAS/Docs/README_Main_Changes_Guide.md#city-plots-ai)\
&emsp;&emsp;[City Production (AI)](/_1_AdvCiv-SAS/Docs/README_Main_Changes_Guide.md#city-production-ai)\
&emsp;&emsp;[Specialists (AI)](/_1_AdvCiv-SAS/Docs/README_Main_Changes_Guide.md#specialists-ai)\
&emsp;&emsp;[Civics (AI)](/_1_AdvCiv-SAS/Docs/README_Main_Changes_Guide.md#civics-ai)\
&emsp;&emsp;[Technologies (AI)](/_1_AdvCiv-SAS/Docs/README_Main_Changes_Guide.md#technologies-ai)\
&emsp;&emsp;[Religions (AI)](/_1_AdvCiv-SAS/Docs/README_Main_Changes_Guide.md#religions-ai)\
&emsp;&emsp;[Leaders (AI)](/_1_AdvCiv-SAS/Docs/README_Main_Changes_Guide.md#leaders-ai)\
&emsp;&emsp;[Barbarians (AI)](/_1_AdvCiv-SAS/Docs/README_Main_Changes_Guide.md#barbarians-ai)\
&emsp;&emsp;[Diplomacy (AI)](/_1_AdvCiv-SAS/Docs/README_Main_Changes_Guide.md#diplomacy-ai)\
&emsp;&emsp;[Buildings (AI)](/_1_AdvCiv-SAS/Docs/README_Main_Changes_Guide.md#buildings-ai)\
&emsp;&emsp;[Units (Common) (AI)](/_1_AdvCiv-SAS/Docs/README_Main_Changes_Guide.md#units-common-ai)\
&emsp;&emsp;[Civilian Units (AI)](/_1_AdvCiv-SAS/Docs/README_Main_Changes_Guide.md#civilian-units-ai)\
&emsp;&emsp;&emsp;[Settlers (AI)](/_1_AdvCiv-SAS/Docs/README_Main_Changes_Guide.md#settlers-ai)\
&emsp;&emsp;&emsp;[Workers (AI)](/_1_AdvCiv-SAS/Docs/README_Main_Changes_Guide.md#workers-ai)\
&emsp;&emsp;&emsp;[Other Civilian Units (AI)](/_1_AdvCiv-SAS/Docs/README_Main_Changes_Guide.md#other-civilian-units-ai)\
&emsp;&emsp;[Military - Common (AI)](/_1_AdvCiv-SAS/Docs/README_Main_Changes_Guide.md#military---common-ai)\
&emsp;&emsp;[Military - Land (AI)](/_1_AdvCiv-SAS/Docs/README_Main_Changes_Guide.md#military---land-ai)\
&emsp;&emsp;[Military - Naval (AI)](/_1_AdvCiv-SAS/Docs/README_Main_Changes_Guide.md#military---naval-ai)\
[UI](/_1_AdvCiv-SAS/Docs/README_Main_Changes_Guide.md#ui)\
&emsp;[UI (Main Menu)](/_1_AdvCiv-SAS/Docs/README_Main_Changes_Guide.md#ui-main-menu)\
&emsp;[UI (Common)](/_1_AdvCiv-SAS/Docs/README_Main_Changes_Guide.md#ui-common)\
&emsp;[UI (Sevopedia)](/_1_AdvCiv-SAS/Docs/README_Main_Changes_Guide.md#ui-sevopedia)\
&emsp;[UI (In-game)](/_1_AdvCiv-SAS/Docs/README_Main_Changes_Guide.md#ui-in-game)\
[General changes](/_1_AdvCiv-SAS/Docs/README_Main_Changes_Guide.md#general-changes)\
[Game Speeds](/_1_AdvCiv-SAS/Docs/README_Main_Changes_Guide.md#game-speeds)\
[World Size/Scaling](/_1_AdvCiv-SAS/Docs/README_Main_Changes_Guide.md#world-sizescaling)\
[Map Generation (Common)](/_1_AdvCiv-SAS/Docs/README_Main_Changes_Guide.md#map-generation-common)\
[Mapscripts](/_1_AdvCiv-SAS/Docs/README_Main_Changes_Guide.md#mapscripts)\
[Handicaps](/_1_AdvCiv-SAS/Docs/README_Main_Changes_Guide.md#handicaps)\
[Terrains / Features](/_1_AdvCiv-SAS/Docs/README_Main_Changes_Guide.md#terrains--features)\
[Bonuses](/_1_AdvCiv-SAS/Docs/README_Main_Changes_Guide.md#bonuses)\
[Improvements](/_1_AdvCiv-SAS/Docs/README_Main_Changes_Guide.md#improvements)\
[Builds](/_1_AdvCiv-SAS/Docs/README_Main_Changes_Guide.md#builds)\
[Specialists](/_1_AdvCiv-SAS/Docs/README_Main_Changes_Guide.md#specialists)\
[Traits](/_1_AdvCiv-SAS/Docs/README_Main_Changes_Guide.md#traits)\
[Civics](/_1_AdvCiv-SAS/Docs/README_Main_Changes_Guide.md#civics)\
[Technologies](/_1_AdvCiv-SAS/Docs/README_Main_Changes_Guide.md#technologies)\
[Eras](/_1_AdvCiv-SAS/Docs/README_Main_Changes_Guide.md#eras)\
[Culture](/_1_AdvCiv-SAS/Docs/README_Main_Changes_Guide.md#culture)\
[Religions](/_1_AdvCiv-SAS/Docs/README_Main_Changes_Guide.md#religions)\
[Corporations](/_1_AdvCiv-SAS/Docs/README_Main_Changes_Guide.md#corporations)\
[Votes](/_1_AdvCiv-SAS/Docs/README_Main_Changes_Guide.md#votes)\
[Event Triggers and Events](/_1_AdvCiv-SAS/Docs/README_Main_Changes_Guide.md#event-triggers-and-events)\
[Barbarians](/_1_AdvCiv-SAS/Docs/README_Main_Changes_Guide.md#barbarians)\
[Civilizations](/_1_AdvCiv-SAS/Docs/README_Main_Changes_Guide.md#civilizations)\
[Leaders](/_1_AdvCiv-SAS/Docs/README_Main_Changes_Guide.md#leaders)\
[Diplomacy](/_1_AdvCiv-SAS/Docs/README_Main_Changes_Guide.md#diplomacy)\
[Buildings](/_1_AdvCiv-SAS/Docs/README_Main_Changes_Guide.md#buildings)\
[Units (Common)](/_1_AdvCiv-SAS/Docs/README_Main_Changes_Guide.md#units-common)\
[Civilian Units](/_1_AdvCiv-SAS/Docs/README_Main_Changes_Guide.md#civilian-units)\
[Military - Common](/_1_AdvCiv-SAS/Docs/README_Main_Changes_Guide.md#military---common)\
[Military - Land](/_1_AdvCiv-SAS/Docs/README_Main_Changes_Guide.md#military---land)\
[Military - Naval](/_1_AdvCiv-SAS/Docs/README_Main_Changes_Guide.md#military---naval)\
[Military - Air](/_1_AdvCiv-SAS/Docs/README_Main_Changes_Guide.md#military---air)\
[Fixes](/_1_AdvCiv-SAS/Docs/README_Main_Changes_Guide.md#fixes)

## Full code diff (very long)

See [Full exhaustive changes (very long)](/README.md#full-exhaustive-very-long-and-exhaustive-changes).

## Main Changes

### Not supported in AdvCiv-SAS

Some features are not supported in AdvCiv-SAS; e.g., non-English languages, pre-AdvCiv-SAS scenarios. See [root README.md: Not supported in AdvCiv-SAS](/README.md#not-supported-in-advciv-sas). See also [KI#166](/_1_AdvCiv-SAS/Docs/README_Known_Issues.md#ki-166).

### Renaming

- "Play Now" → "Simple Game" Clearer, matches code name, better contrasts with "Custom Game", and same as in C2C mod.
- "WFYABTA" → "We fear you are trading more than us." Same mechanic; the new text reflects that it’s about trading volume, not tech pace.
- Fixed misleading event text such as "The forge has been destroyed" used even when building actually survives (e.g. "The forge has caught fire" instead).
- Foreign Advisor's "Active" tab -> "Treaties" tab (no longer shows redundant bonuses).
- "All Units" → "No UnitCombat" in the Military Advisor: was inaccurate: this does not list all units (e.g. military units such as swordsmen), but only `<Combat>NONE</Combat>` units (e.g. Worker, Workboat, Missionary).
- Rename UnitCombats for concision or accuracy: "Recon Units" -> "Recon"; "Siege Weapons" -> "Siege"; "Helicopter Units" -> "Helicopters"; "Armored Units" -> "Armor"; "Naval Units" -> "Naval"; "Gunpowder Units" -> "Gun" (was inaccurate since gunpowder is no longer used in modern guns).
- Nuclear interception text clarifies that displayed chance is **adjusted** by nuke evasion; see the forum thread for context ([civfanatics](https://forums.civfanatics.com/threads/sdi-icbm-and-tactical-nukes.239415/)).
- "Civilopedia" → "Sevopedia".
- "Ressource(s)" → "Bonus(es)".
- "Difficulty" → "Handicap" (matches Civ4 naming).
- Renamed "Horseback Riding" to "Mounted Combat" (includes all mounts, not just horse-based ones).
- Bonus names standardized: Ivory → Elephants; Cow → Cattle; Clam → Molluscs; Wine → Grapes, etc.
- Terrain/Feature: Ice (Terrain) → Ice Sheet; Ice (Feature) → Ice Cap.
- "Great wonders" → "World wonders" (functionally the same).
- Leader names corrected where needed (e.g., Montezuma → Moctezuma II).
- Taoism → Daoism (with matching building/unit names and encyclopedia text).
- Cho-Ko-Nu → Zhuge Nu (non-standard/incorrect Wade–Giles romanization; now using modern pinyin).
- "Unique units" → "Civilization units" (they aren’t singletons; they’re civ-specific).
- Cavalry → Dragoon (too generic/era-agnostic; new name and unit better matches early-industrial role).
- Promotion names made more explicit (e.g., Counter-Archer, Counter-Siege, City Bombard Damage). Roman numerals → Arabic ("Combat 3").
- "Demographics" → "Info Screen" (correct advisor name: does not only show the Demographics tab of the Info Screen) (Sevopedia Shortcuts)
- "Victory Status" and "Victory Conditions" → "Victory Screen" (similar inconsistent naming in Sevopedia Shortcuts)
- "Units" -> "Start Scroll at Units (Production Chooser Bar)" (City Screen) (clearer and so it is not confused with the new AdvCiv-SAS Building Filter buttons' behaviour (filter on click vs set default scroll position))

### Mod Name/Version

- (Requires AdvCiv-SAS 6326+) **Centralized AdvCiv-SAS mod identity:**
  - The main runtime display/project name is now defined once through `SAS_MOD_DISPLAY_NAME` in [`GlobalDefines_advciv_sas.xml`](/Assets/XML/GlobalDefines_advciv_sas.xml), and shared by runtime UI/logging/replay code through AdvCiv's existing `ModName` / `CvModName` infrastructure.
  - This display name is intentionally separate from the actual installed mod-folder name, which BtS/AdvCiv detects dynamically.
  - Known startup/theme paths that cannot use the runtime define are documented next to it as a manual rebranding checklist; a full rename should still search for remaining old-name/path references.
  - This also provides the structural groundwork used by the automatic runtime version/commit reporting added below; the existing commit-count version-number scheme itself is unchanged.
  - See [root README: Version number](/README.md#version-number) and [Quick Install Guide: Modify the mod (its development version)](/_1_AdvCiv-SAS/Docs/README_Quick_Install_Setup_Guide.md#modify-the-mod-its-development-version).
- (Requires AdvCiv-SAS 6357+) **Automatic runtime version/source details:**
  - The same central `ModName` resolver now identifies the current source without a manually maintained version string.
  - Full Git checkouts report the practical commit-count version, exact SHA, branch, commit date, and tracked dirty files; shallow/unknown-history checkouts keep the exact SHA but deliberately omit an unreliable history-derived count.
  - GitHub/git-archive ZIPs use [SASModVersion.txt](/Assets/SASModVersion.txt) with `.gitattributes` `export-subst`: one immutable `SAS_VERSION_ANCHOR_<exact-commit-count>` tag provides the known base count and Git's `describe` distance supplies the exact number of later commits, so normal development/release ZIPs reconstruct the same practical version without per-commit VERSION updates.
  - Archives also retain exact commit/date provenance but cannot verify edits made after extraction, represented explicitly as `dirty=-1` and `dirtyTrackedCount=-1`; live Git uses `0` for verified clean and `1` for tracked changes.
  - BBAI and SASGameRecord write the same `*_SOURCE_CONTEXT` beside their mod and exact loaded-DLL provenance immediately after each new/load lifecycle marker, and BUG/BULL surfaces using `CvModName` receive a compact `version (short-SHA[, dirty])` display.
  - Tracked dirty-file names are capped in log display while the uncapped total dirty-file count is preserved.
- (Requires AdvCiv-SAS 6358+) **Persistent save version/source history:**
  - `CvGame` now stores a compact source-revision lineage with each save: the creation revision, then one transition entry whenever a later load runs under a different practical version or full commit SHA.
  - Each entry records game turn, version, canonical SHA and coarse dirty state; full dirty-file lists remain log/runtime-only.
  - If source metadata itself is unavailable, the creation entry keeps honest unknown values rather than inventing provenance.
  - As with other AdvCiv-SAS save-schema changes, older save layouts are intentionally unsupported rather than migrated.
  - The history is exposed read-only to Python for later Victory/Sevopedia/replay presentation and is emitted by SASGameRecord for immediate testing/analysis.
- (Requires AdvCiv-SAS 6378+) **Visible version/source and save-lineage UI:**
  - Victory Screen -> Settings now replaces the old plain mod-name row with the compact current runtime identity and every persisted save creation/transition entry; the runtime row opens a new dynamic Sevopedia Mods Info page with full version/SHA, branch/date, metadata source, dirty status/files, actual folder/path and current save lineage.
  - Replay Settings persist the compact runtime source and full save lineage into the replay message itself, while replay filenames add the practical AdvCiv-SAS version when known so replay/Hall-of-Fame files remain identifiable without SHA/dirty filename clutter.
  - BUG System keeps its existing compact current-runtime display.
  - Permanent HUD/theme text is intentionally avoided to keep ordinary gameplay uncluttered.

### Tooling

- (Requires AdvCiv-SAS 6581+) **Repository encoding and Windows-path CI:**
  - checks tracked files for unsupported BOMs and paths too long for the standard Steam installation: full installed file paths may use at most 259 UTF-16 units, and parent directory paths at most 247.
  - These limits include the installation root and mod-folder name; the separate filename cap below complements them.
  - Visual Studio project/solution UTF-8 BOMs are preserved for launcher compatibility: [`AdvCiv.sln`](/CvGameCoreDLL/Project/AdvCiv.sln) and [`AdvCiv.vcxproj`](/CvGameCoreDLL/Project/AdvCiv.vcxproj) are examples inherited from the unmodified AdvCiv 1.14 comparison snapshot, rather than SAS-added BOMs. Removing them was followed by the SAS project shortcut no longer opening; the original BOMs were restored.
  - Civ4 source/data BOMs remain rejected.
  - See [repository hygiene CI](/.github/workflows/README.md#buildrepository_hygienepy).
- (Requires AdvCiv-SAS 6595+) **Shorter filenames and filename-length CI:**
  - limit tracked filenames, including extensions, to 56 UTF-16 units alongside the existing full installation-path checks.
  - Shortened eight music filenames and one example shortcut, updating the music XML references without changing file contents or original download links and credits.
  - The existing 52-unit `Gilgamesh__gilgamesh_parent_action_02_affirmative.kf` provided the baseline for choosing 56, adding four units of leeway while keeping descriptive names and installation-path headroom.
  - Its exact name is embedded in `Gilgamesh.kfm`, so renaming only the animation file would break that reference.
  - CI collects all filename/path violations before failing.
  - See [repository hygiene CI](/.github/workflows/README.md#buildrepository_hygienepy).
- **Light source archive helper:**
  - (Requires AdvCiv-SAS 6605+) Separate cumulative patches show committed branch work and the complete working snapshot, including staged/unstaged edits, against the same shared ancestor with the detected default branch. Both filter CRLF/LF and trailing-whitespace noise so reviewers can focus on meaningful changes.
  - added [`make_light_source_zip.py`](/LLM_Helpers/README.md#make_light_source_zippy), which creates a timestamped compressed AdvCiv-SAS source/data/docs archive under the mod root or a chosen output folder.
  - A small tracked Git file-tree manifest is also bundled so ZIP-only LLMs can see repository paths intentionally omitted from the archive.
  - The archive now also includes generated `_SNAPSHOT_CONTEXT` with the current Git/repository state plus useful filtered textual diffs across the current HEAD's K-Mod -> pre-SAS AdvCiv -> AdvCiv-SAS branch history, including later upstream AdvCiv commits merged into that branch, making offline/ZIP-only LLM review and historical investigation much more complete without including the full `.git` repository or historical source trees.
  - (Requires AdvCiv-SAS 6605+) A compact summary in the console and archive makes the review context easier to identify: current versus default-branch progress, staged/unstaged change counts and links to their already-bundled diffs, plus timestamped packaging/runtime observations. See the [output example](/LLM_Helpers/README.md#example-of-output-light_source-zip-git-bash).
  - (Requires AdvCiv-SAS 6605+) A cumulative branch diff and default-branch reference files complement the current source, making the overall change and before/after code directly reviewable by humans or AI/LLM helpers from one archive, without a local Git checkout.
  - (Requires AdvCiv-SAS 6605+) Immediate pre-edit HEAD/index reference files support direct review of partially staged work, while cache-reuse and regeneration progress messages make source packaging easier to follow during long first-time runs.
  - (Requires AdvCiv-SAS 6605+) Omitted DLLs still have exact byte-identity and size-change context, letting source-only reviewers distinguish identical versus changed binaries without adding the binaries to the archive.
  - This avoids repeatedly manually selecting and tediously creating an updated ZIP of each file/folder each time, of the same useful core files/folders ChatGPT or other LLMs can read or needs, while leaving unneeded or too heavy to use ones (e.g., as of now no .fpk, .tga, .dll, or pycache folders, broad art assets folder), when sharing compact mod context with local tools or LLMs. Some image folders are included too because advisors, Sevopedia, main menu, common UI, and rendered `SASGameRecord` map text are very useful for LLM development/review and help show what the mod actually looks like.
  - The `SASGameRecord_map_text` screenshots are included because an LLM may read the raw text-map characters without reconstructing the visual/geographical layout as easily.
  - Local agentic tools like Codex can inspect these screenshots directly, while external/ZIP-only LLMs like ChatGPT can only use them if the archive includes them; adding every image folder would currently add 50+ MB and roughly double the archive.
- (Requires AdvCiv-SAS 6596+) **More compact generated commit-history context:**
  - imported reference documents and archived published changelog copies now retain change summaries and path-history navigation without duplicating their full patches in generated `commit_diffs/`.
  - Current reference/archive files remain available under the normal light-source snapshot rules; maintained README indexes, source files and ordinary development documentation keep their detailed history.
  - The cache policy changes automatically so older cached renderings are not reused under the new filtering.
  - See [light-source history filtering](/LLM_Helpers/README.md#make_light_source_zippy).
- (Requires AdvCiv-SAS 6581+) **Generated documentation CI:**
  - regenerates registered source-to-text conversions and the published handicap XML comparison to detect stale tracked outputs.
  - The AdvCiv manual is the first registered conversion; future conversions can supply their own source, output and converter.
  - Complete PR/push ranges also require paired text updates when a registered source or related artifact changes.
  - See [generated documentation CI](/.github/workflows/README.md#buildgenerated_docspy).
- (Requires AdvCiv-SAS 6584+) **Broader XML, Markdown, diagnostics and revision-history CI:**
  - checks ArtDefine collection/entry structure, unit/building primary technology ordering, all resource trade prerequisites, Markdown menu coverage/order and bold formatting, SAS define declaration/runtime-use coverage and integer bounds, diagnostic format-string safety, shared AI-strategy descriptor coverage, and SASGameRecord revision/history synchronization and committed-history provenance.
  - Refreshed existing document menus to match their body headings.
  - See [current build checks](/.github/workflows/README.md#current-build-checks).
- **Markdown links/images CI:**
  - added GitHub workflow checks for repository-local Markdown images and links, including `#heading` anchors.
  - They catch missing/moved local targets and stale renamed section/KI links automatically; external web links are intentionally not checked.
  - See [build/markdown_images (Readme)](/.github/workflows/README.md#buildmarkdown_imagespy) and [build/markdown_links (Readme)](/.github/workflows/README.md#buildmarkdown_linkspy).
- **AIP predump refresh bot:**
  - added [`aip-predump-refresh.yml`](/.github/workflows/aip-predump-refresh.yml), a tested GitHub Actions bot workflow for the Sevopedia Leader AI Personality predump cache.
  - The existing checker still fails when the committed predump is stale, but the refresh workflow can now run `aip_predump_values.py --write` outside Civ4 and open/update a bot PR containing only `SevoPediaLeaderCachePredumped.py` when real generated AIP data or display-label drift is detected.
  - It runs automatically after pushes to known AIP predump dependency files.
  - Tested in [PR #31](https://github.com/wonderingabout/AdvCiv-SAS/pull/31) with no-op, XML comment-only, numeric Leader XML, and Python label/display drift cases, so harmless comments do not refresh the cache while real generated predump changes do.
- **Python Ruff GitHub workflow:**
  - added [`python-ruff.yml`](/.github/workflows/python-ruff.yml), which prints a full Ruff report and fails on a practical critical gate for syntax/parse errors and Pyflakes bug checks.
  - This helped spot shadowed duplicate callbacks in base Civ4 `Oasis.py` (see [KI#164](/_1_AdvCiv-SAS/Docs/README_Known_Issues.md#ki-164)) and a Base AdvCiv bug of dormant undefined `Point` (instead of using `PointLayout`) helper in `RectLayout.py` (see [KI#165](/_1_AdvCiv-SAS/Docs/README_Known_Issues.md#ki-165)).
- (Requires AdvCiv-SAS 6593+) **Markdownlint GitHub workflow:**
  - added [`markdownlint.yml`](/.github/workflows/markdownlint.yml), which checks non-ignored Markdown throughout the repository using the same rule settings as the editor.
  - New documents are covered automatically; explicit exclusions preserve upstream/reference material, generated helper content and version-named changelog archives while their maintained README indexes remain checked.
  - This complements the existing menu and local-link checks and reports failures without rewriting files.
  - For example, three tab-indented history bullets in the root README were parsed as a code block and caused MD046 warnings on every later fenced example; removing the leading tabs fixed the list and all twelve warnings.
  - See [workflow scope and local command](/.github/workflows/README.md#markdown-lint-workflow).
- **Python 2.4 Docker compile GitHub workflow:**
  - added [`python24-compile.yml`](/.github/workflows/python24-compile.yml), which runs `Assets/Python` and `PrivateMaps` through real CPython 2.4 syntax/bytecode compilation using our separate [`wonderingabout/python-2.4-docker`](https://github.com/wonderingabout/python-2.4-docker) GHCR image.
  - This complements Ruff by checking the old parser Civ4 embeds, without launching Civ4 or importing engine-only modules.
  - A deliberate [test confirmed it fails](https://github.com/wonderingabout/AdvCiv-SAS/actions/runs/27897204427) on Python 2.5+ ternary syntax such as `SAS_MAGIC_PY24_COMPILE_BREAK_TEST = 1 if True else 0` in `SASMagicNumbers.py`.
  - See [`.github/workflows/README.md`](/.github/workflows/README.md#python-24-compile-workflow), and [README.md (Python Scripts)](/README.md#python-scripts) for screenshots.
- **Dependabot for GitHub Actions:**
  - added Dependabot tracking for GitHub Actions versions to help address Node.js runtime deprecation warnings such as `actions/checkout@v4` and `actions/setup-python@v5` still using Node.js 20 (see [dependabot@PR#24](https://github.com/wonderingabout/AdvCiv-SAS/pull/24)). This should reduce manual maintenance and make future action-version updates easier to notice and apply.
  - See [`.github/workflows/README.md` current build checks](/.github/workflows/README.md#current-build-checks).
- **Handicap XML comparison helper:** added [`compare_handicap_infos.py`](/LLM_Helpers/compare_handicap_infos.py), a report-only, mod-agnostic helper that compares two explicit `CIV4HandicapInfo.xml` file paths, inside or outside this mod folder, by handicap index; shows missing fields as blanks, includes absolute/percentage numeric deltas, and embeds a spreadsheet-friendly TSV matrix.
- **External File Opening**: Access from Civ4 ingame local files in the system on Windows. See [KI#87](/_1_AdvCiv-SAS/Docs/README_Known_Issues.md#ki-87).
- Make **paths absolute** in **.gitignore** when relevant to fix folders with an identical name being incorrectly ignored; add **.gitattributes** for more precise archiving.
- (Requires AdvCiv-SAS 6366+) **Lower-CPU Debug-opt compilation:**
  - Debug-opt previously used parallel `bin\jom build` (in [AdvCiv.vcxproj](/CvGameCoreDLL/Project/AdvCiv.vcxproj)), which sustained high CPU usage and audibly drove the CPU fan to full speed.
  - Recent Visual Studio logs measured parallel Debug-opt at `24.83` seconds versus `1` minute `46.22` seconds for sequential Release, only `81.39` seconds saved in a development iteration where an autoplay usually takes `10+` minutes.
  - That small overall workflow-time gain does not justify the repeated CPU load, heat and power use when compiling many or dozens of DLLs during a development day.
  - To fix this, Debug-opt now uses the same sequential `nmake dll` build path as Release to avoid needlessly stressing the hardware; the explicitly named `*-fast` configurations remain available when the shortest compile time is preferred.
  - See [KI#38.3](/_1_AdvCiv-SAS/Docs/README_Known_Issues.md#ki-38.3).

### System - Misc

- (Requires AdvCiv-SAS 6374+) **Permanent automatic saves for ordinary play and AI Auto Play testing:**
  - keep the beginning and outcome of a game without having to tediously remember to save them yourself.
  - Tunable `SASFastSave` corresponding defines create ordinary saves at new-game start, actual game end/victory, and an AI Auto Play stop (whether the requested turns finish or you interrupt it manually).
  - These files stay in `My Games/Beyond the Sword/Saves/single`, separate from Civ4's rotating `Autosaves`, so a useful starting position or completed test is not lost as newer autosaves replace older ones.
  - Automatically descriptive names also avoid repeatedly handling generic names such as `PC 50000 BC`, choosing whether to overwrite them, or manually tediously inventing another filename or having to save or remember to save them everytime not to lose them.
  - Game-start and game-end saves work for ordinary single-player games too; the autoplay-stop option makes it easy to retain a test checkpoint and compare another run from the same starting save.
  - Each option can be disabled independently and is separate from BUG AutoSave settings.
- (Requires AdvCiv-SAS 6374+) **Recognize a game quickly in the narrow save list and find its matching replay files:**
  - `SASFastSave` save file names show the original leader, turn, world size and map first, followed by victory/winner when applicable, game speed, initial map and sync seeds, and UTC timestamp.
  - Compact leader labels and short victory labels leave more useful information visible; `START`/`STOP`/`END` sit at the end.
  - For example, in a SASGameRecord the game was recorded as Alexander Custom Continents game with initial map and sync seeds `41500016`, so the corresponding `SASFastSave` starting and victory save files would be named `ALEXANDER_T0000_HUGE_CUSTOM_CONTINENTS_NORMAL_41500016_41500016_20260909T171232Z_START` and `ALEXANDER_T0459_HUGE_CUSTOM_CONTINENTS_SPACE_ZARA_YAQOB_NORMAL_41500016_41500016_20260910T184208Z`.
  - To find related saves while another autoplay runs, copy an initial seed's number from its game record and search the save folder: `41500016` matches these names directly.
  - Both seeds are plain decimal, in map-then-sync order; use the initial seeds rather than current RNG states, which change during play.
  - (Requires AdvCiv-SAS 6374+) Names use stable XML identities rather than a user-entered name such as `PC`; the original leader remains the same even if autoplay hands control to another player after defeat.
    - Leader labels keep complete name chunks within a length cap, using underscores for spaces and punctuation: `DE_GAULLE` stays intact, while `JULIUS_CAESAR` becomes `JULIUS` rather than ending mid-word.
    - Only a first name chunk that exceeds the cap is truncated.
    - A different winning team's leader follows the victory label; your own leader is not repeated if your team wins.
    - Victory labels are `SPACE`, `DOM`, `CONQ`, `CULT`, `DIPLO`, `TIME` and `SCORE`; custom victory names remain readable sanitized tokens.
    - Abbreviated leaders and reused seeds are not unique game identifiers, so map context and timestamps help distinguish games and repeated tests.
    - Same-second filename collisions receive a numeric suffix rather than overwriting a save.
    - An autoplay victory does not also create a redundant `STOP` save.
    - No existing saves are renamed.
- (Requires AdvCiv-SAS 6374+) AI Auto Play can additionally prefill its turns popup from the selected game speed's configured total turns + 1 (Normal `501`, Marathon `1001`, etc.); disabling that scaling uses the literal `SAS_AIAUTOPLAY_DEFAULT_TURNS_UNSCALED_GAMESPEED` fallback.
  - The seed getters are read-only and do not advance RNG state.
  - See [SASFastSave.py](/Assets/Python/SASFastSave.py) and [GlobalDefines_advciv_sas.xml](/Assets/XML/GlobalDefines_advciv_sas.xml).
- **Autoplay runs now continue without being interrupted/paused when the active player dies or other notices:**
  - an active player's defeat could transfer human control during an ongoing autoplay and make the EXE pause behind an OK-only "it's your turn" prompt; ordinary informational text notices could also require an unnecessary click and unconveniently pause the run that can no longer complete unattended without user supervision or intervention.
  - To fix this, an XML-tunable option now omits only these no-decision prompts during autoplay, without accepting or suppressing diplomacy, events, civics, war, launch, production, technology, or other gameplay choices.

### Code/Performance optimizations

- **Merged** the **old split/interdependent Foreign Advisor** implementation from the two-file setup (`CvForeignAdvisor.py` + `CvExoticForeignAdvisor.py`) into a **single unified** [CvForeignAdvisor.py](/Assets/Python/Screens/CvForeignAdvisor.py) with the **full tab set** (as of now Relations/Glance/Active/Bonus/Info/Tech/Cities).
  - In our findings, **this was the only screen file** using that **interdependent split** pattern, so merging it into one file directly **fixes that inconsistency** and **reduces maintenance burden**; other major screen files such as [CvMainInterface.py](/Assets/Python/Screens/CvMainInterface.py) and [CvInfoScreen.py](/Assets/Python/Screens/CvInfoScreen.py) are lengthy but unified files.
  - By contrast, true alternatives use independent variants (for example BUG/non-BUG [CvBUGMilitaryAdvisor.py](/Assets/Python/Screens/CvBUGMilitaryAdvisor.py) / [CvMilitaryAdvisor.py](/Assets/Python/Screens/CvMilitaryAdvisor.py) paths), not interdependent split files.
- **Remove screen-dependent geometry computation** from `__init__` in advisors because this advisor object is created before a reliable screen context exists; doing layout in `interfaceScreen` **ensures correct values for the current resolution**.
  - Keeping **only screen-independent edge constants** in `__init__` ensures **no redundant recomputation** is made again in `interfaceScreen`, and **catches loud screen-dependent errors** in `__init__`.
- **Heavily refactor and simplify Sevopedia Main** so it is much easier to add, reorder, or modify or otherwise maintain categories. See [Sevopedia reworks (example 0.0)](/_1_AdvCiv-SAS/Docs/README_Sevopedia_Reworks.md#example-00-heavily-refactored-sevopedia-maintain-so-it-is-much-easier-to-maintain-or-customize).
- **Collapse multiline statements to single-line form** (e.g., `draw_expandable_text_panel(screen, self.top, szTitle, self.X_HISTORY, self.Y_HISTORY, self.W_HISTORY, self.H_HISTORY, szText, self.bHistoryExpanded, SAS_MAGIC_PEDIA_PYTHON_HISTORY_EXPAND)`): much **easier** to **scan**, **maintain**, and **grep**, especially **for LLMs**.
  - Often done using scripts to batch process it, and done as we go rather than all at once since this is a recent change.
  - Especially important in [CvMainInterface.py](/Assets/Python/Screens/CvMainInterface.py) where ~2158 lines of code have been collapsed to ~879 lines, and ~187 lines of comments to ~83 lines.
  - See also [LLM_Helpers (README.md)](/LLM_Helpers/README.md) and [PR#23](https://github.com/wonderingabout/AdvCiv-SAS/pull/23). Then done in other py files in a more manual or gradual way.
- **Python 2 bare-print cleanup:**
  - converted simple active `print "..."` statements to Python 2.4/3-compatible single-string calls such as `print("msg %s" % value)`, using [`wrap_python2_prints_for_linting.py`](/LLM_Helpers/wrap_python2_prints_for_linting.py) plus manual comma-print cleanup.
  - This keeps Civ4 Python 2.4 compatibility while reducing Ruff/Python 3 parser noise (e.g., on VS Code) so real issues are easier to see.
  - Also used **local ruff checks** to find and cautiously address likely fixable py linter issues that wouldn't break code changes.
  - See [Modding_Ressources/README.md: How to check with ruff installed locally a folder](/_1_AdvCiv-SAS/Docs/Modding_Ressources/README.md#how-to-check-with-ruff-installed-locally-a-folder-for-py-linter-errors-candidates-to-fix-with-text-report).
- **Cleaned** several **old Python 2-style** BUG syntax patterns so modern Python 3 linters/parsers can inspect them while keeping Civ4 Python 2.4 compatibility.
  - These **distracted** modern Python linters such as ruff and **prevented them from finding errors**.
  - Removed these parser blockers, such as old comma-exception binding `except Exception, e:` (to the already existing `e = sys.exc_info()[1]` in [BugUtil.py](/Assets/Python/BUG/BugUtil.py) pattern) and the old compatibility fallback `True, False = 1, 0` (to an equivalent), which improves code quality.
  - This allowed Ruff to surface and find a **real previously hidden bug** in BUG [configobj.py](/Assets/Python/BUG/configobj.py): `_handle_comment` used undefined `_a_to_u('# ')` instead of the correct `self._a_to_u('# ')`, which we then **fixed**.
  - See also [KI#143](/_1_AdvCiv-SAS/Docs/README_Known_Issues.md#ki-143).
- **Python source encoding cleanup:**
  - fixed Ruff E902 `stream did not contain valid UTF-8` errors in old BUG TechWindow Python files by converting them to UTF-8 in VS Code.
  - UTF-8 is the natural target because most Python files in the mod already use it, and this fixed Ruff parsing while preserving comments and leaving runtime code unchanged.
  - This seems less risky than XML encoding edits so done for these py files.
- **Code cleanup and linting support:**
  - started a **broad Python** cleanup, linting, and source sanitizing work to reduce formatting noise and make large legacy files easier to review.
  - For especially **messy files** where automated scripts are **risky** (typically mapscripts), some files are now also repaired through **full-file LLM review** (e.g., by ChatGPT), then **checked manually** so indentation and code structure are cleaned up carefully.
- **Repo-wide blank-line cleanup (`.py` only):**
  - reduced excessive **consecutive blank lines** to a **single blank line** in Python files with a `(\r?\n)(?:[ \t]*\r?\n){2,}` find to `$1$1` regexp on VS Code.
  - This is a pure formatting no-op (Python ignores blank lines), but it makes large files denser, easier to scan, easier to grep, and more manageable for future review/LLM-assisted cleanup.
  - Comments and code are untouched; only **vertical whitespace** is normalized.
- A later **horizontal whitespace** pass also **stripped spaces/tabs** from otherwise empty Python lines to reduce invisible diff/lint noise: done with the help of `^[\t ]+$` find regexp with replace all (to nothing) globally on VS Code.
- Convert **single quote to double quote** (e.g. `'BONUS_IRON'` to `"BONUS_IRON"`), which allowed to **detect and fix incorrect fields** and later strengthen them with `getInfoTypeOrFail(...)`.

- **GitHub workflow build checks:**
  - added GitHub Actions checks under [`.github/workflows`](/.github/workflows/) to catch forgotten debug/default settings and data drift at each commit.
  - Current checks notably verify launch-guard sentinel values, XML tag references in SAS define text values, integer SAS define bounds, valid main-menu opening-music script setup, disabled BBAI logging defaults, shared UI font defaults, Sevopedia Leader AI Personality Panel predumped-cache defaults and predump file presence, `WorldSizeTypes` enum/XML alignment, and `CvGameCoreDLL/Project/temp_files/` containing only its tracked placeholder plus the intentionally retained/ignored Debug-opt target whose exact PDB enables crash diagnosis.
  - Other generated targets remain visible and fail the check; every configuration's exact target must be deleted before rebuilding it (helps catch stale fast-compile temp files and forgetting to replace the committed DLL after compiling; see [KI#38](/_1_AdvCiv-SAS/Docs/README_Known_Issues.md#ki-38) and [KI#38.2](/_1_AdvCiv-SAS/Docs/README_Known_Issues.md#ki-38.2)).
  - Checks also verify every playable `PrivateMaps/*.py` is listed exactly once across the SAS map-script heaviness/coverage defines while every listed map script exists in `PrivateMaps/`.
  - For example, this helped spot [map scripts that were previously unclassified in SAS map-script heaviness defines](https://github.com/wonderingabout/AdvCiv-SAS/actions/runs/27198308080/job/80295526028); they are now listed explicitly for exhaustiveness (more robust; no gameplay change).
- Other examples: This notably also **helped spot and fix duplicate text, parent, and child XML keys** found by new GitHub workflow check [text](https://github.com/wonderingabout/AdvCiv-SAS/actions/runs/27332010214/job/80746523894), [parent](https://github.com/wonderingabout/AdvCiv-SAS/actions/runs/27258698041/job/80498936912), and [child](https://github.com/wonderingabout/AdvCiv-SAS/actions/runs/27336768909/job/80762873956) (See [KI#148](/_1_AdvCiv-SAS/Docs/README_Known_Issues.md#ki-148), [KI#149](/_1_AdvCiv-SAS/Docs/README_Known_Issues.md#ki-149), [KI#150](/_1_AdvCiv-SAS/Docs/README_Known_Issues.md#ki-150)), and [suspicious malformed-looking XML tag punctuation](https://github.com/wonderingabout/AdvCiv-SAS/actions/runs/27342682844/job/80783239087) (e.g., `<French>>`, `</English>>`) (see [KI#151](/_1_AdvCiv-SAS/Docs/README_Known_Issues.md#ki-151)) and [XML suspicious text chars](https://github.com/wonderingabout/AdvCiv-SAS/actions/runs/27400470104/job/80977135402) (see [KI#152](/_1_AdvCiv-SAS/Docs/README_Known_Issues.md#ki-152)).
  - The checks also run locally with Python 3; see [`.github/workflows/README.md` current build checks](/.github/workflows/README.md#current-build-checks).
- **Launch guard for SAS defines (startup/load/Sevopedia):**
  - Added a strict define-integrity check using one **int sentinel** and one **string sentinel**, validated at pre-game start, game start, save load, Sevopedia creation, and Sevopedia show.
  - If a value is missing or mismatched, Python raises a **fatal error** with `actual` vs `expected` values and guidance to launch from a Windows shortcut.
  - The game will be **unplayable if the config is incorrect**.
  - Empirically confirmed when a UTF-8 BOM caused Civ4's XML parser to reject `GlobalDefines_advciv_sas.xml`: the missing integer sentinel resolved to `0`, and the guard raised the intended visible error from both `BugEventManager.preGameStart` and `CvEventInterface.onLoad`.
  - Change in: [`SASDefineGuard.py`](/Assets/Python/SASDefineGuard.py).
  - See [KI#38.4](/_1_AdvCiv-SAS/Docs/README_Known_Issues.md#ki-38.4) and the [Quick Install Guide](/_1_AdvCiv-SAS/Docs/README_Quick_Install_Setup_Guide.md#place-a-shortcut-of-the-mod-on-your-desktop).
- **Lazy init and cache SAS defines** (e.g., in advisors) to `None` and then to their actual value once if cache is `None`.
  - This prevents and **empirically fixes** direct caching in init, that empirically causes **python-like crashy behaviour** (i.e. similar behaviour to when python code is changed during runtime) on XML SAS define change of a value (instead of no effect and requiring a restart for the change to take effect).
  - See for example [CvMilitaryAdvisor.py](/Assets/Python/Screens/CvMilitaryAdvisor.py)'s `initDefines` method.
  - Possibly similar issues and crashes encountered when early caching sevopedia playground's background and cache.
  - See [KI#128](/_1_AdvCiv-SAS/Docs/README_Known_Issues.md#ki-128).
- **Fail-fast XML lookup hardening:**
  - replaced almost all `getInfoTypeForString(...)` calls (terrain/features/bonuses and similar tags) with `getInfoTypeOrFail(...)` (in [SASUtils.py](/Assets/Python/SASUtils.py)), so **missing/renamed XML tags raise explicit errors** instead of silently using invalid `-1` values.
  - Notably allowed to detect and fix incorrect `getInfoTypeOrFail("DIRECTION_NORTH")` instead of correct `DirectionTypes.DIRECTION_NORTH` (in Planet Generator map), or incorrect `getInfoTypeOrFail("PLOT_PEAK")` failing instead of correct `PlotTypes.PLOT_PEAK` (in Peirce map).
  - Also added `findInfoTypeNumOrFail(...)` for older `CvUtil.findInfoTypeNum(...)` callsites that need getter/count function.
  - Note: now also double-checked by a GitHub workflow check which now blocks new raw static/literal Python info-type lookups unless they are explicitly listed as dynamic/guarded legacy exceptions.
- **Static define-key audit:**
  - added [`audit_define_keys.py`](/LLM_Helpers/audit_define_keys.py), which statically greps every literal `getDefine{BOOL,FLOAT,INT,STRING}("X")` call across Python and DLL and verifies `X` is declared in mod XML, base BTS XML, or vanilla Civ4 XML.
  - Catches **per-key drift** (renamed/removed XML entries) before runtime.
  - A runtime fail-loud wrapper isn't practical here because `getDefine*` returns `0`/`""` for both missing and legitimately zero/empty keys (unlike `getInfoTypeForString` which has a clean `-1` sentinel); a static grep is also **cheaper and simpler to maintain** than wrapping every call site.
  - See [LLM_Helpers (README.md)](/LLM_Helpers/README.md).
- **Unused-text-key audit + cleanup:**
  - added [`audit_unused_text_keys.py`](/LLM_Helpers/audit_unused_text_keys.py), which flags mod GameText `<Tag>TXT_KEY_*` entries referenced **nowhere** in the mod, base BTS, or vanilla Civ4 (Python + DLL + XML); dynamically-built (`"TXT_KEY_BUG_OPT_"+id`) and engine-derived (`_PEDIA/_HELP/...`) key families are routed to a separate review bucket to avoid false positives.
  - Used it to **remove confirmed dead GameText entries** from [AdvCiv-SAS_main.xml](/Assets/XML/Text/AdvCiv-SAS_main.xml).
  - See [LLM_Helpers (README.md)](/LLM_Helpers/README.md).
- **C++ signature cleanup:**
  - collapsed **safe multiline C++ function declarations/definitions** to one physical line across the DLL source, making **signatures easier to grep, compare, and review** in diffs/LLM-assisted passes.
  - Done with the help of the new [`collapse_cpp_signatures.py`](/LLM_Helpers/README.md#collapse_cpp_signaturespy), a **conservative** signature-only helper, which intentionally skips ordinary multiline calls and unsafe comments, while also supporting traceable hoisting for reviewed whole-line comments inside signatures; review its output as a mechanical source-cleanup tool rather than a general C++ formatter.
  - It was empirically idempotent after the first full DLL pass (rerunning reported no safe C++ signature collapses found).
  - This is especially useful for **overloaded functions** like `void addMessage`, or e.g., `void CvDeal::endTrade(TradeData trade, PlayerTypes eFromPlayer, PlayerTypes eToPlayer, bool bTeam, /* advc.036: */ bool bUpdateAttitude, PlayerTypes eCancelPlayer) // advc.130p`, instead of its old three-line equivalent.
- **C++ inline-return cleanup:**
  - collapsed simple inline DLL header functions whose bodies were exactly one `return` statement to one physical line, making trivial getters/wrappers easier to grep and compare.
  - Done with the help of [`collapse_cpp_inline_returns.py`](/LLM_Helpers/README.md#collapse_cpp_inline_returnspy), a conservative mechanical helper that skips constructors/destructors, multi-statement bodies, body comments, multiline signatures, and overlong results.
  - For example, `int getGoldPerTurnByPlayer(PlayerTypes ePlayer) const { return m_aiGoldPerTurnByPlayer.get(ePlayer); } // Exposed to Python`, instead of its old four-line equivalent.
- **Build cleanup - disabled unneeded DLL test code in normal builds:**
  - compile-gated disabled reproducibility-test code (`ReproTest.cpp`) so normal DLL builds no longer compile it unless explicitly enabled for debugging.
  - This keeps dormant test/debug code out of normal builds, may slightly reduce DLL/build size, and does **not** change gameplay.
  - Found with the help of the new [`find_cpp_dead_code_candidates.py`](/LLM_Helpers/find_cpp_dead_code_candidates.py) LLM/manual-review helper.
- **Civ4 GameText character check:**
  - added a GitHub workflow check for Unicode typography that creates visible artifacts in Civ4 text, such as the em dash (`U+2014`) formerly used in the Automobiles Civilopedia instead of a simple hyphen or other ASCII punctuation.
  - It also catches repeated question marks that indicate lost characters, while allowing readable accented letters and ignoring XML comments.
- **Technology audio check:**
  - added a GitHub workflow check that resolves every technology's normal and multiplayer audio through the mod-local audio tables and ensures each technology keeps a distinct spoken recording.
  - This catches missing local definitions that otherwise produce quote text without audio, as happened with Drama, as well as wrong-layer script/sound references.
- **Performance & code cleanup:** Broad Python/XML/C++ optimizations (e.g., removed giant ConfigObj docstring (see [KI#21.5](/_1_AdvCiv-SAS/Docs/README_Known_Issues.md#ki-21.5)); cached hot values via `static const`/hoisting; normalized inconsistent asset names (see for example [KI#18](/_1_AdvCiv-SAS/Docs/README_Known_Issues.md#ki-18))), **map-type heaviness caching** (`CvGame::recomputeMapnameHeaviness`) so land/naval checks run once per game (feeds UNITAI tuning) (see [KI#42](/_1_AdvCiv-SAS/Docs/README_Known_Issues.md#ki-42), **land/water bonus-specific builds/improvements caching** (`CvPlot::SAS_getBonusSpecificBuild`), [AI Personality Panel performance optimization notes](/_1_AdvCiv-SAS/Docs/README_AI_Personality_Panel.md#notes-about-performance-optimization-of-the-ai-personality-panel-caching)), **feature-level caching** (AI Personality Panel), refactor **extremely redundant code** (e.g. `refreshUnitSelection` in [CvMilitaryAdvisor.py](/Assets/Python/Screens/CvMilitaryAdvisor.py)), and other changes.
- **Logging pre-gating:**
  - logging is generally pre-gated and diagnostic-only values are computed only when the relevant logging is enabled, so ordinary non-logging gameplay has minimal diagnostic overhead, ideally none.
  - (Requires AdvCiv-SAS 6584+) For example, **`AI_buildingValue` hot-path / diagnostic cleanup:** the authoritative construction-value cache is checked before gathering unrelated food, health, trade, rank and domain-throughput context, so cache hits stay cheap.
  - Expensive Building Production level-3 strings/context are also pre-gated behind the disabled-by-default log level.
  - See [KI#48.9](/_1_AdvCiv-SAS/Docs/README_Known_Issues.md#ki-48.9) and [KI#48.15](/_1_AdvCiv-SAS/Docs/README_Known_Issues.md#ki-48.15).
- (Requires AdvCiv-SAS 6584+) **Shared enemy-power and local-rival context:**
  - `SASWarPowerContext` centralizes the SAS strong/weak enemy-power thresholds and aggregate current/chosen-enemy power/current-war classifications across city, player, team and unit AI, while deliberately keeping player focus-war, team war preparation, area land-war and city-local danger separate because they answer different strategic/tactical questions.
  - It also preserves the special `0%` no-applicable-enemy result instead of accidentally treating it as proof that an AI is overwhelmingly stronger.
  - `SASLocalAreaRivalContext` similarly centralizes known/unknown independent rival blocs/cities plus our, combined-local and strongest-local rival power: KI#53.5 land-army saturation compares against the **combined** local burden, while KI#48.20 World-Wonder exposure asks whether **one strongest local rival** can still punish discretionary investment.
  - Both refactors were validated as behavior-neutral before being used as shared foundations: the war-power pass matched **505/505 core-state**, **505/505 authoritative-RNG** and **59/59 run-status** checkpoints; the local-rival pass matched **365/365 core-state**, **365/365 RNG** and **42/42 run-status** checkpoints.
  - See [KI#53.5](/_1_AdvCiv-SAS/Docs/README_Known_Issues.md#ki-53.5) and [KI#48.20](/_1_AdvCiv-SAS/Docs/README_Known_Issues.md#ki-48.20).
- **Found-value** evaluation now uses `int` end-to-end (CitySiteEvaluator, AIFoundValue, CvPlot cache, SPI, Python callback), not `short`.
  - Why: **avoid overflow/underflow** and short wraparounds in site scoring; keep the logic the same while **improving reliability**.
  - Expectation: more robust site values, fewer edge-case penalties/boosts from short overflow; no known negative gameplay change.
  - Performance: `int` is typically at least as fast as short on modern CPUs; no slowdown expected.
  - Tested: autoplay ran fine.
  - See [Change from short to int the Found value pipeline](/README.md#change-from-short-to-int-the-found-value-pipeline).
- Removed the old clunky Base AdvCiv's `CONCEPT_TRAIT` XML clutter. Now using the more efficient `WIDGET_PYTHON` similarly instead.
- **Avoid** some **hardcoded values**; e.g., `szButton = "Art/Interface/Buttons/Civilizations/Barbarian.dds"` in Advisors UI code replaced with `szButton = gc.getCivilizationInfo(player.getCivilizationType()).getButton()`; adding a new `<Button>` field for eras.
- **Centralize magic numbers** in [`SASMagicNumbers.py`](/Assets/Python/SASMagicNumbers.py), an import-free leaf helper for named constants used across screens, Sevopedia, mapscripts, and map generator utilities.
  - Examples: Sevopedia/Tech Chooser `WIDGET_PYTHON` data1 IDs and Sevopedia Movie/Music packed-key type IDs.
  - This avoids duplicated definitions, widget-id drift/collisions, and circular-import/load-order risk.
- (Requires AdvCiv-SAS 6347+) **Shared diagnostic provenance/time helpers:**
  - `CvGameCoreUtils` now centralizes canonical mod-context fields, exact loaded-DLL provenance fields, UTC timestamp formatting, millisecond UTC formatting, and the monotonic elapsed-millisecond helpers used by SAS diagnostics.
  - Unrelated core timing remains deliberately separate: Civ4 player-time/RNG uses and the high-resolution profiler keep their existing specialized clocks rather than being coupled to logging helpers.
- (Requires AdvCiv-SAS 6348+) **Shared diagnostic serialization and finalized-initial-state helpers:**
  - further centralized parser-safe narrow/wide text quoting and escaping, empty-value/list formatting, diagnostic filename construction, team-ID text, diplomacy trade-item/list serialization, and the authoritative initial team/technology/deal field builders in `CvGameCoreUtils`.
  - BBAI, SASGameRecord, and relevant diplomacy logging now reuse the same semantics instead of maintaining recorder-specific copies.
  - Superficially similar enum/type helpers with different `NO_*` meanings remain separate rather than being mechanically unified.
  - Logging/refactor only; no gameplay-decision change.
- **Shared DLL diagnostic metadata:** added `WarDeclarationCause` (as of now `WAR_DECLARATION_DIRECT`, `WAR_DECLARATION_GAME_SETUP`, `WAR_DECLARATION_ALWAYS_WAR`, `WAR_DECLARATION_PERMANENT_ALLIANCE`, `WAR_DECLARATION_DEFENSIVE_PACT`, `WAR_DECLARATION_VASSAL_ALIGNMENT`, `WAR_DECLARATION_DIPLOMACY`, `WAR_DECLARATION_VOTE`, and `WAR_DECLARATION_NUCLEAR_ATTACK`), `getSASWarDeclarationCause`, and `getSASMemoryType` for SASGameRecord war and diplomacy rows.
- Similarly added `TechAcquisitionCause` (`TECH_ACQUISITION_UNKNOWN`, `TECH_ACQUISITION_RESEARCH`, `TECH_ACQUISITION_TRADE`, `TECH_ACQUISITION_FREE_CHOICE`, `TECH_ACQUISITION_GOODY`, `TECH_ACQUISITION_GREAT_PERSON`, `TECH_ACQUISITION_ESPIONAGE`, `TECH_ACQUISITION_RANDOM_EVENT`, `TECH_ACQUISITION_TECH_SHARE`, `TECH_ACQUISITION_TEAM_MERGE`, `TECH_ACQUISITION_INHERITANCE`, `TECH_ACQUISITION_ADVANCED_START`, `TECH_ACQUISITION_BARBARIAN_RESEARCH`, `TECH_ACQUISITION_GAME_SETUP`, and `TECH_ACQUISITION_DEBUG`) and `getSASTechAcquisitionCause` so technology-acquisition rows can identify how each technology was obtained.
  - Logging only; no gameplay change.
- Added `getSASGameType` and `getSASGameMode` for shared raw names of the existing `GameType` and `GameMode` enums. Added shared `getSASCalendarType` for calendar context. Logging only; no gameplay change.
- (Requires AdvCiv-SAS 6584+) **Canonical AI diagnostic descriptors:**
  - when the same fixed AI state family needs both complete iteration and stable machine-readable names, AdvCiv-SAS increasingly uses one typed descriptor table as the source of truth instead of separate preprocessor macros, numeric-range scans, name switches and CI copies that can drift apart.
  - `AIStrategy` is the shared/codebase-wide example: AdvCiv-SAS first added canonical raw strategy names in 6481, and 6584 consolidates every strategy value/name into `SASAIStrategyDescriptor`, reused by SASGameRecord state hashes/checkpoints/transitions, the strategy UI and CI; the obsolete `MAX_AI_STRATEGIES`/lowest-to-highest-bit assumptions are no longer needed.
  - (Requires AdvCiv-SAS 6584+) While adding SASGameRecord revision 130, the inherited local `BUILDINGFOCUS_*` macros were similarly replaced with a typed `BuildingFocusTypes` bitmask plus one local descriptor table for diagnostic iteration/names, preserving every inherited bit and composite focus behavior.
    - Raw diagnostic names remain separate from translated/player-facing labels.
    - These are logging/maintenance refactors rather than AI-policy changes, and building-focus formatting remains behind level-3 diagnostic pre-gating.
- (Requires AdvCiv-SAS 6486+) Added shared `getSASAreaAIType` for canonical machine-readable names of the existing `AreaAITypes` theater-posture enum so SASGameRecord and other AI diagnostics can use stable raw names instead of opaque integers.
  - The SASGameRecord build check guards the enum-name mapping.
  - Logging/refactor only; no AI-decision change.
- (Requires AdvCiv-SAS 6490+) Added shared `getSASContactType` for canonical machine-readable names of the existing `ContactTypes` diplomacy-contact enum, keeping proactive-contact diagnostics readable without recorder-local copies of the gameplay vocabulary. Logging/refactor only; no AI-decision change.
- Added `SASAutoPlayEndCause` (`SAS_AUTOPLAY_END_UNSPECIFIED`, `SAS_AUTOPLAY_END_SCHEDULED`, `SAS_AUTOPLAY_END_USER_INTERRUPTED`, `SAS_AUTOPLAY_END_VICTORY`, `SAS_AUTOPLAY_END_ACTIVE_PLAYER_DEFEATED`, `SAS_AUTOPLAY_END_DESYNC`, `SAS_AUTOPLAY_END_ASSERTION`, `SAS_AUTOPLAY_END_RISE_FALL`, and `SAS_AUTOPLAY_END_OTHER`) and shared `getSASAutoPlayEndCause`, also exposed to Python so the AI Auto Play controller can report why a run stopped.
  - Autoplay control/logging only; no AI or gameplay-decision change.
  - See [KI#203](/_1_AdvCiv-SAS/Docs/README_Known_Issues.md#ki-203).
- **Cache once per session precomputed** Sevopedia Traits' Charts and Sevopedia Improvements' Weights (Leaders) Chart on category open before any entry is clicked on.
  - This uses a simplified version of the AI Personality Panel's caching approach.
  - See [AI Personality panel caching](/_1_AdvCiv-SAS/Docs/README_AI_Personality_Panel.md#notes-about-performance-optimization-of-the-ai-personality-panel-caching).
- **Load Precomputed Files**: optionally load the Sevopedia Leader's **AI Personality Panel**'s information to display as a **predumped** file ([SevoPediaLeaderCachePredumped.py](/Assets/Python/Contrib/Sevopedia/SevoPediaLeaderCachePredumped.py)): (~550k Python literal characters) (default ON, recommended): since these values never change for players, this is more efficient.
  - For modders, CI now checks that the committed predump still matches leader XML/shared AIP display logic; if it fails, `aip_predump_values.py --write` can refresh the file outside Civ4.
  - This checker notably caught 394 stale label entries, such as `Build Unit (52)` missing the updated `Build Unit % (52)` text and then helped generate an updated predump.
  - The old manual in-game `PythonDbg.log` dump path remains a manual fallback or alternative way of refreshing the predump.
- Refactor to **helper files** reusable logic; e.g., debug dropdown making, place legend link, advisor footer tabs, or geometry for advisors [SASUtils.py](/Assets/Python/SASUtils.py); expanded text and animation panels or draw none text ([_sevopedia_helpers.py](/Assets/Python/Contrib/Sevopedia/_sevopedia_helpers.py)), and many others.
- **Sortable icon-only table cells**: for actual image-icon cells where visible text is empty, add an invisible `<font=1>` sort key using **zero-width Unicode marks** `U+200B`, `U+200C`, `U+200D`, `U+200E`, and `U+200F`.
  - **Sevopedia charts** and **in-category chart icon helpers** use `chart_sort_key` in [_sevopedia_helpers.py](/Assets/Python/Contrib/Sevopedia/_sevopedia_helpers.py); **advisors** use `getAdvisorIconSortKey` in [SASUtils.py](/Assets/Python/SASUtils.py), reused for example by the Military Advisor Battles tab and Info Screen Score Tab leader/civ/research image cells.
  - This is intentionally **only for separate icon-button** cells: **GameFont chars** such as religions, attitudes, trait icons, trade, war/peace, etc. **already sort as text**.
  - The hardcoded `font=1` is intentionally not upscaled because the key must stay invisible and must not affect row height.
- Fix/cleanup **incorrect char formatting**: e.g.; `"Berang�r"` -> `"Beranger"`; removed non-English hints.
- Consolidated **long, unneeded, or unused comments** into (Requires AdvCiv-SAS 6594+) [Long_Comments/](/_1_AdvCiv-SAS/Docs/Long_Comments/) and replaced them with short custom markers or files to keep code clean, lean, and readable. > ~500k characters reduced and still ongoing.

### Logging

#### Logging (Common)

- (Requires AdvCiv-SAS 6478+) **Disabled diagnostic logging is kept lightweight:**
  - after SASGameRecord and BBAI expanded across many more gameplay/AI paths, their effective XML log levels are cached once after all GlobalDefines/module overrides finish loading.
  - The widespread runtime gates are therefore direct cached reads rather than repeated out-of-line getter calls, while logging-only scans, snapshots, formatting, and other arguments remain pre-gated where relevant.
  - Enabled diagnostic output is unchanged; ordinary games with these logs disabled avoid paying for the added diagnostic coverage.
- (Requires AdvCiv-SAS 6598+) **Diagnostic caller pre-gating is stricter:**
  - independent BBAI/SASGameRecord compound guards lead with the cached logging gate so disabled categories short-circuit before unrelated diagnostic predicates, while existing outer gates continue to skip whole logging-only loops/setup without redundant inner level checks.
  - This is a disabled-logging performance/readability cleanup only; enabled diagnostic output and log-level semantics are unchanged.
- (Requires AdvCiv-SAS 6599+) **Explicit diagnostic log guards are easier to grep:**
  - simple one-statement guards keep the actual BBAI/SASGameRecord/domain logging pre-gate and log-call head on the same line, while long argument tails remain multiline.
  - Explicit guards distinguish logging requirements from gameplay conditions, making diagnostic coverage easier to inspect. AI decisions and gameplay conditions are preserved.
- (Requires AdvCiv-SAS 6567+) **Literal percent signs in enabled BBAI/SASGameRecord logging no longer corrupt rows or intermittently crash the game:**
  - Civ4's EXE logger interpreted already-formatted diagnostic lines as printf text again, so a valid fragment such as `100% second=` could consume an unrelated integer as a string pointer.
  - One old run completed despite corrupting all 5,976 affected candidate rows and writing no literal percent bytes, while another crashed during Settler evaluation.
  - BBAI and SASGameRecord now share a final emission boundary that safely preserves literal `%`; a fresh Debug-opt smoke autoplay preserved all 1,293 tested candidate rows and completed normally.
  - Structured SASGameRecord fields continue using explicit `Percent`/`X100` names where appropriate.
  - Disabled logging incurs no added gameplay work.
  - See [KI#375.3](/_1_AdvCiv-SAS/Docs/README_Known_Issues.md#ki-375.3).
- (Requires AdvCiv-SAS 6347+) **BBAI and SASGameRecord share canonical mod/DLL provenance:**
  - both diagnostic logs now place the same escaped mod context and exact loaded-DLL context immediately after the new/load lifecycle row and before log settings/game activity.
  - Shared helpers in `CvGameCoreUtils` provide the canonical fields instead of separate recorder-specific implementations.
  - The DLL context records the nmake build configuration (`Debug`, `Debug-opt`, `Release`, `Assert`, `Profile`, or `Final_Release`), assertion/debug defines, exact DLL byte size, last-write UTC time, PE/linker timestamp, and a cached FNV-1a 64-bit fingerprint of the actually loaded `CvGameCoreDLL.dll`.
  - This notably lets BBAI AI-decision traces and SASGameRecord run-state logs be tied to the same local candidate binary, including same-source/dirty rebuilds.
  - The fingerprint is a diagnostic binary identifier, not a cryptographic security hash; Git/source version identity and possible XML/Python/non-DLL content identity remain separate provenance concerns.
  - DLL fingerprinting is runtime-gated and cached at most once per Civ4 process.
  - (Requires AdvCiv-SAS 6459+) A later KI#53.6 validation demonstrated this directly: an intended enabled naval-AI run accidentally reused the pre-cure DLL, and the unchanged `FNV1A64:6838100B3CC2C572` fingerprint plus the absence of the new candidate-only BBAI field exposed the stale binary; all 360,396 targeted naval-war diagnostic rows were byte-for-byte identical to the control.
    - The corrected run loaded `FNV1A64:5F1E6780C73485F1` and exercised the candidate normally.

#### BBAI log

- (Requires AdvCiv-SAS 6607+) **BBAI categories share one maintenance registry:**
  - One category list connects XML log-level defines to their cached settings, keeping startup loading, enabled-category detection and the recorded diagnostic profile consistent when categories are added or changed.
  - Runtime logging gates retain their direct cached reads, and CI recognizes the registry as real define usage only when its runtime lookup is present.
- **BBAI logging is now XML-tunable:**
  - BBAI AI-decision logging can now be enabled/disabled and adjusted through **SAS defines**, rather than editing and recompiling the DLL as in base AdvCiv.
  - This is deemed valuable **not only for modders, but also for users** who can now view or generate `BBAI.log` files for review.
  - Note: `LoggingEnabled=1` in `CivilizationIV.ini` is still required for Civ4 to write.
  - This allowed for example to surgically identify and fix the root cause of a city not producing its first settler until ~T90 at normal game speed, which is way too late (`bWarPlan` was too aggressively gating settler production; now it only blocks settler production after early expansion has reached enough cities (inefficient at that point, better switch strategy)).
  - Note: BBAI logs can be very long; for broad raw-log diagnosis, we usually give to an external LLM/AI such as ChatGPT the log file, as it is as of now agentic-token-free analysis (also good to give us a different point of view/review if needed).
- (Requires AdvCiv-SAS 6600+) **Clearer AI city-site diagnostics (KI#505.2):**
  - For players and modders investigating Settler choices, Found logs now explain the evaluations the AI actually uses. Scores recalculated for hypothetical comparisons are labeled separately, so they are easier to distinguish from the original decision. For example, inherited `IFLOG` replay controls were replaced with explicit logging checks, and the unused AdvCiv `CvPlot::debugStr()` helper was retired after its descriptions exposed actual bonus/owner identities in player-known traces.
  - Choose level 1 for brief selection/founding records, level 2 for numeric candidate-score breakdowns, or level 3 for detailed plot, resource and improvement explanations. Settling rules are preserved.
  - Related plot details are combined into structured rows, but levels 2/3 still produce very large logs over full autoplays. Use their extra detail when investigating AI behavior; SASGameRecord provides compact whole-game context. See [KI#505.2](/_1_AdvCiv-SAS/Docs/README_Known_Issues.md#ki-505.2) for implementation history, measured log sizes and test results.
- (Requires AdvCiv-SAS 6604+) **UWAI diagnostics now use the normal caller-pre-gated BBAI infrastructure:**
  - AdvCiv's separate `UWAIReport.cpp`/`.h` logger and `uwai<turn>.log`/Textile output are retired, consolidating AI diagnostics into one logging system. Agent, Military Analyst, Invasion Graph, Armament Forecast and War Utility diagnostics have independent cached `SAS_BBAI_UWAI_*_LOG_LEVEL` controls and write into the same timestamped BBAI log as other AI diagnostics.
  - Disabled categories are checked at their callers before logging-only arguments, string formatting or report-only setup are evaluated; this notably removes inherited muted-call overhead from the very hot War Utility diagnostic path.
  - The replacement `UWAILogMuteState.h` stores only nested mute depth for hypothetical sub-evaluations, so speculative alternatives can stay silent instead of being confused with the selected scenario.
  - Logging-only selected-scenario reruns are observation-only and cannot supply gameplay utility or use WarEvaluator caches. Enabling diagnostics is intended to preserve AI decisions and synchronized RNG, so investigating the AI does not change the behavior being investigated; full-run comparisons are documented in KI#505.3.
  - (Requires AdvCiv-SAS 6606+) UWAI output uses stable uniquely greppable `UWAI_*` event identifiers and explicit fields rather than the inherited report's narrative prose. Closely related calculation stages are consolidated into one result row where practical, while genuine one-to-many details remain separate.
  - (Requires AdvCiv-SAS 6606+) Enable only the UWAI categories needed for an investigation: level 1 provides compact outcomes, while higher levels expose more calculation and simulation detail. Level 3 can produce very large full-autoplay logs; the final all-five-category validation produced about 2.50 GiB (2,681,295,993 bytes).
  - Normal defaults remain disabled. See [KI#505.3](/_1_AdvCiv-SAS/Docs/README_Known_Issues.md#ki-505.3) for implementation history and validation.
- (Requires AdvCiv-SAS 6479+) **Deeper disabled-diagnostic call-site cleanup:**
  - shared gameplay/logging helpers now keep BBAI-only enrichment behind the relevant category gate instead of carrying it through ordinary AI work.
  - Notably, Settler escort scans skip diagnostic unit classification/defender-need/list formatting unless requested, player-turn logging is not entered while PLAYER logging is disabled, AI production avoids several logging-only snapshots/name/value preparations, Worker Phase-0 logging retains only a cheap pre-mutation feature ID until the enabled log point, and Great Person weighting no longer constructs eight level-3 diagnostic maps when CULTURE logging is off.
  - Enabled log semantics are unchanged; this pass removes diagnostic bookkeeping rather than AI/gameplay evaluation.
- **Timestamped BBAI log files:**
  - Optionally log each new game and loaded save to a **new UTC-timestamped file** (e.g., `BBAI_20260705T071718Z_new1.log` or `BBAI_20260705T071718Z_load2.log`) instead of expanding the existing `BBAI.log`, which was very tedious to clean up or identify/store/read/review/upload, and so repeated save-file tests no longer require restarting Civ4.
  - Each log begins notably with the new/load time, currently active BBAI log levels, turn/year, map and game settings (including `landheavy` and `navalheavy` actual game information), player counts, and RNG states, avoiding mixed logs and the need to restart Civ4 between repeated save-file tests.
  - See also [README.md (BBAI logging and head example)](/README.md#bbai-logging-and-head-example).
- (Requires AdvCiv-SAS 6603+) **BBAI helps compare founded city sites with alternatives:**
  - Found level 2+ summarizes other shortlisted sites and coastal sites with known water resources; Barbarian diagnostics show the best eligible alternative one or two plots away. This helps investigate whether a nearby or coastal location looked stronger.
  - Summaries identify cached shortlist values separately from evaluated Barbarian scores, so older cached values are not mistaken for fresh comparisons. They reuse available scores instead of repeating site evaluations. See [KI#505.2](/_1_AdvCiv-SAS/Docs/README_Known_Issues.md#ki-505.2) for the diagnostic scope and limitations.
- (Requires AdvCiv-SAS 6602+) **Settler diagnostics are easier to search and compare:**
  - First-city scouting, candidate summaries and ordinary Settler actions use stable `FIRST_CITY_*` / `SETTLER_*` event prefixes with named fields instead of prose-only messages.
  - Duplicate target-only prose rows are removed where the structured mission row already preserves that information; log levels and AI decisions remain unchanged.
- **BBAI worker log split:**
  - added separate worker and Work Boat / `UNITAI_WORKER_SEA` BBAI log-level defines so ordinary `SAS_BBAI_UNIT_LOG_LEVEL` can focus on non-worker unit behavior such as scrapping, army movement, and combat-unit decisions.
  - Worker and worker-sea logs had become noisy enough to hide those unit decisions, and Work Boats use distinct logic from land workers, so they now have their own category as well.
- **BBAI Settler log split:**
  - added `SAS_BBAI_SETTLER_LOG_LEVEL`, separated from UNIT so Settlers and other units can be inspected independently.
  - This notably covers city-site decisions, Settler movement, scouting, returns, founding, parking reasons, danger context, and city escort pools; FOUND as of now separately controls detailed city-site scoring calculations.
  - This notably lets UNIT investigate other unit behavior without enabling the multi-megabyte first-city Settler traces needed for example for the Berlin, Karakorum, and Cuzco regression tests.
- (Requires AdvCiv-SAS 6569+) **Auditable Settler decisions:**
  - dedicated BBAI Found/Settler diagnostics now expose each candidate's component score, top XML-valid improvement outcomes, home-plot opportunity, defense, distance shaping, rejection reason, seafood alternatives, known versus omniscient value, and best-1/2/3/6/10/14 workable-plot distribution.
  - Movement rows distinguish local rechecks, candidate eligibility, scouting directions, cached-site comparisons, returns, escort/security branches, and final founding.
  - The expensive omniscient and string-rich detail is level-3-only and pre-gated, so disabled diagnostics do not add it to ordinary gameplay.
- **Overseas-transport BBAI diagnostics:**
  - added `SAS_BBAI_OVERSEAS_TRANSPORT_LOG_LEVEL`, separated from CITY and UNIT so overseas settlement and conquest logistics can be inspected without enabling those broad logs.
  - It notably connects known valuable island sites and cities with their distance and defenders, map type, transport production and cargo capacity, loaded units, units actually available to board, incoming cargo already assigned within 1/3/7 turns, pickup/loading failures, launch thresholds and decisions, naval reachability, and Settler/Worker cargo handling.
- **BBAI War diagnostics:**
  - added `SAS_BBAI_WAR_LOG_LEVEL`, separated from TEAM so UWAI war-target choice can be inspected without broad team/diplomacy logs.
  - It notably records target utility, limited-vs-total war comparison, naval/preparation context, forced peace, attitude, closeness, city distance, power, selected target, target switches, direct-war adoption, war-plan lifecycle changes, and optional high-utility scenario/aspect breakdowns, which should help investigate problematic faraway wars before changing behavior.
- **Great General BBAI diagnostics:**
  - added `SAS_BBAI_GREAT_GENERAL_LOG_LEVEL`, separated from UNIT notably so Military Academy construction, Military Instructor joining, Great General unit attachment, target-city choice, and target-unit choice can be inspected together without enabling broad unit logs.
  - This notably helped isolate a bad France sample where `AI_join` rejected all cities for safety/path reasons, then the plot-danger branch attached a Great General to a low-value Longbowman.
- **Culture-victory BBAI diagnostics:**
  - added a new dedicated `SAS_BBAI_CULTURE_LOG_LEVEL` notably for diagnosing premature, uneven, or wasteful AI culture investment without enabling broad player/city logs.
  - It records for example culture-stage reasons, current/projected Legendary-city countdowns and rival ranking, competing victory progress, actual culture-slider decisions, per-city weights and production, and culture contributed by production processes and assigned specialists.
- **Citizen-allocation BBAI diagnostics:**
  - added a dedicated `SAS_BBAI_CITIZEN_LOG_LEVEL` notably for finding worked plots or specialists that the AI's full city-aware comparison would replace, but its faster preliminary selection overlooked.
  - At the highest level, it also reports suspicious worked/unworked plot pairs under a simple food-first yield check.
- **Evacuation and retreat BBAI diagnostics:** added `SAS_BBAI_EVACUATION_LOG_LEVEL`. It notably helps investigate evacuation/retreat behavior without broad city or unit logging, for doomed cities, workers, and naval units.
- **BBAI logging guard cleanup:**
  - now that BBAI logging is runtime-tunable rather than compile-gated, some older logging-only code paths are **guarded more explicitly** through the relevant log-level defines.
  - This avoids running logging passes or expensive/unneeded evaluation and formatting of **log parameters** that ultimately won't be used when that log category is disabled.
- (Requires AdvCiv-SAS 6455+) **Dedicated no-production BBAI logging:**
  - `SAS_BBAI_PRODUCTION_NO_TARGET_LOG_LEVEL` isolates KI#51 investigation from broad city and military-production logging.
  - Level 2 records any civilization city that reaches its control-path-aware turn boundary without a production target outside disorder; level 3 adds legal units/buildings/Projects/Processes and strategic context, plus exact AI chooser reasons (`EXPECTED_DISORDER` versus an abnormal `FINAL_FALLTHROUGH`).
- **Dedicated AI military-production BBAI logging**: added `SAS_BBAI_MILITARY_PRODUCTION_LOG_LEVEL` notably to isolate production diagnostics without enabling broad city logging.
  - Higher detail notably records unit-build probability, spending limits, building/nonunit gates, unit selection and rejection reasons, and (Requires AdvCiv-SAS 6584+) proactive-fortification decisions.
- (Requires AdvCiv-SAS 6584+) **Dedicated building-production BBAI forensic audit:**
  - AdvCiv-SAS adds `SAS_BBAI_BUILDING_PRODUCTION_LOG_LEVEL` to link candidate value, contextual policy and the production target ultimately selected.
  - Level 3 notably records exact before/delta attribution for happiness/anger, health and unhealthy-food relief, food-kept growth, maintenance, trade, yields/commerce, conventional/nuclear defense, `iAIWeight`/flavor, generic and domain military production/XP, domain applicability, Wonder placement/race/continuation, Government Centers, Palace relocation, National Park pressure and proactive fortification.
  - (Requires AdvCiv-SAS 6584+) The first broad exact-component audit produced about **198,700 attribution rows**, so this deliberately remains an opt-in forensic microscope; expensive scans/formatting are gated behind the log level and diagnostic evaluation adds no chooser RNG.
    - See [KI#48.5](/_1_AdvCiv-SAS/Docs/README_Known_Issues.md#ki-48.5) and [KI#48.9](/_1_AdvCiv-SAS/Docs/README_Known_Issues.md#ki-48.9); see also [KI#880](/_1_AdvCiv-SAS/Docs/README_Known_Issues.md#ki-880) for an inherited happiness-arithmetic defect and [KI#821](/_1_AdvCiv-SAS/Docs/README_Known_Issues.md#ki-821) for construction-value cache-lifetime/observer-effect correctness exposed by the same audit.
- (Requires AdvCiv-SAS 6448+) **Dedicated AI space-production BBAI logging**: added `SAS_BBAI_SPACE_PRODUCTION_LOG_LEVEL` category because SASGameRecord could identify the strategic outcome and production switches but could not explain the city-level project valuation and branch that displaced them.
  - Level 2 as of now records invested spaceship reevaluation/release/final-target transitions and selected projects; level 3 as of now additionally records every spaceship candidate's legality, production-rank/fit decision and the space-specific project value before and after victory-stage, area-war and production-capacity adjustments.
- (Requires AdvCiv-SAS 6449+) **Dedicated AI one-copy Project-production BBAI logging:**
  - added `SAS_BBAI_LIMITED_PROJECT_PRODUCTION_LOG_LEVEL` for Manhattan, Internet, SDI and Apollo after SASGameRecord exposed large amounts of stored Project production being parked and later invalidated.
  - Level 2 records reevaluation, release/final-target transitions and selected limited Projects; level 3 additionally records candidate legality, production rank/fit, stored production, team-making counts and final candidate value.
  - This complements SASGameRecord's exact production-loss history by explaining the city-level decision that caused a Project to move.

#### SASGameRecord log

(Requires AdvCiv-SAS 6440+) Note: `SASGameRecord` log has its own revision system, see [`README_SASGameRecord_Revisions.md`](/_1_AdvCiv-SAS/Docs/README_SASGameRecord_Revisions.md) for details and miscellanous information.

- (Requires AdvCiv-SAS 6343+) **SASGameRecord setup/provenance is more reproducible and explicit:**
  - setup rows now notably record every stored map-script custom option, including hidden options, its numeric value, the currently installed script's default, whether the selection equals that default, and readable selected/default descriptions when the script is available.
  - Advanced Start records both the configured points and the default recomputed from the current start-era/world-size XML.
  - A separate war-AI settings row also resolves the active war/peace implementation (`UWAI` or legacy K-Mod), separately reports whether UWAI is full/background/disabled, and records only the few UWAI defines needed to explain that result; the ordinary `options=` field remains a compact list of enabled `GAMEOPTION_*` values rather than dumping disabled options or unrelated AI-tuning defines.
- Added a separate `SASGameRecord_*.log` for compact autoplay and AI-strength review.
  - It notably records high-level run context such as initial map/landmass geography and bonus/yield context, economy, expansion, city and battle history, worked plots, diplomacy, unit composition, game state, and in-game bonus appearances/disappearances, complementing BBAI logs by recording what happened before or alongside detailed AI-decision traces.
  - It also logs autoplay start/end/change rows, explicit player appearance/revival/elimination rows, and compact run-status rows with alive/eliminated players, current winner/victory if any, and top score/power players, so stopped autoplay logs no longer require inferring these from missing snapshots.
- (Requires AdvCiv-SAS 6551+) **Consumer/reproducibility context is clearer without bloating every run:**
  - opt-in level-3 display context now distinguishes multi-adapter/multi-vendor Windows systems so hybrid-GPU or driver/device-selection issues are easier to recognize, while structured text-map legend/layer rows repeat their turn for self-locating parser/grep slices.
  - The runtime log intentionally does not emit an exhaustive static manifest of every row type the build could produce; exact source/revision/settings identify the producer, while schema/parser guidance stays in source/docs/checkers.
  - See [`README_SASGameRecord_Revisions.md`](/_1_AdvCiv-SAS/Docs/README_SASGameRecord_Revisions.md#consumerparser-notes).
- (Requires AdvCiv-SAS 6552+) **Level-3 periodic city detail now uses bounded checkpoint-plus-delta encoding:**
  - the five largest repetitive city families keep full rows at session start and every tenth observation, while intermediate rows emit only changed fields with explicit reconstruction links.
  - New/load files restart the chains, a strict helper can restore the old full-row shape, and levels 0-2 are unaffected.
  - In the revision-123 autoplay, the encoded record was 53.81 MB versus 59.73 MB after expansion (about 9.9% smaller overall).
- (Requires AdvCiv-SAS 6441+) Every emitted structured `GAME_RECORD_*` row receives a session-local monotonic `seq`, preserving exact semantic-row order after filtering, chunking or database import without numbering raw ASCII-map drawing lines.
  - Generic causal `tx` scopes additionally make related rows explicitly joinable; city acquisition and razing are the first covered operation, including an immediate AI/automatic raze within the acquisition transaction.
  - These diagnostic identities do not affect gameplay or save state.
- (Requires AdvCiv-SAS 6442+) Level-3 records pair each authoritative-RNG lifecycle checkpoint with a compact semantic CORE-state checkpoint.
  - `recordRevision`-identified component fingerprints distinguish the first observed divergence in game, team, player, city, unit, selection-group, plot or deal state, including selected durable/latent AI and gameplay bookkeeping that ordinary human-readable snapshots do not expose.
  - The comparison helper validates these fingerprints, reports their measured computation cost and distinguishes RNG-first from deterministic/non-authoritative-RNG-first divergence; all scanning remains pre-gated behind enabled level-3 recording and does not affect gameplay or save state.
- (Requires AdvCiv-SAS 6443+) Exact level-3 `PLOT_OWNER_CHANGED` rows record chronological territory transfers at the authoritative plot setter, including old/new owner and team, immediate cause, culture pressure, contested second owner, forced-unowned state, resource/improvement/route context and the surrounding causal `tx` when available.
  - This makes border changes from city founding/acquisition/culture, war/peace, vassalage and direct Python/WorldBuilder actions explicit between periodic political maps; CORE plot fingerprints now also include contested second-owner and forced-unowned state.
- (Requires AdvCiv-SAS 6460+) Periodic `GAME_RECORD_NAVAL_ASSAULT_POSTURE` rows make overseas-force readiness visible without copying per-target UWAI/BBAI diagnostics into the game record.
  - They summarize `UNITAI_ASSAULT_SEA` transport stock/training, total and open-ocean-capable lift, actual cargo/utilization and cargo type/UnitAI composition, transport/group loading state, exact AI-base presence via `CvTeam::isBase`, MissionAI/queue/healing state, and grouped sea-combat support.
  - Mixed fleets are counted once per selection group and actual transport cargo is enumerated directly, including same-team passengers; no pathfinding, target search, UWAI reevaluation or RNG is added.
  - The canonical war-AI settings row also records `SAS_UWAI_INVASION_GRAPH_NAVAL_LOGISTICS_DEPLOYMENT_OPTIMIZE`, while detailed invasion-candidate reasoning remains in BBAI.
- (Requires AdvCiv-SAS 6468+) **Naval operational intent is more directly reconstructible:**
  - level-3 `GAME_RECORD_NAVAL_ASSAULT_POSTURE` additionally serializes each assault group's already-stored MissionAI destination with current/target coordinates, target team/city, cargo/capacity, grouped sea-combat support, base state and open-ocean capability.
  - Together with the existing lift/loading/cargo fields and war/victory context, this helps distinguish insufficient transport capacity, fleets that never mobilized, and fleets aimed at peripheral or wrong targets without performing any new pathfinding, target search, UWAI evaluation or RNG.
- (Requires AdvCiv-SAS 6461+) `GAME_RECORD_SOURCE_CONTEXT` additionally fingerprints the tracked runtime-relevant working-tree diff against `HEAD` across `CvGameCoreDLL`, `Assets/Config`, `Assets/Python`, `Assets/XML`, and `PrivateMaps`, recording both `trackedCodeRulesDiffBytes` and `trackedCodeRulesDiffFingerprint`.
  - This distinguishes different edits even when the same paths are marked modified and complements the separately fingerprinted DLL actually loaded by Civ4; e.g. changed C++ source with an unchanged DLL fingerprint makes a stale/unrebuilt/unreplaced DLL immediately visible, while XML/Python changes can legitimately change the source-diff fingerprint without changing the DLL.
  - The scope deliberately excludes docs/LLM helpers and untracked files, so `dirtyTrackedCount`/`dirtyFiles` can still be nonzero when only excluded tracked files changed.
  - With no tracked runtime-relevant diff, bytes are `0` and the fingerprint is the deterministic empty value `FNV1A64:CBF29CE484222325`; `-1`/`-` means the Git-derived value was unavailable rather than clean.
- (Requires AdvCiv-SAS 6464+) **Compact AI strategic-intent context:**
  - periodic team state now notably preserves each current war target/plan/age, and AI military-production snapshots expose the full high-level military strategy set alongside the existing `focusWar`, alert and war-plan counters.
  - Real foreground UWAI target selection records only the selected target's already-computed utility/drive/rank, direct/naval posture, evaluated preparation time, forced-peace time and compact strategic context immediately before the plan or declaration, plus one level-3 `DEFERRED_ROLL` row when the single best eligible target fails the timing roll; exhaustive candidate/gate reasoning remains in BBAI.
  - Espionage snapshots likewise distinguish where Spies physically are from their stored `MISSIONAI_ATTACK_SPY` destinations and summarize recon/guard intent without reevaluating movement or mission values.
- (Requires AdvCiv-SAS 6481+) **AI strategy history is now explicit and complete:**
  - level-2 `GAME_RECORD_AI_STRATEGY_CHANGE` rows record the exact final start/stop of every gameplay-active non-default `AIStrategy` after `AI_updateStrategyHash` finishes, and periodic `GAME_RECORD_AI_STRATEGIES` checkpoints preserve the whole active set for loaded-save/truncated-log analysis.
  - A shared raw enum-name helper also replaces BBAI's old multiline stringification macros while preserving its detailed transition diagnostics and trigger values.
  - The recorder reuses already-computed strategy state only; it adds no strategy valuation, RNG or pathfinding.
- (Requires AdvCiv-SAS 6482+) **AI victory-route stage history is now exact rather than only periodic:**
  - level-2 `GAME_RECORD_AI_VICTORY_STAGE_CHANGE` rows record each final Culture, Space, Conquest, Domination or Diplomacy stage transition as `oldStage` -> `newStage` at the authoritative `AI_updateVictoryStageHash` boundary, including resets from capitulation/no-capital/invalid state.
  - Existing `GAME_RECORD_AI_VICTORY_STAGES` checkpoints remain the compact full 0..4 route view.
  - No victory-stage calculation, RNG or pathfinding is repeated solely for recording.
- (Requires AdvCiv-SAS 6483+) **Worst-enemy history now records exact final team transitions:**
  - level-2 `GAME_RECORD_WORST_ENEMY_CHANGE` rows preserve the authoritative `oldEnemyTeam` -> `newEnemyTeam` change together with the old/new enmity scores already computed by `AI_updateWorstEnemy` and `recursiveRecheck=0/1`.
  - If enemy-trade-memory decay triggers AdvCiv's recursive reevaluation, only that final recursive result is committed/logged rather than the tentative first-pass candidate.
  - Existing periodic `GAME_RECORD_DIPLO_STATUS worstEnemyTeam=` fields remain the checkpoint.
  - No extra rival scan, attitude/enmity calculation, RNG or pathfinding is added solely for recording.
- (Requires AdvCiv-SAS 6484+) **Level-3 diplomacy snapshots now explain the full AI attitude arithmetic, not only the final number:**
  - `GAME_RECORD_DIPLO_ATTITUDE_BREAKDOWN` records each known cross-team major-AI relationship with non-zero contributions such as `differentReligion`, `openBorders`, `trade`, `closeBorders`, `sharedWar`, `favoriteCivic`, `rivalTrade`, aggregate `memories`, and the remaining hidden/additive modifiers used by `AI_updateAttitude`.
  - (Requires AdvCiv-SAS 6484+) It also preserves the pre-clamp component sum, its normal clamped value, cached raw attitude, gameplay-effective attitude and any forced vassal/master delta.
    - Existing `GAME_RECORD_DIPLO_MEMORIES` rows retain the exact memory-type counts/contributions and share the same scan.
    - This is level-3-only because reconstructing every component is more expensive than reading the cached total; levels 0-2 add no new attitude calculation.
- (Requires AdvCiv-SAS 6469+) **Compact AI peace-decision provenance:**
  - `GAME_RECORD_AI_PEACE_DECISION` records only the real shared peace-negotiation boundary after either UWAI or the legacy peace system has decided to negotiate.
  - It preserves the caller family, war age, initial/final end-war benefit values, provisional gold/technology/city reparations, the human-counterproposal path, final trade lists when available, and whether negotiation was blocked/insufficient, sent to a human, or actually implemented between AIs.
  - The recorder does not rerun end-war utility, trade searches or counterproposal valuation; detailed peace reluctance/probability/aspect reasoning remains in BBAI.
- (Requires AdvCiv-SAS 6470+) **Compact AI religion-choice provenance:**
  - `GAME_RECORD_AI_RELIGION_DECISION` records meaningful state-religion switch decisions at the actual `AI_doReligion` boundary: the pre-spread best and runner-up chooser scores, current/selected/favorite religions, missionary/flavor and spread/holy-city context, the raw values gameplay actually used for switch probability, the real conversion probability/roll and whether the AI converted, deferred the switch or was blocked by K-Mod's low-spread gate.
  - At level 3 the row also includes every eligible religion's already-computed chooser score.
  - Routine keep-current evaluations are suppressed, and the recorder performs no extra religion valuation or RNG.
- (Requires AdvCiv-SAS 6469+) **Knowledge-limited Barbarian pressure complements the omniscient history:**
  - periodic `GAME_RECORD_BARBARIAN_PRESSURE` rows summarize only Barbarian units/cities the player's team can actually see, reveal or legitimately deduce, their proximity to owned cities, factual Explorer/city-attack roles and `MISSIONAI_EXPLORE` groups, plus the AI's existing Barbarian-defense-focus and attacker-need state.
  - Level-3 `BARBARIAN_UNIT_SPAWNED` rows also preserve spawn-plot terrain/feature/ownership, how many major teams had the plot revealed/visible, and the nearest major-civilization city.
  - This helps separate objective Barbarian activity from the pressure a player could reasonably react to without rerunning spawn logic, pathfinding or Barbarian target scoring.
- (Requires AdvCiv-SAS 6471+) **Explicit fog-busting / defensive map-control intent:**
  - real `AI_guardCitySite` assignment/retarget changes now record the maintained city site and its found-value rank together with the final same/adjacent guard tile, already-computed defense and `seeFromLevel` contributions, total guard score, path/end-turn state and stay/move action.
  - This captures AdvCiv's deliberate choice to park guards on nearby hills/other visibility-favorable tiles for fog-busting rather than treating generic exploration as a proxy.
  - (Requires AdvCiv-SAS 6471+) `AI_guardYield` likewise records assignment/retarget changes for the high-yield improved tile selected for anti-Barbarian protection, its defense contribution and distance to the already-found nearest invisible land.
  - Periodic Barbarian-pressure state also counts persistent city-site `MISSIONAI_GUARD_CITY` groups and, at level 3, serializes their maintained-site/guard/current coordinates, so loaded-save analysis still sees units already camping on fog-control positions.
  - No extra target search, pathfinding or invisible-plot scan is added solely for SASGameRecord.
- (Requires AdvCiv-SAS 6584+) **Realized AI building-choice history:**
  - SASGameRecord notably records compact actual building commitments alongside ordinary production-flow summaries, including regular/National/Team/World-Wonder category, chooser origin, stored/required production and overlapping building-effect families.
  - Level 3 can preserve the exact chosen building/city/focus, same-pass selection score, helper threshold/max-turn/odds/actual roll context, production progress and relevant city/effect context.
  - Direct border-culture, proactive-fortification and opportunistic-Wonder commitments are covered as well.
  - (Requires AdvCiv-SAS 6584+) Building focuses use the same typed/canonical descriptor approach described above, so combined focus masks and names cannot silently drift between recorder helpers.
  - This complements BBAI: SASGameRecord answers what the AI actually committed to, while the Building Production forensic log can explain rejected candidates and component-level reasoning.
- (Requires AdvCiv-SAS 6576+) **Completed Worker-build history between periodic development snapshots:**
  - at SASGameRecord level 2+, `GAME_RECORD_WORKER_BUILDS_COMPLETED` compactly aggregates state-changing completed Worker Builds by player and turn, including exact Build and improvement-transition counts, new improvements versus outright removals/replacements, worked versus unworked replacements, route and feature work, BFC/outside-BFC and bonus context, XML-defined irrigation-carrier outcomes/losses, and maturation-chain results/replacements.
  - It also retains direct completed reversals observed from the current log session, including reversals within 5 turns, so realized improvement churn can be measured from the broad game record while candidate valuation, decision paths and rejected alternatives remain in dedicated BBAI diagnostics.
- (Requires AdvCiv-SAS 6472+) **Corporation operational intent now bridges production, routing and realized spread outcomes:**
  - when an AI city actually commits to an Executive, `GAME_RECORD_AI_CORPORATION_DECISION kind=EXECUTIVE_PRODUCTION` preserves the selected corporation/unit, adjusted Executive value, the real production threshold/stage, production need, team-HQ state and current corporation-city context.
  - New/retargeted `kind=SPREAD_TARGET` rows preserve the preferred player chosen by `AI_executiveValue`, the already-computed HQ/local-corporation/competing-corporation adjustment/population/path score components, final target score, spread cost/treasury context and whether the action is move/spread/wait-for-gold.
  - `kind=TRANSIT_TARGET` separately preserves deliberate Executive `AIRLIFT` and `SEA_TRANSPORT` routing with the already-selected destination/score/path context, so overseas or air-relocated Executives do not appear to jump strategically between ordinary spread decisions.
  - (Requires AdvCiv-SAS 6472+) Periodic `GAME_RECORD_CORPORATION_POSTURE` preserves long-lived corporation state for loaded-save analysis: policy-enabled corporations separately from actually present corporation types/city instances and team headquarters, total/idle Executives, Executives and groups on `MISSIONAI_SPREAD_CORPORATION` (including Executive cargo carried by sea transports), groups already at target, waiting for gold or ready to spread, and level-3 compact current/target city/cost context.
  - The row stays absent before any corporation is actually present/founded and before any Executive/spread mission exists.
  - Continuing the same mission is intentionally not re-logged as a decision.
  - These additions reuse live chooser values and stored MissionAI/unit/cargo state and add no corporation target search, valuation or pathfinding solely for logging.
- (Requires AdvCiv-SAS 6473+) **Espionage mission choice is now reconstructible at the real commit boundaries:**
  - the general scored `AI_bestPlotEspionage` path emits `GAME_RECORD_AI_ESPIONAGE_DECISION` with its winning and runner-up missions/targets together with each candidate's already-computed raw value, chooser random factor/result, Spy/interception overhead, mission-cost penalty, cost and final score.
  - (Requires AdvCiv-SAS 6473+) It also keeps the chooser's estimated base interception risk, Spy/escape value, EP/rate, espionage strategies, target treasury and stationary turns.
    - K-Mod's separate tactical `AI_revoltCitySpy` path deliberately bypasses that chooser, so `GAME_RECORD_AI_ESPIONAGE_TACTICAL_DECISION` records its factual city-revolt commit and remaining-defense gate without inventing comparison scores.
    - No mission valuation, tactical strength scan or chooser RNG is repeated solely for logging.
- (Requires AdvCiv-SAS 6473+) **Every real before/after-mission interception check now exposes the risk actually faced, including successful non-interceptions:**
  - `GAME_RECORD_SPY_INTERCEPTION_CHECK` records the already-computed base/final chance, the same synchronized draw/result used by gameplay (`roll=-1` when the result needs no draw), counterespionage state, counter-Spy-defense predicate, defending Spy/counter-Spy-unit counts, attacking Spy count, city espionage-defense modifier and whether the recent-mission exposure condition applies.
  - Travel interception remains intentionally unexpanded.
  - Together with the existing completed `ESPIONAGE_MISSION` and caught-Spy rows, this separates AI choice, interception risk/result and realized effects such as repeated treasury theft while keeping all extra scans behind GameRecord level 2+.
- (Requires AdvCiv-SAS 6474+) **AI technology choice now preserves the decision that precedes the existing research history:**
  - `GAME_RECORD_AI_RESEARCH_DECISION` distinguishes teammate research coordination, Python-selected targets and the ordinary `AI_bestTech` chooser.
  - For `AI_bestTech`, level 2 records the chosen/requested technology, the actual current research after queueing, deeper strategic aim, already-computed chooser/path values, distinct runner-up, path-value margin and chooser breadth; level 3 additionally serializes every distinct returned-tech candidate with its chooser/path value and deeper aim.
  - (Requires AdvCiv-SAS 6474+) Rare free-technology choices use the same provenance at depth 1.
  - The recorder does not rerun `AI_techValue` or chooser RNG.
  - Technology-path diagnostics also identify the true deepest endpoint after K-Mod path backfilling/reordering and AdvCiv's rare scenario fallback, so the recorded "aim" and selected path describe what the chooser actually used rather than assuming path element 0 is always the endpoint.
- (Requires AdvCiv-SAS 6475+) **AI resource-trade provenance now explains why a proactive bonus became the proposal anchor rather than showing only the resulting market/deal:**
  - `GAME_RECORD_AI_BONUS_TRADE_DECISION` preserves both live `AI_proposeResourceTrade` chooser sides - resources the AI wanted to receive and resources it considered giving - with the buyer's already-computed `AI_bonusTradeVal`, seller keep-value estimate, resulting bias, real probability-gate result/context, real random score, winner/runner-up and candidate counts.
  - (Requires AdvCiv-SAS 6475+) The row then joins those chooser winners to the final counterproposal terms and whether they are about to be presented to a human or implemented between AIs. Cheap winner-only context records era/current copies, base health/happiness/AIObjective and master-vassal-cluster state.
- **Update (requires AdvCiv-SAS 6577+):** the old named metal/mount flags are replaced by the buyer's shared ordinary and short-term trade marginal values, so the record follows current civilization unlocks, technology readiness, obsolescence and substitutes without rerunning candidate choice or RNG.
- (Requires AdvCiv-SAS 6475+) **Bonus tribute has its own provenance because it uses a different chooser:**
  - `GAME_RECORD_AI_BONUS_DEMAND_DECISION` records the actual randomized sorted winner/runner-up, whether the 0.6 non-surplus sorting adjustment applied, candidate/selection counts, selected resource bundle/value, minimum demand threshold and final ordinary deal valuation before a peace-treaty item can be appended.
  - Existing `GAME_RECORD_TRADE_MARKET` remains the periodic resolved availability/denial/price view; these new event rows add only the proactive AI choice that produced an actual proposal/demand.
  - No bonus valuation, trade denial, counterproposal search or RNG is repeated solely for recording, and winner factual context is gathered only when an event row is emitted.
- (Requires AdvCiv-SAS 6508+) **Realized AI `GIVE_HELP` gifts now preserve which of the three live aid algorithms produced them:**
  - `GAME_RECORD_AI_GIVE_HELP_DECISION` distinguishes relation-based resource gifts, relation-based technology gifts, and ordinary proactive technology gifts.
  - It keeps the already-selected random chooser score plus the relevant live gate state (relationship direction/pre-gift bonus ownership, tech-score ratio/gift probability, or proactive contact multiplier/assets/attitude threshold) while revision 100 remains authoritative for the exact gifted package.
  - No gift candidate scan, trade valuation or chooser RNG is repeated solely for recording.
- (Requires AdvCiv-SAS 6512+) **Realized AI deal cancellations now preserve why the AI ended or renegotiated the deal instead of only showing the later teardown:**
  - `GAME_RECORD_AI_DEAL_CANCELLATION_DECISION` distinguishes offer-rejection renegotiation, dual-agreement denial, outgoing-resource denial, an incoming resource the AI no longer needs, and the separate per-rival GPT-cap cancellation pass.
  - It retains the triggering item/denial, the live dual-denial cancellation chance, and GPT overdraft/deal-amount boundary while the existing `DIPLO_DEAL_ENDED` action remains authoritative for the actual ended payload.
  - The recorder reuses the already-computed denial/resource results and GPT-cap state and repeats no offer valuation, denial query or synchronized RNG.
- (Requires AdvCiv-SAS 6516+) **AI vassalage/capitulation provenance:**
  - `GAME_RECORD_AI_VASSALAGE_DECISION` distinguishes ordinary voluntary-vassal diplomacy, UWAI seeking a third-party protector while losing a war, and legacy/UWAI capitulation to the current enemy.
  - It keeps the relevant enemy/war-age context, voluntary-contact cadence inputs, human-capitulation counterproposal result and final package without rerunning denial checks, master search, counterproposal valuation or RNG.
  - Automatic colony/system vassalage is deliberately excluded because it is not an AI diplomatic decision; detailed UWAI surrender/protector reasoning remains in BBAI.
- (Requires AdvCiv-SAS 6521+) **Automatic vassal resource tribute now preserves the master's chooser provenance:**
  - `GAME_RECORD_AI_VASSAL_RESOURCE_TRIBUTE` records the highest-value missing resource selected from a vassal, its already-computed `AI_bonusTradeVal`, runner-up/value margin and eligible-candidate count before the one-sided deal or human tribute popup obscures that choice.
  - The native chooser remains deterministic and repeats no resource valuation or RNG solely for recording; ordinary `DIPLO_DEAL` rows remain authoritative for transfers to AI masters, while `delivery=HUMAN_POPUP` distinguishes requests that a human vassal may still decline.
- (Requires AdvCiv-SAS 6524+) **AI colony / split-empire decisions now preserve the live strategic boundary before the transfer destroys it:**
  - `GAME_RECORD_AI_COLONY_SPLIT_DECISION` distinguishes ordinary positive-value colony creation from UWAI's forced pre-capitulation colony-liberation attempt and records the selected area's already-computed aggregate `AI_splitEmpireValue`, area city/population scale and parent empire city/population context.
  - Existing player/vassal/city/unit actions remain authoritative for the resulting colony; the recorder repeats no split-value evaluation, candidate-area scan or RNG, and deeper `AI_splitEmpireValue` components remain BBAI/source-analysis territory.
- (Requires AdvCiv-SAS 6526+) **AI spaceship launch timing now preserves the strategic reason without duplicating space-strategy internals:**
  - `GAME_RECORD_AI_SPACESHIP_LAUNCH_DECISION` records each realized AI launch as either `FULL_SUCCESS` or `RIVAL_DEADLINE`, together with the live success percentage, nearest rival spaceship countdown and whether that rival deadline triggers the native override.
  - Existing `SPACESHIP_LAUNCHED` remains authoritative for the actual countdown/arrival/component state; no rival scan, spaceship valuation or RNG is repeated solely for recording.
- (Requires AdvCiv-SAS 6525+) **Missionary religion-spread destination intent is now explicit without copying the full UnitAI target scorer:**
  - `GAME_RECORD_AI_RELIGION_SPREAD_TARGET` records a newly assigned/retargeted Missionary destination with its religion, target city/player/scope and the already-computed player multiplier, path turns and final target score, while continuing the same `MISSIONAI_SPREAD` destination stays silent.
  - Realized Missionary airlift reroutes use the same compact row.
  - Existing `RELIGION_SPREAD_ATTEMPT` rows remain authoritative for success/failure/displacement; rejected-city/player scoring, multiplier decomposition and transport/path internals remain BBAI/source-analysis territory, and no target search, pathfinding or RNG is repeated solely for recording.
- (Requires AdvCiv-SAS 6523+) **Realized AI conscription now preserves which live draft trigger released the city to spend population:**
  - `GAME_RECORD_AI_DRAFT_DECISION` distinguishes full turtle-strategy drafting, local-danger drafting when nearby enemy attack strength exceeds local defense, the non-critical randomized/value route, and the existing forced-caller API path.
  - Ordinary rows retain the already-computed conscript unit/population cost, danger/land-war flags, good-value/excess-population context, poor worked plots, happiness allowance, unit-cost pressure, and trigger-specific local strength or `AI_buildUnitProb` value immediately before `conscript()` mutates population/anger and creates the unit.
  - Factual unit completion now also distinguishes true `source=CONSCRIPT` drafts from `source=COLONY_FREE_DEFENDER` units created by split-empire bootstrap; level-2 production flow keeps separate colony-defender count/material/type buckets instead of inflating conscription.
  - Rejected/no-draft city turns remain unlogged, and no draft valuation, local-strength query or RNG is repeated solely for recording.
- (Requires AdvCiv-SAS 6522+) **Realized AI city hurries now preserve why the AI paid the population/gold cost:**
  - `GAME_RECORD_AI_HURRY_DECISION` distinguishes panic-forced hurries, unhappiness relief, food-loss relief, and ordinary unit/building value decisions.
  - Value-based rows retain the live happiness/food context, subjective population/gold costs, target value, valued overflow and final value-minus-cost margin immediately before `hurry()` mutates the city; the existing `CITY_HURRIED` action remains authoritative for the actual production, treasury, population and anger result.
  - Rejected hurry types stay unlogged, and no hurry valuation or RNG is repeated solely for recording.
- (Requires AdvCiv-SAS 6518+) **Proactive AI map swaps now preserve their pre-exchange value asymmetry:**
  - `GAME_RECORD_AI_MAP_TRADE_DECISION` records how much the proposing AI values the target map, how much the target values the proposer map, their live value advantage, and the strict minimum target-value threshold that admits a realized `MAPS <-> MAPS` offer.
  - The existing `CONTACT_TRADE_MAP` row remains authoritative for delivery/package.
  - Both `AI_mapTradeVal` results are the already-live values used by the native gate; neither map is rescanned solely for recording.
- (Requires AdvCiv-SAS 6514+) **Automatic deal invalidation now preserves the exact first validity failure instead of appearing as an unexplained teardown:**
  - `GAME_RECORD_DEAL_INVALIDATION` distinguishes a resource-supply deficit, lost trade network, giver/recipient bonus obsolescence, broke-GPT enforcement and normal peace-treaty expiry.
  - It retains the failing giver/recipient and trade item plus the already-live bonus-count or gold/rate boundary when relevant; `DIPLO_DEAL_ENDED` remains authoritative for the full payload.
  - The inherited validity checks keep their original short-circuit order, and no validity query or synchronized RNG is repeated solely for recording.
- (Requires AdvCiv-SAS 6509+) **Realized AI technology trades now preserve which live proposal algorithm produced the final `CONTACT_TRADE_TECH` package:**
  - `GAME_RECORD_AI_TECH_TRADE_DECISION` distinguishes a directly acceptable randomized technology proposal, a proposal balanced by `AI_counterPropose`, and AdvC's separate high-research-progress technology-for-gold purchase.
  - It retains the net contact multiplier, randomized receive-tech winner/score and locus-suppression state, the already-selected progress fallback/raw research points and whether locus policy suppresses it, and the live recipient-side value-match or affordability context while revision 100 remains authoritative for the resulting trade items. No technology scan, trade valuation, counterproposal search or synchronized RNG is repeated solely for recording.
- (Requires AdvCiv-SAS 6476+) **AI civic/revolution choice now preserves the reasoning behind the existing civic-change history:**
  - `GAME_RECORD_AI_CIVIC_DECISION` records the live `AI_doCivics` candidate when a different best civic is found, including current/best values, value delta, tested/current bundle anarchy, percent threshold, absolute/anarchy slack, pass/fail gates and whether the choice occurred on the first pass or a bundle recheck.
  - Accepted switches are level 2; level 3 additionally keeps wanted-but-rejected and ordinary rejected candidates so anti-oscillation/hysteresis behavior can be diagnosed without rerunning `AI_bestCivic` or `AI_civicValue`.
  - (Requires AdvCiv-SAS 6476+) `GAME_RECORD_AI_CIVIC_OUTCOME` then records only meaningful accepted-bundle outcomes: the final `REVOLUTION`, `WAIT_RESEARCH` for an imminent civic that can be bundled without extra anarchy, `BLOCKED_GOLD`, or an unexpected `BLOCKED_CAN_REVOLUTION`.
  - It preserves the pending/final civic pairs plus anarchy/timer/research/gold/war/financial context; `BLOCKED_GOLD` additionally records the net deal gold-per-turn actually used by its reserve formula, and the outcome joins naturally to canonical `CIVIC_CHANGED` actions.
  - The same audit gates older BBAI-only financial-trouble/war-state civic diagnostics so logging-disabled gameplay no longer pays those diagnostic lookups.
- (Requires AdvCiv-SAS 6465+) **Compact city-development and expansion-site context:**
  - territory snapshots now notably write explicit `unimproved*` counts instead of forcing repeated subtraction from development/improvement totals, including separate BFC/suburb land and sparse developable-water coverage.
  - Level-3 periodic city detail adds `GAME_RECORD_CITY_DEVELOPMENT`, distinguishing the full geometric city radius, its owned subset, and the owned plots assigned to that city; `notOwnedRadiusPlots` and `ownedUnassignedPlots` make the two structural gaps directly grep-able.
  - The row then records development and improvement coverage, unimproved feature/resource types, and bounded coordinates for the remaining unimproved land/water and feature/resource plots.
  - This is factual map state only: it deliberately does not ask whether a Worker *should* improve/chop a plot, scan legal Builds, pathfind or duplicate BBAI Worker valuation.
  - (Requires AdvCiv-SAS 6465+) The row is therefore useful for spotting/localizing persistent underdevelopment (for example many idle Workers alongside several untouched Forest/Jungle/resource plots) while dedicated BBAI logging remains responsible for explaining the AI decision.
    - At the same level, `GAME_RECORD_CITY_SITES` serializes each non-empty AI city-site shortlist as cached rank + coordinates + current found value, complementing the regular expansion row's site count/threshold/primary site without recomputing city placement or Settler paths; this makes prospective expansion visible even before a Settler is trained.
- (Requires AdvCiv-SAS 6601+) Level-2 `GAME_RECORD_AI_SETTLER_SITE_DECISION`:
  - now preserves the exact ordinary Settler founding-target assignment/retarget boundary, including the target's then-current shortlist rank and plot-cached found value, the raw and scenario-adjusted found values actually considered by `AI_found`, path turns/path-adjusted score, safety state, and previous mission/target context when assigning or retargeting.
  - Repeated travel toward the same target emits nothing, and the recorder performs no extra site evaluation or pathfinding; this bridges the score that originally sent a Settler toward a plot to the later founding-time shortlist/rescore rows even when values or the shortlist change while it travels.
- (Requires AdvCiv-SAS 6565+) **City-network and XML-driven Worker-development context:**
  - periodic territory rows now report every XML improvement that carries irrigation, split into irrigated/dry, bonus, BFC, suburb and per-improvement counts, rather than treating Farm as the format's permanent irrigation representative.
  - Routes on Worker-developable land are likewise split between BFC and suburb coverage with counts, deltas, percentages and route-type lists.
  - (Requires AdvCiv-SAS 6565+) New `GAME_RECORD_CITY_NETWORK` rows list all known cities sharing each plot-group component that contains one of the player's cities, together with its plot, route, river-network, land and water composition.
  - The recorder reuses its existing map scan and performs no diagnostic pathfinding: component infrastructure explains broad connectivity but does not claim one exact transport mode for each city pair.
- (Requires AdvCiv-SAS 6468+) **Runtime, executable, and installation provenance:**
  - `GAME_RECORD_EXE_CONTEXT` fingerprints the actual loaded `Civ4BeyondSword.exe` independently from the mod DLL through file size, timestamps and a cached FNV-1a fingerprint.
  - The default system-context level also records native Windows versus Wine/Proton, raw Windows build/update (`UBR`) identity, process/native architecture and WOW64 state, plus a coarse `STEAM` / `GOG` / `FIRAXIS_RETAIL` / `RETAIL_OR_OTHER` installation hint and available Steam AppID/build/beta metadata.
  - (Requires AdvCiv-SAS 6468+) Level 3 explicitly opts into more identifying coarse CPU/GPU vendor families, logical processor count, physical RAM and exact Civ4 EXE/mod paths; CPU/GPU model names and device IDs remain unrecorded.
  - Display context is retried after loaded-save graphics initialization so short sessions still obtain resolution/options and, at level 3, GPU-vendor context.
  - The Steam path was runtime-tested at levels 2 and 3; GOG, Firaxis retail/other and Wine/Proton detection are source-reviewed but not yet runtime-tested.
- (Requires AdvCiv-SAS 6343+) Successful game-level Debug-mode toggles are also recorded as compact `DEBUG_MODE_CHANGED` old/new actions.
  - Free-text fields such as city, player, leader, civ, map-script, and log-file names are quoted/escaped, so names with spaces are easier to parse robustly.
  - It also notably records compact per-turn exploration, map changes and environmental status, explicit spaceship launches and project-victory progress, and a full final snapshot when the game is won.
- (Requires AdvCiv-SAS 6345+) Meaningful `SASGameRecord` boundaries use monotonic `sessionWallMilliseconds` from the log-session start: initialization completion, autoplay start/change/end, full snapshot begin/end, and text-map begin/end.
  - This exposes pauses and user delay before autoplay without repeating an identical clock on every gameplay or fixed-width map row.
  - Snapshot boundaries additionally record UTC to milliseconds, time since the previous snapshot, and time spent writing the snapshot; map boundaries similarly report the whole text-map block duration.
- (Requires AdvCiv-SAS 6345+) Autoplay rows retain their separate request-local `autoplayWallMilliseconds`.
  - Buffered actions retain event-time stamps only if initialization aborts and the raw fallback transcript is needed.
  - Real time intentionally includes pauses and user interaction; these values establish that time passed rather than what the user did between observations.
  - (Requires AdvCiv-SAS 6347+) Shared `CvGameCoreUtils` helpers keep absolute UTC formatting separate from monotonic elapsed timing, and unsigned subtraction preserves the intended `timeGetTime()` rollover behavior.
- `SASGameRecord` also records each autoplay request's requested/completed turns, starting turn/player, explicit end cause, active-player-change counts, (Requires AdvCiv-SAS 6345+) and `autoplayWallMilliseconds` from that request's start.
  - The wall value is 0 on the start row, accumulates on any intermediate change row, and gives the final real duration on the end row; it remains separate from whole-session/snapshot timing and intentionally includes pauses or user interaction.
- Separate `ACTIVE_PLAYER_CHANGED` rows show exactly when and from/to whom each transfer occurred.
  - In the full confirming run, player 0 changed to player 1 on turn 392 with 109 autoplay turns remaining; the handoff prompt was suppressed, and the final row reported one player change plus `endCause=VICTORY` and 500 completed turns from the 501-turn request because a Time victory ended the run first.
  - A separate three-request test also correctly distinguished two scheduled completions from a manual interruption: the interrupted 31-turn request stopped after 18 completed turns with 13 remaining and reported `endCause=USER_INTERRUPTED`.
  - See [KI#203](/_1_AdvCiv-SAS/Docs/README_Known_Issues.md#ki-203).
- Anonymous process-memory, system memory-load percentage, available 32-bit address space, and exact foreground/minimized samples can be disabled separately without disabling timing or gameplay records.
  - A separate 0-3 system-context privacy/detail level defaults to level 2: level 1 records display presentation context, level 2 adds software/runtime compatibility and coarse installation provenance, and level 3 additionally opts into coarse CPU/GPU vendor families, processor/RAM capacity and exact Civ4 EXE/mod paths.
  - The actual EXE binary fingerprint remains separate provenance, while CPU/GPU model names and device IDs are not recorded.
- (Requires AdvCiv-SAS 6392+) **SASGameRecord now standardizes map crowding and rival-distance context instead of requiring repeated offline reconstruction:**
  - initial geography notably records global and per-civilization starting density, inhabited and uninhabited land reserves, habitable land per starting civilization, same-landmass occupancy, pairwise start spacing, and nearest-start distances.
  - Periodic expansion history additionally distinguishes true other-team territory from teammate-inclusive foreign land and records met versus actually located rivals, nearest revealed other-team/enemy territory, and nearest known foreign/enemy cities from the capital, including same-landmass context.
  - The new fields reuse the existing geography and map-visibility scans where possible and avoid expensive own-city-by-foreign-city comparisons, making recurring questions such as "map too crowded, isolated, or simply poor AI expansion?" much easier to answer compactly.
- **Level-3 text-map previews:** `SASGameRecord` includes separate geography, terrain, bonus, feature, and political pictures so humans and LLMs can immediately see landmass shapes, islands, ice barriers, narrow passages, resource distribution, environmental cover, and player positions instead of reconstructing them from coordinates.
- (Requires AdvCiv-SAS 6436+) Initial context also includes independently tunable 25%- and 50%-per-dimension geography overviews, sampled directly from the original plots, for quick small and medium impressions before inspecting the authoritative full pictures.
  - Either overview can be disabled, and malformed percentages are bounded safely.
  - When the bounded full preview resamples a larger map, an independently tunable native-geography picture additionally preserves one logical cell per real plot and explicit coordinate extents; maps that already fit natively omit this duplicate automatically.
- The terrain picture distinguishes Coast from Ocean and overlays ice: one character cannot show both ice and its underlying water terrain, but blocked naval access matters more strategically and the obscured Coast shape is normally inferable from adjacent Coast and land.
- Terrain-letter case also shows natural food at a glance: uppercase plots can feed the population point working them, so `G` distinguishes flat Grass from a `g` Grass hill and `D` distinguishes Flood Plains/Oasis Desert from bare `d` Desert.
  - Geography shows broad land/water and plot shapes plus ice barriers without duplicating Coast terrain.
  - The political picture intentionally remains ownership-only, because physical symbols would either hide owned water or appear inconsistently only outside borders.
  - Normally stable geography, terrain, and bonuses appear once at setup/load, with exact later exceptions preserved by coordinate-bearing map/bonus-change rows.
  - When different bonuses share one resampled cell, `+` exposes the collision while exact setup rows retain their identities and coordinates.
- The pictures are omniscient diagnostic maps rather than a civilization's contemporary knowledge; notably, the bonus picture includes resources that no civilization has the technology to reveal yet.
  - The initial feature picture shows how jungle, forest, flood plains, oases, and ice affect settling, movement, health, and yields; feature and political pictures then repeat at each full interval and victory snapshot to show forest/jungle clearing and regrowth, late-game fallout, territorial expansion, conquest, and collapse across the run.
  - Blank water and dotted featureless land keep feature and bonus pictures readable without overlaying geography.
- Each layer also reports its logical-cell total, exact drawing-character total, and per-symbol cell counts, so broad composition and change can be measured without manually counting the picture; horizontal visual repetition does not inflate these cell counts.
  - Separate structured rows directly record player pollution, its sources, city health, global warming, and exact bonuses; the maps supply spatial context.
  - Clean fixed-width rows preserve the generated map's proportions in horizontal, square, or vertical maps; legends map configurable symbols and runtime numeric IDs to terrain/feature/bonus types, while political symbols map to player IDs whose setup rows provide the civilization and displayed player name.
  - XML defines control the picture bounds, monospace-width correction, and validated symbols.
  - Each picture is enabled by default because the spatial context is useful to humans and LLMs, but can be disabled independently to reduce log size and clutter.
- The initial text maps also include directional river edges, which show whether moving or attacking across each plot's southern or eastern boundary crosses a river instead of merely marking riverside plots.
  - This makes fresh-water settling corridors, river commerce, defensive lines, attack approaches, and later Levee opportunities visible.
  - Blank ocean, marked lakes, and dotted land keep rivers attached to recognizable landmasses without mistaking inland water near a coast for an ocean inlet.
  - Exact source-edge totals accompany the picture; large resampled maps preserve whether either river-edge orientation occurs within each reduced cell, while compact coordinate rows retain every original edge.
  - Rare later river edits are recorded individually too.
- Note: it currently contains spoilers: a spoiler-free implementation was considered, so the human/active player can give to an LLM or such their play summary and get LLM or such suggestions or feedback/analysis, but it would require an extensive rework, so currently this is not supported; therefore, `SASGameRecord` log should as of now not be treated as a spoiler-free player-advice export.
  - Note 2: detailed/full snapshots still follow `SAS_GAME_RECORD_INTERVAL_TURNS_UNSCALED_GAMESPEED`, so a run ending on T200 can have its last full snapshot at T190; (Requires AdvCiv-SAS 6587+) one tiny `GAME_RECORD_TURN_COMPLETED` row is nevertheless emitted for every fully processed turn at every enabled level, so a truncated/crashed record exposes its last completed turn without forcing another large snapshot.
  - For stronger LLM analysis, also provide compact source context when possible; for example, the AdvCiv-SAS light source ZIP from [`make_light_source_zip.py`](/LLM_Helpers/README.md#make_light_source_zippy) includes useful source/XML/docs/helper context and full `SASGameRecord` examples while staying compact enough to upload and share.
  - See also the full raw [SASGameRecord example log](/_1_AdvCiv-SAS/SASGameRecord_log/SASGameRecord_example.log), and [README.md (DLL Logging)](/README.md#dll-logging).
- (Requires AdvCiv-SAS 6379+) **Additional factual SASGameRecord setup and gameplay boundaries:**
  - player setup now notably also records whether civilization/leader choice provenance is known and, when known, whether each was random or manually selected; unavailable legacy/imported provenance remains explicit instead of being mislabeled as manual, exact team merges before the absorbed team membership disappears, and nuclear use at the real launch/interception boundary.
  - Successful detonations add one compact consequence row from the already-computed explosion results, including newly created fallout, destroyed improvements/features/buildings, damaged/killed units, affected cities, and population killed; intercepted nukes correctly have no detonation-effects row.
- (Requires AdvCiv-SAS 6434+) Each affected city additionally retains its population before/after, nuke-defense modifier and exact destroyed building types, including protected cities that suffered no loss.
  - Level 3 identifies every directly damaged, cargo-killed and noncombat-killed unit with its location, transport context and real damage/death operation, so strategic totals can be traced back to their concrete tactical consequences.
- (Requires AdvCiv-SAS 6348+) **Successful initialization is logged as authoritative finalized state instead of procedural construction chatter:**
  - BBAI and SASGameRecord now share `*_INITIAL_TEAM_STATE`, `*_INITIAL_TEAM_TECHS`, `*_INITIAL_DEAL`, and `*_INITIAL_STATE_SUMMARY` semantics.
  - Readable player/civilization identities and numeric slot bounds appear before the team rows they explain.
  - Team rows record finalized membership/identity and relations such as contacts, wars, open borders, defensive pacts, forced peace, permanent war/peace, vassal/master state, and trading capabilities; technology rows record exact team-level ownership/levels, including Civ4's separate storage for ordinary versus repeat technologies.
  - SASGameRecord emits one technology payload for every distinct initial team technology set and explicitly lists all teams sharing it, avoiding repeated long rows in later-era starts without assuming that every team or scenario begins alike.
  - (Requires AdvCiv-SAS 6348+) A compact SASGameRecord ruleset row names the loaded technologies that grant map, technology, gold, open-borders, defensive-pact, permanent-alliance, and vassal-state trading, preserving the useful causal information formerly repeated on every setup acquisition.
  - Successful new games no longer replay the internal setup sequence of starting-tech acquisition, self/contact setup, Barbarian/setup war changes, or starting deal construction.
  - SASGameRecord instead reports how many buffered setup actions were intentionally not replayed; if initialization aborts before finalized context, its raw buffered actions remain available with incomplete-context semantics for diagnosis.
  - Ordinary non-scenario Advanced Start can collapse its quadratic reciprocal peace-deal shape into the exact `forcePeace=` team relation plus compact deal/cancellation-window counts, while arbitrary scenario/WB and other nontrivial surviving deals retain exact detail rows.
- (Requires AdvCiv-SAS 6431+) **SASGameRecord now preserves the realized Civ4 random-event (`EventTrigger` / `EventInfo`) lifecycle in substantially greater detail:**
  - level-2+ records identify each actually delivered trigger using its player-local triggered-data ID and stable XML type, distinguish human-popup and AI-immediate delivery, describe whether the trigger normally participates in weighted random selection, is forced when eligible, or normally requires a direct/special firing path, and preserve its selected city, other-player city, unit, plot, player, religion, corporation, building, prerequisite-event and Python-hook context together with whether those concrete targets still exist.
  - Only triggers that actually enter the gameplay lifecycle are recorded; speculative trigger candidates, rejected `AI_eventValue` alternatives and other AI search traces remain intentionally outside SASGameRecord.
- (Requires AdvCiv-SAS 6431+) **The selected EventInfo reply/application is recorded as an authoritative transaction rather than inferred afterward:**
  - rows preserve the trigger ID/type, selected EventInfo XML type, primary/immediate-follow-up/delayed-follow-up path, reply age, `bUpdateTrigger`, `canDoEvent`, accepted/rejected disposition, broad declared effect families, relevant Python hooks, concrete target validity, and durable trigger-fired/EventInfo-occurrence state before and after the transaction.
  - Unit-local requirements are derived through a reusable `CvEventInfo` semantic matching every field actually consumed by `CvUnit::applyEvent`; a contextual stored unit can therefore be distinguished from an EventInfo that genuinely requires that unit to realize its payload.
  - This also makes stale-target cases such as KI#809 and KI#810 directly diagnosable without implementing either repair in this change.
- (Requires AdvCiv-SAS 6431+) **Realized EventInfo consequences are retained at the layer where they actually occur:** randomized/fixed/technology-related treasury results preserve the already-calculated range and actual gold movement; technology effects preserve the dynamically selected technology and research progress before/after; city- and empire-scoped pillaging preserves advertised minimum/maximum attempts, the actual already-rolled attempt count, successful improvement destruction and failed attempts; EventInfo-created free units preserve triggering versus affected player, resolved civilization-specific unit type, requested/created count and spawn location; successfully scheduled additional EventInfos preserve their actual countdown; successful `ClearEventChance` operations preserve their player/team/global scope and the durable occurrences actually cleared.
- (Requires AdvCiv-SAS 6434+) Deterministic city, unit and player consequences now preserve meaningful before/after state such as population, food, output, happiness/health, culture, occupation, healing/damage, experience, immobility, promotion, rename/disband execution, support, inflation, space production, espionage and forced bonus revelation.
  - Persistent UnitCombat/UnitClass promotions record both the stored future rule and existing units newly affected, while building yield/commerce/happiness/health modifiers retain their selected-city or empire scope even when the relevant building does not yet exist and no immediate output change could reveal the lasting effect.
- (Requires AdvCiv-SAS 6431+) **Persistent quest/event state is also followed after the initial popup:**
  - meaningful later expiry records preserve the exact existing expiry branch, such as a missing city/unit/other player, changed state religion/civic, obsolete technology or Python expiry check.
  - Routine non-quest occurrence-map timeout housekeeping is intentionally omitted so ordinary one-shot events do not generate a redundant second lifecycle row a few turns later.
- (Requires AdvCiv-SAS 6431+) **Event rows complement rather than duplicate the rest of SASGameRecord:**
  - broad `eventEffects` tokens make an EventInfo understandable at a glance, while resulting wars, technology acquisitions, plot changes, religion/corporation changes, city state, diplomacy and other gameplay consequences remain authoritative in their existing canonical rows.
  - Randomized presentation text is likewise not treated as gameplay history.
  - (Requires AdvCiv-SAS 6431+) The first full level-3 autoplay produced **355 delivered triggers**, all of which resolved exactly once through **339 primary EventInfo applications plus 16 genuine AI no-selection outcomes**; six additional applications were legitimate immediate chained EventInfos.
    - The run also exercised 95 gold results, 15 technology results, 10 pillage results, 16 meaningful expirations and 5 occurrence-clearing operations.
    - Pillage arithmetic reconciled exactly with canonical random-event plot changes and reached the advertised inclusive maximum in a real `EVENT_LOOTERS_2` result.
    - Synchronized RNG call ordering/counts and save layout remain unchanged; the diagnostics observe already-realized gameplay rather than adding event selection, valuation or random rolls.
- (Requires AdvCiv-SAS 6393+) **SASGameRecord now preserves the complete factual lifecycle of diplomatic vote-source elections and resolutions (including the Apostolic Palace and United Nations in standard BtS) instead of only periodic voting-source state:**
  - each real triggered vote notably records its vote source, proposal context, source religion/building/host city, Secretary General/proposer, resolution effects, eligibility, vote thresholds and targeted player/team/city.
  - A single compact result row then preserves the complete weighted ballot, election winner, yes/no/abstain/defy totals, grace-period votes defaulted to abstention, actual defiers and endorsers, cancellation, threshold versus final passage, and whether persistent resolutions activate, deactivate or remain active.
  - Existing canonical war, deal, city-transfer and victory history remains responsible for the resulting gameplay changes, connecting each resolution to what actually happened without duplicating those downstream events.
- (Requires AdvCiv-SAS 6489+) **AI vote history also explains the live decision rather than only the final ballot:**
  - AI Secretaries preserve the proposal they actually selected together with the existing random score and diplomatic-victory boost used by `AI_chooseElection`, while every real non-human ballot records its triggered-vote id, typed `AI_diploVote` cause and already-computed value/threshold/random context where meaningful.
  - Proposal screening calls are intentionally excluded, and no vote valuation or RNG is repeated for logging.
- (Requires AdvCiv-SAS 6490+) **Realized proactive AI diplomacy now preserves the missing contact-intent link:**
  - level-2 `GAME_RECORD_AI_DIPLO_CONTACT` rows record the contact class, human-contact versus immediate AI-deal delivery, current attitude, optional non-package subject, and final give/receive package once a real approach/deal has been formed.
  - Failed contact rolls/candidate scans are omitted; existing deeper peace and proactive resource-trade provenance remains authoritative instead of being duplicated.
- (Requires AdvCiv-SAS 6496+) **AI city transfers now preserve their own realized proposal intent instead of being mislabeled as generic diplomatic contacts:**
  - level-2 `GAME_RECORD_AI_CITY_TRADE_INTENT` rows cover free liberation, strategic city gifts, city swaps/negotiated city-centered packages and same-team forced completion, recording the selected pair rank/value-gap context, exact formation path and final give/receive package.
  - Failed city candidates are omitted, and no cede valuation or counterproposal search is repeated for logging; canonical deal/offer/rejection history remains responsible for what ultimately happens to the proposal.
- (Requires AdvCiv-SAS 6498+) **Realized AI joint-war hiring now preserves the strategic reason behind the final `CONTACT_JOIN_WAR` package:**
  - level-2 `GAME_RECORD_AI_WAR_TRADE_INTENT` rows distinguish full-UWAI from legacy target selection, retain the live war-age offer cadence, selected target value/score and hire price, and identify ordinary tech/gold payment versus the rare city/counterproposal fallback.
  - Ordinary rows preserve the final live balance values used by the existing acceptance test; city fallback rows preserve war-success/city-fitness context.
  - The revision-100 generic contact row remains authoritative for the exact final trade items, and no target valuation, balancing search or synchronized RNG is repeated for recording.
- (Requires AdvCiv-SAS 6499+) **AI requests that a human join an existing war now preserve the target-selection rationale lost by the generic contact row:**
  - level-2 `GAME_RECORD_AI_JOINT_WAR_REQUEST` rows retain the shared-war-adjusted mean war age that scaled contact probability, the personality contact divisor, the selected enemy's combined random/war-duration score and random component, its current war age, the human's peace counter with that enemy, and whether UWAI's reasonableness screen was active.
  - The revision-100 `CONTACT_JOIN_WAR` row remains authoritative for the actual requested war subject; no contact roll, candidate RNG, recent-peace gate or UWAI denial evaluation is repeated for recording.
- (Requires AdvCiv-SAS 6500+) **Realized embargo requests now preserve whether they were direct diplomacy or a fallback from a rejected joint-war request:**
  - level-2 `GAME_RECORD_AI_EMBARGO_REQUEST` rows distinguish the ordinary STOP_TRADING contact path from the Annoyed/worst-enemy and UWAI-denial redirects in `AI_proposeJointWar`, retaining the live team-attitude gap and already-selected joint-war target/score plus denial where applicable.
  - The revision-100 `CONTACT_STOP_TRADING` row remains authoritative for the actual embargo subject; no contact roll, target RNG, worst-enemy selection or UWAI denial evaluation is repeated solely for recording.
- (Requires AdvCiv-SAS 6502+) **Realized AI help and tribute requests now preserve the chooser reasoning hidden behind their generic contact packages:**
  - level-2 `GAME_RECORD_AI_HELP_REQUEST` rows distinguish ordinary `AI_doDiplo` from UWAI `amendTensions`, retain the competing city/technology candidates and live values, liberation state, the existing 2/3 city-preference roll result when reached, and the selected item.
  - `GAME_RECORD_AI_TRIBUTE_REQUEST` similarly records caller origin, demand type, minimum/final value and the live technology/map chooser state; revision 87 remains authoritative for detailed bonus-demand candidates and revision 100 for the final request packages.
  - No candidate scan, trade valuation or synchronized RNG is repeated solely for recording.
- (Requires AdvCiv-SAS 6493+) **Ordinary AI Great Person history now explains the action choice rather than only the eventual consumed unit:**
  - one compact level-2 `GAME_RECORD_AI_GREAT_PERSON_DECISION` row records the realized Discover, Golden Age, Trade Mission, Great Work, Join, Construct, Hurry or movement continuation together with the live Slow/Discover/Golden-Age/Trade/Great-Work scores, score threshold, selected Slow target details and previous mission state.
  - Recon, danger-discover, retreat, stranded, safety and skip fallbacks are explicit but do not fabricate a score winner.
  - The bridge reuses values and targets already produced by `AI_greatPersonMove` and performs no extra candidate scan, valuation, pathfinding or RNG; Great Generals remain on their separate `AI_generalMove` path and existing deep Great Artist BBAI diagnostics remain complementary.
- (Requires AdvCiv-SAS 6585+) Great-Merchant rows additionally separate the transformed Trade action score from the winning city's raw mission gold/full path, preserve its already-used commerce/continuation/random score factors, and distinguish the final target city from this-turn waypoint; completed Trade Missions record the compact live payout formula/modifier anatomy so unusual city-to-city gold differences are directly explainable without logging every candidate.
- (Requires AdvCiv-SAS 6587+) **Cheap exact turn-completeness boundary:**
  - every enabled recorder level emits one `GAME_RECORD_TURN_COMPLETED` row after the current turn's event reporting and autoplay/synchronization cleanup (and after the richer level-3 `END_GAME_TURN` RNG/state checkpoint when enabled), but before the game clock advances.
  - This adds only one tiny row per completed turn and no snapshot/state scan/RNG, while making the last fully completed turn explicit even in lower-detail crashed or truncated records.
  - Consumer guidance also distinguishes exact/realized rows, periodic snapshots/checkpoints and derived diagnostic context without repeating that metadata on every row.
- (Requires AdvCiv-SAS 6494+) **Great General history now preserves the exact ordered policy stage that produced each realized decision:**
  - one compact level-2 `GAME_RECORD_AI_GREAT_GENERAL_DECISION` row distinguishes preferred/ordinary Military Instructor joins, early/looser-limit/late-random Military Academy construction, danger or offensive-war unit attachment, progressively looser fallback stages, and retreat/stranded/safety/skip outcomes.
  - Successful helper stages preserve the already-selected specialist/building/unit, value, final target and this-turn waypoint; attachment rows also retain the live strength/healing criteria, while the late Academy gate preserves its existing 1-in-3 roll.
  - The bridge adds optional recorder-only output context to the existing `AI_join`, `AI_construct` and `AI_lead` passes rather than repeating city/unit scans, valuation, pathfinding or synchronized RNG, and canonical Great Person use/Great General attachment rows plus deeper BBAI candidate diagnostics remain complementary.
- (Requires AdvCiv-SAS 6380+) **Resolved diplomacy history is more complete without logging speculative AI search:**
  - it notably also records resolved help, tribute/demand, religion, civic, join-war, and embargo interactions with accepted/refused outcome, factual subject where available, and observed attitude, memory, war-plan, and war-state changes.
  - Human-to-AI ordinary submitted offers record the exact package and accepted/rejected evaluation boundary, while counterproposal rows record only the final proposed additions rather than the AI's internal candidate search; accepted trades continue through the existing canonical deal rows.
  - (Requires AdvCiv-SAS 6381+) AI-to-human ordinary offers rejected by the human also record their exact offered/requested trade items through BUG's resolved `DealRejected` UI event.
  - That bridge passes only raw trade identity into the DLL so SASGameRecord retains canonical C++ formatting, and Python lazy-caches/pre-gates the recorder level before any bridge-list construction so disabled/level-1 logging does not pay that work.
- (Requires AdvCiv-SAS 6432+) **SASGameRecord now preserves financial-strike crises and clearer city citizen/specialist state:**
  - player snapshots record whether the civilization is currently in financial strike and the inherited cumulative strike-turn counter, while level-2+ additionally records every realized strike turn so a short bankruptcy can no longer begin and end invisibly between interval snapshots.
  - Exact strike rows preserve treasury before the already-calculated gold change, the projected negative treasury before Civ4's zero clamp, cumulative strike turns and any units actually disbanded by the strike mechanic.
  - The logging path is entered only after the normal treasury calculation has already produced a negative balance and performs no additional economic decision, RNG or gameplay work.
  - (Requires AdvCiv-SAS 6432+) Periodic city and empire records also show angry and working population and city-disorder counts, improving post-game analysis of happiness shocks, occupation/disruption and citizen management.
  - Specialist reporting clearly distinguishes population assigned as specialists, actual typed free-specialist instances, and the separate allowance that lets assigned specialists consume no ordinary population; detailed type lists and corresponding empire totals/deltas use the same meanings.
  - Note: The explicit record labels also avoid Civ4's misleading inherited API terminology: `getNumGreatPeople()` counts typed free-specialist instances here, not born Great People, while `totalFreeSpecialists()` reports the separate allowance.
- (Requires AdvCiv-SAS 6396+) **SASGameRecord now preserves the otherwise-missing attempts behind consumed or intercepted special units:** Missionary actions notably record the acting unit, target city, direct spread chance and pre-attempt religion count before any membership change, distinguishing ordinary success, outright failure and K-Mod's secondary success that spreads the new religion while displacing a specifically named existing one; the existing `RELIGION_SPREAD`/`RELIGION_REMOVED` rows remain the canonical resulting state changes.
  - (Requires AdvCiv-SAS 6396+) Corporation Executives similarly notably record successful or failed attempts with their spread chance, exact gold cost and treasury before/after, so an Executive consumed after paying for a failed spread no longer disappears without explanation.
  - Spy interceptions during or after mission execution now preserve the intended/completed mission and readable target context, including destructible targets captured before they disappear, while ordinary travel interceptions remain compact.
  - Completed missions and interceptions share one target decoder, and all new target/provenance work is level-2 gated and observational: the existing spread and interception RNG, gameplay ordering and mission outcomes are unchanged.
- (Requires AdvCiv-SAS 6382+) **Research history now separates genuine completion accounting from invested-target redirections:** level-2 `RESEARCH_COMPLETED` rows notably also record the exact team progress/cost threshold, fresh modified research applied, incoming stored overflow before/after research-modifier conversion, raw modified excess, outgoing unmodified overflow granted, resulting player overflow, and the post-KI#404 stored-progress clamp; non-research technology acquisitions remain on the generic `TECH_ACQUIRED` source row without meaningless overflow fields.
  - (Requires AdvCiv-SAS 6382+) Compact `RESEARCH_TARGET_CHANGED` rows notably appear only when a player leaves an incomplete technology that already has team research invested, suppressing zero-progress changes and ordinary completed-tech queue progression; shared factual causes currently distinguish Great Person reevaluation, first-discovery-perk invalidation, diplomacy research coordination, explicit player research commands, free-tech reevaluation, or an honest unknown fallback, while team context shows whether another living teammate continues the old technology.
  - The new work is pre-gated at level 2 and reuses research values already calculated by gameplay code rather than adding speculative AI valuation logging.
- (Requires AdvCiv-SAS 6397+) **SASGameRecord can now preserve the active player's current trade market rather than only the deals that eventually happen:**
  - level-3 interval snapshots notably record one compact row per known foreign civilization with willingness to talk, trade-network and technology-trading availability, current pair gold-per-turn balance, maximum additional gold/gold-per-turn the other player can offer, resources and technologies each side can currently offer, exact diplomatic refusal reasons, and distinct mechanical reasons when an item cannot be traded at all.
  - Optional independently gated resource-price fields reproduce the GPT ask/pay information exposed by AdvCiv's Foreign Advisor; during Improved AI Auto Play, where the original human slot is temporarily AI-controlled, they instead preserve the equivalent current AI-controller market quote without changing player-control state.
- (Requires AdvCiv-SAS 6486+) Level-2 SASGameRecord now preserves exact `AreaAI` theater-posture changes (`OFFENSIVE`, `DEFENSIVE`, `MASSING`, naval `ASSAULT` variants, etc.) from both ordinary team calculation and UWAI's later land/naval alignment override, with compact periodic non-neutral checkpoints (including the separately snapshotted Barbarian team) for loaded/truncated-log analysis.
- (Requires AdvCiv-SAS 6487+) Level-2 SASGameRecord now preserves exact per-player/per-area AI target-city changes from normal target search, K-Mod random clearing, diplomacy coordination, area reassignment, and city removal, with the already-computed winning search value and current AreaAI/war-plan context where applicable.
  - Compact periodic target checkpoints include K-Mod's city-target refresh timer so loaded/truncated records remain reconstructible.
- (Requires AdvCiv-SAS 6488+) Foreground UWAI war-plan lifecycle changes now carry compact causal provenance immediately before the real mutation: preparation conclusion/cancellation, target switches, active plan-type switches, imminent-war cancellation, and victory-denial direct declarations reuse the utility/comparison/threshold values UWAI already computed.
  - Generic `WAR_PLAN_CHANGED` rows remain the authoritative state transition; BBAI retains the exhaustive review trace.
- (Requires AdvCiv-SAS 6485+) The same active-viewer market snapshot now adds compact `GAME_RECORD_TRADE_STRATEGIC` rows for bilateral treaty/vassal/capitulation denial state, cities the rival will currently cede or refuse, and Foreign Advisor Glance-style third-party war-for-hire targets with exact denial reasons.
  - These are periodic derived market observations, not fabricated exact transitions.
- (Requires AdvCiv-SAS 6397+) A separate optional layer notably records `AI_techTradeVal` for mechanically transferable technologies from the receiving team's perspective, allowing a technology's resolved trade value to be compared with whether it is offered or refused and why.
  - Existing `DIPLO_DEAL` / `DIPLO_DEAL_ENDED`, economy, bonus and diplomatic-state rows remain authoritative for deals and state changes that actually occur, avoiding duplicate active-deal history.
  - All market work is level-3 and independently pre-gated, uses const/query valuation paths without synchronized RNG or gameplay mutations, and can be disabled without affecting ordinary SASGameRecord history.
- (Requires AdvCiv-SAS 6394+) **SASGameRecord closes several remaining consequential city, unit, naval and membership-history gaps without turning ordinary activity into per-turn spam:**
  - level-2 action history notably records exact city hurries with production, treasury, population and hurry anger before/after; attributes successful pillaging to the acting unit and victim with the affected improvement/route and realized gold while retaining the existing canonical plot-change row; and brackets naval blockades with their range, affected teams/cities, duration and accumulated plunder while each realized trade-route plunder records the exact city and gold transfer.
  - Periodic military posture also records the current number of blockading units.
  - Authoritative gameplay boundaries additionally preserve unit gifts, religion and corporation additions/removals, and circumnavigation with the actual permanent sea-movement bonus.
  - The new observations remain event-driven and logging-gated, reuse gameplay state already being calculated where practical, and leave speculative AI valuation or candidate reasoning to focused diagnostics.
- (Requires AdvCiv-SAS 6398+) **SASGameRecord now preserves ordinary city population flow and substantially richer factual city state:**
  - natural growth and starvation are notably compacted into per-player level-2 interval totals, including population gained/lost and Avoid Growth prevention/food discarded, while level 3 additionally notably records each realized city population transition with its population, stored-food, granary and growth-threshold state before and after the change.
  - Periodic city snapshots now distinguish food yield from actual stored/granary food; preserve growth thresholds, Avoid Growth, occupation/disorder, maintenance, owner culture/culture level and cultural pressure, religions/corporations, and each city's gold/research/culture/espionage output and modifiers.
  - (Requires AdvCiv-SAS 6398+) Player city summaries notably add corresponding stored-food, granary, maintenance, occupied-city and Avoid Growth totals.
  - Existing happiness/health detail also records the remaining durations of whipping, conscription, defiance, temporary happiness and espionage unhappiness/unhealth, and rare culture-level expansion callbacks are preserved as exact level-2 actions rather than inferred from snapshots.
  - The additions remain observation-only and logging-gated: a 317-turn validation run reconciled all 1,658 natural population transitions and every new player/city aggregate exactly while reproducing the preceding run's existing gameplay actions, battles, plot changes and final game state unchanged.
- (Requires AdvCiv-SAS 6384+) **Goody-hut and partial-research history records realized outcomes rather than only nominal reward types:**
  - level-2 `GOODY_RECEIVED` rows notably also record the actual randomized gold, newly revealed plot count, experience/healing applied, partial or completed technology reward and stored progress, granted unit type/ID/promotions, hostile Barbarian units actually spawned, and AdvCiv follow-up/upgrade outcome provenance; rare exhausted attempts can be recorded explicitly instead of silently disappearing.
  - Periodic economy rows now include exact current-technology team progress/cost, while the existing Barbarian summary compactly lists concurrent incomplete Barbarian technologies as `TECH:progress/cost`; exact individual Barbarian spawn history remains level 3.
- (Requires AdvCiv-SAS 6385+) **City-raze history now preserves the destroyed city's strategic context and exact post-destruction consequences:**
  - level-2 `CITY_RAZED` rows bracket the actual disband so they distinguish the razer, previous/original ownership, and automatic/human/AI raze mode; record the city's population/history, culture, connectivity/distances, buildings/wonders, religions/holy-city status, corporations/headquarters, and relevant AI victory stages; and report exact player/team city, land, and population before/after deltas, the city plot's owner after culture is recalculated, and every enabled land/population victory's before/after progress against dynamically recomputed thresholds.
  - The older ambiguous `oldOwner` field is replaced because conquest ownership had already changed by the old reporter boundary.
  - Existing acquisition BFC/unit rows remain the detailed tactical context instead of being duplicated, while internal AI raze valuation/reasons remain in BBAI diagnostics.
- (Requires AdvCiv-SAS 6491+) **AI captured-city disposition now preserves the compact decision behind KEEP/RAZE/LIBERATE:**
  - level-2 `GAME_RECORD_AI_CONQUER_CITY_DECISION` rows record the realized outcome and typed reason for every `AI_conquerCity` exit, including no-raze/domination keeps, SAS forced-raze safeguards, normal or Barbarian raze valuation, successful liberation, and liberation withheld because the recipient holds one of our cities.
  - When normal valuation runs, the row reuses its already-computed distance/local-power, maintenance, population, personality/domination, remaining-factor and random components rather than rerunning conquest evaluation; the detailed BBAI raze diagnostics and canonical `CITY_RAZED` consequence row remain complementary.
- It notably also records war-declaration causes and sponsors, diplomacy memories, progress for all enabled victories, project-victory resets and failures, and compact full-map revelation.
  - It notably also further records active mod and DLL-slot identity, research and production overflow, production conversion and lost-production gold, city buildings and pollution, trade unlocks, and compact territory development including routes, improvements, irrigation, and improved/unimproved bonus types, and also names player-wide health/happiness modifier sources and records AI victory-stage strategies and the target's victory context when war is declared.
- It notably also records `UnitCombat` shares alongside raw army-composition counts, compact air/missile/nuclear posture including carrier and missile cargo, and per-city air-unit occupancy/capacity.
  - It also records current/undamaged city-defense state and bombard damage on city snapshots, synthetic consecutive city-bombard sequences with actual before/after defense reduction and unit counts, plus individual air strikes, interceptions, and plot-air-bomb attempts so military-unit ownership can be compared with actual tactical use.
- It notably also records compact production and military interval flows plus exact civic, effective state-religion, and war-plan transitions.
  - Technology-acquisition rows also identify whether each technology came from research, trade, a free choice, a goody hut, a Great Person, espionage, a random event, technology sharing, a team merge, inheritance, Advanced Start, Barbarian research, game setup, debug tools, or an unknown Python/script source.
  - Each log also identifies whether the session is a new or loaded single-player, scenario, network, Hotseat, PBEM, Pitboss, or replay game, including its multiplayer turn options.
- (Requires AdvCiv-SAS 6438+) **Level-3 authoritative-RNG checkpoints make otherwise-silent reproducibility divergence localizable without per-roll log spam:**
  - the map and synchronized `CvRandom` streams retain their initial/current states, interval/session call counts, seed replacements, NULL-message and EXE-wrapper calls, plus ordered value-stream and call-provenance fingerprints at each game-turn boundary and at new-game, load, manual map-regeneration, autoplay, victory, game-end, and session-finalization boundaries.
  - Async/UI randomness, temporary local RNG objects, Python/third-party RNGs, and other external nondeterminism are deliberately outside this layer.
  - Tracking is diagnostic-only, enabled only at level 3, adds no RNG calls, and changes neither `CvRandom` serialization nor save layout.
  - (Requires AdvCiv-SAS 6438+) These are rolling comparison fingerprints rather than a full per-call trace: one validated loaded-game run summarized 59,696,008 synchronized RNG calls into 502 checkpoint rows occupying about 608 KB (1.17% of its 51.8 MB level-3 record).
    - The first checkpoint with different state, call counts or stream fingerprint localizes where actual random consumption diverged; matching stream data with a different call-provenance fingerprint instead points to changed labels, data arguments or EXE-wrapper origin.
    - If the authoritative checkpoint states and fingerprints match while game state differs, investigate randomness or mutable state outside these two streams, such as Python or third-party code.
    - `LLM_Helpers/compare_sasgamerecord_rng.py` validates these invariants and reports the first divergence between two `.log` or single-log `.zip` records automatically.
- (Requires AdvCiv-SAS 6391+) SASGameRecord notably also preserves the production lifecycle behind AI target switching instead of treating every build choice as an isolated snapshot: compact interval history distinguishes normal completion from actual target switches/clears/resumptions, records invested production parked and later resumed plus target-type transitions, and periodically summarizes every city's active and parked unit/building/Wonder/project production, including food-produced units, completion progress and accumulated inactivity.
  - Actual production loss remains separately identified through decay, maxed/invalidated production and failure/overflow accounting, while obsolete-unit production transfers record both transferred production and any pre-existing destination production preserved when the banks are accumulated.
  - Exact per-city target changes and parked inventories remain level-3 detail.
  - See [KI#882](/_1_AdvCiv-SAS/Docs/README_Known_Issues.md#ki-882).
- It notably also synthesizes one summary for every war between civilization teams, including declaration and peace context, elapsed turns, war-success changes, units destroyed and their total production cost, city-plot battle wins, and cities/population captured.
  - Wars already active when a save is loaded are explicitly partial, while wars still active at victory or autoplay end remain marked ongoing rather than being misreported as peace.
  - A full turn-474 test matched all 115 declarations to exactly 91 ordinary-peace summaries, 4 capitulations, 13 state-change/elimination summaries, and 7 wars still ongoing at victory.
- Setup context notably also records the calendar, starting turn/year, and each player's actual assigned color with its primary RGB values, which helps connect the text record to maps and screenshots. When the game is won, one final row per civilization pairs its ordinary score with Civ4's normalized final score.
- (Requires AdvCiv-SAS 6390+) Military history notably now also distinguishes army quantity from veterancy, readiness, promotion choices, veteran attrition and combat luck: periodic unit posture records military-only XP/levels, promotion readiness, Great-General-led units, current XML/ruleset production value, wounds and health bands, with military promotion inventory.
  - Compact interval and recorder-session history records actual XP gained split between combat/noncombat, XP prevented by caps or removed by adjustments, ordinary promotion choices by type, Great-General leader-promotion applications, and veteran XP destroyed/lost in combat.
- (Requires AdvCiv-SAS 6390+) Withdrawals and combat-limit fights are now aggregated alongside ordinary wins/losses, while ordinary civilization-vs-civilization binary battles use Civ4's exact pre-combat odds to record expected-versus-observed wins, upsets and extreme lucky/unlucky outcomes; withdrawal, siege/combat-limit and Barbarian special-rule fights are deliberately excluded from that statistical sample.
  - Some of these changes are inspired by [AdvCiv-SAS's Battles Tab and Summary Tab in the Military Advisor](/Assets/Python/Screens/CvMilitaryAdvisor.py).
- Barbarian activity is also preserved without forcing ordinary civilization economy/diplomacy rows onto the Barbarian player.
  - Interval snapshots record Barbarian cities, animals, unit types/roles, and how many units are in Barbarian, unowned, or civilization-owned territory; concise city rows show their development and defenses.
  - At level 3, exact positions, fog/goody-hut/cargo spawn causes, and Barbarian combat outcomes make pressure on nearby civilizations traceable.
  - In a confirming full run, these rows captured 84 spawns, 182 combats, Barbarian city development, and eventual eradication, while adding only about 350-400 rows to an 18.5 MB record.

### 48 Civs DLL

- **48 Civs DLL** is the **default and only bundled DLL**, supporting the larger SAS24/SAS32/SAS40/SAS48 world sizes and their higher default player counts. (Requires AdvCiv-SAS 6590+) One maintained DLL avoids switching builds during testing or playing; games with fewer civilizations are still supported.
- Add a **notification at game start** (once per session) when the active DLL's **maximum civilization count** is lower than the selected world size's **default player count**.
  - (Requires AdvCiv-SAS 6590+) For example, a user compiling a custom 18-civ DLL and selecting SAS48 would receive a warning that the map may never be fully populated, with advice to use or compile a DLL with sufficient capacity and start a new game.
  - The check uses the active DLL's actual capacity and the current world-size XML values.
  - Change in [CvEventManager.py](/Assets/Python/CvEventManager.py).
  - See [KI#127](/_1_AdvCiv-SAS/Docs/README_Known_Issues.md#ki-127).

## AI

### AI — General behaviour

#### General changes (AI)

- Global AI settings lean toward **pragmatic, opportunistic** play over role-play: religion weight adjustments, lower revolt chance, lower war anger, more willingness to demand tribute (and sometimes declare if refused), and less inclination to take bad wars (especially multi-front).
  - For exact values, see [`AI_Variables_GlobalDefines.xml`](/Assets/XML/AI_Variables_GlobalDefines.xml), [`GlobalDefines_advc.xml`](/Assets/XML/GlobalDefines_advc.xml), and [`BBAI_Game_Options_GlobalDefines.xml`](/Assets/XML/BBAI_Game_Options_GlobalDefines.xml).

#### Victories (AI)

Note: cultural victory bullets as of now assume 3 legendary cities are required, if this is changed in a mod mod or such the below explanation would need to be adjusted accordingly.

Note 2: AI victory stages are internal AI strategy markers per victory route (`AI_VICTORY_*1` to `AI_VICTORY_*4`); stage 4 means a late/near-completion push.

A victory countdown means the AI has an active turns-to-win estimate/launch countdown.

UWAI war preparation means the AI has selected `WARPLAN_PREPARING_*` rather than declaring immediately; this can be too slow when a rival is close to winning.

Note 3: Before a spaceship launches, capturing its owner’s capital only moves the capital to another remaining city; completed spaceship parts remain because progress (e.g. 75%) belongs to the team.

After launch, however, capturing the owner’s current capital resets the active Space-victory countdown and stops that spaceship.

This is why AdvCiv-SAS targets the capital more aggressively as the decisive target during this narrow window, especially when a militarily weak player is about to win through a Space victory against stronger rivals.

- **AI victory denial is much more urgent:**
  - BBAI testing in save file 450 showed a deep UWAI issue that UWAI could let a rival close to winning survive too easily: in the Lincoln runs, Lincoln was very weak militarily (~0.7 military power) but still won Space despite much stronger nearby AIs (up to ~2.1) and no war declared on him in the checked late-game snapshots (`wars=-` at T260, T270, T280, T290, then wins Space at T292); in the Ramesses runs, Ramesses was militarily strong (top 2 at ~1.9 vs ~2.1 for top 1) and likewise won Space without being at war in the checked late-game snapshots either (`wars=-` at T260, T270, T280, then wins Space at T290).
  - After fixing the missing-war response, follow-up tests exposed additional failure modes: war could start too late, start but end after 1-2 turns through ordinary peace deals, or happen without enough impact.
  - The decisive checks in this fix are now therefore low-level enough, because higher-level targeted patches did not reliably solve the observed chain of failures (similar in spirit to the AdvCiv-SAS scrapping or `AI_chooseUnit` final guards): in `UWAI::Team::scheme` and related functions.
  - The fix now boosts war utility for stage-4/countdown victory threats, can convert that into direct war when the target is weak and near enough, and refuses ordinary peace while the target remains a serious victory threat.
  - In the final Lincoln-branch retest, Lincoln was weak at T260 as usual (4 cities / 55 military units / 1818 power), but this time Persia, Egypt, France, and Babylon declared at T267-T269, peace was blocked, and America was eliminated at T272.
  - In the separate Ramesses-branch retest, strong Ramesses still survived, but was no longer ignored: he lost 2 cities, 49 city-population points, 28 total units / 30 military units, and about a quarter of his power from T260 to T290.
  - The direct-war gates stay intentionally narrow so distant or naval-heavy threats can receive utility pressure without forcing reckless cross-map attacks.
  - Note: the countdown windows are expressed in Normal-speed turns and scale with `VictoryDelayPercent`, matching the victory countdown itself so the same urgency phase applies on other game speeds.
  - See [KI#184](/_1_AdvCiv-SAS/Docs/README_Known_Issues.md#ki-184).
- Update: Follow-up diagnostics found 57 cases in which an AI tried to make ordinary peace with a rival close to victory.
  - The earlier AdvCiv-SAS fix kept the war active as intended, but didn't take into account that UWAI had already accepted the peace treaty internally even though the teams remained at war: the SAS rejection was too late in the process.
  - To fix this, these treaties are now rejected before UWAI processes them, so its decisions and logs correctly show that the war continues; capitulation remains available, and confirming runs had zero false peace successes.
  - See [KI#184](/_1_AdvCiv-SAS/Docs/README_Known_Issues.md#ki-184).
- **Space victory denial now starts before the final launch window when parts are already high:**
  - After KI#184 made AIs react faster and harder to immediate Space-win threats, BBAI follow-up logs showed that some Space rivals were sometimes still too close to winning before the countdown response began.
  - In save file 450, Arabia still won because its recent war with Hannibal, the strongest nearby threat, had ended in normal Civ4 forced peace, temporarily preventing Hannibal from immediately restarting the war; by the time another anti-Space war could happen, Arabia was already at countdown 4.
  - Save file 452 showed why this was not only a forced-peace edge case: Churchill's fast Space race was already dangerous while still pre-countdown / stage 3, so the KI#184 countdown/stage-4 response had not yet become strong enough.
  - So Stage-3 Space leaders now also receive earlier utility pressure, direct-war consideration, and peace refusal once their spaceship is sufficiently complete and they are close enough to the spaceship-parts leader.
  - In save file 452, Churchill still won, but was attacked around the 8/16-parts point and lost Oxford next turn; in save file 450, the Ramesses branch won, but likewise triggered wars and peace blocking at 8/16 parts, with later direct wars and cities changing hands.
  - This is meant to start real pressure before the countdown, not to dogpile every Apollo builder.
  - See [KI#184.2](/_1_AdvCiv-SAS/Docs/README_Known_Issues.md#ki-184.2).
- Update: In save file 449, Gandhi remained completely at peace from turn 300 through launching his spaceship on turn 328 and winning on turn 338.
  - BBAI logging showed why the earlier SAS victory-denial rules did not produce a war: UWAI's ordinary contact rule rejected targets that needed more than about 3 cached path turns, removing around `100000` war value even though SAS separately allowed an emergency attack up to 10 turns away.
  - To fix this, stage-4 threats and qualifying launch countdowns now consistently use the wider SAS emergency limit.
  - Our first version of this update fix also gave that wider limit to the previous stage-3 change, substantially broadening it beyond its earlier close-contact behavior; at only 8/16 spaceship parts, this caused four independent declarations, followed by three vassals joining.
  - The two stage-3 choices are therefore controlled separately: the earlier response remains enabled, so a qualified stage-3 Space threat still encourages preparation and can trigger direct war when it passes the normal close-contact limit, while extending those early direct wars to the wider emergency distance is disabled by default but remains XML-tunable.
  - In the final confirming run, one close stage-3 threat triggered direct war without the wider limit, then four launched spaceships were stopped by capturing their owners' capitals; the fifth and strongest contender survived the response and won Space.
  - The AI therefore reacts effectively without making every stage-3 Space leader an automatic global target.
  - See [KI#184.2](/_1_AdvCiv-SAS/Docs/README_Known_Issues.md#ki-184.2).
- **When enabled, armies now try to capture the capital after a rival launches its spaceship to cancel the Space-victory countdown, instead of winning ordinary cities while the game-ending countdown continues:**
  - Earlier AdvCiv-SAS victory-denial improvements could start and preserve the necessary war against a rival threatening a Space victory, but Base AdvCiv/K-Mod still chose cities by normal conquest value.
  - For example, in save file 450, nearby Djenne scored `249333` while India's current capital Delhi scored only `3477` after launch, so although taking Djenne could remain profitable by ordinary conquest standards, only taking the current capital during the active countdown could remove the launched spaceship and prevent the imminent defeat.
  - So to fix this, city-assault armies that can reach the capital before the countdown ends now make it their emergency objective and remain focused on reaching it, rather than being diverted by regrouping, healing, upgrading or less important battles.
  - Once the capital is captured and the spaceship is stopped, they return to normal targeting.
  - Note: an understrength army can wait near the capital to recover or receive reinforcements if enough countdown turns remain, but if waiting more turns would let the rival win, the army attacks immediately despite the unfavorable odds.
  - In the confirming save-file 450 run, all 49 direct advances succeeded, 16 understrength armies deliberately waited, and two made final-turn assaults; Delhi was captured and razed on turn 265, Timbuktu on turn 279, and London had already been captured on turn 254.
  - Native America still won from well-defended Cahokia on turn 291 because it was strong enough to repel the attackers so it could complete the Space victory.
  - So this fix does not guarantee successful Space denial, but makes it more likely, especially against weaker contenders.
  - Note 3: as it is gameplay-altering, it is tunable and can be disabled to restore the old Base AdvCiv/K-Mod behavior.
  - See [KI#190](/_1_AdvCiv-SAS/Docs/README_Known_Issues.md#ki-190).
- (Requires AdvCiv-SAS 6448+) **AI finishes the spaceship parts it needs to launch instead of repeatedly interrupting them:**
  - **The original `SASGameRecord_20260907T142638Z_new1.log` already showed repeated interruptions:** Rome completed Apollo on turn 283, nearly 60 turns before Khmer, but repeatedly switched away from invested Engines and Docking Bays.
  - When Khmer won on turn 406, Rome had 15 of the 16 possible parts but still lacked the mandatory Docking Bay and had never launched.
  - Two later, unfixed replays (`BBAI_20260912T193450Z_load1.log` and `BBAI_20260912T194918Z_load1.log`) used newly added BBAI space-production logging to explain those production decisions: Rome again failed to launch, finishing with 14/16 and 15/16 parts.
  - In `BBAI_20260912T194918Z_load1.log`, the logging exposed a particularly clear example on turn 412: Rome abandoned its last required Docking Bay at **1469/1880 production (78%), just 5 turns from completion**, for a **1-turn Work Boat**, despite no immediate danger to the city.
  - Source inspection and these diagnostics traced the problem to inherited Base AdvCiv/K-Mod production reevaluation and city selection for projects: ordinary local priorities could displace a nearly finished part needed to win.
  - To fix this, **the AI now gives finishing those parts priority:** once seriously pursuing Space victory (SPACE3 onward), a safe city keeps an invested, still-required component if it is within (as of now default) 25 turns of completion, or at least 40% complete with at most 40 turns remaining (Normal-speed values, scaled with project production speed).
  - When several cities build the same part, only the copies still needed for the minimum launch set receive protection, favouring earlier completion, then greater invested production; optional duplicates can still be reconsidered, and immediate city danger still takes priority.
  - **In the two final tests, Rome launched in both games**: on turn 357 after Ethiopia launched on 351, losing the race; and on turn 347 before Ethiopia launched on 351, winning it.
  - The improvement is that the AI follows through on a viable launch plan instead of repeatedly replacing its last required parts with unrelated production.
  - See [KI#184.3](/_1_AdvCiv-SAS/Docs/README_Known_Issues.md#ki-184.3).
- (Requires AdvCiv-SAS 6449+) **AI avoids wasting production by repeatedly moving one-copy Projects such as Apollo, Manhattan, SDI and the Internet between cities:**
  - Base AdvCiv/K-Mod could park an invested Project during ordinary city-production reevaluation, after which another city could start the same one-copy Project while the earlier hammers remained stored; when one copy eventually finished, the parked copies were invalidated.
  - The original France example lost **5,259 production** this way across parked Manhattan and Apollo copies, and two present-source control runs still lost **1,444** and **4,408** production across one-copy non-space Projects.
  - The AI now keeps a safe, still-positively-valued active Project when it is genuinely close to completion (10 Normal-speed-equivalent Project turns by default), or when it is already substantially committed (40% complete with at most 20 turns remaining by default); the turn limits scale with project-production game speed.
  - This is intentionally not a sunk-cost lock: immediate city danger, zero/negative current Project value, and low-progress/slow assignments remain free to switch.
  - Testing deliberately refined the thresholds from broader early candidates after two accidental control runs showed that they would preserve too many weak assignments; with the final 40% profile, two full runs logged 45 continuity decisions each, and no Project that entered continuity was later casually released by ordinary production reevaluation.
  - Remaining invalidated Project hammers came from low-progress copies deliberately outside the guard.
  - See [KI#174.2](/_1_AdvCiv-SAS/Docs/README_Known_Issues.md#ki-174.2).
- **Cultural-victory AI is viability and bottleneck-focused:**
  - The new `SAS_BBAI_CULTURE_LOG_LEVEL` BBAI logging showed Base AdvCiv could treat an AI raising the culture slider only for happiness or border pressure as evidence that it was pursuing a cultural victory.
  - Even the early preparation stage could then raise culture priorities in every city.
  - As a result, the AI could spend production, specialists, and commerce on cities culturally ranked lower than the cultural-victory target group (i.e., the top 3 cities that must reach Legendary culture), on a target city already much closer to Legendary than another target city, or even on a city that had already become Legendary.
  - This was inefficient and could leave the AI dangerously underinvested in stronger economic, scientific, or military priorities.
  - To address these issues, the AI no longer treats its culture slider alone as a cultural-victory commitment, and early preparation no longer raises culture priority throughout the empire.
  - Before serious cultural victory pursuit begins, at least 2 of the 3 target cities must now each have already accumulated a tunable share of the Legendary requirement (10% by default), and the AI must rank among the configured number of leading cultural contenders (No point pursuing culture if we won't win culturally anyway).
  - Once committed, only the target cities receive victory-specific culture weight, shifted toward the city projected to take longest (no point having a city have 70% legendary culture and another 10% legendary culture only, as several legendary cities are required to win culturally, so have the spread be more even); a city that is already Legendary keeps only independent local-border value.
  - Converting production to culture remains available for first-ring expansion and strongly contested borders, but otherwise only to active target cities.
- **Hopeless late cultural pursuits are abandoned:**
  - The new `SAS_BBAI_CULTURE_LOG_LEVEL` BBAI logging also showed a late cultural pursuit in which the least advanced target city was projected hundreds of turns from Legendary when the estimated game had little more than 100 turns remaining (e.g., Future Era at T~400 at Normal game speed), despite likely faster Space or other victory routes.
  - The AI now returns to preparation when its maximum affordable culture-slider projection misses the estimated deadline beyond the tunable grace or falls too far behind competing cultural contenders, freeing resources for stronger victory routes.
- **Cultural pursuit gated by strong current or planned war enemies:**
  - no point investing heavily in a future cultural victory if the AI may lose the war first.
  - The AI no longer pursues serious culture-victory if its current or planned war enemies reach the shared `bEnemyStrong` power threshold (>=120% enemy power ratio by default) (the local `bDanger` is intentionally absent, because one threatened city should not veto an empire-wide victory strategy), reducing culture-victory pressure on production, specialists, and commerce while preserving ordinary local-border culture.
  - This is not an absolute ban but relative to military high enemy power and cultural advancement: Culture 2 can still remain available against weaker current or planned war opponents, culture can resume after peace if relevant, and a close late cultural win is not abandoned if all target cities already have credible high-culture countdowns.
  - In BBAI tests, a midgame map test showed the rule firing 25 times for Spain at 124% to 288% enemy power, blocking Culture 2+ pursuit despite otherwise meeting the new culture-victory foundation gates; Culture 2 remained available during a weaker war at 67% enemy power and resumed for four consecutive turns after peace.
- **AI prioritizes a more advanced victory plan over an unprepared cultural bid:**
  - when an AI has advanced far enough (a tunable AI Victory Stage as of now >= 3 (out of max 4) by default) toward another victory (Space, Conquest, Domination, or Diplomatic), it no longer diverts production, specialists, and commerce into serious cultural-victory spending unless all 3 required Legendary-city candidates are already advanced enough to keep culture credible.
  - Ordinary local-border culture remains unaffected, and an AI already close to winning culturally can still finish the job.
  - In a confirming save file 437 BBAI test, this stopped Rome's serious cultural spending 17 times after it reached Domination 3, when only 1 of its 3 required culture cities was advanced and a cultural win remained 499 to 737 turns away.
  - Rome could still experiment with culture before Domination 3, while Spain remained free to pursue culture because it had no similarly advanced alternative victory.
- **AI abandons clearly impossible cultural deadlines sooner:**
  - once all 3 required cities have progressed far enough to produce a complete estimate at the maximum affordable culture slider, the AI now immediately compares its slowest city with the estimated turns remaining instead of waiting for the late game.
  - AI drops from serious Culture 2+ investment to Culture 1 when the estimate exceeds the tunable grace (as of now 125% of the remaining turns): it stops victory-specific cultural spending for now, preserves ordinary local-border culture, and can reconsider later if conditions improve.
  - Before all 3 prospective Legendary cities are developed enough for a reliable estimate, Culture 2 remains available to help the least-developed of them catch up.
  - Unlike the previous change, this does not require another victory plan to be more advanced; an impossible cultural timeline is enough.
  - Another save file 437 test's BBAI logging confirmed Spain entered Culture 2 while its third prospective Legendary city was still too undeveloped for a reliable three-city finish estimate; on one such turn, it raised the culture slider to 30% and gave that lagging city the strongest culture priority.
  - Once all 3 cities could be estimated, even the maximum affordable culture slider projected that Spain needed 398 to 401 turns to win culturally, but only 170 to 171 turns remained, so the AI stopped serious cultural spending.
- **Cultural-victory production focus follows the actual Legendary-city targets:**
  - during Culture 2, Base AdvCiv exempted the 4 cities with the highest current culture rates from some routine military production choices, intending to leave more of their production available for cultural development.
  - This could exempt cities that needed no additional cultural help while an actual lagging Legendary-city target continued performing routine production duties.
  - The AI now aligns the culture-production role with victory candidates: it grants this exemption only to the 3 required candidates that still have positive bottleneck-balanced culture investment; other cities remain available for ordinary production.
  - In other save file 437 runs, BBAI logging showed the old rule granted 21 logged production exemptions: 14 involved cities that the balancing logic judged needed no further victory-specific culture, and 6 involved cities outside the selected top 3.
  - After the change, all 4 observed exemptions went to active candidates, and every candidate that qualified received the exemption.
- **AI prioritizes a faster active victory countdown instead of switching to an all-in cultural pursuit:**
  - an active victory countdown is more reliable than a projected cultural finish.
  - When the AI has already started such a countdown and it is shorter than the estimated turns needed for its slowest prospective Legendary city, the AI now remains at Culture 1 instead of entering Culture 2/3/4.
  - This prevents victory-specific production, specialist, and commerce spending while preserving ordinary local culture and culture-slider spending needed for happiness.
  - Save file 428 BBAI logging originally showed Mali enter Culture 3 and set its culture slider to 100% despite needing an estimated 106 turns to win culturally while its launched Space victory had only 4 turns remaining.
  - In the confirming run (i.e. after the change), the new rule fired throughout the final `4 -> 1` Space countdown while culture remained 65-64 turns away: Mali stayed at Culture 1, used only 0-30% culture for happiness, ran no culture process, and continued producing military units.

#### Bonuses (AI)

- (Requires AdvCiv-SAS 6577+) **Player-specific dynamic bonus importance replaces fixed strategic-resource labels:**
  - Base AdvCiv already derived part of a resource's value from units, buildings, projects, routes, health and happiness, but earlier AdvCiv-SAS additions layered nonzero `iAIObjective` labels and named Copper/Iron/Horse/Camel/Elephants rules onto different consumers.
  - Those labels could not recognize that War Elephants age out, that a civilization-specific unit still needs Copper later, or that another available resource already provides an equivalent unit or route.
  - Settler, Worker, trade and strategic AI now share the dynamic value instead.
  - Current `iAIObjective` values return to `0`; the XML field remains as an optional additive author/mod-mod adjustment, with each point consistently adding one final shared value unit.
- (Requires AdvCiv-SAS 6577+) **Resource substitutes lose only the value they actually replace:**
  - same-unit OR prerequisites account for already owned alternatives; different units are compared with the player's available replacement for the same AI role; obsolete/outdated units decline automatically; and XML alternative building/route prerequisites divide their enabling value among available substitutes (currently notably Coal/Oil for Railroad).
  - Independent uses remain valuable, so Iron can still matter beside Copper for Iron-only units/buildings, while late Camel can fall near zero when its mounted line is already obsolete or replaceable.
  - (Requires AdvCiv-SAS 6577+) In one validation civilization, the ordinary internal value (not its GPT trade price) changed from Ancient Iron `16` and Camel `5` to Future-era Iron `2-3` and Camel `1`; owning Coal reduced Oil's raw Railroad contribution from `4000` to `2000` without reducing Oil-only military uses.
  - Stable usefulness, coastal-yield buildings and early siege alternatives are likewise derived from current XML/civilization capabilities instead of named resource, building or unit lists.
  - See [KI#109](/_1_AdvCiv-SAS/Docs/README_Known_Issues.md#ki-109).
- (Requires AdvCiv-SAS 6577+) **Tuning policy:**
  - existing Settler and Worker multipliers can adjust each consumer's use of the shared dynamic value, with `0` disabling that contribution, while `iAIObjective` remains the per-bonus additive author/mod-mod override.
  - The shared evaluator and every internal coefficient deliberately do not receive separate master switches or XML knobs by default: disabling the core would restore incomplete or brittle named-resource assumptions, and exposing every coefficient would recreate the maintenance burden this rework removes.
  - Add another dedicated knob only when an actual balance, mod-mod, or diagnostic use case shows that it is needed.

#### City Plots (AI)

- Fix some K-Mod allocation pathologies: e.g. starving/stagnant cities no longer choose **+1 hammer** over **+4 food** unworked plots; also fix the "food is production" valuation so settlers/workers don’t undervalue food. See [KI#34](/_1_AdvCiv-SAS/Docs/README_Known_Issues.md#ki-34).
- Update - **Refine the AdvCiv-SAS starvation guard to permit partial recovery:**
  - Base AdvCiv blocked a job swap from creating starvation but, once a city was already starving, could allow a swap that worsened its food deficit.
  - An earlier AdvCiv-SAS correction prevented those worsening swaps, but was too strict in the other direction: it rejected every reassignment that left any deficit, even when switching one population member to a higher-food plot substantially reduced starvation.
  - The AI now blocks only food-reducing swaps while a deficit remains; equal-food improvements and partial food recovery can proceed when the full city-aware comparison approves them, while forced and free specialists remain protected.
  - Save file 428 BBAI logging had shown Bursa retain 4-5 Artists while starving by 5-7 food because its available 4-food plot could not eliminate the whole deficit in one reassignment; confirming runs no longer reproduced the severe Bursa and Rome starvation cases.
- If happiness room is **high**, push growth harder: favor food tiles more, deemphasize hammers (unless **food is currently production**). See [KI#40](/_1_AdvCiv-SAS/Docs/README_Known_Issues.md#ki-40).
- Update: now mitigated/halted by **high food loss from unhealthiness**, where it is inefficient to grow more (e.g., York size 25 with 25 health 36 unhealth (-11 food) and 39 happy 27 unhappy, +3 food per turn still, should not grow more and work specialists or other yields than food rather to increase efficiency).
- **Recheck and reassign citizens to safe, clearly better unworked plots:**
  - K-Mod could keep a weaker worked (i.e. assigned to one population in city) plot because its quick initial estimate overlooked another plot that its full city-aware comparison considered better.
  - If ordinary citizen allocation finds no change, the **AI now rechecks plots that improve at least one yield without reducing any**, **trade some production for a large food/commerce gain**, or otherwise **offer a substantial overall gain without losing food**.
  - It switches **only when** the full city-aware valuation confirms the change is **beneficial**, never when the swap would cause starvation, and does not immediately reverse a plot assigned earlier in the same update.
  - The newly added `SAS_BBAI_CITIZEN_LOG_LEVEL` BBAI logging test showed this corrected cases such as Karakorum working **2 food / 0 hammers / 1 commerce** instead of the strictly better **2 food / 1 hammer / 1 commerce** plot, made 624 approved swaps through turn 359 at normal game speed, and reduced remaining plot-to-plot contextual misses from 4,187 to 3,229 (about 23%); specialist-related swaps remain a separate target.
  - Note: for convenience, human cities use the same fallback by default (tunable) through `SAS_CONVENIENCE_HUMAN_CITIZEN_PLOT_FALLBACK_ENABLE`, but only while citizen automation is enabled; manual citizen assignments remain untouched.
- **Recheck modest food trades missed by K-Mod's quick plot estimate:**
  - Additionally to the previous change, BBAI logging still found recurring high-value cases such as working **2 food / 1 hammer / 1 commerce** instead of **3 food / 0 hammers / 1 commerce**, or **3 food / 1 hammer / 1 commerce** instead of **4 food / 0 hammers / 1 commerce**.
  - The fallback now also considers an unworked plot that provides at least **+1 food** while losing no more than **1 hammer and 2 commerce**, but reassigns the population only when the full city-aware comparison confirms that the trade is beneficial and the existing starvation/stability safeguards pass.
  - Across directionally comparable but diverging save file 428 BBAI runs through turn 348, this made 780 approved modest-food trades; unresolved records matching the new pattern fell from 1,460 to 0, all remaining plot-to-plot contextual-miss records fell from 3,702 to 2,404, and those valued above 1,000 fell from 762 to 162.
- **Consider additional beneficial food trades missed by K-Mod's quick plot estimate:**
  - Additionally to the previous change, BBAI logging still found the preceding narrow limits could still exclude worthwhile changes such as **2 food / 0 hammers / 3 commerce** to **3 food / 0 hammers / 0 commerce**, even when the city's full valuation strongly preferred the extra food.
  - The fallback now also checks any plot trade that gains at least **+1 food** without reducing the simple diagnostic score of **3 food + 2 hammers + 1 commerce**.
  - This score only identifies candidates: the full city-aware comparison must still approve every reassignment, and the existing starvation and stability safeguards remain in force.
  - In directionally comparable but diverging save file 428 runs through turn 340, the new rule made 292 approved reassignments, eliminated all 372 previously unresolved records matching this pattern, reduced all remaining plot-to-plot misses from 2,434 to 1,936, and reduced those valued above 1,000 from 211 to 88; no new-rule reassignment was reversed within one turn.

#### City Production (AI)

- **Optional first economic unit before the capital's first defender:**
  - when the first and only capital is not in danger, it can produce one immediately useful Work Boat or required land Worker before filling an empty city garrison.
  - This should be more efficient because it avoids delaying the first improvement and the growth, production, and bonuses it unlocks, and animals are not a serious city-capture threat.
  - However, because a human can try an unusually early rush against the temporarily empty capital, this tradeoff is XML-tunable and can be disabled to restore Base AdvCiv's defender-first behavior.
  - This is as of now only applicable to the first city, so the unchanged no-defender priority resumes after this first economic unit.
- **Prioritize useful Work Boats for seafood before land Workers:**
  - after producing an immediate defender when needed, a safe AI city now builds a Work Boat before its land Worker when it has seafood that can be improved now (unless immediate defense and danger/Turtle safeguards take precedence).
  - This is more efficient because improving seafood increases food without stopping city growth during production, and this extra food helps the following Worker and later production be produced faster.
  - Note: the AI does not waste a Work Boat on unavailable improvements such as Whale before the required technology like as of now `TECH_SEAFARING` is discovered.
  - Note: also remove the Base AdvCiv population-3 cutoff that stopped Work Boat production midway (See [KI#174](/_1_AdvCiv-SAS/Docs/README_Known_Issues.md#ki-174)).
  - After these changes, in save file 443 BBAI tests, Seoul successfully selected a Work Boat for its available Molluscs on turn 5 instead of delaying it until about turn 17 (to produce a worker before it), while Mutal correctly built its Worker first because Maya lacked the as of now required `TECH_FISHING` for Molluscs and Fish, then selected a Work Boat on turn 12 when this tech was discovered and thus these bonuses became improvable.
- **Isolated cities no longer produce land civilian units that cannot perform their role:**
  - for example, in save file 450, BBAI logging showed the one-tile island city Bactra producing a Worker that remained idle because none of its plots could be improved; the same could happen, for example, on a landmass containing only plots the AI cannot improve.
  - To fix this, the Worker-demand calculation no longer creates a minimum need when no local plot can be improved, and the AI now also refuses a Worker when its landmass already has enough, as well as land Explorer units when no exploration remains there, and land Spy units in a secondary isolated colony with neither a known rival city nor an adjacent Spy transport.
  - Primary landmasses remain eligible so an overseas empire can still produce Spies and their transports.
  - Note: this check is central in `CvCityAI::AI_chooseUnit` to make sure no short-circuit or unexpected logic bypasses it, as with AdvCiv-SAS's central unit-scrapping safeguards.
  - See [KI#192](/_1_AdvCiv-SAS/Docs/README_Known_Issues.md#ki-192).
- **Produce non-combat food-production units in suitable cities instead of stalling small/remote city growth:**
  - small or newly founded cities could spend a very long time producing units for which food counts as production (`<bFood>` / `bFoodIsProduction`) (in AdvCiv-SAS, especially Workers), stopping their population growth even when a larger city could supply the same unit very quickly from its much higher total food yield (due to high population) and generally higher hammer yield.
  - This was particularly wasteful when a mature city was already stagnant, because using it as the producer costs little or no growth there.
  - In save file 460 BBAI testing, examples included population-1 Huamanga needing 45 turns for a Worker while population-11 Cuzco could finish one in 3 turns; across the full turn-391 validation run, 87 slow Worker choices were redirected, with the median source at population 2 / 9 turns and the median preferred producer at population 8 / 3 turns.
  - The AI now keeps any fresh choice that is already fast enough (6 Normal-speed-equivalent turns by default, scaled by game speed), so several capable cities can still produce Workers in parallel after heavy losses; otherwise it looks for a safe same-area city that can produce the unit at least as fast, preferring a stagnant city when it is within 3 population of the largest eligible producer and otherwise preferring the largest population.
  - If all cities are slow, the best available producer still builds it, and existing invested production is never abandoned.
  - This logic is generic for non-combat food-production units rather than Worker-specific; Settlers have the same underlying production/growth tradeoff but are currently handled by AdvCiv-SAS's existing capital-only Settler rule and therefore remain exempt from this allocator.
  - The same validation run also had 63 population-4-or-smaller cities correctly keep Workers they could complete within the fast-production gate.
  - See [KI#198.2](/_1_AdvCiv-SAS/Docs/README_Known_Issues.md#ki-198.2).
- (Requires AdvCiv-SAS 6584+) **Timely fortification production:**
  - Walls/Castles can be sturdy enough to turn an otherwise lost invasion, and a human can react to a visible approaching stack by starting them before the siege reaches the city.
  - Inherited K-Mod/AdvC could value fortifications correctly but placed its explicit defense-building opportunity so late in `AI_chooseProduction` that it was reached in only **36/962 audited Walls** and **2/70 Castle** opportunities; among at-war cases followed by capture within 20 turns, **168/222 Walls** and **14/24 Castles** could actually have finished before the city fell.
  - AdvCiv-SAS therefore gives locally relevant fortification a bounded earlier opportunity inside the normal production chooser, while preserving current investment, immediate defenders and other urgent priorities instead of force-rewriting the queue.
  - See [KI#48.8](/_1_AdvCiv-SAS/Docs/README_Known_Issues.md#ki-48.8).

#### Specialists (AI)

- Never pick **Citizen** specialist (very inefficient). Implemented in `CvCityAI::AI_jobChangeValue`. See [KI#44.5](/_1_AdvCiv-SAS/Docs/README_Known_Issues.md#ki-44.5).
- Add **sanity guards**: in small cities (and larger ones with headroom), don’t newly assign specialists until growth caps are reached (with food/happiness checks).
  - **Barbarian** cities, on top of these rules, may only run **Scientist** specialists when they must run any.
  - See [KI#45](/_1_AdvCiv-SAS/Docs/README_Known_Issues.md#ki-45).
- Update: existing specialists **can still switch to a better specialist type** because doing so **does not reduce worked population or growth**.
  - BBAI culture diagnostics exposed this when Barcelona had 8 existing Priests/Merchants but the growth guard forbade considering any switch between specialist types; normal valuation can now compare them and choose another type (e.g. Artist, Engineer, Scientist) when better.
- **Reassign existing AI specialists to a better plot or specialist type:**
  - K-Mod's quick initial estimate could leave a city population member assigned as a specialist even when K-Mod's existing full city-aware comparison rated an available plot or another specialist type higher.
  - The AI now applies that full comparison through an additional pass that can reassign the specialist to the better plot or specialist type (an assignable free-specialist slot (counted by `totalFreeSpecialists()`), which does not use a population member, may change to a better allowed type; fixed-type free specialists (`getFreeSpecialistCount(type)`) remain unchanged, and explicitly AI/DLL-forced specialist assignments are not changed by this pass).
  - This pass only reassigns existing specialists; it never removes a population member from a worked city plot to create a new specialist, preserving the AdvCiv-SAS growth-first safeguards against excessive specialist assignment, and every change must still pass the full city-aware valuation and starvation checks.
  - Compared with the preceding BBAI run without this additional reassignment pass, both measured through turn 402, the new run made 1,963 approved reassignments (918 to plots and 1,045 between specialist types) and reduced remaining logged specialist-origin misses from 2,601 to 0.
  - As a directional sanity check within the new run rather than a before/after comparison, this pass made 201 reassignments from Artist to another job and 46 reassignments to Artist.
- **Reduce one-time corporation-founder pressure on recurring specialist selection:**
  - Before founding a corporation, K-Mod repeatedly used the full value of its headquarters when deciding how strongly cities should favor specialist jobs that generate the required Great Person type.
  - This helps produce the one founder, but a very valuable headquarters could make the AI assign too many of those specialists while waiting for that single Great Person (e.g., if at war but with many artist assignments).
  - The AI now keeps the full value of eventually using the generated Great Person's Join City action, but counts only 50% (by default, tunable) of the corporation's additional value over Join City toward specialist selection.
  - This reduces excessive pre-founder specialist pressure while retaining an incentive to produce the founder, and makes other specialist types or worked plots more competitive without forbidding the required Great Person type.
  - This applies to every Great Person type that can found a corporation, not only Great Artists.
  - Once generated, the Great Person still evaluates founding the corporation at full value.
  - Behavior after the corporation is founded is unchanged.
  - In the BBAI test, this reduced Corporation 7's contribution to Great Artist selection from 31,987 to 18,089; the corporation remained valuable enough that Rome still generated the required Great Artist on turn 297 and founded it on turn 298.
- **Reduce empire-wide specialist pressure from one city's local Join City culture value, especially for Artists (as this specialist gives most culture):**
  - K-Mod uses the best value of having a Great Person Join City anywhere in the empire when weighting every specialist that can generate that Great Person type.
  - BBAI logging showed that one city's high local culture value could therefore make Artists look unusually valuable throughout an empire that was not pursuing cultural victory, including during war: at culture stage 0 with war as its focus, Egypt had 8-9 Artists in Thebes and 7 in Memphis while high Join City culture values elsewhere set its empire-wide Great Artist weight.
  - Outside cultural-victory stages, the AI now counts only 50% of the Join City culture component by default (tunable) when calculating recurring Great Person weights, making other specialist types or worked plots more competitive; all other Join City components keep their full value.
  - The rule checks every Great Person type and reduces any culture its Join City specialist provides, but Great Artists are affected most because their joined specialist provides much more culture.
  - Actual Great Person actions still use full value, and cultural-victory stages 1-4 retain 100% of the culture component.
  - Across directionally comparable but diverging BBAI autoplays, the maximum normalized Great Artist weight fell from 163 to 132 and entries above 130 fell from 111 to 11; the new run still included Join City, Discover, Golden Age, Great Work, and corporation-founding uses of Great Artists.
- **Reduce direct Artist overvaluation in secure, non-cultural AI cities:**
  - After reducing empire-wide Great Artist weight pressure, BBAI diagnostics exposed a narrower local issue: wartime Mediolanum still assigned 5-6 Artists despite having full BFC control, no revolt risk, low culture pressure around 104-105, and an unworked **0 food / 7 hammers / 3 commerce** plot.
  - In AI job comparisons, clean cities now count only 50% of an Artist specialist's direct culture commerce by default (tunable); the Artist's research, Great Person points, and other value remain unchanged.
  - Full Artist culture value is retained for cultural-victory investment, first-ring expansion, occupation/disorder or revolt risk, emergency culture pressure, an insecure city tile, or any unowned, foreign-owned, or culture-contested BFC plot.
  - In the tested autoplay, the 50% rule appeared in 100 secure-city records containing at most 4 Artists, while all 236 records with at least 5 Artists retained 100% because they still had local culture needs; this verifies the deliberately cautious scope rather than proving a controlled overall reduction.
- Exception to the sanity guards rule, and AI enhancement: if a city hasn’t expanded to its **BFC** yet, auto-assign exactly **one Artist** to pop borders, then stop once secured. Implemented in `CvCity::AI_doTurn`.

#### Civics (AI)

- **Civic "Percent Anger" logic (rework):**
  - Fixed the AI tendency to adopt any civic with `iCivicPercentAnger` (e.g. the former CIVIC_EMANCIPATION) just because rivals have it, regardless of its actual value.
  - Replaced with an **opportunistic per-city check**: only consider such civics when enough of our cities would directly **lose anger** from the switch.
  - Implemented in `CvPlayerAI::AI_civicValue`.
  - In autoplay before/after tests, civic choices appeared **stronger**.
  - See [KI#75](/_1_AdvCiv-SAS/Docs/README_Known_Issues.md#ki-75) and [KI#317](/_1_AdvCiv-SAS/Docs/README_Known_Issues.md#ki-317).
- **Civic choice logic — unlimited specialists generalized:**
  - Addressed the AI’s tendency to default to **Caste System** and stay there.
  - Rewrote `CvPlayerAI::AI_civicValue` so **unlimited specialists** are scored by **yields** (Food > Hammers > Gold) and **commerce** (Beakers = Gold > Espionage > Culture), instead of effectively crediting only culture.
  - This lets civics like **Wage Labor** (unlimited Engineers) be chosen when appropriate.
  - Added "high-need" boosts (e.g., extra Culture weight when pushing culture/BFC (quite similar to old logic); extra Engineer/Hammer weight when enemies are strong) and **anarchy-cost awareness** (devalue switches that cost many turns).
  - Leads to a more even spread of civics among AIs based on their strategic needs, and to them being more responsive to change.
  - Details/screenshots: [KI#76](/_1_AdvCiv-SAS/Docs/README_Known_Issues.md#ki-76), [KI#315](/_1_AdvCiv-SAS/Docs/README_Known_Issues.md#ki-315) and [KI#316](/_1_AdvCiv-SAS/Docs/README_Known_Issues.md#ki-316).
  - *(Observed in autoplay: early new Slavery<->Caste swaps; late much more Wage Labor and new occasional late Serfdom.
  - May still subject to tuning.)*
- **Civic-switch damping (anti-oscillation):**
  - Reworked `CvPlayerAI::AI_doCivics` to add a **percent-slack** gate: a new civic must outscore the current one by as of now **~10%** to trigger a switch.
  - Goal: reduce rapid flip-flopping exposed after the broader civic valuation work.
  - Additionally, BBAI diagnostics showed the expensive remaining problem was **paid-anarchy civic churn**, especially direct reversals soon after the normal civic timer expired.
  - Civic switches that add anarchy now need a larger value gain, and paid-anarchy revolutions add a longer cooldown before civics are reconsidered; free/no-extra-anarchy bundled changes remain more responsive.
  - In the tested run, paid-anarchy revolutions dropped from 52 to 23, total anarchy turns from 174 to 81, and both-sides-paid direct reversals from 49 to 17 (See [KI#159](/_1_AdvCiv-SAS/Docs/README_Known_Issues.md#ki-159)).
  - This should be stronger, considering excessive anarchy downtime is inefficient, especially with our as of now AdvCiv-SAS anarchy duration of 2 turns.
  - *Note:* If the slack/cooldown is too high, AIs switch **too rarely** and **react more slowly**; if too low, oscillation returns and players with **anarchy-on-switch** waste turns.
  - *Note 2:* in some cases bypassed/ignored by AIs entirely for some reason.
- **Civic valuation — specialist commerce accounted for:**
  - Civics that grant **extra commerce per specialist** (e.g., `CIVIC_REPRESENTATION` → beakers/gold/culture) were undervalued and thus rarely chosen.
  - Updated `CvPlayerAI::AI_civicValue` to apply SAS **commerce weights** and stronger **derived multipliers**, so large specialist counts are priced properly.
  - *Observed in autoplay:* more Representation picks in mid/late game, with continued Hereditary Rule/Universal Suffrage variation.
  - Adjust to taste.

#### Technologies (AI)

- **Tech military flavor overhaul:** Retuned `FLAVOR_MILITARY` on many techs so the AI better recognizes their true military importance (e.g. `TECH_CORPORATION` **3 → 0** — no longer treated as a military tech; `TECH_CIVIL_SERVICE` **3 → 8** — now strongly flagged as military to better match its role in the reworked tree and its general impact).
- **Master–vassal tech overlap penalty:**
  - In `CvPlayerAI::AI_techValue`, reduce the value of researching techs already known by our vassal(s) or master to discourage redundant research and encourage complementary paths/trades instead.
  - See [KI#77](/_1_AdvCiv-SAS/Docs/README_Known_Issues.md#ki-77).
  - See also [Diplomacy (AI)](/_1_AdvCiv-SAS/Docs/README_Main_Changes_Guide.md#diplomacy-ai).
  - **Update:** also apply the same devaluation between **sibling vassals** in the self-research logic (if any of our fellow vassal(s) already has the tech, we value researching it less ourselves).
- **Military tech priority when weaker:**
  - In `CvPlayerAI::AI_techValue`, give much more weight to key **military techs** when we’re weaker in power; economic or "peaceful" techs are of limited use if we’re likely to be conquered (and can end up effectively researched *for* our future conqueror).
  - In one autoplay sample this seemingly in Ewuare not being able to sustain his early lead anymore.
  - See [KI#81](/_1_AdvCiv-SAS/Docs/README_Known_Issues.md#ki-81).
  - See also [Diplomacy (AI)](/_1_AdvCiv-SAS/Docs/README_Main_Changes_Guide.md#diplomacy-ai).
- (Requires AdvCiv-SAS 6402+) **Feature-heavy low-production starts value productive feature-removal technologies earlier:**
  - Base AdvCiv 1.14 already values feature-removal technologies dynamically in `CvPlayerAI::AI_techValue`, but that inherited value can be too small when AdvCiv-SAS places an important productive-feature removal on a different technology path.
  - AdvCiv-SAS now adds a stronger but conditional own-research value when a city has many matching non-bonus BFC features and one removal still represents substantial native production time.
  - The logic remains mod-agnostic: it follows the XML-defined removal technology/build, uses the **actual city-specific production** from `CvPlot::getFeatureProduction` rather than raw XML hammers, requires at least 4 matching features per city and about 2 Normal-speed-equivalent turns of current base production per chop by default, gives the capital extra weight, and caps the total bonus.
  - It does not affect technology-trade valuation.
  - In the final Joao Great Plains validation, Portugal researched Masonry on **T10 instead of T85**; its first Settler completed on **T27 instead of T43**, and Oporto was founded on **T35 instead of T67**.
  - See [KI#33.3](/_1_AdvCiv-SAS/Docs/README_Known_Issues.md#ki-33.3).

#### Religions (AI)

- **Religion valuation power-weighted:**
  - Reworked `CvPlayerAI::AI_religionValue` so religions are mostly valued by how popular they are among militarily powerful civs, and no longer mostly based on the religion of civs the AI already likes.
  - This makes it much less likely to switch into a fringe religion and end up hated by the *strong* civs, meaning fewer "random" self-sabotaging switches and better diplomacy with them (more trade, fewer wars, more stable blocs).
  - The same power-weighted value is also used wherever religion is evaluated (state-religion civics, espionage "change religion" mission, and trade/demand "adopt religion X").
  - See [KI#82](/_1_AdvCiv-SAS/Docs/README_Known_Issues.md#ki-82).

#### Leaders (AI)

- **Leaders — targeted `iBuildUnitProb` edits:**
  - Julius Caesar 30→40; Augustus Caesar 25→30; Qin Shi Huang 20→30; Genghis Khan 35→40; Alexander 35→40; Tokugawa 30→35; Stalin 30→35; Brennus 30→35; Boudica 30→35; Catherine 25→30; Mao Zedong 25→35; Isabella 25→30; Kublai Khan 25→30; Ramesses II 20→25.
  - Reductions: Willem van Oranje 25→20; Justinian I 35→30; Sitting Bull 35→30; Zara Yaqob 30→25; Mansa Musa 25→20.
- **Global change:** +10 to `iBuildUnitProb` for **all leaders** (applied on top of the per-leader edits).
- **Fix** leaders **inconsistently not having 150** as the total of their **BBAI victory weights**: (iCultureVictoryWeight, iSpaceVictoryWeight, iConquestVictoryWeight, iDominationVictoryWeight, iDiplomacyVictoryWeight, 150): (`LEADER_BISMARCK`, 20, 25, 25, 30, 40, 140) -> 20, 25, 25, 35, 45, 150; (`LEADER_BRENNUS`, 0, 30, 30, 50, 30, 140) -> 0, 30, 35, 55, 30, 140; (`LEADER_LINCOLN`, 10, 35, 35, 15, 45, 140) -> 10, 40, 35, 15, 50, 150; (`LEADER_RAMESSES`, 25, 25, 35, 35, 20, 140) -> 35, 25, 35, 35, 20, 150.
- **Favourite civics rework:**
  - Reassigned each leader’s favourite civic for better historical or thematic fit and to align with AdvCiv-SAS civic changes, using ChatGPT 5.1 + other AIs and manual review.
  - Examples: Julius Caesar — Representation → Police State (authoritarian strongman rather than democrat); Gandhi — Universal Suffrage → Pacifism; Boudica — Universal Suffrage → Nationhood ("defending the people and their land" against Rome); Darius — Free Religion → Bureaucracy (imperial system-builder); Elizabeth — Free Religion → Free Market (commerce-focused); Huayna Capac — Hereditary Rule → State Property (state-controlled Incan economy); Napoleon — Representation → Nationhood (levée en masse, national armies); Pacal II — Hereditary Rule → Caste System (highly stratified Classic Maya society).
  - Some civics are left with **no** favourite leader when no clear historical match exists.
  - Full before/after with rationale: [Assets Rebalancing table (Favourite civics)](/_1_AdvCiv-SAS/Docs/README_Assets_Rebalancing.md#after-changes-favourite-civics-per-leader).
- **Favourite religions rework:**
  - Adjusted favourite religions where they looked clearly off or overstated, based on a conservative reading of sources (see [Assets Rebalancing table (Favourite religions)](/_1_AdvCiv-SAS/Docs/README_Assets_Rebalancing.md#summary-of-the-favourite-religion-leaders-changes-in-advciv-sas)).
  - Examples (old → new, short rationale): Cyrus kept at None (relatively open and not strongly attached to a single faith), unlike Darius — None → Paganism (more overtly religious Achaemenid ruler than Cyrus); Churchill — Christianity → None (no strong personal attachment to one faith, also helps reduce Christianity’s over-representation); Frederick — Christianity → None (deist/agnostic, so a "no favourite religion" tag fits better); Genghis Khan — None → Daoism (historically Tengrist but showed particular respect for the Quanzhen Daoist master Qiu Chuji; Daoism is the closest fit here, even compared to None) —illustrating that **a leader’s favourite religion in-game can be a mechanical "closest fit" choice rather than their literal historical faith**.
  - Note: for now, only favourite religion tags were adjusted; numeric XML religion weights are unchanged.

#### Barbarians (AI)

- **Barbarians** no longer attempt **world wonders** (prevents hammer sinks). See [KI#3](/_1_AdvCiv-SAS/Docs/README_Known_Issues.md#ki-3).
- Barbarians also tend to **target weaker rivals**, and may **raze cities** more than base AdvCiv/Civ4
- **New World barbarian aggression toggle:** preserve both the old (as of now removed in Base AdvCiv) and the new Base AdvCiv behaviors via the `SAS_AI_BARBARIAN_NEW_WORLD_EARLIER_ATTACK_ENABLE` SAS define: The default `1` as of now uses the newer city/population presence rule that can end the local peace period earlier; the old as of now disabled `0` restores the older peaceable population rule.
- **Barbarian Workers no longer retreat to the city they are already inside:**
  - When a Barbarian Worker had no useful job and was already safe in a Barbarian city, Base AdvCiv still ordered it to "retreat" to that same city every turn.
  - This trapped it in a pointless retreat/wait loop.
  - It now simply waits for one turn (or may be scrapped when allowed), then checks again for useful work.
  - See [KI#175](/_1_AdvCiv-SAS/Docs/README_Known_Issues.md#ki-175).
- **Barbarian city placement now aims for stronger captured city sites:**
  - Barbarian cities can be conquered by major civilizations, so their locations should be useful long-term city sites rather than throwaway spawns the players may want to raze if not good enough.
  - This removes the old "`Don't make Barbarian cities too difficult to assail`" base AdvCiv policy (in `AIFoundValue::evaluateDefense()`).
  - In save file 450 BBAI testing, several Barbarian cities settled one tile away from stronger sites: for example `45,18` instead of hill-plant `44,17` with extra seafood, `36,20` instead of hill-plant `37,19` with extra seafood, and an eligible `57,24`/nearby Aryan site instead of stronger `57,23` with Crab.
  - See [KI#177](/_1_AdvCiv-SAS/Docs/README_Known_Issues.md#ki-177).
- **New Barbarian cities use the strongest eligible tile instead of being pushed away from other cities:**
  - the game already allows a Barbarian city to spawn only on land not currently visible to any civilization team and still applies normal founding/spacing rules.
  - Base AdvCiv then additionally used `AIFoundValue::adjustToBarbarianSurroundings()` to lower the score of otherwise eligible fogged tiles close to any existing city.
  - Its randomized distance range spread Barbarian cities out and discouraged touching borders, but could make a clearly stronger nearby tile lose to a weaker farther one.
  - SAS keeps the visibility and founding safeguards but disables this extra proximity penalty, producing cities more useful to the Barbarians and worth keeping when later captured.
  - This rule never made normal civilization Settlers avoid Barbarian cities.
- **Barbarian site evaluation can use the full BFC instead of mostly inner-ring scoring:**
  - inherited base AdvCiv Barbarian scoring intentionally focused on adjacent/inner plots and had coastal-specific logic to avoid one-off-coast Barbarian cities because some human players dislike them.
  - AdvCiv-SAS makes that behavior tunable because our priority is stronger Barbarian cities for themselves and for later conquerors: for example, outer seafood, stronger hill plants, and better workable tiles are stronger and so they should be valued in Barbarian city-site evaluation too, unlike in base AdvCiv.
  - See [KI#177](/_1_AdvCiv-SAS/Docs/README_Known_Issues.md#ki-177).
- **Barbarian coastal pressure remains available but no longer has to dominate city quality:**
  - Base AdvCiv also had a hardcoded Barbarian coastal value bonus, seemingly to accomodate player taste of some disliking a non-coastal barbarian city site.
  - However, AdvCiv-SAS does not care about this, so it keeps this, but it is now as an XML-tunable option.
  - The old value is preserved (as of now of +350), but the rationale is rather that lone-island Barbarian cities are more useful when they can maintain Galley/naval pressure and threaten player land (else they are inert ingame).
  - See [KI#177](/_1_AdvCiv-SAS/Docs/README_Known_Issues.md#ki-177).
- **Barbarian city-site randomization is now tunable:**
  - Base AdvCiv/K-Mod used a random Barbarian city-site multiplier (effectively +0..49%), so a city site valued 1000 could spike up to 1490, and thus may beat a better site of e.g. 1100 or 1200 if they did not have such a lucky random variation gain.
  - While this can add variety, it can make clearly stronger nearby sites (e.g., missing a nearby seafood) lose and place Barbarian cities on weaker sites.
  - AdvCiv-SAS fixes this by disabling such randomization, and makes it tunable if needed (so now the value is 0: disabled, 1000 has no chance of beating a 1001 site outside already existing edge cases).
  - See [KI#177](/_1_AdvCiv-SAS/Docs/README_Known_Issues.md#ki-177).
- **Barbarian global city spawning no longer applies normal-civilization distance shaping:**
  - ordinary civilizations expand from their existing empire, while Barbarian cities are selected from a global map scan.
  - In save file 450, this normal-civ distance rule could hard-reject locally strong Barbarian candidates such as `70,37`, so Barbarian placement now skips that rule.
  - See [KI#177](/_1_AdvCiv-SAS/Docs/README_Known_Issues.md#ki-177).
- **Rejected Barbarian sites stay rejected after area-priority adjustment:**
  - in save file 450, the later area-priority adjustment could turn raw rejected sites such as `30,23` back into valid spawns.
  - AdvCiv-SAS now skips raw rejected Barbarian candidates before area adjustment, and the BBAI diagnostics log raw, area-adjusted, and final chooser values so these cases are visible.
  - See [KI#177](/_1_AdvCiv-SAS/Docs/README_Known_Issues.md#ki-177).
- **Barbarian city sites now value bonus health/happiness too:**
  - Base AdvCiv skipped Barbarian non-yield bonus value because Barbarians do not care about ordinary bonus trading.
  - AdvCiv-SAS instead values Barbarian-founded cities as real long-term city sites: Barbarians benefit from better cities, and players often later capture them.
  - In save file 450 BBAI testing, the Barbarian Hittite site at `(13,35)` now scored nearby Crab with explicit `happyHealth=300 buildingHappyHealth=150`, so seafood health and ordinary building effects help Barbarian placement too.
  - See [KI#178](/_1_AdvCiv-SAS/Docs/README_Known_Issues.md#ki-178).
- (Requires AdvCiv-SAS 6569+) **Barbarian city sites no longer discard founding defense merely to ease conquest:**
  - Base AdvCiv's `AIFoundValue::evaluateDefense()` returned zero for Barbarians with the explicit rationale `Don't make Barbarian cities too difficult to assail`.
  - SAS now applies the same proportional XML-derived founding-defense value used for normal civilizations: Barbarian cities should choose strong sites for themselves, and their locations should remain worthwhile when later conquered rather than becoming cities a player would prefer to raze.
- (Requires AdvCiv-SAS 6569+) **Removed Base AdvCiv's small Barbarian Production bias from city-site scoring:**
  - Barbarian cities still need Food for growth and Commerce for research/economy, while normal scoring already values Production and Barbarian military priorities are handled elsewhere.
  - Slightly reducing Food and Commerce to favor Production did not justify a separate tile-yield valuation layer, especially for such minor values.
- **Barbarian cities improve seafood throughout their workable plots without flooding the map with Work Boats:**
  - in save file 450, BBAI logging showed that Base AdvCiv checked only the eight plots immediately beside each Barbarian city when deciding whether it needed a Work Boat.
  - As a result, seafood in the outer part of the normal workable city radius was ignored: Barbarians produced only 2 Work Boats while safe, buildable and reachable seafood remained unimproved for up to 163 turns.
  - The AI now checks the full workable city radius while keeping each Barbarian city's needs separate.
  - The first retest successfully improved 13 of 14 city-assigned targets, but exposed a second inherited bug: Base AdvCiv counted docked and queued ships only through major-civilization cities, so Hun repeatedly ordered another boat and Barbarians accumulated as many as 20 at once.
  - The completed cure also counts Work Boats sailing in the relevant water area, docked in the current city, or being trained there.
  - In the final confirming run, the peak fell from 20 Work Boats to 2 while 12 city-assigned seafood targets were improved; the other 3 targets belonged to cities conquered before their boats could finish.
  - Note: owned seafood outside every Barbarian city's BFC remains unchanged, because deciding whether Barbarians should spend production connecting a bonus that no city can work is a separate policy choice.
  - See [KI#195](/_1_AdvCiv-SAS/Docs/README_Known_Issues.md#ki-195).

#### Diplomacy (AI)

Note: Most, if not all, diplomacy AI changes apply both to **AI–AI** and **AI–human** interactions (e.g. human master with AI vassal, or vice versa).

Note 2: Master–vassal(s) changes are intended to strengthen overall AI play by improving cooperation between masters and vassals, rather than simply over-boosting one party.

The aim is that, in many cases, they also benefit vassals (or at least do not clearly reduce their own chances of winning) while being in the master's interest, since a stronger vassal is more likely to be useful.

- **Multi-war threat management — "emergency peace" pre-checks**: In `UWAI::Team::considerPeace` (`UWAIAgent.cpp`), add sanity checks to **seek peace** when either (a) the team is fighting **too many simultaneous wars** or (b) the enemies’ **combined power ratio** vs. us is high enough.
  - Goal: **survivability and focus** (fewer dogpiles, more targeted campaigns) while keeping thresholds lax so plenty of wars still occur.
  - In a sample run, Hatshepsut, strongest at ~turn 150, kept the lead to ~turn 200 with these checks (previously died before 200).
  - See **[KI#65](/_1_AdvCiv-SAS/Docs/README_Known_Issues.md#ki-65)**.
- Update: Later BBAI testing showed that the original AdvCiv-SAS rule treated any 3 wars as an emergency and tried to make peace with every opponent, which was good against strong coalitions, but bad and needlessly cautious when their combined power was, for example, only 70% of the AI's power and it could safely fight them all.
  - So instead of a blunt total max wars formula, the AI now tries to end a war only when the combined power of its enemies is dangerous (a tunable gradual increase as of now of 25% for every simultaneous war beyond 2 wars because the more enemies there are the harder and riskier it is: for example, 4 enemies with 120% combined power count as 180% power).
  - When peace is needed, the AI first approaches the opponent whose war is least valuable to continue.
  - In the confirming run, Sumer safely continued 3 to 4 wars when their combined power was weak and captured five cities, then ended one war when 126% combined enemy power became 189% adjusted danger.
  - See [KI#65](/_1_AdvCiv-SAS/Docs/README_Known_Issues.md#ki-65).
- **Tribute/counter-propose consistency fix for pure Vassal/Surrender offers:**
  - base AdvCiv/BtS diplomacy would **reject** direct **Tribute/Demand** ("Not bloody likely"), yet clicking **"What do you want in exchange?"** would **accept** for the same demand (with nothing extra).
  - So players had to do an unnecessary extra click to get the same vassal result.
  - Fixed by making `CvPlayerAI::AI_considerOffer` accept this exact pure one-item vassal/surrender demand directly, and by making [CvDiplomacy.py](/Assets/Python/CvDiplomacy.py) not remember this narrow accepted case as a tribute demand memory (because this is rather a UI consistency fix).
  - See [KI#108](/_1_AdvCiv-SAS/Docs/README_Known_Issues.md#ki-108).
- **Fair bonus trade pricing (building-effect aware).**
  - In `CvPlayerAI::AI_bonusTradeVal`, add a quick pre-check that scales a bonus’s trade value by how many **citywide building effects it actually unlocks with current techs** (granary/market/grocer/harbor/temple/monastery, etc.).
  - Wonders/cathedrals ignored; less pronounced for coastal-building effects, as we assume to simplify many cities aren’t coastal and thus wouldn’t benefit.
  - **Practical result (examples):** Maize/Wheat/Grapes are priced noticeably higher than Pigs/Cows; Molluscs beat low-impact foods; Iron is valued slightly less than before when it has no building bonuses (should still be valuable for units).
  - **Effect:** shuts down early GPT farming, makes AI<->AI swaps fairer, and strengthens AI openings/midgame.
  - See **[KI#66](/_1_AdvCiv-SAS/Docs/README_Known_Issues.md#ki-66)**.
- (Requires AdvCiv-SAS 6577+) **Resource trade pricing uses the shared player-specific bonus importance:**
  - the former AdvCiv-SAS direct `iAIObjective` price markup, named Horse/Camel/Elephants and Copper/Iron era multipliers, and hard dominated-resource buy exclusions are removed.
  - Current XML sets every bonus's `iAIObjective` to `0` because units, buildings, projects, routes, health/happiness, current technology, obsolescence and available substitutes now determine importance dynamically for the actual civilization; this is less tedious to maintain and remains reliable when XML or mod-mod rules change.
  - `iAIObjective` remains available as an optional additive author override rather than the primary strategic classification.
  - Corporation value and the separate building-effect-aware pricing above remain independent.
  - See [Bonuses (AI)](/_1_AdvCiv-SAS/Docs/README_Main_Changes_Guide.md#bonuses-ai), [KI#109](/_1_AdvCiv-SAS/Docs/README_Known_Issues.md#ki-109), [KI#239](/_1_AdvCiv-SAS/Docs/README_Known_Issues.md#ki-239), and [KI#240](/_1_AdvCiv-SAS/Docs/README_Known_Issues.md#ki-240).
- **Aggressive tech trade discounts for widely known techs:**
  - AIs now sell technologies known by ≥1 of their rivals at **heavy discounts** (e.g. 25% of the original cost/value), recognizing that it is better to secure **some gold** immediately via a cheap sale than to hold out and risk getting **0 gold** if a rival sells it first; implemented in `CvPlayerAI::AI_dealVal`.
  - To compensate for this higher trading efficiency, **AI research costs** (`iAIResearchPercent`) have been adjusted (reduced/increased at higher/lower handicaps).
  - See [KI#83](/_1_AdvCiv-SAS/Docs/README_Known_Issues.md#ki-83).
- **Disable AdvCiv relative-strength tech-trade discount rubber-banding (tunable):**
  - AdvCiv change 550a makes a weaker recipient value a stronger civilization's technology less, with technology score weighted twice as strongly as military power and a maximum late-game discount of 50%.
  - Because both AI sides use that discounted value, it guarantees the weaker civilization a better price rather than making it decide whether waiting is preferable to paying the ordinary price.
  - This is rubber-banding rather than merely a strategy change because relative standing directly changes the transaction's price and the intervention grows with game progress, regardless of technology urgency, competing sellers or whether accepting less benefits the stronger seller.
  - `SAS_AI_TECH_TRADE_RELATIVE_STRENGTH_EFFECT_PERCENT` is therefore now `0` by default (restores BTS behavior for this factor: no rubber-banding).
  - Note: it can be set to `100` to fully restore AdvCiv behavior, or intermediate values like `30` or `50` to soften the effect.
  - Other AdvCiv-SAS technology-trade adjustments remain independent.
  - See [KI#201](/_1_AdvCiv-SAS/Docs/README_Known_Issues.md#ki-201).
- **Preferential master–vassal tech trading:**
  - Heavily increase willingness to trade techs with our **master/vassal(s)**; change in `CvPlayerAI::AI_doDiplo`.
  - This synergises with the earlier "devalue techs your master/vassal already knows" change (see [Technologies (AI)](/_1_AdvCiv-SAS/Docs/README_Main_Changes_Guide.md#technologies-ai)) and extends it **to *all* techs**, not just techs that overlap with what our master/vassal(s) already know.
  - In one autoplay sample this *seemed* to **reduce tech overlap** and **improve sharing** between vassals (who had many more techs) and their master (who now picks up more of them).
  - Also updated `AI_contactRoll(CONTACT_TRADE_TECH` to pass an explicit probability multiplier (rather than relying on the default of 1), making tech-trade frequency more dynamic and tunable instead of solely personality-driven.
  - See [KI#78](/_1_AdvCiv-SAS/Docs/README_Known_Issues.md#ki-78).
  - **Update:** Extend the same extra trade bias to **sibling vassal<->vassal** trades.
- **Military-power–aware tech trade targeting:**
  - If a potential tech-trade is *not* between members of the same master–vassal(s) locus, `CvPlayerAI::AI_doDiplo` instead increases tech-trade contact chance toward **militarily weaker** players (safer, less likely to threaten us) and reduces it for **stronger** ones (less free value to future rivals).
  - In one autoplay sample this seemingly produced a much closer game: the former runaway (Ewuare) was repeatedly caught up by rivals, and the game lasted to ~T419 instead of ~T288, with the lead changing hands several times.
  - See [KI#80](/_1_AdvCiv-SAS/Docs/README_Known_Issues.md#ki-80).
- **Master–vassal(s) locus first for tech trades:**
  - Whenever the AI wants to contact other players for a tech trade in `CvPlayerAI::AI_doDiplo`, it now **first checks** whether the desired tech is already tradeable inside its own master–vassal locus (master, vassals, sibling vassals).
  - If so, it **does not ask outside civilizations for that tech**, nudging trades to stay within the group instead.
  - In one autoplay sample this *seemed* to help Ewuare win earlier by finally picking up the Dragoon-related military tech from a vassal, while the locus kept a similar overall tech pace to previous runs.
  - See [KI#79](/_1_AdvCiv-SAS/Docs/README_Known_Issues.md#ki-79).
- **Military priority in tech trades when behind:** When evaluating the value of a tech in trades with players outside the AI’s master–vassal(s) locus in `CvTeamAI::AI_techTradeVal`, the AI values **key military techs** (high `FLAVOR_MILITARY`) more highly if it is **weaker** than its rivals (they matter more for survival than additional economic techs); as of now most such military techs are **untradeable**, which is consistent with autoplay impact being small, but this may matter more if some of them become tradeable.
- **Devaluation of locus-known techs from outsiders:**
  - When evaluating the value of a tech in trades with players *outside* the AI’s master–vassal(s) locus in `CvTeamAI::AI_techTradeVal`, the AI values techs that any member of its locus already knows **moderately less** when they are offered by outsiders (since that knowledge is already available within the group).
  - In one autoplay sample this *seemed* to contribute to more decisive wins for AIs that had accumulated more vassals.

#### Buildings (AI)

- (Requires AdvCiv-SAS 6584+) **Severe-unhealthiness urgency:**
  - inherited K-Mod/AdvC noticed sickness, but severe unhealthy-food loss could still compete almost evenly with optional infrastructure: at health <= -5, health-positive candidates had about **116 median total value** versus **122** for candidates supplying no direct health, and only **51/126** especially revealing selections supplied health.
  - This matters whenever sickness is already costing food/population, and is particularly useful in current AdvCiv-SAS around **Depopulation**, which as of now applies **-9 health** (and -9 happiness) empire-wide.
  - AdvCiv-SAS adds proportional extra value only to health that actually removes unhealthy-food loss, with a further bonus for relief that directly prevents starvation/depopulation.
  - In the first full calibration, post-turn-300 negative-health snapshots fell about **54.6% -> 41.3%** and mean late health moved about **-0.82 -> +1.10**.
  - See [KI#48.6](/_1_AdvCiv-SAS/Docs/README_Known_Issues.md#ki-48.6).
- (Requires AdvCiv-SAS 6584+) **Bomb Shelter / nuclear-defense valuation:**
  - fixes inherited Kek-Mod logic that effectively let every Bomb Shelter in an area claim protection for the same large continental army, strongly overvaluing repeated Shelters even in otherwise safe cities.
  - In the audit, the median nuke-defense contribution was about **+239 overall**, **+360 among selected Bomb Shelters**, and still **+272 in selected safe/no-war/no-war-plan cities**.
  - AdvCiv-SAS keeps the useful nuclear-threat response but caps the protected-stack estimate from actual area/team-city context; the final median fell to about **+131 overall / +167 among selected buildings / +189 in safe selected cities**, while construction remained healthy (123 completed Shelters in the final validation).
  - See [KI#48.7](/_1_AdvCiv-SAS/Docs/README_Known_Issues.md#ki-48.7).
- (Requires AdvCiv-SAS 6584+) **Generic military-production building value:**
  - inherited K-Mod/AdvC mainly valued a military-production modifier from its size plus the city's ordinal production rank, even though rank does not say whether neighboring cities are **100/99/98/97** hammers or **100/10/3/1**.
  - Heroic-Epic-style value could therefore drop from roughly **+45/+37/+32** at ranks 1/2/3 to about +12 or even -40 at rank 4 despite nearly identical real production.
  - AdvCiv-SAS scales the modifier from actual city production throughput and current `AI_buildUnitProb()` military demand instead; the clean Pangaea profile became a much smoother roughly **+33/+28/+27/+22/+21/+21** across ranks 1-6.
  - See [KI#48.10](/_1_AdvCiv-SAS/Docs/README_Known_Issues.md#ki-48.10).
- (Requires AdvCiv-SAS 6584+) **Sea-production building value:**
  - inherited `DOMAIN_SEA` production-modifier value was almost geography-blind: a Drydock's +50% sea-production component had about **+10 median value on both Pangaea and Tiny Islands**, even though subsequent naval military-production share was only about **9.6% versus 40.6%**.
  - A global "Pangaea = no Drydock" rule would be too crude because some Pangaea coastal cities genuinely become fleet producers.
  - AdvCiv-SAS instead estimates expected naval throughput from city production, military demand and inherited water-world/geographic context; median +50% sea-production value became about **+1 on Pangaea versus +9 on Tiny Islands**, while locally naval Pangaea cities can still score highly.
  - See [KI#48.11](/_1_AdvCiv-SAS/Docs/README_Known_Issues.md#ki-48.11).
- (Requires AdvCiv-SAS 6584+) **Sea-unit free-experience building value:**
  - inherited `DOMAIN_SEA` free experience used a fixed coastal multiplier, so Drydock's +3 naval XP was worth about **+21** in any qualifying coastal city whether it expected to build a fleet or almost none.
  - AdvCiv-SAS scales naval-only XP from the same expected naval workload used for sea production: the median +3-XP contribution became about **+30 on Tiny Islands versus +3 on Pangaea**, while genuinely naval Pangaea cities could still reach the full +36.
  - This makes training infrastructure follow the ships that can actually use it rather than the coastline label alone.
  - See [KI#48.12](/_1_AdvCiv-SAS/Docs/README_Known_Issues.md#ki-48.12).
- (Requires AdvCiv-SAS 6584+) **Land-production building value:**
  - inherited `DOMAIN_LAND` production modifiers used the same fixed/rank shape rather than expected land throughput; in one same-turn test, **15-hammer Nidaros and 6-hammer Uppsala both received +20** for the same +50% land modifier.
  - AdvCiv-SAS instead combines actual city production, military demand and land/naval strategic context: the laboratory median +50% LAND value was about **+19 on Pangaea versus +11 on Tiny Islands**, while real land war can still restore full land demand.
  - The evaluator is generic for future SAS balance changes and modmods even when current SAS has no permanent LAND-production building.
  - See [KI#48.13](/_1_AdvCiv-SAS/Docs/README_Known_Issues.md#ki-48.13).
- (Requires AdvCiv-SAS 6584+) **Domain-production applicability:**
  - fixes inherited K-Mod/AdvC awarding production-modifier value where the city cannot train any affected unit.
  - The adversarial SEA laboratory made the bug obvious: inland Carthage could receive about **+73** for a +50% sea-production modifier despite **0 trainable sea units**; Pangaea produced similar inland false value around +35.
  - AdvCiv-SAS removes only that unusable domain component rather than rejecting the whole building, so unrelated health/happiness/XP/etc. still compete normally.
  - Prospective technology valuation can separately recognize units unlocked by the candidate technology or guaranteed prerequisites without mutating team state; complete Tiny/Pangaea validations stayed deterministic when applicability did not actually change.
  - See [KI#48.14](/_1_AdvCiv-SAS/Docs/README_Known_Issues.md#ki-48.14).
- (Requires AdvCiv-SAS 6584+) **Completion-time-aware World-Wonder placement:**
  - a World-Wonder race cares about *finishing* before rivals, not merely which city has the highest raw building value or ordinal production rank.
  - The inherited placement path could therefore prefer a much slower city and lose or abandon many hammers.
  - Across **883** migration rejections, the median rejected/current city needed about **25 turns** while the preferred own city needed about **12**; examples reached 200 versus 25 turns.
  - AdvCiv-SAS compares Wonder fit against actual turns left (including stored production and city-specific construction modifiers).
  - On Tiny Islands, failed Wonder investment fell **8,427 -> 5,688 hammers** and invalidated production **530 -> 104**, while the common-horizon global World-Wonder total remained **39**.
  - See [KI#48.15](/_1_AdvCiv-SAS/Docs/README_Known_Issues.md#ki-48.15).
- (Requires AdvCiv-SAS 6584+) **Heroic Epic specialization:**
  - ordinary `AI_buildUnitProb()` is a sensible estimate for generic military infrastructure, but it can undervalue a one-per-player +50% military-production National Wonder because choosing Heroic Epic is itself a decision to create a dedicated military-production city.
  - AdvCiv-SAS therefore gives Heroic-Epic-style National Wonders an additive specialization assumption inside the throughput formula instead of a force-first override.
  - In the calibrated Pangaea run, Heroic Epic became relevant much earlier (first best-building opportunity T172; first completion T178), yet ordinary production still chose immediate units in **26/34** logged opportunities; the **8** accepted builds were all short, about **1-6 turns**, showing specialization without blindly overriding defense.
  - See [KI#48.16](/_1_AdvCiv-SAS/Docs/README_Known_Issues.md#ki-48.16).
- (Requires AdvCiv-SAS 6584+) **Government Center placement:**
  - a Forbidden-Palace-style building should be valuable where it reduces **empire-wide** distance maintenance, not simply where the candidate city itself is expensive.
  - Inherited K-Mod/AdvC used a coarse local-maintenance proxy and completed **0 Forbidden Palaces** in both isolated Pangaea and Tiny-Islands controls.
  - AdvCiv-SAS projects the distance-maintenance saving across all own cities: Pangaea's natural Guangzhou placement had only about **6.24 GPT** local maintenance but projected **23.67 GPT** empire-wide savings; Tiny-Islands Cashel had only **4.88 GPT** locally but about **19.49 GPT** projected savings.
  - The corrected valuation naturally completed one Pangaea and three Tiny-Islands Government Centers through ordinary production competition.
  - See [KI#48.17](/_1_AdvCiv-SAS/Docs/README_Known_Issues.md#ki-48.17).
- (Requires AdvCiv-SAS 6584+) **Palace-relocation economic safeguard:**
  - inherited K-Mod/AdvC relocation heuristics could satisfy their area/population/production rules while making the empire *less* economically central.
  - In the Tiny-Islands audit they selected San Francisco even though the move projected about **-2.44 GPT raw savings** (roughly **+5.12 GPT maintenance after inflation**) and worsened total own-city distance.
  - AdvCiv-SAS keeps the inherited geography, war, production and completion-time filters but, when capital-specific player modifiers do not make maintenance an incomplete metric, requires a positive projected maintenance result.
  - The same-seed candidate rejected San Francisco on T383 and later naturally chose more-central Boston at about **+1.92 GPT raw / +4.07 GPT after inflation**; both histories still reached the common T390 Space victory.
  - See [KI#48.18](/_1_AdvCiv-SAS/Docs/README_Known_Issues.md#ki-48.18).
- (Requires AdvCiv-SAS 6584+) **National Park military opportunity cost:** National Park can be extremely valuable, but inherited K-Mod/AdvC did not adequately price the immediate military opportunity cost of starting a long one while the city was in serious danger. In the permissive audit, underdefended Uxmal faced **1,213% enemy power** and already had **93/216 hammers** invested in a roughly six-turn Mechanized Infantry, yet switched to a **13-turn National Park**; the defender was delayed and the empire later contracted from five cities to three. AdvCiv-SAS applies a smooth pressure discount only to current production value, softened by stored Park production, while prospective Biology/research value remains uncontaminated. In the validation, **660** pressured adjustments occurred, but **0/62** National-Park best-building opportunities and **0/22** final Park choices happened under `danger || enemyStrong`; cities instead selected defenders when pressure warranted. See [KI#48.19](/_1_AdvCiv-SAS/Docs/README_Known_Issues.md#ki-48.19).
- (Requires AdvCiv-SAS 6584+) **World-Wonder local opportunity cost:**
  - a fresh World Wonder is discretionary investment: on a contested landmass, those hammers may be more valuable as units, Settlers or infrastructure, and an exposed AI that is conquered can effectively have built the Wonder for its rival - an easy weakness for a human to exploit repeatedly.
  - Inherited K-Mod/AdvC could over-invest in this situation, while a blanket "at war = no Wonders" rule is also too crude because secure/island empires can sensibly build them.
  - AdvCiv-SAS therefore applies a moderate local-exposure discount only when a serious rival shares the landmass and the AI lacks the configured power-dominance margin; secure or clearly dominant empires retain full value.
  - (Requires AdvCiv-SAS 6584+) The policy reuses `SASLocalAreaRivalContext` but deliberately compares against the **strongest** known local rival rather than KI#53.5's **combined** local-rival power, because one strong neighbor can make Wonder investment dangerous even when the whole local rival set is small.
    - At the default 125% dominance threshold, the read-only classification marked **82/90 Pangaea** first attempts as locally exposed but only **13/57 Tiny-Islands** attempts, showing that the condition distinguishes the intended map situations before changing behavior.
    - In the corrected Pangaea A/B, first realized Wonder attempts fell **99 -> 57** while eventual-win count stayed **26**, and failed invested production fell **9,635 -> 3,113 hammers**; Tiny Islands remained identical through T155 and first diverged only in the intended exposed Spain/America case.
    - See [KI#48.20](/_1_AdvCiv-SAS/Docs/README_Known_Issues.md#ki-48.20) and [KI#53.5](/_1_AdvCiv-SAS/Docs/README_Known_Issues.md#ki-53.5).

#### Units (Common) (AI)

- Old AdvCiv-SAS default scrapping mode: **Scrapping & UnitAI production quotas (generalized)**: Reworked scrapping across (almost) all `UnitAI` types to stop destroy→rebuild loops.
  - Paired with per-`UnitAI` **max/quotas** in `CvCityAI::AI_chooseUnit` and scrapping guards in `CvUnit::canScrap`.
  - Policy examples: during **war/danger**, suppress explorers/settlers and redirect output to land units; on **naval-heavy** maps (archipelago, etc.) **don’t restrict naval quotas** — sea control can decide wars (e.g., a frigate that sinks transports before they land can neutralize a four-unit invasion) and supports diverse strategies; on **land-heavy** maps (pangaea/continents) set naval quotas **much lower**.
  - In war/danger, naval production quotas drop (sometimes to **0**), but **existing** ships aren’t auto-scrapped; **workers** use a **decaying max** so late-game excess can be scrapped conservatively.
  - Fixes the "naval dementia" case (mass privateers/galleons while land cities fall).
  - See [KI#53](/_1_AdvCiv-SAS/Docs/README_Known_Issues.md#ki-53) (complements [KI#52](/_1_AdvCiv-SAS/Docs/README_Known_Issues.md#ki-52)).
  - **Guardrail impact:** likely also covers hidden loops such as the **workboat infinite loop**; see [KI#23](/_1_AdvCiv-SAS/Docs/README_Known_Issues.md#ki-23).
- **Work Boat overproduction bug follow-up:**
  - after fixing the old Work Boat produce/scrap loop, BBAI logs still showed some minor remaining Work Boat overproduction, confirmed by ingame advisors (e.g., Augustus having built 34 Work Boats by T373, more than any other unit type).
  - Two mismatches were fixed.
  - First, cities could decide whether to build a Work Boat using a different water area than the one used to count seafood targets, so need and availability did not always match.
  - Second, Work Boats could skip owned **off-BFC seafood** even though city production had counted it as a valid target.
  - These mismatches are now fixed.
  - Following this, Work Boat production is reduced (e.g., to ~27-29 Work Boats for Augustus, remaining seeming to be due to heavy pillaging).
  - As an extra enhancement, stale Work Boats now also skip seafood already connected by another Work Boat.
  - See [KI#157](/_1_AdvCiv-SAS/Docs/README_Known_Issues.md#ki-157).
- **Obsolete-unit scrapping exception:** units that have an `ObsoleteTech` can be scrapped once the active scrap mode allows obsolete cleanup, because very outdated units are less efficient to keep. **Cargo-capable units are kept** because an obsolete transport can still be our only practical way to move units.
- Old AdvCiv-SAS default scrapping mode: **Scrapping safeguards**: added pre-checks to stop scrapping **useful units**—**all land military units are protected**, and **no unit can be scrapped before turn 150**.
  - Fixes early "produce then scrap" cases (e.g., macemen) and keeps forces on the map vs. barbarians/rivals (**no economy check**).
  - See **[KI#52](/_1_AdvCiv-SAS/Docs/README_Known_Issues.md#ki-52)**.
- Old AdvCiv-SAS default scrapping mode: Add **scrap tolerance beyond unit support costs**: we expect to lose a few units during war; having a few **extra units** helps us not become weaker than our rivals; overshooting may lead to bankruptcy. Formula: `mult * (era + 1) + add` extra units.
- Old AdvCiv-SAS default scrapping mode: Heavily increase the importance of **XML Unit Cost (Hammers)** (reliable reflection of a unit's overall strength: e.g. A 4 Strength Trebuchet at 80 Hammers Cost is stronger than a 5 Strength Axeman at 35 Hammers) when deciding which unit to scrap first, and **reduce the importance of experience**: A 0 XP swordsman is more valuable than a 10 XP we will likely never upgrade.
  - Also **account for map heaviness** (e.g. prioritize scrapping land units on archipelago, or galleys on pangea).
  - Change in `CvPlayerAI::AI_disbandValue`.
- **AI scrapping update:** added an optional (as of now disabled) absolute XML kill switch to disable all AI scrapping for testing or preference. This is separate from the normal scrapping optimization toggle, so disabling optimization does not accidentally restore broad scrapping.
- **AI scrapping update:**
  - block all AI scrapping, **except for units that have been obsolete for one or a few eras**.
  - Example: as of now Axemen obsolete at Civil Service can still be useful through Medieval, but can be scrapped from Renaissance; Knights obsolete in Renaissance can remain useful through Renaissance, but can be scrapped from Industrial.
  - **Cargo-capable units are still kept**.
  - The rationale is that only very outdated units are no longer useful enough to justify their clutter or upkeep; until then, keeping units is usually better than short-term financial relief from scrapping, because they can defend cities, deter attacks, pressure rivals, or be spent in war.
  - War can also reduce excess units naturally, and successful captures can increase unit support, economy, and long-term growth more than deleting troops would.
  - So this strict-but-not-absolute scrapping gate is believed to be stronger for AI (at least in theory; in autoplay it does not seem weaker at a glance and based on the now enhanced BBAI logging we added as well).

#### Civilian Units (AI)

##### Settlers (AI)

- Avoid settling on bonuses (especially food/metal/high-production/commerce) unless the immediate overall site value clearly justifies it. See [KI#25](/_1_AdvCiv-SAS/Docs/README_Known_Issues.md#ki-25).
- **Unowned bonus extra valuation in city-site valuation:**
  - for example, if we already own Wheat, a site with 1 Maize is **much better for the whole empire** than a city site with 2 more Wheat, which would have **more yields locally for this city** (e.g., ~+3-4 extra food per plot), **but much less long-term empire growth** (e.g., +1 Health +1 Health with Granary for all cities) (which is perhaps better even for this local city).
  - Or, stone, happiness, or other bonuses could provide other advantages, for example **trading, military, accelerating buildables, etc**.
  - Trade imports do not count as owned because they are not permanent.
  - Applied once per bonus type and only after the first city.
  - See [KI#147](/_1_AdvCiv-SAS/Docs/README_Known_Issues.md#ki-147).
  - Update: now do not count as unowned the ones plausibly claimed by one of our existing cities' future BFC (see below bullet and [KI#178](/_1_AdvCiv-SAS/Docs/README_Known_Issues.md#ki-178)).
- **City-site bonus value now uses settlement-specific long-term value instead of broad trade value:** the old non-yield path reused `AI_bonusVal`, which is useful for immediate bonus trading but can be wrong for founding cities (for example health may be mostly useless early but very valuable late game, but short-term trading only the value of a bonus right now matters, the deal may be cancelled anytime or a bonus get obsolete later, but cities are founded generally early, and permanently, so `AI_bonusVal` seems unfit to be used for settler valuation).
- So AdvCiv-SAS now values explicit bonus health/happiness plus ordinary early building effects such as Granary/Harbor, while excluding late/expensive buildings through a tunable prerequisite-era gate (e.g., we'll likely benefit from Health from Harbor if city is Coastal and has connected Crab, but the game may end before we build a Supermarket for some bonus' effect so don't value it).
- In save file 450 BBAI testing, `AI_bonusVal` was massively, seemingly incorrectly valuing Elephants over Crab, despite the Crab site being better locally (food from bonus and greener, vs a desert/plains/hill site to get Elephants), so the AI preferred `(18,40)` Elephants/Rice/Camel site instead of seemingly better stronger `(17,40)` with Crab/Rice/Camel, this change now helped Shaka successfully found uMgungundlovu at stronger `(17,40)`, the log showed Crab and Rice receiving explicit `happyHealth=300 buildingHappyHealth=150` instead of relying on one opaque `AI_bonusVal` spike.
- (Requires AdvCiv-SAS 6577+) Update: Strategic importance is now added as a separate shared dynamic component rather than a static `iAIObjective` label, preserving the settlement-specific long-term health/happiness and yield layers while adapting military/infrastructure value to the civilization and current era.
  - See [Bonuses (AI)](/_1_AdvCiv-SAS/Docs/README_Main_Changes_Guide.md#bonuses-ai) and [KI#178](/_1_AdvCiv-SAS/Docs/README_Known_Issues.md#ki-178).
- Note: Special-building effects are estimated conservatively: religion-linked variants receive a lower per-point value because we are unlikely to build every overlapping variant, nonreligious special buildings receive more, and a shared cap prevents mod-added sets from inflating a site without bound; for example, Incense's 7 Cathedral happiness effects contribute 175 in total instead of 7 * 150 = 1050
- **Bonuses that can be connected soon enough are no longer heavily devalued in city-site scoring:**
  - Following the special building formula rework, save-file 453 BBAI logging also showed that Base AdvCiv valued Barcelona's Incense at only 70% of its health/happiness city-site value, reducing it from 325 to 228, even though Spain completed Calendar on the same turn that it founded the city and could benefit from it very soon.
  - In AdvCiv-SAS, `TECH_CALENDAR` is an early Ancient technology, not Classical, but even in rulesets where Calendar is later, this Base AdvCiv blunt inflexible penalty did not properly account for the actual XML rules; if the required technology was not currently researchable, Base AdvCiv used an even harsher value of roughly 33% which seems very excessive.
  - So the AI now applies a new XML-tunable penalty, as of now 15% per era until we can connect a bonus we can see, starting from the current era (so no penalty if it can be connected in this era, which seems correct as we'll be able to use it and benefit from it soon), one era ahead keeps 85%, two eras ahead keeps 70%, etc. In practice, most visible bonuses are no longer penalized because as of now in AdvCiv-SAS they can be connected soon after they are visible, while a bonus visible several eras before it can be connected still loses value gradually instead of falling into the old arbitrary 70% / 33% tiers.
  - The confirming run restored Incense to the intended full value of 325, preserved strong Barcelona at `(18,16)`, and gave all 66 logged connection evaluations no timing penalty because they were connectable in the current era.
  - See [KI#187](/_1_AdvCiv-SAS/Docs/README_Known_Issues.md#ki-187).
  - Note: this changes only health/happiness valuation; Food, Production, and Commerce from improving the bonus tile, and the difficulty of accessing it on another landmass, retain their existing separate rules.
- **Duplicate bonus value no longer over-counts reliable future-BFC coverage:**
  - trade imports still do not count as owned as in Base AdvCiv, but bonuses already owned or plausibly claimed by one of our existing cities' future BFC no longer receive another full empire-wide duplicate value.
  - Contested bonuses can still count when a known rival future BFC also threatens them, because ownership is not reliable.
  - This prevents new cities from chasing a safe duplicate Crab (better go for bonuses we don't have to grow more) while still letting a contested Camel near Arabia matter (don't count on it empire-wide, but still locally there's a chance we might steal it so value it enough for local valuation only, not empire-wide one as not reliable enough for that).
  - See [KI#178](/_1_AdvCiv-SAS/Docs/README_Known_Issues.md#ki-178).
- (Requires AdvCiv-SAS 6569+) **XML-driven city-site terrain, feature, and improvement value:**
  - Base AdvCiv valued natural yields, but estimated longer-term ordinary plot production and improvement potential through fixed shortcuts rather than enumerating the legal XML outcomes.
  - AdvCiv-SAS now values each usable BFC plot from its actual XML yields, retained feature, best legal Build/Improvement outcome, improvement upgrade chain, and how soon the required technology is available.
  - A modmod can therefore change or add terrain, features, Mines, Farms, Cottages, Lumbermills, Forest Preserves, Watermills, Workshops, or other improvements without maintaining parallel Settler hardcoding.
  - Earlier SAS tables that named Grassland, Plains, Desert, Tundra, Snow, Flood Plains, Forest, and Jungle had reduced Base AdvCiv's tendency to consume valuable Grassland/Hill Grass plots or keep weak Desert/Snow plots in the BFC, but their fixed values could become wrong after any XML balance change; the computed model preserves that strategic intent without a duplicate terrain balance table.
  - See [KI#26](/_1_AdvCiv-SAS/Docs/README_Known_Issues.md#ki-26).
- (Requires AdvCiv-SAS 6569+) **Dynamic weak-plot and first-capital quality checks:**
  - Candidate sites are significantly devalued, not automatically rejected, when too many non-home plots are impassable or remain below a tunable fraction of a sustainable productive plot after their best XML-valid improvement.
  - Visible usable bonuses remain separately valued rather than being called barren.
  - The first city additionally requires enough known good-enough or still-unrevealed plots, and its early growth decision uses the strongest six plot potentials rather than letting a playable core be diluted by every poor outer BFC plot.
  - See [KI#26.2](/_1_AdvCiv-SAS/Docs/README_Known_Issues.md#ki-26.2) and [KI#144](/_1_AdvCiv-SAS/Docs/README_Known_Issues.md#ki-144).
- **Respect water bonuses:** if a coast bonus would be in BFC but the site is non-coastal, apply a stronger penalty so we don’t "count" unusable sea bonuses. Penalty moved from **-165 → -400** (K-Mod used ~-800).
- **Low-food environments**: evaluate food scarcity from each non-home BFC plot's natural food and food added by bonus-specific improvements (e.g., tundra is low food, peak is very low food, but if there are bonuses that give food in the site it reduces the net food-scarcity score (e.g., high tundra high fish/deer may be better than grass/plain hybrid no food from bonuses so do not discard it)).
  - When an inland site has too much net food scarcity, **prefer coastal sites** so the city can grow instead of remaining at very low population.
  - See [KI#32](/_1_AdvCiv-SAS/Docs/README_Known_Issues.md#ki-32).
- (Requires AdvCiv-SAS 6569+) **Separate home-plot opportunity value:**
  - the plot consumed by the city is compared with what that plot could have produced when worked after its best timing-adjusted XML-valid improvement.
  - Burying a strong workable plot receives the full opportunity cost, while replacing a weak plot receives a deliberately smaller reward because the city might never have worked that marginal tile.
  - Under the current XML and default weights, examples compute to about **-40 for a riverside Grassland Hill**, **+4 for ordinary flat Grassland**, and **+52 for barren flat Desert/Ice Sheet**; these are calculated outcomes, not hardcoded terrain values, and automatically change with XML yields, legal improvements, and tuning weights.
  - This dynamically favors founding on a weak low-food plot or one whose city center gains a free hammer, while preserving valuable Hill Grass, Flood Plains, or mod-added equivalents when the surrounding BFC is otherwise comparable.
  - Resource-specific settle-on penalties remain separate, because founding on a bonus can also destroy improvement access and empire-wide resource value.
  - See [KI#25](/_1_AdvCiv-SAS/Docs/README_Known_Issues.md#ki-25) and [KI#26](/_1_AdvCiv-SAS/Docs/README_Known_Issues.md#ki-26).
- **Health tolerance:**
  - don’t reject good jungle sites just for early unhealth.
  - Allow a threshold (more strict for capital, looser later), and consider forest health offsets and flood plains exceptions.
  - Jungle can be chopped later; value the site’s potential, not just immediate health.
  - (Threshold examples: ~≤350 "healthpercent" for capitals; ~≤450 for later cities.)
- **Spacing:** remove old long-distance penalties that forced tight clusters. Cities are evaluated by distance to the nearest city (not the capital), with penalties only for "too close" or "way too far," strongest early and fading as the empire grows.
- **Settler evaluation — midgame desert/bonus bias fix**: Disabled the buggy `AIFoundValue::adjustToCivSurroundings` that, from about **turn 51+**, made settlers overvalue founding **on desert bonus tiles** (e.g., **Camel**) or **inside** desert belts.
  - Replaced it with a very small inline pass in its only caller, `AIFoundValue::evaluate`.
  - Observed effect: no longer plants directly on Camel/desert; instead settles **around** large deserts.
  - **Since the spacing change above**, also tightened distance-to-civs and culture-pressure checks to **hard-reject** sites that are **too far** or **heavily pressed** (instead of just penalizing).
  - See [KI#54](/_1_AdvCiv-SAS/Docs/README_Known_Issues.md#ki-54).
- (Requires AdvCiv-SAS 6569+) **Founding-plot defense update:**
  - Base AdvCiv's dedicated `AIFoundValue::evaluateDefense()` applied a fixed Hill-only bonus of 75, plus another 75 for a defensive leader personality (K-Mod used 100 + 100 and BtS used 200 without the personality split).
  - Earlier SAS commented out that function because its named terrain table already mixed yield, opportunity, and defense.
  - SAS now replaces the disabled fixed-Hill routine with defense derived from XML terrain defense plus the normal Hill defense, valued as a modest percentage of the site's workable-plot economy and doubled for a defensive leader personality by default.
  - This keeps defense useful without letting it overwhelm Food, Production, Commerce, bonuses, or the complete BFC; the Barbarian-specific policy is grouped in the Barbarians section above.
- (Requires AdvCiv-SAS 6401+) **Who builds Settlers / first trained Settler timing:**
  - Updated: only the **capital** can build Settlers.
  - Later Settlers keep the conservative low-population rule, but the crucial first trained Settler now uses a **soft population-5 preference** instead of the old AdvCiv-SAS "any positive growth below population 5 means wait" gate: if the next citizen is within about **5 Normal-speed growth turns** (scaled by game speed), the capital can finish that near growth first; slow or stagnant growth no longer suppresses a worthwhile second-city Settler.
  - Existing danger, war, financial and Worker safeguards still apply.
  - Same-save Great Plains A/Bs removed the observed healthy one-city/no-trained-Settler tail without broadly forcing expansion.
  - See [KI#181.2](/_1_AdvCiv-SAS/Docs/README_Known_Issues.md#ki-181.2).
- **Avoid excessively watery capitals:**
  - if fewer than **12** "good starter BFC tiles" (land + coastal bonus tiles, excluding the city tile), apply a strong penalty so the capital doesn’t start surrounded by mostly low-yield coast.
  - Other cities may still take such sites later.
  - See [KI#44](/_1_AdvCiv-SAS/Docs/README_Known_Issues.md#ki-44).
- **Additional site fundamentals:** value fresh water more (logic refactored/merged) and disable legacy "anti-crowd" biases so the choice stays primarily yield-driven.
- **First-city scoring**: value usable food bonuses, fresh-water city tiles, and river support more for the capital than for ordinary later city sites, because early capital growth snowballs into faster tile access, worker/settler production, whipping, and specialists.
  - See [KI#144](/_1_AdvCiv-SAS/Docs/README_Known_Issues.md#ki-144).
- **Removed map-generation spacing bias from live first-settler choices:**
  - keeping players' assigned starts far enough apart makes sense while generating the map, but after play begins their settlers should be free to found wherever they evaluate as best, whether closer to or farther from rivals.
  - BBAI logging in save file 442 showed that Berlin's greener, bonus-rich `(33,13)` site was valued higher than the `(35,11)` (**4569 to 3934**), but the old map-generation spacing penalties of **-1482 versus -480** reversed them to **3087 versus 3454**.
  - Removing this major cause of missed strong city sites helped but did not complete the Berlin fix alone; the smaller bonus-scoring and scouting changes as of now below then let the AI settler fully reveal and normally rank the stronger site, ultimately choosing `(33,13)` successfully without a Berlin-specific rule.
  - Old Base AdvCiv runtime behavior can optionally be restored (not recommended).
  - See [KI#173](/_1_AdvCiv-SAS/Docs/README_Known_Issues.md#ki-173).
- **Simplified bonus-improvement yield valuation:**
  - disabled AdvCiv's obscure nonlinear `evaluateSpecialYields` aggregation after the new SAS valuation made it redundant and retaining both paths heavily overscored bonus-rich sites.
  - The inherited path combined many hardcoded fixed-point weights, context branches, and another food modifier; it was difficult to understand and could only be tuned by editing and recompiling the DLL.
  - In the save file 360 Karakorum BBAI test, tundra-heavy `(49,43)` with Pig, 3 Elephants, and 2 Fur scored **6934 overall** (**7037** after more tiles were revealed), including **5015 bonus points** and **4239 from the old special-yield path**.
  - It therefore incorrectly beat the nearby river/Grass area: for example, Pig + Maize `(52,40)` scored only **4836**.
  - After simplification, `(49,43)` fell to **3835 overall**, including **2126 bonus points** and **1350 from actual bonus-improvement yields**; it then lost to nearby Pig + Maize river Grassland `(50,40)` at **4502**, which the AI settler chose.
  - The new capital has stronger food, river support, and greener long-term workable land instead of being pulled toward low-food Tundra by duplicate Elephants and Fur.
  - (Requires AdvCiv-SAS 6569+) Actual Food, Production, and Commerce added by a bonus improvement are now counted once; the plot's natural terrain/feature yield is handled by the shared XML-driven plot model.
- (Requires AdvCiv-SAS 6569+) **Layered XML tuning instead of recompilation:**
  - earlier SAS per-terrain home/non-home values were effective at discouraging cities from consuming valuable plots or retaining weak ones, but each fixed terrain number bundled natural yield, city-center yield gain, defense, and consumed-plot opportunity into one balance table that became stale when XML changed.
  - The new model separates those concerns and derives terrain/feature/improvement value from XML.
  - Unlike Base AdvCiv, SAS still exposes global Food, Production, and Commerce emphasis percentages that can broadly bias all Settler site yields, including bonus-improvement yields and scouting reference values.
  - Separate Self-Sustaining, One-Food-Short, and Default weights retain food-scarcity nuance; additional knobs control immediate versus mature improvement value, near/later-technology discounts, strong/weak home-plot opportunity, defensible-site value, first-city growth-core importance, fog information, and the local recheck range.
  - Defaults preserve the tested behavior, while modmods can change broad priorities without retuning every terrain or recompiling the DLL.
- (Requires AdvCiv-SAS 6569+) **Removed Base AdvCiv's separate first-era Commerce boost from city-site scoring:**
  - city sites persist into later eras, Commerce is already valued normally, and early Food and Production are at least as important because they drive growth, workers, settlers, defense, and military; the same fundamentals apply to Barbarian cities.
  - Applying an extra Commerce value only while evaluating the first two cities in the first era added complexity without a coherent long-term settlement priority.
- **Free window:** during the first ~75 turns at Normal speed (scaled by game speed), a "free to produce settler" period lets the capital grow before switching into chosen early strategy (offense, defense, etc.).
- **First city logic:**
  - settlers consider a larger no-penalty scouting radius for founding the capital (`CvUnitAI::AI_foundFirstCity`), and no longer auto-reject non-coastal/non-river sites if a worse "preferred" site exists.
  - It’ll just pick the **best** overall site.
  - See [KI#43](/_1_AdvCiv-SAS/Docs/README_Known_Issues.md#ki-43).
- (Requires AdvCiv-SAS 6569+) **First-city growth-core decision:**
  - the starting Settler distinguishes a genuinely strong opening core from a merely acceptable complete BFC.
  - The six strongest plots represent early workable growth; the ten strongest represent a broader developed city.
  - A nearby site can justify investigation when both cores improve enough to outweigh its lower currently known total value, preventing an attractive but consumable home plot or poor outer-ring tiles from ending the search too early.
  - In the Wang Kon test, this changed immediate founding on a valuable Grass Hill into a five-turn search that found a stronger two-food-bonus capital while preserving the hill as a workable plot.
  - This is yield/XML-driven rather than a hardcoded preference for rivers or named terrain.
- (Requires AdvCiv-SAS 6569+) **First-city scouting, recheck, and efficient return:**
  - Starting Settlers can roam briefly for better information or a better capital, but strong openings now found promptly rather than spending ten turns walking away and returning to the same site.
  - Safe scout steps value newly revealed fog in shared XML-derived plot units, the known site quality near their endpoint, and progress toward a stronger already-known candidate; direction is therefore based on prospective yields and bonuses rather than a large flat reward for revealing any tile.
  - Nearby candidates within the tunable local range are compared by full site and growth-core value without a redundant one-turn movement penalty.
  - Longer exploration remains bounded by the whole scouting-plus-return window; before another step, the Settler protects the strongest reachable known site and queues return movement plus founding together.
  - Empirical tests preserved or improved Berlin, Karakorum, Cuzco, Aztec, Aachen, and Wang Kon cases without map- or civilization-specific rules.
  - See [KI#144](/_1_AdvCiv-SAS/Docs/README_Known_Issues.md#ki-144).
- **Optional naval-heavy-map coastal settling bias:**
  - Naval-heavy maps can now add a tunable first-city or later-city preference for coastal settling in naval-heavy maps.
  - This should be more useful and strategically stronger in such maps, notably for coastal access, work boats, scouting/contact, trade, first galleys, Moai, or other water-focused strategies.
  - However, following the [KI#173](/_1_AdvCiv-SAS/Docs/README_Known_Issues.md#ki-173) and related AI settler/found scoring changes, BBAI logging showed even a value of `0` correctly selects the strong Cuzco coastal site in save file 420, while Japan no longer favors the old tundra-heavy candidates that motivated this new feature.
  - So use only a fairly small naval settling bias positive value for additional strategic coastal preference.
  - See [KI#146](/_1_AdvCiv-SAS/Docs/README_Known_Issues.md#ki-146).
- **Parking addressed (early game):**
  - Reduced cases of Settlers idling in the capital despite safe/urgent expansion, by updating `CvUnitAI::AI_found` so AIs would found sooner when conditions allowed.
  - In autoplay samples, Spain shifted from **1 city @T100** to **~2 by T60–70** and **~3 by T80–90** (Settler production varies somehow, but we move much sooner after we have it now).
  - Update: however, save file 450 BBAI testing showed that this old blind "go settle anyway" safety override solved one-city stagnation too bluntly: it could also allow exposed post-capital cities to be founded without escorts and then razed.
  - Exposed post-capital Settlers now wait, retreat, or require an existing guard-city mission instead of leaving/founding alone.
  - See [KI#68](/_1_AdvCiv-SAS/Docs/README_Known_Issues.md#ki-68) and [KI#179](/_1_AdvCiv-SAS/Docs/README_Known_Issues.md#ki-179).
- **Settler city-escort reassignment and production coordination:**
  - an unescorted Settler parked in one of its own cities can reassign one healthy local defender as escort when the donor city keeps enough healthy defenders behind.
  - (Requires AdvCiv-SAS 6401+) With the current tunable default of **2 healthy defenders remaining**, a city with 3 can lend 1 and keep 2.
  - Production now understands the same safety requirement: if a completed unguarded Settler lacks a detachable escort, the city explicitly requests and preserves the missing real defender instead of letting offensive-unit substitution satisfy the request incorrectly.
  - For the **first trained Settler**, the capital can also prepare a quick defender first when it can finish within about **5 Normal-speed train turns**; a slow defender does not postpone the Settler, and the inherited AdvCiv 1.14 coarse "extra quick defense" fallback no longer silently overrides that more precise SAS decision.
  - The movement-side safety requirement itself is unchanged.
  - See [KI#179](/_1_AdvCiv-SAS/Docs/README_Known_Issues.md#ki-179), [KI#179.2](/_1_AdvCiv-SAS/Docs/README_Known_Issues.md#ki-179.2), and [KI#179.3](/_1_AdvCiv-SAS/Docs/README_Known_Issues.md#ki-179.3).
- **Update: Exception to the above Settler-escort rule for danger-free founding:**
  - this does not conflict with or weaken the preceding rule, which still requires protection when a Settler is travelling toward or founding an actually dangerous exposed site.
  - The exception applies only after an unescorted Settler has already reached a valid site with actual plot danger 0; an unowned plot is not automatically dangerous.
  - Save-file 449 BBAI logging showed why this distinction matters: a lone Indian Settler reached the safe island site at `(44,10)` around turn 142, but the first AdvCiv-SAS KI#179 rule treated every unowned site as dangerous.
  - It remained exposed while waiting or moving for more than 100 turns and founded Agra only on turn 257.
  - The Settler now founds immediately because waiting there does not protect it and only forfeits the new city's growth and yields.
  - In the confirming run, Agra was founded on turn 144; save file 450 also confirmed the other side of the rule, as Nobamba was founded with 2 healthy defenders and another Zulu city on a `targetDanger=12` plot was guarded.
  - See [KI#194](/_1_AdvCiv-SAS/Docs/README_Known_Issues.md#ki-194).
- **Settler valid-tile trap avoided:**
  - a Settler already standing on a foundable plot no longer immediately settles there when another reachable city site is clearly better.
  - Save file 450 BBAI testing showed that after a `(7,45)` Barbarian city spawned, the planned Zulu city 3 space became much smaller and poorer; the AI Settler still founded weak tundra-heavy `(10,47)` anyway, despite a much better reachable `(10,41)` site with unowned Sheep plains and Hill Grass.
  - It now travels to and tries to settle this stronger site instead.
  - See [KI#180](/_1_AdvCiv-SAS/Docs/README_Known_Issues.md#ki-180).
- **Settler no longer bounces back to a near weak site:**
  - the first direct-found guard fixed the bad `(10,47)` founding, but revealed that the existing `AI_found` movement scoring could still pull the Settler back to the same near weak site because it was only one turn away, causing back-and-forth movement without settling for 10-20+ turns.
  - Fix: the same raw-value check now also prevents a very near but much weaker city site from winning for travel-distance reasons.
  - As a result, the confirming run shows Zulu city 3 is founded quickly at the `(10,41)` unowned Sheep plains and Hill Grass site; Zulu AI then conquers the strong nearby `(7,45)` Barbarian city, which has unowned Horse, unowned Deer, unowned Silver, and unowned Gems (that appear during the game), instead of settling a useless cramped tundra city near it; and by ~T131 becomes top 2 player with a strong empire instead of being among the weakest due to loss of early momentum and bad early cities.
  - See [KI#180](/_1_AdvCiv-SAS/Docs/README_Known_Issues.md#ki-180).
- **Post-capital Settlers can scout promising nearby fogged city sites and avoid stale cached targets:**
  - some time after KI#178 improved uMgungundlovu's bonus valuation, save file 450 run changed and exposed a remaining information/movement issue: `(17,40)` could look slightly worse than `(18,40)` only because more of its BFC was unrevealed: BBAI showed `(18,40)` evaluated at total `3064` with 19 revealed / 1 unrevealed non-home BFC plots, while `(17,40)` was only total `2959` with 17 revealed / 3 unrevealed; however, the average evaluation per revealed plot was higher for `(17,40)` (2959 / 17 = 174.04 which is higher than 3064 / 19 = 161.25) so it would have been better to explore the remaining unrevealed nearby plots rather than founding the better explored current site immediately, like a human player would, which is now fixed: post-capital AI Settlers now optionally (as of now default enabled as likely stronger) explore such adjacent promising sites, then recalculate normally and choose the best after enough info is known (the old one remains the best if the new one turns out in fact not to have been better).
  - The follow-up guard also matters: if the Settler reaches a plot that is now directly better than the cached selected target, it founds there instead of walking away to the stale target.
  - In the confirming run, Shaka scouted `(17,40)`, then founded uMgungundlovu there at value `4099` instead of stale adjacent `(18,40)` at `3993`, gaining Crab/Rice/Camel and a better food profile (as in KI#178).
  - This is similar in spirit to first-city scouting, but narrower: later Settlers usually know more of the map and should not become broad roaming scouts.
  - See [KI#185](/_1_AdvCiv-SAS/Docs/README_Known_Issues.md#ki-185).
- (Requires AdvCiv-SAS 6401+; XML-scaled update in AdvCiv-SAS 6569+) **Settler production floor for weak early/midgame sites:**
  - unlike Base AdvCiv, SAS prevents AI cities from training later early/midgame Settlers merely because one low-value legal filler site remains.
  - The crucial **second city is treated separately**: by default a one-city empire uses normal AdvCiv minimum-found-value logic, while the stricter AdvCiv-SAS floor begins once the empire already has **2 cities** and stops applying from Renaissance onward.
  - The 6569 update replaces the old fixed `2000` and optional first-expansion values with XML percentages of a reference BFC made of sustainable productive plots; the default later floor is still exactly 2000 under current yields, but automatically follows changes to citizen food consumption and the shared Settler yield weights.
  - This preserves the anti-filler purpose of the original France/Carthage tests without letting an absolute number become stale after XML balance changes or trapping constrained one-city starts behind a threshold intended for optional later cities.
  - See [KI#181](/_1_AdvCiv-SAS/Docs/README_Known_Issues.md#ki-181) and [KI#181.2](/_1_AdvCiv-SAS/Docs/README_Known_Issues.md#ki-181.2).

##### Workers (AI)

- Strongly discourage building **forts on bonuses** (very poor early yield/time trade). *(Mostly a guardrail now that worker logic is overhauled, but still helps prevent regressions.)* See [KI#24](/_1_AdvCiv-SAS/Docs/README_Known_Issues.md#ki-24).
- **Outside-BFC fort-airbase path disabled by default:** AdvCiv-SAS worker logic already protects BFC plots from **fort** choices which are deemed **inefficient** so never chosen; this also optionally disables the old **late-game path** where workers could build **forts outside BFC for airbase/canal use**: we prefer **not risking future irrigation-chain paths** or spending worker turns on **rarely useful** defensive modifiers, scattered airbases, or fort canal/isthmus effects.
- (Requires AdvCiv-SAS 6402+) Update: Normally improve **bonuses first**, prioritizing **food** bonuses.
  - Major refactor of `CvUnitAI::AI_bestCityBuild` to use a probabilistic value tree tuned for AdvCiv-SAS.
  - The productive-feature Phase 0 below is the deliberate exception: during Worker/Settler production, timely stored feature hammers can be more valuable than immediately connecting another bonus.
- (Requires AdvCiv-SAS 6577+) Update 2: Bonus jobs now use the same marginal dynamic value as Settlers, trade and strategic AI rather than fixed `iAIObjective`, so first copies and currently relevant civilization-specific unlocks receive priority while obsolete resources, substitutes and extra copies naturally lose urgency.
  - See [Bonuses (AI)](/_1_AdvCiv-SAS/Docs/README_Main_Changes_Guide.md#bonuses-ai), [KI#30](/_1_AdvCiv-SAS/Docs/README_Known_Issues.md#ki-30), and [KI#33.2](/_1_AdvCiv-SAS/Docs/README_Known_Issues.md#ki-33.2).
- (Requires AdvCiv-SAS 6565+) **Worker improvement choice is now XML-driven instead of a hardcoded terrain/feature decision tree:**
  - `AI_bestCityBuild` enumerates the currently legal improvement Builds for each plot and values their actual Food, Production and Commerce results.
  - City-specific yield scarcity, build time, whether the plot is worked, improvement upgrade chains and the value lost by replacing an existing improvement all influence the result.
  - An automatically upgrading (e.g. a Cottage maturing into a Hamlet and gaining commerce yield) improvement receives two-thirds of its immediate yield and one-third of its final XML upgrade yield, keeping a new Cottage valuable without pretending it is already a Town; food below citizen consumption is charged for the surplus another plot must supply, naturally preferring self-supporting land without naming Grass or Plains.
  - See also [PR#40](https://github.com/wonderingabout/AdvCiv-SAS/pull/40).
  - (Requires AdvCiv-SAS 6565+) Watermills, Workshops, Lumbermills, Forest Preserves and mod-added or rebalanced improvements can therefore become useful automatically when their XML yields or validity make them competitive; adding/removing a terrain, feature or ordinary economic improvement no longer requires duplicating its identity and expected yields across C++ branches.
    - Fort-like infrastructure remains outside ordinary BFC economic competition, and a replacement deadband plus later-era lower-food guard prevent small score changes from restarting Farm/Cottage or Mine/Windmill oscillation.
    - See [KI#30](/_1_AdvCiv-SAS/Docs/README_Known_Issues.md#ki-30) and [KI#33](/_1_AdvCiv-SAS/Docs/README_Known_Issues.md#ki-33).
- (Requires AdvCiv-SAS 6402+, updated in AdvCiv-SAS 6565+) **Productive-feature Phase 0 converts abundant early feature hammers into useful tempo while preserving a reserve:**
  - once a Worker can legally perform a pure feature removal that gives production to the city, AdvCiv-SAS checks this before ordinary route/tile work instead of relying only on the inherited city-side chop valuation that the custom `AI_bestCityBuild` pipeline no longer consumes.
  - During **normal production**, useful bonus improvement still gets first claim and the higher default threshold (`6`) makes a very feature-heavy city spend only the excess stock before returning to Cottages/Farms/Mines and other development (e.g. 8 eligible features naturally fall toward about 5).
  - (Requires AdvCiv-SAS 6565+) During food-production **Worker/Settler** builds, the lower default threshold (`3`) runs before bonus/route logic and can spend down toward about 2 reserves to accelerate the key civilian.
    - Existing Worker targets count against the uncommitted stock; direct relief of current unhealth/unhappiness can bypass the reserve; feature-preserving bonus improvements are protected; and Phase-0 movement uses `MOVE_TO` rather than laying a Road on the way.
    - Once the stock falls below the hard-priority threshold, the remaining plots return to the ordinary XML-driven evaluator instead of becoming permanently invisible; it can preserve a feature-valid improvement or clear the feature when the complete plot result justifies it.
    - The rule is feature/build driven rather than hardcoded to Forest/Jungle names.
    - In the final Joao validation, the balanced version kept the T27 first Settler while Portugal also resumed ordinary development: by T50 it had 3 improved worked plots instead of 1 in the earlier over-chopping prototype, and by T100 it had **3 cities, 10 improved worked plots, and 2 worked Forest reserves**.
    - See [KI#33.2](/_1_AdvCiv-SAS/Docs/README_Known_Issues.md#ki-33.2).
- One worker per improvement **cap** to avoid over-stacking on slow tiles and reduce capture risk.
- **Reliable Worker availability without early overproduction:**
  - AI cities rebuild enough Workers for each land area before other production priorities can leave them short indefinitely, without producing more than the AI calculates it needs.
  - Maintaining enough Workers throughout the game lets the AI improve food/production/commerce, connect bonuses and routes, repair pillaging, and develop new or underdeveloped cities sooner, especially after Worker losses or earlier underproduction.
  - This originally aimed for 1 Worker per city plus a capital-area reserve.
    - As a refinement, a one-city empire needs at most 1 land Worker until its capital has ever reached a tunable population (5 by default), avoiding early double-Worker production and the resulting halted growth; normal demand resumes after the capital matures or another city is founded.
    - Ordinary Worker choices, cases where a blocked Settler choice falls back to a Worker, general unit selection, and Worker scrapping now use the same centralized target.
    - Four BBAI test runs then completed exactly 1 early Worker per empire, with the only later automatic second Worker beginning when its capital reached population 5.
- Update: **AI primary landmasses now maintain roughly 2 Workers per city instead of roughly 1:**
  - a [Base AdvCiv test report](https://forums.civfanatics.com/threads/ai-city-placement-and-misc-suggestions.695343/page-7#post-16782814) had already shown a long-established city remaining almost unimproved.
  - Save file 459 later reproduced this severely in AdvCiv-SAS: Russia still had only 3 Workers for 6 cities on turn 220, with 58 of its 91 worked plots unimproved on turn 200.
  - This exposed both a minor SAS Worker-demand mismatch and the old standing minimum of only 1 Worker per city plus 1 reserve being too low.
  - To fix this, the AI now counts currently worked featureless land toward Worker demand, and its primary landmass keeps an XML-tunable floor of 2 Workers per city plus a signed reserve.
  - Worker demand, production limits and scrapping share this floor.
  - Initial testing with a +1 reserve greatly improved Russia's land but could delay its second Settler.
  - Further save-file 459 comparisons showed that producing more Workers developed plots sooner but generally hurt population, production, military and expansion.
  - Save file 463 confirmed the final reserve: raising only the 3+-city value from -1 to 0 improved more city-radius land on turn 100 (45.6% versus 39.5%), but left only 35 cities / 176 population / 167 production instead of 41 / 199 / 210; by turn 150 that improvement advantage had disappeared while -1 still led by 57 cities to 52 and 468 population to 415.
    - The final independently tunable `-1 / -1 / -1` reserves provide 1 Worker at 1 city, 3 at 2 cities and 5 at 3 cities, then add 2 per additional city.
    - A neglected jungle or new city may still reveal a local Worker-assignment issue, but globally producing another Worker proved worse.
    - The early one-city safeguard and demand-driven secondary landmasses remain.
    - See [KI#198](/_1_AdvCiv-SAS/Docs/README_Known_Issues.md#ki-198).
- **Workers are no longer delivered to landmasses that already have enough:**
  - in save file 450, BBAI logging showed a Worker accompanying the Settler to the island where Thapsus was founded, then remaining idle there for 93 turns because none of its plots could be improved.
  - Our first attempt at this fix correctly left an unnecessary Worker on the large landmass (i.e. home) instead of sending it to the small unimprovable island, but exposed a separate problem: general Worker logic kept loading the Worker back onto the island-bound Settler transport, so our early fix had to unload the same Worker back onto the large landmass 14 times.
  - After our second version correctly stopped that loop, testing exposed a third failure: a separate Worker-ferry routine still kept sending extra Workers to islands that already had enough, eventually crowding 5 Workers onto Sardis's two-tile landmass and 2 onto Ergili's three-tile landmass, where most had no useful work.
  - To fix this, Settler transports now leave Workers beyond the destination's exact shortage in their original city, unload only the required number at the destination, and keep any excess aboard for another destination.
    - Workers cannot repeatedly reboard the same transport after being left behind, and later Worker ferries now stop once the destination landmass has enough.
    - These checks use current need and current Worker count, so delivery becomes eligible again if losses leave the landmass short or new work requires workers again.
    - In the confirming run, the AI correctly left a Worker on its origin landmass because the destination needed 1 but already had 3.
    - See [KI#192](/_1_AdvCiv-SAS/Docs/README_Known_Issues.md#ki-192).
- **Workers are brought home after finishing every useful task on a secondary landmass:**
  - Base AdvCiv forced most landmasses to retain at least 1 Worker forever, even after a Worker had legitimately finished developing a small island and could contribute elsewhere.
  - Secondary landmasses can now reach zero Worker demand when no useful task remains.
  - Their finished Workers move to the coast and request pickup; one available Settler transport collects each Worker for reuse elsewhere, rather than several ships pursuing the same rescue.
  - The primary landmass keeps its Worker reserve, and demand on a secondary landmass automatically returns if losses or new work require another Worker.
  - In the confirming save file 450 run, Persia recovered all 5 Workers from its completed islands on the same turn, and 14 completed-area Worker boardings were recorded across the game.
  - See [KI#192](/_1_AdvCiv-SAS/Docs/README_Known_Issues.md#ki-192).
  - Note: the above zero-demand and pickup rule above does not apply to the Barbarian player, as Barbarian city improvement planning can report no local jobs until a Worker already exists, and Barbarians do not use the ordinary civilization system to transport and reassign finished Workers between separate landmasses; therefore each isolated Barbarian island keeps local Worker demand (else no Worker is built and Barbarian cities are not improved at all).
- (Requires AdvCiv-SAS 6565+) **Yield-first build execution:**
  - Base AdvCiv 1.14's next-city Worker path could change travel toward an already selected improvement into `MISSION_ROUTE_TO` merely because the Worker started outside the target plot group, stood on an unroaded source plot, or passed a distance-based random roll.
  - Its separate `AI_betterPlotBuild` postprocessing could also replace the chosen build with a Road in several contexts.
  - The first AdvCiv-SAS simplification pursued the right yield-first goal empirically, but sometimes left important bonuses unroaded; later connection patches restored those Roads while some incidental routing remained difficult to distinguish.
  - The new exhaustive, pre-gated BBAI route diagnostics identify the accepted caller and reason, allowing the AI to preserve deliberate city-pair Roads, bonus connections, improvement-yield Routes and later fallback network work while removing only the harmful next-city detours.
  - For SAS-scored next-city jobs, Workers now move directly to the target and execute the exact selected Chop, Mine, Farm, etc. Two diagnostic autoplays had recorded **1,850** and **1,608** route-before-improvement missions on the old path; the clearest visible failure was a Worker spending scarce early turns roading from a movement-costly Grassland Hill and then leaving without constructing its valuable selected Mine.
  - This wastes both Road-building turns and the Hill entry/movement investment while delaying Food/Production/Commerce that helps every subsequent city turn.
  - See [KI#31](/_1_AdvCiv-SAS/Docs/README_Known_Issues.md#ki-31).
- (Requires AdvCiv-SAS 6565+) **Yield balance follows the city rather than fixed tile identities:**
  - Food, Production and Commerce prices respond to the city's current yield balance.
  - Production scarcity adds a bounded preference for strong production plots, so a low-hammer city or small island can develop its scarce Grassland Hills or Workshop-capable land sooner: another high-food flat plot cannot substitute when the city specifically lacks a usable hammer source.
  - The pressure is proportional to the missing strong-production options and ends as they are built, so it improves diversity instead of permanently making Mines dominant.
  - (Requires AdvCiv-SAS 6565+) In the Pangaea BBAI replay, Berlin at population 1 had 0 strong-production plots against a target of 1; the pressure raised its Production weight from 141 to 181, and a Grassland Hill Mine scored 1398 versus the Cottage runner-up's 912.
    - Once the first strong plot existed on turn 14, the weight returned to 141; when population later raised the target to 2, the pressure returned until the second plot was built.
    - Istanbul similarly reached Production weight 224 while missing both target plots at population 6, then fell to 186 after developing two of its target three.
  - (Requires AdvCiv-SAS 6565+) Food remains valuable for growth, food-consuming production plots and specialists; XML improvement-upgrade chains supply their own longer-term value instead of naming the Cottage chain in C++.
    - Automatically upgrading improvements also receive XML-derived maturation-level replacement and irrigation-overwrite protection, so mod-added Food-, Production- or Commerce-growing chains are protected without a named Cottage root or configurable growth-chain list.
    - The remaining strategic thresholds are exposed through focused SAS XML defines rather than reproducing XML yields in code.
- (Requires AdvCiv-SAS 6573+) **Worker improvement replacements use transition-aware, generic anti-oscillation valuation instead of the old later-era lower-food guard:**
  - ordinary `CITY_BUILD` replacements are scored with the midpoint of the city's current and prospective Food/Production scarcity, so a Farm, Workshop, Mine, Windmill or mod-added improvement cannot look attractive mainly because constructing it would immediately remove the shortage that gave it its premium.
  - Active BFC improvement work is pre-credited for Food and strong-production supply so parallel Workers do not all react to the same temporary shortage.
  - Replacing useful infrastructure must also clear both an absolute and percentage improvement margin, with extra cost for remaining Worker build time and XML improvement maturation.
    - This damps marginal A→B→A retools without making named improvements permanently "holy": large food emergencies and other genuinely stronger states can still justify a replacement, while the separate AdvCiv-SAS irrigation-chain planner can still build required carrier improvements through its own route valuation.
    - In the controlled Tiny Islands comparison through turn 370 after the irrigation-dependency fix, <=5-turn reversals fell from **34 to 6**, total reversals from **111 to 41**, and cross-path reversals from **17 to 3**.
    - The rule reduces churn rather than forbidding later adaptation when the city's real circumstances change.
- (Requires AdvCiv-SAS 6574+) **Replacement Food forecasting also accounts for productive specialists and temporarily unworked improvements:**
  - non-free specialists remain real Food demand, while only angry-citizen consumption is discounted for long-lived Worker planning. When replacing an existing improvement, the prospective Food calculation includes that plot's yield change even if citizens are not working it at that exact moment.
  - This prevents a useful Food improvement from looking falsely expendable merely because citizens temporarily move to specialists or another tile, only for the replacement to be worked and recreate the Food shortage; the rule remains generic across XML-defined improvements.
- (Requires AdvCiv-SAS 6565+) **Feature handling is XML-driven too:**
  - the Worker discovers legal pure-removal Builds and values the current feature's actual plot yields, rounded/fractional city health and happiness effects, turn damage, delivered production and Worker time.
  - An improvement that requires the feature, such as a valid Lumbermill or Forest Preserve, keeps it unless a genuine replacement passes the normal gain/hysteresis gate; a harmful or economically inferior feature can be removed without a Jungle/Fallout name check.
  - (Requires AdvCiv-SAS 6565+) Tiwanaku exposed the old failure clearly: two Lumbermills constructed on turns 161-162 were independently chopped on turn 186 even though their Cottage replacements had been rejected, then the Cottages had to be built on turn 190.
    - Feature-valid infrastructure is now preserved, while a Build that really should remove the feature can still use a separate pure-removal step and retain its selected improvement as a flexible follow-up.
    - This preserves the strategic health trade-off of Forest-like features while allowing mod-added Marshes, productive Jungles or differently balanced Fallout to work from their loaded data.
- **Compare the best useful work across all cities:**
  - workers now compare each city's strongest reachable job by its complete Food/Production/Commerce/bonus/terrain evaluation, with only a modest travel cost, instead of favoring cities mainly by population or staying near weaker local work.
  - Cities that lack enough completed or already-assigned improvements for their population receive proportionally more help; counting assigned workers discourages crowding one city, while reserving one worker per plot prevents several workers from targeting the same tile.
  - In BBAI testing in save file 442, this redirected workers from low-value unused Tundra Workshops toward valuable Grass/Plains Hill Mines in underdeveloped cities.
  - See also [KI#39](/_1_AdvCiv-SAS/Docs/README_Known_Issues.md#ki-39) and [KI#41](/_1_AdvCiv-SAS/Docs/README_Known_Issues.md#ki-41).
- **Worker mobility / overimprovement refinement:**
  - workers still rotate away from sufficiently improved small cities, but medium and large cities are less likely to leave useful BFC plots unimproved.
  - Workers also avoid ordinary replacement churn (e.g. **Mine ↔ Windmill**, **Cottage ↔ Farm**) while valid unimproved BFC work remains, except for bonus fixes and irrigation-chain Farms. The small-city rotation shortcut does not make a worker leave when it is already standing on this same city's useful BFC plot and that plot is the city's best current job.
- (Requires AdvCiv-SAS 6565+) **Finish valuable followup builds after feature removal without blindly chopping every feature:**
  - when the selected improvement itself removes the feature, or independently removing a harmful/valuable-to-clear feature has positive value and the selected improvement does not require it, the Worker can perform the standalone XML-defined removal first—for example receiving Forest chop production sooner rather than waiting for a combined Mine Build.
  - It then retains the worthwhile Mine, Farm, Cottage, bonus improvement, etc. as a follow-up for the same plot.
  - (Requires AdvCiv-SAS 6565+) This preserves flexibility to redirect the Worker after receiving the removal benefit, but prevents it from clearing a strong tile, walking away, and paying another trip later when the original job remains worthwhile, which is especially inefficient when Hill movement is involved.
    - Feature-preserving Lumbermills, Forest Preserves, Camps and mod-added equivalents bypass this removal-first path.
    - Weak ordinary follow-ups are still reconsidered instead of being forced, and queued follow-ups wait for the Worker's next turn when removal consumes all movement rather than being silently discarded.
    - See also [KI#176](/_1_AdvCiv-SAS/Docs/README_Known_Issues.md#ki-176).
- (Requires AdvCiv-SAS 6565+) **Useful owned BFC plots across water are no longer stranded by land-area Worker accounting:**
  - the same Worker evaluator can test one off-area BFC plot without pretending it is already pathable.
  - A transport can ferry a Worker when that isolated plot has worthwhile legal work, and a Worker whose small island area is complete can request evacuation for reuse elsewhere.
  - The motivating Archipelago run left Tianjin's only non-city land plot unimproved through turn 350 because the legacy city build cache reported no demand; after the actionable-plot and ferry changes, analogous one-land-plot Arpinum received its Workshop immediately.
- **Retreat logic:**
  - fix excessive retreat → "parking in city" edge cases with a wake-from-retreat and saner thresholds.
  - See [KI#50](/_1_AdvCiv-SAS/Docs/README_Known_Issues.md#ki-50).
  - Update: while this was meant initially to help reduce excessive Worker parking or inactivity, this may have also caused excessive or avoidable Worker deaths, so danger now handling also treats current plot danger as real even inside owned culture, while still ignoring broad inside-border threat-radius noise that caused too much retreating; in save-file-450 BBAI testing, this prevented a Worker from continuing a build on a currently dangerous plot and reduced comparable t200 Worker deaths from 40 to 17.
- **Earlier Worker irrigation-chain hunting:**
  - AdvCiv-SAS initially improved on BFC-only Worker logic by starting from actual dry-Farm / low-food BFC targets and hunting short owned irrigation-chain steps, including first links outside the city BFC.
  - This fixed Chaco Canyon and Harappan-style cases where a nearby water source existed 2-3 plots beyond the BFC, and retained only the strongest connector toward each target instead of treating alternative routes as separate urgent jobs.
  - However, this was still a short heuristic rather than a complete route planner and was later replaced by the follow-up below.
- (Requires AdvCiv-SAS 6565+) Updated: **Workers plan and complete long irrigation routes for dry city Farms and other XML-defined carriers:**
  - BBAI logging had shown that the preceding short-chain hunt could select one nearby connector without preserving a complete route, and it excluded bonus-specific Farms. In save file 450, Mayapan's Maize consequently remained dry (i.e. not irrigated) from turn 121 through turn 312.
  - The AI calculates the least-cost owned route from an actual dry BFC carrier to available irrigation, builds the irrigation source-side improvement first, and recalculates after each completed link so the water advances toward the intended plot.
  - (Requires AdvCiv-SAS 6565+) Owned flat cities can carry the route, while replacing valuable improvements is discouraged and excessive routes are prevented by an XML-tunable maximum.
    - Mayapan completed its five-link route by turn 162, and save files 449 and 452 confirmed longer bonus routes while leaving only targets genuinely blocked by geography or foreign/unowned plots.
    - Carrier and target discovery now follows every improvement whose XML sets `bCarriesIrrigation`, and replacement penalties follow configured XML upgrade chains; Farm remains the current ordinary carrier but is no longer a C++ requirement.
    - See [KI#196](/_1_AdvCiv-SAS/Docs/README_Known_Issues.md#ki-196).
- (Requires AdvCiv-SAS 6572+) **AdvCiv-SAS irrigation chains preserve carriers that still keep downstream plots connected:**
  - this improves the custom SAS deterministic chain system described above, not Base AdvCiv's older `AI_irrigateTerritory` routine.
  - Before ordinary city-yield logic replaces an irrigated XML carrier with a non-carrier improvement, the Worker AI tests the remaining potential-irrigation component as if that carrier were removed.
  - If an owned downstream carrier would then have no surviving fresh-water source, the provider is kept.
  - Providers that currently supply the source-side step of a still-pending SAS irrigation route are likewise protected until the next link is completed.
  - Flat city tiles can bridge these dependencies, while redundant or obsolete connectors remain replaceable.
  - (Requires AdvCiv-SAS 6572+) The motivating Tiny Islands autoplay exposed repeated interference between the custom SAS chain planner and ordinary city-build valuation.
    - With the dependency protection enabled, Barcelona's repeatedly rebuilt `(8,63)` carrier fell from **55 direct reversals to 1** through turn 370; cross-path reversals fell from **67 to 17**, reversals within 5 turns from **86 to 34**, total reversals from **146 to 111**, and replacement assignments from **587 to 486**.
    - `SASGameRecord` stayed identical through turn 151 and first diverged on turn 152, exactly when the new city-bridged carrier protection first changed gameplay.
    - Remaining rapid Farm/Workshop reversals initiated entirely by ordinary `CITY_BUILD` yield weighting are a separate Worker-economics issue rather than being hidden by broader irrigation protection.
    - See [KI#196 (Update 2)](/_1_AdvCiv-SAS/Docs/README_Known_Issues.md#ki-196).

##### Other Civilian Units (AI)

- **Great People Golden Age wait cap:**
  - AI Great People can still wait a few turns for a Golden Age partner (i.e. another Great Person type necessary to start a Golden Age, because in Civ4, later Golden Ages can require more than one Great Person) when that is valuable, but no longer wait indefinitely when the partner does not arrive.
  - After a tunable normal-speed (because it scales with game speed) **cap**, **Golden Age reservation stops blocking lower but useful actions**, so the existing Great Person logic can settle/join, discover, build, trade mission, Great Work, etc. instead.
  - This fixed BBAI-log cases where Persia held Great People for 29-32 turns because **Golden Age value kept blocking all alternatives**, while **preserving useful short waits**.
  - See [KI#154](/_1_AdvCiv-SAS/Docs/README_Known_Issues.md#ki-154).

#### Military - Common (AI)

Note: UWAI means Utility-Based War AI, Base AdvCiv's system for deciding when to start, continue, or end wars.

It gives each possible rival a UWAI war drive: an overall estimate of how advantageous preparing that war would be after considering expected gains and risks such as the rival's power, distance and access, diplomatic relations, other wars, and victory pressure.

Assuming the calculation is correct and reliable, the higher the drive, the more willing the AI is to prepare an attack against that rival.

- Avoid **settlers at critical war timings**: bias `AI_bestUnit` away from settlers when war is likely; prepare for war instead. Related: settler is a **national unit (1 max)**. See **[KI#36](/_1_AdvCiv-SAS/Docs/README_Known_Issues.md#ki-36)** and **[Civilian Units](#civilian-units)**.
- Disable unreliable **UnitAIs** for AIs: `UNITAI_COLLATERAL`, `UNITAI_PILLAGE` (but despite that, AI still seems to occasionally pillage), `UNITAI_PIRATE_SEA` to reduce suicides/baiting and keep stacks focused.
- Retune UnitAI **selection/weights** in `AI_bestUnit`, `AI_bestUnitAI`, `AI_chooseProduction`: land maps emphasize `UNITAI_ATTACK` / `UNITAI_ATTACK_CITY` (with some `UNITAI_RESERVE`); defense mode emphasizes `UNITAI_CITY_DEFENSE` / `UNITAI_CITY_SPECIAL` / `UNITAI_RESERVE`; naval UnitAIs are downweighted on land-focused maps.
  - Goal: fewer wandering/baited units; more decisive stacks.
- **Dedicated land anti-air units stay in anti-air roles:**
  - save file 459 exposed a long-standing composition issue in which Saladin reached 41 Mobile SAMs among 108 combat units on turn 350, and BBAI traces showed Mobile SAM/SAM Infantry could be selected as generic `UNITAI_ATTACK`, `UNITAI_ATTACK_CITY`, or `UNITAI_RESERVE` units instead of only through their specialized anti-air roles.
  - Positive XML `UnitAIs` are not an exclusive whitelist because Base AdvCiv/K-Mod `AI_unitValue` can still accept other generic combat roles through fallback logic.
  - SAM Infantry and Mobile SAM therefore now explicitly add those three generic roles to `NotUnitAIs`; their intended `UNITAI_COUNTER`, `UNITAI_CITY_COUNTER`, and `UNITAI_CITY_SPECIAL` roles remain unchanged.
  - Follow-up save-file 459 logging showed no dedicated AA selections into the forbidden roles.
  - See [KI#199](/_1_AdvCiv-SAS/Docs/README_Known_Issues.md#ki-199).
- (Requires AdvCiv-SAS 6450+) **Peaceful dominant AIs stop endlessly adding fresh land combat units once their primary landmass is already overwhelmingly armed:**
  - many different production branches can ultimately request a unit, so AdvCiv-SAS now applies one conservative saturation brake at the shared concrete `AI_chooseUnit` gate instead of duplicating limits across callers.
  - It only blocks a fresh land combat unit for a master AI that is at peace, has no war plan or local danger, is adequately defended, has no major military strategy/victory push, is on its neutral primary landmass, knows every independent rival still present there, has at most one independent local rival bloc by default, already exceeds a large era-scaled land-army stock threshold, and has at least 125% of the combined known local rival-bloc power (or no independent local rival remains).
  - The stock threshold rises from 4 units per city in Ancient to 10 in Future, with +2 under Aggressive AI; the tested Aggressive-AI Future case therefore needed at least 12 relevant land units per primary-landmass city before suppression was even possible.
  - Multiple independent local blocs, any unmet local rival, war preparation, active war, danger or military push immediately keep/restore normal production, so crowded/Pangaea dogpiles, Space/culture-victory denial and conquest preparation remain unrestricted.
  - A stronger rival on another continent no longer by itself forces still more home-continent land units; naval/air projection remains governed separately.
  - In two fix2 controls the gate produced 0 would-rejects; two enabled Huge Archipelago runs produced 51 and 68 actual rejects, all Future-era and only in the intended peaceful/safe states.
  - One run stopped rejecting on turn 316 immediately before Ethiopia adopted a war plan on turn 317; the other stopped on turn 323 before the next war plan on turn 324.
  - China was also correctly caught at 190% local power despite being only 62% as powerful as the strongest global rival, confirming that local rather than worldwide power is the relevant comparison for home-land land saturation.
  - A separate same-seed rerun of the older Isabella example produced no Spain rejection when that old saturated state failed to recur, while its control identified one qualifying Scandinavian case instead, supporting that the brake is state-dependent rather than leader-specific.
  - See [KI#53.5](/_1_AdvCiv-SAS/Docs/README_Known_Issues.md#ki-53.5).
- (Requires AdvCiv-SAS 6451+) **Safe over-funded AIs now prefer immediately needed cheap infrastructure over one more discretionary offensive land unit:**
  - when an AI master is at peace with no war plan, danger, under-defense, military strategy/victory push, Settler-escort context or secondary-landmass pressure, and its military spending is already at least 15 points above the normal `AI_maxUnitCostPerMil` allowance, the shared concrete `AI_chooseUnit` gate can reject a fresh offensive land unit if the city has a non-limited health/happiness/maintenance building that finishes in the same number of city-specific production turns or faster.
  - Health/happiness require an actual local deficit; maintenance requires at least 3.00 city maintenance, a real negative maintenance modifier and focused value >=50.
  - The veto does not force a building: it simply lets ordinary `AI_chooseProduction` continue, preserving Workers/Settlers, defenders, naval/air needs, projects/processes, invested units and normal building priorities.
  - Two dry-run controls identified 410 qualifying unit decisions; two enabled Huge Archipelago/Custom Continents runs produced 502 unique vetoed city-turns, of which 396 (79%) immediately ended on a building and 314 (63%) on the exact qualifying building.
  - The effect emerged naturally from relative production time—zero Ancient/Classical cases, then 9 Medieval, 42 Renaissance, 107 Industrial, 231 Modern and 113 Future—so no arbitrary era switch is used.
  - The tested 100% building-to-unit time ratio remains the default rather than broadening to 125-150% without evidence.
  - See [KI#197.12](/_1_AdvCiv-SAS/Docs/README_Known_Issues.md#ki-197.12).
- (Requires AdvCiv-SAS 6453+) **Safe over-funded AIs also give very cheap high-return food/production/gold/research infrastructure another chance before one more discretionary offensive land unit:**
  - this softer follow-up to the hard health/happiness/maintenance rule keeps the same peace, primary-neutral-landmass, local-safety/defense, no-war-plan, no-military-push, no-Settler-escort and non-vassal protections plus the same `AI_maxUnitCostPerMil + 15` overspending requirement.
  - Because these economic focuses have no binary deficit, the building must be materially cheaper: at most 75% of the exact unit's city-specific construction time, focused `AI_buildingValue` >=50 and at least +20 value from that focus versus the same building's ordinary value.
  - The gate still does not force the building; it only declines that one fresh offensive unit and lets normal production continue.
  - Four dry-run control logs contained 184 qualifying production evaluations and every one still ended on a unit.
  - In the enabled Huge Archipelago/Custom Continents pair, 97 unique production evaluations were vetoed; 51 (53%) immediately switched to a building and 28 (29%) to the exact qualifying building, while the remaining unit results were exempt needs, invested units or faster alternatives that no longer met the 75% gate.
  - The median qualifying building took 66% of the rejected unit's time, focused value was 120, focus gain +34 and military overspend +82; activations were 0 Ancient/Classical, 2 Medieval, 4 Renaissance, 13 Industrial, 56 Modern and 22 Future.
  - See [KI#197.13](/_1_AdvCiv-SAS/Docs/README_Known_Issues.md#ki-197.13).
- **AI military production now catches up when an empire becomes under-strength for its size:**
  - dedicated BBAI military-production logging together with SASGameRecord exposed a Base AdvCiv/K-Mod asymmetry: relative-power logic could reduce unit production when an AI was already strong, but there was no opposite catch-up when expansion outpaced army growth.
  - This could leave a rapidly expanding empire with too little military for the amount of territory and cities it needed to defend.
  - To address this, the AI now compares its power per city with the city-weighted average of relevant rivals and proportionally increases its normal willingness to build units while under-strength, while still respecting financial/economic reductions, the better-units strategy and the inherited high-power throttle.
  - In two full post-fix runs from save file 458 and an independent post-fix run from save file 457, this catch-up activated in about 46-47% of logged military-production evaluations when applicable.
  - For example, one save-file 458 state had Kublai at 9 cities and 579 power (about 64 power per city versus 125 for the relevant-rival average), increasing his BuildUnitProb from 47 to 70; save file 457 independently showed Shaka at 7 cities and 377 power (about 54 versus 94 per city), increasing it from 57 to 81.
  - In practice, under-strength AIs therefore became noticeably more willing to reinforce their armies until they caught up, while already-strong AIs received no boost.
  - See [KI#197](/_1_AdvCiv-SAS/Docs/README_Known_Issues.md#ki-197).
- **The AI's final important-building retry now respects military-production pressure while another unit still fits its spending budget:**
  - military-production logs showed that Base AdvCiv/K-Mod could reach the end of city production with strong reasons to build units, then give an important building one final chance that ignored that military pressure: in `CvCityAI::AI_chooseProduction`, short-circuit 2 already lowers building odds as `iBuildUnitProb` rises, but short-circuit 3 - the final "last chance to catch important buildings" immediately before generic unit production - recalculated its building chance from building value and land-war state alone.
  - Strong military-production pressure could therefore survive the earlier production logic and then be partly forgotten at the final building retry.
  - In the severe under-strength samples that exposed the issue, this late building retry won 7/9 and 7/8 times in two save-file 458 runs and 15/25 times in save file 457.
  - To fix this, the AI now reduces only this final building chance in proportion to its existing willingness to build units, and only while the normal military-spending gate still has room; once the AI is already over that unit budget, the inherited building odds are left unchanged.
  - The final save-file 458 validation left 375/640 already-over-budget evaluations untouched, while severe under-strength cases that still had spending room fell from about 48% average base building odds to about 33% and chose the building only 5/25 times.
  - This keeps important infrastructure possible while preventing the last building retry from disproportionately overriding the military pressure already used by the surrounding production logic.
  - See [KI#197.2](/_1_AdvCiv-SAS/Docs/README_Known_Issues.md#ki-197.2).
- **Late floating-defender production now respects military-spending limits unless floating defenders are severely below need:**
  - dedicated BBAI military-production logging exposed a Base AdvCiv/K-Mod inconsistency in `CvCityAI::AI_chooseProduction`: the earlier floating-defender pass checks unit spending before adding more defenders, but the later 50% retry did not.
  - In the pre-fix save-file 457 run, 153 of 281 actual floating-defender-2 choices occurred at or above the normal `max+15` allowance, including 97 peaceful cases; save file 456 had 85 of 243 above the same threshold, including 70 peaceful cases.
  - One extreme save-file 456 example had Persia only one floating defender short (6 available versus 7 wanted) while unit spending was already 220 against a normal maximum of 84, yet the late retry still produced another military unit.
  - To fix this, ordinary peaceful shortages now require the normal `max+15` spending headroom, while land wars and severe floating-defender shortages may still use the retry; severe shortage uses the same threshold as the earlier floating-defender pass.
  - In the final save-file 456 validation, 143 ordinary peaceful over-budget candidates were blocked and every actual over-budget choice was explained by the intended war/severe-shortage exceptions.
  - Save file 457 independently reduced actual over-budget choices from 153/281 in the old run to only 12/233, all explained by 10 severe shortages and 2 land-war cases.
  - See [KI#197.3](/_1_AdvCiv-SAS/Docs/README_Known_Issues.md#ki-197.3).
- **Generic peaceful city-hunting-stack production now respects military-spending limits:**
  - BBAI military-production logging exposed another Base AdvCiv/K-Mod spending bypass in `CvCityAI::AI_chooseProduction`: the generic "city hunting stack" heuristic can request `UNITAI_ATTACK` / `UNITAI_ATTACK_CITY` before the later generic military-spending gate, so it could keep growing an offensive stack even when the AI was already far over budget and not at war.
  - In the pre-fix save-file 456 run, 9 of 53 actual `start city attack stack` choices occurred at or above `max+15`; only one was during a land war, leaving 8 peaceful over-budget stack starts.
  - The clearest example was Roosevelt on turn 100: America had 3 cities, no land war and no immediate danger, unit spending 200 against a normal maximum of 76 (`max+15=91`), yet this branch still ordered another `UNITAI_ATTACK_CITY` Axeman.
  - To fix this, generic city-hunting-stack production now requires the normal `max+15` spending headroom while at peace, while land-war behavior remains unrestricted; the separate bounded target-specific Barbarian-city attacker logic remains unchanged.
  - In the final save-file 456 validation, 391 peaceful over-budget evaluations were blocked while 188 over-budget land-war evaluations remained eligible, and none of 44 actual generic stack starts were peaceful and over budget.
  - Save file 457 independently blocked 351 peaceful over-budget evaluations while preserving 20 over-budget land-war evaluations; none of its 45 actual generic stack starts exceeded `max+15`.
  - See [KI#197.5](/_1_AdvCiv-SAS/Docs/README_Known_Issues.md#ki-197.5).
- **Naval-trade sea-explorer fallback now counts docked/queued ships and stops once the relevant water area is fully revealed:**
  - BBAI military-production logging exposed an inherited Base AdvCiv/K-Mod edge case in the special naval-trade explorer branch.
  - Its `<3` safety cap counted only units physically in the water area and it never required any unrevealed water, so it could ignore a substantial docked/queued navy and still build late `UNITAI_EXPLORE_SEA` ships after exploration was complete.
  - In the pre-fix save-file 456 run, 25 of 32 actual choices occurred with zero unrevealed water and 22 of 32 occurred despite at least three actual sea-domain ships including docked/queued units; one turn-258 example had Germany build a 200-hammer Attack Submarine while the main ocean was fully revealed and three naval units were already docked.
  - To fix this, the branch now counts actual sea-domain ships associated with the water area, including docked and queued ships, and requires at least one unrevealed water tile while preserving the inherited three-ship cap and 25% roll.
  - In the confirming save-file 456 rerun, only 44 of 437 evaluations remained eligible and 8 actual choices were made; every one had unrevealed water and fewer than three corrected sea units, while three successful choices still had `seaExplorerTarget=0`, confirming that the distinct trade-exploration fallback remains active where useful.
  - See [KI#197.6](/_1_AdvCiv-SAS/Docs/README_Known_Issues.md#ki-197.6).
- **The AI's early important-building short-circuit now respects military-production pressure while another unit still fits its spending budget:**
  - continued save-file 454 diagnostics exposed that `CvCityAI::AI_chooseProduction`'s short-circuit 1 could still start a fresh building from 0 production even when `BuildUnitProb` had reached 100 and the AI had substantial military-spending headroom.
  - In the pre-fix run, short-circuit 1 chose a building in 10 of 70 evaluations with `BuildUnitProb=100`; 6 of those buildings had 0 production invested, including Birka starting a Forge, Jail, Jewish Monastery, Jewish Temple and Factory while maximum military pressure was already being requested.
  - To fix this, the AI now reduces only this early gate's base building odds in proportion to `BuildUnitProb` while the normal `max+15` military-spending allowance still permits more units.
  - The inherited production-progress bonus inside `AI_chooseBuilding` remains unchanged, so an already-started important building can still be completed, and being below the spending allowance does not itself force military production.
  - In the confirming save-file 454 rerun, 211 short-circuit 1 evaluations had `BuildUnitProb=100` with spending room; 209 rejected the building, while the only 2 successful choices were already 50% and 35% complete.
  - A turn-160 Nidaros example reduced a fresh Granary from 23% base odds to 0 and subsequently selected a Crossbowman through the normal military path.
  - See [KI#197.7](/_1_AdvCiv-SAS/Docs/README_Known_Issues.md#ki-197.7).
- **Under-strength AIs with affordable military headroom now react more strongly even when their personality has a low base BuildUnitProb:**
  - KI#197's power-per-city catch-up was multiplicative, so a peaceful leader could remain relatively timid because even a large military deficit multiplied into a small base probability.
  - To fix this, the AI now converts part of the same capped power-per-city deficit into additional absolute BuildUnitProb points only while the normal `max+15` spending allowance still permits more units; spending remains a brake rather than a target to fill, and existing financial/economy, `GET_BETTER_UNITS`, war-possibility, high-power, and drafting gates remain intact.
  - In save file 459, Mali on turn 200 was about 34% behind relevant rivals in power per city: its personality/XP value of 29 rose to 39 through the existing multiplier and then to 56 through the new +17 affordable response while spending remained 0 against a local maximum of 82.
  - Across that run the absolute response activated in 2,669 of 5,838 logged BuildUnitProb evaluations (about 46%), averaged about +10 points when active, and was blocked 439 times by the affordability brake after reaching that check.
  - This improves catch-up without pretending that `AI_maxUnitCostPerMil` is a literal desired army size; broader mid/late healthy-empire buildup remains separate follow-up work.
  - See [KI#197.8](/_1_AdvCiv-SAS/Docs/README_Known_Issues.md#ki-197.8).
- Rebalance perceived **iPower**: downweight some naval units; adjust certain land units (e.g., warrior, animals) to reduce **fake power** that triggers bad wars. (Requires AdvCiv-SAS 6594+) See [Long_Comments_XML.txt](/_1_AdvCiv-SAS/Docs/Long_Comments/Long_Comments_XML.txt) (search for `UnitAI notes (Units XML)`).
- XML **DefaultUnitAI / UnitAIs / NotUnitAIs**: align roles with efficiency (e.g., swordsmen primarily offensive even if versatile; longbows defensive unless explicitly set otherwise).
- Promotions logic in `CvUnitAI::AI_promotionValue`: add **hard rules** so AIs pick context-appropriate promotions (e.g., City Garrison for `UNITAI_CITY_DEFENSE`, City Raider for `UNITAI_ATTACK_CITY`; avoid Woodsman/Medic on front-line attackers unless context justifies).
  - Rules apply per **UnitAI** (not just by unit type); with exceptions like recon.
  - See **[KI#47](/_1_AdvCiv-SAS/Docs/README_Known_Issues.md#ki-47)**.
- **War target selection — distance guard**: added a **distance-based pre-filter** in `Risk::preEvaluate` (WarUtilityAspect.cpp) so the AI **rejects faraway rivals** during war-risk evaluation.
  - This curbs "bait" wars that leave cores under-defended and keeps armies concentrated near relevant fronts.
  - See [KI#61](/_1_AdvCiv-SAS/Docs/README_Known_Issues.md#ki-61) and [KI#183](/_1_AdvCiv-SAS/Docs/README_Known_Issues.md#ki-183).
- **War target selection — distance guard** - Update: **Faraway-war Risk reject no longer inflates UWAI war utility:**
  - a follow-up BBAI test in save file 450 showed the older AdvCiv-SAS KI#61 hard reject could also fire in the recursive peace scenario.
  - Since UWAI scores war as `war scenario - peace scenario`, this could turn a mediocre war into an absurdly attractive one, e.g. `warScenarioUtility=-220`, `peaceScenarioUtility=-105386`, `finalUtility=105166`.
  - The hard reject no longer applies to the recursive peace scenario; retesting removed all huge positive final-utility artifacts (`positive final: 0`) while preserving the intended negative reject for bad faraway/unreachable war plans.
  - See [KI#183](/_1_AdvCiv-SAS/Docs/README_Known_Issues.md#ki-183).
- Update 2 : later BBAI logging showed that the guard was too invasive after war had already begun.
  - In save file 450, it forced Sumer to make peace after only 2 turns despite capturing New York with twice Lincoln's power, merely because the next enemy city was beyond the distance limit.
  - It is now restricted to pre-war evaluations, leaving normal UWAI logic to decide when an ongoing war should end.
- Update 3: the pre-war contact-distance guard is now substantially less restrictive and XML-tunable at a current, to address the issue, for example in save file 459, AIs having an overwhelming military advantage but not attacking much weaker rivals.
  - The previous `SAS_UWAI_WAR_TARGET_MAX_LAND_CONTACT_TURNS_UNSCALED_GAMESPEED` 3-turn land limit could hard-reject otherwise worthwhile reachable wars before normal UWAI evaluation simply because the army's estimated path to the nearest target city took slightly longer.
  - To fix this, `SAS_UWAI_WAR_TARGET_MAX_LAND_CONTACT_TURNS_UNSCALED_GAMESPEED` / `SAS_UWAI_WAR_TARGET_MAX_SEA_CONTACT_TURNS_UNSCALED_GAMESPEED` are now changed to much higher defaults of 15 (vs 3 before) approximate path turns for land-reachable wars and 20 (vs 12 before to match this change too) for genuinely naval wars.
  - The wider limits preserve a sanity bound for very remote targets while allowing ordinary UWAI power, utility, diplomacy, closeness and war-state logic to decide whether a reachable target is actually worth attacking.
  - See [KI#61](/_1_AdvCiv-SAS/Docs/README_Known_Issues.md#ki-61).
- **UWAI target choice avoids faraway wars:**
  - Following [KI#61](/_1_AdvCiv-SAS/Docs/README_Known_Issues.md#ki-61)'s related fix, BBAI war-target logging in save file 450 showed the AI could still prepare some avoidable faraway wars: for example, on turn 182, Arabia (Team 0) could ignore closer Holy Rome (Team 6) at distance 5, attitudeValue -2, and targetPowerPercent 64, then prepare against the farther Netherlands (Team 5) at distance 10, attitudeValue +1, and targetPowerPercent 92.
  - That is dangerous because a faraway war can split armies from the core, slow reinforcements, expose cities to opportunistic invasions, or make captures harder to hold.
  - The new guard only optionally promotes a nearby land-war target, not an island/naval target, when it is close enough, weak enough, disliked/neutral enough, and has minimal UWAI drive; With the current distance threshold, it only steps in when the alternative target is at least 2 tiles farther away; targets at the same distance or closer are left to normal UWAI.
  - This is intentionally a narrow blunder-fix rather than a war-logic overhaul, so UWAI choices do not become too bland or broadly overridden.
  - In the confirming BBAI run, the guard found 32 valid closer targets and blocked 17 farther alternatives, leaving usual UWAI logic to either pick these closer targets or continue normally if no suitable target remained.
  - See [KI#182](/_1_AdvCiv-SAS/Docs/README_Known_Issues.md#ki-182).
- **Early-game razing — distance-aware captures**: Updated conquer/raze logic so that, for roughly the first **~120 turns** (scaled by game speed), the AI **razes captured cities** that aren’t **close enough** to justify early investment, in `CvPlayerAI::AI_conquerCity`.
  - This addresses the base AdvCiv pattern where forces overextend (most troops sit in a faraway capture while **core cities** thin out) and also preserves scarce early economy (city maintenance, low pop, time to build improvements/culture).
  - Indirect effect: less fog-busting far from us → **more barbarians** there, which tends to pressure rivals.
  - Net: a **stronger, more focused** early AI (fewer "split in two" empires).
  - See [KI#64](/_1_AdvCiv-SAS/Docs/README_Known_Issues.md#ki-64).
- **Captured-city razing avoids destroy-then-resettle waste:**
  - BBAI testing in save file 450 showed AIs repeatedly destroy a city and then soon spend a Settler to rebuild on the exact same `(38,50)` plot: Ur was razed, Tours was founded there and razed, then HRE founded Augsburg there only four turns later and eventually razed it too.
  - This wasted population, buildings, Settlers, regrowth, and military effort.
  - The cause was Base AdvCiv's `AI_isAwfulSite`, which misevaluated and judged existing cities through a separate crude terrain veto that could conflict with how Settlers valued the same site.
  - AdvCiv-SAS replaces it with `AI_isSASCityLikelyToBenefitUsLongTerm`, whose intrinsic-quality component reuses the shared SAS low-food and potential-yield helpers: close cities with enough long-term food or workable potential can be kept even without standout bonuses.
  - In the confirming run, HRE kept Ur when it captured the city at population 3; Ur later reached population 13, built a corporation building, and generated a Great Engineer, supporting a stronger AI outcome instead of another wasteful raze/resettle cycle.
  - See [KI#186](/_1_AdvCiv-SAS/Docs/README_Known_Issues.md#ki-186).
- **Excessively faraway captured cities are rejected regardless of intrinsic site quality:**
  - the same BBAI investigation exposed a separate strategic problem: a city can have excellent food, bonuses, population, and developed plots yet still be a poor long-term acquisition when it is too far from our nearest city on the same land area.
  - Defending it can exhaust units and split the army from the core; if a nearer rival then captures it, that rival receives an already-developed city without paying for its Settler or initial growth.
  - As for us, we lose diverted investments and end up thinner and weaker.
  - `AI_isSASCityLikelyToBenefitUsLongTerm` therefore applies a tunable same-area distance limit independently of intrinsic quality.
  - Razing such an outpost lets the AI take the conquest gold and move on, avoiding its ongoing maintenance and defensive commitment, preserving more military flexibility, and forcing rivals to spend their own Settler and development time.
  - In the confirming run, Yue-Chi at same-area distance 17 and repeatedly conquered Orleans at distance 10 were razed, and neither location was resettled nearby through turn 300.
  - Save-file 453 also confirmed that even an attractive, seemingly moderate-distance capture can be a trap: temporarily raising the tunable limit from 8 to 9 made Inca keep population-17 Cordoba as a possible bridgehead, but two immediate Ottoman captures killed 10 advanced Incan units and left the developed city transferred back to Spain, so the tested default as of now remains 8.
  - See [KI#186](/_1_AdvCiv-SAS/Docs/README_Known_Issues.md#ki-186) and [KI#186.2](/_1_AdvCiv-SAS/Docs/README_Known_Issues.md#ki-186.2).
- (Requires AdvCiv-SAS 6288+) Update: Among exceptions, as in Base AdvCiv, if the AI is closing in on a Domination win, do not raze cities and keep them even if they are bad: overexpansion threat is small because we are strong, but the risk of missing a close Domination win, or losing to other rivals by Space or other victory conditions is higher.
  - This edge case surfaced in late-game autoplays: an AI already close to a Domination win could still raze city after city even when it was no longer meaningfully threatened by overexpansion.
  - The effect could also snowball, because a razed city never becomes a new nearby anchor, so the next capture can remain even farther from the nearest owned city and fail the same distance check again.
  - Isabella exposed the problem especially clearly: while pursuing Domination she first razed Smolensk at only 9 plots from her nearest same-area city against the tested limit of 8, then razed 10 captured cities in a row from turns 389-421, including population-14 Dur-Kurigalzu, population-12 Moscow, and population-13 Coventry, throwing away land and population that could have helped finish the victory.
  - Simply raising the normal distance limit is deliberately not the fix: an earlier 8->9 test made Inca keep exposed population-17 Cordoba, lose 10 advanced units defending it, and lose the city anyway.
  - After having fixed this AdvCiv-SAS Domination raze veto needed regression, in the confirming full-game run, Ramesses rather than Isabella benefited most: once the override became active he kept 16 primary-area city conquests from turn 347 through 454 and won Domination on turn 457 at 47.03% land / 53.72% population against 47% / 48% requirements.
  - See [KI#186.3](/_1_AdvCiv-SAS/Docs/README_Known_Issues.md#ki-186.3).
- **Active-World-Wonder exemption no longer protects unsustainable faraway captured cities:**
  - BBAI logging in a follow-up save-file 450 run showed the concrete failure.
  - England captured the faraway, population-18 city of Elephantine, but an inherited Base AdvCiv rule automatically protected captured cities containing active World Wonders from being razed, even though the city was too far away to benefit its new owner long-term: England committed 15 defenders, but eventually lost at least 7 of them there plus lost Elephantine back to nearby Egypt after only 6 turns anyway.
  - So the AI now optionally disables this Wonder-based protection for faraway cities while preserving it for captures close enough to be sustainable (default because likely stronger and as testing showed too).
  - Razing lets the AI take the conquest gold, permanently deny the Wonder to rivals, and avoid the city's maintenance and defensive commitments and potential future losses and risks, instead of exhausting its army to preserve the Wonder briefly for the nearer rival likely to retake it.
  - Save file 450's run diverged so it could not be reproduced after fix, but Save-file 453 directly confirmed the new path when Inca razed population-17 Madrid at distance 13 despite its active Statue of Zeus.
  - See [KI#186.2](/_1_AdvCiv-SAS/Docs/README_Known_Issues.md#ki-186.2).
- **Unit upgrades - stop wasting gold on tiny gains:**
  - Fix AI tendency to **over-upgrade units** even when the upgrade is **poor value** (high gold cost for a small strength gain), which can burn many turns of **gold** and **slow tech pace**.
  - Before allowing an upgrade, the AI must pass **simple ROI gates** based on **strength** gained vs gold cost: (1) gold per 10% strength gain, and (2) gold per +1 **flat strength** gain; both gates are **more lenient with higher unit EXP** (high-EXP units keep their promotions after being upgraded which is valuable, so spending more gold on them is more justified), while low-EXP units are treated more strictly (often better to let them die and rebuild if needed, unless it is cheap enough to upgrade them).
  - Example (Knight -> Cuirassier): 10 strength -> 13 strength; if one of the gates requires `15 gold per +1 flat strength gain` with `+1 gold per +1 flat strength gain per 1 EXP`, then we won't upgrade the unit if its upgrade cost is more than `15 * 3 = 45 gold` at **0 EXP**, or `((15 + 18) * 3) = 99 gold` at **18 EXP** for this gate.
  - In autoplay, this seemingly leads to **all AIs teching significantly faster**, especially in the **late game** where there are many units and AI would often have its tech pace stall for dozens of turns.
  - Change in `CvUnit::canUpgrade` (single global gate, like `CvUnit::canScrap`); see [KI#88](/_1_AdvCiv-SAS/Docs/README_Known_Issues.md#ki-88).
- **Unit upgrades - prefer the important domain for the map:**
  - AI unit-upgrade value now optionally uses the existing **SAS map-script heaviness** logic to **relatively value** unit upgrades (not a hard ban unless relative value is set high): the real gold price is unchanged; this makes the AI think actual upgrade cost is higher for these unit upgrades.
  - As a result, on **land-heavy maps, naval upgrades are treated as lower priority**, so the AI is more likely to **spend gold upgrading land units first** instead of modernizing old ships that have little strategic value on that map.
  - On **naval-heavy maps, the same idea can apply in reverse for land upgrades**.
- **Remote captured-city attack stacks:**
  - large AI attack **stacks in faraway captured cities no longer let upgrade waiting override a ready offensive target**.
  - If the stack is large enough, has a valid target, and the target is not too strong, it keeps using the existing attack movement logic **instead of parking most of the empire's army far from its core** because several units could upgrade, which was very problematic.
  - In the BBAI-log sample, the Celtic Boston trapped-stack pattern was visible from T156 to T256; the stricter remote-trap diagnostic confirmed it from T172 to T214, with up to 41 units / 62% of Celtic military parked in a faraway captured city while still having ready targets.
  - See [KI#155](/_1_AdvCiv-SAS/Docs/README_Known_Issues.md#ki-155).
- **Ready attack stacks stop parking for unavailable upgrades:**
  - if a city-attack stack is ready, the target is not too strong, and **0 units can upgrade now**, the AI now keeps moving toward the target instead of waiting for possible future upgrades.
  - This fixes real offensive freezes: in one BBAI-log sample, USA Washington waited from **T147-T181** with up to **59 units / 69% of total military** while targets were only **3-5 turns** away; Mali Gao waited from **T233-T255** with up to **42 units / 41% of total military**.
  - The AI still waits for real immediate upgrades: a USA Seattle stack waited while **6 units could upgrade now**, then attacked/moved once that number became **0**.
  - This is thought to be much stronger, because it uses the current military advantage before enemies catch up; old units can be spent in war instead of sitting around draining upkeep, needing future upgrade gold (solved if they die as well), and risking obsolescence; successful captures can then increase economy, unit support, and long-term growth; and the needless wait was very problematic and inefficient too.
  - See [KI#156](/_1_AdvCiv-SAS/Docs/README_Known_Issues.md#ki-156).
- **AI unit upgrades - stop routine upgrades from draining the treasury:**
  - Fixes an AI spending problem where normal unit **upgrades could overshoot the intended upgrade budget**.
  - Previously, the AI only checked whether it was still below budget before starting the next upgrade, but did not check whether that next upgrade itself fit the remaining budget.
  - For example, in BBAI diagnostics, an AI with 280 gold reserved for upgrades could spend 237 gold, still count as under budget, then start another 237 gold upgrade and end up spending 474 gold total, sometimes leaving only a few gold in the treasury.
  - Routine **upgrades now check the actual remaining upgrade budget before upgrading**.
  - **Emergency**-style upgrades, such as important city defenders or special impassable-unit upgrades, **remain more flexible**, but normal upgrades are much less likely to waste all available gold.
  - AIs in **financial trouble also avoid** routine upgrades **unless they are focused on war**.
  - See [KI#160](/_1_AdvCiv-SAS/Docs/README_Known_Issues.md#ki-160).
- **AI attack stacks now opportunistically capture nearby barbarian cities:**
  - before this fix, ready city-attack-capable (i.e. not just `UNITAI_ATTACK_CITY`) stacks could sit idle because the AI had **no normal war target yet**, even with a **nearby pathable barbarian city**.
  - Now, if the AI has **no normal target** and is **not already busy with an active war**, the stack can **attack that barbarian city instead**.
  - This does not override normal wars or normal war targets; it fills the no-target idle gap.
  - This change is usually a **strong** move: barbarian cities are often weak, cost little diplomatically, and capturing them can give land, population, bonuses, unit support, and future growth without spending a settler or interrupting city production nor growth.
  - In the tested save before the fix, nearby barbarian cities survived at game end at ~T400 despite reachable AI armies to Napoleon and Shaka, and barbarian being very weak in comparison, yet bonuses-rich, and Shaka was among weakest players; after the fix, Zulu captured Hurrian at T127, Alemanni at T132, Visigoth at T148, and Gaul at T153, turning those nearby barb cities into an early expansion boost and making Shaka the strongest player for almost all middle game and end game, almost allowing him to win.
  - This change should make AI significantly stronger.
  - See [KI#158](/_1_AdvCiv-SAS/Docs/README_Known_Issues.md#ki-158).
- **AI armies no longer wait indefinitely in one city while preparing only for Barbarian conquest:**
  - SASGameRecord and BBAI logging in save file 453 showed an unfinished Base AdvCiv rule in `CvUnitAI::AI_attackCityMove`, amplified by K-Mod grouping `AI_omniGroup`, kept armies assembled to capture a Barbarian city waiting merely because they had grown larger than needed for their weak target; for example, when the estimated Barbarian garrison was two units and five attackers were otherwise enough, those five could move, but adding a sixth made the entire army "not ready" again.
  - While the army waited, normal grouping kept sending additional units into it, making it even larger and preventing those units from defending elsewhere or forming another expedition.
  - In save file 453, individual waiting groups reached 65-82% of an empire's military - up to 201 units - while the AI paid their upkeep and delayed conquering reachable weak Barbarian cities.
  - To fix this, the AI now divides such an army once into several self-sufficient expeditions that can capture different Barbarian cities, defend territory, or prepare for later wars, and prevents them from immediately joining back together.
  - Normal large armies remain allowed during active wars and when a planned surprise attack is ready.
  - See [KI#188](/_1_AdvCiv-SAS/Docs/README_Known_Issues.md#ki-188).
- **Capable early armies can organize themselves to capture Barbarian cities:**
  - SASGameRecord and BBAI logging also found AIs with 9-14 units individually able (`UNITAI_ATTACK`) to attack and capture cities, but none assigned to organize the others (`UNITAI_ATTACK_CITY`) into a city-assault army, so this is a difference in `UNITAI` assignment, not their abilities.
  - A K-Mod rule automatically broke down any group of 4 or more such units into smaller groups of 2 or 3 units if the group was in territory owned by the AI or one of its teammates.
  - Because of this, many of the AI's military units could remain at home instead of grouping together to capture a reachable Barbarian city.
  - To fix this, if no city the unit can reach within one turn needs it as a defender and the unit is not currently threatened, the AI now checks if enough attackers are available in the area, and if so, the AI assigns that unit as the expedition leader; the others can then group with it and they set out against a Barbarian city.
  - For example, the save file 453 confirming run created four such expedition leaders.
  - Among them, one was of a Spanish Chariot reassigned on T60 had gathered a four-unit group by T61, redirected toward Minoan on T73, and Spain captured it on T77.
  - Another was of a War Elephant reassigned for the Holy Roman Empire on T79 was leading four units toward Khoisan by T86, and the Holy Roman Empire captured it on T90.
  - See [KI#188](/_1_AdvCiv-SAS/Docs/README_Known_Issues.md#ki-188).
- **War preparations now take priority over Barbarian expeditions:**
  - Base AdvCiv/K-Mod treated the period before a planned attack became ready as if no rival war were being prepared.
  - For example, in save file 450, BBAI logging showed Hannibal preparing to attack nearby Willem, whose power was only 54% of Hannibal's, while 7 of Hannibal's units were still being managed for an expedition to the Barbarian city Scythian 22 tiles away instead of building enough strength for the much more profitable rival war.
  - To fix this, the AI now uses normal war readiness as soon as it has any real war plan.
  - In the confirming run, the affected groups of 6-9 units did not leave for distant Barbarian cities.
  - Instead, they continued assembling enough strength for the planned rival war and remained nearby in case they were needed for defense.
  - See [KI#188.4](/_1_AdvCiv-SAS/Docs/README_Known_Issues.md#ki-188.4).
- **AIs no longer do charity Barbarian cleanup for their rivals:**
  - Following the readiness fix above, BBAI logging exposed a broader problem: a fully prepared army could still spend many turns attacking a distant Barbarian city that it would not benefit from and ultimately raze.
  - For example, in save file 450, BBAI logging showed that Hannibal mobilized 3 units on turn 74 against the Barbarian city Libyan, 16 tiles away.
  - Reaching and capturing it took 16 turns, and Hannibal then razed it.
  - He gained no city and only some conquest gold, but lost troops and time that could have supported offense or defense.
  - Shaka, the only nearby player likely to keep the city, gained much more: he founded Bulawayo on the exact plot only 10 turns later without having to capture Libyan himself.
  - With the nearby Barbarian pressure removed, Shaka could also focus on his economy or build up his army without losing units to Barbarian attacks or becoming easier prey for other rivals, and he eventually won this run by Space.
  - To fix this, the AI now avoids trips longer than the new XML-tunable limit of 3 path turns (to preserve quick tactical raids even when the city is not worth keeping) unless `AI_isSASCityLikelyToBenefitUsLongTerm` says the city is worth keeping (as of now, generally because it is close enough and has enough useful land and food after bonuses are considered).
  - In the confirming run, Hannibal did not send his units to clear those distant Barbarian cities around turn 74.
  - Instead, he kept them available for potentially better attacks or defense.
  - Much later, between turns 168 and 195, he captured 5 Arabian cities, which now made capturing and not razing Libyan and Scythian now worth it as they became close enough (and are otherwise valuable enough to keep), which he did on turns 204 and 208.
  - As for Shaka, he spent his own early military effort fighting near Libyan but failed to capture it.
  - He later became Hannibal's vassal, and Hannibal won by Space.
  - See [KI#188.5](/_1_AdvCiv-SAS/Docs/README_Known_Issues.md#ki-188.5).
- **Offensive armies can interrupt or divide an expedition to defend nearby endangered cities:**
  - Following KI#188, similar logging showed a ready AI army, including one already in enemy territory after being backstabbed, could in Base AdvCiv/K-Mod select another enemy city while an underdefended own city was reachable within a few turns.
  - The clearest diagnostic case was in save file 450: Baghdad needed 12 defenders and had 5, but a healthy 11-unit Arabian city-assault army only two turns away waited for more units, grew to 23 units / 32% of Arabia's military, and attacked Prague; Baghdad fell, Prague was not captured, and Arabia retook Baghdad five turns later after its population had fallen from 10 to 4.
  - To fix and enhance this, before choosing its next offensive target, the army now first tries to counter a suitable nearby invader when its area is considered defensive.
  - If no such counterattack is selected, it can send enough units to fill an endangered city's remaining defender shortage within three turns and continue the expedition with the rest.
  - The AI does not begin a rescue when a one-unit group cannot fill a larger shortage or when filling it would consume more than three quarters of the group, and doomed-city evacuation still takes priority.
  - This lets a military advantage serve defense without automatically abandoning every viable offensive.
  - After the fix, history diverged, but a similar save file 450 run showed the intended choices working: an Arabian city-assault group suspended its expedition to counter Holy Roman units near Baghdad, which remained Arabian.
  - Later, a Dutch army sent 10 of its 19 units to protect Amsterdam while keeping 9 in the field; the Dutch won three of four nearby land battles and successfully retained the city.
  - Overall, 11 of the 16 expedition-split decisions targeted cities that survived the immediate war or remained held through the test.
  - The five failures were all on the collapsing Holy Roman front, where Najran, Vienna, Nuremberg, Prague, and Aachen still fell, so the response improves viable defenses without pretending an overwhelmed empire can save every city.
  - See [KI#188.2](/_1_AdvCiv-SAS/Docs/README_Known_Issues.md#ki-188.2).
- **Emergency city reinforcements now complete their assigned rescue:**
  - Similar logging showed that the first prototypes of the cure above could detach defenders only for them to immediately regroup with their offensive army instead of reaching the endangered city.
  - For example, at Chichen Itza, 6 units detached from an 8-unit army, regrouped, and had to be detached again two turns later; the city fell before the rescue was completed.
  - This was not an inherited Base AdvCiv/K-Mod issue, but a missing part of our candidate cure.
  - To complete the cure, emergency reinforcements now retain their city assignment, count as incoming defenders so the AI does not duplicate the same response, and remain in the city while needed; once the danger or defender shortage ends, they return to normal duties.
  - In the confirming save file 450 run, 7 units successfully reached Amsterdam while the other 11 remained together for their campaign.
  - See [KI#188.2](/_1_AdvCiv-SAS/Docs/README_Known_Issues.md#ki-188.2).
- **Large AI armies parked with nothing reachable to attack:**
  - BBAI logging showed a major Base AdvCiv/K-Mod bug: even while at peace, with no planned war and no rival or Barbarian city it could reach, the AI could keep waiting for reinforcements and effectively lock more units into the same army so they could not be released for other duties.
  - For example, in save file 450, a United States group grew from 6 units on T126 to 18 of the empire's 47 military units by T225, and then, after the short war, it returned to peace still holding 21 of 51 units on T247.
  - So it concentrated about 40% of the AI's military in one city and left far fewer units available to defend the rest of its empire or pursue another opportunity, which was very problematic.
  - To fix this, when the AI has nothing to attack, it now stops waiting for more reinforcements: if the army is already in its own territory and not immediately threatened, the AI now only keeps one modest core group ready for a future city attack.
  - As for the excess units, they now instead leave the group and resume defending cities or preparing another expedition.
  - Note: an army that is still abroad or in immediate danger remains together until it returns to its own territory and is no longer immediately threatened, so the fix does not break it apart while exposed.
  - Note 2: the units can also regroup into a large army as soon as a real target or planned war appears.
  - After the fix, confirming runs of save files 450 and 452 showed the intended behavior: in save file 452, Japan reduced its peacetime city-assault group with nothing to attack from 21 to 11 units, freeing 10 units for other duties; at the same time, the AI could still assemble large armies when a real war began, as Carthage attacked Susa with 63 units in save file 450, and China attacked Aksum with roughly 180-202 units in save file 452.
  - See [KI#188.3](/_1_AdvCiv-SAS/Docs/README_Known_Issues.md#ki-188.3).
- **Targetless armies no longer remain parked by gathering several smaller groups in the same city:**
  - some time later, BBAI army diagnostics and SASGameRecord city snapshots showed another form of the inherited Base AdvCiv/K-Mod failure that the first cure above did not cover: in save file 450, Holy Rome was at peace and parked 71 of its 105 military units (68%) in Mainz across 12 groups, although no group exceeded the individual limit of 9.
  - This was especially harmful because Mainz was only population 9 with 14 production and only 2 of its 6 worked plots improved (so rather low value), while stronger cities were left with small garrisons: population-17 Aachen capital produced 48 production and 53 commerce but held only 4 military units, and population-15 Nuremberg produced 57 production but also held only 4.
  - To complete the cure, the retained limit now covers all city-assault-led groups on the same safe owned plot and prevents released units from immediately joining them again; defenders and general attackers outside those groups are unaffected.
  - This reduced repeated release events from 591 to 78 and detachments of units already assigned to other military roles from 1,746 to 331.
  - In the confirming run, Mainz had only 7 military units on turn 190 instead of 71, while genuine war plans still assembled armies up to 47 units.
  - Save file 449 independently remained restrained with only 44 release events and 108 detachments, and still assembled a 57-unit wartime army.
  - See [KI#188.3.2](/_1_AdvCiv-SAS/Docs/README_Known_Issues.md#ki-188.3.2).
- **Units no longer gather and wait in upgrade cities for upgrades the AI will not buy:**
  - Base AdvCiv/K-Mod sent units to technically valid upgrade cities without checking whether the AI could afford the upgrade.
  - AdvCiv-SAS later added economic checks that can also reject an affordable but poor-value upgrade, but the inherited travel-and-wait decision ignored both results.
  - As a result, the Base issue remained and the new SAS mismatch made it substantially worse.
  - For example, in save file 450, BBAI logging recorded 1,081 candidate checks and 1,053 waits in already crowded upgrade cities: 372 lacked enough gold and directly exposed the Base AdvCiv inherited affordability omission, another 631 had enough gold but failed the AdvCiv-SAS economic checks, and only 78 were currently eligible.
  - This repeatedly tied up units for upgrades the AI would not buy.
  - For up to 63 turns, the logs repeatedly recorded the same units being sent toward those cities or forced to skip their turns once there (`MISSION_SKIP`), so those units could not attack, reinforce another city, reposition for defense, or perform any other military duty on those turns, and only 53 of the 99 affected units were eventually upgraded.
  - To fix this, the AI now checks the exact upgrade it intends to buy before travelling or waiting; if it cannot or will not buy it, the unit resumes its normal military role until the upgrade becomes genuinely available.
  - After the fix, crowded-city waits fell from 1,053 to 9 in the confirming run, involving only 5 units that were all upgraded shortly afterward.
  - Save file 449 independently confirmed that every unit waiting before its final turn was upgraded soon afterward; its only unresolved waits occurred immediately before another player won the game.
  - See [KI#188.3.3](/_1_AdvCiv-SAS/Docs/README_Known_Issues.md#ki-188.3.3).
- **AI war preparations no longer repeatedly gamble away their own mobilization:**
  - BBAI logging in save file 452 showed a major UWAI issue: across all AIs, 152 war preparations began, 130 were canceled or redirected to another target, and only 17 became wars.
  - Whenever UWAI reviewed a preparation whose war value had become negative, it converted how doubtful the plan looked into a chance to abandon it and rolled that chance on every negative review.
  - China alone began 36 preparations: repeated abandonment rolls canceled 19, separate target-switch rolls redirected another 13, and only 2 became wars.
  - Established preparations also used a random roll to decide when to become direct war.
  - This could waste the military advantage and upkeep already committed to mobilization: 32 of 74 cancellations even selected the exact same target again later in the same team turn.
  - To fix this, the AI now keeps a newly chosen plan through its first two reviews unless a hard failure, such as an illegal target or expired unprofitable preparation, requires immediate cancellation.
  - After that, it cancels only when the personality- and game-speed-adjusted evidence reaches the tunable maximum for a clearly bad plan.
  - It switches only when another rival is clearly better, uses a stable readiness threshold before beginning direct war, and cannot cancel and recreate the same target during one team turn; other targets remain available throughout.
  - In the final confirming save-file 452 run, cancellations fell from 74 to 34, target switches from 22 to 15, same-turn reselections from 32 to 0, and reselections within three turns from 41 to 5, while actual wars increased from 22 to 29.
  - Save file 450 independently confirmed no same-turn reselection, 39 preparations advancing to direct war, and 35 wars starting.
  - See [KI#189](/_1_AdvCiv-SAS/Docs/README_Known_Issues.md#ki-189).
- **War preparations against near-winning rivals no longer forget why the rival was chosen:**
  - the same investigation found an AdvCiv-SAS integration bug in the victory-denial improvements from KI#184.
  - China selected India on T237 with a war value of 320, including 300 because India threatened victory, but the inherited next-turn review omitted that urgency, saw only 17, and switched away.
  - Reviews and alternative-target comparisons now preserve the victory-denial value; if an existing preparation later meets the same narrow distance, power, naval, and victory-timing limits used for emergency war selection, the AI can declare immediately instead of losing more turns.
  - In the independent save-file 450 confirmation, this later direct-war path fired three times and all three declarations appeared in the game history, including wars against near-Space-victory rivals on T255 and T281.
  - See [KI#189](/_1_AdvCiv-SAS/Docs/README_Known_Issues.md#ki-189) and [KI#184](/_1_AdvCiv-SAS/Docs/README_Known_Issues.md#ki-184).
- **When the AI decides to prepare a war, it now chooses the rival it considers best instead of gambling on worse alternatives:**
  - Base AdvCiv UWAI rolled separately for every eligible rival in ranked order.
  - If its preferred target failed the random roll, it could then prepare against a much less valuable rival whose separate roll happened to succeed.
  - For example, on turn 215 of save file 450, China rejected nearby Ethiopia (distance 4 and only 51% of China's power) despite rating it at 68% war drive, rejected England (distance 5 and 59% power) at 7%, and then selected faraway India (distance 11 and 38% power) at only 5%.
  - To fix this, the AI now only makes one personality-based roll for its highest final-drive target: if it fails, the AI prepares no war that turn instead of attacking someone it considers worse.
  - In the confirming save-file 450 and 452 runs, all 81 successful selections chose the highest final-drive target, while warfare remained active.
  - This behavior is XML-tunable and can be disabled to restore the inherited independent rolls.
  - See [KI#191](/_1_AdvCiv-SAS/Docs/README_Known_Issues.md#ki-191).
- **AI no longer leaves potentially profitable wars to random peace rolls based on volatile estimates:**
  - BBAI logging showed that Base AdvCiv UWAI left a potentially profitable military outcome to a random roll whenever continuing the war briefly looked worse than peace.
  - This was already unreliable as the underlying estimate could change drastically from one turn to the next.
  - But another issue was also that UWAI rolled again on every such review, so even an unchanged 10% chance had about a 65% chance to end the war within ten turns (because with a e.g. 10% chance each turn, the chance of never rolling peace over 10 turns is 0.9^10 ≈ 35%; therefore, the chance of rolling peace at least once is 1 - 0.9^10 ≈ 65%).
  - Save-file 452 showed the problem: after Egypt captured population-12 Satsuma, it led Japan by 8 cities to 5, 762 power to 550 and war success 99 to 64, but its estimate briefly fell to `-32`; but then recovered to `+87` on the next turn.
  - To fix this, instead of gambling on each uncertain estimate, the AI now has a modest commitment and risk-tolerance margin: it pushes for further gains through a small deficit, but seeks peace immediately once the result becomes decisively bad.
  - The default XML-tunable margin is as of now 40 points below the context-sensitive peace threshold, which is always 0 or negative for AI-vs-AI wars.
  - In the confirming run, Egypt continued through the `-32` dip, captured population-7 Tokyo, and then successfully accepted Japan's later peace request after the extended fighting became costly.
  - Mali similarly continued through a `-7` dip but accepted India's request one turn later at `-70`.
  - Opponents can still offer an acceptable treaty or capitulation during the margin, emergency peace remains immediate, and human peace contact keeps personality-based timing.
  - See [KI#189.2](/_1_AdvCiv-SAS/Docs/README_Known_Issues.md#ki-189.2).
- **Considering another war no longer makes the AI abandon an opponent it is decisively defeating:**
  - Base AdvCiv UWAI treated other possible wars as a reason to end the current war, but that cost could overwhelm all of the current advantage.
  - In save file 452, Mali had 7 cities against Maya's 3, 891 power against 397 and an 83-point war-success lead; nevertheless, beginning a separate preparation against India increased this cost from `-18` to `-96` and made Mali accept peace with Maya.
  - To fix this, when the AI is already so clearly winning, a possible future war can still reduce the value of continuing the current one, but can no longer erase most of that value: the default XML-tunable limit as of now is 15%.
  - An early before/after test on the same save file showed the intended result: instead of accepting peace, Mali continued fighting and captured another Maya city, turning its military advantage into immediate territory.
  - In the final-tuning run, history had already diverged before this decision (due to run variety or such), so the exact case did not recur, but the full run still confirmed that clearly bad wars ended quickly.
  - Note: this protection applies only when the current war is already profitable, the rival has fewer cities and at most half the AI's power, and the AI has a substantial war-success lead.
  - The cap does not reduce the effects of military effort, losses, war weariness or the danger from fighting several enemies, so the AI can still seek peace normally if those costs make continuing the war unprofitable.
  - See [KI#189.2](/_1_AdvCiv-SAS/Docs/README_Known_Issues.md#ki-189.2).
- **AI offensive production rises progressively by era, preserving early growth and strategic variety while correcting weak middle and late armies:**
  - the old AdvCiv-SAS 5055 stable release could produce very large AI armies, while later releases generally favored economic efficiency but produced substantially fewer offensive units, especially late in the game.
  - AdvCiv-SAS therefore measures the share of each empire's **base production currently committed to `UNITAI_ATTACK` / `UNITAI_ATTACK_CITY` units** and, when it is below an XML-tunable minimum (the `SAS_AI_CHOOSE_PRODUCTION_MIN_OFFENSIVE_PRODUCTION_CAPACITY` SAS defines), asks an eligible city to help fill the shortfall before opportunistic wonders and broader later building choices.
  - This is a production-capacity floor rather than a fixed unit count, unit-production cap or military-upkeep target, so productive cities contribute proportionally more and personalities, wars, danger, defense and composition logic can always produce more units when needed.
  - Worker, Settler, Explorer, panic, financial, culture and other earlier priorities still take precedence.
  - The default Ancient-to-Future curve as of now is **`0 / 0 / 20 / 30 / 35 / 40 / 40`**: it leaves the Ancient and Classical eras entirely to ordinary AI logic, then deliberately strengthens from the Medieval era onward as foundational growth and infrastructure have less remaining time to compound and military urgency increases.
  - Extensive same-save comparisons showed why a flat target was inferior: high early pressure could displace growth and buildings without retaining a larger later army, while insufficient late pressure preserved an economy that could not defend itself or convert its strength into victory.
  - SASGameRecord and BBAI Logging in save files 463 and 468 both retained substantial later armies with the freer early curve; it won on T389 versus T435 and on T466 versus T491 respectively, with the Huge-map T466 result also allowing Cyrus to pursue a deliberate Cultural victory rather than the Space path followed by the same civilization under the more conservative curve.
  - Industrial 35 also retained military strength more efficiently than 40 in save files 461 and 463.
  - The seven era values remain independently tunable for different player preferences or rulesets.
  - See [KI#197.9](/_1_AdvCiv-SAS/Docs/README_Known_Issues.md#ki-197.9) and [KI#197.10](/_1_AdvCiv-SAS/Docs/README_Known_Issues.md#ki-197.10).
- (Requires AdvCiv-SAS 6389+) **AI military production adapts after its primary landmass is secured:**
  - the era-progressive land-offense floor stays fully active while independent rivals/Barbarians or a land/defensive war remain on the primary area, so an Archipelago AI first secures the island it actually shares.
  - Once that landmass is locally safe, the default forced land-attacker share drops roughly in half.
  - This matters even on non-naval-heavy maps: e.g. an AI that controls its whole Continents landmass no longer keeps a Pangaea-style attacker floor against nobody, and by default the released production returns to normal building/economic/strategic choices.
  - On naval-heavy maps, however, local security usually makes water the next military bottleneck, so the released share is instead redirected to `UNITAI_ASSAULT_SEA` invasion transports plus escorts/attack ships for overseas defense and invasion.
  - The first Archipelago validation showed Korea and China switch profiles immediately after clearing their local landmasses and later conduct overseas conquests.
  - A confirming same-map run also validated the peaceful fleet-stock guard: once a secured five-port Byzantium reached its 25 tracked invasion-role-ship allowance, the extra naval floor stopped; it automatically reopened when real naval-war/projection state began and closed again afterwards.
  - Players can independently tune the secured land/naval era curves, enable some of the naval redirect on Continents-like maps, change the naval role mix, or allow smaller/larger peaceful reserve fleets.
  - This new behaviour is tunable via the `SAS_AI_CHOOSE_PRODUCTION_SECURED_` SAS defines.
  - See [KI#197.11](/_1_AdvCiv-SAS/Docs/README_Known_Issues.md#ki-197.11).

#### Military - Land (AI)

- **Risky AI stack attacks can use obsolete units as expendables / cannon fodder before current units:**
  - BBAI attack-order logging in save file 460 showed the SAS low-power selector could choose the first unit to attack to be obsolete unit, only for inherited Base AdvCiv/K-Mod sacrifice logic to replace it with a current unit first attacking instead; one sample had 189 obsolete-to-current replacements, including an obsolete Archer at 10% winning odds being replaced by a current (plus much stronger) Horse Archer.
  - To fix this, when the current unit would already take a risky attack, the AI can instead spend a tactically suitable obsolete unit first as cannon fodder/expendable, potentially weakening the defender while preserving current stronger units for safer follow-up attacks.
  - Very safe current attacks (90%+ as of now) remain untouched, and XML-tunable sacrifice-value, tactical-role and odds safeguards prevent blanket weakest-unit-first behavior.
  - Confirming runs showed expendables often damaged defenders or withdrew; in one sequence, obsolete attackers improved a preserved Mechanized Infantry's odds from 36% to 99%, after which it won without damage.
  - This also helps clear weak obsolete excess naturally instead of keeping it for unit support or often poor-value future upgrades.
  - See [KI#200](/_1_AdvCiv-SAS/Docs/README_Known_Issues.md#ki-200).
- **Barracks:** lower `FLAVOR_MILITARY` (10 → 6 for the generic Barracks; the base-AdvCiv Zulu Ikhanda is already 6) so non-militaristic leaders do not over-prioritize it.
- **Stable:** lower `FLAVOR_MILITARY` (10 → 6 for the generic Stable) so leaders do not over-prioritize specialized mounted-experience infrastructure; civilization-specific replacements keep their own XML flavor values.
- Early **siege**: adjust UnitAIs so early siege doesn’t get baited away from core attack/defense purposes.
- New-city **minimum defenders**: ensure new cities are founded/left with **2+ defenders** (priority to capital). Optimization disabled after ~turn 120 (scaled) to save compute once empires stabilize. See **[KI#49](/_1_AdvCiv-SAS/Docs/README_Known_Issues.md#ki-49)**.
- **Musketmen — AI role**: now also flagged with `UNITAI_ATTACK_CITY`, so the AI uses Musketmen more often in **offensive city-assault** roles. Civ-specific Musketman variants may also include additional changes.
- **Grenadiers & Bazookas — AI role**: now also flagged with `UNITAI_CITY_DEFENSE`, so the AI may use them more for **city defense**.
- **City evacuation — doomed city auto-retreat (land units only)**: in `CvUnitAI::AI_evacuateCity`, the AI now optionally (default ON, recommended) **evacuates 100% of its land units** from a **doomed city** (i.e. a city that exceeds the existing `AI_EVACUATION_THRESH`-based (default as of now: 138%) attacker/defender strength threshold (note: the threshold is raised for important cities, as it is worth defending them and generally bad to evacuate them)).
  - Fixed an inherited Base AdvCiv percentile inversion that instead gave this important-city protection to low-value cities; see [KI#172](/_1_AdvCiv-SAS/Docs/README_Known_Issues.md#ki-172).
  - Previously it often retreated only a tiny fraction (e.g., 2 of 11 defenders vs a 22+ stack, whereas now all 11 defenders would evacuate if the city is deemed doomed).
  - This change might prolong wars/game length, but the intent is a **more competitive, less self-sabotaging** AI.
  - See [KI#62](/_1_AdvCiv-SAS/Docs/README_Known_Issues.md#ki-62).
- **Minimum Stack size before war**: Added a minimum stack unit count requirement to prevent the stack from being marked as "ready" until it has enough units: no `MOVE_DECLARE_WAR` flag, no premature advance toward the city, and AI keeps building units until the stack is big enough.
  - Helps fix the **issue of "back and forth" war declarations with immediate retreat without city attack**: low-numbered unit stacks were especially sensible to power swings (city defenses, one more enemy units skews win chance, etc.).
  - Change in `CvUnitAI::AI_attackCityMove`.
  - See [KI#104](/_1_AdvCiv-SAS/Docs/README_Known_Issues.md#ki-104).
- **City attack oscillation — pre-check vs strong targets (possible mitigation)**: In `CvUnitAI::AI_attackCityMove`, add a sanity check to **skip marching to/attacking a city** when the **defending stack is too strong**.
  - Goal: prevent the inefficient **advance → abort → backtrack** loop and reduce the risk of being caught without time to retreat.
  - **Note:** in the test sample, this behavior also resolved without the change by running ~2+ autoplay turns in a row instead of 1 turn only from a slightly older save file rather; treat this as a **possible fix/sanity check** (not (at least not yet) broadly tested).
  - See [KI#63](/_1_AdvCiv-SAS/Docs/README_Known_Issues.md#ki-63).
- **Early defender overbuild → switch to versatile offense (AI)**: Heuristic in `CvCityAI::AI_chooseUnit` reduces **early-game overproduction of pure defenders** (especially Longbowmen) by swapping the build to a more **offensive/versatile** unit when defense is already sufficient — e.g., Spearmen, Axemen, Camel Archers, with a preference for civ-specific units like War Chariots when available.
  - Also caps/avoids siege (esp.
  - Trebuchets) when they’re inefficient defensively; production can drop **down to zero** if conditions warrant.
  - If no good replacement exists, the AI can **reflag** the chosen defender to an **attack/attack_city/counter** `UnitAI` instead.
  - In testing, this produced **fewer pure defenders** and **more varied stacks** (e.g., a run where Hatshepsut shifted from ~30 Longbowmen at T130 to ~32 War Chariots, plus Camel Archers).
  - See [KI#53.2](/_1_AdvCiv-SAS/Docs/README_Known_Issues.md#ki-53.2) and [KI#53.2.2](/_1_AdvCiv-SAS/Docs/README_Known_Issues.md#ki-53.2.2).
- **Update :**
  - military-production logging exposed an AdvCiv-SAS-specific edge case where this defender-to-offense substitution could rewrite a `UNITAI_CITY_DEFENSE` request even when the caller had explicitly determined that a real city defender or Settler escort was required.
  - In one pre-fix save-file 456 sample, 98 of 322 minimum-defender requests were converted to another UnitAI; concrete examples included Pasargadae requesting a Longbowman for minimum defense but receiving a Catapult with `UNITAI_ATTACK_CITY`, and Thebes repeatedly requesting defense for an existing Settler but likewise receiving offensive units.
  - To fix this, the optimization now preserves concrete `UNITAI_CITY_DEFENSE` requests when the strict city-defender count is below `AI_minDefenders() + plot Settlers`, plus the existing thin-military post-Settler case.
  - In the final save-file 456 run, all 264 minimum-defender choices, 34 existing-Settler escort choices, 8 zero-defender choices and 5 quick post-Settler defense choices kept their requested UnitAI, while 15 ordinary floating-defender requests were still redirected to offense.
  - Save file 457 independently reproduced the separation: 262 minimum-defender completions, 24 Settler-escort defenses and 9 zero-defender branches had zero SAS role rewrites, while floating-defender production still made 65 ordinary substitutions.
  - See [KI#197.4](/_1_AdvCiv-SAS/Docs/README_Known_Issues.md#ki-197.4).
  - (Requires AdvCiv-SAS 6401+) Update 2: Follow-up Great Plains testing exposed the stricter healthy-defender version of the same problem for Settler escorts: the production layer could correctly request a Longbowman because a completed Settler needed one detachable healthy escort, but the lower substitution layer could still rewrite it into siege.
    - The shared escort-shortage test now preserves those explicit requests too; in one six-save validation, **383/383** logged shortage-driven defender requests retained the requested defender/UnitAI with no offensive substitution.
    - See [KI#179.2](/_1_AdvCiv-SAS/Docs/README_Known_Issues.md#ki-179.2).
- **Siege production — conditional minimization (trebuchets especially)**: In `CvCityAI::AI_chooseUnit`, added targeted checks to keep **siege output** as low as needed—**up to none at all** when conditions say they’re poor value (e.g., when the AI is **weaker** or not preparing **city assaults**).
  - Rationale: **trebuchets** are very **hammer-inefficient** outside direct city attacks; **catapults/cannons** also aren’t versatile enough (and are often misused) for defense.
  - Current tuning intentionally leans **low** versus before (but not permanently zero when other conditions are met).
  - Result: **more varied offensive stacks** and **stronger defense** when behind.
  - See [KI#53.3](/_1_AdvCiv-SAS/Docs/README_Known_Issues.md#ki-53.3).
  - Thresholds are slightly relaxed pre-Renaissance and heavily relaxed from Renaissance onward so Cannons can serve as a defensive pivot.
  - (Requires AdvCiv-SAS 6577+) Update: The former extra relaxation based on possessing named Iron/Copper/Horse/Camel/Elephants is now based on whether the city can actually train a non-siege land attacker of sufficient relative strength (80% of the candidate siege unit by default); this recognizes civilization-specific, mod-added and no-resource alternatives without treating an inaccessible or obsolete named resource as useful.
- **City-counter role pruning (e.g. Macemen)**: Removed `UNITAI_CITY_COUNTER` from **non-versatile** units (e.g., `UNIT_MACEMAN`).
  - For example for macemen, as city defenders they’re too easy to punish—knights counter them hard, and even crossbow stacks can beat defending macemen—so they’re better assigned to **offensive** `UnitAI` roles.
  - While macemen beat pikemen, pikes seldom assault cities efficiently, so the city-counter slot isn’t warranted.
  - The same adjustment applies to any other units with this mismatch.
- **Mounted "Counter" role pruning:**
  - Removed `UNITAI_COUNTER` from the mounted units (`UNIT_CHARIOT`, `UNIT_EGYPTIAN_WAR_CHARIOT`, `UNIT_CARTHAGE_NUMIDIAN_CAVALRY`) that had it.
  - Rationale: mounted units don’t "receive defense bonuses" (note: not related to bonuses/ressources) and can be countered by other units; assigning them to COUNTER can park them in cities (e.g. seemingly seen in autoplay with War Chariots) and occupies a slot **Longbowmen** are more fit to fill as they unlock early and are cheap and versatile defenders, so these mounted unit lines are better used for offense roles rather during its window—or skipped outside it.
- **Stack attack order — spend expendables first:**
  - stack attacks now prefer **bombard/collateral** and **low-effective-strength/low-XP** units first (once the stack commits to attack), preserving elite finishers.
  - Rationale: economically efficient (older/weaker units cost upkeep but scale poorly) and strategically safer (keep high-value units for decisive blows or retreats).
  - Change in `CvSelectionGroupAI::AI_getBestGroupAttacker`.
  - Toggle via SAS defines for A/B testing.
  - See [KI#89](/_1_AdvCiv-SAS/Docs/README_Known_Issues.md#ki-89).
- **Great Generals — prefer Military Instructor, focus on hammer hubs:**
  - AI favours settling Great Generals as **Military Instructors** over attaching them as unit leaders, as the instructor bonus scales better into the late game.
  - **Targeting:** strong bias to the **top 3 hammer cities** with **decreasing weights** (top-1 most, then top-2/3 progressively less).
  - (Requires AdvCiv-SAS 6584+) Extra preference is derived from the city's actual military-production modifiers rather than a hardcoded Heroic Epic building identity, so equivalent civilization-specific or mod-added military-production buildings work naturally.
  - **Limit removed:** cities may host **multiple** Military Instructors (no per-city cap).
  - Implemented in `CvCityAI::AI_permanentSpecialistValue` and `CvUnitAI::AI_generalMove`.
  - See [KI#69](/_1_AdvCiv-SAS/Docs/README_Known_Issues.md#ki-69).
- Update: Rarely in games, for example in save file 450 with France AI, a Great General could be attached to a low-value Longbowman at ~T130.
  - BBAI logging showed the cause was Military Instructor joining was blocked by safety/path gates.
  - To fix it, Great General unit-attachment branches now use tunable minimum target gates that default very high, so Great Generals effectively keep preferring Military Instructor or Military Academy unless these thresholds are deliberately lowered.
  - In the retest after fix, those low-value Longbowman attachments were rejected (and another Great General attachment to USA in the same run that newly occurred as well at ~T180 was successfully blocked) and the Great Generals joined cities as Military Instructors instead.
  - See [KI#69 (Update)](/_1_AdvCiv-SAS/Docs/README_Known_Issues.md#ki-69).
  - Update 2: **Great General safety/reliability:** fixed several inherited Base AdvCiv pitfalls that could spawn, route or leave defenseless Great Generals in danger before the AI could use them, made the additional Military Instructor-first priority era-tunable, and allow an immediate Instructor join when the General is already standing in an otherwise safe city even if nearby radius-2 danger would block travel there.
  - See [KI#204](/_1_AdvCiv-SAS/Docs/README_Known_Issues.md#ki-204).

#### Military - Naval (AI)

- Curb **naval overbuild**: in `CvCityAI::AI_bestUnit`, reduce making excess naval combat units (esp. when landlocked or when land war is likely) to prevent getting invaded on land with too few defenders. See **[KI#35](/_1_AdvCiv-SAS/Docs/README_Known_Issues.md#ki-35)**.
- **Loaded assault ships can take achievable overseas targets instead of waiting for an army larger than they can carry:**
  - Base AdvCiv/K-Mod generally required a loaded fleet to reach a generic army size before checking whether its actual units could capture a specific city by sea.
  - For example, BBAI logging in save file 450 showed that a fleet consisting of one Galley (total Cargo Space: 3) waited for 13 units before carrying reinforcements to an overseas attack already under way, or 26 units before launching its own full invasion.
  - This was a problem because the fleet could not reach either threshold unless more transports joined it; more generally, any fleet whose ships together could not carry its required generic army size could remain in port indefinitely, which was especially bad when nearby Barbarian island cities sometimes even had no defenders at all.
  - This happened even though the AI already had a more relevant check that compares its loaded units' strength with the target's defenders and route, because the generic army-size gate could prevent that check from running.
  - To fix this, once safety, escort and imminent-loading needs are handled, a smaller loaded fleet now also asks the target-specific check whether its loaded force is strong enough to attack a reachable target.
  - Note: the normal thresholds remain for organizing larger invasions, and the fleet still waits when no suitable target exists.
  - For Barbarian targets, it also avoids a long naval trip to a city it expects to raze, but a worthwhile isolated-island city is not rejected merely because it is on another landmass (as per `AI_isSASCityLikelyToBenefitUsLongTerm`).
  - In the confirming save-file 450 run, 7 distinct fleets found valid below-threshold targets, while 76 other evaluations found no acceptable target and correctly kept their fleets waiting; the changed history produced 6 additional Barbarian-city conquests with none razed.
  - Save file 449 independently confirmed the island behavior: India sent 3 units despite a threshold of 9 and captured the Barbarian island city Burgundian six turns later.
  - See [KI#193](/_1_AdvCiv-SAS/Docs/README_Known_Issues.md#ki-193).
- **AIs can build the limited missing transport capacity needed for a nearby profitable overseas city:**
  - the preceding fix could use a small loaded fleet, but Base AdvCiv/K-Mod could still fail to produce any assault transport for a worthwhile island.
  - Its production logic mainly looked for a broad overseas-assault state or the complete absence of an enemy city reachable by land; therefore one unrelated land target could hide every island opportunity.
  - A large available land army could also raise general military spending enough to block the ship needed to move it.
  - For example, save-file 450 BBAI diagnostics found Carthage with 24 available attackers against Cimmerian's 1 defender only 9 plots from its nearest Carthaginian city, yet the missing transport was not requested.
  - To fix this, a safe coastal city can now request only the missing cargo space when the AI knows a nearby overseas city that is worth keeping and has enough available attackers for its defenders.
  - Existing and currently produced transports count toward that requirement, and normal financial-trouble, city-danger, defensive-war and central fleet-limit safeguards remain.
  - In the confirming save-file 450 run, only 7 opportunistic ships were ordered, existing or queued capacity prevented 95 further requests, and all 5 tracked Barbarian targets were captured by turn 242; Cimmerian fell on turn 199 instead of turn 246 in the diagnostic run.
  - Save file 449 independently remained restrained: 140 evaluations found that enough transport capacity already existed, only 7 opportunistic ships were ordered during 456 turns, and ordinary AI assault fleets never exceeded 4 ships.
  - A faraway city 27 plots from the nearest Carthaginian city did not trigger new production, although Hannibal could still profitably use an existing Galleon to capture it with 4 units.
  - See [KI#193.2](/_1_AdvCiv-SAS/Docs/README_Known_Issues.md#ki-193.2).
- (Requires AdvCiv-SAS 6459+) **UWAI better recognizes feasible naval invasions after its fleet has already demonstrated enough surviving lift:**
  - Equal Islands investigation found that passive island AIs were usually not being suppressed by the KI#53.5 land-production brake; instead, `MilitaryAnalyst`/`InvasionGraph` often predicted no city conquest even when the attacker already had a large army, a stronger fleet and substantial assault capacity.
  - In 2,056 failed scheduled city attacks from otherwise favorable naval evaluations, about 94% had already passed the landing/fleet stage; the median surviving landing ratio was 100%, but the inherited multi-area proxy then reduced the AI attacker's usable Army to about 75% before the separate deployment-distance penalty.
  - Human naval attackers already receive a +25 percentage-point allowance for coordinating armies across landmasses.
  - The AI now receives up to the same allowance only for its own evaluated naval invasion and only after winning the simulated landing, scaled by the actual surviving landing ratio; Fleet/cargo losses, travel distance, defender weighting/rallying, bombardment and city-combat checks remain unchanged.
  - In the same-seed Equal Islands validation, synchronized RNG state stayed identical through turn 193; among 380 directly matchable city attacks through the first divergence boundary, 11 modeled defeats became wins and none reversed, while 20 of 72 matched high-level simulations changed from zero projected conquest to at least one.
  - Mean final war-utility change was only about +3.6 (median 0), so the fix changes feasibility rather than applying a blanket aggression bonus.
  - After histories diverged, war starts rose only 67 to 71 while wars with at least one captured city rose 17 to 26 and captured-city events 30 to 36.
  - See [KI#53.6](/_1_AdvCiv-SAS/Docs/README_Known_Issues.md#ki-53.6).

## UI

- Note: when changing some settings such as game resolution, Civ4 needs to be restarted to fully apply some of the changes.

### UI (Main Menu)

- **Randomized Opening menu music:**
  - opening-menu soundtrack now supports **randomized playback** from a **tunable** pool of multiple main menu scripts, using the **shuffle-bag** approach (i.e. **no repeat until all have been played**); once all configured entries have been played once, the **pool refills and shuffles again**.
  - Music is also playable via Sevopedia Music/Media Player.
  - Change in `CvMainMenuInfo::getSoundtrack` in [CvInfo_Misc.cpp](/CvGameCoreDLL/CvInfo_Misc.cpp).
- Beautified Main Menu's home page: removed the **background blue panel**, added **dark and bold text**. See &emsp;[README.md (Home page)](/README.md#home-page).
- Beautified the Main Menu's **Simple Game screens**: removed the **margins** and reduced the **radio buttons' vertical spacing** so **more information** is displayed and the **need for scrolling** is reduced.
  - Change in [Civ4Theme_Button.thm](/Resource/Civ4Theme_Button.thm) and [Civ4Theme_Common.thm](/Resource/Civ4Theme_Common.thm).
  - See [README.md (Simple Game rework)](/README.md#simple-game-rework).
- Make the **Globe animation** (Globe.nif in [CIV4WorldPickerInfos.xml](/Assets/XML/Interface/CIV4WorldPickerInfos.xml)) in Simple Game use **real scaling** instead of demonstrative one, e.g. the new World Size Arena has a ratio to World Size Standard of 0.128, so its model only has 12.8% of the size of the standard-sized globe, and vice versa for sizes bigger than standard.
  - This aims to provide a **better representation** of **real world size differences**.
  - See the new Sevopedia W. Sizes Chart category.

### UI (Common)

- **Optionally** support **upscaling font size** (i.e. UI text size) regardless of game resolution.
  - This affects **main interface** (city screen, map view, `city_billboard`/`CITYBILLBOARD`), **advisors**, **sevopedia**, most of **diplomacy screen** (when it was possible) in [CvDiplomacy.py](/Assets/Python/CvDiplomacy.py) and `CvPlayer::getItemTradeString` (for trading table), **Event Log** window (as of now only **Event Log** and **Combat Log** tabs are supported) and **event message popup** (top-center) in [CvUtil.py](/Assets/Python/CvUtil.py) and [CvEventManager.py](/Assets/Python/CvEventManager.py) and `CvDLLInterfaceIFaceBase::addMessage` (note: Event related message upscaling **only applies to new messages written after upscaling is enabled**, not to previously written event messages before upscaling was applied), miscellaneous screens such as the [CvEraMovieScreen.py](/Assets/Python/Screens/CvEraMovieScreen.py) (i.e. the e.g., "You have entered the Renaissance era!" screen), [CvWonderMovieScreen.py](/Assets/Python/Screens/CvWonderMovieScreen.py), (from hardcoded `<font=4b>` to SAS FONT `TITLE` (more readable without bold)), [CvSpaceShipScreen.py](/Assets/Python/Screens/CvSpaceShipScreen.py), and other places where font size could be changed.
  - Made some **UI adjustments when needed to accommodate the larger text**.
  - Upscaling text can be **useful for high resolution displays** (where text appears very small), but may **also improve readability at smaller resolutions** (like 1920*1080 or lower).
  - A **simple way to upscale all text to max font size** is for example to set `SAS_UI_FONT_TINY`, `SAS_UI_FONT_BODY`, `SAS_UI_FONT_LABEL`, `SAS_UI_FONT_TITLE` and `SAS_UI_FONT_HOVER` all to max font size which is 4.
  - See [GlobalDefines_advciv_sas.xml](/Assets/XML/GlobalDefines_advciv_sas.xml) and related places for the change.
- **Upscaling adjustments** include generally to expand them so text does not overflow, or notably: smaller sevopedia leaders item row (else AI Personality Panel's upscaled text overfills), **smaller Sevopedia speeds first column** (only at **lower resolutions** since these don't have enough horizontal room per column to show them without clipping), **overwrite city `billboard` xml** in [CIV4DetailManager.xml](/Assets/XML/Misc/CIV4DetailManager.xml) **at runtime** via [SASBillboardScale.py](/Assets/Python/SASBillboardScale.py) (**tunable per font size**), increase **GP bar size** and reduce **tech bar size** in **main interface (common)**, **do not upscale icons** in the Foreign Diplomacy Advisor's **Glance Tab** when text is upscaled and there are **>= 2 icons** (e.g., worst enemy + at war icons) (See [KI#116](/_1_AdvCiv-SAS/Docs/README_Known_Issues.md#ki-116)), added **attitude icon chars** in [GameFont.tga](/Assets/Res/Fonts/GameFont.tga) (21x20 icon size): previously they existed only in [GameFont_75.tga](/Assets/Res/Fonts/GameFont_75.tga) (16x16 icon size), causing attitude icon chars to **disappear** (e.g., in Score Tab) **when font was upscaled (attitude font size >= 3)** (see [KI#117](/_1_AdvCiv-SAS/Docs/README_Known_Issues.md#ki-117)), increase **vertical spacing** in Info screen's **Graph Tab** between **leader lines** when text is upscaled (label > 3), **dynamically adjust** the Info Screen's **Graph Tab**'s legend (i.e. leaders') **line height** (**reduced** when there are **many leaders** (e.g., > 36) but **expanded if vertical resolution is high** enough to accommodate it (e.g., >= 1440p)), and adjust legend **line width** depending on **font size** (label >= 3) (and further enlarge it for long leader names).
- **Increase hover text** base **font size** from **2 to 3** and most UI elements (e.g., scoreboard, advisors, main interface, sevopedia) **from body (i.e. 2) to label (i.e. 3)**, and **increased font2 billboard from 100 to 120 percent** so it is **more readable**.
- Added **Emojis** without having to tediously implement them as characters. See [Emojis](/README.md#emojis) and [CIV4ArtDefines_Interface.xml](/Assets/XML/Art/CIV4ArtDefines_Interface.xml).
- Added **"Cannot be traded"** and **"Can be researched multiple times"** tooltips to the Tech Advisor screen and in Sevopedia. See [README.md (Untradeable techs (bTrade) display)](/README.md#untradeable-techs-btrade-display-information) for details.
- removed the **Strategy Text** in Choose Production and Choose Research ("Sid’s tips") popups, as well as in the Civic Revolution popup ("Would you like to start a revolution?"), and in Sevopedia; these blurbs were often outdated or tedious to maintain.
- **Worker builds** that **remove a feature** now **redirect to the corresponding new Sevopedia Builds** page.
  - Applies to any caller (e.g. **Technology Advisor**, **Sevopedia Tech**, **Sevopedia Builds** itself) using the `WIDGET_PYTHON` `6798` approach.
  - See [Sevopedia Reworks (Builds)](/_1_AdvCiv-SAS/Docs/README_Sevopedia_Reworks.md#example-040-builds-category-eg-remove-jungle-build-road-create-a-farm) and [Widget Python 6798 to link (e.g. for Builds, for Traits)](/_1_AdvCiv-SAS/Docs/README_Sevopedia_Reworks.md#widget-python-6798-to-link-eg-for-builds-for-traits).
  - See also [KI#97](/_1_AdvCiv-SAS/Docs/README_Known_Issues.md#ki-97).
  - Note: **hover text** is **preserved** via the `WIDGET_HELP_IMPROVEMENT` hybrid path; this split is intentionally more useful for players (improvement behavior where players expect it in Tech Chooser (i.e. Technology Advisor), and direct Builds-page routing where build details are the main goal in Sevopedia paths).
  - See [KI#113](/_1_AdvCiv-SAS/Docs/README_Known_Issues.md#ki-113).
- **Worker work-rate visibility:**
  - builder units now show their base XML `iWorkRate` from [`CIV4UnitInfos.xml`](/Assets/XML/Units/CIV4UnitInfos.xml) directly as `Work Rate: X%` in the shared unit help used by both **Sevopedia Unit special abilities** and normal **unit hover help** (e.g. `Work Rate: 125%` for the Maya Quick Worker).
  - This makes Worker variants immediately distinguishable at a glance while technology/civic/building build-speed modifiers remain shown separately in Sevopedia.
  - Implemented through `CvGameTextMgr::setBasicUnitHelp` in [CvGameTextMgr.cpp](/CvGameCoreDLL/CvGameTextMgr.cpp) and `TXT_KEY_UNIT_WORK_RATE` in [AdvCiv-SAS_main.xml](/Assets/XML/Text/AdvCiv-SAS_main.xml).

### UI (Sevopedia)

Note: New panels added or modified mostly not mentioned here for concision. See [README: Sevopedia reworks](/_1_AdvCiv-SAS/Docs/README_Sevopedia_Reworks.md).

- Wider content area, narrower main category column; fills available screen space.
- Add **Search Bar**: shared by eligible Sevopedia categories (i.e. that have an item list, including Sevopedia Index); it allows to **search** for entries using the **keyboard**.
  - **Special characters** (`()_/*+-=[]'#&~;,:!?.@\|<>`) are **supported** for keyboard input with a **best-effort QWERTY/physical-key mapping** (e.g., `1` inputs `1`, and `Shift + 1` inputs `!`) because they are rarely used in Civ4 item names.
  - An optional **clickable special-character visual keyboard** (with a `KEYS` **toggle link to hide it**) is also available.
  - The code is in [SevoPediaMain.py](/Assets/Python/Contrib/Sevopedia/SevoPediaMain.py).
  - See also [README.md (search bar)](/README.md#search-bar).
- Add **Keyboard Navigation**: **keyboard UP/DOWN arrow keys** allow browsing Sevopedia items in categories with item lists (e.g., Units: Axeman, Swordsman, Legionary, Worker) (note: in the Sevopedia Index, UP/DOWN moves through entries in reading order, using row selection plus text highlight for the current cell because Civ4 tables seemingly do not expose clean per-cell selection).
  - **keyboard LEFT/RIGHT arrow keys** allow browsing previous and next pages recorded in Sevopedia history, including category pages and item pages in the same category.
  - This matches the BACK/NEXT footer links without having to tediously move the mouse and click their positions every time.
  - Note: Tree category pages (e.g., Promotions Tree, Units Tree) no longer use UP/DOWN arrow-key scrolling so LEFT/RIGHT Back/Next works correctly there too.
  - The code is in [SevoPediaMain.py](/Assets/Python/Contrib/Sevopedia/SevoPediaMain.py).
  - See [Sevopedia_reworks: keyboard (UP/DOWN and LEFT/RIGHT) navigation support](/_1_AdvCiv-SAS/Docs/README_Sevopedia_Reworks.md#example-02-added-keyboard-arrow-updown-and-leftright-navigation-support-used-in-several-sevopedia-pages).
- **Fix empty pages** in Sevopedia **category opening flow:**
  - opening a category now jumps **directly to the first real entry (if any)** instead of landing on a **blank header/spacer** row first; this makes **navigation easier** and avoids **needless and disruptive BACK/NEXT history pollution**.
  - See [KI#119](/_1_AdvCiv-SAS/Docs/README_Known_Issues.md#ki-119).
- **Fix BACK/NEXT navigation history being lost on sevopedia exit** (e.g., to revisit AI Personality Panel pages during a game, or other categories previously visited): now **persists** during the entire **gaming session**.
  - Added a **CLEAR** link to clear history.
  - See [KI#125](/_1_AdvCiv-SAS/Docs/README_Known_Issues.md#ki-125).
- Make **BACK**, **NEXT**, and **CLEAR** links **greyed** when they have **no effect** (e.g., clicking NEXT when current page is already last visited, or BACK on first page visited since last clear).
  - This was especially confusing before the navigation history persistence fix, when after reopening sevopedia after last sevopedia exit, BACK and NEXT links would be there but produce no effect at all (previous history was lost) (links are greyed now making such absence of effect clear).
  - Note: Legend link is not greyed but instead is hidden if a category has no legend link (if there is a legend it is always active, and if there is none is it never active, so no need for a grey link).
  - See [KI#126](/_1_AdvCiv-SAS/Docs/README_Known_Issues.md#ki-126).
- New **Current-Game Player Id Previous/Next** links in **Sevopedia Leader** and **Sevopedia Civilization**: show `[-1` (previous) and `-1]` (next) greyed in the main menu/no **active game**.
  - In game, they show nearby **known living** major-**player ids** for the **active player**, including the active player, without spoilers except in debug mode.
  - Links are greyed when inactive.
  - Example: if player ids are 0 to 10 and the active player is id 0 and knows ids 1, 4, and 9, possible links are `[-1` greyed / `0]` white from a leader/civ not in the current game list, `[0` greyed / `1]` white at id 0, `[0` white / `4]` white at id 1, `[1` white / `9]` white at id 4, and `[4` white / `9]` greyed at id 9.
  - Corresponding **legend** sections added.
  - Note: in custom games with duplicate leaders/civilizations, each duplicate leader/civilization appears once, using the first matching player id.
  - Note 2: player id information is also available in the Info Screen's Score Tab.
- **Entries Grouping**: Depending on the Sevopedia Page, Sevopedia lists are now grouped: **by Civic Type** (e.g. Government, Economy) in Sevopedia Civics; **by Era Type** (e.g. Ancient Era, Classical Era, No Tech Prerequisite (available at any era)); **by Specialist Type** (e.g. Engineer vs Great Engineer) in Sevopedia Specialist; **by Improvement Type** (e.g. Farm -> Wheat/Maize, Pasture -> Sheep/Pig) in Sevopedia Bonus; **by Land/Water** (e.g. Land Improvements -> Farm/Pasture, Water Improvements -> Fishing Boats/Offshore Platform) sometimes with additional subgroups.
  - Code implemented in [SevoPediaMain.py](/Assets/Python/Contrib/Sevopedia/SevoPediaMain.py) and [_sevopedia_main_groupings.py](/Assets/Python/Contrib/Sevopedia/_sevopedia_main_groupings.py).
- the **Default First Sevopedia category opened** is changed from hardcoded Sevopedia Techs no matter its position to now **whichever category is the first in the category order**. Change in [SevoPediaMain.py](/Assets/Python/Contrib/Sevopedia/SevoPediaMain.py).
- **Sevopedia Index** is now moved from the Tabs (bottom) to **its own category** for easier access and navigation between categories.
- **Beautify** Sevopedia Main: **Reorder categories** and **change category icon characters**.
  - See [Sevopedia reworks (Main Refactor)](/_1_AdvCiv-SAS/Docs/README_Sevopedia_Reworks.md#example-00-heavily-refactored-sevopedia-maintain-so-it-is-much-easier-to-maintain-or-customize).
  - See also [KI#133](/_1_AdvCiv-SAS/Docs/README_Known_Issues.md#ki-133).
- **Chart categories (common)**: added a **"LOG" button** to print to `PythonDbg.log`.
- **Unit Chart:**
  - added information about other fields and beautified it: notably 1st Strike, Collateral; and for non-Air units Withdraw, or for Air units Air Evasion, Air Intercept, Air Range.
  - (Requires AdvCiv-SAS 6202+) Also added first strike chances (e.g., `0 - 1` means 0 base (i.e. not granted by promotions or such) first strikes, one base first strike chance).
  - See [KI#323](/_1_AdvCiv-SAS/Docs/README_Known_Issues.md#ki-323).
- **Handicap Chart**: new category to see all handicap XML values in a **sortable chart**, notably **by icons (first column)**, which allows to **group them by theme** (e.g. war, gold, knowledge, etc.).
  - See [Sevopedia reworks (Handicap Chart)](/_1_AdvCiv-SAS/Docs/README_Sevopedia_Reworks.md#example-05-handicap-chart-category).
- **Game Speed Chart**: similarly a new category to see all game speed XML values in a **sortable chart**, notably **by icons (first column)**, which allows to **group them by theme**.
  - In particular, they allow to see the full **Calendar/Timeline** information in lines such as `"+2*10k=30k"` or `"+40*m2=2076m9"` for each month increment and each game speed; and the **Culture Level Infos** at each game speed.
  - For convenience, additional fields are added (e.g. or `Total Turns*` 165 at game speed Turbo) and marked with `*` to distinguish them from regular XML info fields.
  - Also added **analysis rows** (e.g., `Summary 08 (40%)` with `T200=+1400 (120)`; `Increments Years 02` with `750, 500, 250`; `Increments Months 02` with `9000, 6000, 3000`).
  - See [Sevopedia reworks (Game Speed Chart)](/_1_AdvCiv-SAS/Docs/README_Sevopedia_Reworks.md#example-06-game-speed-chart-category).
- **World Sizes Chart**: similarly a new category to see all World Size XML values in a **sortable chart**.
  - See [Sevopedia reworks (World Sizes Chart)](/_1_AdvCiv-SAS/Docs/README_Sevopedia_Reworks.md#example-07-world-sizes-chart-category).
  - Similarly, for convenience, additional fields are added (e.g. or `Ratio to Standard*` 3.640 at World Size SAS24), `Ratio to Largest*` (e.g. "0.502" for SAS24 World Size), `Cells Per Default Player*` (e.g. 158 for World Size Huge) and marked with `*` to help identify them.
  - WorldInfo width/height values are terrain cells rather than final playable plots, whose dimensions depend on the map script; the chart labels the derived quantities accordingly.
  - See [KI#305](/_1_AdvCiv-SAS/Docs/README_Known_Issues.md#ki-305).
- Similarly, added a new Eras Chart category with sortable entries including the list of techs for all eras, and notably **optional era buttons**. See [Sevopedia reworks (Eras Chart)](/_1_AdvCiv-SAS/Docs/README_Sevopedia_Reworks.md#example-08-eras-chart-category).
- New **Media Player** (**shared** between Sevopedia Movies and Music): supports **Replay**, **Eject (Exit)**, **Previous Track** and **Next track**, **Toggle Movies/Music**, **Fast Up** and **Fast Down** (change grouping), **Timer**, and a **Playlist** and a **TV panel** on the right side for **non-bik** files (e.g., for **nif**, **dds**), or **a more compact** queue form **for bik files**.
  - This allows to view without having to exit the screen.
  - Click on **Eject button** or **Right-click** to **stop the media playing**; press Escape or Enter key for main menu.
  - See [Sevopedia reworks (Media Player)](/_1_AdvCiv-SAS/Docs/README_Sevopedia_Reworks.md#example-090-media-player).
  - See also [KI#141](/_1_AdvCiv-SAS/Docs/README_Known_Issues.md#ki-141).
  - Note: Civ selection/action sounds can include several variants.
  - Replay to hear different samples.
  - Note 2: some Improvement 3D sounds as of now can play no or the wrong sound.
  - Try to click Play several times to hear the correct sound.
  - Note 3: Ambience 3D can start late; e.g;, dog begins around 5-20 seconds, and horn beings at around 8-20 seconds (They seem not to start right away, e.g., after entering city screen).
- New **Movies category** (bik, nif, dds) for **Victories**, **Wonders**, **Projects**, **Religions**, **Corporations** (if any; as of now in base AdvCiv there are none), and **Eras**, additional **audio support for non-bik** files (e.g. Religions have a nif + separate audio asset).
  - **Play button** added in related Sevopedia categories to redirect to their entry in Sevopedia Movies.
  - See [Sevopedia reworks (Movies (with audio support))](/_1_AdvCiv-SAS/Docs/README_Sevopedia_Reworks.md#example-091-movies-category-with-audio-support).
- New **Music category**: allows to play **~1750 audio scripts** (i.e. files) in Sevopedia, notably for **Techs**, **Leaders**, **Eras**, **Civilizations**, and many other.
  - **Play button** added in related Sevopedia categories to redirect to Sevopedia Music.
  - Approach broadly similar to that of Sevopedia Movies.
  - **Search bar support** and **widening** allows to find desired tracks.
  - Note: there are **multiple sounds in an item** for each civilization sound item, so **replay many times the same item to hear all civilization sounds**.
  - See [Sevopedia reworks (Music)](/_1_AdvCiv-SAS/Docs/README_Sevopedia_Reworks.md#example-092-music-category-1750-audio-scripts-playable-in-sevopedia).
- New **Expanded Text Panels** so History/Background **text** (e.g. of the Viking unit that is long and requires scrolling to be seen or of the Great General Unit (that includes uniquenames that require scrolling to be scanned quickly)) easier to read, with **EXPAND** and **CLOSE** buttons.
  - It occupies the whole item's page but **preserves the rest of the sevopedia page display** (footer, header, item bar and category bar) for easy browsing.
  - This allows to **browse other category items** (e.g., other leaders from sevopedia leader's item bar) **without having to close and reopen expanded view** everytime item is changed (e.g. when clicking on another leader).
  - Added as an optional helper shared across all eligible categories (main code in [_sevopedia_helpers.py](/Assets/Python/Contrib/Sevopedia/_sevopedia_helpers.py)).
  - Added optional **Playground** to experiment with **text color**, **background**, and **panel styles**, with `+` and `-` button to swap them and descriptions like `TEXT: PLAYER_LIGHT_BLUE_TEXT (147,188,252,255) (57/129)`, `BG: MAINMENU_SLIDESHOW_LOAD (2/8)`, and `STYLE: PANEL_STYLE_MAIN_TAN (36/50)`: helps choosing or testing optimal art style without having to tediously manually testing them one by one.
  - See [Sevopedia reworks (Expanded text panel)](/_1_AdvCiv-SAS/Docs/README_Sevopedia_Reworks.md#example-093-expanded-text-panel-with-expand-and-close-buttons).
- New **Expanded Content (Non-text; e.g. Animation) Panels** similarly so **animations** (e.g. of the Viking unit) can be viewed **maximized** (i.e. using all the item's page), with **EXPAND**, **RELOAD** (so e.g. the animation can be **reloaded** (e.g., to change unit color, reset animation, reset camera position/auto-rotation movement) **without having to exit and reopen expanded view**), and **CLOSE** buttons.
  - Applied for eligible categories: Bonuses, Improvements, Units, Buildings ("Buildings", "N.Wond.", "W.Wond").
  - See [Sevopedia reworks (Expanded Content (Non-text; e.g., Animation) Panels)](/_1_AdvCiv-SAS/Docs/README_Sevopedia_Reworks.md#example-094-expanded-content-non-text-eg-animation-panel-with-expand-reload-and-close-buttons).
- New **Expanded leaderhead panel** similarly with **EXPAND** and **CLOSE** button, and **leaderhead animation** with **expanded height and scaling width** (percentage of height **to maximize display of the full leaderhead** (because a suboptimal width/height ratio causes animation to show less of the leaderhead)), and leaderhead **attitude**, **action**, and (if set) **leaderhead art era buttons** synchronized with the ones in sevopedia leader.
  - See [Sevopedia reworks (Expanded leaderhead panel)](/_1_AdvCiv-SAS/Docs/README_Sevopedia_Reworks.md#example-095-expanded-leaderhead-panel).
- Added a new **Sevopedia Builds** category (e.g. **"Remove Jungle"**, **Chop Down a Forest**, **"Build a Road"**, **"Create a Farm"**) distinct from Sevopedia Improvements, notably with **FeatureStruct** panels and **mod-agnostic new Civilopedia blurbs-based Background panel** (added the new XML field `Civilopedia`> to BuildInfos).
  - Also added them to **Sevopedia Index** as **clickable entries with buttons**.
  - See [Sevopedia reworks (Builds)](/_1_AdvCiv-SAS/Docs/README_Sevopedia_Reworks.md#example-040-builds-category-eg-remove-jungle-build-road-create-a-farm).
- Sevopedia — Improvements: Added a new sortable **Improvement Weights (Leaders) Chart**, showing **Weights**, **Count** per Weight, and **clickable Leader buttons** for each weight and improvement.
  - Added **Graphical Only** (e.g., Land Worked, Water Worked) (similarly to how Hill and Peak are shown in Sevopedia Terrain) and **Special Map** (e.g., City Ruins, Tribal Village) so Sevopedia Improvements can redirect to them and for exhaustiveness, under these groupings.
- Sevopedia — Terrain: add new **Peak** add **Hill** entries (they were previously hidden in Base AdvCiv). Even though they technically are plot types, adding them helps show some information specific to these "terrains". See also [KI#153](/_1_AdvCiv-SAS/Docs/README_Known_Issues.md#ki-153).
- Sevopedia — Civilizations: Add **cities panel**. Add optional **grouping by Art style** (e.g. European, Asian).
- Sevopedia — Leaders: New **AI Personality panel** with raw attributes and cross-leader comparisons, and optional **grouping by civilization (with an icon)**.
  - See also [SevoPediaLeaderAIPValues.py](/Assets/Python/Contrib/Sevopedia/SevoPediaLeaderAIPValues.py).
  - Beautify **Traits with icon characters**.
  - Make a free promotion's UnitCombats span **in the same line** instead of taking too much vertical space, and **add a blank line between Traits** for readability.
  - Also **add linking** to their **corresponding Sevopedia Traits entry**: change in [CvGameTextMgr.cpp](/CvGameCoreDLL/CvGameTextMgr.cpp).
  - Add **Attitude** (furious, annoyed, cautious, pleased, friendly) and **Action buttons** (noanim/idle, greeting, agree, disagree) (imports from AdvCiv-SAS-NIF-Gallery), and (if set) **leaderhead art era buttons**.
  - On click, the animation changes its behaviour to that of the buttons clicked.
  - See also: [How to read the AI Personality panel](/_1_AdvCiv-SAS/Docs/README_AI_Personality_Panel.md#displaying-the-ai-attributes-in-the-ai-personality-panel-and-how-to-read-the-tablespanels).
  - Moved **BACK/NEXT/EXIT** buttons (in the **footer**) to **the left of the AI Personality Panel** so they are clickable despite intentional vertical overflow of the AI Personality Panel (to maximize usable space and show more rows/fields).
  - Note: **AI Personality Panel categories' organization** is adjusted depending on **vertical room** (**resolutionY**); e.g., since higher resolutions like 1440p have more vertical room that allows to show categories like ACL-related categories (that are not visible at 1080p), we optimize this further by moving **No War At** from `middle_categories` to `left_categories` so the middle column has enough room to fully show ACL-related categories.
- Also, make **item bar shorter** with a **per font size tunable width** to accommodate the **AI Personality panel wider columns when text is upscaled**.
  - Note: this applied **only for lower resolutions**, because higher resolutions have enough width to accommodate an enlarged AI Personality Panel when text is upscaled, so there is no need to reduce item bar width in Sevopedia Leader to accommodate it.
- Sevopedia — Concepts (`ConceptInfos`) (placed under the "Outdated" *Sevopedia category*): added reference entries — e.g. `concept_rivers`, `concept_global_warming`, `concept_customization`.
  - **"Outdated" is a Sevopedia category label** we use for pages **not maintained** to match AdvCiv-SAS rules; these exist for general Civ4 background and UI reuse.
  - See: [Concepts (Outdated)](/README.md#concepts-as-of-now-in-the-outdated-sevopedia-category).
- Sevopedia — Mods Infos (`NewConceptInfos`): Added **Mods Infos** (e.g., K-Mod, AdvCiv, AdvCiv-SAS).
  - Added **Legend items** showing the legend (e.g., for the Info Screen's new Score Tab; for the Sevopedia Leader's AI Personality Panel, Sevopedia Categories, abbreviation meaning (e.g., for numTxt `+50/-75` or `InC`; for the AI Personality Panel `T.W Likely -> getMaxWarRand` or `Build Unit % -> getBuildUnitProb`)), with `(Inverted)` information; for Sevopedia Speeds.
  - See also: [README.md (Legends)](/README.md#legends).
- Sevopedia — Corporations: **reworked** and **beautified** the page layout, with **new Competes With, Bonuses Consumed, Bonuses Generated, and Movie** panels.
  - Exposed `getBonusProduced` to Python in [CvInfo_Organization.h](/CvGameCoreDLL/CvInfo_Organization.h), [CvInfo_Organization.cpp](/CvGameCoreDLL/CvInfo_Organization.cpp), and [CyInfoInterface3.cpp](/CvGameCoreDLL/CyInfoInterface3.cpp) so the new generated-bonus panel is cleanly supported.
  - See [Sevopedia reworks (Corporations)](/_1_AdvCiv-SAS/Docs/README_Sevopedia_Reworks.md#example-15-corporations-category).
- Many entries (religions, some techs/buildings/units/bonuses/terrains/features) now use neutral encyclopedia-style blurbs (often Wikipedia-based) for clarity/context.
- Sevopedia — Traits: New **Trait Charts**: 2 **sortable** charts showing how widespread **pairs are in a Trait**, and **ranking for each Trait combination**, with **clickable leader buttons**.
  - The Leaders' panel's **header shows statistics** (e.g. "Leader 12/53 (22%)").
  - Additionally, **Beautified** and added a **new Background panel** with **mod-agnostic blurbs**.
  - See [Sevopedia reworks (Traits)](/_1_AdvCiv-SAS/Docs/README_Sevopedia_Reworks.md#example-15-traits-category-traits-charts-and-other-changes).
- New **Votes category** featuring **exhaustive XML info** about `VoteSourceInfos` (e.g., Apostolic Palace, UN) and `VoteInfo` (e.g., a resolution/vote).
  - Votes are **grouped by vote source**, **abridged** in item bar when necessary.
  - Example of panels include notably Source Civic, Source Free Specialist, Vote's Requirements, Effects, Force Civics.
  - Also added if relevant corresponding Vote Source and Vote panels or blurbs in Sevopedia Civics and Specialists.
  - Added **"Legend"** text similarly.
  - See also [KI#121](/_1_AdvCiv-SAS/Docs/README_Known_Issues.md#ki-121) and [Sevopedia reworks (Votes category)](/_1_AdvCiv-SAS/Docs/README_Sevopedia_Reworks.md#example-041-votes-category-voteinfo-and-votesourceinfos).
- New **Event Triggers category** featuring **exhaustive XML info** about `CvEventTriggerInfo` (e.g., "Careless Apprentice") and `CvEventInfo` (e.g., event 1: "Organize a rebuilding effort; event 2: "Investigate the cause of the fire"; event 3: "We are too pressed to spare any attention to a small fire") and their detailed effects, with a **"Legend"** similarly.
  - The implementation notably has repeated text prevention (e.g., use "x2", "x3" when bullets are the same), sevopedia links to other assets' pages (like traits in sevopedia leader), and **direct and indirect era groupings** (depending on whether the eventtrigger depends on tech's and/or prereqs directly or on related assets' related own conditions; see sevopedia legend).
- Sevopedia — Techs: New **Starting Techs Charts**: **sortable** charts showing Civilization **pairs and combinations** for **each starting tech**, mirroring the Traits' Charts.
  - New **Untradeable Techs** Chart shows per era untradeable techs similarly.
  - Add a new **Obsoletes panel** showing **Buttons (i.e. icons) of obsolete assets**, with an updated **thinner RedX art**.
  - Also **beautified** with a new **Enables** panel enriched with **more information** (Civics, Promotions, Bonuses, various effects such as "Centers World Map", "Can Adjust Commerce Rate", etc.), and a **new First to Discover** panel (Founds Religion, Receives a Great Person, Receives a Free Tech) and **Untradeable panel** for the currently selected tech (Emoji or "Yes").
  - See [Sevopedia reworks (Techs)](/_1_AdvCiv-SAS/Docs/README_Sevopedia_Reworks.md#example-16-techs-category-starting-and-untradeable-techs-charts-and-other-changes).
- Sevopedia — Specialists: new **Extra Slots** (showing slot, free slots, unlimited specialists) and **Extra Yields** panel showing detailed yield information per asset for each specialist.
- Sevopedia — Units: Notably prepend **unique names** before the **pedia text** in the History (i.e. the "Background") panel; e.g., "Albert Einstein", "Plato" for the Great Scientist unit; "Sargon", "Sun Tzu" for the Great General unit.
  - See also [Sevopedia reworks (Units)](/_1_AdvCiv-SAS/Docs/README_Sevopedia_Reworks.md#example-7-units-category).
- Sevopedia — Shortcuts: Add **Cheats** item (from the [CFC thread: All Keyboard Cheats for Civ 4](https://forums.civfanatics.com/threads/all-keyboard-cheats-for-civ-4.664273/)). **Beautify: Equalize tab indentation** (was seemingly aligned for font 2 instead of **label 3** following upscaling changes (note: other font sizes may be misaligned), and fixed a few misaligned rows).
  - Generalize **shortcut spacing** (e.g., `Alt+F4` -> `Alt + F4` as in Cheats item).
  - Order **alphabetically** shortcut items. Added an entry for the existing `Alt+S` shortcut (in **Map Layers** item) — opens the tile-label dialog (**"Enter caption"**) to add/remove text on the selected tile.

### UI (In-game)

- **Beautified** and **expanded**, and add **support for text upscaling** for miscellaneous **screens**: [CvDawnOfMan.py](/Assets/Python/Screens/CvDawnOfMan.py) ("The sun rises on the Year" greeting new game screen) is **expanded laterally and vertically** to **avoid scrolling**, add **icons** for buildings and units, add icon **chars for traits**; [CvTechSplashScreen.py](/Assets/Python/Screens/CvTechSplashScreen.py) (when a new tech is discovered) is moderately **expanded** to **avoid scrolling**, vertically center screen (was slightly excentered); [CvHallOfFameScreen.py](/Assets/Python/Screens/CvHallOfFameScreen.py) is **maximized to full screen** with enlarged columns, and **new Leader, Civilization, Player Color, and Traits columns** with **icons** or **colors** or **chars** (based on Info Screen's Score Tab: note that Civilization icon is approximative and will not work if Civilization name is not the base civ mod name (e.g. "The United States" -> "My American Custom Name Empire") (would add too much complexity in DLL for a working UI feature outside this edge case); note 2: trait info is indicative only (if traits in XML were different when the save file was made (e.g., expansive trait removed, leader trait allocation changed), this information will be lost)); [CvReplayScreen.py](/Assets/Python/Screens/CvReplayScreen.py) is **maximized to full screen**; [CvDanQuayle.py](/Assets/Python/Screens/CvDanQuayle.py) ("you have displayed the leadership abilities" post-game screen) is **maximized to full screen** and has a **greatly expanded leader NIF** and **maximized vertically score table** to **avoid scrolling**.
  - See [README.md (Miscellaneous screens)](/README.md#miscellaneous-screens)
- **Reorganized advisors**: **merged** the **espionage** screen into the **Foreign Advisor**, the **Economic Advisor's Finance Tab** into the **Domestic Advisor**, moved away the **Environment Tab** from the former **Economic Advisor** (now removed), the **Religion** Advisor and **Corporation** Advisor (so it is always visible and easier to access) into the new **Policy Advisor**; so they are now **tabs of these instead** (deleted the old now obsolete lone advisor .py files); **swap advisors**: **Victory screen** to F9 and **Info Screen** to F8 so the Info Screen is easier to split to F7 and F8 later on and so layout is **more logically organized** from internal to external advisors; **split advisors**: the **Foreign Advisor** is now split into **Foreign Trade Advisor (F4)** (TREATIES, BONUSES, TECHS, CITIES), and **Foreign Diplomacy Advisor (F3)** (RELATIONS, GLANCE, INFO, ESPIONAGE) as it **was crowded** to reduce the need for **tab navigation** and for **clearer thematic differentiation**, using a shell (i.e. almost empty file for **linking only**) [CvForeignDiplomacyAdvisor.py](/Assets/Python/Screens/CvForeignDiplomacyAdvisor.py) file for the 2nd advisor.
- Rearrange Advisor **keyboard shortcuts** (in [CIV4ControlInfos.xml](/Assets/XML/Units/CIV4ControlInfos.xml) and DLL files like [CvEnums.h](/CvGameCoreDLL/CvEnums.h), [CvEnums.h](/CvGameCoreDLL/CvEnums.h), [CvGameInterface.cpp](/CvGameCoreDLL/CvGameInterface.cpp), and [CyEnumsInterface.cpp](/CvGameCoreDLL/CyEnumsInterface.cpp)) and adjust the **order of advisor buttons** in map view and **Sevopedia shortcuts** corresponding text to match the advisors changes.
- Add **debug mode support** (for player selection) in the Domestic Advisor. Add **Barbarian** to **selectable leaders** in debug mode in advisors where it is relevant: Domestic Advisor, Policy Advisor, World Advisor, and Info Screen.
- Preserve **selected debug/vassal perspective** when reopening or switching tabs in supported advisors where the selected player is meaningful (all supported advisors except for the Victory Screen's **Settings tab**: it intentionally stays anchored to the **real active player**, because it describes that player's game setup rather than a selected rival's victory progress).
- Preserve the **Military Advisor Map tab selected leader(s)** (icon clicked on) when **switching** Military Advisor tabs or **closing/reopening** the advisor (when valid/safe in relevant context (e.g., player still alive, known outside debug mode)).
  - Previously, the Map tab reset its leader selection to the active player whenever the tab was redrawn, which **was tedious** in **debug mode** when comparing leaders, following unit production counts, or repeatedly checking a specific AI's units.
  - Change in [CvMilitaryAdvisor.py](/Assets/Python/Screens/CvMilitaryAdvisor.py).
  - See also [KI#145](/_1_AdvCiv-SAS/Docs/README_Known_Issues.md#ki-145).
- Allow the **current active player** (i.e. the human player or whichever player is active if cheats are enabled) to **select and view its alive vassals' information** in advisors' **player selection dropdown** even outside debug mode.
  - This follows the active player (vassals are subordinate enough to make their domestic/world/policy state relevant).
  - This **does not reveal unrelated players or hidden map** data as debug mode would.
- Always show advisor tabs where hidden tabs reduced discoverability: Foreign Advisor **Glance** and **Cities** tabs are now always shown, so the corresponding old BUG menu visibility toggles are now removed; Victory Screen **Resolutions** and **Members** tabs are also always shown, with a fallback table showing required wonders (as of now Apostolic Palace or United Nations) when no vote source is currently active.
- Preserve built heavy **advisor tables** where safe, to avoid rebuilding expensive UI tables when their player/history/language/layout/settings signature has not changed.
  - This is used for the **Info Screen Timeline** table across close/reopen, and for the **Military Advisor Battles** table across **same-open advisor tab switches** (Map <-> Battles/Summary/Composition remains lightweight as long as the Military Advisor is not exited).
  - We deliberately do **not** preserve the whole Military Advisor across full close/reopen: testing `screen.setPersistent(True)` there made very large late-game Battles histories faster to reopen, but broke Civ4's native minimap lifecycle, causing the Map tab to reopen as an empty blue panel and also break after debug-mode transitions until save reload.
  - So the Battles cache is kept for tab switching only, while full Military Advisor close/reopen rebuilds the screen/minimap for reliability.
  - See also [KI#129](/_1_AdvCiv-SAS/Docs/README_Known_Issues.md#ki-129).
- **Expand** and most **advisors** so they scale to **game resolution dynamically** instead of hardcoded advisor screen dimensions.
  - The code was also refactored to improve **dynamic scaling**, ensuring **most UI elements automatically realign** if dimensions are changed.
  - Advisors are now **excentered** so **overall more information** is displayed, thus **reducing the need for scrolling**, while still **preserving key relevant information** for display (scoreboard, map, commerce sliders and values, etc.).
  - Some of their dimensions are **harmonized**.
  - Note: some **right space for scoreboard** and **top space for the top bars** of the main interface was made (since commerce sliders are now as of now on the right side, they don't need left space anymore).
  - See [Main Advisors reworks](/README.md#main-advisors-reworks).
Make some of the data-dense advisors use using a **higher constrast background theme** so text is **more readable** (e.g. plain dark grayish in Overview tabs (Domestic Advisor), plain dark brownish in the Score Tab (Info screen), plain blue in the Battles tab (Military Advisor)).
- **Beautified** the **Domestic Advisor**: now shows the **button (i.e. icon) of the current item being built** (unit, building, project, process) in "Producing" column.
  - Use the **shared advisor shell layout** (**top/bottom bars** + single content panel, remove the old exit icon) so Domestic Advisor **mirrors other advisors' structure** instead of a floating table, and is now has **tabs**; **enhanced** the Domestic Advisor so it now has **4 overview tabs** with detailed city information (modifiers, yields, specialists, etc.) with a corresponding **legend link**, and an **enter city on click button** (mirroring the info screen's wonders tab).
  - Change in [CvDomesticAdvisor.py](/Assets/Python/Screens/CvDomesticAdvisor.py).
- Minor **beautified** the [CvCustomizableDomesticAdvisor.py](/Assets/Python/Screens/CvCustomizableDomesticAdvisor.py): increase **legend culture text width** so it is not cramped when upscaled.
  - Note: although it is functional, the Custom Domestic Advisor is **not supported** in AdvCiv-SAS as of now, so it is lacking the Finance Tab.
  - See also [KI#123](/_1_AdvCiv-SAS/Docs/README_Known_Issues.md#ki-123).
- New **Policy Advisor**.
  - **Beautified**: enlarge **top panels text** so **long civic names don't overflow**.
  - **Remove multiline nationality** information which was **uninfomative** and **crowded vertically** in the **Religions tab** and the **Corporations tab**; use more precise and compact `Leader (ID)` and `HQ (ID)` **one-liner instead** (**ID** also handles **one leader name being used by several players**); **increase row spacing** (now **unified** (was crowded)).
  - **Remove multiline wrapped bonuses** splitting logic in the **Corporations tab**: use one natural comma-separated line (bonuses row height reduced/standardized) since there is enough space after expanding the Policy Advisor and it is more readable.
  - Also add **new rows** (**Bonuses Generated**, **Yields Generated**), and add **first column descriptions** for rows (as of now `Built By:`, `Consumes:`, `Generates:`, `Generates 2:`).
  - Add a **Great Person icon** in `Built By:` row.
  - Change in [CvPolicyAdvisorScreen.py](/Assets/Python/Screens/CvPolicyAdvisorScreen.py).
- **Foreign Advisor**: refined its tabs so they can display **up to 12 players**.
  - Notably renamed the Active tab to the **Treaties** tab and **beautified** it with **icons**.
  - Resource-only deals remain omitted, but mixed resource/treaty or resource/GPT rows show every term that their cancellation action affects, with bonus, gold and leader icons for easier scanning.
  - Show **gold per turn** in **Bonuses Tab** even when **not connected to trade network** (if gold trading is available): previously this was hidden, and would require tediously opening the diplomacy screen of each player, and this is now also consistent with flat gold being shown in techs tab for all players (if gold trading is available).
  - See [KI#132](/_1_AdvCiv-SAS/Docs/README_Known_Issues.md#ki-132) and [KI#241](/_1_AdvCiv-SAS/Docs/README_Known_Issues.md#ki-241).
- **Foreign Advisor**: Always use the unified **table-style info layout** (`drawInfoImproved`) for the Info Tab for stable column behavior: the old hardcoded text spacing (`drawInfoOriginal`) did not **scale with upscaled font text** nor **adjust depending on advisor screen size**.
  - Plus the improved layout has additional **information** like general or specific **colored leader attitude text** related to civic/religion choices (e.g., `[-2]` or `[+1]`), so the old **Enhanced Info Tab** BUG option was removed (to better support upscaled text and game resolution changes).
  - The remaining Glance-tab attitude dropdown now has its own explicit **Glance Tab Attitudes** row for clarity instead of looking attached to the Info-tab option, and now defaults to **Both (numbers and smilies)** instead of the previous **Numbers**-only default, after Large Pangaea/default-player testing showed an upscaled long case such as `-17` with at-war and worst-enemy icons still displayed fully after the per-page player cap.
  - Higher-player games were not tested here and may behave differently; the tunable cap remains the adjustment point if needed.
  - See [KI#139](/_1_AdvCiv-SAS/Docs/README_Known_Issues.md#ki-139)
- **Foreign Advisor (Glance Tab)**: Added **"Willing to become vassal"** and **"Vassal"** **icons** in the Foreign Advisor's Glance tab in the human player's row, to help avoid the need to tediously check these up through other screens, in [CvForeignAdvisor.py](/Assets/Python/Screens/CvForeignAdvisor.py), and in [CvDLLWidgetData.cpp](/CvGameCoreDLL/CvDLLWidgetData.cpp) for the corresponding **tooltip** (on hover).
  - See ["Willing to become a vassal" and "vassal" icons](/README.md#willing-to-become-a-vassal-and-vassal-icons-in-foreign-advisors-glance-tab).
- Cap **maximum number of players per page** in the **Glance Tab** (e.g., max 16 players per page gives 3 pages at SAS48 Worldsize (if all players are known)), with **clickable left and right arrows for navigation**.
  - Fixes **issue of Glance Tab clipping horizontally** if more than 24 players are known and columns being too tight.
  - Last page visited is **remembered and resumed from** on Glance Tab next opening.
  - Diagonal player-vs-itself cells are now left empty instead of showing incorrect/inconsistent `+0` attitude display; see [KI#140](/_1_AdvCiv-SAS/Docs/README_Known_Issues.md#ki-140).
  - See also [KI#134](/_1_AdvCiv-SAS/Docs/README_Known_Issues.md#ki-134).
- **Foreign Advisor (Espionage Tab)**: **Beautified** similarly; notably **removed redundant Espionage slider** now that commerce sliders are on the right side of main interface in map view, and **improved text spacing** that **was colliding or overflowing** beyond its panel, and **moved + - buttons under the leader icon** where there is more room and it is more intuitive and easier to click it.
  - Removed redundant and distracting `Weight:`, espionage char icon, and `Cost` text (add a **Legend** link explaining the row fields), and added a new **compact mode** with tunable met rivals cap (0 effectively means always use it) so large player counts (e.g., in SAS48 world-size) are visible and selectable.
- Reworked the **Military Advisor** in [CvMilitaryAdvisor.py](/Assets/Python/Screens/CvMilitaryAdvisor.py) now with Unit Combat and Units **icons**.
  - Note: **unit icon size** is **increased** at **higher resolutions** (e.g., from 24 to 32 if resolution height >= 1440), and **promotion icon size** is **increased** (no longer DLL-hardcoded (was 16)).
  - **Lift up the unit panel to full height** (reduces the need for scrolling vertically) and **expand it laterally** to **accommodate lengthy unit rows** (e.g., avoid line wrap if unit name is long, if there are many promotions).
  - Make **show/hide individual unit text clickable** too (more accessible than the small +/- button alone).
  - **Increase leader icon bar from 1 to multiple rows**, and **do not reduce icon size until all rows are full**.
  - For example, at 1080p, 5 rows allow to show up to 45 players (including barbarian) at full leader icon size before leader icon size is reduced.
  - **Reduce Leader icon size reduction** when all rows are full (**mild reduction**, no longer halved).
  - In **debug mode**, the **Map tab** now draws the **full minimap section** instead of only the active player's known minimap section: the full map is visible in debug anyway, and this lets the human/debugger locate selected players' units even outside the active player's revealed area.
  - See [KI#142](/_1_AdvCiv-SAS/Docs/README_Known_Issues.md#ki-142).
  - See also [KI#123](/_1_AdvCiv-SAS/Docs/README_Known_Issues.md#ki-123), [KI#130](/_1_AdvCiv-SAS/Docs/README_Known_Issues.md#ki-130), and [KI#131](/_1_AdvCiv-SAS/Docs/README_Known_Issues.md#ki-131).
  - Note 2: optionally changed **default first tab opened at Civ4 launch** as of now to **Map tab** for easier immediate unit review.
  - Note 3: as of now the [CvBUGMilitaryAdvisor.py](/Assets/Python/Screens/CvBUGMilitaryAdvisor.py) is not supported.
- Added a new **Battles tab** in the Military Advisor, showing as a **sortable table** the detailed **record of each battle** for the selected player in debug dropdown (or the active player or its vassals outside debug mode): **Turn**, **Year**, **Result** (Won, Lost, Retreat) as emoji, **Estimated Odds %** (simple starting effective strength comparison, not exact Civ4 combat odds), **Role** (Are we the attacker?) as emoji, **base Strength** (XML), **start effective Strength** (including modifiers at that battle), **end effective Strength**, **max effective Strength**, **XP before combat** (yellow if Great-General-led), **Our Unit**, **Their Unit** (unit name in yellow if Great-General-led), **same** strength/XP columns for their unit, **Civ icon**, **Player icon**, **Player ID** (to cover the same leader picked by several leaders (custom games, etc.)), **Captured Unit type**, **Captured Unit icon**, **Hill or Peak icon**, **Terrain icon**, **Feature icon**, **Click Plot icon** (**on click camera is moved to this plot** (as the wonder tabs' (Info Screen) text does), with distinct art for normal plot, defended city, and captured city battles). Icons are **hoverable to sevopedia** (or have a camera effect). **Text color** is added for Result (Won: green; Lost: red; Retreat: yellow), effective Strength (white: 100%, green: > 66%, yellow: > 33%; red <= 33%), and captured units (we gained a unit amount: green; we lost a unit amount: red). Implemented a **legend link**, and **LOG Button** (as the Timeline tab). Examples of LOG: `163 | 760 AD | Lost | 43 | A | 6 | 6.04 |  | 7.20 | 8 | 0 | Camel Archer | Longbowman | 5 | 0 | 4 | 8.10 | 5.04 | 9.00 | 9 | 0 |  |  | TERRAIN_GRASS |  | 54 | 30`, `133 | 10 BC | Won | 64 | A | 3 | 3.30 | 1.81 | 3.30 | 2 | 0 | Trireme | Galley | 1 | 0 | 2 | 1.84 |  | 2.20 | 48 | 0 |  |  | TERRAIN_COAST |  | 32 | 10`. Main files changed: [CvMilitaryAdvisor.py](/Assets/Python/Screens/CvMilitaryAdvisor.py), [SASBattleHistory.py](/Assets/Python/SASBattleHistory.py), [CvEventManager.py](/Assets/Python/CvEventManager.py), [BugEventManager.py](/Assets/Python/BUG/BugEventManager.py), [CvUnit.cpp](/CvGameCoreDLL/CvUnit.cpp), [CvDllPythonEvents.cpp](/CvGameCoreDLL/CvDllPythonEvents.cpp), [CvDllPythonEvents.h](/CvGameCoreDLL/CvDllPythonEvents.h), [CvEventReporter.cpp](/CvGameCoreDLL/CvEventReporter.cpp), [CvEventReporter.h](/CvGameCoreDLL/CvEventReporter.h).
- Added a new **Summary tab** in the Military Advisor, showing a **synthetic view** of our Military:
  - **Support** (detailed Cost and Supply, and Unit Cap numerical breakdown (e.g., `P x R x M    (1 x 100 x 82) / 10000 = 0 gold icon char`), (Requires AdvCiv-SAS 6586+) **data-driven Military Buildings** (up to five XML-effect-selected rows, including up to two national buildings; civ-specific replacements use their actual localized names, icons, and effects), Military Production (some not shown for concision as panel is mostly full) and XP information, and Allied Hammers for total hammer value of allies' (e.g., vassals, same team) army)
  - **Army** (detailed numerical overview for military/civilian units, Domain repartition (Land/Sea/Air), Strongest, Costliest, Total/Avg Hammers as a power estimation of units, Power Rank (Known) (e.g., `9/11` (in fact `44/48` if debug mode is enabled for spoilers)), Top promotions, Covert Units (Invisible, Spies, Hidden Nationality), And upgradeable units costs (Min/Avg/Max/Total))
  - **Deployment** (Allied/Neutral/Enemy Cities/Territory/Roaming information with Avg Defense and Best defense of our cities, (Requires AdvCiv-SAS 6586+) **data-driven fortification-building coverage** (up to two XML-effect-selected rows, e.g., `1 / 2`), Wounded information (Max/Avg/Min and Full/High/Medium/Low Health (e.g, `15 (100%)`)), and Battles reusing the [SASBattleHistory.py](/Assets/Python/SASBattleHistory.py) added with the new Battles Tab (Won/Retreated/Lost (e.g., `35 (33%)`)), Good-Luck Wins and Bad-Luck Losses counts (`e.g., 11 (31%)`), Best/Worst (e.g., `6.3 vs 10.0 1300 AD (39%)`)); also added a **legend link**.
- Added a new **Composition Tab**, as a set of **sortable tables** for **Current** (unlike saved data in the Statistics Tab in the Info Screen) **Units** (non-civilian, including Animal (including Barbarian in debug mode)), **Promotions**, and **Unit Combats** **counts**, default-**sorted from most to least** for no tedium (as done in the Statistics Tab).
  - See also [KI#129](/_1_AdvCiv-SAS/Docs/README_Known_Issues.md#ki-129).
- Similarly Beautified and Enhanced the **Technology Advisor** in [CvTechChooser.py](/Assets/Python/Screens/CvTechChooser.py) so it uses more of the screen while still preserving key information.
  - Players can **optionally** set an **alternative lower right space for scoreboard** on **lower game resolutions** (e.g., 1920*1080) so that an **extra column or more is visible** in the technology advisor, and an **alternative top space space** for **advisor buttons** (in map view) because this alternative right space may cover these advisor buttons.
  - **Boldened title** for consistency.
  - See also [KI#114](/_1_AdvCiv-SAS/Docs/README_Known_Issues.md#ki-114).
  - Shifted the **default horizontal tech-tree start position** left so, at 1080p, the current visible Ancient-era last column renders fully and symmetrically instead of being partially cut off to the right, and without having to tediously nudge the horizontal scrollbar at game start.
  - **When opening** Technology Advisor, **right-align scoreboard leader text** so it is easier to read (because text is often not visible due to advisor width expansion).
  - **On Technology Advisor close**, it **reverts back** to usual alignment.
- **Inverted BUG Options** for the **Technology Advisor** and the **Religion Advisor** so they are more accessible.
  - They are now **enabled as a default**, and ticking their box disables them.
  - Rewrote their text for clarity (e.g. "GP Research" -> **"Hide bulbing indicators"**).
  - See [Inverted BUG options](/README.md#inverted-bug-options).
- New **World Advisor**: shows **detailed BFC, suburbs, and territory geography** information as **sortable tables**.
  - BFC tabs show **counts per city (rows) and per field (columns)**, and have a **click to enter city button** (similar to the Info Screen's Wonders tab).
  - The BFC1 tab shows mostly **plot type** (Peak, Hill, Water, Land) and other information (**River, Lake, Coastal Land, sortable City found year and population**) and **Terrains/Features** information; the BFC2 tab shows mostly **Bonuses** and **Improvements** with other information (**Routes**).
  - The Territory tab shows a **condensed count summary** with 4 tables of the former BFC tabs, with a BFC, suburbs (i.e. plots outside BFC), and **total columns** for each field (rows), **total fields (rows)** for each table, **hover support** and **pedia redirect on click** (if a corresponding pedia page exists), and a legend link.
  - Note: totals are not the strict sum of all rows as some may overlap (e.g., river and hill).
  - The **environment tab** is now part of the World advisor (better thematic fit).
- Did a similar rework of the **Info Screen** in [CvInfoScreen.py](/Assets/Python/Screens/CvInfoScreen.py); in particular in **Top 5 Cities** we now use a multilist to display more wonders in a prettier way, and in the **Wonders** tab we now **show the Wonders' Buttons** (i.e. icons) in the first column (**click to move to city** mechanic is moved to the City column instead); the **Statistics** tab now also shows buttons (plus **change default ordering** from alphabetical to **from most to least Built** (avoids tedious manual user ordering each time)), and the **Graph** Tab and possibly other tabs also show buttons (e.g., **leader icons before text**, **emojis**).
  - Also added other information (e.g., turn numbers in the Graph Tab).
  - **Removed** the **Improvements table** from the Statistics tab, because it was **inconsistent** with how Units and Buildings show all-time total **built**, Improvements would only show **current** possession (e.g., building a farm, replacing it with a cottage, then replacing it again with a farm would be Current 1 farm, not total built 2 farms), plus the World Advisor is a **better thematic fit**.
- Additionally, added in the **Info Screen** a new **Timeline Tab**, showing a **text-colored spoiler-free** account of the key moments that happened each turn.
  - Based on the **ReplayScreen**'s text, so **preserves information on save reload** unlike the Event Log.
  - Optional **LOG button** allows to export to `PythonDbg.log` text file the output, and has optional **caching** for performance.
  - Change in [CvInfoScreen.py](/Assets/Python/Screens/CvInfoScreen.py).
  - See [README.md: Timeline Tab](/README.md#timeline-tab-in-the-info-screen-advisor).
- Added in the **Info Screen** a new **Score Tab** showing a **sortable (default ordered by highest score) table** with all relevant scoreboard fields, plus **new fields** like Player color, Player ID (to distinguish identical Name/Civ/Leader/Player Color), Traits (T1 and T2) as **icon chars**, total Power (of a player, distinct from power ratios), total Power per City, Land%, V/M, Tech (total techs).
  - Added a clickable **"Legend"** text that **redirects to Sevopedia Concepts' "Score Tab Columns" page**.
  - Note that unlike military ratios, PowT and PoT/C are shown even for the human player, since these vary every turn.
  - Change in [CvInfoScreen.py](/Assets/Python/Screens/CvInfoScreen.py).
  - See [README.md: Score Tab](/README.md#score-tab-in-the-info-screen-advisor).
- Beautified the **Victory Screen**: **added Buttons/Icons** (Leaders, Religions, Apostolic Palace, UN, Spaceship parts, Traits as chars, etc.), **Thousands Separators** (note: now the same in all languages, e.g., `,` so others can be added/used if needed), **Victory Culture Percentages** (mirroring the changes in [CvInfoScreen.py](/Assets/Python/Screens/CvInfoScreen.py)).
  - Add **grid size** information in Settings Tab (e.g., `Large World Size (74 x 52)`) and optionally show the cached **AI Map Heaviness** fields (`landHeavy` and `navalHeavy`) used by DLL behavior (with a **legend link**).
  - Change in [CvVictoryScreen.py](/Assets/Python/Screens/CvVictoryScreen.py).
  - See also [KI#115](/_1_AdvCiv-SAS/Docs/README_Known_Issues.md#ki-115).
- Beautified the **Diplomacy Screen**'s **trading rows** with **icons** (e.g., Gold, Gold Per Turn, World Map, Open Borders, Vassal, Defensive Pact; and **leader icons** (make peace / declare war / embargo)). Change in `CvPlayer::getItemTradeString`. See [README.md: Diplomacy Screen](/README.md#diplomacy-screen).
- **Main Interface (Common)**: Moved **minimap to the right-edge** for quicker read.
  - Add **details** to most **progress bars**; e.g. in the Tech bar `Fusion: 2245 / 5774 (35)`.
  - Show **chars in the GP bar even if 100%** (e.g. `Great Scientist: Lyons (3) - [RESEARCH_CHAR] 100%`) Change in [GPUtil.py](/Assets/Python/BUG/GPUtil.py) and [CvMainInterface.py](/Assets/Python/Screens/CvMainInterface.py).
  - **Swap top left** (e.g., Treasury) **and top right** (e.g., Turn and Year) **text position**, and optionally **move commerce sliders to the right side** (allows to freely expand advisors so they use the entire left side, and is visually closer to scoreboard and most mouse controls) **in city screen or map view**.
  - **Reduce commerce sliders fast increment** (`++ and --` optional buttons ("Min/Max Buttons in BUG Menu")) from **100 to 50** and make them **tunable**.
  - When **Min/Max commerce buttons** are enabled in **map view**, move the green commerce rate values (e.g. `+297/Turn`) into a **compact row below the sliders** (e.g. `[RESEARCH_CHAR]297; [ESPIONAGE_CHAR]16`) so the slider block can be shifted right enough to keep both the **percentage text** and the **Min/Max buttons** visible near expanded advisors such as the Technology Advisor, while avoiding right-edge overflow in normal map view too.
  - Show **Era text default ON** (as of now on top left); **strip " Era" suffix** so text is now only e.g., `Renaissance` not `Renaissance Era` (would overflow during golden age when text is upscaled, plus it is unneeded).
  - See [Main interface rework](/README.md#main-interface-rework).
- **Main Interface (City Screen) - Breakdowns**: Enhanced with a new **Great Person +n ICON** information in relevant Buildings' rows; added a new **Specialists Breakdown** and **Culture Breakdown**.
  - (Requires AdvCiv-SAS 6546+) Hovering either compact breakdown shows one combined, font-upscaling-aware tooltip for both Great Person Points and Culture, avoiding mouse movement between the adjacent rows, which aims to reduce user tedium.
  - It names every source with the same icon characters used by the compact formulas, colors key modifier/rate/progress values, adds at-a-glance progress percentages alongside the exact fractions, and includes the post-modifier Culture additions and turn estimates.
  - If the Culture Breakdown's second row (because it is the longest) is likely to be too long (as of now when text is upscaled and resolution is low (as of now label text > 3 and resolution width < 2560)), the spacing is reduced a bit (but formula's values shown are the same) so it fits.
  - (Requires AdvCiv-SAS 6207+) **Exact Great Person and culture modifiers in the Specialists and Culture Breakdowns:** they read the combined DLL modifiers directly instead of estimating them from already rounded final or base rates, so values such as +25% and +50% are no longer omitted or shown as misleading intermediate percentages.
  - See [KI#308](/_1_AdvCiv-SAS/Docs/README_Known_Issues.md#ki-308) and [KI#308.2](/_1_AdvCiv-SAS/Docs/README_Known_Issues.md#ki-308.2).
  - (Requires AdvCiv-SAS 6544+) The upper row includes exact Culture-slider commerce among the base sources affected by the displayed Culture modifier.
  - The lower row separately and permanently shows Build Culture and No-Espionage's transferred Espionage as post-modifier additions (including zero values), followed by the exact final Culture rate and progress.
  - Integral values omit redundant `.00`, while nonzero hundredths remain exact.
  - See [KI#1063](/_1_AdvCiv-SAS/Docs/README_Known_Issues.md#ki-1063).
- **Main Interface (City Screen) - Bottom**: added **optional extra rows** in the **Production Chooser Bar** (with optional separate blue ribbon, bar Y position and bar height position fine-tuning tunables (e.g., helps remove needless scroll bar when there are not more than max rows, or keep display symmetric or prettier)) and **optionally hide the unit bar** (`"PlotListPanel"`) (note: does not affect the production chooser outside of the city screen)
- **Main Interface (City Screen)**: added **Buttons** (i.e. icons) in the **Production Queue**; fixed some hard anchored items that now **dynamically** adjust to sides' width.
  - **Beautified** it as well: removed the 3 gray bars ("Trade Routes", "Buildings", "Specialists"), increased sides' width, made bonuses columns even in width, and other changes.
  - These changes allow to now display more information and reduce the need for scrolling or hovering.
  - Also **enlarged** the **Bottom-Left** and **Bottom-Right** panels so they align dynamically with the enlarged **Side Panels**.
  - Enhanced the **city bar** and **other bars**, e.g., `Lakamha: 16 - 12372 (4500BC)` or `Growing - 30/43 (4 Turns)`.
  - Adjust **city camera** to the north (tunable) so extra rows can be shown without overflowing into bottom BFC plots in `CvGame::onCityScreenChange`.
  - And other changes.
  - See also [KI#101](/_1_AdvCiv-SAS/Docs/README_Known_Issues.md#ki-101).
- **Main Interface (City Screen - Widgets/Clickable Buttons)**: Add **redirect to Sevopedia on click** (preserving `WIDGET_HELP` hover) for **Religion** and **Corporation** icons.
  - Add a tunable **city bonus panel render mode**: **mode 0** keeps the old AdvCiv-SAS text glyphs (Civ4's `screen.setLabelAt`: smaller; also used as overflow fallback), **mode 1** uses base AdvCiv-style framed image buttons (Civ4's `setImageButton`) but now with **larger icons** when there is room, and **mode 2** uses frameless DDS rendering (Civ4's `addDDS`) with the same larger-size logic for a cleaner look.
  - Active non-obsolete **Bonus** icons can open Sevopedia too (mode 0 text glyphs did not redirect to Sevopedia in testing; mode 1: left/right click; mode 2: right click only in our Python-only attempts so far).
  - Split the **Draft group** into hurry controls on the right (and a **new draft button**) and config options on the right.
  - Add building list filtering buttons ("Show All Buildings", "Show Regular Buildings Only", "Show National Wonders Only", "Show World Wonders Only") and restored **Production Chooser Bar** row-jump buttons with clearer hover text ("Scroll to Units/Buildings/Wonders").
  - Reduce **food/production top bars' size** so text does not overflow when upscaled (also helps preserve overall distance to side panels due to increasing base font size from body to label).
  - Add **commerce sliders fast increment buttons** (`++ and --` optional buttons ("Min/Max Buttons in BUG Menu")) in city screen (matching the ones in map view) (not added in Finance Tab or Espionage Tab because these still have access to commerce sliders as of now at their right in map view).
  - Change in [CvMainInterface.py](/Assets/Python/Screens/CvMainInterface.py).
- **Main Interface (Map View)**: **Reordered buttons** (end turn, city change arrows) and wrapped them in a **pretty panel**.
  - Added buttons to unit stack, selected unit.
  - Added **help text box** (`DefaultHelpArea`) (on the left side) optional **width adjustment** (negative value shrinks it).
  - Make **x and y positions** (resolution-scaled), and **width** (hard pixel; not resolution-scaled) of the **unit bar** (`PlotListPanel`) **adjustable**, and **fix horizontal overflow of unit bar into scoreboard** when unit bar is full and scoreboard is hovered on (or toggle lock scoreboard hover is ON) (see [KI#124](/_1_AdvCiv-SAS/Docs/README_Known_Issues.md#ki-124)).
  - Make **left-click on GP bar** open the **corresponding city's city screen**.
  - Added a **Toggle show map annotations** (Alt + S **captions**, Alt + X **BFC marks**) button (next to the other buttons **above the Minimap**); the toggled state **persists on save and load**; reorganized buttons as part of this change (this takes the Event Log's former HUD icon former (it gets another one instead)).
  - To accommodate this new minimap button's width, **reduce minimap buttons horizontal interspacing** so **last minimap button doesn't collide horizontally with globe view** button (except for squarish maps like as of now `BTG_Cross.py`, where this is a fine trade-off, and still improved significantly with this adjustment).
  - See [README.md (Map annotations)](/README.md#map-annotations) and [KI#135](/_1_AdvCiv-SAS/Docs/README_Known_Issues.md#ki-135).
  - Optionally show **Plot coordinates on hover** (e.g., `63,46`) in hover help text.
  - This helps discuss screenshots, review, or compare directly with BBAI logs and DLL diagnostics.
- **Main Interface ((Map View) Scoreboard)**: do **not draw beyond** bottom of commerce sliders (previously it would overflow beyond top of the screen), add **+** (up, fast up) and **-** (down, fast down) **scroll buttons** (e.g., 1 by 1, 5 by 5) to choose **first and last leader shown** when the **number of leaders is high** (e.g., if there are 48 leaders to display and the scoreboard can only show ~30 leaders max).
  - Note: the Info screen's score tab allows to see all leaders so this is a fine trade-off to avoid vertical overflow.
  - Add a **Center scoreboard scroll on the active player** button, and a **Toggle autocenter scoreboard scroll on the active player** button (so that it auto does it without having to tediously redo it regularly; note: if the scroll up/down buttons are clicked, it disables autocenter to apply manual scrolling (can be reenabled by reclicking the toggle autocenter button again)) (default state at Civ4 launch as of now optionally ON).
  - Add a **Toggle lock scoreboard hover button** so scoreboard is always hovered on even without hover (e.g., to see power ratios without having to hover), with an **optional default value** (as of now ON, to avoid always tediously clicking it the first time).
  - Note: in first turns of the game the lock button effectively has no effect since power ratios are not yet visible.
  - Add a **Toggle Background Style button** (AdvCiv **default at each save load/new game kept**; tunable) (e.g., blue panel, black50, empty (transparent)).
  - Change in [Scoreboard.py](/Assets/Python/BUG/Scoreboard.py) and [CvMainInterface.py](/Assets/Python/Screens/CvMainInterface.py).
  - See [README.md (Scoreboard)](/README.md#scoreboard).
- **Main Interface (civilization flag):**
  - **Removed the civilization flag** from the production buttons bar (next to minimap) to show more buttons instead and have a more efficient display.
  - Instead, added **optional compact top-left civilization flag** (with same preserved BtS hover/click behavior) (if **enabled**, left **top text spacing is shifted to make room for the compact flag**).
  - Unlike base AdvCiv, this **optional flag** is **also visible in city screen** because it does not interfere with bottom city-action buttons.
  - This flag is useful to know player civ, hover for player traits, and click to capital's city screen.
  - Default disabled (tunable) as it can be distracting and undesired.
- **Worker recommendation highlights disabled**: turned off tile coloring for "worker-recommended plot to improve" (in `CvGame::updateColoredPlots` within `CvGameInterface.cpp`).
  - Reasons: (1) avoids extra computation in both debug ([see for related info this autoplay info](/_1_AdvCiv-SAS/Docs/Modding_Ressources/README.md#how-to-autoplay-let-the-ai-play-for-you-super-fast-gameplay--testing-tool-in-map-loaded-save-file-new-game-etc-view)) and normal play; (2) our worker logic now relies on `CvUnitAI::AI_bestCityBuild`, so the vanilla highlight often disagrees and can mislead; (3) the highlight was easily confused with **city site** recommendations.
  - City-site suggestions remain enabled and intentionally prominent; in practice, they frequently pick **strong** locations—quite often as good as, or better than, typical manual choices.
  - See [Settlers (AI)](/_1_AdvCiv-SAS/Docs/README_Main_Changes_Guide.md#settlers-ai).
  - *Note:* the **displayed recommended settle city plots** don’t always match what the AI would settle if it were playing your civ in autoplay; the UI hint path may be (not sure) using a different logic from the reworked/optimized `AIFoundValue::evaluate`.
  - Treat them as general information, not guarantees.

## General changes

- **Start map behavior**: less "terrain polishing"; more starting **vision** so you can choose a spot rather than forcing ideal tiles.
- **Slavery**: `HURRY_POPULATION` production increased (**24 → 27**). See [CIV4HurryInfo.xml](/Assets/XML/GameInfo/CIV4HurryInfo.xml) (`HURRY_POPULATION`).
- **Leader random selection**: `PER_EXTRA_LEADER_CIV_SELECTION_WEIGHT` **20 → 0** (all civs equal chance regardless of leader count). See [GlobalDefines_advc.xml](/Assets/XML/GlobalDefines_advc.xml).
- **We Love the King Day**: restored (present in AdvCiv-SAS).
- **Cost rounding to 5 removed (human-side)**: Eliminated the base AdvCiv "round to 5" rule from `CvPlayer::getProductionNeeded` (units, buildings, projects) that applied only to human players, so **costs are no longer altered for the human player** — e.g., a Swordsman at 42 hammers costs 42 (not 40), at 43 costs 43 (not 45), and an Ancient Maceman at 18 costs 18 (not 20).
  - Also removed analogous rounding in `CvPlayer::greatPeopleThreshold`, `CvPlayer::getNewCityProductionValue`, and `CvTeam::getResearchCost`.
  - This fixes the Sevopedia vs. in-game mismatch, and the issue of human players having a different cost than AI players even if cost would have otherwise been the same (e.g., at Noble, the AI paid 42 while the human paid 40; 43 vs. 45).
  - Fine-grained costs now also enable true **per-hammer** tuning—e.g., you can set 40, 41, 42, 43, 44, or 45 instead of being forced to choose 40 or 45.
  - See [KI#67](/_1_AdvCiv-SAS/Docs/README_Known_Issues.md#ki-67).
- **Hints**: remove multiple copies of the same hint. Change in [CIV4Hints.xml](/Assets/XML/GameInfo/CIV4Hints.xml).
- **Mod scope:** keep overall gameplay relatively lightweight (no clutter), but go deeper on the systems that are changed.

## Game Speeds

- `GAMESPEED_MARATHON` **shortened** total turns **1250 → 1000** (vs base AdvCiv). Adjusted the associated speed modifiers to align with the new length.
- **New Game Speeds**: `Slow (1500 turns / 300%)` and `Very Slow (2000 turns / 400%)`, **slower than Marathon (1000 turns / 200%)**; and `Turbo (250 turns / 50%)` and `Nitro (165 turns / 33%)`, **faster than Quick (330 turns / 67%)**, that should provide varied experiences.
  - See full game speed details in the **new Sevopedia Speeds category**.
- **Clarified** and made more **outwardly visible** the **Game Speed** information: e.g. `"Normal (500 / 100%)"` bullet; `Epic (750 / 150%)` bullet; `GAME SPEED (50 000 BC - 2105 AD)` header, and clarified other fields.
- **Reordered** them in XML from fastest to slowest so they appear in this order in the UI (Main Menu)'s **Simple Game** Game Speed section.
- **Fix base AdvCiv `iExtraFreeOutsideUnits` incorrect default bug:**
  - Faster-than-Marathon speeds ended up with **100 extra free outside units** due to two issues: the `iExtraFreeOutsideUnits` XML tag was missing for those speeds, and the DLL fallback default for that tag was incorrectly set to **100** (Marathon explicitly had **1**).
  - Fixed by changing the DLL default from 100 to **0**, and by adding the missing `iExtraFreeOutsideUnits` game-speed XML field with a value of **0** so nothing relies on defaults.
  - Also added `iUnitCostPercent` in XML for exhaustiveness (value **100**) without changing its DLL default (**100**) since it was correct.
  - Spotted thanks to the new Sevopedia Game Speed Chart.
  - See: [KI#92](/_1_AdvCiv-SAS/Docs/README_Known_Issues.md#ki-92).
- **Timeline & pace**: fast-paced but long enough to play through.
  - Start at **-50 000 BC**; on **Normal**, early turns advance by **5 000 years/turn**, then the step size **gradually decreases**.
  - After a few dozen turns at **Normal** speed, you’ll be around the **Bronze Age**.
  - Late-game turns are kept relatively brisk; the game ends at **2105 AD** (**turn 500** on Normal).
  - See full game speed details in the **new Sevopedia Speeds category**.

## World Size/Scaling

- **New World Sizes**: `SAS24`, `SAS32`, `SAS40`, `SAS48`, **bigger than Huge** (Recommended for about 24, 32, 40, 48 players).
  - They are [modified versions of the XXL World mod](/README.md#new-advciv-sas-world-sizes-sas24-sas32-sas40-sas48-arena)'s largest maps.
  - (Requires AdvCiv-SAS 6590+) The shipped `Assets/CvGameCoreDLL.dll` supports up to 48 civilization players; the former prebuilt 18-civ alternative is no longer shipped, while modders can still compile a different `MAX_CIV_PLAYERS` limit from source.
  - Also a new `Arena` World Size **smaller than Tiny**.
  - See full World Sizes details in the **new Sevopedia W. Sizes category**.
- **Research — sea level**: `iResearchPercent = 100` for High / Medium / Low (**was** `117 / 100 / 87`). Sea level no longer alters research pace.
- **Research — world size**: `iResearchPercent` rescaled from `95 / 98 / 102 / 115 / 137 / 150` to `85 / 90 / 95 / 100 / 105 / 110` (so **Standard = 100**).
  - Rationale: on larger maps the AI doesn’t start with proportionally more early cities, so heavier early research multipliers left it behind.
  - Technology costs are tuned directly in XML; see Sevopedia/XML for current values.

## Map Generation (Common)

- **Optional XML-tunable BTS/AdvCiv standard-map generation:**
  - AdvCiv-SAS's default map generation uses the shared [`CvMapGeneratorUtil.py`](/Assets/Python/CvMapGeneratorUtil.py) helpers and [`CvMapGenerator.cpp`](/CvGameCoreDLL/CvMapGenerator.cpp) DLL behavior inherited from base AdvCiv.
  - New independent SAS defines let players select BTS alternatives for individual shared generation steps, including standard terrain and feature generation, fractal defaults, latitude, coastal Peaks, rivers, lakes, and bonus placement/distribution, without recompiling the DLL.
  - The defaults continue to use AdvCiv's behavior because that remains the intended AdvCiv-SAS setting.
  - These controls do not override separate Python mapscripts, or undo the changes that AdvCiv-SAS has made to Terrain/Feature/Bonus XML Info and other map-related GlobalDefines.
  - A fresh map also generated successfully with the complete BTS-style shared-generator setting combination; this smoke-tests the alternate branches without claiming exact BTS map parity or balance because SAS content and mapscript behavior remain independent.
  - See the documented map-generation block in [`GlobalDefines_advciv_sas.xml`](/Assets/XML/GlobalDefines_advciv_sas.xml).
- **Fixed AdvCiv's OneArea-resource fallback:**
  - AdvCiv's capped first distribution pass could leave fewer food and luxury resources such as Banana, Wheat, Dye and Spices than their calculated targets because its uncapped second pass could not revisit landmasses already tried.
  - The second pass now reconsiders those landmasses when needed.
  - This does not change XML frequencies or calculated targets: mapscripts where the first pass already succeeds are directly unchanged, while mapscripts that previously undershot can gain the missing copies up to their existing targets; exact locations and later resource placement can consequently differ.
  - See [KI#202](/_1_AdvCiv-SAS/Docs/README_Known_Issues.md#ki-202).

## Mapscripts

- Modify **Highlands** so it supports AdvCiv-SAS's new world sizes.
  - Also remove its hardcoded 18-player limit and use the active DLL's civilization capacity (currently 48).
  - Change in the new [Highlands.py](/PrivateMaps/Highlands.py).
  - Grid size **computed dynamically beyond Huge**.
  - See [SAS_WorldSizeUtils.py](/Assets/Python/SAS_WorldSizeUtils.py).
  - See also: [Modding ressources (Mapscripts)](/_1_AdvCiv-SAS/Docs/Modding_Ressources/README.md#mapscripts).
- Similarly also fix **other maps** to make them **compatible with AdvCiv-SAS**: [aDebugMap.py](/PrivateMaps/aDebugMap.py), [Fantasy_Realm.py](/PrivateMaps/Fantasy_Realm.py), [Great_Plains.py](/PrivateMaps/Great_Plains.py), [Ice_Age.py](/PrivateMaps/Ice_Age.py), [Inland_Sea.py](/PrivateMaps/Inland_Sea.py), [Lakes.py](/PrivateMaps/Lakes.py), [Maze.py](/PrivateMaps/Maze.py), [Hub.py](/PrivateMaps/Hub.py), [Mirror.py](/PrivateMaps/Mirror.py), [Oasis.py](/PrivateMaps/Oasis.py), [Ring.py](/PrivateMaps/Ring.py), [Team_Battleground.py](/PrivateMaps/Team_Battleground.py), [Tilted_Axis.py](/PrivateMaps/Tilted_Axis.py), [Wheel.py](/PrivateMaps/Wheel.py), [Arboria.py](/PrivateMaps/Arboria.py), [Donut.py](/PrivateMaps/Donut.py), [Earth2.py](/PrivateMaps/Earth2.py) (see below), [Global_Highlands.py](/PrivateMaps/Global_Highlands.py), [PerfectMongoose.py](/PrivateMaps/PerfectMongoose.py), [Rainforest.py](/PrivateMaps/Rainforest.py), [RandomScriptMap.py](/PrivateMaps/RandomScriptMap.py), [Terra.py](/PrivateMaps/Terra.py).
- Added shared Mapscript-related world-size helpers.
  - Note: done in [SAS_WorldSizeUtils.py](/Assets/Python/SAS_WorldSizeUtils.py) rather than in the [PrivateMaps](/PrivateMaps/) folder as a .py file.
  - This is because such a previous design caused the helper-as-a-playable-mapscript related bug, in AdvCiv-SAS only, of RandomScriptMap and Archipelago, e.g. at Huge world size, being generated far too small (`60 x 44`).
  - After the fix it returned to `120 x 84`, matching base AdvCiv visually at a glance, and not being overly cramped anymore.
  - The same issue may also have affected other mapscripts that imported the old helper.
  - The old helper now only keeps the minimal new playable mapscript code, not imported by other mapscripts anymore, in the new **SAS_Simple_Flat_Grass**: a simple sandbox mapscript with almost all land and mostly flat grass tiles.
  - See [SAS_Simple_Flat_Grass.py](/PrivateMaps/SAS_Simple_Flat_Grass.py).
- Tuned several inherited mapscripts: [RandomScriptMap.py](/PrivateMaps/RandomScriptMap.py) is trimmed slightly; [Medium_and_Small.py](/PrivateMaps/Medium_and_Small.py) is enlarged a little from Large onward because lower islets usually are not assigned to players, leaving the upper continent-style area cramped; [Big_and_Small.py](/PrivateMaps/Big_and_Small.py), [Custom_Continents.py](/PrivateMaps/Custom_Continents.py), and [Donut.py](/PrivateMaps/Donut.py) are trimmed on larger tiers because Huge tested too large.
- Fixed Random Script Map's incorrect Players' Start label: `Coastal or Largest Continent` now reads `Coastal on Largest Continent`, matching its intended combined validator.
  - This remains best-effort: when no qualifying plot has positive found value, generic placement can still produce a noncoastal start.
  - See [KI#275](/_1_AdvCiv-SAS/Docs/README_Known_Issues.md#ki-275) and [KI#275.2](/_1_AdvCiv-SAS/Docs/README_Known_Issues.md#ki-275.2).
- **Reduced the grid size** of some mapscripts that are **almost entirely composed of land** plots: city spacing **was too wide** due to the **excess of land** and **lack of water** plots. See [SAS_WorldSizeUtils.py](/Assets/Python/SAS_WorldSizeUtils.py).
- **Earth2 start distribution rework:**
  - for Huge and beyond world sizes, keep Earth2 at its existing **Huge-scale sizing** (already large enough), but solve crowding by **populating underused landmasses instead** (e.g., min 16 players before Africa is populated, min 32 players before the whole world is populated) of enlarging the script further.
  - **Thresholds are based on player count** (not world size).
  - See [`GlobalDefines_advciv_sas.xml`](/Assets/XML/GlobalDefines_advciv_sas.xml).
- **Reduce grid size** for each worldsize in [Global_Highlands.py](/PrivateMaps/Global_Highlands.py) by one world size (e.g. World Size Huge was `(38,24)` -> `(32,20)` (old World Size Large's dimensions)): empirically even accounting for the many peaks it has (compensated by a larger grid at each world size than usual), **cities were still too sparse**.
  - See [KI#138](/_1_AdvCiv-SAS/Docs/README_Known_Issues.md#ki-138).
- Add **Deer percentage mapscript option** (e.g., `0/10/25/50/75/100`) for [Arboria.py](/PrivateMaps/Arboria.py) and [Boreal.py](/PrivateMaps/Boreal.py): **high-food terrain** (e.g. grass) mapscripts like Arboria, or **lower-food terrain** ones (e.g. tundra/plains) like Boreal, had **too much deer**.
  - See also [KI#105](/_1_AdvCiv-SAS/Docs/README_Known_Issues.md#ki-105).
- Performance optimizations in [Arboria.py](/PrivateMaps/Arboria.py).
- Rebalanced Rainforest grid sizes: keep enough room for viable Arena starts while reducing the BTS/AdvCiv Duel-through-Huge sizes and scaling SAS24-SAS48 to avoid huge empty areas per player.
  - See [KI#278.2](/_1_AdvCiv-SAS/Docs/README_Known_Issues.md#ki-278.2) and [KI#278.3](/_1_AdvCiv-SAS/Docs/README_Known_Issues.md#ki-278.3).
- **Order mapscripts alphabetically** in **Simple Game** or **Custom Game** (by adding missing mapscripts in **PrivateMaps**).
- **Remove from Simple Game** some mapscripts that are deemed not essential or unbalanced; they are remain playable via the **Custom Game** menu: [Big_and_Small.py](/PrivateMaps/Big_and_Small.py), [Great_Plains.py](/PrivateMaps/Great_Plains.py), [Earth2.py](/PrivateMaps/Earth2.py), [Hemispheres.py](/PrivateMaps/Hemispheres.py), [Lakes.py](/PrivateMaps/Lakes.py) (has too few lakes), [Mixed_Continents.py](/PrivateMaps/Mixed_Continents.py), [Oasis.py](/PrivateMaps/Oasis.py), [Shuffle.py](/PrivateMaps/Shuffle.py), [Tectonics.py](/PrivateMaps/Tectonics.py).
- **Add** or **redo** the .dds (i.e. image) in **Simple Game** for mapscripts so it is more illustrative or accurate than a dark or incorrect globe map.
  - See [Readme.md (New .dds for mapscripts in Simple Game)](/README.md#new-dds-for-mapscripts-in-simple-game) and [CIV4WorldPickerInfos.xml](/Assets/XML/Interface/CIV4WorldPickerInfos.xml).
- **Add** new AdvCiv-SAS original maps: [SAS_Longworld](/PrivateMaps/SAS_Longworld.py): A lightweight long-format world for AdvCiv-SAS: one connected horizontal landmass with wavy borders, inspired by Ringworld3_102_mst.py but heavily simplified, similarly also [SAS_Spiky_Avenues.py](/PrivateMaps/SAS_Spiky_Avenues.py) (Compact tactical mapscript with repeated streets: each street has facing houses (spikes), a central avenue, and bridges on left/right edges to connect streets), [SAS_Large_Facing_Islands.py](/PrivateMaps/SAS_Large_Facing_Islands.py) (Each player starts on a large island, with rivals on the east and west sides, and a rival facing north or south (when world size is large enough)), and [SAS_Parallel_Lines.py](/PrivateMaps/SAS_Parallel_Lines.py) (Parallel lines mapscript where the world is optionally (default) connected horizontally (WrapX) and vertically (WrapY).
  - Fixed number of players per line (3 or 4), and height per player).
  - See [Readme.md (AdvCiv-SAS original mapscripts)](/README.md#advciv-sas-original-mapscripts-eg-sas_simple_flat_grass-sas_large_facing_islands-sas_longworld-sas_parallel_lines-sas_spiky_avenues).
- **Add** and **adapt** [BTG_Cross.py](/PrivateMaps/BTG_Cross.py) mapscript (from the Beyond the Game mod (v2.43)): remove non-AdvCiv BTG options/logic, **fix script compatibility issues**, support **max player count** and up to **world size SAS48**, **change some options** (e.g., allow Desert as Default), and expose the mapscript in both **Simple Game** and **Custom Game** lists.
  - **Increase BTG_Cross grid dimensions** with explicit per-world-size rows (instead of all derived from Huge only): **Huge** and several smaller base sizes were **too cramped**, and direct rows make the tuning adjustable.
  - Similarly add **other BTG maps**: [BTG_Grid.py](/PrivateMaps/BTG_Grid.py), [BTG_Lagoon.py](/PrivateMaps/BTG_Lagoon.py).
  - See also [KI#137](/_1_AdvCiv-SAS/Docs/README_Known_Issues.md#ki-137).
- **Add and adapt** [Planet_Generator_0_68.py](/PrivateMaps/Planet_Generator_0_68.py) (from [CFC](https://forums.civfanatics.com/resources/planet-generator-0-68.1817/)): keeps its core behavior (unpredictable continent shapes with balanced continent sizes) while applying AdvCiv-SAS mapscript compatibility changes, notably **world-size percentage scaling**, **removed unneeded options** (map-size override and redundant bonuses choice), and **bug fixes** (including robust direction handling).
  - Expose the mapscript in both **Simple Game** and **Custom Game** lists.
  - Similarly add [Equal_Islands_V2_beta.py](/PrivateMaps/Equal_Islands_V2_beta.py) (from [CFC](https://forums.civfanatics.com/resources/equal-islands-v2-beta.19405/)), [Water.py](/PrivateMaps/Water.py) (from [CFC](https://forums.civfanatics.com/resources/water.15334/)) (actually has many lakes unlike [Lakes.py](/PrivateMaps/Lakes.py); also **adjusted city spacing** that **was too wide**).
- Make other mapscripts **also appear in Simple Game**: [Boreal.py](/PrivateMaps/Boreal.py), [Donut.py](/PrivateMaps/Donut.py), [Ring.py](/PrivateMaps/Ring.py), [Wheel.py](/PrivateMaps/Wheel.py)

## Handicaps

Note: The shipped stable example [`handicap_infos_compared.md`](/LLM_Helpers/examples/handicap_infos_compared.md) compares base AdvCiv 1.12 vs AdvCiv-SAS. See also [LLM_Helpers (README.md)](/LLM_Helpers/README.md#compare_handicap_infospy).

- **Expanded handicap difficulty ladder:**
  - added **Rookie** below Settler and **Deity+** above Deity.
  - Rookie gives new/casual players a softer entry point (AdvCiv-SAS Settler is harder and more gradual to other higher handicaps and less of a tutorial sandbox than base AdvCiv), while Deity+ gives expert players an extra challenge above Deity (AdvCiv-SAS Deity has less cheats and more gradual to previous handicaps).
- **Cleaner handicap difficulty rating scale:**
  - reworked `iDifficulty` from the old Settler-to-Deity `10, 20, 30, 40, 50, 60, 70, 80, 90` curve to a new Rookie-to-Deity+ `18, 26, 34, 42, 50, 58, 66, 74, 82, 90, 98` curve.
  - This keeps Noble at `50` as the even/baseline difficulty, and places the new lower/higher difficulties around that scale without using `0` or values above `100` (`iDifficulty` is not the main source of direct AI bonuses or player penalties; those still come from fields like `iAIResearchPercent`, `iAITrainPercent`, maintenance, happiness, barbarian modifiers, and so on.
  - Instead, `iDifficulty` is a general difficulty rating used by scoring, comparisons, and code that needs to average or estimate handicap strength).
- **More robust handicap averaging:**
  - updated DLL code that converts an averaged difficulty score back into a handicap.
  - Previously, some code effectively averaged raw handicap enum/order IDs, which **only works reliably when difficulty rows are fixed and evenly spaced**.
  - It now averages XML `iDifficulty` values and maps back to the nearest handicap, so added or future non-linear handicap levels behave according to the XML difficulty rating rather than accidental enum position.
- **No free techs** for AI or human.
- **Human costs fixed at 100 on all handicaps** for tech/unit (`iResearchPercent`, `iTrainPercent`, `iGPThresholdPercent` (great person) stay constant for the human). AI equivalents scale with handicap.
- `iBaseGrowthThresholdPercent` (base food required to grow a city): **Fixed at 100 on all handicaps.**
  - Rationale: In base AdvCiv this value varies by difficulty to add a **human growth/expansion penalty** (and it can also shift early expansion pacing via settler cost logic tied to the initial growth threshold).
  - Consequence: Removing this scaling makes **high handicaps** (e.g. Deity) **easier for the human player** (faster early growth/expansion than base AdvCiv), and makes **low handicaps** (e.g. Chieftain) **harder for the human player** (less of an early growth/expansion boost).
- **Unit maintenance** (`iUnitCostPercent`, `iAIUnitCostPercent`) aligned across handicaps for fairness; difficulty tuned through other params.
- **Unit supply parity:** Raised `iAIUnitSupplyPercent` from 50 to 100, so AIs no longer get half-price unit supply outside their borders; human and AI players now pay the same supply costs (unlike base AdvCiv).
- **Unit upgrade costs — aligned and reduced.**
  - Increased `iAIUnitUpgradePercent` from **50 → 100**, so AIs now use the **same unit upgrade multiplier** as humans (no more half-cost AI upgrades).
  - In parallel, `CvUnit::upgradePrice` now applies a global **modifier** (current value: **75%**, tunable), because the old human rate felt too expensive in play.
  - Net effect: in base AdvCiv the AI paid **50%** and the human **100%** of the base upgrade cost; in AdvCiv-SAS, **both now pay 75%** of that cost.
- **No delayed starts**: `INCREASE_START_TURN` disabled; all handicaps start on **turn 0**.
- **AI worker speed**: `iAIWorkRateModifier` **0** on all handicaps.
- **AI starting workers**: `iAIStartingWorkerUnits` **0** on all handicaps.
- **Goodies (huts)**: remove **worker/settler** pops; rebalance barbarian outcomes (fewer strong, more weak at high handicaps); increase **experience** rewards at higher handicaps.
- **Animals per handicap**: less animals in highest handicaps (there seemed to be too many) but more and gradually so in lowest handicaps.
- **Explicit and tighter barbarian handicap fields:**
  - added **explicit values** for previously missing barbarian-related handicap fields (`iSeaBarbarianBonus`, `iBarbarianCityAttackBonus`, `iSeaBarbarianExtraMoves`) and tightened their low-difficulty curves.
  - This avoids hidden XML/default behavior and reduces broad low-difficulty freebies across most of the ladder; for example, barbarian city attack modifiers no longer implicitly stay at `-25` through Noble, sea barbarian combat modifiers no longer implicitly stay at `-10`, and sea barbarian extra movement penalties no longer implicitly apply through Deity.
  - The new curves concentrate strong protection on Rookie/Settler/Chieftain, then reach neutral values earlier.
- **Lower and more gradual barbarian/animal handicap protection:**
  - retuned several **barbarian** and **animal** handicap modifiers so only the lowest difficulties protect players as of now.
  - For example, `iAnimalBonus`, `iAIAnimalBonus`, `iBarbarianBonus`, and `iAIBarbarianBonus` now follow a clean `-30`/`-20`/`-10`/`0` curve for Rookie/Settler/Chieftain/Warlord and then stay neutral at higher handicaps.
  - In base AdvCiv, several of these protections lasted much farther up the ladder: `iAnimalBonus` still gave `-25` at Noble and remained non-neutral through Immortal, while `iAIAnimalBonus` and `iAIBarbarianBonus` stayed at `-25` all the way from Settler through Deity.
  - This keeps the lowest difficulties forgiving, gradually less so, while removing broad barbarian/animal freebies from most difficulties.
- **More difficulty-sensitive barbarian city creation:** reworked `iBarbarianCityCreationProb` from base AdvCiv’s very flat curve into a wider Rookie-to-Deity+ scaling curve. Base AdvCiv only rises from `4` on Settler to `8` on Deity, while AdvCiv-SAS now scales from `0` on Rookie to `40` on Deity+.
- **Handicap normalization (starting units)**: Reduced/standardized several handicap fields so they’re **the same at all handicaps**: `iStartingDefenseUnits`, `iStartingWorkerUnits`, `iStartingExploreUnits`, `iAIStartingDefenseUnits`, `iAIStartingWorkerUnits`, `iAIStartingExploreUnits`.
  - This reflects recent AI improvements (workers, building choices, scrapping rules, unit mix) and aims to keep games fair while the AI remains competitive.
- **Difficulty scaling — `iAIHandicapIncrementTurns`**: **disabled (set to 0 on all handicaps)**.
  - This removes the **per-turn** AI discount that made games progressively harder; with AdvCiv-SAS’s improved hammer efficiency, that scaling led to runaway AIs at high levels.
  - Now AI cost/research **modifiers** come **only** from the static **handicap XML fields** (e.g., `iAITrainPercent`, `iAIBuildingCostPercent`, `iAIResearchPercent`, …), which have been **retuned to be linear by handicap (not per-turn)**—hard but fair.
  - Practical effects: fewer excess units and lower bankruptcy risk, healthier mid→late-game economies, and challenge that relies on AI competency gains rather than exponential scaling.

## Terrains / Features

- Overall: **snow**, **desert**, and **water** tiles strengthened mainly via bonus/building changes (see Bonus/Buildings sections).
- **Floodplains persist after raze**: destroying a city does not remove Flood Plains, so previously-developed land can still stay profitable.
- **Ground explore/recon units**: can move through **all land terrains** from game start (including **peaks**) via `bCanMoveImpassable` (no water tiles). They also **ignore terrain movement costs** (not just the Renaissance Explorer). See **Sevopedia Terrain: Peak** or XML for details.
- **Other units** (example: Quechua Warrior): some specific units also have `bCanMoveImpassable` for **all land terrains**; see **Sevopedia Terrain Peak/Hill** entries or XML/docs for per-unit details.
- **Air explore/recon and land-air units**: the **Airship** (see its Sevopedia entry or this guide's military section) and **Gunship** can move through **all terrains, including peaks and water tiles** using `bCanMoveImpassable` and `bCanMoveAllTerrain`.
- **Movement restrictions (outside cultural borders)**: some units (e.g. chariots, some siege) **cannot enter forest/jungle** feature tiles outside borders.
  - This restriction still applies even if the tile is roaded (base Civ4/AdvCiv logic reused).
  - See Sevopedia Feature entries and the feature-category rework ([example-3](/_1_AdvCiv-SAS/Docs/README_Sevopedia_Reworks.md#example-3-features-category)) or unit pages for the exact list.
- **Forest/jungle combat penalties**: some units (notably UNITCOMBAT_MOUNTED_MELEE) now get forest/jungle penalties; UNITCOMBAT_MOUNTED_RANGED penalties are milder/limited (defense-only).
  - See Sevopedia Feature entries ([example-3](/_1_AdvCiv-SAS/Docs/README_Sevopedia_Reworks.md#example-3-features-category)) and unit pages for the exact modifiers.
- **Optional unit terrain/feature combat modifiers**: Added new restrained terrain, feature, and Hill combat effects where they make unit roles more intuitive or varied; for example, wheeled chariots and some siege units are much worse in dense Forest/Jungle, while some units are better suited to Desert, Plains, Hills, or other environments.
  - The goal is useful tactical variety without making terrain too swingy or letting a favorable map decide combat by itself.
  - Terrain/Hill modifiers generally use **10% or 20%** magnitudes and are capped at **20%**; broader affinities can instead combine a few smaller values.
  - Narrower feature modifiers can reach **40%**, because features normally do not remain on city plots and therefore generally do not affect city combat.
  - Generic units only receive effects where particularly relevant.
  - Combat-capable civ-specific units generally each receive a focused terrain/feature identity: normally one strong terrain or at most two weak terrains, while relevant generic-unit affinities can carry through or be strengthened and narrower feature specializations are balanced separately.
  - Terrain/feature attack and defense effects remain independently optional through SAS defines.
  - See unit pages, Sevopedia Terrain/Feature pages ([example-9](/_1_AdvCiv-SAS/Docs/README_Sevopedia_Reworks.md#example-9-terrains-category), [example-3](/_1_AdvCiv-SAS/Docs/README_Sevopedia_Reworks.md#example-3-features-category)), and [Assets Rebalancing: Unit terrain and feature combat modifiers](/_1_AdvCiv-SAS/Docs/README_Assets_Rebalancing.md#unit-terrain-and-feature-combat-modifiers) for the current values and detailed rationale.
- **Build: Remove Jungle** now yields **+20 hammers** on removal (see `BUILD_REMOVE_JUNGLE` in [CIV4BuildInfos.xml](/Assets/XML/Units/CIV4BuildInfos.xml)).
- **Feature: Jungle rework**: remove the food penalty (no food malus), increase unhealth to `iHealthPercent` **-50**, set feature defense `iDefense` **-25** (still +25% rival defense in features, so net 0 in jungle if you consider that baseline).
- **Feature: Fallout** `iHealthPercent` **-50 -> -75**.
- **Feature: Flood Plains** `iHealthPercent` **-40 -> -50**.
- **Terrain: Tundra** base yield adds **+1 commerce**.

## Bonuses

- (Requires AdvCiv-SAS 6581+) **Resource trade unlocks follow resource availability:**
  - aligned stale `TechCityTrade` requirements with the current XML reveal and resource-improvement Build prerequisites.
  - For example, Plantation resources such as Banana and Dye become usable/tradable at Agriculture instead of separately requiring Calendar; Marble and Stone follow their Mining unlock, and Uranium follows its Radio reveal instead of waiting for Fission.
  - This brings forward connected-resource benefits and access for assets whose own technologies are already available; units and buildings still require their own technologies.
  - A general CI prerequisite-graph check now covers every resource, including separate same-column technologies, without named resource exceptions.
- **Grapes**: now improved by **Plantation** (not Winery). Happiness moved to **Grocer** (Grapes and Sugar both give happiness via Grocer). See Sevopedia/XML.
- **Hit bonuses** functionally disabled (kept late in tree, obsolete on reveal) to avoid LSystem issues; dependent assets rebalanced or left as‑is where appropriate.
- **Animal livestock Bonuses** (as of now `BONUS_CATTLE`, `BONUS_SHEEP`, `BONUS_PIG`): increase the number of assets that are strengthened or rebalanced with these Bonuses, which are otherwise weak for example as compared to food stapple bonuses like Maize Rice or Wheat that scale for example with ubiquitous Granaries, or for example to Mount Animal Bonuses that have a strong military purpose.
  - As of now they now notably are in early assets like **Temple of Artemis** (happiness from each bonus), Summeria's **Ziggurat** (health from Sheep), Rome's **Forum** (happiness from Pig), and Mongolia's **Ger** (happiness from Cattle (Pastoral Economy Theme)).
  - This also increases the strategic importance of these bonuses.
- **Camel** replaces Hit Movies; spawns on **desert**. High food profile; supports camel‑mounted units (see Military section).
- **Bonuses — distribution (repartition) & yields (with stronger competition):**
  - Adjusted **distribution/spawn chances** and **yields** across many bonuses:
  - **Yields:** formerly weak bonuses (e.g., **Silver**, **Deer**) are **stronger**; a few outliers (e.g., **Gold**) are **slightly weaker**; overall, bonuses grant **less commerce** to keep pacing gradual.
  - **Repartition:** **land food bonuses spawn less often** to create **real competition** for high-value spots; after testing, rates were raised enough that maps still feel lively.
    - Several bonuses were **reworked** for clearer roles (e.g., **Sugar** gives **base hammer**; **Horse** gives **base food** on the tile but **hammer** once improved).
    - **Deer** now **spawns more** (primary tundra/snow food) with **strong yields**, making **tundra cities** near Deer more viable.
    - See **Sevopedia → Bonus** for current values and **[commit b841968](https://github.com/wonderingabout/AdvCiv-SAS/commit/b841968682f022e2cefbe1722efb3520492659b8)**; screenshots: **[5238](https://drive.google.com/file/d/1178did3ROEoIV_wbonpH91jZcdaNtzRg/view?usp=sharing)**, **[5241](https://drive.google.com/file/d/15acCk947FpTMmDJdzg6sMXCXOjYwiUB0/view?usp=sharing)**.
- **Water bonuses — increased but conservative spawns:** **Molluscs** and **Fish** now spawn **more frequently** (they’re "general" water bonuses). Rates were tuned with tests to avoid over-spawning—**significant yet conservative** increases. See **Sevopedia → Bonus** for current values.
- **Whales — buff & placement:** **Whales** have been **buffed** and can now **spawn on coast** as well as ocean. Details in **Sevopedia → Bonus → Whales**.
- **Horses — no tundra placement:** **Horses no longer spawn on tundra.** Food needs there are primarily handled by **Deer**. Concentrating Horses on non-tundra improves typical access patterns on most starts.
- **Edge-weighted bonus placement:** Shifted bonus distribution to **favor map edges** more (it was too **center-weighted**), reducing the advantage of central starts and making **edge starts more competitive**.
- **Desert rework — Camel replaces Movies:**
  - On **desert**, **BONUS_CAMEL** now **replaces BONUS_MOVIES**, providing a **desert-friendly food bonus** and serving as a **mounted unit bonus** that **competes with Horses** (see **[Military: camel changes](/README.md#military-tree-and-changes)**).
  - Camel XML derives from Horse with AdvCiv-SAS adjustments; desert tiles become more viable via **high food yield**.
- **Horse frequency reduced:** With **Camels** added as an alternative mount, **Horse** **spawn frequency is reduced** to keep mounted-bonus balance.
- **High-food animals slightly rarer:** Other **high-food land animals** are **slightly reduced** in frequency so **Camels** can be present without making food abundances trivial—keeping **competition for top sites** intact.
- Adjust some **animal Food Bonuses**' terrains/features for balance, accuracy, or thematic fit: **Sheep**: Grassland + Plains -> **Plains only** (Plains are weaker, Grassland food is overpowered and over-represented, real world population is mostly on savanna-like biomes and culturally closer to African, Western Asian, and Australian regions); **Pig** Jungle -> **Forest** (real world population is mostly around typical forest biomes, this helps counter-balance the Sheep removal from Grassland, and Jungle is stronger in AdvCiv-SAS (no inherent food-penalty) so it can accommodate this change).
- **Gemstones** can spawn on **flood plains**. See Sevopedia/XML.

## Improvements

- **Tech prereqs** adjusted (e.g., **Quarry** earlier; **Railroads earlier:** available **sooner** at **Steam Power** (the Railroad tech is considered inefficient as a tech and is now removed) so the late game feels less "too far away").
- **Farms without irrigation** (`bIgnoreIrrigation`) unlock much sooner (currently **Water Wheel**; see tech tree/Sevopedia for current placement).
- **Workshops on tundra** enabled (`<TerrainMakesValids>`).
- **Workshop balance/timing:** **somewhat nerfed** via the reworked tech **pacing** (some extra hammer yield **unlocks are later**), but some improvements/builds **also gain new extra yields in the Future era** (see Sevopedia/XML).

## Builds

- **Plantations** are available earlier (as of now at **Agriculture**).
- **Remove Jungle** and jungle-clearing improvement builds are moved away from overpowered military path, only the weaker jungle remains, and sooner (as of now at **Metal Casting**) to buff jungle too.
- **Chop Down a Forest** is similarly moved out of the overpowered military path, now to the builder **Masonry** path.
- **Remove Jungle** and **Chop Down a Forest** builds now use distinct buttons.
  - These are visible both in the **Technology Advisor** and in **Sevopedia Techs' Enables** and in the new **Sevopedia Builds** category panel.
  - See also [Sevopedia Reworks (Builds)](/_1_AdvCiv-SAS/Docs/README_Sevopedia_Reworks.md#example-040-builds-category-eg-remove-jungle-build-road-create-a-farm).

## Specialists

- **Citizen**: now adds **+1 commerce** (total **+1 hammer, +1 commerce**). See Sevopedia/XML.
- **Priest**: now adds **+1 culture** (total **+1 hammer, +2 commerce, +1 culture**).
- **Engineer**: now adds **+1 hammer** (total **+3 hammers**).
- Extend the "force one **Artist until BFC**" helper to **human** players for convenience; toggle via `CONVENIENCE_HUMAN` defines in [`GlobalDefines_advciv_sas.xml`](/Assets/XML/GlobalDefines_advciv_sas.xml) (default/recommended: enabled).
- Add an option (default/recommended: **enabled**) to disable **auto-Citizen specialist** assignments for the **human** player for convenience, since they’re annoying to check in every city every turn and are almost always an inefficient choice unless absolutely no workable tiles or decent specialists are available.
  - Implemented in `CvCityAI::AI_jobChangeValue`; toggle via `CONVENIENCE_HUMAN` defines.
  - See [KI#44.6](/_1_AdvCiv-SAS/Docs/README_Known_Issues.md#ki-44.6).
- Add a similar option (default **disabled**) to restrict **other auto-specialist** assignments for the **human** player (e.g. allow auto-Scientists/Engineers/Merchants/Priests, but block auto-Spies and auto-Artists), for players who prefer tighter manual control.
  - Does not override explicit helper logic such as the "force one Artist until BFC" rule.
  - Implemented in `CvCityAI::AI_jobChangeValue`.
  - Tunable via `CONVENIENCE_HUMAN` defines.

## Traits

- Several Traits reworked.
  - Weaker ones are buffed/modified; some previously OP ones are adjusted.
  - Example: **Protective** is stronger; **Industrious** shifts away from "wonders faster" toward **worker/production-oriented** effects (elements formerly in Expansive).
  - See in-game Sevopedia → Traits for current effects (+x culture / +x hammer / +x commerce as listed there).
- **Removed Trait — Expansive:** In AdvCiv-SAS, **Protective** is broadened from "just city defense" to an empire-wide protection theme: it now also covers health and no anarchy (political stability), and with **Imperialist** already handling expansion (settler discount + now also trade-route–scaled yields), **Expansive** no longer had a distinct role, was overlapping with the former, and was weak/volatile to balance → removed.
- **Expansive Trait clean-up:** Removed its building modifiers (**Granary**, **Harbor**)—too generic/OP and no balanced replacements.
- **Trait icons (UI) & cleanup:** Updated the Sevopedia "char" (glyph/logo) for **Imperialist** and **Organized** Traits; commented out the obsolete **Expansive** Trait entries now that the Trait is removed. Implemented in [/Assets/Python/BUG/TraitUtil.py](/Assets/Python/BUG/TraitUtil.py).

## Civics

- **Civics rebalance (selective):**
  - `CIVIC_REPRESENTATION` adjusted (unlocks earlier in AdvCiv-SAS; effects tweaked to fit timing; **Upkeep: Medium → High** to counter-balance its per-specialist scaling and thematically reflect the heavier apparatus needed to sustain such systems); `CIVIC_SLAVERY` buffed (**Upkeep: Medium → Low**); `CIVIC_CASTE_SYSTEM` nerfed (**Upkeep: Medium → High**) to curb early snowball; `CIVIC_VASSALAGE` buffed (**Upkeep: High → Medium**); and other changes.
  - See Sevopedia/XML.
- **Civic reworks:**
  - `CIVIC_WAGE_LABOR` (former Emancipation) retuned into an earlier **production/economy** civic for greater impact and historical accuracy; `CIVIC_SERFDOM` strengthened into a more **agrarian/food-focused** civic.
  - Some religion civics are buffed/reworked to be refocused on religious play and to stay relevant longer; notably, `CIVIC_THEOCRACY` now uses `iCivicPercentAnger` = 200 (vs 400 on former civic_emancipation) (value info added to Sevopedia and Policy Advisor); economy civics are also more specialized/distinct, e.g. the old Environmentalism civic is replaced with `CIVIC_TRADE_BLOC` (earlier regional union/league-style trade civic); `CIVIC_PROTECTIONISM` is now about having **less trade**, not **no trade** (not isolationism); and `CIVIC_STATE_PROPERTY` now redistributes wealth via yield shifts (e.g. −20% hammers in the capital, +10% in all cities → effectively -20 + 10 = −10% in the capital, vs +10% in all other cities); plus other reworks such as for `CIVIC_NATIONHOOD`.
  - Net effect: more viable paths, fewer must-picks.
- **Civic unlocks moved in the tech tree:**
  - Some civics are available earlier/later than in base AdvCiv or in strategically more distinct or versatile paths: notably, **Slavery** moves from overpowered, strong military Tech Bronze Working to **Masonry**, keeping the effect early without placing it at the beginning; **Organized Religion** moves to **Polytheism** so monastery-free missionaries can affect early religious spread; and **Pacifism** moves later to **Medicine**, which is more distinct than the religious path (plus its Great Person scaling is too strong for the early religious/culture branch).
  - This improves pacing and balance/strategy variety while reducing the "everything funnels through Bronze Working" effect.
  - See Tech Tree/Sevopedia/XML for current placements.
- **Civics order aligned to tech pace:** Reordered civics in XML so the **Policy Advisor** displays them in rough unlock order (UI only; no rules change). Examples: **Universal Suffrage** before **Police State**; **Caste System** before **Vassalage**; **Pacifism** before **Theocracy**.

## Technologies

- **Large reorder** for historical flow (e.g., Pottery before Wheel; Metal Casting before Currency and Bronze Working; Medicine earlier).
  - Overall tech count stays similar to base (some low-value techs removed, with the **Future era expanded**).
  - **Quotes/announcer lines** were reassigned to better fit the new placements (including Future era) and for accuracy.
  - See [Assets rebalancing: Tech quotes](/_1_AdvCiv-SAS/Docs/README_Assets_Rebalancing.md#tech-quote-swaps-rework).
- **More distinct branches:**
  - **Polytheism** branches toward **Drama** and **Code of Laws -> Monarchy -> Monotheism**; Drama + Monotheism then converge at **Sacred Architecture -> Aesthetics**.
  - **Constitution + Paper -> Broader Education** separately branches into **Medicine -> Naturalism** and the **Currency + Broader Education -> Banking -> Democracy** institutional/economic line.
- Gameplay timings shift: **culture**, **slavery**, **plantations**, etc., often **earlier**.
- **Selective tech-trade bans for snowball techs:**
  - Some high-impact techs are now flagged **"Cannot be traded"** (`bTrade = 0`) to curb extreme tech-brokering/"tech whoreism" chains (e.g. turning one breakthrough into a long series of trades), which AIs are especially vulnerable to and for balance.
  - The goal is to tone down decisive, snowbally trades while still allowing normal tech trading (unless you enable the **No Tech Trading** game option, which disables it entirely).
  - Info added in UI: see [UI (In-game)](/_1_AdvCiv-SAS/Docs/README_Main_Changes_Guide.md#ui-in-game) and [UI (Sevopedia)](/_1_AdvCiv-SAS/Docs/README_Main_Changes_Guide.md#ui-sevopedia) sections.
- **TECH_COST_NOTRADE_MODIFIER** **-23 → -20** (when No Tech Trading option is on). See [GlobalDefines_advc.xml](/Assets/XML/GlobalDefines_advc.xml).
- **Exponential pacing**: fewer late-era techs/unit types (Future era is the exception and is expanded), partly reflecting that ancient eras cover far more "years" than modern ones.
- **FirstFreeUnitClass** assignments reshuffled to diversify GP races (one GP type per tech; often earlier). *Exception:* the Future era has its own GP assignments.
- **Delayed Requirements**: Some techs (e.g. `TECH_MOUNTED_COMBAT`) can optionally be researched fairly early (e.g. at Classical Era) or skipped until significantly later when they become mandatory (e.g. mid-Medieval Era for `TECH_CHIVALRY`).
  - This gives more strategic flexibility and research costs management, as these techs would be cheaper to research later on with a stronger economy, while shorter-term concerns have more room to be addressed sooner if desired.
- **Hastened Requirements**: Conversely, some techs (e.g. `TECH_CORPORATION`) are available earlier than their column in tech tree would suggest (since their requirements are few), for versatility and diverse strategical choices (thematically, it is also sensical as an alternate reality possibility).

## Eras

- **Future era** reworked into a full era with techs/units/buildings and its own music; begins with **tech_depopulation** as a balancing/historical nod. See Sevopedia/XML, and [Copyright and Disclaimer](/README.md#copyright-and-disclaimer).
- **Eras — research scaling**: `iResearchPercent = 100` (**was** `100 / 90 / 80 / 70 / 60 / 50 / 40` for all eras); `iTechCostModifier = 0`. Tech costs now come directly from TechInfos (no hidden per-era multipliers). Tech costs have been adjusted to match this change.
- **Eras — anarchy scaling**: `iAnarchyPercent = 100` for all eras (**was** `50 / 50 / 40 / 40 / 34 / 34 / 34`). In practice this yields about **2 turns** of anarchy when you change civics (or similar). Intent: add strategic weight without tedium; **indirectly buffs** Traits with **no anarchy** (e.g., Spiritual).
- **Eras — growth & production scaling flattened**: set `iGrowthPercent` (food-to-grow), `iTrainPercent` (unit cost), `iConstructPercent` (building cost), and `iCreatePercent` (project cost) to **100 for all eras** (**was** `100 / 100 / 100 / 90 / 80 / 70 / 60`).
  - Removes late-era discounts that sped up mid/late game; pacing is now **predictable and gradual**, with balance handled directly via per-item `iCost` instead.
- **Eras — worker builds & improvements scaling flattened**: set `iBuildPercent` and `iImprovementPercent` from **100/100/100/90/80/70/60 → 100** at **all eras**.
  - The original reason for the era-based scaling isn’t known; this change is intentional to keep tile-development pacing **predictable and consistent** across eras and to tune balance directly through base costs/worker rates rather than hidden era multipliers.
- **Eras — culture scaling flattened**: set `iCulturePercent` from **100/100/80/70/60/50/50 → 100** at **all eras**.
  - Removes late-era culture acceleration so culture growth stays **consistent** and driven by **commerce/buildings**, not hidden era multipliers.
  - In **AdvCiv-SAS**, if we ever want modern (or ancient) culture to weigh differently, we prefer adjusting the **explicit culture values on buildings/units** rather than reintroducing era-wide modifiers.
- Add **new XML field** `<Button>` for eras, to cleanly display them in Sevopedia and ingame without having to hardcode the path.
  - Change in [CIV4GameInfoSchema.xml](/Assets/XML/GameInfo/CIV4GameInfoSchema.xml), [CIV4EraInfos.xml](/Assets/XML/GameInfo/CIV4EraInfos.xml), [CvEraMovieScreen.py](/Assets/Python/Screens/CvEraMovieScreen.py).
  - See [README: Button for eras](/README.md#button-for-eras).

## Culture

- **Great Artist rebalance:** culture bomb strength reduced (Requires AdvCiv-SAS 6462+) (**700 → 550 per era**), but long-term culture output increased (**12 → 14 culture/turn**). This strengthens long-term culture play while toning down early spikes.

## Religions

- **Religions roster:**
  - added **Paganism**; removed **Confucianism** (chosen partly because it has fewer adherents in our framing).
  - Rationale: a lineup that feels simpler and (to us) more historically fitting.
  - We **keep Judaism** as the in-game representative of early **monotheism**, and we continue to prefer a **small set (~7) of religions** for clarity and efficiency.
- **Religion founding is distributed across more varied paths:** religions are available early enough to shape spread, diplomacy, and conflict rather than being concentrated in one later religious branch; notably, Judaism is founded at **Code of Laws**, Christianity at **Monotheism**, and Islam at **Navigation**.
- **Missionaries:** significantly **cheaper**.
- **Monasteries:** simplified — now **espionage** buildings with a **low Great Person rate**; **no longer obsolete**; **no longer give science** (keeps science buildings cleaner and the theming clearer)..
- **Shrines:**
  - now require **(1) the religion present in the city** and **(2) the religion’s tech** (previously shrine TechPrereq was `NONE`).
  - This makes shrines show up properly in the **Religion** Sevopedia building list and in the **tech tree**, and prevents "conquer (or any way to acquire) holy city ⇒ build shrine immediately" without the religion/tech.
  - See [KI#13](/_1_AdvCiv-SAS/Docs/README_Known_Issues.md#ki-13), and [README_Sevopedia_Reworks.md#example-5-religion-category](/_1_AdvCiv-SAS/Docs/README_Sevopedia_Reworks.md#example-5-religion-category).

## Corporations

- **Executives:** significantly **cheaper**.
- **Executives** now **require the same Tech(s)** to be built as their corresponding corporation's does (e.g. `TECH_MARINE_TECHNOLOGY` and `TECH_CORPORATION` for the Sid Sushi's executive unit; was `NONE`.) to be built.
  - This simplifies the Sevopedia Unit Era grouping implementation so they are correctly listed (e.g. "at Modern Era", not at "No Technology Prequisite"), and executive's effective availability (i.e. not until the later eras).
  - This also balances **rival city acquisition** so a player **cannot produce executives if we do not have the corresponding Corporation** in the city (mirroring the shrine prerequirement change).
- **Great Person variety:** each corporation uses **one distinct Great Person type** (no overlaps).

## Votes

- Add `<Civilopedia>` field to the [CIV4GameInfoSchema.xml](/Assets/XML/GameInfo/CIV4GameInfoSchema.xml) and populate them in [CIV4VoteInfo.xml](/Assets/XML/GameInfo/CIV4VoteInfo.xml) so votes show **text** in the new **Sevopedia Votes** (from existing Civ4 text in the "Outdated" sevopedia category (`ConceptInfos`)).
- Moved Vote XML **read time** from `CvXMLLoadUtility::LoadPostMenuGlobals` to `CvXMLLoadUtility::LoadPreMenuGlobals` (else sevopedia votes would be empty until a game is started).

## Event Triggers and Events

- The standard new game mode (**Simple Game**) now has **Events Disabled** by default.
  - We find they can be annoying, distracting, or frustrating; they do not offer a strong gameplay value, and setting up a Custom Game every time to disable them is tedious.
  - Players can still enable events via the **Custom Game** mode: make sure the "No Random Events" option is unticked, or alternatively revert this change.
  - Change in [CIV4GameOptionInfos.xml](/Assets/XML/GameInfo/CIV4GameOptionInfos.xml) (`GAMEOPTION_NO_EVENTS`'s `bDefault`: `0` -> `1`).
- (Requires AdvCiv-SAS 6219+) **World Trade Forum event:**
  - rethemed the inherited Earth Day event after AdvCiv-SAS redirected its requirement and international outcome from Environmentalism to **Trade Bloc**.
  - Its announcement, choices and help now describe international commerce and economic integration, matching the existing Trade Bloc requirement/adoption effect; costs, happiness and gameplay effects are unchanged.
  - See [KI#314](/_1_AdvCiv-SAS/Docs/README_Known_Issues.md#ki-314).
- Expose `CvEventTriggerInfo` and `CvEventInfo` **python getters** (done so SevoPedia Event Triggers can show them), and fix 3 incorrect ones (see [KI#122](/_1_AdvCiv-SAS/Docs/README_Known_Issues.md#ki-122)).
- Moved Vote XML **read time** from `CvXMLLoadUtility::LoadOptionalGlobals` to `CvXMLLoadUtility::LoadPreMenuGlobals` (similarly to votes else SevoPedia Event Triggers are not loaded in main menu's launch time and are empty in Sevopedia).

## Barbarians

- **Barbarians — stronger early game & smarter aggression**: economically **stronger in the early game**, growing at a pace closer to human/AI civs and **founding cities more often**, so they stay **relevant longer** before gradually fading by midgame.
  - They still aren’t a full player (**no diplomacy**; **scattered independent cities** that collectively count as the Barbarian civilization).
  - This pairs with the AI’s improved **hammer efficiency and availability** (e.g. and notably, far fewer scrapping issues): by engaging and "burning off" surplus early units (e.g., **ancient macemen, archers, longbowmen**), barbarians help prevent runaway over-production and early bankruptcies while empires keep growing.
  - For definitions and exact knobs, see the Sevopedia **Barbarian Civilization** entry or XML: [CIV4CivilizationInfos.xml](/Assets/XML/Civilizations/CIV4CivilizationInfos.xml), [CIV4BuildingInfos.xml](/Assets/XML/Buildings/CIV4BuildingInfos.xml), and [CIV4HandicapInfo.xml](/Assets/XML/GameInfo/CIV4HandicapInfo.xml).
- Barbarian civ-specific **buildings** can be captured (`iConquestProb`) with **half the chance** of their generic building equivalent (since their cities are overall easier and lower risk to capture, it balances it out). See Sevopedia/XML.
- Barbarian **workers** are civ-specific and **cannot be captured** (prevents worker farming).
- Barbarian **workboat** is civ-specific mainly for cost/balance.

## Civilizations

- New civs: **Kingdom of Benin**; **Ireland**. See also [World map with civs](/README.md#world-map-with-civs).
- **Starting techs — reworked**: `TECH_AGRICULTURE` & `TECH_THE_WHEEL` are **no longer** starting techs; `TECH_POTTERY` **now is**.
  - Even distribution across civs (**~14 per tech**).
  - A **"pair"** = a civ’s two starting techs (unordered), e.g., **Native Americans** now `HUNTING + MINING` (fits Dog Soldier / Bronze Working).
  - **Current** max pair frequency is **4** (not a hard cap).
  - Done with **heavy assistance from ChatGPT-5** plus manual tuning.
  - See: [Assets Rebalancing table (starting-techs (civ list & rationale))](/_1_AdvCiv-SAS/Docs/README_Assets_Rebalancing.md#after-most-recent-as-of-now-rework), or the values and rankings only in the new **Sevopedia Techs' Starting Techs Charts**.

## Leaders

- New leaders: **Ewuare**; **Grace O'Malley**, **Michael Collins**.
- Add support for optional **leaderhead art per era** with the new `EraArtDefineTags` and `EraArtDefineTag` XML fields; e.g., Gandhi's appearance and button changing at Ancient, Classical, Medieval, etc. The feature **is technically functional**, but **currently no leaderhead era art has been added**.
  - Note: This is **per-player** not human-centered; e.g. even if the human player is at Medieval Era, if the rival Gandhi is still at Classical Era then Gandhi will use the Classical Era (if set).
  - If none is set or disabled in SAS defines, the art defaults to the base `ArtDefineTag`.
  - Note 2: due to a Civ4 limitation, if a leader (e.g., Gandhi) is used by several players (e.g., in Custom Game), only the first of these leaders' current era is used to select the art for all instances of that leader regardless of their own progress (e.g., if Gandhi 1 is at Medieval and Gandhi 2 and 3 are still at Classical, then Gandhi 2 and 3 will use Gandhi 1's Medieval art).
  - See [README.md (EraArtDefineTags and EraArtDefineTag for leaderhead era art)](/README.md#eraartdefinetags-and-eraartdefinetag-for-leaderhead-era-art) and [KI#120](/_1_AdvCiv-SAS/Docs/README_Known_Issues.md#ki-120).
- Each leader now has **distinct leader music** (previous BTS/AdvCiv shared tracks split per leader).
  - See [Copyright & disclaimer](/README.md#copyright-and-disclaimer).
  - Note: switched **De Gaulle** from Louis' music in Base AdvCiv to Base Civ4's Napoleon's music, and added an AdvCiv-SAS specific new music for Napoleon so each leader has its own specific music, see [KI#98](/_1_AdvCiv-SAS/Docs/README_Known_Issues.md#ki-98).
- **Trait overhaul:**
  - Overhauled for historical fit + gameplay relevance.
  - **Examples**: Julius Caesar Organized + Imperialist → Aggressive + Imperialist *(military conqueror; offensive pressure)*; Saladin Protective + Spiritual → Spiritual + Charismatic *(religious leader with battlefield charisma; warmonger in Civ IV terms)*; Hatshepsut — Spiritual + Creative → Spiritual + Financial *(historical emphasis on trade/wealth; modest buff in practice)*.
  - Full before/after with rationale in [Assets Rebalancing table (Leader Traits)](/_1_AdvCiv-SAS/Docs/README_Assets_Rebalancing.md#leaders-traits-rework), or the values and rankings only in the new **Sevopedia Traits' Charts**.
  - *Note: leader "flavors" unchanged.*
- Barbarian **leader button/icon** is distinct (no longer Genghis Khan’s).

## Diplomacy

- `BBAI_DEFENSIVE_PACT_BEHAVIOR` **disabled (0)** to restore default BTS behavior (pacts break on DoW).
- **Voluntary** vassals are **permanent** (thematically a full culture absorption/merger).
- **Permanent Alliance** moved off the main military path (was on both **Communism** and **Fascism**) to reduce military beeline dominance and respect "one tech per major unlock" consistency.

## Buildings

- **Granary:** unlocked at **Pottery**; cheaper, slightly weaker food retention (partly to balance the Slavery/Hurry changes). Civ-specific granaries keep their relative advantages. Note: the Incan granary (Qullqa) was reworked toward a production/growth role rather than a culture role.
- **Water building line reworked/buffed** *(Goal: make **water tiles more viable** while requiring you to work them for the benefits; yields apply per worked tile, not passively from routes.)*:
  - **Harbor** (first water building): health/food oriented.
  - **Lighthouse:** gold oriented (**+x commerce** from worked tiles where applicable).
  - **Port (new):** production oriented (**+x hammer** from worked tiles where applicable).
  - **Moai Statues:** world wonder with broader water synergy; somewhat stronger overall.
  - **Customs House:** buffed.
- **Great Person Buildings** (e.g. Academy, Scotland Yard) now **require a tech** (e.g. `TECH_LITERATURE`, `TECH_MONARCHY`) so they appear in the tech tree and in the Sevopedia Building's corresponding group rather than in the "No Tech Prerequisite" group.
  - This may make strategic considerations more specialized and help slightly curb military beelines in particular.
- Fewer **national** wonders and relatively more **world** wonders.
- **Palace-likes** (Forbidden Palace, Versailles, etc.) mostly reworked as **world** wonders with tech prereqs so they appear in the tech tree and AIs don’t build them too early. (Barbarian Palace unchanged; used as a balance lever.)
- Many world-wonder effects and timings simplified/retuned; overall costs adjusted (often slightly lower).
  - Flat culture and Great Person points now progress consistently with tech timing; late wonders receive culture only when it fits their role, and a culture percentage modifier no longer stacks with flat wonder culture.
  - See also [wonder_culture_gpp_columns.py (.github workflows README)](/.github/workflows/README.md#buildwonder_culture_gpp_columnspy).
- Most `BonusProductionModifiers` (e.g., +100% with Marble) are greatly reduced. Example: Parthenon is now +25% with Marble (was +100%). Wonder costs were also adjusted to fit this change.
- Removed buildings that didn’t carry their weight or distorted pacing (e.g., **Space Elevator**, **West Point**).
- **Iron Works:** now requires **both Coal and Iron empire-wide**; city requirement count lowered; cost up moderately (e.g., 700 -> 800).
- **Heroic Epic** (`iMilitaryProductionModifier`): **100% → 50%**. The always-on military production boost was too strong and could let AIs run away; this tones it down while keeping the building relevant.
- Some **civ-specific** buildings replaced (especially late-game or underwhelming ones).
  - Example: Celtic **Dun** (Walls, mostly useless for an aggressive-leaning Civ) replaced with **La Tène Smithy** (forge-based), or the USA's too late to be useful **Supermarket** with the sooner and more thematic and tailored to the Civ's needs **Federal Courthouse** (helps the Civ scale into a large empire as it seems to often do in autoplay, with a spy/security theme too that can be useful for defense or offense).
  - See [Assets rebalancing: Civ-specific buildings rework](/_1_AdvCiv-SAS/Docs/README_Assets_Rebalancing.md#civ-specific-buildings-rework) for detailed rationales and before/after changes table.
- Added `BuildingClassRequired` chains to curb spam and improve AI focus: e.g., **Drydock** now requires **Port**.
- **Culture rebalance:**
  - Trimmed incidental culture from science buildings (e.g., **Library: +2 → +1**) and from some non-purely-cultural national wonders (**Heroic Epic**, **National Epic**, **Forbidden Palace**).
  - In exchange, **buffed/reworked culture-focused buildings** (e.g., **Colosseum** and civ variants) to keep the culture path meaningful.

## Units (Common)

- New **optional XML field**: `ObsoleteTech` **for units**: if set, this unit **can no longer be produced once we have discovered this tech** (e.g. Ancient Maceman (`UNIT_WARRIOR`) obsolete at `TECH_BRONZE_WORKING`).
  - This XML field is **optional**: if **not set** (`<ObsoleteTech>NONE</ObsoleteTech>`, e.g. for the `UNIT_SETTLER`) or missing entirely (better **inter-mod portability**), a **fallback default of `NONE`** is provided anyway, so the unit is never obsolete, meaning no tech discovery will stop allowing to produce it (base AdvCiv/Civ4 behaviour).
  - This new XML field allows to **avoid unit clutter in later eras** (no more **aberrant obsolete units in later eras** (e.g. Ancient Maceman produced in Medieval Era; Pikemen in Industrial Era)) being **produced** or shown in the **Production Chooser Bar**.
  - **AI** becomes **naturally stronger** as a result of these changes.
  - Additionally, this also helps set **clean scrapping rules** to computationally cheaply detect and remove old units for the AI's military composition, or **further enhancements**.
  - **Sevopedia** Blurbs with links fully implemented in [CvGameTextMgr.cpp](/CvGameCoreDLL/CvGameTextMgr.cpp).
  - See [README: ObsoleteTech for units](/README.md#obsoletetech-for-units).
  - See also [KI#93](/_1_AdvCiv-SAS/Docs/README_Known_Issues.md#ki-93).
- **Build timing** tweaks for some units for historical pacing; see the Technology Advisor.
- Some units have additional explicit technology requirements when an alternate path would otherwise make their displayed unlock too permissive. For example, **Chariots require both The Wheel and Animal Husbandry**, so the Masonry path to The Wheel is not sufficient without first developing animal domestication.
- Detailed civ-specific unit documentation now lives in [Assets rebalancing: Civ-specific units rework](/_1_AdvCiv-SAS/Docs/README_Assets_Rebalancing.md#civ-specific-units-rework), covering civilian and military civ-specific units with before/after/rationale rows for the current rework pass.
  - For updated in-game data, see also Sevopedia Unit.

## Civilian Units

- `DOW_UNIT_CAPTURE_CHANCE` **0 → 50**: workers can be captured **on the turn war is declared**, same as other turns. Aligns with the base capture chance and rewards opportunistic warfare.
- **Settler** is now a **national unit** (**max 1 per player**). This prevents AI from building two at once or building a new one while an existing settler is travelling, improves hammer/food efficiency, and raises the stakes on timing/escorts. See [KI#37](/_1_AdvCiv-SAS/Docs/README_Known_Issues.md#ki-37).
- **Scouts**, **Explorers**, **Airships**, and **Robotic Recon Drones** are now **national units** (**max 2 each per player**): reduces AI scout overproduction, keeps unit maintenance more efficient, and makes early scouting choices a bit more strategic.
- **Explorer-class** units can **attack**, but with low strength.

## Military - Common

- **Unit Combat Types** expanded/refined (e.g., archers split into short/long/crossbow; mounted split into `UNITCOMBAT_MOUNTED_MELEE` vs `UNITCOMBAT_MOUNTED_RANGED`).
  - Lets counters be more specific (e.g., pikes vs mounted-melee but not vs mounted-ranged).
  - Some civ-specific units shift types accordingly (e.g., **Ballista Elephant** is mounted-ranged while **War Elephant** is mounted-melee).
  - **Airship** becomes **Recon** (no longer sees subs; later recon can).
- **Reassignments:** e.g., **Mechanized Infantry** counts as **Armored** (not "Gun") for counter logic.
- **City defenses** affect all units, including gunpowder and later. See `bIgnoreBuildingDefense` in `CIV4UnitInfos.xml`.
- **Collateral** values normalized (fixed cases like **Stealth Bomber** having higher base collateral than its limit).
  - Most units now show a **collateral damage limit** (starting at 0 for many) (hidden for civilian units since they don't use it).
  - **Collateral promotions 1/2** are broadly available where eligible; (Requires AdvCiv-SAS 6202+) zero-base-damage military units can now acquire them when their nonzero limit/max values and UnitCombat eligibility support collateral, instead of being rejected by the inherited base-damage gate.
  - Info shown in Sevopedia (Unit chart/Special Abilities) and unit hover panels.
  - See [KI#324.2](/_1_AdvCiv-SAS/Docs/README_Known_Issues.md#ki-324.2).
- **Promotions retuned:**
  - Some buffed (e.g., **Woodsman**), some reworked/nerfed (e.g., **Hills Master 2 and 3**), some prereqs changed (e.g., **Archers** can take **City Bombard**; **Logistics** (formerly Commando) requires **Combat 1** or **City Raider 1**; higher-tier promotions (**City Raider/Garrison/First Strike 3**) also need **Combat 2**).
  - **Logistics** now at **The Wheel** (was Military Science).
  - Removed redundant promotions (e.g., **Tactics**), to avoid broken retreat odds.
- **Upgrade paths** simplified and made more sensible:
  - Mounted tiers upgrade along mounted lines (e.g., **Chariot → Horse Archer**; **War Elephant → Knight**).
  - Cross-mount upgrades allowed (**Horse <-> Camel** where logical).
  - **Longbows/Crossbows/Macemen/Pikemen → Musketmen** (more historical/balanced; reduces abuse by **Cuirassiers**, which are slightly stronger but more expensive).
  - **Dragoons → Tanks** now.
- **Support costs:** some late/robotic units have **no support cost**; **Privateer** now **has support** (was free).
- **Weak/late or overly narrow civ-specific** units are often **replaced by earlier, broader or more impactful ones** (e.g., Netherlands East Indiaman → Netherlands Swift Worker; German Panzer → Hussar; Mongol Keshik → Khishigten — better window with the Ger, longer medieval span, higher hammer rate, and more historical; English **Redcoat** → **Yeoman archer** — crossbow-based, fits England’s defensive profile, and gives cover when short on early bonuses; Holy Roman Landsknecht → Houfnice — cannon-based aligns with HRE’s aggressive profile, hits the tight medieval window, and avoids Landsknecht’s obsolescence/balance issues (anti-melee niche fades; weak vs. Cuirassiers); Maya Holkan → Quick Worker, replacing a narrow early Spearman role with a broadly useful +25% work-rate civilian UU).
  - Detailed civ-specific unit documentation now lives in [Assets rebalancing: Civ-specific units rework](/_1_AdvCiv-SAS/Docs/README_Assets_Rebalancing.md#civ-specific-units-rework).
  - For updated in-game data, see also Sevopedia Unit.
- **No naval civ-specific units as of now**: naval replacements can be effectively useless on land-heavy maps but disproportionately valuable on naval-heavy maps, especially when most other civilizations have no comparable naval replacement.
  - Current examples of moving away from that design include the **Netherlands East Indiaman → Swift Worker** and **Portugal Carrack → Cacador** reworks.
  - This keeps civ-specific unit value relevant across map types without requiring a much larger roster of naval counterparts; ordinary naval units and naval warfare remain available as usual.
  - See [Assets rebalancing: Civ-specific units rework](/_1_AdvCiv-SAS/Docs/README_Assets_Rebalancing.md#civ-specific-units-rework) for the fuller rationale.
- **Broad unit reworks/rebalances:** e.g., French **Musketeer** — Move 2 → 1; gains anti-mounted and small collateral kit for a stronger Renaissance+ role (especially vs Cuirassier units and their upgrades).
- **Bonus requirements relaxed (selective):**
  - Some units now accept additional **bonus** prerequisites where appropriate (to avoid odd lockouts).
  - Examples: **Samurai** can use **Copper** as well as Iron; **Swordsman-class civ-specific variants** (e.g., **Roman Legionary**, **Incan Quechua**) can also use **Copper** (only the **Gallic Warrior** could before; which seemed inconsistent and unbalanced without a clear reason why: now equalized); **Crossbowman-class civ-specific variants** (e.g., **Zhuge Nu**, **Disciplined Crossbowman**) Bronze/copper hardware was common (Benin bronzes; Chinese bronze/iron mix long before ubiquitous steel) and to buff them; Holy Roman **Houfnice** can also use **Copper** (historically plausible bronze artillery).
  - The **generic Swordsman** and the **generic Cannon** remain **Iron-only** to preserve strategic pressure.
  - Thematically consistent.
  - See Sevopedia/XML.
- Units more **versatile:** e.g., Swords defend better; Archers can attack more credibly.
- **Stealth** removed where it didn’t make sense (kept for subs, spies, stealth bombers).

## Military - Land

- **Camel-mounted line:** Added new camel units (Camel Archer, Camel Knight, Camel Cuirassier) tied to `BONUS_CAMEL` as an alternative to horse units, and reworked Arabia’s civ-specific unit as the Hajjan, a Camel Knight variant.
- **Melee Unit Combat Type split (rework + balance change):**
  - `UNITCOMBAT_MELEE` is now split into `UNITCOMBAT_MELEE_POLEARM` (melee units that attack from some distance with a lance-like weapon; e.g. Spearmen, Pikemen) and `UNITCOMBAT_MELEE_SHOCK` (close-contact/brawl; e.g. Axemen, Swordsmen, Macemen).
  - For example, after this change, axes still counter polearms but are only slightly stronger than swords; pikemen are no longer worse than axemen which was very odd considering cost and tech requirements difference.
  - Civ-specific variants such as the Babylonian Bowman, or simply the generic Crossbow, now only apply vs. `UNITCOMBAT_MELEE_SHOCK` (i.e., weak vs pikes).
  - **General idea for melee units:** Axeman > Spearman; Crossbow > Axeman/Swordsman/Maceman; Pikeman > Crossbow; Pikeman ≈ Axeman; Maceman > Pikeman.
  - Civ variants retagged to match gear (e.g., Persia Immortal → polearm).
  - **General idea for mounted units:** Spearman > Chariot (`UNITCOMBAT_MOUNTED_MELEE`); generic Horse/Camel Archer (`UNITCOMBAT_MOUNTED_RANGED`) > Spearman; generic Horse/Camel Knights (`UNITCOMBAT_MOUNTED_MELEE`) < Pikemen (`UNITCOMBAT_MELEE_POLEARM`); generic Horse/Camel Cuirassiers > Pikemen (`UNITCOMBAT_MELEE_POLEARM`).
  - See Sevopedia/XML.
- **Animal** units broadly rebalanced; see Sevopedia/XML for details.
- **Barracks** now require **Hunting**.
- **Warrior / "Ancient Maceman"** rebalanced: higher base strength, lower city-combat, gains **vs Animals** bonus; cost slightly up.
- **Archer / Longbowman:** both available earlier with lower base strength and cost but with new stat mixes; **Archer** no longer upgrades to late units; **Longbow** upgrades to **Crossbow**.
- **Spearman line:** base Strength **4 → 5**; anti-mounted role strengthened; civ-specific variants further differentiated (see Sevopedia/XML).
- **Grenadiers & Bazooka (formerly Anti-Tank Infantry)**: rebalanced into **hybrid, versatile "foot-bomber" roles**.
- **Gun units buffed (relative to pre-gun)**: units that **carry firearms** (e.g., **Musketman**, **Horse/Camel Cuirassiers**) are **relatively stronger** than their pre-gun counterparts.

## Military - Naval

- **Naval upgrades** cleaned up; **Ironclad** can enter ocean and is buffed, serving as a real upgrade from Frigates. See [README: military tree](/README.md#military-tree-and-changes) for full upgrade maps.
- **Naval** units broadly rebalanced; see Sevopedia/XML for details.
- **Caravel** removed (privileged border passing felt wrong); shift that niche to **Privateer** (buffed) and streamline the era’s naval roster.
- **Nuclear Submarine** removed (under-impactful relative to complexity).

## Military - Air

- **Air** unit costs generally increased.

## Fixes

See [Known issues / fixes](/README.md#known-issues-that-may-be-fixed-or-not-fixed-in-base-advciv-or-civ4) for the indexed list and links to before/after notes and screenshots.
