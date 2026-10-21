# SASGameRecord Revision History

Historical reconstruction from the AdvCiv-SAS commit-diff corpus, reconciled through practical version 6439 (`03b44b8932a0715be234e8c5332fcb8a1d1cf3b2`).

## Revision policy

`SASGameRecord` uses one monotonically increasing **revision** number as a downstream-update signal, not as a compatibility promise. The purpose is simple: an external mod/parser/tool that supports revision N and encounters N+1 knows that the official SASGameRecord implementation changed and should review/update its copy.

- Increment the revision for every intentional SASGameRecord-related code change that a downstream copy may want to review: emitted data/semantics, row/field names, recording conditions, recorder bug fixes, relevant bridges/call sites/configuration/checkers, implementation/refactors/optimizations, and code comments or provenance headers in those implementation files.
  - The goal is deliberately simple: if the official SASGameRecord-related code changed, a copied implementation can notice that there is something new to review.
- Do **not** spend time deciding whether a change is backward-compatible or behavior-changing. When in doubt, increment.
- Do not increment for standalone documentation/history edits, refreshed example logs/screenshots/packages, or unrelated gameplay commits that merely used SASGameRecord as validation evidence.
- Multiple SASGameRecord changes deliberately combined in one Git commit are one revision; this is an update counter, not a hunk/field counter.
- `SAS_GAME_RECORD_REVISION` in `GlobalDefines_advciv_sas.xml` is the single official revision source.
  - The revision is reported as `GAME_RECORD_SOURCE_CONTEXT recordRevision=...` in the SASGameRecord log.
  - The build JSON records build/source/binary evidence separately and has no dedicated recorder-revision field.
  - Treat it as a commit/update label, not a compatibility API: runtime code does not branch on the revision, and Python does not need revision-specific behavior merely because GlobalDefines makes the value accessible.
  - Each qualifying commit increments it exactly once, never more than once; the sequence must not regress or skip numbers. Several recorder changes deliberately combined in one commit still produce one revision.
  - Python/tooling-only revisions can advance without rebuilding an otherwise unchanged DLL. Exact Git/source/build/DLL provenance provides separate evidence about binary/source state.
- The official revision sequence belongs to SASGameRecord. A downstream mod should report the highest official revision it has implemented and should not invent official revision numbers for private extensions.

## Current reconstructed number and first explicit marker

The original reconstruction counted **67 implementation-changing commits** from the feature introduction at SAS practical 6022 through practical 6438.

Under the stricter maintenance rule above, practical 6439 also counts because it intentionally changed the provenance/header comments in the dedicated `sas_game_record_log.py` checker. Practical 6440 then introduced the explicit revision mechanism itself and therefore counts as the next implementation change.

Therefore:

- **Historical state at practical 6438 / Git `6673cf44...`: revision 67.**
- **Historical state at practical 6439: revision 68.**
- **The commit that introduces the explicit revision field/history/check should be revision 69**, because the mechanism itself changes SASGameRecord-related code.

Current emitted source-context field:

```text
GAME_RECORD_SOURCE_CONTEXT recordRevision=160 ...
```

After this, each qualifying SASGameRecord commit increments `SAS_GAME_RECORD_REVISION` in `GlobalDefines_advciv_sas.xml` exactly once and adds one short latest-first entry to this file in the same commit. The revision remains a downstream-update signal; exact runtime source/build identity remains separate provenance in `GAME_RECORD_SOURCE_CONTEXT` and the DLL/build context.

## Consumer/parser notes

- **Runtime rows are the dynamic game record, not an exhaustive static schema manifest.**
  - The build's exact revision/source identity and `GAME_RECORD_LOG_SETTINGS` tell a consumer which implementation/settings produced the file; loaded-XML/rules rows preserve runtime-dependent mappings.
  - An exhaustive per-run list of every row type the source *could* emit would repeat gameplay-independent implementation metadata and still require maintenance for conditional/dynamic `GAME_RECORD_ACTION type=...` families.
  - Keep such schema/consumer guidance in source/docs/checkers rather than enlarging every record with a static manifest unless a concrete downstream consumer later proves that worthwhile.
- **Interpret evidence according to how the row is produced.**
  - Exact/realized action or transition rows are emitted at the live mutation/decision boundary and mean that event actually happened; snapshot/checkpoint rows are observations at the stated boundary and do not imply that unrecorded intermediate state was unchanged; derived/diagnostic fields are explanatory calculations from live state and are not necessarily separately stored gameplay state or causal chooser inputs unless the row/documentation says so.
  - AI-decision rows intentionally preserve selected/realized provenance without promising an exhaustive dump of every rejected candidate.
- **`GAME_RECORD_TURN_COMPLETED` is the authoritative cheap turn-completeness marker from revision 132 onward.**
  - It is emitted once at every enabled log level after `CvEventReporter::endGameTurn`, autoplay/synchronization cleanup and the optional level-3 `END_GAME_TURN` RNG/state checkpoint, but before the game clock advances.
  - In a truncated record, the highest such `turn` is therefore the last game turn known to have reached that boundary; level-3 consumers can additionally use the preceding RNG/state checkpoint for deterministic comparison.
- **Object IDs are namespace-local unless a row explicitly says otherwise.** In particular, Civ4 `cityId` and `unitId` values are player-local; trace them as `(player, cityId)` and `(player, unitId)`, not as globally unique integers. Team/player slot IDs remain their ordinary game slot identities.
- **Missing-value interpretation is field-specific.** `-` is the common textual/not-applicable token, while numeric `-1` is also used by native Civ4 `NO_*` IDs and by some unavailable numeric fields. Do not globally rewrite every `-1` as null; parse the documented field meaning/context.
- **Text-map body rows deliberately remain raw pipe-framed art.** Structured `BEGIN`, `LEGEND`, `LAYER_BEGIN`, `LAYER_END`, and `END` framing rows carry the parseable metadata; revision 122 makes every structured legend/layer frame self-locating by repeating `turn`, while the fixed-width `|...|` drawing rows remain undecorated.
- **Level-3 periodic city detail is checkpoint-plus-delta encoded from revision 123 onward.**
  - Full `GAME_RECORD_CITY`, `GAME_RECORD_CITY_DEVELOPMENT`, `GAME_RECORD_CITY_HAPPINESS`, `GAME_RECORD_CITY_HEALTH`, and `GAME_RECORD_CITY_BUILDINGS` rows establish complete state.
  - Matching `*_DELTA` rows inherit every omitted field from the immediately preceding observation of the same `(player, cityId)` and base family, apply only explicitly present changed fields, and expose `previousTurn`/`fullBaseTurn` so missing chains are detectable.
  - Every new/load log session and every tenth observation returns to a full row.
  - `GAME_RECORD_CITY_TRADE_PARTNERS` remains full.
  - The reference `LLM_Helpers/expand_sasgamerecord_city_deltas.py` can restore these five families to ordinary full rows while validating the chain.
- **Detailed AI-intent coverage follows AdvCiv-SAS-supported gameplay rather than every inherited compatibility mode.**
  - War-plan causal provenance targets foreground UWAI.
  - `GAME_RECORD_WAR_AI_SETTINGS` still identifies inherited K-Mod/Legacy configurations and factual `WAR_PLAN_CHANGED` history remains available, but SASGameRecord does not promise equivalent deep causal provenance for intentionally unsupported Legacy war/peace planning.
  - Deeper rejected-candidate/value decomposition remains BBAI/source-investigation territory.

## Reconstruction method

The history below counts commits that changed the recorder implementation itself (`SASGameSummaryLog.*` / `SASGameRecordLog.*`) or changed runtime/configuration call sites directly participating in the structured record.

It intentionally excludes broad text-search hits that only mentioned the record in documentation/comments, used it as test evidence, refreshed example logs, or changed packaging/tooling without changing the recorder implementation.

This is why the original reconstruction produced 67 revisions through practical 6438 rather than the much noisier ~166 broad-history-hit figure. The stricter current policy additionally counts later intentional code-comment/header-only updates in SASGameRecord-related implementation/checker files.

Because this numbering is reconstructed after the fact, the descriptions are concise summaries of the canonical commit diffs/messages rather than claims that these revision numbers were emitted by historical builds.

## History (latest first)

### Revision 161 - SAS practical 6654

- **Date:** 2026-10-21
- **Git commit:** pending
- **Change:** Correct three recorder accessor misuses exposed by an assertion-enabled Debug-opt run; row layouts are unchanged.
  - Player fingerprints omit the obsolete first-contact field; current contact state is already covered by team fingerprints. Removing that contribution changes player fingerprints.
  - Eliminated-team fingerprints use `NO_ERA` without invoking the era calculation that requires living members.
  - Session, run-status and player-appearance/revival records use AdvCiv's maintained ever-alive player count instead of its initialization-only recounting API.

### Revision 160 - SAS practical 6650

- **Date:** 2026-10-20
- **Git commit:** `2f911392037fe188cb05e91533211e480564533a`
- **Change:** Remove one redundant blank separator in the recorder as part of the source-spacing cleanup; emitted data and recording behavior are unchanged.

### Revision 159 - SAS practical 6646

- **Date:** 2026-10-19
- **Git commit:** `61c86a8949e29f1c6f0cd0379c563c70596d54ca`
- **Change:** Limit shared revision-marker classification to implementation/configuration roots so archive rules, local tests and general workflow-policy edits do not require recorder revisions. Recorder-specific checkers and actual source/configuration changes remain covered; emitted records are unchanged.

### Revision 158 - SAS practical 6644

- **Date:** 2026-10-19
- **Git commit:** `39a7ad075bd95ce94f6bb683b3436da693ec4033`
- **Change:** Emit final writer health only for an opened new/load recording session. Startup reset no longer creates a timestamp-only SASGameRecord file, and repeated finalization no longer duplicates its health row; pending observations and shared-writer retries still drain. See KI#382.3.

### Revision 157 - SAS practical 6643

- **Date:** 2026-10-19
- **Git commit:** `e58d41b6e8a6e6fe69993c4b8aa056f8679d82bd`
- **Change:** Keep build-report evidence separate from SASGameRecord implementation changes when checking revision progression. Source paths and recorded Git status inside a refreshed DLL report no longer request a revision increment; actual recorder, bridge, configuration and checker edits remain covered.
  - Both recorder histories require a nonempty `Change` explanation through shared entry validation; dates, commit labels, hidden comments or fenced templates cannot replace it. A committed bump missing its explanation remains an error even if a later commit supplies the text.

### Revision 156 - SAS practical 6642

- **Date:** 2026-10-18
- **Git commit:** `b0ddbda713d15d04f229a377af1d0c392e886351`
- **Change:** Share UTC formatting and native user-root discovery with the optional current-runtime status publisher while retaining SASGameRecord's existing output semantics.
  - BUG's root handoff updates the replaceable status snapshot independently of historical log handles; BBAI/SASGameRecord retain their committed append root after successful writes.
  - Runtime status includes loaded-DLL identity from the same cached binary context used by historical log headers, so current-state observations can be tied to their actual implementation without enabling those logs. Its paired snapshot/timeline outputs share a separate compiled runtime-recorder revision and history.
  - Runtime status uses finite typed update reasons and states, with enum/name catalogs keeping readable JSON labels synchronized and avoiding string-backed state comparisons.
  - `Logs/SASRuntimeStatus.json` is a fixed-name current observation for agents, enabled independently of the timestamped historical logs and Civ4 INI logging switches. The status snapshot exposes failed-publication counts and links to a per-process activity timeline with monotonic state durations and autoplay end causes; SASGameRecord remains the detailed gameplay/action history.

### Revision 155 - SAS practical 6641

- **Date:** 2026-10-18
- **Git commit:** `7cd0c44ffced52c13505d6a62d411ab4a68178e1`
- **Change:** Add compact shared diagnostic-writer health to existing SASGameRecord lifecycle checkpoints, so live/autoplay analysis can tell whether prompt logging is keeping up rather than inferring health only from file growth.
  - `GAME_RECORD_DIAGNOSTIC_WRITER` follows each `GAME_RECORD_TURN_COMPLETED` marker and is also emitted before session finalization/handle release. It reports current/peak pending text bytes, the current blocked-file count, output failures, retry attempts/successes, and dropped rows/bytes.
  - Process-lifetime `processWrittenBytes`, `bbaiProcessWrittenBytes`, `gameRecordProcessWrittenBytes` and `otherProcessWrittenBytes` count bytes successfully accepted by Windows. They deliberately are not called file sizes: fixed-name output can append to pre-existing files, and external mutation is possible while sharing is enabled.
  - Counters piggyback on writer operations that already occur; no per-checkpoint file-size query, directory scan or per-write GameRecord row is added. Disabled diagnostics create no writer rows/counter work.
  - The health snapshot is taken before writing its own row, so that row appears in the following checkpoint's byte totals. A failed writer can also prevent the summary itself from reaching disk; `SAS_DIAGNOSTIC_DATA_LOSS` remains the direct gap marker and the health row supplements it.

### Revision 154 - SAS practical 6640

- **Date:** 2026-10-18
- **Git commit:** `05d68f1ce1d9a6068254c3b779ef03845797a175`
- **Change:** Make SASGameRecord (and BBAI) readable while Civ4 keeps running, preserving successful final diagnostic rows across ordinary process crashes instead of holding them until a turn boundary.
  - A shared DLL-owned writer submits each row promptly to Windows through persistent read/write/delete-shared handles. This supports live inspection and preserves last-moment evidence useful in investigations such as KI#895/KI#896; Windows handles ordinary disk caching without forced physical-disk writes per row.
    - Existing filenames, `OverwriteLogs` intent, Unicode paths and BUG's `CvAltRoot` are retained. Direct output preserves literal percent signs without the EXE logger's second printf interpretation.
    - Early startup rows wait for BUG's root and are published when it arrives. Failed writes keep an ordered, bounded retry queue; checkpoints retry output and session boundaries close handles. Terminal shutdown permits native fallback when no later BUG handoff remains.
    - Pending or unfinished writes can still be lost on a process crash; power/system failure has no durability guarantee. Readers can retry an incomplete trailing row caught during a write.
  - Shared retry storage is bounded across files; overflow is disclosed with `SAS_DIAGNOSTIC_DATA_LOSS` notices before later accepted rows, preserving the gap's chronological position even when another file exhausted the shared budget. Indexed filename history preserves first-open behavior without a per-row linear scan across repeated loads.
  - SAS logging gates control these logs independently of the INI's `LoggingEnabled`. Disabled logging skips row construction and file I/O; one-time root setup and empty lifecycle checks remain.
  - The VC2003 harness covers immediate visibility, handle reuse/release, failed/partial writes and abrupt process termination without cleanup.
    - The [2026-10-18 Debug-opt comparisons](/_1_AdvCiv-SAS/Docs/README_Logging_Flushing_Comparison.md) support lower overhead under heavy logging: two different SAS48 games with Found, Settler and all UWAI categories at maximum detail took roughly 40% less autoplay time, with matching recorded state checkpoints.
    - A Huge-map run with SASGameRecord alone differed by less than 1%, within possible run-to-run variation; the detailed report retains the measurement conditions and limits.
    - Live reads succeeded while BBAI/SASGameRecord grew during stress autoplay and after completion with Civ4 still open; the old-logger controls also permitted these reads.

### Revision 153 - SAS practical 6639

- **Date:** 2026-10-17
- **Git commit:** `52871e8dcf69034f7b8d83affac2ffe9d3c96af9`
- **Change:** Comparing SAS diagnostics with RI r5642's `RIBugReport.py` highlighted gaps in CustomAssets and BUG-string context, prompting additions suited to SAS's existing session/settings record and privacy controls.
  - SAS already recorded configured `NoCustomAssets`, but lacked evidence of whether the directory was present. `CUSTOM_ASSETS_CONTEXT` supplies that context without collecting personal paths or scanning its contents.
    - Failed inspection is distinguished from absence, so unavailable evidence does not become a misleading diagnosis. Directory presence and configured loading policy do not establish which assets the EXE actually loaded.
  - RI also reports stored BUG strings. `Scores__DisplayOrder` was accepted for completeness; our uncommitted SAS alphabet-filter draft admitted personal text unchanged, so it receives the redaction described below.
    - Session-local replacement IDs preserve evidence that the reviewed value stayed the same or changed without disclosing its text. IDs cannot identify values across sessions; unreadable values remain distinguishable from successfully redacted ones.
    - Default-off `SAS_GAME_RECORD_BUG_STRING_VALUES_ENABLE` allows explicitly opting into exact reviewed values. Unreviewed BUG strings remain excluded.
  - Runtime context matters when comparing the mods: RI's reviewed Python initialization changes cyclic garbage collection during live reload and removes the BUG `Table` finalizer.
    - As of this comparison, SAS uses the inherited AdvCiv/BUG initialization/finalizer. RI's live-reload behavior should therefore not be assumed to apply to SAS; the diagnostic additions here do not change reload handling.
  - Independently, the Python-only additions also exposed an unnecessary coupling: advancing the recorder's update label required rebuilding an otherwise unchanged DLL.
    - Keep the single `SAS_GAME_RECORD_REVISION` label in shared XML so Python/tooling-only updates can advance it without recompilation. It remains an update signal rather than a compatibility switch.
    - With the label independent of DLL compilation, strengthen CI to detect missing or invalid revision changes more reliably while preserving one increment per qualifying implementation commit.
    - Build/source/DLL provenance remains separate evidence for stale or mixed binaries.

### Revision 152 - SAS practical 6637

- **Date:** 2026-10-15
- **Git commit:** `46eeba49f191077a975e9749a1512da75f97cd9f`
- **Change:** Broaden the per-session configured INI context for reproducibility and troubleshooting, retaining configured evidence separately from effective runtime state.
  - Read all sections of `CivilizationIV.ini` and the mod INI, fixing the former `[CONFIG]`-only omission of `[DEBUG]` settings; preserve section/key identity, unusual configured values and explicit missing-key information.
  - Cover reviewed support, graphics/audio, startup and developer settings. Default-off `SAS_GAME_RECORD_INI_PRIVATE_VALUES_ENABLE` and `SAS_GAME_RECORD_INI_NETWORK_VALUES_ENABLE` independently control private/path/free-text and network/account values; withheld keys remain visible, `CheatCode` is recorded directly as a public debug/autoplay setting, and unknown values are not exposed.
  - Collect only at session initialization behind the existing recorder gate, with no recurring INI polling. Disable parser interpolation/list expansion to avoid reinterpreting configured values; recording a value does not imply the EXE accepted it or supports changing it live.

### Revision 151 - SAS practical 6636

- **Date:** 2026-10-14
- **Git commit:** `760f7b2da50b2086722f64b666cbac2d1f26ffaf`
- **Change:** Add a compact level-2+ action/interface trace alongside the independent settings/runtime-view record, distinguishing requests from their results without high-volume input.
  - `SAS_GAME_RECORD_ACTION_TRACE_ENABLE` defaults to 1; setting it to 0 suppresses control/screen/interface/commerce action rows while retaining settings/runtime-view state and unrelated recorder detail. Record the configured switch in GAME_RECORD_LOG_SETTINGS.
  - Native controls record End Turn/forced End Turn/automoves, save/load and selected camera requests; accepted describes the immediate handler gate, not asynchronous completion. All traced controls delivered to doControl also record rejection at its initial gate.
  - Capture active-human commerce rebalancing at the authoritative setter, including every percentage changed by one request.
  - Python advisor/options/WorldBuilder entry points emit SHOW_REQUEST before drawing; registered close callbacks and Sevopedia show/navigation/Back/Forward retain numeric IDs. Native city-screen SHOW/HIDE transitions include owner/city IDs when available.
  - Snapshot final scoreboard state once per new/load/reload session after scroll clamping; record actual scroll/background/expand/autocenter and annotation changes separately from button actions.
  - Pair Ortho, Flying, Mouse Flying and Top-down requests with change-only CAMERA results. One native ControlTypes label list serves requests and camera keys; rotation/flying-mode cycling remain request-only, with no live zoom/map-position polling.
  - Cache levels and the subject switch, pre-gate logging-only reads/formatting, reuse Python wrappers and centralize named Python labels. Preserve Python 2.4 compatibility and declare CvPlayer before its first recorder signature.
  - The existing CI checker enforces shipped defaults: recorder 0, action trace 1, system context 2, and hardware identity/exact paths/process-context opt-ins 0, catching accidentally staged test settings.
  - Runtime checks confirmed turn/automove and save/load requests, commerce changes, scoreboard/annotation controls, advisor callbacks, Sevopedia Back/Forward, city transitions and one scoreboard baseline per tested session; Debug-opt compiled without warnings/errors and PythonErr.log stayed empty.
  - All four camera modes produced both state transitions. Flying required allowflying=1 in CivilizationIV.ini and a restart; with 0, requests were accepted without an immediate state change. Capture AllowFlying in the startup settings baseline; later records may legitimately show 0 when the user does not use Flying Camera.
  - With action recording off at level 3, action-family rows disappeared while Grid/Yields/globe-layer state remained. Rejected requests and no-op scoreboard actions were not specifically exercised; the final rename/folding compiled as Release without warnings/errors, and a post-rename sanity run confirmed revision 151, ACTION_TRACE_ENABLE=1 and emitted action/state rows.
  - Some camera/filtering test logs identify the provisional revision as 152; this work was folded into revision 151 before finalizing the amendment. Buffered final rows can still be lost on a crash; no physical flushing guarantee is provided.

### Revision 150 - SAS practical 6634

- **Date:** 2026-10-13
- **Git commit:** `5248293cd375b2e789e75f493ffa5b1c357959f8`
- **Change:** Add a compact session-configuration baseline and change-only timeline, complementing existing canonical game/map-option rows without per-turn polling.
  - Schema-1 `GAME_RECORD_SESSION_SETTINGS` rows use sorted, percent-encoded values and `old>new` changes for persistent BUG settings, selected Civ4/mod-INI startup keys, and configured Civ4 player/graphics/audio/view profile values.
  - Separate configured player options from the active player's instantaneous effective state. Because the EXE can apply profile options after the GameStart baseline, effective transitions are recorded at `CvPlayer::setOption` instead of being misattributed to a later options-screen exit.
  - At recorder level 2+, add a native live-view baseline and change-only rows for grid, Bare Map, yields, scoreboard visibility, globe view, and the resolved strategy/trade/unit/resource/religion/culture layer; configured profile preferences remain separately labeled. No UI polling or shortcut inference is added.
  - BUG callbacks and Civ4 options-screen exit emit only relevant configured changes; Python reload establishes a fresh baseline. INI files are sampled only at the baseline.
  - Lazy-cache Python recorder levels after first runtime use; native hooks reuse cached integers and pre-gate logging-only state reads/formatting. Initialization checks remain live across new/load sessions.
  - Exclude free-form strings and paths, retaining numeric Field of View; preserve float precision, separate collector success from individual file readability, retain prior callback state after collection failures, and explicitly propagate shutdown/interrupt signals under Python 2.4.

  - Runtime checks confirmed audio/BUG changes, correctly timed initial player-option application, grid/Bare Map/yields/scoreboard toggles, and all six named globe layers across new/load sessions; PythonErr.log remained empty. Globe-view on/off transitions and live camera zoom/scroll tracking were not verified by these samples.

### Revision 149 - SAS practical 6633

- **Date:** 2026-10-12
- **Git commit:** `7238c4c5ed9e5f9c6bc78be847d612c9b8114e90`
- **Change:** Add build-side DLL provenance and targeted graphics/runtime support context motivated by issue-reporting gaps, while reusing the recorder's existing source, binary, install, system and logging diagnostics.

- A configuration-local generated header embeds `BUILD`/`REBUILD`/`UNKNOWN`, pre-build object/PCH state and unfinished `*.@` marker counts directly in the DLL, alongside its existing build-configuration/compiler identity.
  - Generation precedes precompilation and invalidates the one embedding object so even a no-op build refreshes its evidence; artifact presence does not prove reuse.
  - This replaces the initial sidecar design, which required an extra manual copy when installing a DLL; no sidecar parsing, authentication, installation or `linkCompleted` claim remains.
  - IDE builds additionally keep a current full log/JSON pair, including Git identity, result/duration, recognized diagnostics, temporary artifacts and DLL hashes; post-build success/failure evidence stays outside the DLL and recorder.
- Independent runtime project/temp observations report directory presence, output/temp DLL size/FNV-fingerprint states, object/PCH residue, PDB presence and unfinished-marker counts.
  - Fingerprints provide practical identity evidence, not byte comparison or cryptographic authentication. Deleting/replacing build artifacts changes current observations without erasing embedded pre-build facts.
  - Compiler-warning text is not scraped because localization, parallel output and stale logs make it unreliable; failed compile/link operations can leave unfinished markers instead.
- `GAME_RECORD_D3D9_CONTEXT` records the loaded graphics-library identity and coarse origin (`EXE_DIRECTORY`, `WINDOWS_SYSTEM`, `OTHER`, or `UNKNOWN`) after graphics initialization; origin does not identify a particular wrapper product.
- At system-context level 1+, the post-Python lifecycle diagnostic records Render Quality, AA index/sample count, Globe View quality and Movie Quality after GameStart/OnLoad.
  - Level 0 skips profile access. The disabled AdvCiv Graphics Level preset and redundant resolution-profile index remain omitted.
- System-context level 3 retains processor-count/RAM-capacity information; hardware identity, exact paths and process inspection additionally require separate supporting defines, all defaulting off and gating collection itself.
  - Hardware opt-in records CPU vendor/model and Windows-enumerated GPU vendors/models/counts, without claiming which adapter Civ4 uses. Device IDs, serial numbers and hardware UUIDs are not emitted; RAM model collection is not implemented.
  - Path opt-in permits absolute EXE/mod/D3D9 paths. Process opt-in permits priority, affinity, debugger and development-process observations, which describe sampling time rather than permanent state.
- No per-turn hardware polling is added; D3D9 binary identity is cached, while existing source/binary/install/logging/save diagnostics remain complementary context.
  - Wrapper-specific logs/settings and shader-pack fingerprints remain outside the recorder to avoid coupling it to one graphics add-on.

- Centralize maintained samples under `Docs/examples/`: the unchanged raw SASGameRecord log and RNG-comparison report now share `sasgamerecord/`; helper output paths, documentation links and CI references follow the moves. Helper code remains in `LLM_Helpers/`.

Developed with ChatGPT-5.6-Sol and GPT-6.1-Sol.

### Revision 148 - SAS practical 6632

- **Date:** 2026-10-12
- **Git commit:** `9c6c31341fe24736c109520a747e686a0ca60865`
- **Change:** Group recorder checks and behavioral tests by subject, restoring blocking revision-history validation alongside the other retained CI rules and automated helper tests.

- Preserve recorder behavior, emitted fields and the existing revision-update policy; this revision identifies recorder-related checker/test reorganization and restored automation.
- Resolve the history checker's canonical branch once, sharing that result with its unavailable-ancestry notice.

Developed with ChatGPT-5.6-Sol; reviewed and adjusted with GPT-6.1-Sol.

### Revision 147 - SAS practical 6631

- **Date:** 2026-10-12
- **Git commit:** `25cd76ef56f04cce25b06d4c7ab5467d38656d62`
- **Change:** Simplify SASGameRecord CI to disabled logging defaults and revision/history/output synchronization, leaving detailed implementation checks to compilation, runtime testing and review.

- Remove retired recorder source-shape checks and their dependent tests while preserving useful behavioral regression tests for manual runs.
- Retain the existing downstream-update rule: intentional recorder-related code/checker/comment changes increment the revision even when recorded semantics are unchanged; standalone documentation/example/package refreshes do not.

Developed with ChatGPT-5.6-Sol; reviewed and adjusted with GPT-6.1-Sol.

### Revision 146 - SAS practical 6630

- **Date:** 2026-10-12
- **Git commit:** `941d30c1211e95f067bf2df302e17116a0026343`
- **Change:** Make recorder settings and diagnostic vocabularies self-maintaining, and add compact level-3 performance-machine scheduling context without per-turn monitoring or gameplay work.

- Replace the positional `GAME_RECORD_LOG_SETTINGS` format/argument list with one iterable scalar-setting descriptor array that preserves existing getters, caching/clamping policy and field order while automatically formatting names/values.
  - Include the previously omitted scalar `SAS_GAME_RECORD_MAP_ASCII_TERRAIN_UPPERCASE_MIN_NATURE_FOOD_SURPLUS`; validated text-map palette strings remain specialized and are still reported through their resolved legends.
  - CI compares every integer `SAS_GAME_RECORD_*` XML Define against this registry, so a future scalar recorder setting cannot silently disappear from session provenance.
- At system-context level 3, add the quoted CPU brand/model plus a one-shot Civ4 process priority-class and process/system affinity-mask sample. Query failure remains explicit (`-` / availability 0), and lower privacy levels retain the same uncollected sentinels.
- Generate private `BuildingFocusTypes` independent bits and the recorder descriptor table from one local X-macro list while keeping composite `BUILDINGFOCUS_WORLDWONDER` explicit; all inherited bit values, names, iteration order and behavior remain unchanged.
- Generate the existing shared AI-strategy descriptors and enum bits from one canonical list too, retaining the getter/iteration APIs, zero sentinel, bit values and ordering; CI checks both generators and the recorder consumers.
- Give 25 recorder-owned reason/source/action vocabularies shared enum/label lists too, preserving numeric values, schema labels, sentinels and fallback behavior; existing switches/indexed arrays remain compile-time generated with no added runtime setup.
  - Shared Found-log contexts, autoplay-end causes, war-declaration causes, tech-acquisition causes and research-target-change causes use the same approach; legitimate UNKNOWN enum entries retain their existing labels.

Developed with ChatGPT-5.6-Sol; reviewed and adjusted with GPT-6.1-Sol.

### Revision 145 - SAS practical 6629

- **Date:** 2026-10-11
- **Git commit:** `225fc8b07664876a78b51f0bad9b86dea4b9d16c`
- **Change:** Make each SASGameRecord session self-describing about diagnostic/logging overhead, build instrumentation and save-stream format without adding per-turn monitoring.

- Add one finalized new/load `GAME_RECORD_DIAGNOSTIC_CONTEXT` row after Python lifecycle handlers finish, so Autolog running state reflects the current session.
  - Reuse BBAI's iterable registry to distinguish configured levels from effective categories when its master switch is off.
  - Include the BBAI session sequence, timestamped files, score-interval activity and whether the later-city Food-support audit and high-war-utility logging are armed; one supporting-options group shares readers and profile formatting with `BBAI_LOG_SUPPORTING_SETTINGS`, retaining each option's policy and runtime cache.
- Record the DLL-readable CivilizationIV.ini logging state (`MessageLog`, `RandLog`, `SynchLog`, `OverwriteLogs`), map-stat/per-player logging gates, BUG screen/file logging levels and Autolog master/running state. The DLL cannot read the top-level `LoggingEnabled` INI key, so the row records that limitation explicitly rather than inferring a value.
- Extend shared `DLL_CONTEXT` with MSVC version and internal/TSC/AI-autoplay profiler and ReproTest compile switches so performance runs can distinguish instrumented builds as well as Debug-opt/Release/Profile configuration.
- Share DLL/EXE binary-identity formatting and read actual numeric file/product version resources when available; these are binary metadata, distinct from source and save-format revisions. AdvCiv links the DLL resource only for `Final_Release`, so absent resources remain `-`.
- Use one runtime/display row schema at every context level, retaining explicit uncollected fields (`-1` / `-`) while keeping OS/hardware/process queries behind their existing detail gates.
- At system-context level 3 only, extend `GAME_RECORD_RUNTIME_CONTEXT` with debugger presence and a point-in-time boolean sample for `VCExpress`, `devenv`, `MSBuild`, `nmake`, `cl` and `link`. No PIDs, window titles or unrelated process names are recorded, and process presence is not interpreted as an active build.
- Add `GAME_RECORD_SAVE_FORMAT`: the mod/replay `SAVE_VERSION` Define, the current CvGame stream-layout write flag and, for loaded saves, the exact stream flag read from that save. The existing hardcoded write value remains 27 and is centralized as the constant used directly by the writer and diagnostics; serialized bytes do not change.
  - `GAME_RECORD_SAVE_COMPONENT_FORMATS` adds the current write markers for core/AI objects, statistics, UWAI, Rise & Fall and replay records, using one iterable `SASSaveStreamFormat.h` registry shared with their writers.
  - Existing values, encoded marker arithmetic and read/write layouts remain unchanged; CI rejects registered writers that revert to separate literals and recognizable gameplay serializers missing from the registry; XML-info cache writers remain outside its scope.
  - These are current writer markers, including optional inactive subsystems, not values captured from every loaded object or a complete save-schema compatibility certificate; exact source/DLL provenance still identifies the implementation as a whole.
- Keep `GAME_RECORD_LOG_SETTINGS` explicit in revision 145 rather than forcing it into BBAI's homogeneous category-level registry; revision 146 later gives recorder scalars their own lightweight iterable descriptor array while retaining each getter's policy.

Developed with ChatGPT-5.6-Sol; reviewed and adjusted with GPT-6.1-Sol, after the revision-144 Reddit/empire-management audit and the BBAI logging-performance investigation.

### Revision 144 - SAS practical 6628

- **Date:** 2026-10-11
- **Git commit:** `a80bd0e525d5f2a710a15f53d86a5e905b22b048`
- **Change:** Add compact empire-cost/city-management context for expansion analysis without inferring player neglect or logging UI behavior.

- `GAME_RECORD_CITIES` now separates pre-city-modifier maintenance into distance, number-of-cities, colony and corporation components while retaining actual total maintenance, and counts zero-Food-surplus cities, cities with assigned specialists, their overlap, citizen/production automation, and cities whose maintenance is currently waived.
- Level-3 `GAME_RECORD_CITY` exposes the same maintenance decomposition, maintenance-waiver state and the two automation flags; existing Food surplus and specialist fields remain the authoritative city-level state.
- `GAME_RECORD_CITIES_DELTAS` adds actual total-maintenance change so periodic empire-cost movement is visible without comparing full aggregate rows manually.
- `GAME_RECORD_ECONOMY` now exposes gold-commerce/deal income, exact inflated costs, native unit cost/supply, city maintenance, civic upkeep and inflation rate. `inflatedCosts` is reconstructed from the existing gold-rate identity while the economy row reuses the player snapshot's already-computed gold rate, removing the former duplicate Python extra-expenses callback.
- Split the two longest recorder format strings into logical field groups using adjacent C++ literals; emitted rows remain byte-identical.
- These are direct current-state calculations/getters only. No persistent gameplay state, AI choices, RNG, screen-inspection telemetry or inferred "neglected for N turns" state is added; existing happiness timers, production target history and periodic city snapshots remain the appropriate evidence for those questions.

Developed with ChatGPT-5.6-Sol; reviewed and adjusted with GPT-6.1-Sol, from a Civ IV empire-management/administrative-burden telemetry audit.

### Revision 143 - SAS practical 6627

- **Date:** 2026-10-11
- **Git commit:** `f2e68580627b2e6754f0f6663b045210823a0615`
- **Change:** Centralize recorder-only chooser context, diplomacy/deal serialization, and resource-trade bookkeeping in `SASGameRecordLog.*` without changing emitted rows, AI choices, level gates, RNG, or gameplay behavior.

- Move technology/espionage chooser contexts into the recorder header; AI headers retain forward declarations.
- Centralize raw deal/trade-item/diplomacy-event formatting and deal-invalidation capture; existing gameplay call sites and pre-gates remain unchanged.
- Move bonus-trade initialization and winner/runner-up tracking onto its recorder-owned context without adding initialization when logging is disabled.
- Fold the Settler duplicate-destination wrapper into its recorder helper, retaining pre-dispatch timing because founding can consume the unit.
- Keep shared gameplay/BBAI utilities and local decision-data gathering at their existing sites. No row/schema changes beyond the revision marker.

### Revision 142 - SAS practical 6626

- **Date:** 2026-10-11
- **Git commit:** `93847322da1d5e9313f7e4e42d2f5ebfac715899`
- **Change:** Preserve compact founded-site Food-support evidence and fresh quality for one cached alternative at level 2+, following the KI#144.3/KI#178.2/KI#144.4 Settler investigations.

- `GAME_RECORD_CITY_SITE_FOOD_SUPPORT` records player-known 3/6/10/14 worked-plot budgets for the founded site and, when present, the first cached alternative: value-ranked potential/natural Food surplus, food-ranked selected-potential surplus, projected unsupported worked value, and the applied capital Food-support adjustment. City-center yields are free; consumption uses the current engine define.
  - `evaluationAvailable=0` marks a rejected/incomplete rescore; `coreAvailable=0` marks insufficient accepted non-home plots, whose zero surplus placeholders are unavailable. A measured surplus of `-1` is valid; `unsupportedProjectedScore=-1` means the comparison is unavailable/infeasible.
  - Budgets measure sustain-only capacity, excluding growth paths, health/happiness, technology timing and empire-wide shared-plot allocation. Food ranking reorders selected improvements rather than finding the absolute maximum Food; projected gaps are explanatory scores, not additional penalties.
- `GAME_RECORD_CITY_SITE_ALTERNATIVE_QUALITY` compares one alternative's cached value with its fresh score and 1/2/3/6/10/14 plot-value distribution under the same player-known, planned-site-ignoring settings. It is not a path-tested winner or the original decision score.
- The Settler investigations showed that cached rankings and larger-core Food gaps need this context before implying a bad founding choice; numerical examples remain in the referenced KIs. Exhaustive candidate histories and detailed resource components remain BBAI diagnostics.
- All added work remains behind the existing pre-founding level-2 gate: reuse the founded site's player-known pass and rescore at most one alternative; periodic snapshots do no additional evaluation. No new XML settings, serialized state, AI decisions or RNG draws are introduced.

Developed with GPT-6.1-Sol, building on ChatGPT-5.6-Sol and GPT-6.1-Sol's Settler investigation.

### Revision 141 - SAS practical 6617

- **Date:** 2026-10-10
- **Git commit:** `dcf1bb459586b5a9a27dfaf5f158a1f9e6293a29`
- **Change:** Persist and emit exact save-checkpoint identity without adding another autosave system or guessing native Civ4 filenames.

Every `CvGame` serialization embeds a private checkpoint trailer only in the serialized copy of game script data; the trailer is stripped immediately when loading, so Python-visible `getScriptData()` remains unchanged. Checkpoint persistence is independent of SASGameRecord logging so archival saves created with logging disabled remain identifiable later.

The checkpoint records saved turn/elapsed turn, millisecond UTC and both authoritative RNG states, while existing initial map/sync seeds identify the game lineage.

At level 1+, `GAME_RECORD_SAVE_CHECKPOINT_SERIALIZED` records the checkpoint as serialization occurs and `GAME_RECORD_SAVE_CHECKPOINT_LOADED` exposes the same stable `checkpointId` after a later load. Native Civ4 filenames remain unknown at this boundary and therefore keep `filenameKnown=0` rather than being inferred.

SASFastSave always passes its already-chosen basename/reason into the DLL around the synchronous save; fast-save checkpoints therefore carry `kind=SAS_FAST_SAVE`, `filenameKnown=1`, `reason` and `originalBasename` without duplicating Python filename construction, even if SASGameRecord is disabled when the save is created.

Disabled logging skips only the `GAME_RECORD_SAVE_CHECKPOINT_SERIALIZED` output. Loading always extracts any valid current-format trailer to keep Python-visible script data unchanged; if logging is enabled for that later session, the loaded checkpoint can then be reported. Saves without a valid trailer report `available=0`; earlier draft formats are not supported.

### Revision 140 - SAS practical 6616

- **Date:** 2026-10-10
- **Git commit:** `2ffe5568570ef82aa0d514cb316db790e6c6c9bf`
- **Change:** Maintain shared first-city scouting expected-gain summaries and the optional linear-opportunity output only when Settler BBAI level 2+ or SASGameRecord level 2+ consumes them; recorded fields and AI decisions are unchanged.

### Revision 139 - SAS practical 6615

- **Date:** 2026-10-10
- **Git commit:** `38aba896ef201c61bfc2a43f963a5ac105cc42d0`
- **Change:** Added compact realized Settler-scout provenance and rare loaded-transport defender-selection diagnostics without changing AI or combat behavior.

Settler scouting now preserves only realized actions rather than BBAI's rejected-candidate search:

- `GAME_RECORD_AI_FIRST_CITY_CORE_SCOUT_STEP` records the bounded stronger-growth-core investigation that can begin a capital search.
- `GAME_RECORD_AI_FIRST_CITY_SCOUT_STEP` records chosen capital-scout moves with the already-computed settle-now quality, information/upside and delay terms.
- `GAME_RECORD_AI_FIRST_CITY_SCOUT_END` records stops caused by the hard deadline, low expected upside, or absence of a safe scout step.
- `GAME_RECORD_AI_SETTLER_FOG_SCOUT_STEP` separately records later promising-fog detours with the selected-site versus scout-site evidence that caused the move.

At level 3, `GAME_RECORD_NAVAL_TRANSPORT_DEFENDER_SELECTION` is emitted only when a sea attack selects among at least two eligible defenders and at least one is carrying cargo.

The row preserves the chosen ship and best alternative with damage, level/experience/leader state, cargo count/asset value, LFB defender odds/rank, value-adjustment applicability, relative-value/healer/defensive adjustments and the explicit cargo-protection penalty.

It is deliberately diagnostic-only: transported cargo is already excluded from defender candidates and inherited K-Mod LFB already applies loaded-transport protection, so an external report alone is not evidence for changing defender choice.

### Revision 138 - SAS practical 6611

- **Date:** 2026-10-10
- **Git commit:** `2cfce0006eacdadca77c5ab7601287622ca6661f`
- **Change:** Reflowed AdvCiv-SAS custom prose comments in recorder implementation, declarations, call sites and checker code at logical sentence boundaries as part of the shared source-comment maintenance pass.

Preserves technical wording, credits and KI references while separating substantial sentences and joining arbitrary physical wraps. Recorder fields, recording conditions and emitted rows are unchanged; this revision signals the intentional recorder-related code-comment update.

### Revision 137 - SAS practical 6601

- **Date:** 2026-10-08
- **Git commit:** `124cfe178cf1fc151e79faa0f9d62f79ea15cce0`
- **Change:** Following the KI#505.2 Found-logging fix/refactor, review of the resulting diagnostic sample exposed a remaining SASGameRecord gap: later site values did not preserve the original decision that sent a Settler there. Added one compact founding-target decision row to bridge assignment-time values to later founding-time values.

`GAME_RECORD_AI_SETTLER_SITE_DECISION` records level-2+ ordinary `AI_found` destination assignments and retargets, suppressing repeated travel toward the same active target. ChatGPT-5.6-Sol's validation-log review found changed cached values in 31 of 62 matched later foundings, ranging from -720 to +800; preserving the original decision therefore makes later founding-time values interpretable.

It reuses computed raw/scenario-adjusted/path-adjusted scores, path turns and safety, plus maintained shortlist/cache and previous-target context, without additional evaluation, pathfinding or RNG. Logging-only winning-score copies are captured only when level-2+ recording is enabled.

Current-plot rechecks are identified separately; `pathAdjustedScore=-1` means no path-adjusted comparison was used.

The row captures intent before mission dispatch; actual founding remains recorded separately.

Shortlist counts and rejected-candidate reasoning were deliberately not added: they describe the candidate search, which BBAI already covers, rather than the original chosen destination missing from this compact history. Further recorder fields should address a demonstrated missing-history case rather than duplicate detailed BBAI traces.

### Revision 136 - SAS practical 6600

- **Date:** 2026-10-08
- **Git commit:** `89a9db59b34b98a14fe7a9e5f9c08acab9a523d8`
- **Change:** Labeled the recorder's existing known-map and true-map founding rescores for the Found diagnostic redesign, and clarified their hypothetical context. See KI#505.2.

The recorder retains its existing comparison calculations and row layout. Their evaluator traces now use typed contexts converted to `RECORD_KNOWN_MAP_COMPARISON` and `RECORD_TRUE_MAP_COMPARISON` only during enabled output, while ordinary Found diagnostics capture actual evaluations separately.

The revision records the intentional recorder implementation/comment update; it does not signify a new serialized format.

### Revision 135 - SAS practical 6599

- **Date:** 2026-10-07
- **Git commit:** `7c684d203aae30dfc6c8ec15dd1c2179ff1b0463`
- **Change:** Normalized simple explicit diagnostic logging guards and short log-call layout after the caller pre-gating pass.

One-statement BBAI/SASGameRecord/domain logging guards now keep the explicit logging pre-gate and log-call head together, while long argument tails remain multiline. The formatter recognizes logging families rather than only `logBBAI`, but intentionally leaves semantic-only inner predicates, inherited `IFLOG` control flow, and UWAI's separate report system unchanged.

This is formatting/tooling only: statement ordering/behavior, emitted SASGameRecord rows, and log-level semantics are unchanged.

### Revision 134 - SAS practical 6598

- **Date:** 2026-10-07
- **Git commit:** `6b90bb0070a1640f6117e370ed4867141ebbfa5c`
- **Change:** Led independent BBAI/SASGameRecord diagnostic conditions with their cached logging gates and added conservative pre-gate/formatting review helpers.

Disabled diagnostics now short-circuit before unrelated semantic checks wherever the predicates are independent, while existing outer gates continue to skip whole logging-only scans without duplicating per-call checks. This is a pre-gating/readability optimization only: emitted SASGameRecord rows and log-level semantics are unchanged.

The accompanying advisory helpers detect obvious missing/late caller gates and compact simple explicit logging pre-gates without rewriting semantic-only branches or making heuristic style checks blocking CI. Inherited `IFLOG` control-flow macros and UWAI's separate report system remain intentionally outside this revision for dedicated follow-up passes.

### Revision 133 - SAS practical 6588

- **Date:** 2026-10-06
- **Git commit:** `688c7bc797821ef641e6ba5cd5ef0d967b586901`
- **Change:** Aligned production-upgrade telemetry with the KI#882 repair of an inherited BtS destination-production overwrite.

Obsolete-unit production now accumulates into an upgrade target instead of replacing production already stored there. `PRODUCTION_UPGRADED` and interval production-flow rows consequently report destination production preserved by the repair rather than obsolete overwrite-loss fields; the existing level gates and upgrade scan remain unchanged.

See [KI#882](/_1_AdvCiv-SAS/Docs/README_Known_Issues.md#ki-882).

### Revision 132 - SAS practical 6587

- **Date:** 2026-10-06
- **Git commit:** `e3d8fe69e2ea7c6479693d70ee95075a1a66deff`
- **Change:** Added one cheap authoritative completed-turn marker at every enabled level and clarified recorder evidence semantics after a full backlog audit.

`GAME_RECORD_TURN_COMPLETED` is emitted once at the end of each fully processed game turn, after autoplay/synchronization cleanup and the level-3 RNG/state checkpoint when present, but before `incrementGameTurn`.

This gives level 1/2 truncated or crashed records the same unambiguous last-completed-turn boundary without adding a snapshot, state scan, pathfinding, RNG, or other expensive work; level 3 keeps the uniform marker beside its richer checkpoint.

Consumer guidance now explicitly distinguishes exact/realized rows from snapshots/checkpoints and derived diagnostic context rather than widening every runtime row with repeated metadata. The corresponding build check pins the completion marker to its authoritative `CvGame::doTurn` position.

### Revision 131 - SAS practical 6585

- **Date:** 2026-10-06
- **Git commit:** `fdcd91c772f0038430f64288a8d04cc0c9aa5a8f`
- **Change:** Added compact Great-Merchant trade provenance and tightened recurring C++ GlobalDefine caching.

Level-2 Great Person decision rows now preserve the winning Trade Mission city's raw gold, full path length, final target versus current waypoint, and the already-used commerce/continuation/random factors behind the transformed Trade score. Completed Trade Missions additionally record the live trade-profit and unit-formula components needed to explain the final gold payout.

The recorder reuses the existing AI scan and computed values: no candidate dump, repeated valuation/pathfinding, or additional synchronized RNG is introduced, and realized payout anatomy is gathered only for completed level-2+ missions.

Recurring recorder paths now cache stable string-backed GlobalDefine lookups when repeated name lookup would otherwise be paid for each city/action (including `TEMP_HAPPY` and `CAPITAL_TRADE_MODIFIER`). Enum-backed `CvGlobals` define reads remain direct because that overload already uses the global integer cache; one-shot or intentionally mutable reads remain uncached.

### Revision 130 - SAS practical 6584

- **Date:** 2026-10-06
- **Git commit:** `bc01b185a11cb546696af3904d459dd454c74d8c`
- **Change:** Added compact realized AI building-choice history after the building-value and production-policy audit replaced broad hard gates with inherited valuation plus targeted cures.

At level 2+, `GAME_RECORD_PRODUCTION_FLOW` now aggregates actual AI building commitments by loaded building type, chooser origin, regular/National/Team/World-Wonder category, stored/required production and a compact overlapping `aiBuildingChoiceEffects` vocabulary.

Effects distinguish actual health-food relief and starvation prevention, happiness relief, food retention, maintenance/trade/production effects, Government Centers/Palaces/National-Park-style buildings, defense, military/domain production and free experience without widening the interval row for every future mechanic.

The two full validation runs showed that a separate current-health-relief family was structurally identical to health-food relief, so revision 130 retains only the latter.

At level 3, `GAME_RECORD_AI_BUILDING_CHOICE` preserves each realized commitment's helper/direct origin, readable and numeric focus, selected same-pass score when available, actual production progress and city food/happiness/health/maintenance state, projected happiness and both actual-current-bonus and AI-assumed-strategic health changes, loaded-XML yield/commerce/domain/military/economic effects, unit-combat experience, bonus-specific production modifiers and improvement-granted free-specialist potential.

Palace choices additionally identify the current capital being replaced. Helper-only max-turn/threshold/odds/random fields are explicitly `-1` for direct chooser origins rather than overloading one field with unrelated branch-local meanings.

Direct border-culture, proactive-fortification and opportunistic-Wonder branches are included because they bypass `AI_chooseBuilding`.

The recorder reuses the required chooser pass, its winning score and real RNG result. It does not repeat candidate building valuation, placement comparison, threat search, rank refresh or RNG; rejected candidates and exact inherited value-component attribution remain in BBAI.

Every choice call is explicitly pre-gated at the caller: level 2 owns aggregation, level 3 alone constructs readable focus/detail strings, and the emitter intentionally performs no hidden log-level guard/assert. This keeps disabled-path cost and future contract regressions visible at call sites.

While adding the shared focus vocabulary, the inherited local `BUILDINGFOCUS_*` preprocessor constants were replaced with an ordinary local enum and one canonical focus descriptor table, paralleling the earlier AI-strategy diagnostic macro cleanup while keeping the private building-choice mask inside `CvCityAI`.

The same audit consolidated AI-strategy values and raw names into one shared descriptor table. SASGameRecord state hashes, periodic strategy checkpoints, exact strategy-transition rows and the existing strategy UI now iterate that table instead of maintaining separate lowest/highest-bit ranges; adding a future strategy therefore has one canonical diagnostic registration point.

Corrected revision-provenance validation to use the repository's canonical default-branch ancestry instead of feature-branch `HEAD`. This preserves the real mainline hash when a feature branch contains a rewritten or cherry-picked counterpart; local checks visibly abstain from ancestry validation when the default branch is unavailable.

### Revision 129 - SAS practical 6581

- **Date:** 2026-10-02
- **Git commit:** `a3292818a85573483763800cc87f0714e92c74f6`
- **Change:** Added a full-history CI check that recorded revision commit hashes remain ancestors of the checked source, and that hashes from the explicit-marker era identify a source with the matching revision.

Only the latest entry may remain pending, since a commit cannot contain its own final hash; finalized older entries must resolve after amend/rebase.

Strengthened the existing revision/documentation check to reject malformed headings and incomplete latest-entry metadata, with regression tests for a source-only bump, a missing history entry and a stale emitted revision example.

### Revision 128 - SAS practical 6577

- **Date:** 2026-09-26
- **Git commit:** `f48b79828be7be2ea0e6832d066ea7da4bda6c82`
- **Change:** Replaced named metal/mount context in AI bonus-trade and tribute decisions with the buyer's shared player-specific ordinary and short-term trade marginal bonus values, while retaining `iAIObjective` as explicit manual XML-policy context.

The new fields follow current civilization unlocks, technology readiness, substitutes, obsolescence, health and happiness instead of assuming fixed Copper/Iron/Horse/Camel/Elephants families.

The obsolete named-resource configuration was removed after its final gameplay consumer became XML-driven too; `iAIObjective` remains visible because nonzero values now provide an additive shared author/mod-mod override.

### Revision 127 - SAS practical 6576

- **Date:** 2026-09-25
- **Git commit:** `cbdc0dd36c778c694fa8eb5faf62210981c89bb9`
- **Change:** Added compact per-player/per-turn completed Worker-build history so improvement churn and irrigation outcomes remain reconstructible between periodic territory snapshots.

At level 2+, `GAME_RECORD_WORKER_BUILDS_COMPLETED` aggregates the exact realized Builds and improvement transitions, separating new improvements, outright removals and replacements, with replacements also split into worked versus unworked plots.

It also retains direct/within-5-turn completed reversals observed from replacement history available in the current log session, route changes, feature removals, BFC/bonus context, matching bonus-improvement results, irrigation-carrier results and losses, and XML-defined maturation-chain results/replacements.

The reversal fields are per emitted player/turn row rather than cumulative session totals; pre-load replacement history is intentionally not invented.

One row covers all Builds completed by one player in a turn; rejected candidates and numeric AI scoring remain in dedicated BBAI diagnostics.

### Revision 126 - SAS practical 6569

- **Date:** 2026-09-25
- **Git commit:** `6a2f1747805760d905517f0581f689eb80b2027e`
- **Change:** Expanded broad settlement, vassal-freedom and exact-battle context without copying detailed BBAI decision spam.

At level 2+, new `GAME_RECORD_CITY_SITE_QUALITY` rows describe only the site actually founded: coastal/known-versus-omniscient-seafood/area/nearest-city context plus player-known and diagnostic-omniscient founding-time rescored values and `best1/2/3/6/10/14` BFC quality. The existing `GAME_RECORD_CITY_SITE_CHOICE` remains authoritative for the cached shortlist/rank context that led to founding.

One-per-session `GAME_RECORD_VASSAL_RULES` records the loaded freedom thresholds and native master-land semantics. `GAME_RECORD_VASSAL_STATUS` records active relationships at creation and periodic team snapshots with current/stamped land, population ratios, capitulation state and the native relative-size/loss-rule results.

At level 3, exact lethal/nonlethal battle rows also retain the attacker's pre-combat origin and separate river-crossing/amphibious condition-versus-penalty facts. Level-2 luck-only tracking performs no extra tactical-context work.

### Revision 125 - SAS practical 6567

- **Date:** 2026-09-23
- **Git commit:** `6cb9809003a45d95e7fdb0182f42134c7d3247ba`
- **Change:** Routed final SASGameRecord emission through the shared literal-safe diagnostic log boundary so already-formatted percent signs cannot be reinterpreted by Civ4's EXE logger.

`emitSASGameRecordLine` now uses `logSASDiagnosticLiteralLine`, shared with BBAI. The helper doubles each literal `%` only after the recorder's own `CvString::formatv`/row assembly is complete, so dynamic text or future fields containing percent signs reach the log literally rather than becoming a second printf conversion.

This changes no GameRecord schema and adds no work while SASGameRecord is disabled; it hardens only enabled diagnostic emission. Structured fields should still prefer explicit names such as `Percent`/`X100` where that is clearer for machine readers, but that convention is no longer relied upon for memory safety.

See also [KI#375.3](/_1_AdvCiv-SAS/Docs/README_Known_Issues.md#ki-375.3).

### Revision 124 - SAS practical 6565

- **Date:** 2026-09-23
- **Git commit:** `0e0379b78b74cd03827feb1ed1ed94d0b5b93c75`
- **Change:** Generalized periodic territory irrigation reporting from hardcoded Farms to every improvement whose loaded XML sets `bCarriesIrrigation`, and added compact city-network component rows.

`GAME_RECORD_TERRITORY_DEVELOPMENT` now reports irrigation carriers and their irrigated/dry, bonus, BFC, suburb and by-improvement breakdowns. This matches the Worker evaluator's XML-driven irrigation model, so mod-added food, production or commerce carriers remain visible without new C++ type branches.

The same existing territory scan also splits routes on Worker-developable land into periodic BFC and suburb counts, deltas, percentages and route-type lists, making useful city development distinguishable from outer-territory routing.

New `GAME_RECORD_CITY_NETWORK` rows directly list all known cities sharing each `CvPlotGroup` that contains one of the player's cities, along with component plot, route, river-network and terrain-network composition.

The implementation reuses the existing expansion map scan and performs no diagnostic pathfinding; conduit counts describe infrastructure present in the connected component rather than claiming an exact mode for every city pair.

### Revision 123 - SAS practical 6552

- **Date:** 2026-09-21
- **Git commit:** `37448599d5f14a155645fe08838df74f41f9db82`
- **Change:** Added bounded level-3 checkpoint-plus-delta encoding for the five largest repetitive periodic city-detail families while retaining ordinary full rows as self-contained recovery points.

`GAME_RECORD_CITY`, `GAME_RECORD_CITY_DEVELOPMENT`, `GAME_RECORD_CITY_HAPPINESS`, `GAME_RECORD_CITY_HEALTH`, and `GAME_RECORD_CITY_BUILDINGS` now emit a full row on the first observation of each city/family in a log session and every tenth observation thereafter.

Intermediate `*_DELTA` rows keep `(player, cityId)`, `previousTurn`, `fullBaseTurn`, `changed`, and only serialized fields that changed.

New/load log rolls clear every chain; a lifecycle generation guard prevents reused/reacquired city identities from inheriting stale state. `GAME_RECORD_CITY_TRADE_PARTNERS` deliberately remains full because its small payload became larger with delta metadata.

The level-3-only compressor runs after the ordinary city snapshot is already computed/formatted, so levels 0-2 add no city scan, AI query, pathfinding, or RNG work.

`LLM_Helpers/expand_sasgamerecord_city_deltas.py` is the strict reference expander: it validates `previousTurn`/`fullBaseTurn`, fails closed on missing bases/links, copies unrelated rows byte-for-byte, and restores the five encoded families to ordinary full rows for older/simple consumers.

Runtime validation used the actual revision-123 C++ output: 14,835 delta rows and 2,200 full city-detail checkpoints compressed a 59,726,600-byte expanded record to 53,805,598 bytes, saving 5,921,002 bytes (about 9.9% overall).

Against the revision-122 same-parent control, all 425 state checkpoints, all 425 gameplay-significant RNG checkpoints, and all 37,458 pre-existing factual actions matched after ignoring timing/sequence-only fields.

Expansion reproduced the logical city-detail history; its byte-level comparison also exposed a separate pre-existing exact-boundary `CvString::formatv` termination defect, left for a dedicated source fix rather than mixed into this format revision. See also [KI#375.2](/_1_AdvCiv-SAS/Docs/README_Known_Issues.md#ki-375.2).

### Revision 122 - SAS practical 6551

- **Date:** 2026-09-21
- **Git commit:** `21c841ea2b4d431c130489bc0410e3d26a3e326a`
- **Change:** Bundled small SASGameRecord consumer/reproducibility improvements after the broad AI-intent sweep reached saturation: richer opt-in multi-GPU display topology, self-locating structured text-map frames, and explicit supported-scope/parser guidance.

Level-3 `GAME_RECORD_DISPLAY_CONTEXT` now preserves the number of non-mirroring Windows display adapters, desktop-attached-adapter count, recognized vendor-family count, Windows-primary adapter vendor and the existing deduplicated vendor-family list.

This makes hybrid/multi-GPU support cases such as Intel + NVIDIA laptops distinguishable and can help explain graphics/performance anomalies when multiple adapters or drivers coexist (for example a less-suitable or problematic adapter being selected), without recording GPU model names or claiming that the Windows-primary adapter is necessarily the exact Direct3D adapter Civ4 selected. Levels 0-2 retain stable placeholder fields and pay no adapter enumeration.

Structured `GAME_RECORD_MAP_ASCII_LEGEND`, `GAME_RECORD_MAP_ASCII_LAYER_BEGIN`, `GAME_RECORD_MAP_ASCII_LAYER_END` and map-config-error rows now repeat `turn`, matching the existing map-block begin/end chronology and making grep/parser slices self-locating without decorating or widening the raw fixed-width `|...|` drawing rows.

Consumer notes document player-local city/unit ID namespaces, field-specific `-`/`-1` semantics, the choice to keep static schema guidance in docs/source rather than emit an exhaustive runtime manifest, and the deliberate scope boundary that full causal war-plan provenance follows supported foreground UWAI rather than inherited K-Mod/Legacy compatibility modes. No gameplay, synchronized RNG, pathfinding or AI valuation is changed.

### Revision 121 - SAS practical 6526

- **Date:** 2026-09-20
- **Git commit:** `873f0e52db90c8a59576dc4331826646b201d587`
- **Change:** Added compact level-2 `GAME_RECORD_AI_SPACESHIP_LAUNCH_DECISION` provenance for realized AI spaceship launch timing.

The row distinguishes launches made after reaching 100% success from the native urgent-rival override that can launch below 100% when the nearest rival spaceship is due at or before our own travel delay.

It preserves the selected victory, live launch-success percentage, nearest rival team/countdown and whether that rival deadline is at or before our arrival threshold immediately before `launch()` mutates the team's victory countdown/can-launch state. The following factual launch row remains authoritative for our exact travel time.

Recording reuses `AI_launch`'s existing nearest-rival scan and retained gate values and performs no extra rival scan, spaceship valuation or synchronized RNG. With logging disabled, the urgent-rival route still skips the launch-success project scan exactly as before.

Level 2 performs that scan only when needed to classify a realized urgent launch. Existing `SPACESHIP_LAUNCHED` remains authoritative for the actual post-launch countdown, arrival turn and component state; deeper spaceship strategy remains BBAI/source-analysis territory.

### Revision 120 - SAS practical 6525

- **Date:** 2026-09-20
- **Git commit:** `42c5fb7554dfce7d223fc71bd8adef7afb3648de`
- **Change:** Added compact level-2 `GAME_RECORD_AI_RELIGION_SPREAD_TARGET` provenance for realized/new Missionary religion-spread destinations and Missionary airlift reroutes.

Ordinary land targeting preserves the selected religion/city, target scope/player, already-computed player multiplier, path turns and final target score only when `AI_spreadReligion` assigns a new or different `MISSIONAI_SPREAD` destination. Continuing the same long-lived destination stays silent, and the eventual `RELIGION_SPREAD_ATTEMPT` remains authoritative for the actual spread outcome.

The same row preserves the realized target/score when `AI_spreadReligionAirlift` deliberately reroutes a Missionary by air. No religion/player/city multiplier decomposition, rejected-target list, target search, pathfinding or RNG is repeated solely for recording; deeper spread-target scoring remains UnitAI/BBAI/source-analysis territory.

### Revision 119 - SAS practical 6524

- **Date:** 2026-09-20
- **Git commit:** `8a9b6393a942851a05ca6b0086478523460082c5`
- **Change:** Added compact level-2 `GAME_RECORD_AI_COLONY_SPLIT_DECISION` provenance for realized AI colony / split-empire decisions.

The row distinguishes ordinary positive-value colony creation from UWAI's unique forced pre-capitulation colony-liberation path. It preserves the selected area's already-computed aggregate `AI_splitEmpireValue` together with its pre-transfer city/population scale and the parent's total city/population context before `splitEmpire()` moves the cities and obscures that decision boundary.

Recording is reached only after the native positive-value/forced gate and `canSplitArea` checks select the area, immediately before the committed split. The recorder does not rerun `AI_splitEmpireValue`, rescan candidate areas, or duplicate the downstream player/vassal/city/unit consequences; detailed split-value components remain BBAI/source-analysis territory.

### Revision 118 - SAS practical 6523

- **Date:** 2026-09-20
- **Git commit:** `409944dbf0748f03f730afe7688956e5e3426f16`
- **Change:** Added compact level-2 `GAME_RECORD_AI_DRAFT_DECISION` provenance for realized AI city conscription.

The row distinguishes turtle-strategy drafting, local-danger drafting when nearby enemy attack strength exceeds local defense, the final non-critical randomized/value route, and the existing forced-caller API exit.

Ordinary decisions preserve the already-computed conscript unit/population cost, danger and land-war state, good-value/excess-population context, poor worked plots, happiness allowance, unit-cost pressure, and the trigger-specific local-strength or `AI_buildUnitProb` input.

Recording is reached only immediately before an actual `conscript()` call. Rejected/no-draft city turns remain intentionally unlogged; the live poor-plot count is cached once and reused by gameplay/BBAI/recorder consumers; and no draft valuation, local-strength query or synchronized RNG is repeated solely for recording.

Validation also exposed an older factual-source ambiguity: `CvPlayer::splitEmpire` creates free colony defenders through the same `initConscriptedUnit()` helper, so they had been counted and labeled as ordinary conscription.

Unit-completion provenance now uses explicit `PRODUCTION`, `CONSCRIPT` and `COLONY_FREE_DEFENDER` sources; level-2 production flow keeps separate colony-defender count/material/type buckets, while true `AI_doDraft` rows continue to pair with `UNIT_COMPLETED source=CONSCRIPT`.

### Revision 117 - SAS practical 6522

- **Date:** 2026-09-20
- **Git commit:** `5aa5122bee1e089e723de0926a0572209673b0a3`
- **Change:** Added compact level-2 `GAME_RECORD_AI_HURRY_DECISION` provenance for realized AI/automated-governor city hurries.

The row distinguishes panic-forced hurries, population whipping that immediately relieves unhappiness or food loss, and ordinary unit/building value decisions.

Value-based rows preserve the already-computed subjective population/gold costs, final target value, valued overflow and value-minus-cost margin together with live happiness/food context; the following `CITY_HURRIED` action remains authoritative for the actual production/population/treasury/anger mutation.

Recording is reached only immediately before an actual `hurry()` call. Rejected hurry types remain intentionally unlogged, no target/building valuation or synchronized RNG is repeated solely for recording, and ordinary no-hurry city turns pay no recorder-only state collection.

### Revision 116 - SAS practical 6521

- **Date:** 2026-09-20
- **Git commit:** `7b12ec59733e99164ea2badfeac4d35420a97be3`
- **Change:** Added compact level-2 `GAME_RECORD_AI_VASSAL_RESOURCE_TRIBUTE` provenance for the master's automatic vassal-resource chooser.

The row preserves the selected missing resource and its already-computed `AI_bonusTradeVal` together with the runner-up/value margin and eligible-candidate count.

This chooser is separate from ordinary negotiated resource trading: after other diplomacy gates, the master scans resources its vassal can provide, requires the master to have none, and deterministically chooses the highest-valued tradeable resource.

For an AI vassal, the row is emitted immediately before the existing one-sided resource deal. For a human vassal, `delivery=HUMAN_POPUP` records the selected tribute request before `BUTTONPOPUP_VASSAL_GRANT_TRIBUTE`; a later transfer still depends on the human response.

Candidate/runner-up bookkeeping is level-2-only, every `AI_bonusTradeVal` result is reused from the native scan, and no resource valuation, tradeability query, counterproposal or synchronized RNG is added solely for recording.

### Revision 115 - SAS practical 6518

- **Date:** 2026-09-20
- **Git commit:** `f3cf0b1a0a8f07110e2850188f8b25d93402e136`
- **Change:** Added compact level-2 realized `GAME_RECORD_AI_MAP_TRADE_DECISION` provenance for proactive AI map swaps.

The row preserves the two live pre-exchange `AI_mapTradeVal` results: how much the proposer values the target map and how much the target values the proposer map. It also records their value advantage and the strict `DIPLOMACY_VALUE_REMAINDER` threshold applied to the target's received-map value.

A final `MAPS <-> MAPS` package cannot reconstruct those inputs because implementing the deal reveals both maps and changes subsequent map valuation. Existing generic `CONTACT_TRADE_MAP`/deal actions remain authoritative for delivery and payload.

The native contact roll, bilateral trade-denial checks, two map valuations and acceptance inequalities remain unchanged, and no map valuation or synchronized RNG is repeated solely for recording.

### Revision 114 - SAS practical 6516

- **Date:** 2026-09-20
- **Git commit:** `8af8c0f70bc2cbc8bf6d0efaab1f28f35031a6cd`
- **Change:** Added compact level-2 realized `GAME_RECORD_AI_VASSALAGE_DECISION` provenance for AI-originated vassalage and capitulation.

The row distinguishes ordinary `AI_doDiplo` voluntary-vassal contact, UWAI's `tryFindingMaster` third-party protector path, and legacy/UWAI capitulation to the current enemy. It preserves the enemy/war-age context where applicable, the live voluntary-contact probability multiplier/rank inputs, human-capitulation counterproposal result, and final AI-gives/AI-receives package.

Automatic colony/system vassalage is intentionally excluded because it is not an AI diplomatic decision. Existing generic contact/deal/vassalage actions remain authoritative for delivery and state transition, detailed UWAI protector/capitulation reasoning remains in BBAI, and no denial, target search, counterproposal or synchronized RNG is repeated solely for recording.

### Revision 113 - SAS practical 6514

- **Date:** 2026-09-20
- **Git commit:** `0372d02af23b9eb891d1ec7053f0b54c7f5225b4`
- **Change:** Added compact level-2 realized `GAME_RECORD_DEAL_INVALIDATION` provenance for automatic `CvDeal::verify()` teardown.

The row distinguishes a resource-supply deficit, lost trade network, giver- or recipient-side resource obsolescence, AdvC's broke-GPT enforcement, and ordinary peace-treaty expiry. It preserves the exact first failing direction/item plus already-live bonus-count or GPT gold/rate boundary where applicable.

Existing `DIPLO_DEAL_ENDED` actions remain authoritative for the ended deal payload. The live validity tests retain their inherited short-circuit order, and no bonus/network/obsolescence/gold-rate/treaty query or synchronized RNG is repeated solely for recording.

### Revision 112 - SAS practical 6512

- **Date:** 2026-09-19
- **Git commit:** `6784e785cb05d6028454a0dc96a1def3878b286d`
- **Change:** Added compact level-2 realized `GAME_RECORD_AI_DEAL_CANCELLATION_DECISION` provenance for AI deal teardown/renegotiation decisions in `AI_checkCancel` and the later per-rival GPT-cap pass.

The row distinguishes offer-rejection renegotiation, dual-agreement denial, outgoing-resource denial, an incoming resource no longer needed, and GPT-limit cancellation. It preserves the triggering trade item/denial when one exists, the static 20%/40%/immediate dual-denial cancellation chance, and the already-live GPT overdraft/deal amount boundary.

Existing `DIPLO_DEAL_ENDED` actions remain authoritative for the ended deal payload. No offer valuation, denial/resource query, GPT-cap query or synchronized RNG is repeated solely for recording.

### Revision 111 - SAS practical 6509

- **Date:** 2026-09-19
- **Git commit:** `3518d2d93b81fe89b212df40b4cc75dcb3fc8e8a`
- **Change:** Added compact level-2 realized `GAME_RECORD_AI_TECH_TRADE_DECISION` provenance for the three live `CONTACT_TRADE_TECH` outcomes in `AI_doDiplo`: a directly acceptable randomized technology proposal, that same proposal after successful `AI_counterPropose` balancing, and AdvC's separate research-progress technology-for-gold purchase.

The row preserves the net live contact-probability multiplier and known-tech percentage, the pre-locus randomized receive-technology winner and its existing random score, whether that winner was suppressed by the master/vassal-locus preference, the already-selected high-progress fallback technology/raw research-point amount and whether locus policy suppresses it, and the main proposal's selected give technology/recipient-side trade values/value-match delta or the progress purchase's live receive-tech value/max-gold boundary.

Revision 100 remains authoritative for the final package, including counterproposal additions. No technology candidate scan, trade valuation, counterproposal search or synchronized RNG is repeated solely for recording.

### Revision 110 - SAS practical 6508

- **Date:** 2026-09-19
- **Git commit:** `2129ec3e1161b5d872d53e8a89328b844a34f624`
- **Change:** Added compact level-2 realized `GAME_RECORD_AI_GIVE_HELP_DECISION` provenance for the three live `CONTACT_GIVE_HELP` algorithms in `AI_doDiplo`: relation-based resource aid, relation-based technology aid, and ordinary proactive technology aid.

The row preserves which algorithm produced the realized gift together with the already-selected random score and algorithm-specific live gate state: relationship direction and pre-gift bonus ownership for resource aid, tech-score ratio/gift probability for relation technology aid, and the contact-probability multiplier plus asset/attitude gate context for proactive technology aid.

Revision 100 remains authoritative for the actual gifted package. All specialized formatting/derived factual context is level-2 pre-gated, and no candidate scan, trade valuation or synchronized RNG is repeated solely for recording.

### Revision 109 - SAS practical 6503

- **Date:** 2026-09-19
- **Git commit:** `fbe6e098a3c0bc1efb7d2d18701a43641042f558`
- **Change:** Added compact level-2 realized AI help/tribute request provenance around `AI_askHelp` and `AI_demandTribute`, complementing revision 100's final `CONTACT_ASK_FOR_HELP` / `CONTACT_DEMAND_TRIBUTE` packages with the live chooser state and caller origin that disappear afterward.

Help rows distinguish ordinary `AI_doDiplo` from UWAI `amendTensions`, retain the competing best city/technology and their live values, liberation context, whether the existing 2/3 city-preference roll was reached/passed, and the selected item.

Tribute rows preserve the same caller origin, `AIDemandTypes` route, minimum/final deal value, and the already-computed technology/map chooser context where meaningful; revision 87 remains authoritative for detailed resource-candidate selection.

Optional recorder contexts are supplied only at level 2+, and the helpers reuse existing live valuations/results without repeating candidate scans or synchronized RNG.

### Revision 108 - SAS practical 6500

- **Date:** 2026-09-19
- **Git commit:** `1c4cd80537b784a902896235d20833dfd672c7e1`
- **Change:** Added one compact level-2 realized AI-to-human embargo-request provenance row that distinguishes ordinary `CONTACT_STOP_TRADING` initiation from the two intentional `AI_proposeJointWar` fallback redirects that revision 100's generic embargo subject cannot reconstruct.

The row records direct contact versus joint-war attitude/UWAI redirect origin, the actual current worst-enemy embargo target and team-attitude gap used by `AI_proposeEmbargo`, plus the already-selected joint-war target/score and cached UWAI denial when a rejected war request caused the fallback.

The generic revision-100 `CONTACT_STOP_TRADING` row remains authoritative for the actual embargo subject. Recording occurs only after the existing embargo helper has succeeded; direct calls add no chooser work, and the UWAI path merely retains the denial result it already computes rather than evaluating it twice.

### Revision 107 - SAS practical 6499

- **Date:** 2026-09-19
- **Git commit:** `067d8ed44e57c3c5db3e6c0ecad57639a65ba276`
- **Change:** Added one compact level-2 realized AI-to-human joint-war request row at `AI_proposeJointWar`, complementing revision 100's generic `CONTACT_JOIN_WAR` subject with the live cadence and target-selection rationale that cannot be reconstructed afterward.

The row preserves the shared-war-adjusted mean war age used to scale contact probability, personality contact divisor, selected enemy's combined random/war-duration score and derived random component, selected-war age, the human team's peace counter with that target, and whether UWAI's reasonableness screen was active.

It emits only after the existing attitude/UWAI gates have accepted a real request and derives cheap current counters immediately before diplomacy begins; no contact roll, recent-peace gate, target RNG or UWAI denial evaluation is repeated.

### Revision 106 - SAS practical 6498

- **Date:** 2026-09-19
- **Git commit:** `edd5314594c785f02e5d13b79b5e93583cbae6cf`
- **Change:** Added one compact level-2 realized AI joint-war-hiring provenance row at `AI_proposeWarTrade`, complementing revision 100's generic `CONTACT_JOIN_WAR` package with the live target-selection and payment rationale that cannot be reconstructed afterward.

The row records full-UWAI versus legacy selection, the adjusted minimum war-age/cadence denominator, selected war target, UWAI joint-war value or legacy randomized target score, the hireling's cached war price, candidate technology payment, and whether the realized deal used ordinary tech/gold balancing or the rare city/counterproposal fallback.

Ordinary deals also preserve the final two live balance values used by the existing 75% acceptance test; city fallback rows preserve war-success rating, selected city and its existing acquire-minus-keep fitness instead.

The existing generic `GAME_RECORD_AI_DIPLO_CONTACT` row remains authoritative for the final trade package, so revision 106 adds no duplicate trade-list serialization, target valuation, balancing search or synchronized RNG.

### Revision 105 - SAS practical 6496

- **Date:** 2026-09-19
- **Git commit:** `55670b590461b121aa56ea097e17fb6e4ef436c7`
- **Change:** Added one compact level-2 realized AI city-trade intent row at `AI_proposeCityTrade`, covering free liberation, strategic one-way gifts, city swaps/negotiated city-centered packages, and same-team forced completion without pretending they belong to a generic `ContactTypes` class.

The row records the selected sorted-pair rank/count, signed initial cede-value gap, whether selection had fallen back to the inverse gap sign, initial city ids, liberation/evacuation/team/negotiability context, exact formation path, and final AI-gives/AI-receives package.

It emits only after a real human contact or immediate AI deal has been formed and reuses live state already computed by gameplay; no city valuation, `AI_intendsToCede`, counterproposal search or RNG is repeated for recording. Existing canonical deal/offer/rejection rows remain responsible for the downstream outcome.

### Revision 104 - SAS practical 6495

- **Date:** 2026-09-19
- **Git commit:** `f6f3372d294b3660650c6faf95636e6231a8db20`
- **Change:** Removed two exact duplicate Great General Academy/Instructor helper passes accidentally introduced in practical 5081, avoiding repeated city/specialist/building valuation and pathfinding after the identical earlier helper had already failed with unchanged relevant state.

The structured row fields are unchanged, but the live Great General stage vocabulary is reduced from 17 to 15 values: the now-dead revision-103 `SECOND_ACADEMY` and `SECOND_INSTRUCTOR` labels are removed together with the duplicate gameplay stages that could emit them.

CI now guards the 11 live helper-preparation stages, the reduced helper-pass counts and the absence of those deleted labels. This SAS-specific practical-5081 performance/tidiness fix was found while reviewing SASGameRecord revision 103 Great General provenance and is documented as KI#69 Update 2.

### Revision 103 - SAS practical 6494

- **Date:** 2026-09-19
- **Git commit:** `437d6eb6bfe50d1435215c47658f036765d3061b`
- **Change:** Added compact level-2 Great General decision provenance from `AI_generalMove`: the realized ordered policy stage (Instructor, Academy, danger/offensive attachment, progressively looser fallback, retreat/stranded/safety/skip) plus the already-selected helper target, value/path context, attachment criteria, late construct roll and previous mission state.

The helper bridges expose only live results already computed by gameplay and do not repeat candidate scans, valuation, pathfinding or RNG. Also corrected Great General and ordinary Great Person fallback waypoint reporting to use the realized post-helper action plot rather than often-empty or longer-term MissionAI target metadata.

### Revision 102 - SAS practical 6493

- **Date:** 2026-09-19
- **Git commit:** `09b183d2cc6fb69a7d53b0788d7e1692a21f4a83`
- **Change:** Added compact level-2 ordinary-AI Great Person action provenance from `AI_greatPersonMove`: the realized scored action or movement continuation plus the already-computed Slow/Discover/Golden-Age/Trade/Great-Work values, threshold, slow-action target metadata and previous mission state.

Later danger/recon/retreat/stranded/safety/skip fallbacks are explicit without pretending they won the earlier score comparison; Great Generals remain on their separate decision path.

### Revision 101 - SAS practical 6491

- **Date:** 2026-09-18
- **Git commit:** `41e6193015c2c71402eeb453316d1a1d6999c1fa`
- **Change:** Added one compact level-2 realized AI captured-city disposition row for KEEP/RAZE/LIBERATE outcomes, preserving forced reasons or the already-computed normal raze-value component boundaries/random result without repeating conquest valuation.

### Revision 100 - SAS practical 6490

- **Date:** 2026-09-18
- **Git commit:** `a8452813b9aa0819ad20e454d8e7a34cbd4d3d27`
- **Change:** Added compact level-2 realized proactive AI diplomatic-contact provenance: contact class, human-contact versus immediate AI-deal delivery, current attitude, optional non-package subject, and the final AI-gives/AI-receives package.
  - Existing deeper peace and proactive resource-trade rows remain authoritative rather than being duplicated.

### Revision 99 - SAS practical 6489

- **Date:** 2026-09-18
- **Git commit:** `c4cc6993fa06974fa9c91a8c45cde8f5c57c1f5a`
- **Change:** Added compact level-2 AI AP/UN proposal-selection and real-ballot provenance: AI secretaries preserve the selected valid proposal's existing random/victory-boost score, while actual AI ballots preserve a typed live decision cause plus already-computed value/threshold/roll context where available and join directly to the triggered vote by id.

### Revision 98 - SAS practical 6488

- **Date:** 2026-09-18
- **Git commit:** `b6cff05de7004bc0c8039bafbbdc87ef512d7fea`
- **Change:** Added compact level-2 foreground-UWAI war-plan lifecycle provenance for realized preparation conclusion/cancellation, target switching, active plan-type switching, imminent-war cancellation and victory-denial direct declaration, reusing already-computed UWAI values immediately before the authoritative mutation.

### Revision 97 - SAS practical 6487

- **Date:** 2026-09-18
- **Git commit:** `c154939e23a030b1654b40ac4fbf5c492c2add65`
- **Change:** Added exact level-2 per-player/per-area AI target-city history for normal area search, K-Mod random clearing, diplomacy coordination, area reassignment and effective clears when a referenced city is removed, plus compact periodic target checkpoints with the city-target refresh timer.
  - Normal search exposes its already-computed winning target value through an optional diagnostic output; old-target provenance remains level-2 gated without duplicating the refresh loop.

### Revision 96 - SAS practical 6486

- **Date:** 2026-09-18
- **Git commit:** `86485bdd6be96fcb0c3bb7d92ad6885a0fc197a2`
- **Change:** Added exact level-2 AreaAI theater-posture transitions from the two authoritative writers (`CALCULATED` and UWAI land/naval alignment), plus compact periodic non-neutral AreaAI checkpoints and a shared canonical raw enum-name helper.
  - Initial `NO_AREAAI` setup assignment is omitted from transition history; disabled logging keeps the original inner calculation loop.

### Revision 95 - SAS practical 6485

- **Date:** 2026-09-18
- **Git commit:** `084b2bc93ce3734416f2a205eac6b035b3fe7c2d`
- **Change:** Extended the opt-in level-3 active-viewer trade-market snapshot with `GAME_RECORD_TRADE_STRATEGIC`: exact bilateral denial state for maps/vassalage/capitulation/open borders/defensive pacts/permanent alliances/disengagement, city ids the rival will currently cede or refuse with denial reasons, and Glance-style third-party war-for-hire targets/denials.

The scan reuses the existing Foreign-Advisor market gate and viewer scope rather than evaluating every possible observer. Derived willingness is periodic rather than presented as an exact transition because Civ4 computes these queries on demand instead of storing one authoritative willingness state.

### Revision 94 - SAS practical 6484

- **Date:** 2026-09-18
- **Git commit:** `da80e04a653cfed2ec20621d9b8c6185acf33903`
- **Change:** Added complete periodic level-3 AI attitude-component provenance.
  - `GAME_RECORD_DIPLO_ATTITUDE_BREAKDOWN` reconstructs each known cross-team major-AI relationship from the same additive components and order used by `CvPlayerAI::AI_updateAttitude` (first impression, rank/team size, close borders, peace/war, religion, resource/open-border/defensive-pact effects, rival pacts/vassals, expansionism, shared war, favorite civic, trade/rival trade, aggregate memories and attitude-extra), plus the pre-clamp component sum, its normal -100..100 clamped value, cached raw value and gameplay-effective value.

This makes fluctuating diplomacy such as different religion ("heathen"), open borders and fair trade explainable from the record rather than exposing only the final attitude number.

The existing `GAME_RECORD_DIPLO_MEMORIES` row remains the per-memory detail and now shares the same memory scan with the broader breakdown instead of recomputing it separately. Full breakdowns are level-3-only and limited to gameplay-active major AI players (including AI Auto Play); ordinary human outgoing pseudo-attitudes and same-team forced relations are not presented as additive AI reasoning.

No extra attitude work is added when level 3 is disabled.

### Revision 93 - SAS practical 6483

- **Date:** 2026-09-18
- **Git commit:** `56f4bce56342897f452363f8f6a52a31c8e52b1c`
- **Change:** Added exact level-2 team worst-enemy transition history.
  - `GAME_RECORD_WORST_ENEMY_CHANGE` records only the final committed `AI_updateWorstEnemy` replacement as `oldEnemyTeam` -> `newEnemyTeam`, together with the old/new enmity values already computed by the real selection pass plus `recursiveRecheck=0/1`.
  - AdvCiv's optional recursive reevaluation after enemy-trade-memory decay therefore cannot emit its tentative first-pass candidate; the recursive final pass commits and records the authoritative result.

The bridge adds no enemy scan, attitude/enmity evaluation, RNG or pathfinding solely for logging. Existing periodic `GAME_RECORD_DIPLO_STATUS worstEnemyTeam=` fields remain the checkpoint for loaded saves and truncated logs.

### Revision 92 - SAS practical 6482

- **Date:** 2026-09-18
- **Git commit:** `d4a67ae0effbeaf879061e9f24c4382da83a7efe`
- **Change:** Added exact readable AI victory-route stage history at level 2.
  - `GAME_RECORD_AI_VICTORY_STAGE_CHANGE` records each final Culture/Space/Conquest/Domination/Diplomacy 0..4 stage transition after `AI_updateVictoryStageHash` finishes, including authoritative resets caused by invalid/capitulated/no-capital state; the existing periodic `GAME_RECORD_AI_VICTORY_STAGES` row remains the loaded-save/truncated-log checkpoint.
  - The bridge compares only the already-computed before/after victory-stage bitfields and repeats no victory evaluation, RNG or pathfinding.

The periodic and exact rows now share one AI/AI-Auto-Play eligibility helper. The revision checker also verifies the revision history's current emitted `recordRevision` example against the source constant so that descriptive marker cannot silently lag after a revision bump.

### Revision 91 - SAS practical 6481

- **Date:** 2026-09-18
- **Git commit:** `9a2348b4029ec5e4bc1c609e00018235d62d0123`
- **Change:** Added complete readable AI-strategy history at level 2.
  - `GAME_RECORD_AI_STRATEGY_CHANGE` records each final gameplay-active non-default `AIStrategy` bit that starts/stops after `AI_updateStrategyHash` finishes its validity cleanup, while periodic `GAME_RECORD_AI_STRATEGIES` checkpoints preserve the complete active set without requiring replay from turn 0.
  - Existing specialized decision rows remain for local context and BBAI retains its detailed K-Mod transition diagnostics/trigger values.

A shared `getSASAIStrategyType` helper in `CvGameCoreUtils` now provides canonical raw enum names to both logs, replacing the fragile BBAI stringification macros without changing their emitted strategy names.

Existing SASGameRecord strategy predicates share one AI-Auto-Play-aware wrapper, and the build check guards the contiguous strategy-bit layout, enum-name coverage, and all complete strategy scans against future `AIStrategy` additions.

No AI strategy evaluation, RNG or pathfinding is repeated solely for recording.

### Revision 90 - SAS practical 6480

- **Date:** 2026-09-17
- **Git commit:** `e25c3e02b18e1252d873e5b5eb978edf40225d37`
- **Change:** Source-readability maintenance only.
  - Oversized `logSASGameRecord(...)` argument lists are wrapped at top-level argument boundaries while their format strings remain unchanged.
  - A conservative helper now preserves this argument-layout convention alongside the matching BBAI cleanup while deliberately leaving existing prose-comment layout untouched.
  - Emitted SASGameRecord row text, fields, conditions, and gameplay behavior are unchanged.

### Revision 89 - SAS practical 6477

- **Date:** 2026-09-17
- **Git commit:** `18e8765652b15b1726e3c97724f2a150656ef6ad`
- **Change:** Tightened disabled-recorder overhead after the expanded provenance work. `SAS_GAME_RECORD_LOG_LEVEL` is now cached once after all GlobalDefines/module overrides load, making the widespread level gates direct integer reads instead of out-of-line getter calls in ordinary non-LTCG Release builds.

Caller-gated plot/random-event placeholder states no longer initialize unused fields at level 0/1, the plot-owner cause scope avoids an unused disabled-path member write, and unit-event before/after snapshots now live wholly inside the enabled branch.

Tiny empty vectors that must span gameplay mutation remain intentionally accepted and locally documented; they perform no recorder scan/allocation/population while logging is disabled.

Emitted record semantics are unchanged.

### Revision 88 - SAS practical 6476

- **Date:** 2026-09-17
- **Git commit:** `d8a8df2f2dc046fa57ba8d4679e7d98cf130ab33`
- **Change:** Added compact AI civic/revolution provenance.
  - `GAME_RECORD_AI_CIVIC_DECISION` records live `AI_doCivics` switch candidates from the already-computed current/best civic values, percent threshold, absolute/anarchy slack, bundle-anarchy state and first/recheck pass; accepted candidates are level 2 while rejected/wanted-but-rejected candidates are level 3.
  - `GAME_RECORD_AI_CIVIC_OUTCOME` records meaningful accepted-bundle outcomes: actual revolution, delaying for an imminent researched civic, insufficient-gold blocking, or an unexpected final `canRevolution` block, including the pending/final civic bundle and real timer/anarchy/research/gold context; `BLOCKED_GOLD` also records the net deal gold-per-turn actually used by its reserve formula.
  - No civic valuation or revolution eligibility search is repeated solely for recording.

The same call-site review also gates older BBAI-only financial-trouble/war-state diagnostic lookups so ordinary player-log-disabled gameplay no longer performs that diagnostic work.

### Revision 87 - SAS practical 6475

- **Date:** 2026-09-17
- **Git commit:** `727e094fdc31b3e785825433407b47bacdeccaf8`
- **Change:** Added compact AI resource-trade choice provenance.
  - `GAME_RECORD_AI_BONUS_TRADE_DECISION` records the real proactive `AI_proposeResourceTrade` receive/give chooser winners and runner-ups from already-computed buyer trade value, seller keep-value estimate, bias, probability gate and random final score, then joins them to the actual counterproposal terms that are about to be shown to a human or implemented between AIs.
  - Winner-only factual resource/strategic context is gathered only when an event is emitted; the shared `AI_bonusTradeVal` function itself remains uninstrumented.

`GAME_RECORD_AI_BONUS_DEMAND_DECISION` separately records the different tribute chooser's randomized sorted winner/runner-up, non-surplus sorting adjustment, selected resource bundle/value, minimum threshold and final deal valuation. Existing periodic `GAME_RECORD_TRADE_MARKET` remains the resolved availability/denial/price view.

The new rows repeat no bonus valuation, counterproposal search or RNG solely for recording.

### Revision 86 - SAS practical 6474

- **Date:** 2026-09-17
- **Git commit:** `318c4c070c954a548fd44ae819b59819981d3e1f`
- **Change:** Added compact AI research/free-technology choice provenance.
  - `GAME_RECORD_AI_RESEARCH_DECISION` identifies teammate coordination, Python overrides and the real `AI_bestTech` fallback; the latter preserves the chosen/requested technology, actual current research after queueing, deeper strategic aim, chooser/path values, distinct runner-up and chooser breadth from values already computed by gameplay.
  - Level 3 additionally serializes each distinct returned-tech candidate with its chooser/path value and deeper aim, while free-tech choices use the same depth-1 machinery.
  - No technology valuation or chooser RNG is repeated for recording.

The technology-path diagnostic also recovers the actual deepest endpoint after K-Mod backfills/reorders paths with unrelated depth-0 technologies, rather than assuming `path.front()` is always the long-term aim; AdvCiv's rare scenario fallback likewise reports the path actually selected rather than always path 0. These are diagnostic/provenance corrections only and do not change research selection.

### Revision 85 - SAS practical 6473

- **Date:** 2026-09-17
- **Git commit:** `f94667171d4523a595a9153e253ad68c5486680b`
- **Change:** Added compact AI espionage mission-choice and mission-phase interception provenance.
  - `GAME_RECORD_AI_ESPIONAGE_DECISION` retains the scored chooser winner/runner-up and both candidates' already-computed valuation/randomization/overhead/cost components plus chooser risk/EP/strategy context without rerunning valuation or RNG; the separate K-Mod tactical city-revolt path emits `GAME_RECORD_AI_ESPIONAGE_TACTICAL_DECISION` with its factual remaining-defense gate instead of fabricated scores.

`GAME_RECORD_SPY_INTERCEPTION_CHECK` records every real before/after-mission base/final chance and authoritative result/draw plus counterespionage, counter-Spy-defense/Spy, city-defense and recent-mission context; successful non-interceptions are therefore visible alongside existing completed-mission and caught-Spy outcomes.

The ordinary logging-disabled/travel-interception path keeps the original `SyncRandSuccess10000` behavior, and logging-only plot/unit scans remain level-2 gated.

### Revision 84 - SAS practical 6472

- **Date:** 2026-09-17
- **Git commit:** `b93cfcd8ae7a6279729c067373532f66d884ea8d`
- **Change:** Added compact AI corporation operational provenance.
  - `GAME_RECORD_AI_CORPORATION_DECISION` records actual Executive production commits, new/retargeted `AI_spreadCorporation` destinations and deliberate Executive airlift/sea-transport routing with only already-computed chooser scores/path context; no corporation valuation or pathfinding is repeated for logging.
  - Periodic `GAME_RECORD_CORPORATION_POSTURE` separates policy-enabled corporations from actually present corporation types/city instances/headquarters and preserves persistent Executive spread missions, including Executive cargo carried by sea transports, at-target/waiting-gold/ready state and level-3 compact targets across loaded saves; it stays absent before meaningful corporation/Executive state exists.

### Revision 83 - SAS practical 6471

- **Date:** 2026-09-17
- **Git commit:** `23a3c7abe45b7f775a7c03f4ab904fe8a31bb1a3`
- **Change:** Added explicit AI fog-control / anti-Barbarian map-control provenance.
  - `GAME_RECORD_AI_MAP_CONTROL_DECISION` records real `AI_guardCitySite` and `AI_guardYield` assignment/retarget changes with their already-computed site/guard/yield, defense/sight/fog-distance, path and action context, without extra target search, pathfinding or fog scans.
  - Periodic `GAME_RECORD_BARBARIAN_PRESSURE` also identifies persistent city-site `MISSIONAI_GUARD_CITY` posture, including level-3 site/guard/current coordinates, so loaded saves retain ongoing fog-control state.

### Revision 82 - SAS practical 6470

- **Date:** 2026-09-17
- **Git commit:** `a864aa13593c9e80df49d4ff8967b7139bd8d00d`
- **Change:** Added compact AI religion-choice provenance at the real `AI_doReligion` boundary.
  - Level 2 records meaningful switch/spread-block decisions with the pre-spread best and runner-up post-bias scores, current/selected religions, favorite/missionary/flavor context, best-religion spread/holy-city state, the raw values actually used by gameplay for switch probability, the real conversion probability/roll and realized outcome.
  - Level 3 additionally serializes every eligible religion's already-computed chooser score.
  - The recorder performs no extra religion valuation or RNG and suppresses routine keep-current evaluations to avoid per-turn noise.

### Revision 81 - SAS practical 6469

- **Date:** 2026-09-16
- **Git commit:** `3bcaa9af2c70561ee62b2c60e938cce216949849`
- **Change:** Added compact peace-decision provenance and knowledge-limited barbarian pressure.
  - `GAME_RECORD_AI_PEACE_DECISION` records the real shared peace-negotiation boundary at level 2: UWAI versus legacy source, war age, initial/final end-war benefit values, provisional reparations, human-counterproposal use, final trade lists when available, and resolved outcomes such as victory-denial block, insufficient reparations/counterproposal, offer sent or AI-AI peace implemented/rejected.
  - It reuses the live negotiation values/package rather than rerunning UWAI or trade valuation.

`GAME_RECORD_BARBARIAN_PRESSURE` complements the omniscient global barbarian rows with what each player's team can actually know: visible/non-invisible barbarian units and city proximity, revealed/visible/actionable-known barbarian cities, explore/city-attack UnitAI state, `MISSIONAI_EXPLORE` groups, and the AI's existing barbarian-defense-focus/attacker-need predicates.

`BARBARIAN_UNIT_SPAWNED` also records plot ownership/terrain/feature, revealed/visible major-team counts and the nearest major-civilization city.

The new observations add no spawn reevaluation, pathfinding or barbarian target scoring.

### Revision 80 - SAS practical 6468

- **Date:** 2026-09-16
- **Git commit:** `1cb708ab8dfc36d7200bd0b4e02cd70c4d6f62b1`
- **Change:** Added privacy-tiered runtime/install provenance and compact naval operational intent.
  - `GAME_RECORD_EXE_CONTEXT` fingerprints the loaded `Civ4BeyondSword.exe`; level 2 records Windows/Wine runtime/build/update and process/native architecture plus coarse distribution/Steam AppID-build-beta metadata, while level 3 additionally records coarse CPU/GPU vendors, logical processors/RAM and exact EXE/mod paths.

`GAME_RECORD_DISPLAY_CONTEXT` now retries after loaded-save graphics initialization so short sessions do not omit display/GPU context.

Level-3 naval posture also records each assault group's existing MissionAI destination, target team/city, cargo/capacity, support/base and ocean capability without new pathfinding or AI evaluation; war-start context now reuses the shared spaceship-parts-percent helper.

Note: Steam detection/path/build metadata are runtime-validated; GOG, Firaxis retail/other and Wine/Proton branches are source-reviewed but not yet runtime-tested.

### Revision 79 - SAS practical 6465

- **Date:** 2026-09-16
- **Git commit:** `439081be51b7ec481f72cda74688fce4ca8b5b33`
- **Change:** Added explicit territory/BFC/suburb unimproved-development totals and level-3 `GAME_RECORD_CITY_DEVELOPMENT` rows with owned/assigned-radius gaps, development/improvement coverage, remaining feature/resource types and bounded plot coordinates.
  - Level 3 also serializes each already-maintained AI city-site shortlist as rank/coordinates/current found value.
  - These are descriptive/cached-state additions only: no Worker build-legality/value scan, city-site reevaluation or pathfinding.

### Revision 78 - SAS practical 6464

- **Date:** 2026-09-16
- **Git commit:** `fb4c3dcb0b8d8fe9f085620a7bdcaf2ea059bc63`
- **Change:** Added compact strategic AI intent: current team war plans/ages, remaining high-level military-strategy flags, selected UWAI war-target context, and level-3 best-target `DEFERRED_ROLL` events without logging exhaustive rejected candidates.
  - Espionage snapshots now distinguish foreign Spy locations from stored attack/recon/guard MissionAI intent.
  - The RNG/state comparator also requires equal explicit `recordRevision` values before recipe-sensitive combined interpretation.

### Revision 77 - SAS practical 6463

- **Date:** 2026-09-16
- **Git commit:** `989eb50ce43e551708d7f573db4b140e28902307`
- **Change:** Added compact expansion/city-site intent: maintained shortlist/threshold/primary-site context, settler `MISSIONAI_FOUND` targets/ranks/cached values, normal founding choice plus two cached alternatives, and exact barbarian chooser winner/two runner-ups with raw/area/final values and random/site-spacing context.
  - All reuse already-computed state/scans, adding no map-wide rescoring or pathfinding; also removed a duplicate `CvInfo_Symbol.h` include.

### Revision 76 - SAS practical 6461

- **Date:** 2026-09-14
- **Git commit:** `66f72899345d54e552c3a40da59b26d7cbdac270`
- **Change:** Strengthened runtime provenance with a compact tracked code/rules/scripts diff fingerprint and byte count in `GAME_RECORD_SOURCE_CONTEXT`, scoped to `CvGameCoreDLL`, `Assets/Config`, `Assets/Python`, `Assets/XML`, and `PrivateMaps`.
  - This distinguishes different edits to the same runtime-relevant dirty paths and, together with the independent exact-byte loaded-DLL fingerprint, makes stale/unrebuilt DLL mistakes directly visible without hashing docs/LLM helpers or adding per-file timestamp noise.

### Revision 75 - SAS practical 6460

- **Date:** 2026-09-14
- **Git commit:** `40b3580cfa826150dfdb659452ec9f9fcf85cbd3`
- **Change:** Added compact periodic naval-assault posture covering assault lift/capacity/loading, current open-ocean capability, fleet-group loading/base-location/mission/support state and exact transported unit composition; also recorded the SAS UWAI naval-logistics deployment gameplay toggle in the canonical war-AI settings row.

### Revision 74 - SAS practical 6455

- **Date:** 2026-09-14
- **Git commit:** `320e73215cf08fa5b5af9fb4231c612a7cebf8f8`
- **Change:** Added one compact all-city `CITY_PRODUCTION_NO_TARGET` outcome at control-path-aware turn boundaries.
  - Manual human cities are sampled before end-turn production processing, while AI-controlled and production-automated cities are sampled afterward; disorder is suppressed, and detailed legality/chooser-path evidence is kept in the dedicated BBAI category.

### Revision 73 - SAS practical 6446

- **Date:** 2026-09-12
- **Git commit:** `f8329120b31aad17a867ff5d1a679b474640108e`
- **Change:** Canonicalized invalid `TEAM_MET` first-contact coordinate pairs as `-1,-1` instead of serializing values that are undefined when their validity flags are false; ordinary valid coordinates and gameplay behavior are unchanged.

### Revision 72 - SAS practical 6443

- **Date:** 2026-09-10
- **Git commit:** `8ee577a98bdb2b362b20c3fbe68cdcd52c8521b4`
- **Change:** Added exact level-3 plot-ownership transition history with conservative immediate mechanism/source provenance, expanded causal transactions across city founding/culture expansion/flips, war/peace, vassalage and direct Python/WorldBuilder ownership chains, and extended CORE plot fingerprints with contested second-owner/forced-unowned state.

### Revision 71 - SAS practical 6442

- **Date:** 2026-09-10
- **Git commit:** `ac49c56121108e58b021d9bd127440ad03d68700`
- **Change:** Added compact level-3 semantic CORE state checkpoints at the existing authoritative-RNG lifecycle boundaries, with component/combined fingerprints, latent gameplay/AI state and measured computation time; extended the RNG comparison helper to validate and localize first RNG-versus-state divergence.

### Revision 70 - SAS practical 6441

- **Date:** 2026-09-10
- **Git commit:** `e7c316b1988fca98dc8baa1271ab47049b26d2d0`
- **Change:** Added session-local monotonic `seq` identity to every emitted structured `GAME_RECORD_*` row and generic causal `tx` scopes, initially bracketing city acquisition/raze so filtered records retain exact chronology and related city-transfer consequences remain explicitly joinable.

### Revision 69 - SAS practical 6440

- **Date:** 2026-09-10
- **Git commit:** `9337aece920266486be61a7bf60ec9f96e406bb9`
- **Change:** Added the explicit emitted `GAME_RECORD_SOURCE_CONTEXT recordRevision` field, public `SAS_GAME_RECORD_REVISION` constant, committed revision history, maintenance guidance, and consistency checks so downstream copies can cheaply detect any later official SASGameRecord-related code update.

### Revision 68 - SAS practical 6439

- **Date:** 2026-09-10
- **Git commit:** `03b44b8932a0715be234e8c5332fcb8a1d1cf3b2`
- **Change:** Standardized provenance headers across active AdvCiv-SAS files, including the SASGameRecord implementation, comparison helper and dedicated build checker.
  - Runtime recorder behavior was unchanged, but the SASGameRecord-related code artifacts changed and therefore count under the current strict revision policy.

### Revision 67 - SAS practical 6438

- **Date:** 2026-09-10
- **Git commit:** `6673cf44c57730764b1da664e24673c9bd68a835`
- **Change:** Added level-3 authoritative map/synchronized RNG checkpoints with call/fingerprint/stream provenance, corrected session-rollover finalization, and added the SASGameRecord RNG comparison/validation helper.

### Revision 66 - SAS practical 6437

- **Date:** 2026-09-10
- **Git commit:** `c6d05a34b95b9da2df330d9c4ab537527310933c`
- **Change:** Added independently tunable 25%/50% initial geography overviews and an optional one-cell-per-real-plot native geography map with explicit extents.

### Revision 65 - SAS practical 6434

- **Date:** 2026-09-09
- **Git commit:** `04ed39adfe019a482136a72b4903c1c667d46bb8`
- **Change:** Expanded realized random-event and nuclear consequences with per-city/per-unit nuke effects and accepted EventInfo city/unit/player/building-modifier before/after consequences.

### Revision 64 - SAS practical 6432

- **Date:** 2026-09-09
- **Git commit:** `6efad6885ae140d115578b91291e2a2539fdef9d`
- **Change:** Added financial-strike state and exact strike-turn actions, disorder-city counts, angry/working population, and clearer assigned/free-specialist state and aggregates/deltas.

### Revision 63 - SAS practical 6431

- **Date:** 2026-09-09
- **Git commit:** `22aec5ffbb588a2d280e8a68480d1195642c3c87`
- **Change:** Added realized Civ4 random-event lifecycle history: delivered triggers, selected EventInfo/application state, target validity, gold/tech/pillage/free-unit/follow-up results, occurrence clearing and meaningful expiry, while excluding speculative AI valuation.

### Revision 62 - SAS practical 6399

- **Date:** 2026-09-04
- **Git commit:** `11fb13b1d07d8d706b5a41b57409c57de60636c4`
- **Change:** Optimized SASGameRecord implementation overhead by pre-gating recorder-only calculations, caching predicates and avoiding redundant production/bonus/trade/Worker/Settler/combat work; intended record semantics remain unchanged.

### Revision 61 - SAS practical 6398

- **Date:** 2026-09-04
- **Git commit:** `1ffe3c616ef20ae99d343df6a4667363e9f8e77c`
- **Change:** Expanded city population/state history with exact growth/starvation/prevented-growth, stored/granary food, maintenance, occupation/disorder/culture/religion/corporation/commerce/timers and richer empire aggregates.

### Revision 60 - SAS practical 6397

- **Date:** 2026-09-04
- **Git commit:** `c8dd8a9a52e273e274234f58df2ea708cd9d959f`
- **Change:** Added level-3 active-player trade-market snapshots for resources/technologies, willingness/network/trading availability, refusal/unavailability reasons, GPT capacity/quotes and optional AI technology values.

### Revision 59 - SAS practical 6396

- **Date:** 2026-09-04
- **Git commit:** `8abdbb31564245987390c96a182b14fc761d9e10`
- **Change:** Added Missionary and Executive attempt provenance (including failures and realized outcomes) and richer Spy interception/mission target identity, while preserving canonical consequence rows.

### Revision 58 - SAS practical 6394

- **Date:** 2026-09-04
- **Git commit:** `1ef349e7cf12b3be45f5b18f294cc74adac4ba42`
- **Change:** Added consequential action history for city hurry, pillage and realized gold, naval blockade lifecycle/plunder, unit gifts, religion/corporation membership changes and circumnavigation completion/bonus.

### Revision 57 - SAS practical 6393

- **Date:** 2026-09-04
- **Git commit:** `5e5e5463de7b3871bd5f4d50f48b2a5545293462`
- **Change:** Added complete diplomatic vote lifecycle history: proposal/trigger context, weighted ballots, election/results, defiers/endorsers, cancellation, persistent-effect activation/deactivation and one-shot application state.

### Revision 56 - SAS practical 6392

- **Date:** 2026-09-04
- **Git commit:** `a44fa2d733e09d6065c434d1f2e361973948ccf8`
- **Change:** Added standardized global/per-civilization starting-density and spacing history plus stronger periodic rival/known-city/other-team territory context.

### Revision 55 - SAS practical 6391

- **Date:** 2026-09-03
- **Git commit:** `00dcf84d8f7664a1cd5c88103e4a49803506f441`
- **Change:** Added production lifecycle history: genuine AI target switches/clears/resumptions, parked/restored production, aggregate transition counts and level-3 production-pipeline/parked-investment snapshots.

### Revision 54 - SAS practical 6390

- **Date:** 2026-09-03
- **Git commit:** `041a6f470867001f47919c24bae1664a42f2d4cc`
- **Change:** Expanded military quality/combat history with XP/levels/promotions/leadership/wounds, XP gained/prevented, promotion selections, veteran quality exchanged, nonlethal combat/attrition and expected-vs-observed combat-luck context.

### Revision 53 - SAS practical 6385

- **Date:** 2026-09-02
- **Git commit:** `1a54feb6433383e3799f89b61f7fe1109affcee0`
- **Change:** Greatly expanded city-raze history with pre/post city/player/team/victory context, explicit razer/previous/original ownership, city contents/state, land/population/city deltas and post-raze tile ownership.

### Revision 52 - SAS practical 6384

- **Date:** 2026-09-02
- **Git commit:** `7ca547151beb63f258d1062a4bdf84f56a5b79d0`
- **Change:** Added realized goody outcomes (gold, reveal, XP/healing, tech progress/acquisition, free/promoted/Barbarian units) and exact partial-research progress; expanded periodic current-tech progress/cost context.

### Revision 51 - SAS practical 6382

- **Date:** 2026-09-02
- **Git commit:** `ce3eb23439961915555570664aeb29623f3ed34e`
- **Change:** Added research lifecycle history: exact completion/overflow accounting and meaningful research-target redirections with team context and explicit causes.

### Revision 50 - SAS practical 6381

- **Date:** 2026-09-02
- **Git commit:** `c629781a52a463a4ef0be4dcce09f78450300db4`
- **Change:** Added exact rejected AI-to-human ordinary trade offers through a BUG-to-DLL bridge while preserving canonical SASGameRecord trade formatting.

### Revision 49 - SAS practical 6380

- **Date:** 2026-09-02
- **Git commit:** `4cd378ab4d7a152f90a7b252f897cc0642091f58`
- **Change:** Replaced generic diplomacy-event history with semantic resolved interactions (help/demands/religion/civic pressure/join-war/embargo), accepted/refused outcomes, before/after attitude/memory/war state, submitted human offers and exact AI counterproposals.

### Revision 48 - SAS practical 6379

- **Date:** 2026-09-02
- **Git commit:** `5d43e0ce2eedf80d37e56ab1026974d5f9379190`
- **Change:** Added random/manual civilization and leader provenance, exact team-merge boundaries, nuclear-launch context, and realized nuclear explosion consequences such as fallout, destroyed assets, unit damage/loss and population killed.

### Revision 47 - SAS practical 6358

- **Date:** 2026-08-31
- **Git commit:** `94f6359e28ab8c4b3cce757cff6ff148273fd9bf`
- **Change:** Added persistent savegame source-version history to SASGameRecord so creation source and later version/SHA transitions survive loads.

### Revision 46 - SAS practical 6357

- **Date:** 2026-08-31
- **Git commit:** `3a8b85dab7557eff3b135533bdfec269d3a27cbd`
- **Change:** Added automatic runtime source provenance: practical version, Git commit/short commit, branch, commit date/metadata source and dirty tracked-tree state, including Git-archive support.

### Revision 45 - SAS practical 6353

- **Date:** 2026-08-31
- **Git commit:** `d8358a19dc29cdb44f55a05d298e2428c3d30eeb`
- **Change:** Further compacted initialization/timing output: authoritative finalized setup replaces procedural chatter, player identities precede team data, identical starting-tech sets are grouped, and wall-clock fields stay on meaningful boundaries.

### Revision 44 - SAS practical 6351

- **Date:** 2026-08-31
- **Git commit:** `9068ad28635d8be44965efadf9bce1738e1b412d`
- **Change:** Normalized finalized initialization reporting around shared authoritative `INITIAL_*` team/technology/deal state, reduced routine setup chatter/deal floods, and centralized quoting/list/filename/team/trade serialization helpers.

### Revision 43 - SAS practical 6347

- **Date:** 2026-08-31
- **Git commit:** `78378a2e3c9521680f9d478214c7e6c88617047b`
- **Change:** Added shared loaded-DLL provenance: build target/configuration, compile flags, size, write UTC, PE timestamp and cached FNV-1a binary fingerprint; moved common provenance/timing helpers into shared game-core utilities.

### Revision 42 - SAS practical 6345

- **Date:** 2026-08-31
- **Git commit:** `5a71f874e5c716d183338fbe07c30ee501b8ba65`
- **Change:** Added `sessionWallMilliseconds` to every emitted row, authoritative Debug-mode transition history, per-request autoplay wall time, and preservation of original timestamps for buffered initialization actions.

### Revision 41 - SAS practical 6343

- **Date:** 2026-08-30
- **Git commit:** `bfa4dfdeef990a67279600891674d8716326b90d`
- **Change:** Expanded setup provenance with custom map-script option selections/defaults/descriptions, Advanced Start points, resolved UWAI/legacy war-AI mode, and buffered initialization actions so stable finalized setup context precedes them.

### Revision 40 - SAS practical 6328

- **Date:** 2026-08-30
- **Git commit:** `98ee0c269abf6dc0459e4d8c5062ab0a39d38445`
- **Change:** Unified diagnostic UTC identity with BBAI by adding shared process-level UTC provenance and using the generic shared UTC formatter for SASGameRecord sessions.

### Revision 39 - SAS practical 6326

- **Date:** 2026-08-29
- **Git commit:** `66b46c22d2485e744d2703f8c9a9773cb15ae16b`
- **Change:** Changed mod provenance to distinguish the centralized branded/display name from the actual loaded folder name/path (`displayName`, `folderName`, `modPath`).

### Revision 38 - SAS practical 6293

- **Date:** 2026-08-27
- **Git commit:** `617843aa2bc9c108530719f7783c6467b1c85ea7`
- **Change:** Hardened SASGameRecord military counting against invalid/unplaced unit-container entries so malformed objects cannot crash snapshots or count toward military posture.

### Revision 37 - SAS practical 6250

- **Date:** 2026-08-24
- **Git commit:** `f0ee784b6444e9a0ebedb67722fa4b89f9e29314`
- **Change:** Fixed six record defects: exact failed-attack/battle coordinates and dependent context, inclusive battle-summary boundaries, session-vs-lifetime counter naming, `-1` production sentinels, disorder process-conversion suppression, and safe rollover finalization of buffered old-map observations.

### Revision 36 - SAS practical 6249

- **Date:** 2026-08-24
- **Git commit:** `f2d72e7f90a8640dce6600f1afe0c85b62cb3887`
- **Change:** Fixed recorder correctness/safety issues: nearby-enemy hostility now uses each unit's actual plot, long structured rows grow instead of truncating at 2047 bytes, and Spy mission-discount preparation is counted only for valid ordinary Spies.

### Revision 35 - SAS practical 6124

- **Date:** 2026-08-18
- **Git commit:** `234f87f13c4d7c23411c83faa13186f4f5bc7823`
- **Change:** Compacted production-overflow reporting by moving exact overflow evolution (`rawModifiedOverflow`, `unmodifiedOverflow`, `keptOverflow`, lost production, unused capacity and overflow gold) into detailed completion rows while retaining aggregate flow rows.

### Revision 34 - SAS practical 6123

- **Date:** 2026-08-18
- **Git commit:** `574e50ba532286ada297fca3a49a9af375c1bcc5`
- **Change:** Improved Great Person/Great General provenance by adding newborn unit IDs and spawn coordinates and recording direct player-caused deaths of defenseless Great People.

### Revision 33 - SAS practical 6119

- **Date:** 2026-08-16
- **Git commit:** `3ffd61d7959e56861c8a056237981310642bcee1`
- **Change:** Added explicit autoplay request IDs, requested/completed turn counts, player changes, wall/run state and authoritative autoplay end causes.

### Revision 32 - SAS practical 6118

- **Date:** 2026-08-15
- **Git commit:** `e67adf522de45321fad3d46ce137796f3ab83e48`
- **Change:** Made each SASGameRecord text-map layer independently optional through XML defines; recorder output can therefore omit selected map layers intentionally.

### Revision 31 - SAS practical 6111

- **Date:** 2026-08-12
- **Git commit:** `8ab3703280e87ffb7a8a8cc3fb9c349c7ffa165a`
- **Change:** Encoded natural food quality in terrain-map letter case using terrain/hill/feature/lake/river/permanent-yield context and a tunable threshold.

### Revision 30 - SAS practical 6110

- **Date:** 2026-08-11
- **Git commit:** `ea2f16e4e5904286e8facc7e026d259cf5e5f573`
- **Change:** Added directional river maps, exact river-edge totals/coordinates, and recording of rare later river edits.

### Revision 29 - SAS practical 6109

- **Date:** 2026-08-11
- **Git commit:** `129d438d80ac106aa77b3acebe613eb9fab3b6e2`
- **Change:** Added initial bonus maps, runtime-validated symbol legends, type totals, resampling/collision information and per-layer quantitative counts.

### Revision 28 - SAS practical 6108

- **Date:** 2026-08-11
- **Git commit:** `404d43461d2c67af87862341b7609ac882782b0e`
- **Change:** Added configurable terrain and evolving feature map layers, with changing feature/political layers repeatable during the run.

### Revision 27 - SAS practical 6107

- **Date:** 2026-08-11
- **Git commit:** `ad20f152bd488a871c26a72c4471e5d3beba63cd`
- **Change:** Added aspect-correct SASGameRecord ASCII map rendering with explicit player/civilization/name mappings.

### Revision 26 - SAS practical 6106

- **Date:** 2026-08-11
- **Git commit:** `a7a071f1be63b652070811ccb7eda27a7b1cfbdf`
- **Change:** Expanded Barbarian history with city, spawn, position, unit and combat-pressure context.

### Revision 25 - SAS practical 6105

- **Date:** 2026-08-11
- **Git commit:** `724500d4a0ac6604f0df2e817629981a09e42ec9`
- **Change:** Added calendar/start-turn/start-year, player-color, and final-score/replay-related context to the record.

### Revision 24 - SAS practical 6104

- **Date:** 2026-08-11
- **Git commit:** `a84d4747204a99de6214942f94a80e7c2360745f`
- **Change:** Added millisecond timing and tunable privacy-tiered system/performance context, including snapshot UTC/wall timing and optional process/window/runtime/memory information.

### Revision 23 - SAS practical 6103

- **Date:** 2026-08-11
- **Git commit:** `e990260efc0d4f22d0718a443d18e66b522e0e79`
- **Change:** Added synthetic whole-war outcome summaries covering duration, war success, losses/production value destroyed, city-plot battles, conquests and captured population, including ongoing/end-state context.

### Revision 22 - SAS practical 6102

- **Date:** 2026-08-10
- **Git commit:** `fd869b094ef88b16a9c7bc4952462e9fba0d6106`
- **Change:** Added durable session context for game type/mode, multiplayer format/options and turn handling, and standardized shared enum tokens.

### Revision 21 - SAS practical 6101

- **Date:** 2026-08-10
- **Git commit:** `5d00ec19e86215271fb6aa37b4933f1a55c7e3ac`
- **Change:** Added explicit technology-acquisition provenance, distinguishing research, trade, free choices, huts, Great People, espionage, sharing, inheritance, setup and other paths.

### Revision 20 - SAS practical 6100

- **Date:** 2026-08-10
- **Git commit:** `d24d15505cd41a53df3579aa9844c347cb618025`
- **Change:** Added compact interval production and military flow histories plus policy/civic/religion-style state history, while retaining detailed level-3 evidence.

### Revision 19 - SAS practical 6097

- **Date:** 2026-08-09
- **Git commit:** `c7bfe6cc181619ccc59a9f0694b4b8b898558ae2`
- **Change:** Added exact winner/loser unit IDs to `GAME_RECORD_BATTLE` so combat results can be joined reliably to unit-specific attack-order diagnostics.

### Revision 18 - SAS practical 6096

- **Date:** 2026-08-09
- **Git commit:** `b5a184403ef9fdd75d3dd30c5e3a3ef070b56f55`
- **Change:** Renamed the snapshot-interval define and logged setting from `SAS_GAME_RECORD_TURN_INTERVAL` to `SAS_GAME_RECORD_INTERVAL_TURNS_UNSCALED_GAMESPEED`, making its intentional literal-turn semantics explicit.

### Revision 17 - SAS practical 6092

- **Date:** 2026-08-07
- **Git commit:** `c30e4e1ef0d37675aba12d668ca8b514e079ce26`
- **Change:** Added current/undamaged city-defense and bombard state, synthetic consecutive city-bombard sequences, and individual air-strike/interception/plot-air-bomb history.

### Revision 16 - SAS practical 6091

- **Date:** 2026-08-07
- **Git commit:** `63eef1e11158ca8a0b69d415f927fab70c5a7bb9`
- **Change:** Added `UnitCombat` composition shares, compact air/missile/nuclear posture including cargo, and per-city air-unit occupancy/capacity.

### Revision 15 - SAS practical 6090

- **Date:** 2026-08-07
- **Git commit:** `1a80ca7cff111011fa27c8f19ebf19fe44b5fe0b`
- **Change:** Added initial map/landmass geography and resource/yield context.

### Revision 14 - SAS practical 6086

- **Date:** 2026-08-05
- **Git commit:** `bff444456275a42631f5ad449fec942dfb6ff310`
- **Change:** Corrected SASGameRecord army accounting by separating true `combatUnits` from Civ4's narrower XML-based `militarySupportUnits` concept.

### Revision 13 - SAS practical 6085

- **Date:** 2026-08-05
- **Git commit:** `f651eeade3e95dcb49670625ee2fd5dcaa3a92dd`
- **Change:** Added AI military-production information to SASGameRecord alongside BBAI diagnostics.

### Revision 12 - SAS practical 6084

- **Date:** 2026-08-05
- **Git commit:** `b67e6210638191af81b21fd1c8079344c48365b1`
- **Change:** Extended existing rows with espionage context: actual per-rival EP spending, espionage strategy flags, and per-city espionage output/modifier/defense information.

### Revision 11 - SAS practical 6083

- **Date:** 2026-08-05
- **Git commit:** `cac3fa4d2bdd225f744c7ee3bc5231e63aad37d0`
- **Change:** Renamed the feature from `SASGameSummary` to `SASGameRecord` throughout filenames, symbols, XML defines and row names (`GAME_SUMMARY_*` -> `GAME_RECORD_*`).

### Revision 10 - SAS practical 6077

- **Date:** 2026-07-22
- **Git commit:** `b80afdad17c550772150ccfc41ed01067897335c`
- **Change:** Added territory-development coverage, routes/improvements/irrigation and bonus-development context, runtime/mod identity, enabled victory/game-limit context, AI victory-stage strategy context, and player-wide health/happiness modifier sources.

### Revision 9 - SAS practical 6075

- **Date:** 2026-07-22
- **Git commit:** `22dc2f0abd71e870dcb333a5c82ed6c9338f5f18`
- **Change:** Expanded state reporting with production overflow/loss, production-to-commerce conversion, wonder/project fail gold, pollution sources, city buildings, trade-capability unlocks, research overflow context, and clearer human/autoplay-control identity.

### Revision 8 - SAS practical 6058

- **Date:** 2026-07-20
- **Git commit:** `0b7a1566d7c17085171cc0ea49a30ffe419aed33`
- **Change:** Added war-declaration origins/sponsors, diplomacy memories, broader victory-progress context, project-victory reset/failure context, and compact full-map revelation history.

### Revision 7 - SAS practical 6057

- **Date:** 2026-07-19
- **Git commit:** `05b03177383c158c0962000fb2979a06850456ef`
- **Change:** Added final victory snapshots, project-victory progress, exploration history, and map/environment change reporting.

### Revision 6 - SAS practical 6046

- **Date:** 2026-07-16
- **Git commit:** `559ee966cbc17bcf6b9891acf367a9e0bfa10f24`
- **Change:** Added factual army-concentration/attack-stack context, Great Person use, and espionage-action information to the structured record alongside corresponding BBAI diagnostics.

### Revision 5 - SAS practical 6034

- **Date:** 2026-07-13
- **Git commit:** `cecab8af1e29b99c0996bbf68418aebbb66dd0d7`
- **Change:** Refactored structured-record trade-item, diplomacy-event and war-plan token naming onto shared stable SAS token helpers. Intended record semantics stayed equivalent, but downstream copied implementations should review the helper change.

### Revision 4 - SAS practical 6033

- **Date:** 2026-07-13
- **Git commit:** `d8bdd0602e765b4d945e6a4e25f447415b074241`
- **Change:** Made free-text fields parser-safe through quoting/escaping; also expanded run-status and lifecycle context such as player appearance/revival/elimination and autoplay state so stopped runs could be interpreted without inferring from missing snapshots.

### Revision 3 - SAS practical 6030

- **Date:** 2026-07-12
- **Git commit:** `26b0d5967d691fc5d59a350856ffc7597b47c290`
- **Change:** Added Worker danger/death context and recorded in-game bonus appearances/disappearances.

### Revision 2 - SAS practical 6026

- **Date:** 2026-07-11
- **Git commit:** `876158dff2f27852320810189730865c68fd69e7`
- **Change:** Added AI Settler security/context information to the structured record, including nearby-unit/defender context used to understand whether Settlers were adequately protected.

### Revision 1 - SAS practical 6022

- **Date:** 2026-07-10
- **Git commit:** `c023244d86276285ac0a6511f02316d71047dabc`
- **Change:** Introduced the independent structured `SASGameSummary` recorder, separate from BBAI, with lifecycle/action rows plus compact economy, expansion, worked-plot, statistics, city/battle-history and unit-composition reporting.

## Maintenance template

For the next qualifying committed update, increment `SAS_GAME_RECORD_REVISION` in `GlobalDefines_advciv_sas.xml` exactly once and prepend one entry like this:

```markdown
### Revision N - SAS practical XXXX

- **Date:** YYYY-MM-DD
- **Git commit:** pending
- **Change:** Briefly state what changed in SASGameRecord-related code and why a downstream copy should review/update.
```

Keep only the latest entry's Git commit as `pending`; when adding the next revision, replace the preceding entry's `pending` with its finalized full SHA from the default branch, reconciling after an amend or rebase.

Do not put the current commit's own SHA into its entry: editing the document would change that SHA again. Current runtime Git/source identity is recorded independently by `GAME_RECORD_SOURCE_CONTEXT`.

Standalone docs/history/example-log/package-only updates do not bump the revision. If a qualifying implementation change and those maintenance files are committed together, they remain one revision. Do not pre-bump, double-bump or reuse a revision for a later qualifying commit.
