# Civ4 / AdvCiv-SAS logging reference

**Authors:** GPT-6.1-Sol (Codex), 2026-10-19; historical provenance research and rebase by ChatGPT-5.6-Sol, 2026-10-19.

Use this reference to locate diagnostic output, identify its writer or enabling conditions, and distinguish current SAS evidence from older or other-mod files.

**Inventory snapshot: 2026-10-19.** The presence, counts, settings and file dates below describe the local Beyond the Sword user folder at inspection time; they are not live status or a claim that every installation produces these files.

The EXE is closed source, mods can supply arbitrary filenames, and some outputs require particular situations or compile options. Consequently, an exhaustive universal filename list is unavailable. The tables distinguish observed files from source-supported possibilities.

## Historical lineage and ownership

The files below do not belong to one logging framework. Civ4 accumulated several generations of engine, Python and mod diagnostics, and the distinction matters when deciding what SAS should modernize. Dates identify the earliest evidence established by this review, not necessarily the first private implementation.

| Output / family | Producer / lineage | Established history and purpose | SAS ownership / modernization implication |
| --- | --- | --- | --- |
| `PythonDbg.log`, `PythonErr.log`, `PythonErr2.log` | Firaxis Civ4 Python/EXE logging | All three are documented together on 2005-10-28, during Civ4's launch period. `PythonDbg` is the general Python/debug stream; `PythonErr` carries Python exceptions; `PythonErr2` is a secondary startup/interpreter stream rather than simply another traceback file. | The EXE owns file creation and early startup output. SAS can consistently format Python-written rows after `SASPythonLogging` installs, but cannot reconstruct timestamps for earlier EXE-written rows. |
| `app.log`, `init.log`, `initmgr.log`, `audio.log`, `resmgr.log`, `xml.log`, `LSystem.log`, `network-<port>.txt` | Firaxis engine | Native application, initialization, audio, resource, XML, LSystem and networking diagnostics. Old Civ4 troubleshooting captures already show the engine's numeric elapsed-time-style prefixes. | Treat as EXE-owned evidence: correlate/archive by run rather than attempting wholesale format changes. DLL calls may add specific rows to some files, notably `xml.log`, but SAS does not own the complete stream. |
| `MPLog.txt`, optional `MPLog<player>.log`, `ASyncRand.log` | Firaxis message-control/RNG logging, later wrapped and extended by AdvCiv | The INI's message/random/synchronization logging family is old Civ4 machinery. AdvCiv introduced `CvDLLLogger` in 2019 to encapsulate the message-control log and supports per-player MP filenames; a 2020 AdvCiv fix made asynchronous RNG logging work correctly. | Preserve deterministic comparability. Session/header context can help, but wall-clock decoration must not obscure RNG or OOS comparisons. |
| BUG Autolog | eotinb -> BUG -> K-Mod/AdvCiv/SAS | Publicly introduced on 2005-11-15 as a note-taking/gameplay journal "with succession games in mind". Its author explicitly distinguished it from Civ4's existing debug/error logs. | Preserve its human-readable journal role. It is configurable text/HTML output, not a machine diagnostic stream. |
| BUG StatusDump | Ruff_Hi / BUG, inherited through K-Mod/AdvCiv | Current SAS loads the inherited `StatusDump` module. Ctrl+Alt+D invokes a support dump using the Autolog path and a `<player>_<year>.txt` filename. Large sections remain literal placeholders such as `Spinner stuff here`, `Player Cities here` and `AIs stuff here`; the older unused helper class still hardcodes `StatusDump.txt`. | Document as an incomplete inherited support facility, not as a dependable SAS diagnostic. Do not modernize it merely because it can emit a file. |
| `BBAI.log` -> `BBAI_*.log` | Better BTS AI -> K-Mod -> AdvCiv -> SAS | Better BTS AI 0.82 publicly documented its AI decision logging system on 2009-11-05: the purpose was to expose major AI decisions each turn for analysis and improvement. The original file was `BBAI.log`. | SAS owns its current BBAI wrapper/filename/session improvements while preserving the log's AI-decision semantics. This is a legacy diagnostic worth modernizing, not replacing. |
| `bull.log` | BULL, the DLL companion to BUG | BULL 1.0 was released with BUG 4.2 on 2009-11-15. Public BULL-derived sources use `bull.log` for DLL-side BUG/options diagnostics. No current SAS filename producer was found. | Treat the local file as historical/foreign evidence unless a current producer is established. |
| `OOSLog - <player> - Turn <turn>.log` | Kek-Mod -> AdvCiv, with idea credited to Fall from Heaven 2 / Kael | Kek-Mod 0.25 on 2019-07-16 included `OOSLogger`; AdvCiv merged the Kek-Mod implementation in 2019. It dumps deterministic game state when an OOS is detected. | Keep distinct from `MPLog`: OOSLog is a state snapshot for diffing after divergence, while MPLog helps trace message/RNG activity leading to divergence. |
| `IFP_log.txt` | Inherited DLL internal profiler | Function/call timing profiler output; exact first introduction was not established by this review. | Keep numerical/data-oriented. Prefer run metadata or a header to per-row decoration. |
| `TSCProfile.log` | We the People profiler -> AdvCiv | AdvCiv imported Time Stamp Counter profiling code from We the People on 2019-11-05 and integrated it with Civ4 logging. | Keep profiler rows compact and machine-friendly; add provenance/header context only if needed. |
| `StartingPos.log` | AdvCiv | AdvCiv's Starting Position Iteration work began publicly on 2020-05-27 as a fairer-start algorithm; this log traces that subsystem when enabled. | Algorithm trace, not general diagnostics. Header/run context is preferable to verbose per-line timestamps. |
| `juggle_log.txt`, `combat.txt` | K-Mod/AdvCiv developer diagnostics | Compile-time specialist diagnostics: citizen-assignment efficiency and combat-odds verification respectively. Exact first introduction was not established here. | Preserve as experiment datasets. A small metadata header is safer than making every record verbose. |
| `SASGameRecord_*.log`, `SASRuntimeStatus.json`, `SASRuntimeHistory_*.jsonl` | AdvCiv-SAS | SAS-native provenance, reproduction and runtime-observation systems. | Fully SAS-controlled structured diagnostics; their own revision/history documentation is authoritative. |

### Modernization rule of thumb

Use ownership and purpose before changing a format:

- **SAS-controlled diagnostics:** direct timestamp, filename, session, provenance and flushing improvements are appropriate when they preserve semantics and performance.
- **Shared Python streams:** SAS can improve rows that pass through its Python wrapper after installation, while clearly preserving the existence of EXE-written startup rows that bypass it.
- **EXE-owned Firaxis logs:** prefer correlation, archival and run identity rather than pretending SAS controls the writer.
- **RNG/OOS/profiling/experiment data:** preserve stable comparison-friendly rows; a header or sidecar is often better than per-row wall-clock text.
- **Human journals such as Autolog:** optimize for readable gameplay notes, not grep-oriented diagnostic structure.
- **Foreign/stale files from other mods:** identify their probable or verified origin, but do not add SAS support merely because they remain in the shared user folder.

## Files present in the user Logs folder

The folder is `Documents/My Games/beyond the sword/Logs`. Presence establishes that a file exists, not that the current SAS run wrote it. Empty files are included.

**Latest modified (UTC)** is the filesystem's last-write time; for a family it is the newest member's time. It helps identify stale evidence but does not establish generation time or which mod wrote it: copying/restoring files can preserve or change filesystem dates. Embedded process/session timestamps provide stronger run identity where available.

| File or family | Present | Latest modified (UTC) | Role / evidence |
| --- | --- | --- | --- |
| `PythonDbg.log` | Yes | 2026-10-19 13:43:13 | Python stdout, direct prints, BUG messages and some DLL diagnostics. |
| `PythonErr.log` | Yes, empty | 2026-10-19 13:41:03 | Python error output; an empty file is distinct from an absent file. |
| `PythonErr2.log` | Yes | 2026-10-19 13:41:03 | Secondary Python output; current sample includes converter warnings and the Python search path. |
| `app.log` | Yes | 2026-10-19 13:41:06 | EXE/application activity; current sample includes activation and framebuffer information. |
| `init.log` | Yes | 2026-10-19 13:42:31 | Startup/version and initialization messages. |
| `initmgr.log` | Yes | 2026-10-19 13:42:32 | EXE initialization manager; current sample includes new-game initialization and elapsed time. |
| `audio.log` | Yes | 2026-10-19 13:41:03 | Audio-manager initialization and asset loading. |
| `resmgr.log` | Yes | 2026-10-19 13:42:49 | Resource-loading diagnostics; current sample includes texture-load failures. |
| `xml.log` | Yes | 2026-10-19 13:41:18 | XML loading and related errors; current DLL also explicitly writes this file. |
| `LSystem.log` | Yes | 2026-10-19 13:42:34 | City/plot LSystem diagnostics; current sample begins with the city LSystem XML. |
| `network-2056.txt` | Yes | 2026-10-19 13:43:13 | Network diagnostics with the configured port in its filename. Other ports/instances may produce other names. |
| `BBAI_*.log` | Yes, 28 files | 2026-10-18 14:08:54 | Timestamped detailed AI diagnostics, including older runs. |
| `SASGameRecord_*.log` | Yes, 1,108 files | 2026-10-19 13:42:55 | Timestamped game/session records, including older runs. |
| `SASRuntimeStatus.json` | Yes | 2026-10-19 13:43:13 | Latest fixed-name runtime observation and linked history identity. |
| `SASRuntimeHistory_*.jsonl` | Yes, 21 files | 2026-10-19 13:43:13 | Per-process runtime history. |

The BBAI/SASGameRecord counts above describe timestamped files, not a requirement that their legacy fixed filenames exist. Configured filename/session/player variations are handled by the shared filename builder.

### Other diagnostic files present

These files also exist, but no corresponding filename producer was found in current SAS source during this inventory. They should not be treated as evidence that SAS currently generates them; the shared Logs folder contains output from other mods and earlier runs. External-source identifications below describe matching public writers, not proof of which exact executable produced the local copy.

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
| `ResourceAllocation.log` | Yes | 2026-06-04 08:12:21 | No trustworthy Civ4 producer was established in this review; retain as unknown foreign/historical residue. |
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

`debug_SPaH.log` appears only in a commented-out source statement; it is not an active output path. Similarly, obsolete commented filenames should not be presented as currently supported logs.

### Outputs outside the user Logs folder

| Output | Present | Meaning |
| --- | --- | --- |
| `CvGameCoreDLL/Project/<configuration>/AdvCiv.log` | Yes, Release and Debug-opt | Compiler/linker diagnostics, not game output. |
| `<DLL filename>.build.json` | Yes | Build status, configuration, inputs, warning/error counts and binary identity; includes the installed DLL's companion report. |
| BUG Autolog / StatusDump directory | No current file found in the checked default paths | Autolog and StatusDump normally use their configurable/default Autolog directory rather than requiring `Logs`. |
| `ThemeParseLog.txt` | Not inventoried in this snapshot | Historical BtS engine/theme parser diagnostic reported in the Beyond the Sword installation directory rather than the user `Logs` folder. It records `.thm` parsing/resource errors; some warnings can be benign. |
| Crash dump files | Not inventoried here | Binary crash evidence, not a text log. `GenerateCrashDumps=1` is currently configured; actual dump location/presence needs separate inspection. |

### Historical engine outputs not present in this snapshot

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

## Python diagnostic messages

`SASPythonLogging` wraps Python stdout/stderr and supplies the same row formatter to Civ4's `CvUtil.RedirectDebug` and `RedirectError` classes. Direct prints, BUG file messages and multiline traceback text therefore share one timestamp path after installation.

Each nonempty Python-written row receives a UTC date/time with milliseconds. Fragmented writes retain line-start state, and each write is forwarded immediately; no partial row is held until its newline. Blank rows remain blank. BUG's timestamp option controls the shared formatter, while its existing message-level filtering and on-screen messages remain unchanged.

The timestamp comes from the DLL's Windows UTC sampler. The Python layer requires a DLL exposing `getSASDiagnosticUtcTimestamp`; disabling timestamps avoids sampling the clock. The leader-cache serializer uses an explicit raw-output path so its generated Python module remains copyable.

Timestamp work occurs when Python writes output; there is no frame timer or background polling. BUG-filtered messages never reach the stream, and disabling timestamps skips clock sampling while retaining stream processing.

The wrapper does not gate on the INI's `LoggingEnabled` switch. If Python still emits text while the engine suppresses file output, formatting work can still occur; the logging-disabled case has not been measured here.

EXE output produced before installation or written directly to its files bypasses these streams. In particular, the Python startup preamble and early `PythonErr2.log` warnings cannot be assumed to pass through the Python wrappers.

## Startup rows and run boundaries

Rows emitted before the stream wrapper is installed remain in the original log, with their text unchanged. Their exact per-row UTC times cannot be reconstructed from a later snapshot. Preserving a copy retains this evidence, but does not recover those missing timestamps.

When runtime recording is enabled, `SASRuntimeStatus.json` identifies the process through `processId` and `processStartUtc`, and the current new/load session through `sessionSerial`, `sessionKind` and `sessionStartedUtc`. `nativeReadyUtc` distinguishes session initialization from the game becoming ready.

The companion `SASRuntimeHistory_*.jsonl` preserves dated transitions, including completed activity intervals and reset/exit observations. A clean shutdown publishes `PROCESS_EXIT` with `EXITED_CLEANLY`; its observation timestamp identifies the recorded shutdown boundary.

There is no separate universal `sessionEndedUtc` field, and a crash or forced termination can leave only the last observation rather than an exact end time.

## Before and after: shared Python log timestamps

The Before samples come from preserved logs captured before this change. The After samples show the shared stdout/stderr formatter running with a matching Release DLL on 2026-10-19. These are complete selected lines; they need not be adjacent in the original file. The error comparison explicitly labels its reconstructed Before block.

Each file has its own comparison below, with 20 complete nonempty rows per sample where available. Shorter files show all their rows; empty files are stated explicitly. PythonDbg combines a warning/activity slice with the final ten rows; PythonErr2 centers its slice on a warning. These selected rows are not necessarily consecutive across a whole block.

The capture was taken while Civ4 was open, so coverage counts describe that snapshot rather than a final session-size measurement.

| File | Before | After |
| --- | --- | --- |
| `PythonDbg.log` | [Before](#before-pythondbglog) | [After](#after-pythondbglog) |
| `PythonErr.log` | [Before](#before-pythonerrlog) | [After](#after-pythonerrlog) |
| `PythonErr2.log` | [Before](#before-pythonerr2log) | [After](#after-pythonerr2log) |
| `app.log` | [Before](#before-applog) | [After](#after-applog) |
| `audio.log` | [Before](#before-audiolog) | [After](#after-audiolog) |
| `init.log` | [Before](#before-initlog) | [After](#after-initlog) |
| `initmgr.log` | [Before](#before-initmgrlog) | [After](#after-initmgrlog) |
| `resmgr.log` | [Before](#before-resmgrlog) | [After](#after-resmgrlog) |
| `xml.log` | [Before](#before-xmllog) | [After](#after-xmllog) |
| `LSystem.log` | [Before](#before-lsystemlog) | [After](#after-lsystemlog) |

### `PythonDbg.log`

304 of 470 nonempty rows carry timestamps. 164 rows precede installation; 2 later rows bypass the wrapper. Module-loading and WorldBuilder output now join ordinary screen messages in the shared timestamp path.

#### Before: PythonDbg.log

```log
99 SCREEN TURNED ON
load_module BugUnitPlot
15:41:20 WARN : BugData not loaded; loading now
load_module CvWBDesc
load_module SASGameRecordSettings
load_module CvWBInterface
PY:saveDesc:C:\Users\PC\Documents\My Games\Beyond the Sword\Saves\WorldBuilder\WBQuickSave, curDir:C:\Program Files (x86)\Steam\steamapps\common\Sid Meier's Civilization IV Beyond the Sword\Beyond the Sword
WBSave done
load_module encodings.ascii
2 SCREEN TURNED ON
Disabling Map Tab
2 SCREEN TURNED ON
SCREEN OFF
PY:City Built Event: Cuzco
PY:City Built Event: Tiwanaku
200 SCREEN TURNED ON
SCREEN OFF
PY:CDP: Setting autoplay to 0
PY:OnUnInit
UnInit Python
```

#### After: PythonDbg.log

```log
2026-10-19T15:29:01.879Z 200 SCREEN TURNED ON
2026-10-19T15:29:01.879Z SASFontUtils: initialized SAS_UI_FONT_LABEL = 3
2026-10-19T15:29:01.881Z WARN : SpyUtil - incorrect previous game turn found, ignoring
2026-10-19T15:29:01.881Z load_module SASGameRecordSettings
2026-10-19T15:29:01.989Z load_module encodings.latin_1
2026-10-19T15:29:01.990Z 275 - gold
2026-10-19T15:29:01.990Z 276 - research
2026-10-19T15:29:01.990Z 277 - culture
2026-10-19T15:29:01.990Z 278 - espionage
2026-10-19T15:29:01.990Z 256 - food
2026-10-19T15:29:02.925Z load_module CvWBInterface
2026-10-19T15:29:02.925Z load_module CvWBDesc
2026-10-19T15:29:02.931Z PY:saveDesc:C:\Users\PC\Documents\My Games\Beyond the Sword\Saves\WorldBuilder\WBQuickSave, curDir:C:\Program Files (x86)\Steam\steamapps\common\Sid Meier's Civilization IV Beyond the Sword\Beyond the Sword
2026-10-19T15:29:02.955Z WBSave done
2026-10-19T15:29:02.962Z load_module encodings.ascii
2026-10-19T15:29:02.969Z load_module CvGameInterface
2026-10-19T15:29:02.971Z load_module CvGameInterfaceFile
2026-10-19T15:29:03.060Z SASFontUtils: initialized SAS_UI_FONT_HOVER = 3
2026-10-19T15:29:08.068Z 200 SCREEN TURNED ON
2026-10-19T15:29:09.716Z SCREEN OFF
```

### `PythonErr.log`

A deliberate caught exception was reported through Civ4's existing `CvUtil.myExceptHook` on 2026-10-19. The After block is the actual captured traceback from process 45732; its temporary trigger was removed afterward.

#### Before: PythonErr.log

Reconstructed by removing only the UTC prefixes from the actual traceback; the preserved baseline file was empty.

```log
Traceback (most recent call last):

  File "CvScreensInterface", line 75, in showBugOptionsScreen

  File "CvScreensInterface", line 64, in _sasRaiseErrorTimestampTrial

RuntimeError: SAS_LOG_TIMESTAMP_TRIAL: deliberate caught exception; game state unchanged
```

#### After: PythonErr.log

Actual captured output. All four nonempty traceback rows receive UTC timestamps; blank rows remain unchanged.

```log
2026-10-19T15:58:02.556Z Traceback (most recent call last):

2026-10-19T15:58:02.556Z   File "CvScreensInterface", line 75, in showBugOptionsScreen

2026-10-19T15:58:02.556Z   File "CvScreensInterface", line 64, in _sasRaiseErrorTimestampTrial

2026-10-19T15:58:02.556Z RuntimeError: SAS_LOG_TIMESTAMP_TRIAL: deliberate caught exception; game state unchanged
```

### `PythonErr2.log`

123 of 133 nonempty rows carry timestamps; the initial 10 rows precede installation. Every nonempty row after installation is timestamped in this capture.

#### Before: PythonErr2.log

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
```

#### After: PythonErr2.log

```log
2026-10-19T15:28:54.653Z load_module BugUtil
2026-10-19T15:28:54.655Z load_module ColorUtil
2026-10-19T15:28:54.655Z load_module BugInit
2026-10-19T15:28:54.655Z load_module BugPath
2026-10-19T15:28:54.657Z load_module BugConfigTracker
2026-10-19T15:28:54.657Z load_module shutil
2026-10-19T15:28:54.661Z load_module BugTypes
2026-10-19T15:28:54.662Z load_module xmllib
2026-10-19T15:28:54.673Z ..\WARLORDS\ASSETS\PYTHON\SYSTEM\xmllib.py:9: DeprecationWarning: The xmllib module is obsolete.  Use xml.sax instead.
2026-10-19T15:28:54.677Z load_module configobj
2026-10-19T15:28:54.681Z load_module __future__
2026-10-19T15:28:54.687Z load_module validate
2026-10-19T15:28:54.692Z validate import failed
2026-10-19T15:28:54.694Z load_module BugOptionsScreen
2026-10-19T15:28:54.695Z load_module BugErrorOptionsTab
2026-10-19T15:28:54.695Z load_module BugOptionsTab
2026-10-19T15:28:54.696Z load_module BugHelp
2026-10-19T15:28:54.697Z load_module Popup
2026-10-19T15:28:54.698Z load_module CityUtil
2026-10-19T15:28:54.698Z load_module ReligionUtil
```

### `app.log`

Writer format unchanged by the Python stream wrapper. These are different runs, so existing timing values and messages may differ.

#### Before: app.log

This capture contains only 2 nonempty rows; all are shown.

```log
[30720.515] FBMode = FBMODE_MULTISAMPLES_2
[30721.078] CvApp::OnActivate
```

#### After: app.log

This capture contains only 3 nonempty rows; all are shown.

```log
[37191.859] FBMode = FBMODE_MULTISAMPLES_2
[37192.391] CvApp::OnActivate
[37205.188] CvApp::OnDeactivate
```

### `audio.log`

Writer format unchanged by the Python stream wrapper. These are different runs, so existing timing values and messages may differ.

#### Before: audio.log

```log
[30717.468] Audio Manager: Loading Audio Defines file XML\Audio\AudioDefines.xml...
[30717.531] Audio Manager: Successfully loaded Audio Defines file XML\Audio\AudioDefines.xml.
[30717.531] Audio Manager: Loading 2D Scripts file XML\Audio\Audio2DScripts.xml...
[30717.578] ERR: FAudioManager::GetAudioTagIndex(): Could not find string DIPLO_PETER_SFX of TagId 0
[30717.625] Audio Manager: Successfully loaded 2D scripts file XML\Audio\Audio2DScripts.xml.
[30717.625] Audio Manager: Loading 3D Scripts file XML\Audio\Audio3DScripts.xml...
[30717.718] Audio Manager: Successfully loaded 3D scripts file XML\Audio\Audio3DScripts.xml.
[30717.718] Audio Manager: Successfully loaded Soundscape scripts file XML\Audio\AudioSoundscapeScripts.xml.
[30717.718] Audio Manager: Initializing
[30717.718] Audio Manager: Attempting to initialize the audio system.
[30717.718] Audio System: Initializing Miles Audio System...
[30717.718] Audio System: redistribution directory: ..\Miles\win32
[30717.718] Audio System: Starting Miles.
[30717.718] Audio System: Opening the sound driver.
[30717.750] Audio System: CAPS: Total HW Bytes: 0
[30717.750] Audio System: CAPS: Max Contiguous Free HW Bytes: 0
[30717.750] Audio System: CAPS: Total HW 3D Buffers: 0
[30717.750] Audio System: CAPS: Static HW 3D Buffers: 0
[30717.750] Audio System: CAPS: Streaming HW 3D Buffers: 0
[30717.750] Audio System: CAPS: Total Mixing HW 3D Buffers: 1
```

#### After: audio.log

```log
[37188.718] Audio Manager: Loading Audio Defines file XML\Audio\AudioDefines.xml...
[37188.796] Audio Manager: Successfully loaded Audio Defines file XML\Audio\AudioDefines.xml.
[37188.796] Audio Manager: Loading 2D Scripts file XML\Audio\Audio2DScripts.xml...
[37188.843] ERR: FAudioManager::GetAudioTagIndex(): Could not find string DIPLO_PETER_SFX of TagId 0
[37188.890] Audio Manager: Successfully loaded 2D scripts file XML\Audio\Audio2DScripts.xml.
[37188.890] Audio Manager: Loading 3D Scripts file XML\Audio\Audio3DScripts.xml...
[37188.984] Audio Manager: Successfully loaded 3D scripts file XML\Audio\Audio3DScripts.xml.
[37188.984] Audio Manager: Successfully loaded Soundscape scripts file XML\Audio\AudioSoundscapeScripts.xml.
[37188.984] Audio Manager: Initializing
[37188.984] Audio Manager: Attempting to initialize the audio system.
[37188.984] Audio System: Initializing Miles Audio System...
[37188.984] Audio System: redistribution directory: ..\Miles\win32
[37188.984] Audio System: Starting Miles.
[37188.984] Audio System: Opening the sound driver.
[37189.031] Audio System: CAPS: Total HW Bytes: 0
[37189.031] Audio System: CAPS: Max Contiguous Free HW Bytes: 0
[37189.031] Audio System: CAPS: Total HW 3D Buffers: 0
[37189.031] Audio System: CAPS: Static HW 3D Buffers: 0
[37189.031] Audio System: CAPS: Streaming HW 3D Buffers: 0
[37189.031] Audio System: CAPS: Total Mixing HW 3D Buffers: 1
```

### `init.log`

Writer format unchanged by the Python stream wrapper. These are different runs, so existing timing values and messages may differ.

#### Before: init.log

```log
[30716.062] DBG: CIV Init
[30716.062] VERSION: App: C:\Program Files (x86)\Steam\steamapps\common\Sid Meier's Civilization IV Beyond the Sword\Beyond the Sword\Civ4BeyondSword.exe
[30716.062] VERSION: Build: Thu May 29 04:28:49 2014
[30716.062] VERSION: 3.1.9.0 (128100)
[30716.921] VERSION: Mod Loaded: Mods\AdvCiv-SAS\
[30716.921] DBG: FILE Cat Init
[30717.375] DBG: Game Init
[30717.375] DBG: Multiplayer Init BEGIN
[30717.468] DBG: Multiplayer Init END
[30717.468] DBG: Audio Init
[30717.750] DBG: ArtFileMgr Init
[30717.750] DBG: Python Init
[30720.515] VERSION: CIV Version: 319
[30720.515] VERSION: Minimum Version: 319
[30720.515] VERSION: Save Version: 14223
[30720.515] DBG: Input Init
[30720.515] DBG: Engine Init
[30720.625] DBG: Checking available screen resolution
[30720.796] DBG: Validating screen resolution
[30720.796] DBG: Creating rendererer
```

#### After: init.log

```log
[37187.390] DBG: CIV Init
[37187.390] VERSION: App: C:\Program Files (x86)\Steam\steamapps\common\Sid Meier's Civilization IV Beyond the Sword\Beyond the Sword\Civ4BeyondSword.exe
[37187.390] VERSION: Build: Thu May 29 04:28:49 2014
[37187.390] VERSION: 3.1.9.0 (128100)
[37188.140] VERSION: Mod Loaded: Mods\AdvCiv-SAS\
[37188.156] DBG: FILE Cat Init
[37188.593] DBG: Game Init
[37188.593] DBG: Multiplayer Init BEGIN
[37188.718] DBG: Multiplayer Init END
[37188.718] DBG: Audio Init
[37189.031] DBG: ArtFileMgr Init
[37189.031] DBG: Python Init
[37191.843] VERSION: CIV Version: 319
[37191.843] VERSION: Minimum Version: 319
[37191.843] VERSION: Save Version: 14223
[37191.843] DBG: Input Init
[37191.859] DBG: Engine Init
[37191.953] DBG: Checking available screen resolution
[37192.125] DBG: Validating screen resolution
[37192.125] DBG: Creating rendererer
```

### `initmgr.log`

Writer format unchanged by the Python stream wrapper. These are different runs, so existing timing values and messages may differ.

#### Before: initmgr.log

```log
[30732.781] CvInitMgr::GameCoreNew()
[30734.406] InitMgr elapsed time = 1.54
[30734.734] DBG: Total Frame MS: 1869.0  FPS: 001  Min:001 Max:000 Avg:001  SampleFilter:0.010000
 Time   :   Ave  :  Min% :  Max% : Num : Profile Name
-----------------------------------------------------
--------------------------------------------------
[30734.734] RELEASE build, World size=Arena
[30793.422] InitMgr elapsed time = 0.77
[30793.859] DBG: Total Frame MS: 16784.0  FPS: 000  Min:10000000 Max:000 Avg:000  SampleFilter:0.010000
 Time   :   Ave  :  Min% :  Max% : Num : Profile Name
-----------------------------------------------------
--------------------------------------------------
[30793.859] RELEASE build, World size=Arena
[30805.469] CvInitMgr::GameCoreNew()
[30806.297] InitMgr elapsed time = 0.83
[30806.625] DBG: Total Frame MS: 9969.0  FPS: 000  Min:10000000 Max:000 Avg:000  SampleFilter:0.010000
 Time   :   Ave  :  Min% :  Max% : Num : Profile Name
-----------------------------------------------------
--------------------------------------------------
[30806.625] RELEASE build, World size=Arena
```

#### After: initmgr.log

This capture contains only 6 nonempty rows; all are shown.

```log
[37197.016] InitMgr elapsed time = 1.38
[37197.422] DBG: Total Frame MS: 1788.0  FPS: 001  Min:001 Max:000 Avg:001  SampleFilter:0.010000
 Time   :   Ave  :  Min% :  Max% : Num : Profile Name
-----------------------------------------------------
--------------------------------------------------
[37197.422] RELEASE build, World size=Arena
```

### `resmgr.log`

Writer format unchanged by the Python stream wrapper. These are different runs, so existing timing values and messages may differ.

#### Before: resmgr.log

This capture contains only 11 nonempty rows; all are shown.

```log
RESMGR: Texture FlagDECAL.dds failed to load
RESMGR: Texture PlayerColor01.tga failed to load
RESMGR: Texture PlayerColor02.tga failed to load
RESMGR: Failed loading file art/LeaderHeads/Augustus_Caesar/augustus_caesar_BG_background.kf. Reason: (code:1) Cannot open file.
RESMGR: Failed loading file art/LeaderHeads/Augustus_Caesar/augustus_caesar_BG_background.kf. Reason: (code:1) Cannot open file.
RESMGR: Texture FlagDECAL.dds failed to load
RESMGR: Texture PlayerColor01.tga failed to load
RESMGR: Texture PlayerColor02.tga failed to load
RESMGR: Texture FlagDECAL.dds failed to load
RESMGR: Texture PlayerColor01.tga failed to load
RESMGR: Texture PlayerColor02.tga failed to load
```

#### After: resmgr.log

This capture contains only 3 nonempty rows; all are shown.

```log
RESMGR: Texture FlagDECAL.dds failed to load
RESMGR: Texture PlayerColor01.tga failed to load
RESMGR: Texture PlayerColor02.tga failed to load
```

### `xml.log`

Writer format unchanged by the Python stream wrapper. These are different runs, so existing timing values and messages may differ.

#### Before: xml.log

```log
[30719.718] SetGlobalClassInfo (Civ4EntityEventInfos/EntityEventInfos/EntityEventInfo)
[30719.718] Loading XML file xml\Units/CIV4BuildInfos.xml
[30719.718] Load XML file xml\Units/CIV4BuildInfos.xml SUCCEEDED
[30719.718] SetGlobalClassInfo (Civ4BuildInfos/BuildInfos/BuildInfo)
[30719.718] Loading XML file xml\Units/CIV4UnitInfos.xml
[30719.734] Load XML file xml\Units/CIV4UnitInfos.xml SUCCEEDED
[30719.734] SetGlobalClassInfo (Civ4UnitInfos/UnitInfos/UnitInfo)
[30720.281] Loading XML file xml\Civilizations/CIV4UnitArtStyleTypeInfos.xml
[30720.281] Load XML file xml\Civilizations/CIV4UnitArtStyleTypeInfos.xml SUCCEEDED
[30720.281] SetGlobalClassInfo (Civ4UnitArtStyleTypeInfos/UnitArtStyleTypeInfos/UnitArtStyleTypeInfo)
[30720.281] Loading XML file xml\Civilizations/CIV4CivilizationInfos.xml
[30720.296] Load XML file xml\Civilizations/CIV4CivilizationInfos.xml SUCCEEDED
[30720.296] SetGlobalClassInfo (Civ4CivilizationInfos/CivilizationInfos/CivilizationInfo)
[30720.296] SetGlobalClassInfo (Civ4CivilizationInfos/CivilizationInfos/CivilizationInfo)
[30720.296] Loading XML file xml\GameInfo/CIV4Hints.xml
[30720.296] Load XML file xml\GameInfo/CIV4Hints.xml SUCCEEDED
[30720.296] SetGlobalClassInfo (Civ4Hints/HintInfos/HintInfo)
[30720.296] Loading XML file xml\Art/CIV4MainMenus.xml
[30720.296] Load XML file xml\Art/CIV4MainMenus.xml SUCCEEDED
[30720.296] SetGlobalClassInfo (Civ4MainMenus/MainMenus/MainMenu)
```

#### After: xml.log

```log
[37191.031] SetGlobalClassInfo (Civ4EntityEventInfos/EntityEventInfos/EntityEventInfo)
[37191.031] Loading XML file xml\Units/CIV4BuildInfos.xml
[37191.031] Load XML file xml\Units/CIV4BuildInfos.xml SUCCEEDED
[37191.031] SetGlobalClassInfo (Civ4BuildInfos/BuildInfos/BuildInfo)
[37191.046] Loading XML file xml\Units/CIV4UnitInfos.xml
[37191.062] Load XML file xml\Units/CIV4UnitInfos.xml SUCCEEDED
[37191.062] SetGlobalClassInfo (Civ4UnitInfos/UnitInfos/UnitInfo)
[37191.609] Loading XML file xml\Civilizations/CIV4UnitArtStyleTypeInfos.xml
[37191.609] Load XML file xml\Civilizations/CIV4UnitArtStyleTypeInfos.xml SUCCEEDED
[37191.609] SetGlobalClassInfo (Civ4UnitArtStyleTypeInfos/UnitArtStyleTypeInfos/UnitArtStyleTypeInfo)
[37191.609] Loading XML file xml\Civilizations/CIV4CivilizationInfos.xml
[37191.609] Load XML file xml\Civilizations/CIV4CivilizationInfos.xml SUCCEEDED
[37191.609] SetGlobalClassInfo (Civ4CivilizationInfos/CivilizationInfos/CivilizationInfo)
[37191.625] SetGlobalClassInfo (Civ4CivilizationInfos/CivilizationInfos/CivilizationInfo)
[37191.625] Loading XML file xml\GameInfo/CIV4Hints.xml
[37191.625] Load XML file xml\GameInfo/CIV4Hints.xml SUCCEEDED
[37191.625] SetGlobalClassInfo (Civ4Hints/HintInfos/HintInfo)
[37191.625] Loading XML file xml\Art/CIV4MainMenus.xml
[37191.625] Load XML file xml\Art/CIV4MainMenus.xml SUCCEEDED
[37191.625] SetGlobalClassInfo (Civ4MainMenus/MainMenus/MainMenu)
```

### `LSystem.log`

Writer format unchanged by the Python stream wrapper. These are different runs, so existing timing values and messages may differ.

#### Before: LSystem.log

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
```

#### After: LSystem.log

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
```

## Source references

### Current/local source

- [DLL logger](../../CvGameCoreDLL/CvDLLLogger.cpp), [RNG initialization](../../CvGameCoreDLL/CvGlobals.cpp), [shared filename builder](../../CvGameCoreDLL/CvGameCoreUtils.cpp).
- [Python logging utilities](../../Assets/Python/BUG/BugUtil.py), [shared SAS Python writer](../../Assets/Python/SASPythonLogging.py), [OOS logger](../../Assets/Python/Contrib/OOSLogger.py), [Autolog](../../Assets/Python/Contrib/autolog.py).
- [StatusDump event manager](../../Assets/Python/Contrib/StatusDumpEventManager.py), [older StatusDump helper](../../Assets/Python/Contrib/StatusDump.py), [StatusDump config](../../Assets/Config/StatusDump.xml).
- [Starting-position iteration](../../CvGameCoreDLL/StartingPositionIteration.cpp), [internal profiler](../../CvGameCoreDLL/FProfiler.cpp), [TSC profiler](../../CvGameCoreDLL/TSCProfiler.cpp).
- [Inherited AdvCiv manual](../../_0_Common_Docs/AdvCiv_Base_Doc/manual.txt), especially message-control logging and multiplayer debugging.

### Historical / external provenance

- CivFanatics, 2005-10-28: [launch-era report documenting `PythonDbg.log`, `PythonErr.log` and `PythonErr2.log`](https://forums.civfanatics.com/threads/possible-python-error-temp-solution.134453/).
- CivFanatics, 2005-11-15 onward: [eotinb's original Autolog thread](https://forums.civfanatics.com/threads/mod-log-changes.141164/), explicitly describing a succession-game/note-taking journal and distinguishing it from debug/error logs.
- CivFanatics, 2009-11-05: [Better BTS AI 0.82 decision-logging documentation](https://forums.civfanatics.com/threads/bts-in-game-debug-mode.279903/), including `BBAI.log`, `/DLOG_AI` and interpretation of its timing prefix.
- CivFanatics, 2009-11-15: [BUG 4.2 / BULL 1.0 release](https://forums.civfanatics.com/threads/download-bug-and-bat-here.274636/).
- CivFanatics, 2019-07-16: [Kek-Mod 0.25 release](https://forums.civfanatics.com/threads/modmod-kek-mod.563377/) adding OOSLogger and crediting the idea to Fall from Heaven 2 by Kael.
- AdvCiv Git, 2019-08-27: [`CvDLLLogger` introduction](https://github.com/f1rpo/AdvCiv/commit/553b1a0f8d92b00f09f64f2ee1ecb4d696ebe43e), encapsulating the message-control log.
- AdvCiv Git, 2019-03-27: [OOSLogger merge from Kek-Mod](https://github.com/f1rpo/AdvCiv/commit/81c89a9e0511aa58630f7821fdc9287ecffa8edf).
- AdvCiv Git, 2019-11-05: [TSC profiler import from We the People](https://github.com/f1rpo/AdvCiv/commit/031365f32f6d71519e2fd09c6f6e963ecbef4bd3).
- AdvCiv Git, 2020-05-27: [initial Starting Position Iteration implementation](https://github.com/f1rpo/AdvCiv/commit/a70e4ce3a1db01c51da7fa370a706d8784191b38).
- AdvCiv Git, 2020-11-18: [AsyncRand logging fix](https://github.com/f1rpo/AdvCiv/commit/64e3335495dcd86aebf5068bf8903c0a1f83c508).
- Public RoM-AND/C2C-derived source provides matching producers for `Checksum.log`, `MLF.log`, `XmlDependencyTypes.log`, `CvGlobals_logInfoTypeMap.log` and MLF enumeration diagnostics; these establish external lineage but do not prove which mod produced the user's local copies.
