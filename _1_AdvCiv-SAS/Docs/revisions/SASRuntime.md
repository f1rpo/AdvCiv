# SAS runtime recorder revisions

`SASRuntimeStatus.json` describes the latest observed state; its companion `SASRuntimeHistory_*.jsonl` retains timed activity transitions. Both report the same compiled `SAS_RUNTIME_REVISION` as `revision`, identifying their recorder implementation independently of SASGameRecord's revision.

Increment once per commit changing the loaded runtime recorder, its interfaces, configuration, call sites or shared runtime services, including implementation comments/provenance. Keep one pending revision throughout drafting and amend cycles.

Pure CI/checker, test, documentation and historical-example maintenance does not change the loaded recorder and does not require an increment or DLL rebuild.

Add a latest-first entry explaining the behavior and rationale. The diagnostics check requires consecutive history entries, agreement with the compiled value, and exactly one increment per recognized implementation commit. Shared-code detection uses changed runtime symbols; review unusual refactors manually because CI cannot infer every dependency.

## History

### Revision 3 - SAS practical 6644

- **Date:** 2026-10-19
- **Git commit:** pending
- **Change:** Make each per-process JSONL timeline more useful as a standalone archive while removing repeated constant identity from every transition.
  - Add one typed `run` record with process start/PID, DLL-load UTC and full loaded-DLL identity. Transition records retain the recorder revision and compact run ID so a copied/grepped line still identifies its source run without repeating the full identity object.
  - Add one typed `session` record for each ready new/load session, normally before its first ready transition, with session timestamps/counters, ready-game turn/active-player IDs, map/game configuration and known loaded-save checkpoint evidence. Capture that context once so a later retry preserves the original ready observation even after the game advances or resets.
  - Keep `transition` records focused on timed activity boundaries and presentation observations. Record prior status/history publication-failure counters on archival rows so recovered evidence discloses earlier gaps.
  - A failed run-metadata publication does not block later transitions. Open/partial failures retry on later updates; a fully written row with an unconfirmed close is not duplicated, and failure counters disclose the uncertainty. Root migration treats such possibly written rows as evidence too, so typed records remain together when `CvAltRoot` takes over.

### Revision 2 - SAS practical 6643

- **Date:** 2026-10-18
- **Git commit:** `e58d41b6e8a6e6fe69993c4b8aa056f8679d82bd`
- **Change:** Add event-driven presentation/lifecycle observations without frame polling or side-effectful diagnostic queries.
  - Record the EXE's existing main-menu scene and loading-screen asset requests as explicitly named evidence, without treating either request as proof that a rendered frame reached the user.
  - Distinguish native WorldBuilder mode from the Python WorldBuilder screen; time active editor intervals as their own coarse activity while recording screen show/hide separately and ignoring Advanced Start's reuse of the same Python screen.
  - Restore `PLAYING` when `GAMESTATE_OVER` transitions to `GAMESTATE_EXTENDED`, so "Just one more turn" does not leave external status stuck at `GAME_ENDED`.
  - Preserve whole activity durations when recording presentation-only observations. Timeline rows distinguish interval starts from observations inside an existing interval; a repeated activity label no longer implies that its clock restarted or that another completed duration should be counted.
  - Filter Advanced Start inside the cached native gate, retaining real screen observations even when native mode is reported later. Pending menu/exit requests remain authoritative while editor callbacks finish arriving.

### Revision 1 - SAS practical 6642

- **Date:** 2026-10-18
- **Git commit:** `b0ddbda713d15d04f229a377af1d0c392e886351`
- **Change:** Publish optional current-state observations and a per-process activity timeline so agents can inspect creation, loading and autoplay while Civ4 remains open, with timing and loaded-DLL identity to interpret those observations.
  - Use one implementation revision across both outputs, replacing the draft's separate layout `schema` labels. The number refers to this history; it does not select compatibility branches or identify the Windows process.
  - Keep snapshot and activity serials as independent per-process counters: snapshots can advance each completed turn, while activity serials connect transitions with the snapshot that describes them.
  - Disclose publication failures and retain the last observed state without claiming that an absent process proves a crash or that a shutdown observation proves successful process termination.

## Maintenance template

Increment compiled `SAS_RUNTIME_REVISION` once for the next qualifying commit and prepend one entry:

```markdown
### Revision N - SAS practical XXXX

- **Date:** YYYY-MM-DD
- **Git commit:** pending
- **Change:** Explain the runtime-recorder change and why it matters to readers of its observations.
```

Keep only the latest entry's Git commit as `pending`; when adding the next revision, replace the preceding entry's `pending` with its finalized full SHA from the default branch, reconciling after an amend or rebase.

Do not put the current commit's own SHA into its entry: editing the document would change that SHA again. Exact loaded-DLL identity remains separate evidence in both runtime files.
