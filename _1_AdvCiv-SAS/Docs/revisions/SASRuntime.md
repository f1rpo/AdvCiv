# SAS runtime recorder revisions

`SASRuntimeStatus.json` describes the latest observed state; its companion `SASRuntimeHistory_*.jsonl` retains timed activity transitions. Both report the same compiled `SAS_RUNTIME_REVISION` as `revision`, identifying their recorder implementation independently of SASGameRecord's revision.

Increment once per commit changing the loaded runtime recorder, its interfaces, configuration, call sites or shared runtime services, including implementation comments/provenance. Keep one pending revision throughout drafting and amend cycles.

Pure CI/checker, test, documentation and historical-example maintenance does not change the loaded recorder and does not require an increment or DLL rebuild.

Add a latest-first entry explaining the behavior and rationale. The diagnostics check requires consecutive history entries, agreement with the compiled value, and exactly one increment per recognized implementation commit. Shared-code detection uses changed runtime symbols; review unusual refactors manually because CI cannot infer every dependency.

## History

### Revision 1 - SAS practical 6642

- **Date:** 2026-10-18
- **Git commit:** pending
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
