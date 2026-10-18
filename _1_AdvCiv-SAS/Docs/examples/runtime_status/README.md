# Runtime-status examples

These companion files were captured from the user's real Civ4 run on 2026-10-18, with the final snapshot sampled at `20261018T193858.716Z`. They retain the reporting Release DLL's identity and process/session fields; these are observed runtime records, not synthetic harness output.

The process created three Advanced Start games, recorded autoplay ending at victory, continued through "Just one more turn", returned to the menu twice, and exited directly from the final game. These are functional observations, not a controlled performance comparison.

## Files and purpose

| File | Meaning |
| --- | --- |
| [SASRuntimeStatus_example.json](SASRuntimeStatus_example.json) | One indented JSON object describing the latest observed state, loaded-DLL identity and current/previous activity timing. |
| [SASRuntimeHistory_example.jsonl](SASRuntimeHistory_example.jsonl) | One complete JSON object per line, retaining activity transitions through the same observation. |

The snapshot's `historyFilename` is normalized to the example companion's filename; no other fields are removed. Its companion contains 20 consecutive entries through the snapshot's activity serial. The final snapshot reports `EXITED_CLEANLY` / `SHUTTING_DOWN`, and Windows inspection confirmed the process had exited.

No timestamps, DLL identity or measurements were replaced. The DLL's fingerprint matches the installed binary and successful Release build report; all 362 recorded build inputs matched current source at review, with zero build warnings/errors.

Live output stays in the resolved user root's `Logs` folder, outside the repository. These repository examples are deliberately retained documentation; they are not required by Civ4.

## Recorder revision and serials

The current recorder reports `revision` in both files, using one compiled `SAS_RUNTIME_REVISION`. Its [revision history](../../revisions/SASRuntime.md) records implementation changes, including minor changes, once per qualifying commit. This revision is independent of SASGameRecord and binary-save versions.

All captured files report `revision: 2` from the rebuilt DLL, including presentation observations and the explicit interval-start distinction.

Serials count observations within one process, independently of revision and Windows process ID. `statusSerial` counts prepared snapshots; `activitySerial` in the snapshot matches `stateSerial` in timeline rows. This capture reached snapshot 35 and entry 20 in process 53492.

From revision 2, `stateSerial` also orders presentation-only timeline observations; it is a row counter, not a count of completed activity intervals. Counters can advance despite failed publication, so use the explicit failure counters when interpreting gaps.

## Why two formats?

JSONL means JSON Lines: each physical line is a separate JSON document. Read it line by line; the whole file is not one JSON array.

Spreading a JSONL record across multiple lines would break that line-by-line reading contract. The current-state JSON is one complete document, so it can use multiline indentation for direct inspection.

Line-oriented searches such as `rg USER_INTERRUPTED SASRuntimeHistory_*.jsonl` return the complete matching transition, including its identity and timing fields, without collecting neighboring lines. This is a practical search advantage; one long line still contains all of its text and does not make the record free to read.

The fixed-name JSON is replaced to keep current-state discovery small and readable. The timestamped JSONL appends transitions across successive new/load sessions without copying all earlier history on every completed turn or retaining a growing history in memory.

The fixed snapshot name lets a reader locate the latest published observation without choosing among old runs. A new process gets its own timestamp/PID history filename so earlier process histories are retained instead of overwritten.

A single JSON document could hold both, but an embedded history array would require rewriting that accumulated history or managing in-place array edits. The snapshot instead links to its companion through `historyFilename`.

## Field coverage

The timeline is not a full snapshot archive. Activity, activity serial, update time/reason and previous-interval fields have corresponding timeline names such as `state`, `stateSerial`, `recordedUtc` and `reason`; DLL/process/session identity and loaded-save checkpoints are retained too.

Current-only fields include per-turn elapsed/last-completed-turn observations, player/map configuration, session counters/readiness timestamps and status/history publication counters. The timeline records activity transitions, not each completed-turn snapshot; its autoplay completion cause is retained even after later snapshots replace the current reason.

For coarse phase timing, a full snapshot copy at every turn is unnecessary. SASGameRecord remains the detailed game-history source. To preserve an older process's final published snapshot as well as its timeline, copy the pair before a later process replaces the fixed-name JSON.

## Reading time and history

`updatedUtc` is when the snapshot's observation was sampled. `activityStartedUtc` locates the current interval, and `activityElapsedMilliseconds` reports elapsed time at that observation; it does not keep advancing inside a static file.

Previous-activity fields retain its start/end UTC and completed duration. Production elapsed durations use the monotonic diagnostic clock, independently of system-clock corrections.

In revision 2, a timeline row with `intervalStarted=1` closes `previousState` and starts `state` at `recordedUtc`. A row with `intervalStarted=0` is an observation inside the current activity; its `previousState` is `NONE`, its previous timing fields are unavailable, and no completed interval should be counted again.

`stateStartedUtc` and `stateElapsedMilliseconds` locate either row within its ongoing activity. Presentation-only observations retain the original activity start and the snapshot's most recently completed interval, so a loading-asset request does not shorten the eventual creation/load duration.

State/session serials connect observations across new games and loaded saves; DLL/process identity ties them to the implementation and process that reported them.

Completed intervals recorded in this real example are:

| Completed activity | Duration | Following observation |
| --- | --- | --- |
| First new-world creation | 2,170 ms | Native game initialization ready |
| Autoplay | 2,862 ms | `VICTORY` end cause at turn 11 |
| End-game interval | 15,713 ms | `EXTENDED_GAME_STARTED`, restoring `PLAYING` |
| First menu-return request through reset | 223 ms | `NO_GAME` |
| Second new-world creation | 1,313 ms | Native game initialization ready |
| Second menu-return request through reset | 59 ms | `NO_GAME` |
| Third new-world creation | 939 ms | Native game initialization ready |
| In-game exit request through orderly destruction entry | 151 ms | `SHUTTING_DOWN` / `PROCESS_EXIT` |

SASGameRecord corroborated `GAMEOPTION_ADVANCED_START`, 400 configured points and placement-screen calls in all three sessions. The runtime timeline contains no false WorldBuilder activity for them; the second session also recorded game end at turn 0, without identifying its cause.

The loading-asset getters were not observed inside these new/load intervals, so their timestamp remains unavailable. No diagnostic call was added to manufacture that observation.

The final shutdown interval has a start observation but no later completion observation. `EXITED_CLEANLY` records entry into orderly game destruction; the separate Windows process check confirmed termination. An elapsed value of 0 in the final snapshot describes that shutdown-entry observation.

A scene or loading-asset request is evidence that the EXE requested that presentation asset, not proof that a rendered frame reached the user. Loaded-save checkpoints likewise do not prove the filename actually opened.

`statusWriteFailures` counts earlier failed snapshot publications, including root/open/write/close failures, and becomes visible in a recovered snapshot. This captured run reported zero status/history write failures; `PythonErr.log` was empty, and SASGameRecord's final writer-health row reported no pending text, output failures or dropped rows/bytes.

`historyRowsWritten` counts confirmed successful appends. A row can already be readable when close reports failure, so this counter can be lower than the physical line count; `historyWriteFailures` includes that unconfirmed result.

A crash or forced termination leaves an unfinished last interval, not an invented end time or crash cause. Match the live Windows process using PID and process-start UTC; inspect history-write failures/state serials before assuming the timeline is complete.

## Real output and test fixtures

The linked JSON and JSONL files provide a real example of the recorder's output, with matching process/DLL identity and timeline entries through the snapshot's `activitySerial`. Their captured fields and timings are preserved; only the companion filename is normalized as described above.

The native harness can separately export synthetic fixtures to a scratch directory:

```powershell
python tests/diagnostics/run_writer_tests.py --export-runtime-examples "$env:TEMP\sas-runtime-fixture-examples"
```

That command compiles only the temporary harness, without building/installing the game DLL or reading private user logs. Its controlled test fixtures are distinct from the real-run examples provided here; ordinary test runs do not update either set.
