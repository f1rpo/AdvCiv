# Civ4 / AdvCiv-SAS logging inventory

**Authors (2026-10-19):** GPT-6.1-Sol (Codex); historical provenance research and rebase by ChatGPT-5.6-Sol. Minor review from Claude Sonnet 5.5: 2026-10-20.

Use this inventory to locate diagnostic output, identify its writer or enabling conditions, distinguish current SAS evidence from older or other-mod files, and compare meaningful inherited/base behavior with current SAS behavior.

**Editorial scope:** this is a canonical reference/inventory, not an iterative changelog.

- Keep Before/After comparisons when they show a meaningful inherited/base/legacy -> current SAS difference or make a SAS enhancement easier to understand.
- Treat the **After/current SAS** side as canonical; replace superseded intermediate SAS stages instead of stacking them as implementation chronology.
- Keep step-by-step development history in Git, KIs or subsystem revision histories.
- Retain historical details here when they explain provenance, ownership or compatibility, or help identify an older file/output someone may encounter.

Dated snapshots below identify when local presence, counts, settings and file dates were inspected; they are not live status or a claim that every installation produces these files.

The EXE is closed source, mods can supply arbitrary filenames, and some outputs require particular situations or compile options. Consequently, an exhaustive universal filename list is unavailable. The tables distinguish observed files from source-supported possibilities.

## Overview

This inventory brings together game, AI, Python, engine and build diagnostics, including what SAS changes relative to inherited output and which diagnostics SAS adds itself.

The file inventory and source references distinguish current writers from historical or other-mod output. Matched Before/After sections show important inherited/current differences without preserving obsolete intermediate SAS stages.

- `PythonDbg.log` (debug messages), `PythonErr.log` (tracebacks) and `PythonErr2.log` (including module-loading output) have captured [Before/After samples](#before-and-after-shared-python-log-timestamps); `xml.log` separately now has real pre-change and rebuilt-DLL After captures for its DLL-controlled UTC/`record=` modernization.
- `BBAI_*.log`: [AI decision diagnostics](#bbai), including Settler behavior and formerly independent UWAI reports now merged into BBAI; explicitly covered producers include `MilitaryAnalyst`, `InvasionGraph`, `ArmamentForecast`, `WarEvaluator` and `WarUtilityAspect`.
- SAS-created `SASGameRecord_*.log`: [gameplay/run context](#sasgamerecord), with a 30-row captured excerpt and a separate full example.
- `RandLog` / `MessageLog` INI controls and RNG/message traces such as `MPLog.txt`, `MPLog<player>.log` and `ASyncRand.log`; `OOSLog - <player> - Turn <turn>.log` captures state after multiplayer desynchronization.
- `StartingPos.log` for starting-position diagnostics and `IFP_log.txt` / `TSCProfile.log` for profiling; BUG Autolog supplies a configurable text/HTML gameplay journal.
- `SASRuntimeStatus.json` / `SASRuntimeHistory_*.jsonl`: [latest runtime observation and transition history](#runtime-status-and-history), linked to representative examples.
- Compiler `AdvCiv.log` / `CvGameCoreDLL.dll.build.json`: [build output, progress/results and identity](#dll-build-reports), with tracked example captures and links to the compilation guide. Ordinary generated build logs remain local; the linked `_example` captures are deliberately tracked documentation files.
- Historical/other-mod outputs such as `bull.log` (BULL), `C2C.log` (Caveman2Cosmos) and `MLF.log` (modular XML loading), with uncertain cases such as `ResourceAllocation.log` kept explicit.

## Historical lineage and ownership

The files below do not belong to one logging framework. Civ4 accumulated several generations of engine, Python and mod diagnostics, and that lineage affects ownership, format and safe modernization. Dates identify the earliest documented evidence cited here, not necessarily the first private implementation.

| Output / family | Producer / lineage | Established history and purpose | SAS ownership / modernization implication |
| --- | --- | --- | --- |
| `PythonDbg.log`, `PythonErr.log`, `PythonErr2.log` | Firaxis Civ4 Python/EXE logging, with occasional DLL-written `PythonDbg.log` rows | All three are documented together on 2005-10-28, during Civ4's launch period. `PythonDbg` is the general Python/debug stream; `PythonErr` carries Python exceptions; `PythonErr2` is a secondary startup/interpreter stream rather than simply another traceback file. | The EXE owns file creation and early startup output. SAS consistently formats Python-written rows after `SASPythonLogging` installs. The one current direct DLL-written Starting Position Iteration row in `PythonDbg.log` also uses SAS UTC/`record=` formatting. Earlier/direct EXE rows remain outside SAS control and cannot have their original timestamps reconstructed. |
| `app.log`, `init.log`, `initmgr.log`, `audio.log`, `resmgr.log`, `xml.log`, `LSystem.log`, `network-<port>.txt` | Firaxis engine, with live DLL-origin rows in `xml.log` | Native application, initialization, audio, resource, XML, LSystem and networking diagnostics. Old Civ4 troubleshooting captures already show the engine's numeric elapsed-time-style prefixes. Current SAS source has live DLL writers for `xml.log`; `resmgr.log` retains only a commented-out inherited writer, and no live DLL writer was found for the other listed engine logs. | SAS adds millisecond UTC and stable `record=` fields only to the DLL-owned `xml.log` rows while preserving the native elapsed prefix. EXE-owned rows remain in their native format. Absence of a current DLL writer means SAS has no in-place row-format hook there today; it does not prove that future interception or integration would be impossible. |
| `MPLog.txt`, optional `MPLog<player>.log`, `ASyncRand.log` | Firaxis message-control/RNG logging, later wrapped and extended by AdvCiv | The INI's message/random/synchronization logging family is old Civ4 machinery. AdvCiv introduced `CvDLLLogger` in 2019 to encapsulate the message-control log and supports per-player MP filenames; a 2020 AdvCiv fix made asynchronous RNG logging work correctly. | Preserve deterministic comparability. Session/header context can help, but wall-clock decoration must not obscure RNG or OOS comparisons. |
| BUG Autolog | eotinb -> BUG -> K-Mod/AdvCiv/SAS | Publicly introduced on 2005-11-15 as a note-taking/gameplay journal "with succession games in mind". Its author explicitly distinguished it from Civ4's existing debug/error logs. | Preserve its human-readable journal role. It is configurable text/HTML output, not a machine diagnostic stream. |
| BUG StatusDump | Ruff_Hi / BUG, inherited through K-Mod/AdvCiv | Current SAS loads the inherited `StatusDump` module. Ctrl+Alt+D invokes a support dump using the Autolog path and a `<player>_<year>.txt` filename. Large sections remain literal placeholders such as `Spinner stuff here`, `Player Cities here` and `AIs stuff here`; the older unused helper class still hardcodes `StatusDump.txt`. | Document as an incomplete inherited support facility, not as a dependable SAS diagnostic. Do not modernize it merely because it can emit a file. |
| `BBAI.log` -> `BBAI_*.log` | Better BTS AI -> K-Mod -> AdvCiv -> SAS | Better BTS AI 0.82 publicly documented its AI decision logging system on 2009-11-05: the purpose was to expose major AI decisions each turn for analysis and improvement. The original file was `BBAI.log`. | SAS owns its current BBAI wrapper/filename/session improvements while preserving the log's AI-decision semantics. This is a legacy diagnostic worth modernizing, not replacing. |
| `bull.log` | BULL, the DLL companion to BUG | BULL 1.0 was released with BUG 4.2 on 2009-11-15. Public BULL-derived sources use `bull.log` for DLL-side BUG/options diagnostics. No current SAS filename producer was found. | Treat the local file as historical/foreign evidence unless a current producer is established. |
| `OOSLog - <player> - Turn <turn>.log` | Kek-Mod -> AdvCiv, with idea credited to Fall from Heaven 2 / Kael | Kek-Mod 0.23 (2019-03-18) included `OOSLogger`; AdvCiv merged it on 2019-03-27. Kek-Mod 0.25 (2019-07-16) then incorporated OOSLogger fixes from AdvCiv. It dumps deterministic game state when an OOS is detected. | Keep distinct from `MPLog`: OOSLog is a state snapshot for diffing after divergence, while MPLog helps trace message/RNG activity leading to divergence. |
| `IFP_log.txt` | Inherited DLL internal profiler | Function/call timing profiler output; earliest introduction not established. | Keep numerical/data-oriented. Prefer run metadata or a header to per-row decoration. |
| `TSCProfile.log` | We the People profiler -> AdvCiv | AdvCiv imported Time Stamp Counter profiling code from We the People on 2019-11-05 and integrated it with Civ4 logging. | Keep profiler rows compact and machine-friendly; add provenance/header context only if needed. |
| `StartingPos.log` | AdvCiv | AdvCiv's Starting Position Iteration work began publicly on 2020-05-27 as a fairer-start algorithm; this log traces that subsystem when enabled. | Algorithm trace, not general diagnostics. Header/run context is preferable to verbose per-line timestamps. |
| `juggle_log.txt`, `combat.txt` | K-Mod/AdvCiv developer diagnostics | Compile-time specialist diagnostics: citizen-assignment efficiency and combat-odds verification respectively. Exact first introduction was not established here. | Preserve as experiment datasets. A small metadata header is safer than making every record verbose. |
| `uwai<turn>.log` | AdvCiv `UWAIReport` | Separate war-evaluation reports, with Textile markup, turn-number filenames and report interval/threshold controls. Writers include `UWAIAgent`, `MilitaryAnalyst`, `InvasionGraph`, `ArmamentForecast`, `WarEvaluator` and `WarUtilityAspect`. | Historical in SAS: the standalone report files/logger were retired in favor of five independently controlled BBAI categories; see [UWAI diagnostics comparison](#uwai-diagnostics-standalone-advciv-reports-vs-current-bbai-records). |
| `SASGameRecord_*.log` | AdvCiv-SAS | A new structured game record, complementing inherited BBAI decision traces with run context, periodic state, gameplay history and reproduction evidence. | Independent `SAS_GAME_RECORD_LOG_LEVEL`; its [revision history](revisions/SASGameRecord.md) documents recorder changes. |
| `SASRuntimeStatus.json`, `SASRuntimeHistory_*.jsonl` | AdvCiv-SAS | Latest runtime observation and per-process transition history, including process/DLL identity and new/load/autoplay activity. | Independent `SAS_RUNTIME_STATUS_ENABLE`; its [revision history](revisions/SASRuntime.md) documents runtime-recorder changes. |

### Modernization principles

Format changes follow the ownership and purpose of each diagnostic family:

- **SAS-controlled diagnostics:** direct timestamp, filename, session, provenance and flushing improvements are appropriate when they preserve semantics and performance.
- **Shared Python streams:** SAS can improve rows that pass through its Python wrapper after installation, while clearly preserving the existence of EXE-written startup rows that bypass it.
- **EXE-owned Firaxis logs:** do not pretend SAS controls the writer. Modernize DLL-owned rows that deliberately share such a file, but keep EXE-owned rows and their native timing semantics explicit.
- **RNG/OOS/profiling/experiment data:** preserve stable comparison-friendly rows; a header or sidecar is often better than per-row wall-clock text.
- **Human journals such as Autolog:** optimize for readable gameplay notes, not grep-oriented diagnostic structure.
- **Foreign/stale files from other mods:** identify their probable or verified origin, but do not add SAS support merely because they remain in the shared user folder.

## Files present in the user Logs folder

The folder is `Documents/My Games/beyond the sword/Logs`. Presence establishes that a file exists, not that the current SAS run wrote it. Empty files are included.

Inventory snapshot: **2026-10-20 08:34:07 UTC**. The Python captures were taken after process 32608 exited; its runtime status records `EXITED_CLEANLY`.

**Latest modified (UTC)** is the filesystem's last-write time; for a family it is the newest member's time. It helps identify stale evidence but does not establish generation time or which mod wrote it: copying/restoring files can preserve or change filesystem dates. Embedded process/session timestamps provide stronger run identity where available.

| File or family | Present | Latest modified (UTC) | Role / evidence |
| --- | --- | --- | --- |
| `PythonDbg.log` | Yes | 2026-10-20 08:30:49 | Python stdout, direct prints, BUG messages and some DLL diagnostics. |
| `PythonErr.log` | Yes, empty | 2026-10-20 08:16:03 | Python error output; an empty file is distinct from an absent file. |
| `PythonErr2.log` | Yes | 2026-10-20 08:16:03 | Secondary Python output; current sample includes converter warnings and the Python search path. |
| `app.log` | Yes | 2026-10-20 08:30:48 | EXE/application activity; current sample includes activation and framebuffer information. |
| `init.log` | Yes | 2026-10-20 08:16:30 | Startup/version and initialization messages. |
| `initmgr.log` | Yes | 2026-10-20 08:16:30 | EXE initialization manager; current sample includes new-game initialization and elapsed time. |
| `audio.log` | Yes | 2026-10-20 08:16:03 | Audio-manager initialization and asset loading. |
| `resmgr.log` | Yes | 2026-10-20 08:16:31 | Resource-loading diagnostics; current sample includes texture-load failures. |
| `xml.log` | Yes | 2026-10-20 08:16:26 | XML loading and related errors; current DLL also explicitly writes this file. |
| `LSystem.log` | Yes | 2026-10-20 08:16:32 | City/plot LSystem diagnostics; current sample begins with the city LSystem XML. |
| `network-2056.txt` | Yes | 2026-10-20 08:30:49 | Network diagnostics with the configured port in its filename. Other ports/instances may produce other names. |
| `BBAI_*.log` | Yes, 28 files | 2026-10-18 14:08:54 | Timestamped detailed AI diagnostics, including older runs. |
| `SASGameRecord_*.log` | Yes, 1,119 files | 2026-10-20 08:17:39 | Timestamped game/session records, including older runs. |
| `SASRuntimeStatus.json` | Yes | 2026-10-20 08:30:49 | Latest fixed-name runtime observation and linked history identity. |
| `SASRuntimeHistory_*.jsonl` | Yes, 32 files | 2026-10-20 08:30:49 | Per-process runtime history. |

The BBAI/SASGameRecord counts above describe timestamped files, not a requirement that their legacy fixed filenames exist. Configured filename/session/player variations are handled by the shared filename builder.

### Other diagnostic files present

These files exist locally, but no corresponding filename producer was found in current SAS source. They are not evidence that SAS currently generates them; the shared Logs folder can contain output from other mods and earlier runs.

External-source identifications below describe matching public writers, not proof of which exact executable produced the local copy.

| Filename | Present | Latest modified (UTC) | External lineage / interpretation |
| --- | --- | --- | --- |
| `ANewDawn.log` | Yes | 2026-06-04 07:51:16 | Probably Rise of Mankind: A New Dawn by filename/context, but an authoritative producer for this exact file was not established. Keep origin/purpose explicitly unverified. |
| `C2C.log` | Yes | 2026-01-23 07:08:51 | Verified in current Caveman2Cosmos source as a broad generic DLL/debug log used by unrelated systems. |
| `Checksum.log` | Yes | 2026-09-04 13:37:08 | Verified in RoM-AND/C2C lineage: `CvGlobals` writes per-info checksums, useful for comparing loaded XML/info state. |
| `CvGameTextMgr_buildCityBillboardString.log` | Yes | 2026-06-04 08:12:28 | Matching public Civ4-mod evidence identifies this as city-billboard debug output; another project removed an unconditional writer as needless disk I/O. The origin of this local copy is not established. |
| `CvGlobals_logInfoTypeMap.log` | Yes | 2026-08-13 15:58:52 | Verified in modular-DLL descendants as a dump of the string-to-integer info-type map. |
| `CvTechInfo_readPass2.log` | Yes | 2026-08-13 15:58:53 | Producer not verified. The filename suggests second-pass tech-info XML diagnostics, but that interpretation must not be presented as established fact. |
| `CvXMLLoadUtilitySetMod_MLFEnumerateFiles.log` | Yes | 2026-08-30 15:18:26 | Verified in WoC/RoM/C2C-style modular-loading source: traces directories/subdirectories while enumerating module XML files. |
| `MLF.log` | Yes | 2026-08-30 15:18:20 | Verified Modular Loading Framework / modular-loading-control diagnostic, including module selection and load order. |
| `ResourceAllocation.log` | Yes | 2026-06-04 08:12:21 | Producer unverified; retain as unknown foreign/historical residue. |
| `XmlDependencyTypes.log` | Yes | 2026-08-13 15:58:54 | Verified in True Modular / RoM-derived XML loaders as dependency/type-resolution diagnostics. |
| `bull.log` | Yes | 2026-09-13 08:31:55 | BULL/BUG DLL diagnostic lineage is verified in public source, but no current SAS producer was found. |

The manually created ` - Copie` files are backups of existing log types, not additional Civ4 outputs. `Archipelago.cfg`, `Inland_Sea.cfg` and a saved Git diff also reside in Logs but are not log streams.

## Conditional outputs not found in the current Logs folder

These are supported by current source or inherited documentation. Absence does not indicate a fault: their enabling conditions may never have occurred, or an optional facility may not be compiled/enabled.

| File or family | Present here | Source / condition |
| --- | --- | --- |
| `MPLog.txt` | No | Inherited manual identifies the EXE message-control log. Current DLL calls `messageControlLog`; the EXE chooses its actual default filename. |
| `MPLog<player>.log` | No | Current `CvDLLLogger.cpp` explicitly uses this form for RNG logging in network multiplayer with `PER_PLAYER_MESSAGE_CONTROL_LOG`. |
| `ASyncRand.log` | No | `CvGlobals.cpp` assigns this filename to the extended asynchronous RNG; output requires the corresponding RNG logging path to run. |
| `OOSLog - <player> - Turn <turn>.log` | No | `OOSLogger.py` writes desynchronization state when OOS is visible, message logging is enabled and WorldBuilder is not active. Player names can appear in the filename. |
| `StartingPos.log` | No | Starting-position iteration diagnostics when its logging is enabled. |
| `IFP_log.txt` | No | Internal profiler output when that profiling facility runs. |
| `TSCProfile.log` | No | Timestamp-counter profiling, conditional on `USE_TSC_PROFILER` and collected samples. |
| `juggle_log.txt` | No | Citizen-assignment diagnostic inside `LOG_JUGGLE_CITIZENS`. |
| `combat.txt` | No | Combat-outcome diagnostic inside `LOG_COMBAT_OUTCOMES`. |
| BUG Autolog `<configured name>.txt` / `.html` | Not found in Logs or the checked default Autolog directories | User-configurable gameplay journal; its filename, directory and format are configurable, so this is not proof that no output exists elsewhere. |
| BUG StatusDump `<player>_<year>.txt` | Not found in the checked default Autolog directories | Current inherited module listens for Ctrl+Alt+D and writes through `BugFile` using the Autolog path. The implementation is incomplete and still emits placeholder sections; an older unused helper class separately hardcodes `StatusDump.txt`. |

`RandLog` and `SynchLog` are INI options, not verified standalone filenames in this inventory. RNG messages can go into the message-control log or an explicitly assigned RNG file. The synchronization-output filename is EXE-owned and has not been established here.

`debug_SPaH.log` appears only in a commented-out source statement; it is not an active output path. `Profile output.txt` likewise remains in a commented-out block in `TSCProfiler.cpp`; the active profiler writer uses `TSCProfile.log`. Obsolete commented filenames should not be presented as currently supported logs.

## Outputs outside the user Logs folder

| Output | Present | Meaning |
| --- | --- | --- |
| `CvGameCoreDLL/Project/<configuration>/AdvCiv.log` | Yes, Release and Debug-opt | Compiler/linker diagnostics, not game output. |
| `<DLL filename>.build.json` | Yes | Build status, configuration, inputs, warning/error counts and binary identity; includes the installed DLL's companion report. |
| BUG Autolog / StatusDump directory | No current file found in the checked default paths | Autolog and StatusDump normally use their configurable/default Autolog directory rather than requiring `Logs`. |
| `ThemeParseLog.txt` | Not inventoried in this snapshot | Historical BtS engine/theme parser diagnostic reported in the Beyond the Sword installation directory rather than the user `Logs` folder. It records `.thm` parsing/resource errors; some warnings can be benign. |
| Crash dump files | Not inventoried here | Binary crash evidence, not a text log. `GenerateCrashDumps=1` is currently configured; actual dump location/presence needs separate inspection. |

## Historical engine outputs not present in this snapshot

- `legacy.log` appears in old Civ4 troubleshooting captures with low-level graphics/resource compatibility messages such as legacy tree/asset use. It was not present in the inspected SAS Logs snapshot.
- `Python.log` appears in some early Civ4 troubleshooting material, but its exact relationship to the persistent `PythonDbg.log`/error trio across versions was not established. Treat it as a historical variant, not an expected current SAS file.

## Inspected switches and timestamp evidence

The inspected `CivilizationIV.ini` has:

| Setting | Value |
| --- | --- |
| `LoggingEnabled` | `1` |
| `OverwriteLogs` | `1` |
| `MessageLog` | `0` |
| `RandLog` | `0` |
| `SynchLog` | `0` |
| `GenerateCrashDumps` | `1` |

BBAI, SASGameRecord and SAS runtime recording have their own SAS controls; these files are not governed solely by the EXE logging switches.

Several existing engine files already begin messages with numeric timing tags such as `[30717.468]`. Old Civ4 captures show this style long before SAS.

Better BTS AI's original logging documentation describes its own left-hand values as time in seconds relative to Civ startup, with some uncertainty about the precise origin. This does not establish the clock source of every EXE log prefix; do not reinterpret those numbers as UTC calendar timestamps.

At this inventory baseline, `BugUtil` added local `HH:MM:SS` prefixes to its own messages, whereas direct Python prints and multiline dumps followed their writers' formatting. The shared Python writer described below uses UTC timestamps.

BBAI, SASGameRecord and runtime output already carry substantial instrumentation; timestamp/archival work should account for that existing context. Before/after comparisons should use real captured samples and distinguish timestamps emitted by a writer from observation times supplied by an external collector.

## SAS AI and game-record diagnostics

BBAI and SASGameRecord answer different questions: BBAI explains detailed AI decisions and candidate calculations; SASGameRecord supplies the broader game/run history needed to interpret them. SASGameRecord is an additional SAS-created output, not a renamed inherited BBAI or UWAI report.

Both can use UTC-stamped new/load filenames, shared source/DLL identity and the live-readable diagnostic writer. These are separate from the Python-stream modernization described below: BBAI and SASGameRecord retain their own compact row formats rather than receiving `record=` or a UTC clock prefix on every row.

### Session headers and provenance

Both log families place canonical mod/source context and exact loaded-DLL build/binary provenance immediately after the session lifecycle marker.

The source row records the practical commit-count version when available, exact Git commit, branch/date metadata when known, tracked dirty state/files for a live Git checkout, and a compact fingerprint/byte count of tracked gameplay code/rules/scripts changes relative to `HEAD`.

The shared DLL row separately records the build target, file size, a diagnostic FNV-1a 64-bit fingerprint of the DLL bytes, DLL last-write and PE/linker timestamps, and relevant compile flags, so source/rules edits and the binary actually loaded by Civ4 can be distinguished independently.

For example, a changed C++ diff fingerprint with an unchanged DLL fingerprint exposes a rebuild/copy mistake, while XML/Python edits can legitimately change only the source-diff fingerprint.

With no scoped tracked diff the byte count is `0`; unavailable Git-derived provenance remains explicit instead of being mislabeled as clean. These fingerprints are diagnostic identities, not cryptographic security hashes.

Identity rows place the stable `processUtc` for one Civ4 launch before plain `utc`; the row type identifies that second timestamp as a new-game, save-load or individual snapshot observation. Timestamped filename counters such as `new1` and `load2` preserve game order within the process.

`dirty` is tri-state: `-1` means unavailable because there is no inspectable Git worktree (for example an extracted GitHub ZIP), `0` means Git verified the tracked tree clean, and `1` means tracked local edits were found.

Source commit identifiers in these documentation examples are mapped to the rewritten Git history; recorded runtime and binary measurements are retained.

### SASGameRecord

Enable `SAS_GAME_RECORD_LOG_LEVEL` in [`GlobalDefines_advciv_sas.xml`](../../Assets/XML/GlobalDefines_advciv_sas.xml); `0` disables the recorder independently of BBAI. `SAS_GAME_RECORD_LOG_USE_TIMESTAMPED_FILENAME` selects per-session `SASGameRecord_*.log` files or the fixed `SASGameRecord.log` name.

The record brings together map/geography context, settings and provenance, economy/expansion, city and battle history, diplomacy and military composition. It also records exact AI strategy/victory-stage transitions, RNG evidence and shared diagnostic-writer health according to the selected level and supporting switches.

- Search examples include `GAME_RECORD_SOURCE_CONTEXT`, `GAME_RECORD_DLL_CONTEXT`, `GAME_RECORD_AI_STRATEGY_CHANGE`, `GAME_RECORD_AI_VICTORY_STAGE_CHANGE` and `GAME_RECORD_DIAGNOSTIC_WRITER`.
- `SAS_GAME_RECORD_INTERVAL_TURNS_UNSCALED_GAMESPEED` controls periodic snapshots; with interval 10 they occur at completed turns 10, 20, 30 and so on, while turn 0 has separate initial context. Chronicle actions can appear between snapshots.
- This is an all-player diagnostic and can contain spoilers. Use the [Main Changes Guide's SASGameRecord section](README_Main_Changes_Guide.md#sasgamerecord-log) and [recorder revision history](revisions/SASGameRecord.md) for coverage/controls, and the [representative raw record](examples/sasgamerecord/SASGameRecord_example.log) for actual output.

#### SASGameRecord session header example

For example, `SASGameRecord_*.log` starts with lifecycle, provenance and run-setup rows before adding RNG/state checkpoints and compact turn snapshots. These selected revision-137 rows come from the same October 9, 2026 development test as the [BBAI header](#bbai-session-header-example); gaps in `seq` mean unrelated intervening rows are omitted:

```log
GAME_RECORD_SAVE_LOADED seq=1 processUtc=20261009T160524Z utc=20261009T160535Z logFile="SASGameRecord_20261009T160535Z_load1.log" turn=0 elapsed=0 year=-50000 scenario=0 activePlayer=0 activeCivilization=CIVILIZATION_INCA activeHandicap=HANDICAP_MONARCH playersDefined=16 playersAlive=16 playersEverAlive=16 humans=1 sessionWallMilliseconds=0
GAME_RECORD_SESSION_CONTEXT seq=2 gameType=GAME_SP_LOAD gameMode=GAMEMODE_NORMAL newGame=0 savedGame=1 scenario=0 gameMultiplayer=0 networkMultiplayer=0 hotseat=0 pbem=0 pitboss=0 simultaneousTeamTurns=0 mpOptions=-
GAME_RECORD_GAME_SETTINGS seq=3 mapScript="Archipelago" map=120x84 landHeavy=0 navalHeavy=1 world=WORLDSIZE_HUGE climate=CLIMATE_TEMPERATE seaLevel=SEALEVEL_MEDIUM gameSpeed=GAMESPEED_NORMAL startEra=ERA_ANCIENT calendar=CALENDAR_DEFAULT startTurn=0 startYear=-50000 gameHandicap=HANDICAP_MONARCH maxTurns=500 targetScore=0 advancedStartConfiguredPoints=-1 advancedStartXmlDefaultPoints=-1 victories=VICTORY_SCORE,VICTORY_TIME,VICTORY_CONQUEST,VICTORY_DOMINATION,VICTORY_CULTURAL,VICTORY_SPACE_RACE,VICTORY_DIPLOMATIC options=GAMEOPTION_AGGRESSIVE_AI,GAMEOPTION_NO_EVENTS
GAME_RECORD_WAR_AI_SETTINGS seq=8 warPeaceAI=UWAI uwaiMode=FULL engineAggressiveAI=1 USE_KMOD_AI_NONAGGRESSIVE=0 DISABLE_UWAI=0 UWAI_IN_BACKGROUND=0 SAS_UWAI_INVASION_GRAPH_NAVAL_LOGISTICS_DEPLOYMENT_OPTIMIZE=1
GAME_RECORD_RUNTIME_CONTEXT seq=9 systemContextLevel=2 win32Runtime=NATIVE_WINDOWS wineVersion=- windowsMajor=10 windowsMinor=0 windowsBuild=19045 windowsUpdateRevision=6466 windowsServicePackMajor=0 windowsServicePackMinor=0 windowsDisplayVersion="22H2" pointerBits=32 processArch=X86 nativeArch=X64 wow64=1 cpuVendor=- logicalProcessors=-1 totalPhysicalMemoryMB=-1
GAME_RECORD_GAME_RNG seq=10 mapRandState=1378213624 syncRandState=909279002 initialMapRandSeed=13016467 initialSyncRandSeed=13016467
GAME_RECORD_MOD_CONTEXT seq=11 displayName="AdvCiv-SAS" folderName="AdvCiv-SAS" modPath="Mods\\AdvCiv-SAS\\"
GAME_RECORD_SOURCE_CONTEXT seq=12 recordRevision=137 version="6606" commit="39e40d78917c42d67dfa9c87a1fc92da178ad7dd" shortCommit="39e40d7891" branch="main" commitDate="2026-10-09T17:28:00+02:00" metadataSource="git" dirty=1 dirtyTrackedCount=6 trackedCodeRulesDiffBytes=16408 trackedCodeRulesDiffFingerprint="FNV1A64:F3AD9234EBE21773" dirtyFiles="[MM] .github/workflows/build/xml/sas_define_usage.py; [MM] .github/workflows/tests/test_sas_define_usage.py; [ M] Assets/CvGameCoreDLL.dll; [ M] Assets/XML/GlobalDefines_advciv_sas.xml; [MM] CvGameCoreDLL/BBAILog.cpp; [ M] _1_AdvCiv-SAS/Docs/README_Main_Changes_Guide.md"
GAME_RECORD_DLL_CONTEXT seq=13 build=Debug-opt moduleFound=1 fileReadable=1 fileSizeBytes=13774848 dllFingerprint=FNV1A64:97635FA4B4F6CE96 dllLastWriteUtc=20261009T160322.407Z peTimestampRaw=1791561793 peTimestampUtc=20261009T160313Z fassertEnabled=0 debugDefine=0 ndebugDefine=1
GAME_RECORD_EXE_CONTEXT seq=14 moduleFound=1 fileReadable=1 fileSizeBytes=12594192 exeFingerprint=FNV1A64:98123F43E2DC14E6 exeLastWriteUtc=20250328T141834.588Z peTimestampRaw=1401330529 peTimestampUtc=20140529T022849Z
GAME_RECORD_INSTALL_CONTEXT seq=15 systemContextLevel=2 distributionHint=STEAM steamAppId=8800 steamBuildId=283175 steamBetaKey=- absoluteExePath=- absoluteModPath=-
GAME_RECORD_RNG_CHECKPOINT seq=1005 turn=0 reason=SAVE_LOADED rngFingerprintSchema=1 mapSessionStartState=1378213624 mapIntervalStartState=1378213624 mapState=1378213624 mapIntervalCalls=0 mapSessionCalls=0 mapIntervalNullMessageCalls=0 mapSessionNullMessageCalls=0 mapIntervalExternalCalls=0 mapSessionExternalCalls=0 mapIntervalDeterministicRangeCalls=0 mapSessionDeterministicRangeCalls=0 mapIntervalSeedSets=0 mapSessionSeedSets=0 mapIntervalStreamFingerprint=FNV1A32:811C9DC5 mapSessionStreamFingerprint=FNV1A64:CBF29CE484222325 mapIntervalCallFingerprint=FNV1A32:811C9DC5 mapSessionCallFingerprint=FNV1A64:CBF29CE484222325 syncSessionStartState=909279002 syncIntervalStartState=909279002 syncState=909279002 syncIntervalCalls=0 syncSessionCalls=0 syncIntervalNullMessageCalls=0 syncSessionNullMessageCalls=0 syncIntervalExternalCalls=0 syncSessionExternalCalls=0 syncIntervalDeterministicRangeCalls=0 syncSessionDeterministicRangeCalls=0 syncIntervalSeedSets=0 syncSessionSeedSets=0 syncIntervalStreamFingerprint=FNV1A32:811C9DC5 syncSessionStreamFingerprint=FNV1A64:CBF29CE484222325 syncIntervalCallFingerprint=FNV1A32:811C9DC5 syncSessionCallFingerprint=FNV1A64:CBF29CE484222325
GAME_RECORD_STATE_CHECKPOINT seq=1006 turn=0 reason=SAVE_LOADED coverage=CORE everAliveTeamCount=17 everAlivePlayerCount=17 cityCount=0 unitCount=42 groupCount=42 plotCount=10080 dealCount=0 gameFingerprint=FNV1A64:42C52DAC031376BE teamsFingerprint=FNV1A64:9930DDDBA105F78D playersFingerprint=FNV1A64:DFEA4FECF846B4A2 citiesFingerprint=FNV1A64:CBF29CE484222325 unitsFingerprint=FNV1A64:2609F54E25323597 groupsFingerprint=FNV1A64:150853F5D03277E4 plotsFingerprint=FNV1A64:247E00FA3931F2B9 dealsFingerprint=FNV1A64:CBF29CE484222325 combinedFingerprint=FNV1A64:45EFBFA446592383 computeMilliseconds=1
GAME_RECORD_ACTION seq=1007 turn=0 type=DEBUG_MODE_CHANGED old=0 new=1 activePlayer=0
```

#### SASGameRecord captured example

These are **30 consecutive, unchanged rows** from `SASGameRecord_20261019T160718Z_load1.log`, lines 11678–11707 (recorder revision 159). They follow the Ottoman player at turn 60 through resources, AI priorities, economy/production, diplomacy and unit composition; this is an excerpt of existing output, not a Before/After comparison with an inherited recorder.

```text
GAME_RECORD_PLAYER seq=10544 turn=60 player=15 team=15 civ=CIVILIZATION_OTTOMAN leader=LEADER_MEHMED isHuman=0 humanSlot=0 currentlyHumanControlled=0 autoplayControlled=0 rank=5 deltaValid=1 score=141 scoreDelta=+31 cities=2 citiesDelta=+1 pop=9 popDelta=+3 land=34 landDelta=+9 units=7 unitsDelta=+1 combatUnits=5 combatUnitsDelta=+1 militarySupportUnits=5 militarySupportUnitsDelta=+1 power=50 powerDelta=+14 gold=33 goldDelta=-9 gpt=-1 gptDelta=-3 financialStrike=0 cumulativeStrikeTurns=0 researchRate=32 researchRateDelta=+13 researchPercent=100 currentResearch=- researchOverflow=19 noResearchAvailable=0 researchTurns=-1 era=ERA_CLASSICAL stateReligion=- techScorePercent=100 combatXP=0 greatPeopleCreated=0 greatGeneralsCreated=0 greatGeneralThreshold=24 goldenAgeTurns=0 loggedGoldenAgeTurns=0 anarchyTurns=0 loggedAnarchyTurns=0 revolutionTimer=0 conversionTimer=0 wars=-
GAME_RECORD_PLAYER_HISTORY seq=10545 turn=60 player=15 deltaValid=1 historyScore=141 historyScoreDelta=+31 historyEconomy=31 historyEconomyDelta=+10 historyIndustry=7 historyIndustryDelta=+0 historyAgriculture=25 historyAgricultureDelta=+11 historyPower=44 historyPowerDelta=+13 historyCulture=117 historyCultureDelta=+20 historyEspionage=233 historyEspionageDelta=+40
GAME_RECORD_POLLUTION seq=10546 turn=60 player=15 total=90 buildings=0 bonuses=0 power=0 population=90
GAME_RECORD_BONUSES seq=10547 turn=60 player=15 deltaValid=1 bonusTypes=4 bonusTypesDelta=+1 bonusInstances=5 bonusInstancesDelta=+2 imports=0 importsDelta=+0 exports=0 exportsDelta=+0
GAME_RECORD_BONUSES_AVAILABLE seq=10548 turn=60 player=15 available=BONUS_MOLLUSCS:1,BONUS_RICE:1,BONUS_GEMSTONES:1,BONUS_SILK:2
GAME_RECORD_BONUSES_TRADEABLE seq=10549 turn=60 player=15 tradeable=BONUS_MOLLUSCS:1,BONUS_RICE:1,BONUS_GEMSTONES:1,BONUS_SILK:2
GAME_RECORD_BONUSES_IMPORT_EXPORT seq=10550 turn=60 player=15 imported=- exported=-
GAME_RECORD_AI_STRATEGIES seq=10551 turn=60 player=15 team=15 strategies=-
GAME_RECORD_AI_TARGET_CITIES seq=10552 turn=60 player=15 team=15 timer=0 targetCount=0 targets=-
GAME_RECORD_AI_VICTORY_STAGES seq=10553 turn=60 player=15 team=15 playerMaxStage=1 teamMaxStage=1 culture=0 space=0 conquest=0 domination=1 diplomacy=0
GAME_RECORD_AI_MILITARY_PRODUCTION seq=10554 turn=60 player=15 personalityBuildProb=52 unitSpending=0 maxUnitSpendingNoArea=107 spendingGapNoArea=107 aggressiveAI=1 financialTrouble=0 economyFocus=0 getBetterUnits=0 focusWar=0 dagger=0 crush=0 alert1=0 alert2=0 turtle=0 lastStand=0 finalWar=0 fastMovers=0 landBlitz=0 airBlitz=0 nuclear=0 totalWarPlans=0 preparingTotalWarPlans=0 sneakPreparing=0 sneakReady=0
GAME_RECORD_POLICIES seq=10555 turn=60 player=15 civics=CIVICOPTION_GOVERNMENT:CIVIC_DESPOTISM,CIVICOPTION_LEGAL:CIVIC_BARBARISM,CIVICOPTION_LABOR:CIVIC_TRIBALISM,CIVICOPTION_ECONOMY:CIVIC_DECENTRALIZATION,CIVICOPTION_RELIGION:CIVIC_PREHISTORIC_RELIGION stateReligion=- cityReligions=- cityCorporations=- playerExtraHealth=0 playerExtraHappiness=0 extraHealthSources=- extraHappinessSources=-
GAME_RECORD_ECONOMY seq=10556 turn=60 player=15 gold=33 goldRate=-1 goldCommerceRate=0 dealGoldPerTurn=0 inflatedCosts=1 unitCost=0 unitSupply=0 cityMaintenance=1 civicUpkeep=0 inflationRate=0 totalCommerce=31 sliders=COMMERCE_GOLD:0,COMMERCE_RESEARCH:100,COMMERCE_CULTURE:0,COMMERCE_ESPIONAGE:0 commerceTypeRates=COMMERCE_GOLD:0,COMMERCE_RESEARCH:32,COMMERCE_CULTURE:2,COMMERCE_ESPIONAGE:4 flexible=COMMERCE_GOLD:1,COMMERCE_RESEARCH:1,COMMERCE_CULTURE:0,COMMERCE_ESPIONAGE:0 currentResearch=- currentResearchTeamProgress=-1 currentResearchCost=-1 researchRate=32 researchOverflow=19 noResearchAvailable=0 researchTurns=-1
GAME_RECORD_PRODUCTION_PIPELINE seq=10557 turn=60 player=15 activeFiniteItems=2 activeStored=35 activeNeeded=90 activeProcesses=0 activeFoodProductionUnits=1 activeFoodProductionUnitStored=28 parkedItems=1 parkedStored=2 parkedNeeded=45 citiesWithParked=1 maxParkedItemsOneCity=1 maxParkedStoredOneCity=2 parkedHalfComplete=0 parkedThreeQuarterComplete=0 parkedUnitItems=1 parkedUnitStored=2 parkedFoodProductionUnitItems=0 parkedFoodProductionUnitStored=0 parkedBuildingItems=0 parkedBuildingStored=0 parkedWonderItems=0 parkedWonderStored=0 parkedProjectItems=0 parkedProjectStored=0 inactivityCounterItems=1 accumulatedInactiveTurnsTotal=1 maxAccumulatedInactiveTurns=1
GAME_RECORD_PRODUCTION_PARKED seq=10558 turn=60 player=15 items=16385:UNIT:UNIT_CATAPULT:2/45@1
GAME_RECORD_STATISTICS seq=10559 turn=60 player=15 currentCities=2 persistentCitiesBuilt=2 persistentCitiesRazed=0 loggedCitiesAcquired=0 loggedCitiesLost=0 loggedCitiesConquered=0 loggedCitiesLostByConquest=0 loggedCitiesTradedIn=0 loggedCitiesTradedOut=0 loggedCityNet=+0 loggedBattleWins=0 loggedBattleLosses=0 loggedCityBattleWins=0 loggedCityBattleLosses=0 loggedBattleNet=+0 loggedWithdrawals=0 loggedEnemyWithdrawals=0 loggedCombatLimitAttacks=0 loggedCombatLimitDefenses=0 loggedLuckEligibleBattles=0 loggedLuckEligibleWins=0 loggedExpectedWinsX1000=0 loggedLuckDeltaX1000=+0 loggedUpsetWins=0 loggedUpsetLosses=0 loggedLowestOddsWinPermille=-1 loggedHighestOddsLossPermille=-1 loggedXpGained=0 loggedCombatXpGained=0 loggedNonCombatXpGained=0 loggedXpPreventedByCap=0 loggedXpLostAdjustments=0 loggedPromotionsChosen=0 loggedLeaderPromotionApplications=0 loggedEnemyXpDestroyed=0 loggedOwnXpLost=0
GAME_RECORD_ESPIONAGE seq=10560 turn=60 player=15 team=15 espionageRate=4 espionagePercent=0 teamEP=244 unspentEP=188 weights=- spending=6:2,11:2 pointsAgainst=6:97,11:91 modifiers=0:+105,1:+100,2:+100,3:+99,4:+99,5:+100,6:+100,7:+97,8:+109,9:+100,10:+100,11:+99,12:+100,13:+108,14:+108 bigEspionage=0 espionageEconomy=0 spies=0 greatSpies=0 spiesInForeignTerritory=0 spiesInForeignCities=0 stationarySpies=0 maxFortifyTurns=0 spyTargets=- attackSpyIntent=0 spyMissionTargets=- reconSpyIntent=0 guardSpyIntent=0
GAME_RECORD_ESPIONAGE_DELTAS seq=10561 turn=60 player=15 deltaValid=1 espionageRateDelta=+0 espionagePercentDelta=+0 teamEPDelta=+40 unspentEPDelta=+40
GAME_RECORD_DEMOGRAPHICS seq=10562 turn=60 player=15 rank=5 score=141 population=9 land=34 food=27 production=7 commerce=31 research=32 culture=2 espionage=4 goldRate=-1 power=50
GAME_RECORD_DEMOGRAPHICS_DELTAS seq=10563 turn=60 player=15 deltaValid=1 scoreDelta=+31 populationDelta=+3 landDelta=+9 foodDelta=+12 productionDelta=-1 commerceDelta=+11 researchDelta=+13 cultureDelta=+0 espionageDelta=+0 goldRateDelta=-3 powerDelta=+14
GAME_RECORD_ATTITUDES seq=10564 turn=60 player=15 towardValues=6:+2,11:+0
GAME_RECORD_DIPLO_ATTITUDE_BREAKDOWN seq=10565 turn=60 player=15 toward=6 componentSum=+2 componentValue=+2 cachedRawValue=+2 effectiveValue=+2 forcedDelta=+0 components=firstImpression=+2
GAME_RECORD_DIPLO_MEMORIES seq=10566 turn=60 player=15 toward=6 attitudeValue=+2 memoryAttitude=+0 memories=MEMORY_TRADED_TECH_TO_US=2/+0,MEMORY_RECEIVED_TECH_FROM_ANY=2/+0
GAME_RECORD_DIPLO_ATTITUDE_BREAKDOWN seq=10567 turn=60 player=15 toward=11 componentSum=+0 componentValue=+0 cachedRawValue=+0 effectiveValue=+0 forcedDelta=+0 components=-
GAME_RECORD_DIPLO_MEMORIES seq=10568 turn=60 player=15 toward=11 attitudeValue=+0 memoryAttitude=+0 memories=MEMORY_TRADED_TECH_TO_US=1/+0
GAME_RECORD_DIPLO_STATUS seq=10569 turn=60 player=15 team=15 worstEnemyTeam=-1 worstEnemyPlayers=- worstEnemyOfTeams=- atWar=- openBorders=11 defensivePacts=- forcePeace=- canContact=6,11 canContactWilling=6,11 wontTalkTo=- wontTalkFrom=-
GAME_RECORD_UNIT_POSTURE seq=10570 turn=60 player=15 total=7 military=5 landMilitary=5 seaMilitary=0 airMilitary=0 attackAir=0 defenseAir=0 carrierAir=0 missileAir=0 icbm=0 carrierSea=0 missileCarrierSea=0 airCargo=0 carrierAirCargo=0 missileCargo=0 nukes=0 blockadingUnits=0 workers=2 settlers=0 recon=1 cityDefenders=4 fieldArmy=1 ownTerritory=6 enemyTerritory=0 neutralTerritory=1 unitsInCities=4 enemyUnitsInTerritory=0 totalXP=0 avgXpX100=0 maxXP=0 promotionReady=0 level2Plus=0 level4Plus=0 level6Plus=0 promotionInstances=4 militaryXP=0 avgMilitaryXpX100=0 maxMilitaryXP=0 avgMilitaryLevelX100=100 maxMilitaryLevel=1 militaryPromotionReady=0 greatGeneralLedMilitary=0 militaryCostedUnits=5 militaryXmlProductionCost=163 militaryProductionNeeded=146 woundedMilitary=0 militaryHealthMeasured=5 avgMilitaryHealthX100=10000 minMilitaryHealthX100=10000 maxMilitaryHealthX100=10000 militaryHealthFull=5 militaryHealthHigh=0 militaryHealthMedium=0 militaryHealthLow=0 militaryPromotionInstances=4
GAME_RECORD_UNIT_POSTURE_DELTAS seq=10571 turn=60 player=15 deltaValid=1 totalDelta=+1 militaryDelta=+1 workersDelta=+1 settlersDelta=-1 fieldArmyDelta=-1 cityDefendersDelta=+2 enemyUnitsInTerritoryDelta=+0 totalXPDelta=+0 promotionReadyDelta=+0
GAME_RECORD_UNIT_COMPOSITION seq=10572 turn=60 player=15 unitTypes=UNIT_WORKER:2,UNIT_WARRIOR:1,UNIT_ARCHER:1,UNIT_LONGBOWMAN:3 unitAI=UNITAI_WORKER:2,UNITAI_ATTACK:1,UNITAI_CITY_DEFENSE:3,UNITAI_EXPLORE:1 unitCombatTotal=5 unitCombat=UNITCOMBAT_ARCHER_BOW_SHORT:1,UNITCOMBAT_ARCHER_BOW_LONG:3,UNITCOMBAT_MELEE_SHOCK:1 unitCombatPercentX100=UNITCOMBAT_ARCHER_BOW_SHORT:2000,UNITCOMBAT_ARCHER_BOW_LONG:6000,UNITCOMBAT_MELEE_SHOCK:2000
GAME_RECORD_NAVAL_ASSAULT_POSTURE seq=10573 turn=60 player=15 assaultTransports=0 assaultTransportsTraining=0 cargoCapacity=0 loadedCargo=0 cargoUtilizationPercentX100=-1 openOceanTransports=0 openOceanCapacity=0 openOceanLoadedCargo=0 openOceanGroups=0 transportsEmpty=0 transportsPartial=0 transportsFull=0 damagedTransports=0 assaultGroups=0 groupsEmpty=0 groupsPartial=0 groupsFull=0 groupsAtBase=0 groupsEmptyAtBase=0 groupsEmptyNoMissionAIAtBase=0 groupsLoadedAwayFromBase=0 groupsNoMissionAI=0 groupsAssault=0 groupsPickup=0 groupsReinforce=0 groupsOtherMissionAI=0 groupsMissionQueueNonempty=0 groupsHealing=0 groupsWithSeaCombatSupport=0 groupedSeaCombatSupportUnits=0 loadedCargoCanAttack=0 loadedCargoCannotAttack=0 transportUnitTypes=- cargoUnitTypes=- cargoUnitAI=- missionTargets=-
```

For the full range of record families, including initial map context, city details and gameplay history, see the [full SASGameRecord example](examples/sasgamerecord/SASGameRecord_example.log). That separate capture retains its own date and revision; it is not the source of this excerpt.

### BBAI

#### BBAI session header example

For example, this save-load header was captured during the iterable-category-registry development test on October 9, 2026, before that change was committed.

Its source identity and Debug-opt DLL describe that test build; they are not the current shipped Release DLL. The settings row includes disabled categories explicitly so zero levels can be distinguished from missing categories:

```log
BBAI_SAVE_LOADED processUtc=20261009T160524Z utc=20261009T160535Z logFile="BBAI_20261009T160535Z_load1.log" turn=0 elapsed=0 year=-50000 scenario=0 activePlayer=0 activeCivilization=CIVILIZATION_INCA activeHandicap=HANDICAP_MONARCH playersDefined=16 playersAlive=16 playersEverAlive=16 humans=1
BBAI_GAME_SETTINGS mapScript=Archipelago map=120x84 landHeavy=0 navalHeavy=1 world=WORLDSIZE_HUGE climate=CLIMATE_TEMPERATE seaLevel=SEALEVEL_MEDIUM gameSpeed=GAMESPEED_NORMAL startEra=ERA_ANCIENT gameHandicap=HANDICAP_MONARCH options=GAMEOPTION_AGGRESSIVE_AI,GAMEOPTION_NO_EVENTS
BBAI_GAME_RNG mapRandState=1378213624 syncRandState=909279002
BBAI_MOD_CONTEXT displayName="AdvCiv-SAS" folderName="AdvCiv-SAS" modPath="Mods\\AdvCiv-SAS\\"
BBAI_SOURCE_CONTEXT version="6606" commit="39e40d78917c42d67dfa9c87a1fc92da178ad7dd" shortCommit="39e40d7891" branch="main" commitDate="2026-10-09T17:28:00+02:00" metadataSource="git" dirty=1 dirtyTrackedCount=6 trackedCodeRulesDiffBytes=16408 trackedCodeRulesDiffFingerprint="FNV1A64:F3AD9234EBE21773" dirtyFiles="[MM] .github/workflows/build/xml/sas_define_usage.py; [MM] .github/workflows/tests/test_sas_define_usage.py; [ M] Assets/CvGameCoreDLL.dll; [ M] Assets/XML/GlobalDefines_advciv_sas.xml; [MM] CvGameCoreDLL/BBAILog.cpp; [ M] _1_AdvCiv-SAS/Docs/README_Main_Changes_Guide.md"
BBAI_DLL_CONTEXT build=Debug-opt moduleFound=1 fileReadable=1 fileSizeBytes=13774848 dllFingerprint=FNV1A64:97635FA4B4F6CE96 dllLastWriteUtc=20261009T160322.407Z peTimestampRaw=1791561793 peTimestampUtc=20261009T160313Z fassertEnabled=0 debugDefine=0 ndebugDefine=1
BBAI_LOG_SETTINGS SAS_BBAI_LOG_ENABLE=1 SAS_BBAI_LOG_USE_TIMESTAMPED_FILENAME=1 SAS_BBAI_PLAYER_LOG_LEVEL=2 SAS_BBAI_TEAM_LOG_LEVEL=2 SAS_BBAI_WAR_LOG_LEVEL=0 SAS_BBAI_CITY_LOG_LEVEL=0 SAS_BBAI_PRODUCTION_NO_TARGET_LOG_LEVEL=0 SAS_BBAI_MILITARY_PRODUCTION_LOG_LEVEL=0 SAS_BBAI_SPACE_PRODUCTION_LOG_LEVEL=0 SAS_BBAI_LIMITED_PROJECT_PRODUCTION_LOG_LEVEL=0 SAS_BBAI_BUILDING_PRODUCTION_LOG_LEVEL=0 SAS_BBAI_CITIZEN_LOG_LEVEL=0 SAS_BBAI_UNIT_LOG_LEVEL=0 SAS_BBAI_OVERSEAS_TRANSPORT_LOG_LEVEL=0 SAS_BBAI_GREAT_GENERAL_LOG_LEVEL=0 SAS_BBAI_SETTLER_LOG_LEVEL=0 SAS_BBAI_FOUND_LOG_LEVEL=0 SAS_BBAI_EVACUATION_LOG_LEVEL=0 SAS_BBAI_WORKER_LOG_LEVEL=0 SAS_BBAI_WORKER_SEA_LOG_LEVEL=0 SAS_BBAI_MAP_LOG_LEVEL=0 SAS_BBAI_DEAL_CANCEL_LOG_LEVEL=0 SAS_BBAI_BONUS_LOG_LEVEL=0 SAS_BBAI_CULTURE_LOG_LEVEL=0 SAS_BBAI_UWAI_AGENT_LOG_LEVEL=0 SAS_BBAI_UWAI_MILITARY_ANALYST_LOG_LEVEL=0 SAS_BBAI_UWAI_INVASION_GRAPH_LOG_LEVEL=0 SAS_BBAI_UWAI_ARMAMENT_FORECAST_LOG_LEVEL=0 SAS_BBAI_UWAI_WAR_UTILITY_LOG_LEVEL=0 SAS_BBAI_SCORE_LOG_INTERVAL_TURNS_UNSCALED_GAMESPEED=100
```

#### Categories and inherited Settler logging

Base AdvCiv's `BBAILog.h` used compile-time `LOG_AI` / `LOG_FOUND_VALUE` switches and fixed category levels. Its `logBBAI` wrote `BBAI.log`, or `BBAI<player>.log` in network multiplayer. SAS exposes the settings through XML and caches them after define loading, so diagnostic levels can be changed before launch without rebuilding the DLL.

Enable `SAS_BBAI_LOG_ENABLE` and select the relevant category levels. `SAS_BBAI_LOG_USE_TIMESTAMPED_FILENAME` chooses new/load session files instead of fixed-name appending; ordinary diagnostic categories ship disabled. The XML comments describe exact levels and supporting controls; the table below is a subject index, not a claim that every category emits the same detail at each level.

These controls all select categories within the same BBAI output, including the UWAI controls below; they do not create separate files such as `Settler.log` or `MilitaryAnalyst.log`.

| Current control | Diagnostic subject / search examples |
| --- | --- |
| `SAS_BBAI_PLAYER_LOG_LEVEL`, `SAS_BBAI_TEAM_LOG_LEVEL`, `SAS_BBAI_WAR_LOG_LEVEL` | General player/team decisions and dedicated war-plan/target decisions. |
| `SAS_BBAI_CITY_LOG_LEVEL`, `SAS_BBAI_PRODUCTION_NO_TARGET_LOG_LEVEL` | General city decisions and missing-production-target/chooser-path diagnostics. |
| `SAS_BBAI_MILITARY_PRODUCTION_LOG_LEVEL`, `SAS_BBAI_BUILDING_PRODUCTION_LOG_LEVEL` | Military-unit versus infrastructure opportunities and production decisions. |
| `SAS_BBAI_SPACE_PRODUCTION_LOG_LEVEL`, `SAS_BBAI_LIMITED_PROJECT_PRODUCTION_LOG_LEVEL` | Spaceship projects and one-copy non-spaceship projects. |
| `SAS_BBAI_CITIZEN_LOG_LEVEL`, `SAS_BBAI_CULTURE_LOG_LEVEL` | Citizen assignments and culture-victory decisions. |
| `SAS_BBAI_UNIT_LOG_LEVEL`, `SAS_BBAI_OVERSEAS_TRANSPORT_LOG_LEVEL`, `SAS_BBAI_GREAT_GENERAL_LOG_LEVEL` | General unit behavior, transport/cargo decisions and Great General actions. |
| `SAS_BBAI_SETTLER_LOG_LEVEL` | Movement, target decisions, founding, parking and escorts; examples include `SETTLER_MISSION_DECISION`, `SETTLER_PARKED` and `SETTLER_CITY_ESCORT_POOL`. |
| `SAS_BBAI_FOUND_LOG_LEVEL` | City-site scoring and selection; examples include `FOUND_SITE_SELECTED` and `FOUND_SITE_STORED`. |
| `SAS_BBAI_EVACUATION_LOG_LEVEL`, `SAS_BBAI_WORKER_LOG_LEVEL`, `SAS_BBAI_WORKER_SEA_LOG_LEVEL` | Retreat/evacuation, land Workers and Work Boats. |
| `SAS_BBAI_MAP_LOG_LEVEL`, `SAS_BBAI_DEAL_CANCEL_LOG_LEVEL`, `SAS_BBAI_BONUS_LOG_LEVEL` | Map/start context, deal cancellation and resource valuation/trade. |

The inherited `CvUnitAI::AI_found` / `AI_foundFollow` path already logged messages such as `Settler heading for site %d, %d` and `Settler founding at plot %d, %d (follow)` under `gUnitLogLevel >= 2`. SAS separates Settler diagnostics from general UNIT output and adds named fields for decisions, reasons and safety/escort context; FOUND separately follows the scoring used to choose sites.

`ReproTest.cpp` is another BBAI/message-control producer rather than a separate filename family: when compiled with `ENABLE_REPRO_TEST`, it brackets the replay comparison with `ReproTest: start`, `ReproTest: reloading` and `ReproTest: done` in enabled BBAI output, and also uses `messageControlLog` when engine logging is active.

Those quoted Settler strings are source format templates, not captured game rows. The split matters in practice: enabling FOUND alone does not request the whole Settler movement/escort trace, and enabling SETTLER does not request full per-plot found-value calculations.

#### Settler Before/After: selecting a movement target

This comparison reconstructs the **source formats**, not a recorded Settler trip: the checked captures did not supply a matching movement row. Both sides describe moving toward a selected founding site; `%d` / `%S` retain the actual source placeholders instead of inventing a unit, turn or coordinates.

Before (`CvUnitAI::AI_found`, base AdvCiv; general UNIT level 2+):

```text
    Settler heading for site %d, %d
```

After (SAS `SAS_logSettlerMissionDecision`, SETTLER level 2+; action/reason resolved from the `AI_found` call):

```text
SETTLER_MISSION_DECISION turn=%d player=%d %S action=PUSH_MOVE_TO_FOUND reason=AI_FOUND_MOVE unitId=%d x=%d y=%d target=(%d,%d) endTurn=(%d,%d) targetOwner=%d targetFoundValue=%d pathTurns=%d groupId=%d groupUnits=%d groupSettlers=%d groupDefenders=%d groupHealthyDefenders=%d groupCanDefend=%d plotDanger=%d targetDanger=%d missionAI=%d
```

The old line identifies only the destination. The SAS row also identifies the player/unit, turn, current and end-turn positions, selected-site value, path length, group/escort composition, danger and mission context, so it can explain how the Settler is reaching that target. Separate `SETTLER_PARKED` and `SETTLER_CITY_ESCORT_POOL` rows expose waiting and escort availability.

#### UWAI diagnostics: standalone AdvCiv reports vs current BBAI records

Base AdvCiv's `UWAIReport.cpp` wrote `uwai<turn>.log` through the EXE logger, with Textile-oriented headings and a conversion note. `UWAIAgent.cpp` selected report turns through `REPORT_INTERVAL` or a forced report, while utility thresholds selected additional detailed evaluation output; `DISABLE_UWAI_REPORT` was a compile-time suppression option.

SAS merged these formerly independent UWAI diagnostics into BBAI and retired `UWAIReport.cpp` / `UWAIReport.h`, so war analysis uses the same session files, XML category controls and live-readable sink as other AI decision traces. Caller gates also avoid the disabled-report argument/setup work described in KI#505.3.

The war-planning subsystems now write directly to BBAI under the following five controls; these select portions of BBAI output, not separate `MilitaryAnalyst.log`, `InvasionGraph.log` or other per-class files:

| Current control | Explicit source / responsibility | Search examples in BBAI |
| --- | --- | --- |
| `SAS_BBAI_UWAI_AGENT_LOG_LEVEL` | `UWAIAgent.cpp`: top-level team war planning and negotiation flow. | `UWAI_AGENT_` |
| `SAS_BBAI_UWAI_MILITARY_ANALYST_LOG_LEVEL` | `MilitaryAnalyst.cpp`: scenario participants, phases and projected war/capitulation/elimination outcomes. | `UWAI_MILITARY_ANALYSIS_` |
| `SAS_BBAI_UWAI_INVASION_GRAPH_LOG_LEVEL` | `InvasionGraph.cpp`: invasion targets, deployment, clashes and conquest simulation. | `UWAI_INVASION_` |
| `SAS_BBAI_UWAI_ARMAMENT_FORECAST_LOG_LEVEL` | `ArmamentForecast.cpp`: predicted military production, upgrades and power increases. | `UWAI_ARMAMENT_` |
| `SAS_BBAI_UWAI_WAR_UTILITY_LOG_LEVEL` | `WarEvaluator.cpp` and `WarUtilityAspect.cpp`: war/peace scenario totals and individual utility contributions. | `UWAI_WAR_` |

Level 1 selects important outcomes, with higher levels exposing more calculation context where implemented; the Agent and Military Analyst currently have the same detail at levels 2 and 3. Caller gates skip disabled diagnostic argument/setup work; shared nested mute state keeps selected internal hypothetical passes silent without changing gameplay calculations.

See [KI#505.3](README_Known_Issues.md#ki-505.3) for the investigation and conversion rationale, and the [Main Changes Guide's BBAI section](README_Main_Changes_Guide.md#bbai-log) for diagnostic behavior and controls. Historical `uwai<turn>.log` files belong to the old report path; absence of those files in current SAS is expected.

##### UWAI Before/After: separate prose report to shared BBAI records

**After** consists of six real rows selected in order from `BBAI_20261018T080928Z_load1.log`, lines 29319–29339; intervening rows are omitted. The surrounding captured team-member row identifies player 2 as Lincoln.

**Before** reconstructs the corresponding base AdvCiv `MilitaryAnalyst.cpp`, `InvasionGraph.cpp` and `ArmamentForecast.cpp` message formats using the same captured name/values.

This is a layout comparison, not a claim that base AdvCiv replayed this SAS game; its default hardcoded `bLOG_AI=false` also muted AI-player armament forecasts, so those old-style rows are shown as they would read when enabled.

Before (`uwai21.log`, reconstructed old formats):

```text
Military analysis from the pov of Lincoln
Constructing invasion graph
(no civs are currently at war)
Armament forecast for *Lincoln*
Production per turn: 3
Total production for build-up: 5 hammers
```

After (captured SAS BBAI rows):

```text
UWAI_MILITARY_ANALYSIS_BEGIN turn=21 agentPlayer=2 agentTeam=2 targetTeam=13 scenario=WAR
UWAI_INVASION_GRAPH_BEGIN turn=21 agentPlayer=2 scenario=WAR warPartyCount=0
UWAI_INVASION_GRAPH_NO_WAR_PARTIES turn=21 agentPlayer=2 scenario=WAR
UWAI_ARMAMENT_FORECAST_BEGIN turn=21 analystPlayer=2 player=2 targetTeam=13 scenario=WAR horizon=4
UWAI_ARMAMENT_FORECAST_PRODUCTION_INPUTS turn=21 player=2 baseHammersPerTurn=3 productionPortionPercent=100 effectiveHammersPerTurn=3 upgradeHammers=0 upgradingDisabled=0
UWAI_ARMAMENT_FORECAST_TOTAL_PRODUCTION turn=21 player=2 hammers=5
```

The SAS rows retain the analysis/forecast values and make turn, analyst, player, target team and war/peace scenario explicit at the relevant boundary. They can be searched alongside other BBAI categories without opening another per-turn report or converting Textile; more detailed context remains controlled by each subsystem's XML level.

## Runtime observations and DLL build reports

### Runtime status and history

`SASRuntimeStatus.json` and `SASRuntimeHistory_*.jsonl` complement gameplay logs: the fixed-name JSON gives the latest observation, while the timestamped JSONL preserves transitions across games/loads in a process. Neither replaces the detailed BBAI/SGR history; see the [runtime examples](examples/runtime_status/README.md) and [runtime revision history](revisions/SASRuntime.md).

### DLL build reports

`CvGameCoreDLL/Project/<configuration>/AdvCiv.log` carries compiler/linker messages, and `CvGameCoreDLL.dll.build.json` reports build progress/result, Git/source context, warnings/errors and binary identity. These help distinguish the source being reviewed from the DLL actually built or loaded.

SAS enriches the existing `AdvCiv.log` rather than replacing its compiler output: the reporting wrapper adds UTC start/end times, configuration/request and Git context, temporary-artifact state, source/project input-change summary counts, final result/exit code/duration and distinct coded warning/error counts.

These build-level summaries help explain what was compiled and how it finished; ordinary compiler lines keep their existing format.

The companion JSON adds structured detail, including source/project manifests and changes, recognized diagnostic lines and before/after DLL identity. Thus the text log remains readable compiler evidence while the report supports automated checks and comparisons; the JSON is an additional SAS output, not a pre-existing engine log.

For example, these five rows come from the [captured Release compiler log](examples/build_reports/AdvCiv_Release_example.log), dated 2026-10-12 (IDE indentation omitted, intervening rows omitted; `<MOD_ROOT>` is the example's existing path substitution):

```text
Started UTC: 2026-10-12T21:24:06.628Z
Configuration: Release; request: BUILD; branch: main; commit: -
Ended UTC: 2026-10-12T21:25:52.293Z; result: SUCCEEDED; exit code: 0; duration milliseconds: 105657
Distinct coded diagnostic lines: warnings=0; errors=0
Build details (JSON): <MOD_ROOT>\CvGameCoreDLL\Project\Release\CvGameCoreDLL.dll.build.json
```

The build pair describes a development process; the runtime JSON/JSONL describes observations from Civ4. They share the goal of identifying ongoing activity and its result, but have different producers and lifetimes.

- The [root README's build-report overview](../../README.md#dll-build-reports) introduces their role alongside other development tools.
- The [DLL Compilation Guide](Modding_Ressources/README_DLL_Compilation.md#persistent-build-reports) explains generation, fields, replacement on the next attempt and shipping requirements.
- Complete Release examples: [build JSON](examples/build_reports/AdvCiv_Release_example.build.json) and [compiler log](examples/build_reports/AdvCiv_Release_example.log). The guide also links [Debug-opt examples](Modding_Ressources/README_DLL_Compilation.md#example-build-output).

## Python diagnostic messages

`SASPythonLogging` wraps Python stdout/stderr and supplies the same row formatter to Civ4's `CvUtil.RedirectDebug` and `RedirectError` classes. Direct prints, BUG file messages and multiline traceback text therefore share one timestamp path after installation.

Each nonempty Python-written row receives a UTC date/time with milliseconds. Fragmented writes retain line-start state, and each write is forwarded immediately; no partial row is held until its newline. Blank rows remain blank. BUG's timestamp option controls the shared formatter, while its existing message-level filtering and on-screen messages remain unchanged.

The timestamp comes from the DLL's Windows UTC sampler. The Python layer requires a DLL exposing `getSASDiagnosticUtcTimestamp`; disabling timestamps avoids sampling the clock. The leader-cache serializer uses an explicit raw-output path so its generated Python module remains copyable.

Timestamp work occurs when Python writes output; there is no frame timer or background polling. BUG-filtered messages never reach the stream, and disabling timestamps skips clock sampling while retaining stream processing.

The wrapper does not gate on the INI's `LoggingEnabled` switch. If Python still emits text while the engine suppresses file output, formatting work can still occur; the logging-disabled case has not been measured here.

EXE output produced before installation or written directly to its files bypasses these streams. In particular, the Python startup preamble and early `PythonErr2.log` warnings cannot be assumed to pass through the Python wrappers.

## Startup rows and run boundaries

Rows emitted before the stream wrapper is installed remain in the original log, with their text unchanged. Their exact per-row UTC times cannot be reconstructed from a later snapshot. Preserving a copy retains this evidence, but does not recover those missing timestamps.

This is the boundary of the current Python interception, not a demonstrated impossibility of native interception. Capturing EXE-written startup or engine-log rows at emission would require a separate integration approach; its feasibility and cost were not established by these Python/DLL writer changes.

When runtime recording is enabled, `SASRuntimeStatus.json` identifies the process through `processId` and `processStartUtc`, and the current new/load session through `sessionSerial`, `sessionKind` and `sessionStartedUtc`. `nativeReadyUtc` distinguishes session initialization from the game becoming ready.

The companion `SASRuntimeHistory_*.jsonl` preserves dated transitions, including completed activity intervals and reset/exit observations. A clean shutdown publishes `PROCESS_EXIT` with `EXITED_CLEANLY`; its observation timestamp identifies the recorded shutdown boundary.

There is no separate universal `sessionEndedUtc` field, and a crash or forced termination can leave only the last observation rather than an exact end time.

## Reading Python screen and font messages

Screen notifications retain their numeric ID and add its canonical Python constant name, such as `screenId=99 screenType=CvScreenEnums.MAIN_INTERFACE`; `13` is `REPLAY_SCREEN` and `18` is `INFO_SCREEN`.

Names are derived once from `CvScreenEnums.py`, which now also contains the IDs from the removed standalone `SevoScreenEnums.py` under class `SevoScreenEnums`, alongside the original general-screen and vanilla Civilopedia definitions.

The class keeps overlapping encyclopedia IDs distinct; the lookup selects the active encyclopedia while retaining general screen names. Unregistered IDs remain explicit.

`record=SCREEN_OFF` includes the last notified screen as context; it does not establish a verified individual screen-close callback.

Font initialization messages identify the icon role, XML type, font character code and description. For example, the formerly ambiguous `329 - judaism` and `330 - judaism` refer to the religion and holy-city glyphs, respectively, rather than religion enum IDs. The richer messages preserve the inherited icon-map behavior.

SAS-formatted diagnostics identify their kind through a `record=` field, such as `record=SCREEN_TURNED_ON`, `record=SCREEN_OFF` or `record=FONT_ICON`. Searching for `record=SCREEN_` groups screen/navigation messages; `record=WB_BRUSH_SELECTED` finds brush rows with `mode=ADVANCED_START`, `PLAYER` or `MAP`.

Other labelled families include `record=VICTORY_SCREEN_*` for vote-definition scans/tallies, `record=DIPLOMACY_*` for optional diplomacy debugging, `record=MOD_NET_MESSAGE`, `record=GAME_END_CALLBACK` and `record=SEVOPEDIA_*` initialization notices.

These labels describe existing diagnostics; game-end callbacks are distinct from process exit, and vote-definition scans are distinct from player choices.

These message refinements concern chiefly Python-emitted output in `PythonDbg.log`. Shared stream timestamps also cover Python error output; the one direct C++ Starting Position Iteration row now follows the same UTC/`record=` convention. Engine-written `app.log` and `audio.log` retain their original formats, while DLL-controlled `xml.log` rows are modernized separately as described above.

## Before and after: shared Python log timestamps

Each pair shows the same captured messages in the same order. **Before** rows reconstruct the old message formats, rather than presenting a second recorded run.

PythonDbg **After** rows reconstruct the current `record=` fields and vote labels from captured data; PythonErr/PythonErr2 After rows remain direct captures. Only formatting and diagnostic fields change, not the recorded screen visits, glyphs or timestamps.

The PythonDbg/PythonErr2 captures are from 2026-10-20 with the matching Release DLL and the in-progress Python UI-resource draft loaded. The PythonErr startup-failure excerpt comes from a later run on 2026-10-20, before correcting the visual-resource import order. Excerpts may omit intervening rows, but their order and message data are preserved.

| File | Change | Examples |
| --- | --- | --- |
| `PythonDbg.log` | UTC prefixes and named screen/glyph fields | [Before](#before-pythondbglog) / [After](#after-pythondbglog) |
| `PythonErr.log` | UTC prefixes on traceback rows | [Before](#before-pythonerrlog) / [After](#after-pythonerrlog) |
| `PythonErr2.log` | UTC prefixes on module-loading/warning rows | [Before](#before-pythonerr2log) / [After](#after-pythonerr2log) |
| `app.log` | Native EXE format; no live SAS/DLL writer found | [Current format](#applog) |
| `audio.log` | Native EXE format; no live SAS/DLL writer found | [Current format](#audiolog) |
| `init.log` | Native EXE format; no live SAS/DLL writer found | [Current format](#initlog) |
| `initmgr.log` | Native EXE format; no live SAS/DLL writer found | [Current format](#initmgrlog) |
| `resmgr.log` | Native format; no live writer, only a commented-out inherited DLL writer | [Current format](#resmgrlog) |
| `xml.log` | DLL-controlled rows now use UTC + `record=` fields while preserving the native elapsed prefix | [Before](#before-xmllog) / [After](#after-xmllog) |
| `LSystem.log` | Native EXE format; no live SAS/DLL writer found | [Current format](#lsystemlog) |

### `PythonDbg.log`

A varied 30-row excerpt covers Sevopedia, glyph initialization, game/advisor screens, module loading, a WorldBuilder save and shutdown. All 17 screen notifications from the capture are included, with repeated switches retained; these are screen notifications, not individual category/tab selections.

450 of 618 nonempty rows carry timestamps in the full capture. 165 precede wrapper installation; the three later unprefixed rows are two `import failed` messages and `UnInit Python`. The final row below illustrates this direct-output limit.

#### Before: PythonDbg.log

Reconstructed old formatting of the same 30 rows:

```log
200 SCREEN TURNED ON
SCREEN OFF
275 - gold
329 - judaism
330 - judaism
3 SCREEN TURNED ON
99 SCREEN TURNED ON
load_module BugUnitPlot
load_module CvWBInterface
load_module CvWBDesc
PY:saveDesc:C:\Users\PC\Documents\My Games\Beyond the Sword\Saves\WorldBuilder\WBQuickSave, curDir:C:\Program Files (x86)\Steam\steamapps\common\Sid Meier's Civilization IV Beyond the Sword\Beyond the Sword
WBSave done
load_module encodings.ascii
0 SCREEN TURNED ON
9 SCREEN TURNED ON
30 SCREEN TURNED ON
0 SCREEN TURNED ON
9 SCREEN TURNED ON
30 SCREEN TURNED ON
4 SCREEN TURNED ON
14 SCREEN TURNED ON
31 SCREEN TURNED ON
18 SCREEN TURNED ON
15 SCREEN TURNED ON
votesource
secgen
SCREEN OFF
30 SCREEN TURNED ON
PY:OnUnInit
UnInit Python
```

#### After: PythonDbg.log

Current formatting reconstructed from captured rows, with the same selection and order.

The rebuilt-DLL run also captured the direct C++ Starting Position row exactly as `utc=20261020T170806.389Z record=STARTING_POSITION_ITERATION phase=BEGIN source=AdvCiv`. It bypasses `SASPythonLogging`, so its compact `utc=` field is intentionally not rewritten into the Python wrapper's extended ISO prefix.

```log
2026-10-20T08:16:11.208Z record=SCREEN_TURNED_ON screenId=200 screenType=CvScreenEnums.SevoScreenEnums.PEDIA_MAIN
2026-10-20T08:16:23.371Z record=SCREEN_OFF previousScreenId=200 previousScreenType=CvScreenEnums.SevoScreenEnums.PEDIA_MAIN
2026-10-20T08:16:28.877Z record=FONT_ICON role=commerce type=COMMERCE_GOLD charCode=275 description=gold
2026-10-20T08:16:28.878Z record=FONT_ICON role=religion type=RELIGION_JUDAISM charCode=329 description=judaism
2026-10-20T08:16:28.878Z record=FONT_ICON role=holy-city type=RELIGION_JUDAISM charCode=330 description=judaism
2026-10-20T08:16:28.878Z record=SCREEN_TURNED_ON screenId=3 screenType=CvScreenEnums.TECH_CHOOSER
2026-10-20T08:16:29.114Z record=SCREEN_TURNED_ON screenId=99 screenType=CvScreenEnums.MAIN_INTERFACE
2026-10-20T08:16:29.363Z load_module BugUnitPlot
2026-10-20T08:16:30.925Z load_module CvWBInterface
2026-10-20T08:16:30.926Z load_module CvWBDesc
2026-10-20T08:16:30.932Z PY:saveDesc:C:\Users\PC\Documents\My Games\Beyond the Sword\Saves\WorldBuilder\WBQuickSave, curDir:C:\Program Files (x86)\Steam\steamapps\common\Sid Meier's Civilization IV Beyond the Sword\Beyond the Sword
2026-10-20T08:16:31.417Z WBSave done
2026-10-20T08:16:31.433Z load_module encodings.ascii
2026-10-20T08:16:34.331Z record=SCREEN_TURNED_ON screenId=0 screenType=CvScreenEnums.DOMESTIC_ADVISOR
2026-10-20T08:16:34.903Z record=SCREEN_TURNED_ON screenId=9 screenType=CvScreenEnums.POLICY_ADVISOR_SCREEN
2026-10-20T08:16:35.232Z record=SCREEN_TURNED_ON screenId=30 screenType=CvScreenEnums.FOREIGN_DIPLOMACY_ADVISOR
2026-10-20T08:16:36.142Z record=SCREEN_TURNED_ON screenId=0 screenType=CvScreenEnums.DOMESTIC_ADVISOR
2026-10-20T08:16:39.191Z record=SCREEN_TURNED_ON screenId=9 screenType=CvScreenEnums.POLICY_ADVISOR_SCREEN
2026-10-20T08:16:41.438Z record=SCREEN_TURNED_ON screenId=30 screenType=CvScreenEnums.FOREIGN_DIPLOMACY_ADVISOR
2026-10-20T08:16:45.207Z record=SCREEN_TURNED_ON screenId=4 screenType=CvScreenEnums.FOREIGN_ADVISOR
2026-10-20T08:16:47.863Z record=SCREEN_TURNED_ON screenId=14 screenType=CvScreenEnums.MILITARY_ADVISOR
2026-10-20T08:17:03.784Z record=SCREEN_TURNED_ON screenId=31 screenType=CvScreenEnums.WORLD_ADVISOR
2026-10-20T08:17:09.160Z record=SCREEN_TURNED_ON screenId=18 screenType=CvScreenEnums.INFO_SCREEN
2026-10-20T08:17:14.145Z record=SCREEN_TURNED_ON screenId=15 screenType=CvScreenEnums.VICTORY_SCREEN
2026-10-20T08:17:17.857Z record=VICTORY_SCREEN_VOTE_SOURCE_MATCH voteId=0
2026-10-20T08:17:17.857Z record=VICTORY_SCREEN_SECRETARY_GENERAL_MATCH voteId=0
2026-10-20T08:17:20.783Z record=SCREEN_OFF previousScreenId=15 previousScreenType=CvScreenEnums.VICTORY_SCREEN
2026-10-20T08:17:23.955Z record=SCREEN_TURNED_ON screenId=30 screenType=CvScreenEnums.FOREIGN_DIPLOMACY_ADVISOR
2026-10-20T08:30:49.505Z PY:OnUnInit
UnInit Python
```

### `PythonErr.log`

This real startup failure was captured on 2026-10-20 while testing the visual-resource refactor: `SASVisualUtils` imported `BugConfig` first, leading through `BugCore` to `BugOptions` before `BugConfig.Handler` existed. The sample preserves that pre-fix traceback, including its original file/line references.

Direct EXE `ERR:` messages interleaved with the timestamped traceback. This excerpt retains the first three and omits subsequent repetitions; the exception's final fragments remain separate instead of being joined into a reconstructed exception row. It illustrates that direct EXE rows bypass the Python wrapper, rather than implying every error row receives a timestamp.

#### Before: PythonErr.log

Reconstructed by removing only the UTC prefixes from the same selected rows:

```log
Traceback (most recent call last):
ERR: Call function onEvent failed. Can't find module CvEventInterface
  File "<string>", line 1, in ?
ERR: Call function onEvent failed. Can't find module CvEventInterface
ERR: Call function onEvent failed. Can't find module CvEventInterface
  File "<string>", line 52, in load_module
  File "CvEventInterface", line 16, in ?
  File "<string>", line 52, in load_module
  File "BugEventManager", line 109, in ?
  File "<string>", line 52, in load_module
  File "SASVisualUtils", line 13, in ?
  File "<string>", line 52, in load_module
  File "BugConfig", line 33, in ?
  File "<string>", line 52, in load_module
  File "BugCore", line 12, in ?
  File "<string>", line 52, in load_module
  File "BugOptions", line 1270, in ?
AttributeError
:
'module' object has no attribute 'Handler'
Failed to load python module CvEventInterface.
```

#### After: PythonErr.log

Real captured rows, in their original order:

```log
2026-10-20T13:57:18.566Z Traceback (most recent call last):
ERR: Call function onEvent failed. Can't find module CvEventInterface
2026-10-20T13:57:24.356Z   File "<string>", line 1, in ?
ERR: Call function onEvent failed. Can't find module CvEventInterface
ERR: Call function onEvent failed. Can't find module CvEventInterface
2026-10-20T13:57:25.138Z   File "<string>", line 52, in load_module
2026-10-20T13:57:25.529Z   File "CvEventInterface", line 16, in ?
2026-10-20T13:57:25.921Z   File "<string>", line 52, in load_module
2026-10-20T13:57:26.106Z   File "BugEventManager", line 109, in ?
2026-10-20T13:57:26.283Z   File "<string>", line 52, in load_module
2026-10-20T13:57:26.434Z   File "SASVisualUtils", line 13, in ?
2026-10-20T13:57:26.595Z   File "<string>", line 52, in load_module
2026-10-20T13:57:26.762Z   File "BugConfig", line 33, in ?
2026-10-20T13:57:26.920Z   File "<string>", line 52, in load_module
2026-10-20T13:57:27.098Z   File "BugCore", line 12, in ?
2026-10-20T13:57:27.257Z   File "<string>", line 52, in load_module
2026-10-20T13:57:27.433Z   File "BugOptions", line 1270, in ?
2026-10-20T13:57:27.609Z AttributeError
:
'module' object has no attribute 'Handler'
Failed to load python module CvEventInterface.
```

### `PythonErr2.log`

120 of 131 nonempty rows carry timestamps in the full 2026-10-20 capture; 11 precede wrapper installation. The matched 30-row excerpt combines the 20-row warning/module-loading slice with the final 10 rows, including `PY:OnInit`.

#### Before: PythonErr2.log

Reconstructed by removing only the UTC prefixes from the same rows:

```log
load_module BugUtil
load_module ColorUtil
load_module BugInit
load_module BugPath
load_module BugConfigTracker
load_module shutil
load_module BugTypes
load_module xmllib
..\WARLORDS\ASSETS\PYTHON\SYSTEM\xmllib.py:9: DeprecationWarning: The xmllib module is obsolete.  Use xml.sax instead.
load_module configobj
load_module __future__
load_module validate
validate import failed
load_module BugOptionsScreen
load_module BugErrorOptionsTab
load_module BugOptionsTab
load_module BugHelp
load_module Popup
load_module CityUtil
load_module ReligionUtil
load_module CvAdvisorUtils
load_module SASBattleHistory
load_module InputUtil
load_module SASBillboardScale
load_module SASFastSave
load_module AIAutoPlay
load_module ChangePlayer
load_module Tester
load_module encodings.utf_8
PY:OnInit
```

#### After: PythonErr2.log

Real captured rows:

```log
2026-10-20T08:16:03.531Z load_module BugUtil
2026-10-20T08:16:03.533Z load_module ColorUtil
2026-10-20T08:16:03.533Z load_module BugInit
2026-10-20T08:16:03.534Z load_module BugPath
2026-10-20T08:16:03.535Z load_module BugConfigTracker
2026-10-20T08:16:03.536Z load_module shutil
2026-10-20T08:16:03.544Z load_module BugTypes
2026-10-20T08:16:03.545Z load_module xmllib
2026-10-20T08:16:03.560Z ..\WARLORDS\ASSETS\PYTHON\SYSTEM\xmllib.py:9: DeprecationWarning: The xmllib module is obsolete.  Use xml.sax instead.
2026-10-20T08:16:03.564Z load_module configobj
2026-10-20T08:16:03.570Z load_module __future__
2026-10-20T08:16:03.576Z load_module validate
2026-10-20T08:16:03.582Z validate import failed
2026-10-20T08:16:03.585Z load_module BugOptionsScreen
2026-10-20T08:16:03.586Z load_module BugErrorOptionsTab
2026-10-20T08:16:03.587Z load_module BugOptionsTab
2026-10-20T08:16:03.589Z load_module BugHelp
2026-10-20T08:16:03.590Z load_module Popup
2026-10-20T08:16:03.591Z load_module CityUtil
2026-10-20T08:16:03.592Z load_module ReligionUtil
2026-10-20T08:16:03.860Z load_module CvAdvisorUtils
2026-10-20T08:16:03.865Z load_module SASBattleHistory
2026-10-20T08:16:03.868Z load_module InputUtil
2026-10-20T08:16:03.869Z load_module SASBillboardScale
2026-10-20T08:16:03.870Z load_module SASFastSave
2026-10-20T08:16:03.871Z load_module AIAutoPlay
2026-10-20T08:16:03.874Z load_module ChangePlayer
2026-10-20T08:16:03.875Z load_module Tester
2026-10-20T08:16:03.876Z load_module encodings.utf_8
2026-10-20T08:16:03.877Z PY:OnInit
```

### Engine-log captured formats and SAS comparisons

The Python stream changes did not alter the native captures for `app.log`, `audio.log`, `init.log`, `initmgr.log`, `resmgr.log` and `LSystem.log`. `xml.log` now has both the real pre-change baseline and a rebuilt-DLL After capture from 2026-10-20.

The native baselines below are from the earlier 2026-10-20 run. The `xml.log` After sample is from the rebuilt Debug-opt run beginning at `20261020T170755Z`. Longer excerpts combine selected head/tail or record-family rows as noted; shorter files are shown in full.

#### `app.log`

All 4 nonempty rows in the capture.

```log
[21962.546] FBMode = FBMODE_MULTISAMPLES_2
[21963.391] CvApp::OnActivate
[22098.547] CvApp::OnDeactivate
[22844.109] CvApp::OnActivate
```

#### `audio.log`

30 of 38 nonempty rows: the first 20 and final 10.

```log
[21958.656] Audio Manager: Loading Audio Defines file XML\Audio\AudioDefines.xml...
[21958.734] Audio Manager: Successfully loaded Audio Defines file XML\Audio\AudioDefines.xml.
[21958.734] Audio Manager: Loading 2D Scripts file XML\Audio\Audio2DScripts.xml...
[21958.796] ERR: FAudioManager::GetAudioTagIndex(): Could not find string DIPLO_PETER_SFX of TagId 0
[21958.875] Audio Manager: Successfully loaded 2D scripts file XML\Audio\Audio2DScripts.xml.
[21958.875] Audio Manager: Loading 3D Scripts file XML\Audio\Audio3DScripts.xml...
[21958.984] Audio Manager: Successfully loaded 3D scripts file XML\Audio\Audio3DScripts.xml.
[21958.984] Audio Manager: Successfully loaded Soundscape scripts file XML\Audio\AudioSoundscapeScripts.xml.
[21958.984] Audio Manager: Initializing
[21958.984] Audio Manager: Attempting to initialize the audio system.
[21958.984] Audio System: Initializing Miles Audio System...
[21958.984] Audio System: redistribution directory: ..\Miles\win32
[21958.984] Audio System: Starting Miles.
[21959.000] Audio System: Opening the sound driver.
[21959.046] Audio System: CAPS: Total HW Bytes: 0
[21959.046] Audio System: CAPS: Max Contiguous Free HW Bytes: 0
[21959.046] Audio System: CAPS: Total HW 3D Buffers: 0
[21959.046] Audio System: CAPS: Static HW 3D Buffers: 0
[21959.046] Audio System: CAPS: Streaming HW 3D Buffers: 0
[21959.046] Audio System: CAPS: Total Mixing HW 3D Buffers: 1
[21959.046] Audio System: VOICES: 2D: 54
[21959.046] Audio System: VOICES: 3D: 64
[21959.046] Audio System: Initializing 2D samples...
[21959.046] Audio System: Successfully initialized 2D samples.
[21959.046] Audio System: Initializing 3D samples...
[21959.046] Audio System: Successfully initialized 3D samples.
[21959.046] Audio System: Successfully initialized base audio system.
[21959.046] Audio System: Successfully initialized Miles Audio System.
[21959.046] AudioManager: Successfully initialized the audio system.
[21959.046] AudioManager: Successfully initialized the audio manager.
```

#### `init.log`

30 of 32 nonempty rows: the first 20 and final 10.

```log
[21955.968] DBG: CIV Init
[21955.968] VERSION: App: C:\Program Files (x86)\Steam\steamapps\common\Sid Meier's Civilization IV Beyond the Sword\Beyond the Sword\Civ4BeyondSword.exe
[21955.968] VERSION: Build: Thu May 29 04:28:49 2014
[21955.968] VERSION: 3.1.9.0 (128100)
[21957.312] VERSION: Mod Loaded: Mods\AdvCiv-SAS\
[21957.312] DBG: FILE Cat Init
[21958.437] DBG: Game Init
[21958.437] DBG: Multiplayer Init BEGIN
[21958.656] DBG: Multiplayer Init END
[21958.656] DBG: Audio Init
[21959.046] DBG: ArtFileMgr Init
[21959.046] DBG: Python Init
[21962.531] VERSION: CIV Version: 319
[21962.531] VERSION: Minimum Version: 319
[21962.531] VERSION: Save Version: 14223
[21962.546] DBG: Input Init
[21962.546] DBG: Engine Init
[21962.671] DBG: Checking available screen resolution
[21962.828] DBG: Validating screen resolution
[21962.828] DBG: Creating rendererer
[21963.547] DBG: Engine: Scene Lights Initialized
[21963.563] DBG: Music Start
[21963.578] DBG: Font Init
[21963.594] DBG: Begin MenuManager
[21963.594] DBG: Total Frame MS: 7694.0  FPS: 000  Min:000 Max:000 Avg:000  SampleFilter:10.000000
 Time   :   Ave  :  Min% :  Max% : Num : Profile Name
-----------------------------------------------------
--------------------------------------------------
[21983.063] DBG: Engine: Camera Initialized
[21986.672] DBG: SyncRandSeed is 21963585, MapRandSeed is 21963585
```

#### `initmgr.log`

All 6 nonempty rows in the capture.

```log
[21985.328] InitMgr elapsed time = 3.51
[21986.641] DBG: Total Frame MS: 4828.0  FPS: 000  Min:000 Max:000 Avg:000  SampleFilter:0.010000
 Time   :   Ave  :  Min% :  Max% : Num : Profile Name
-----------------------------------------------------
--------------------------------------------------
[21986.641] RELEASE build, World size=Huge
```

#### `resmgr.log`

All 3 nonempty rows in the capture.

```log
RESMGR: Texture FlagDECAL.dds failed to load
RESMGR: Texture PlayerColor01.tga failed to load
RESMGR: Texture PlayerColor02.tga failed to load
```

#### Before: xml.log

This real capture predates the DLL-row UTC/`record=` modernization described above and is retained as its matched pre-change baseline.

30 of 627 nonempty rows: the first 20 and final 10.

```log
[21958.046] Loading XML file xml\GameInfo/CIV4PlayerOptionInfos.xml
[21958.062] Load XML file xml\GameInfo/CIV4PlayerOptionInfos.xml SUCCEEDED
[21958.062] SetGlobalClassInfo (Civ4PlayerOptionInfos/PlayerOptionInfos/PlayerOptionInfo)
[21958.062] Loading XML file xml\GameInfo/CIV4GraphicOptionInfos.xml
[21958.062] Load XML file xml\GameInfo/CIV4GraphicOptionInfos.xml SUCCEEDED
[21958.062] SetGlobalClassInfo (Civ4GraphicOptionInfos/GraphicOptionInfos/GraphicOptionInfo)
[21959.625] Loading XML file xml\GlobalDefines.xml
[21959.625] Load XML file xml\GlobalDefines.xml SUCCEEDED
[21959.625] Loading XML file xml\GlobalDefinesAlt.xml
[21959.625] Load XML file xml\GlobalDefinesAlt.xml SUCCEEDED
[21959.625] Loading XML file xml\PythonCallbackDefines.xml
[21959.625] Load XML file xml\PythonCallbackDefines.xml SUCCEEDED
[21959.625] Loading XML file xml\GlobalDefines_devel.xml
[21959.625] Load XML file xml\GlobalDefines_devel.xml SUCCEEDED
[21959.625] Loading XML file xml\GlobalDefines_advc.xml
[21959.625] Load XML file xml\GlobalDefines_advc.xml SUCCEEDED
[21959.625] Loading XML file xml\GlobalDefines_advciv_sas.xml
[21959.640] Load XML file xml\GlobalDefines_advciv_sas.xml SUCCEEDED
[21959.656] Loading XML file xml\BBAI_Game_Options_GlobalDefines.xml
[21959.656] Load XML file xml\BBAI_Game_Options_GlobalDefines.xml SUCCEEDED
[21981.750] SetGlobalClassInfo (Civ4AttachableInfos/AttachableInfos/AttachableInfo)
[21981.750] Loading XML file xml\GameInfo/CIV4DiplomacyInfos.xml
[21981.766] Load XML file xml\GameInfo/CIV4DiplomacyInfos.xml SUCCEEDED
[21981.766] SetDiplomacyInfo (Civ4DiplomacyInfos/DiplomacyInfos/DiplomacyInfo)
[21981.781] Loading XML file xml\Misc/Civ4TutorialInfos.xml
[21981.781] Load XML file xml\Misc/Civ4TutorialInfos.xml SUCCEEDED
[21981.781] SetGlobalClassInfo (Civ4TutorialInfos/TutorialInfo)
[21981.781] Loading XML file xml\GameInfo/CIV4EspionageMissionInfo.xml
[21981.781] Load XML file xml\GameInfo/CIV4EspionageMissionInfo.xml SUCCEEDED
[21981.781] SetGlobalClassInfo (Civ4EspionageMissionInfo/EspionageMissionInfos/EspionageMissionInfo)
```

#### After: xml.log

SAS formats the DLL-owned rows through the narrow `logSASCiv4LegacyLogRow` helper; it does not intercept or rewrite EXE-owned rows that may share the file. The structured DLL record families cover file loading, cache activity, global/info setup and unknown XML types. `XML_CACHE` and `XML_UNKNOWN_TYPE` are source-supported even when a particular startup run does not exercise them.

The rebuilt Debug-opt capture also contains 627 nonempty rows. The identical observed row count is reassuring because the modernization restructures existing DLL messages rather than intentionally adding a second stream of startup rows, but it is not treated as a formal invariant across runs.

The selection below keeps the first 10 rows, all 10 `XML_GLOBAL_STRING_ARRAY` rows and later action/diplomacy/end-of-load rows so the main captured record families are visible.

```log
[53871.687] utc=20261020T170755.441Z record=XML_FILE_LOAD phase=BEGIN path=xml\GameInfo/CIV4PlayerOptionInfos.xml
[53871.703] utc=20261020T170755.447Z record=XML_FILE_LOAD phase=END outcome=SUCCEEDED path=xml\GameInfo/CIV4PlayerOptionInfos.xml
[53871.703] utc=20261020T170755.447Z record=XML_GLOBAL_CLASS_INFO tag=Civ4PlayerOptionInfos/PlayerOptionInfos/PlayerOptionInfo pass=1 final=1
[53871.703] utc=20261020T170755.448Z record=XML_FILE_LOAD phase=BEGIN path=xml\GameInfo/CIV4GraphicOptionInfos.xml
[53871.703] utc=20261020T170755.449Z record=XML_FILE_LOAD phase=END outcome=SUCCEEDED path=xml\GameInfo/CIV4GraphicOptionInfos.xml
[53871.703] utc=20261020T170755.449Z record=XML_GLOBAL_CLASS_INFO tag=Civ4GraphicOptionInfos/GraphicOptionInfos/GraphicOptionInfo pass=1 final=1
[53872.687] utc=20261020T170756.440Z record=XML_FILE_LOAD phase=BEGIN path=xml\GlobalDefines.xml
[53872.687] utc=20261020T170756.441Z record=XML_FILE_LOAD phase=END outcome=SUCCEEDED path=xml\GlobalDefines.xml
[53872.703] utc=20261020T170756.444Z record=XML_FILE_LOAD phase=BEGIN path=xml\GlobalDefinesAlt.xml
[53872.703] utc=20261020T170756.445Z record=XML_FILE_LOAD phase=END outcome=SUCCEEDED path=xml\GlobalDefinesAlt.xml
[53873.140] utc=20261020T170756.888Z record=XML_GLOBAL_STRING_ARRAY tag=Civ4Types/AnimationOperatorTypes/AnimationOperatorType
[53873.140] utc=20261020T170756.888Z record=XML_GLOBAL_STRING_ARRAY tag=Civ4Types/FunctionTypes/FunctionType
[53873.140] utc=20261020T170756.888Z record=XML_GLOBAL_STRING_ARRAY tag=Civ4Types/FlavorTypes/FlavorType
[53873.140] utc=20261020T170756.888Z record=XML_GLOBAL_STRING_ARRAY tag=Civ4Types/ArtStyleTypes/ArtStyleType
[53873.140] utc=20261020T170756.888Z record=XML_GLOBAL_STRING_ARRAY tag=Civ4Types/CitySizeTypes/CitySizeType
[53873.140] utc=20261020T170756.888Z record=XML_GLOBAL_STRING_ARRAY tag=Civ4Types/ContactTypes/ContactType
[53873.140] utc=20261020T170756.888Z record=XML_GLOBAL_STRING_ARRAY tag=Civ4Types/DiplomacyPowerTypes/DiplomacyPowerType
[53873.140] utc=20261020T170756.888Z record=XML_GLOBAL_STRING_ARRAY tag=Civ4Types/AutomateTypes/AutomateType
[53873.140] utc=20261020T170756.888Z record=XML_GLOBAL_STRING_ARRAY tag=Civ4Types/DirectionTypes/DirectionType
[53873.140] utc=20261020T170756.888Z record=XML_GLOBAL_STRING_ARRAY tag=Civ4Types/FootstepAudioTypes/FootstepAudioType
[53882.109] utc=20261020T170805.863Z record=XML_GLOBAL_ACTION_INFO action=SET
[53882.125] utc=20261020T170805.869Z record=XML_GLOBAL_CLASS_INFO tag=Civ4AttachableInfos/AttachableInfos/AttachableInfo pass=1 final=1
[53882.125] utc=20261020T170805.870Z record=XML_FILE_LOAD phase=BEGIN path=xml\GameInfo/CIV4DiplomacyInfos.xml
[53882.125] utc=20261020T170805.874Z record=XML_FILE_LOAD phase=END outcome=SUCCEEDED path=xml\GameInfo/CIV4DiplomacyInfos.xml
[53882.125] utc=20261020T170805.874Z record=XML_DIPLOMACY_INFO tag=Civ4DiplomacyInfos/DiplomacyInfos/DiplomacyInfo
[53882.141] utc=20261020T170805.884Z record=XML_FILE_LOAD phase=BEGIN path=xml\Misc/Civ4TutorialInfos.xml
[53882.141] utc=20261020T170805.885Z record=XML_FILE_LOAD phase=END outcome=SUCCEEDED path=xml\Misc/Civ4TutorialInfos.xml
[53882.141] utc=20261020T170805.885Z record=XML_GLOBAL_CLASS_INFO tag=Civ4TutorialInfos/TutorialInfo pass=1 final=1
[53882.141] utc=20261020T170805.886Z record=XML_FILE_LOAD phase=BEGIN path=xml\GameInfo/CIV4EspionageMissionInfo.xml
[53882.141] utc=20261020T170805.887Z record=XML_FILE_LOAD phase=END outcome=SUCCEEDED path=xml\GameInfo/CIV4EspionageMissionInfo.xml
[53882.141] utc=20261020T170805.887Z record=XML_GLOBAL_CLASS_INFO tag=Civ4EspionageMissionInfo/EspionageMissionInfos/EspionageMissionInfo pass=1 final=1
```

No `XML_CACHE` or `XML_UNKNOWN_TYPE` row occurred in this successful startup capture, so neither is illustrated in the excerpt above.

#### `LSystem.log`

30 selected rows: the first 20 plus 10 distinct later rows; repeated identical warnings are omitted.

```log
Initial System for XML/Buildings/CIV4CityLSystem.xml:
------------------------------------------------------------------------
Nodes:
	    Emergency_Overflow: [w=0.5 h=0.5] [0 art, 1 prods]
	<T> Leaf_1x1: [w=1.0 h=1.0] [39 art, 0 prods]
	    Node_2x2: [w=2.0 h=2.0] [0 art, 6 prods]
	    Node_3x1: [w=3.0 h=1.0] [0 art, 5 prods]
	    Node_6x6: [w=6.0 h=6.0] [0 art, 9 prods]
	    EONode: [w=4.0 h=4.0] [0 art, 5 prods]
	<T> Leaf_2x1: [w=2.0 h=1.0] [66 art, 0 prods]
	    Node_3x2: [w=3.0 h=2.0] [0 art, 4 prods]
	    Node_4x1: [w=4.0 h=1.0] [0 art, 4 prods]
	<T> Leaf_2x2: [w=2.0 h=2.0] [22 art, 0 prods]
	<T> Leaf_3x1: [w=3.0 h=1.0] [14 art, 0 prods]
	    Node_16x16: [w=16.0 h=16.0] [0 art, 3 prods]
	    Node_3x3: [w=3.0 h=3.0] [0 art, 3 prods]
	    Node_4x2: [w=4.0 h=2.0] [0 art, 5 prods]
	    CITY_ROOT: [w=16.0 h=16.0] [0 art, 1 prods]
	<T> Leaf_3x2: [w=3.0 h=2.0] [2 art, 0 prods]
	    Node_2x1: [w=2.0 h=1.0] [0 art, 3 prods]
	<T> BLeaf_3x2: [w=3.0 h=2.0] [55 art, 0 prods]
	<T> Leaf_Farm_1x1: [w=1.0 h=1.0] [7 art, 0 prods]
	<T> BLeaf_4x2: [w=4.0 h=2.0] [7 art, 0 prods]
------------------------------------------------------------------------
building:Art/Structures/Buildings/Public_Transportation/Public_Transportation.nif does not contain a node called SHADOW. Its shadow, if present, will be corrupt!
Failed to place goal building ART_DEF_BUILDING_LIGHTHOUSE.
building:Art/Structures/Buildings/Grocer/Grocer.nif does not contain a node called SHADOW. Its shadow, if present, will be corrupt!
building:Art/AdvCiv_SAS/Buildings/Natya_Shastra/nif/Brihadeeswarar.nif does not contain a node called SHADOW. Its shadow, if present, will be corrupt!
Failed to place goal building ART_DEF_BUILDING_HOLLYWOOD.
building:Art/Structures/Buildings/Levee/Levee.nif does not contain a node called SHADOW. Its shadow, if present, will be corrupt!
```

## Source references

### Current/local source

- [DLL logger](../../CvGameCoreDLL/CvDLLLogger.cpp), [RNG initialization](../../CvGameCoreDLL/CvGlobals.cpp), [shared filename builder](../../CvGameCoreDLL/CvGameCoreUtils.cpp).
- [BBAI category registry and session writer](../../CvGameCoreDLL/BBAILog.cpp), [cached BBAI gates](../../CvGameCoreDLL/BBAILog.h), [Settler decision producers](../../CvGameCoreDLL/CvUnitAI.cpp), [site selection/stored values](../../CvGameCoreDLL/CvPlayerAI.cpp) and [SASGameRecord writer](../../CvGameCoreDLL/SASGameRecordLog.cpp).
- UWAI producers and support:
  - [UWAIAgent](../../CvGameCoreDLL/UWAIAgent.cpp), [MilitaryAnalyst](../../CvGameCoreDLL/MilitaryAnalyst.cpp), [InvasionGraph](../../CvGameCoreDLL/InvasionGraph.cpp), [ArmamentForecast](../../CvGameCoreDLL/ArmamentForecast.cpp), [WarEvaluator](../../CvGameCoreDLL/WarEvaluator.cpp) and [WarUtilityAspect](../../CvGameCoreDLL/WarUtilityAspect.cpp).
  - [Shared nested mute state](../../CvGameCoreDLL/UWAILogMuteState.h).
- Inherited comparison: local sibling `AdvCiv/CvGameCoreDLL/BBAILog.h`, `BBAILog.cpp`, `CvUnitAI.cpp`, `UWAIReport.cpp` and `UWAIAgent.cpp` (AdvCiv 1.14); [KI#505.3](README_Known_Issues.md#ki-505.3) preserves the SAS investigation of the retired report path.
- [Python logging utilities](../../Assets/Python/BUG/BugUtil.py), [shared SAS Python writer](../../Assets/Python/SASPythonLogging.py), [OOS logger](../../Assets/Python/Contrib/OOSLogger.py), [Autolog](../../Assets/Python/Contrib/autolog.py).
- [Inherited-log row formatter](../../CvGameCoreDLL/CvGameCoreUtils.cpp), [XML log producer](../../CvGameCoreDLL/CvXMLLoadUtility.cpp) and [XML setup/cache producers](../../CvGameCoreDLL/CvXMLLoadUtilitySet.cpp).
- [StatusDump event manager](../../Assets/Python/Contrib/StatusDumpEventManager.py), [older StatusDump helper](../../Assets/Python/Contrib/StatusDump.py), [StatusDump config](../../Assets/Config/StatusDump.xml).
- [Starting-position iteration](../../CvGameCoreDLL/StartingPositionIteration.cpp), [internal profiler](../../CvGameCoreDLL/FProfiler.cpp), [TSC profiler](../../CvGameCoreDLL/TSCProfiler.cpp).
- [Inherited AdvCiv manual](../../_0_Common_Docs/AdvCiv_Base_Doc/manual.txt), especially message-control logging and multiplayer debugging.

### Historical / external provenance

- CivFanatics, 2005-10-28: [launch-era report documenting `PythonDbg.log`, `PythonErr.log` and `PythonErr2.log`](https://forums.civfanatics.com/threads/possible-python-error-temp-solution.134453/).
- CivFanatics, 2005-11-15 onward: [eotinb's original Autolog thread](https://forums.civfanatics.com/threads/mod-log-changes.141164/), explicitly describing a succession-game/note-taking journal and distinguishing it from debug/error logs.
- CivFanatics, 2009-11-05: [Better BTS AI 0.82 decision-logging documentation](https://forums.civfanatics.com/threads/bts-in-game-debug-mode.279903/), including `BBAI.log`, `/DLOG_AI` and interpretation of its timing prefix.
- CivFanatics, 2009-11-15: [BUG 4.2 / BULL 1.0 release](https://forums.civfanatics.com/threads/download-bug-and-bat-here.274636/).
- CivFanatics, 2019-03-18: [Kek-Mod 0.23 release](https://forums.civfanatics.com/threads/modmod-kek-mod.563377/) included OOSLogger and credited the idea to Fall from Heaven 2 by Kael. The same changelog lists OOSLogger fixes from AdvCiv in 0.25 (2019-07-16).
- AdvCiv Git, 2019-03-27: [OOSLogger merge from Kek-Mod](https://github.com/f1rpo/AdvCiv/commit/81c89a9e0511aa58630f7821fdc9287ecffa8edf).
- AdvCiv Git, 2019-08-27: [`CvDLLLogger` introduction](https://github.com/f1rpo/AdvCiv/commit/553b1a0f8d92b00f09f64f2ee1ecb4d696ebe43e), encapsulating the message-control log.
- AdvCiv Git, 2019-11-05: [TSC profiler import from We the People](https://github.com/f1rpo/AdvCiv/commit/031365f32f6d71519e2fd09c6f6e963ecbef4bd3).
- AdvCiv Git, 2020-05-27: [initial Starting Position Iteration implementation](https://github.com/f1rpo/AdvCiv/commit/a70e4ce3a1db01c51da7fa370a706d8784191b38).
- AdvCiv Git, 2020-11-18: [AsyncRand logging fix](https://github.com/f1rpo/AdvCiv/commit/64e3335495dcd86aebf5068bf8903c0a1f83c508).
- Public RoM-AND/C2C-derived source provides matching producers for `Checksum.log`, `MLF.log`, `XmlDependencyTypes.log`, `CvGlobals_logInfoTypeMap.log` and MLF enumeration diagnostics; these establish external lineage but do not prove which mod produced the user's local copies.
