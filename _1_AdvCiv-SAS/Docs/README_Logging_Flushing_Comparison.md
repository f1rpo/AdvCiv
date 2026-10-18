# Logging flushing comparison — 2026-10-18

The prompt-write revision-154 candidate was compared with revision 153's EXE logger on the same local machine. Normal logging showed a small timing difference; a diagnostic-heavy SAS48 run took substantially less time with the new writer. These are observed results from one candidate/control pair per workload, not a repeated benchmark or a general gameplay speed claim.

## Builds and workloads

Both DLLs were freshly built as **Debug-opt** with VC2003. Each pair used the corresponding starting save and matching logging settings, starting at game turn 0. The loaded-DLL context recorded clean build-artifact baselines for both builds.

| Implementation | Record revision | Loaded DLL fingerprint |
| --- | ---: | --- |
| Prompt writes through persistent shared handles | 154 | `FNV1A64:60140FB5EDA00BBF` |
| Original EXE logger control | 153 | `FNV1A64:A04C6CC0710846CF` |

Both were based on Git `52871e8dcf69034f7b8d83affac2ffe9d3c96af9`; the candidate included the pending revision-154 changes. SASGameRecord was level 3 with its snapshot interval set to 10 turns in all four runs.

| Workload | Starting game | Logging and duration |
| --- | --- | --- |
| Normal | Korea; 16 players; Huge Archipelago, 120×84 | BBAI disabled; 251 autoplay turns |
| Stress | Mongolia; 48 players; SAS48 Pangaea, 154×108 | BBAI enabled; Found, Settler and all five UWAI categories at level 3; 21 autoplay turns |

Both games used Normal speed, Ancient era, Monarch difficulty, Temperate climate and Medium sea level. The stress categories were UWAI Agent, Military Analyst, Invasion Graph, Armament Forecast and War Utility; the separate later-city food-support audit remained disabled.

## Timings

Autoplay totals come from `AUTOPLAY_ENDED autoplayWallMilliseconds`, excluding time spent waiting before autoplay started. Both pairs completed the requested number of turns with `endCause=SCHEDULED`.

| Workload | New writer | Old logger | Reduction relative to old logger |
| --- | ---: | ---: | ---: |
| Normal: 251 turns | 271.421 s | 273.030 s | 0.59% |
| Stress: 21 turns | 49.545 s | 87.720 s | 43.52% |

The normal difference was 1.609 seconds and is small enough to be run-to-run variation. The stress difference was 38.175 seconds: the old-logger run took about 1.77 times as long. This supports the expected possibility of lower logging overhead under heavy output, without isolating which part of the writer change accounts for the improvement.

The following cumulative timings use `GAME_RECORD_TURN_END sessionWallMilliseconds` minus the autoplay-start session timestamp. They are game-turn checkpoints, not counts of completed autoplay turns: turn 250 belongs to the 251-turn run. The final snapshot includes work after the autoplay-end marker, so its time is slightly higher than the autoplay total.

| Normal: through game turn | New writer | Old logger | Reduction |
| --- | ---: | ---: | ---: |
| 50 | 13.811 s | 13.996 s | 1.32% |
| 100 | 35.922 s | 36.237 s | 0.87% |
| 150 | 87.635 s | 88.257 s | 0.70% |
| 200 | 164.901 s | 165.745 s | 0.51% |
| 250 | 271.915 s | 273.539 s | 0.59% |

| Stress: through game turn | New writer | Old logger | Reduction |
| --- | ---: | ---: | ---: |
| 10 | 23.010 s | 39.769 s | 42.14% |
| 20 | 49.892 s | 88.089 s | 43.36% |

The first stress scenario also has matching save-serialization timestamps every four turns. These are individual elapsed UTC intervals between the first `ENGINE_SAVE_UNKNOWN` checkpoint at each turn, rather than cumulative timings or exact completed-turn boundaries.

| First stress: between save checkpoints | New writer | Old logger | Reduction |
| --- | ---: | ---: | ---: |
| Turn 4 to 8 | 6.782 s | 11.365 s | 40.33% |
| Turn 8 to 12 | 8.097 s | 13.913 s | 41.80% |
| Turn 12 to 16 | 9.757 s | 17.543 s | 44.38% |
| Turn 16 to 20 | 11.727 s | 21.210 s | 44.71% |

## Output and live access

Sizes below are bytes read after the corresponding autoplay completed. They describe the observed files, including startup and any later recorded activity; they are not measurements of bytes written strictly inside the timed autoplay interval. BBAI continued emitting turn-21 diagnostics after the stress autoplay ended at turn 20.

| Workload | File | New writer | Old logger |
| --- | --- | ---: | ---: |
| Normal | SASGameRecord | 34,093,809 | 34,092,723 |
| Stress | SASGameRecord | 5,041,658 | 5,040,572 |
| Stress | BBAI | 2,056,465,887 | 2,056,464,800 |

Combined sizes for all three scenarios, including the later Celtic comparison below:

| Scenario | New writer: total bytes | Old logger: total bytes |
| --- | ---: | ---: |
| Normal: Huge Archipelago | 34,093,809 | 34,092,723 |
| First stress: SAS48 Pangaea | 2,061,507,545 | 2,061,505,372 |
| Second stress: SAS48 Custom_Continents | 1,824,581,959 | 1,824,579,801 |

The similar sizes support comparing similar output volume; they do not mean the files are byte-identical. Source/build metadata and timing fields differ.

- **Normal candidate and control:** inspected after autoplay completed while Civ4 remained open. Neither normal run was sampled during autoplay in this comparison.
- **Stress candidate:** both logs were read during autoplay and after completion. During live sampling, BBAI grew from 1,533,770,354 to 1,721,822,711 bytes; SASGameRecord grew from 3,749,352 to 3,812,832 bytes. The sampled record had completed turn 16 and entered turn 17.
- **Stress control:** both old-logger files were also readable during autoplay and after completion while Civ4 remained open. BBAI grew from 582,512,826 to 757,133,142 bytes; SASGameRecord grew from 2,186,003 to 2,295,228 bytes. The sampled record had completed turn 5 and entered turn 6.

The old logger therefore also allowed live reading in these runs. Earlier difficulties accessing logs motivated explicit writer ownership and sharing, but these observations do not establish that the old logger always blocked access. Directory listings sometimes reported zero bytes even when direct reads returned growing output; direct reads supplied the measurements above.

The candidate's 16 MiB limit applies to shared pending text awaiting a root or write recovery, not total file size. Producing approximately 2.06 GB of BBAI output does not imply that amount was held in Civ4 RAM. Pending-memory peaks, write-call timings, physical disk traffic and SSD wear were not instrumented in this experiment.

## Record integrity and limits

The maintained [RNG/state comparison helper](/LLM_Helpers/compare_sasgamerecord_rng.py) validated all 254 RNG/state checkpoint pairs in each normal record and all 24 pairs in each stress record. State fingerprints matched across each candidate/control pair. RNG states, counts and value-stream fingerprints matched too.

Only `syncIntervalCallFingerprint` and `syncSessionCallFingerprint` differed, first appearing at turn 5 in both comparisons. These describe call provenance rather than different random values; the precise cause was not established.

The helper conservatively reports this divergence and suppresses combined cross-revision ordering interpretation. Matching state hashes are observed evidence, not an exhaustive gameplay-equivalence proof.

No `SAS_DIAGNOSTIC_DATA_LOSS` notices were found in either complete candidate run, including a full scan of its stress BBAI file. No Python exceptions were found in the inspected error logs.

These checks do not establish crash durability in Civ4: abrupt-process preservation was exercised separately by the [local writer harness](/tests/README.md#diagnostic-writer), not by deliberately killing either timed game run.

Live reads were part of the stress measurements, at different moments in the two runs. Cache state, background activity and read overhead were not controlled statistically. There were no repeated runs, Release-build comparison or disabled-logging performance pair.

The results support this tested Debug-opt workload; they should not be generalized into a fixed percentage improvement for ordinary games.

## Local evidence identifiers

The original logs were in the user's Civ4 `Logs` directory; the multi-gigabyte files are not included here. Filenames identify the measured records for later local review. UTC timestamps in filenames differ from the local Europe/Paris date-time display.

| Run | SASGameRecord filename | BBAI filename |
| --- | --- | --- |
| Normal candidate | `SASGameRecord_20261018T062025Z_load1.log` | Disabled |
| Normal control | `SASGameRecord_20261018T064423Z_load1.log` | Disabled |
| Stress candidate | `SASGameRecord_20261018T063227Z_load1.log` | `BBAI_20261018T063227Z_load1.log` |
| Stress control | `SASGameRecord_20261018T065355Z_load1.log` | `BBAI_20261018T065355Z_load1.log` |

## Later test on 2026-10-18: second stress scenario

A further new-writer run used a different starting save: Celts, 48 players, SAS48 Custom_Continents at 178×124, Arid climate and Medium sea level. It retained the stress logging categories, SASGameRecord level 3 and 10-turn snapshot interval, and used the same candidate DLL fingerprint listed above.

The 21-turn autoplay completed as scheduled in **42.432 seconds**. After completion, direct reads returned **1,820,359,858 bytes of BBAI** and **4,222,101 bytes of SASGameRecord**. All 24 RNG/state checkpoint pairs validated within this record; SASGameRecord contained no loss notices.

For this second stress scenario, the agent first read the logs **after autoplay had completed, while Civ4 remained open**; it did not read them during autoplay. The first normal scenario was likewise first inspected after completion, whereas both implementations in the first stress scenario were sampled during autoplay as well as after completion.

This records agent inspection timing, not a claim that no other software accessed the files. The second stress scenario is not a matched repeat of the first: its map, initial state and RNG states differ, so 42.432 seconds must not be compared directly with the first scenario's 49.545/87.720-second timings.

The corresponding old-logger run then completed the same 21 turns as scheduled in **75.766 seconds**. Both implementations were first read after completion while Civ4 remained open; neither Celtic run was read during autoplay by the agent.

| Celtic scenario | New writer | Old logger | Reduction relative to old logger |
| --- | ---: | ---: | ---: |
| Through game turn 10 | 20.249 s | 34.606 s | 41.49% |
| Through game turn 20 | 42.889 s | 76.244 s | 43.75% |
| Autoplay total: 21 turns | 42.432 s | 75.766 s | 44.00% |
| BBAI bytes observed after autoplay | 1,820,359,858 | 1,820,358,779 | — |
| SASGameRecord bytes observed after autoplay | 4,222,101 | 4,221,022 | — |
| Combined log bytes observed after autoplay | 1,824,581,959 | 1,824,579,801 | — |

Additional timestamps come from the first `ENGINE_SAVE_UNKNOWN` save checkpoint at each listed turn. Unlike the cumulative turn-end timings above, these are individual elapsed UTC intervals between matching save-serialization points, not exact completed-turn durations.

| Between save checkpoints | New writer | Old logger | Reduction |
| --- | ---: | ---: | ---: |
| Turn 4 to 8 | 6.145 s | 10.320 s | 40.46% |
| Turn 8 to 12 | 7.537 s | 13.247 s | 43.10% |
| Turn 12 to 16 | 8.419 s | 15.461 s | 45.55% |
| Turn 16 to 20 | 9.469 s | 17.642 s | 46.33% |

Both Celtic records validated all 24 RNG/state checkpoint pairs. Across the pair, recorded state fingerprints, RNG states, counts and value streams matched; only the two sync call-provenance fingerprints differed from turn 5, as in the earlier pairs. No Python exceptions were found in the inspected logs.

The candidate evidence files are `SASGameRecord_20261018T070329Z_load1.log` and `BBAI_20261018T070329Z_load1.log`; the control files are `SASGameRecord_20261018T070719Z_load1.log` and `BBAI_20261018T070719Z_load1.log`.

### Focus and run-order follow-up

The foreground indicators provide useful context for interpreting the two stress results:

| Scenario | Candidate foreground at sampled checkpoints | Control foreground at sampled checkpoints |
| --- | --- | --- |
| Normal Archipelago | Yes, turns 10 through 250 | Yes, turns 10 through 250 |
| First stress: Mongolian Pangaea | No, turns 10 and 20 | No, turns 10 and 20 |
| Second stress: Celtic Custom_Continents | No, turns 10 and 20 | Yes, turns 10 and 20 |

The second scenario therefore has an observed focus mismatch; rendering/background behavior may contribute to its measured difference, and the entire 44.00% cannot be confidently attributed to the writer. The first stress pair did not have that observed mismatch. These are snapshots, not a continuous focus trace.

Both stress scenarios ran the candidate before the control. The second pair tests a different map and post-completion-only agent inspection, rather than a reversed-order repeat of the first pair. A further controlled benchmark should keep focus consistent and reverse or alternate run order.

The combined-output table above includes this later pair; the normal pair's only enabled diagnostic file was SASGameRecord, so its combined sizes equal that file's size.
