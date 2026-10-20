# SAS runtime recorder revisions

`SASRuntimeStatus.json` describes the latest observed state; its companion `SASRuntimeHistory_*.jsonl` retains timed activity transitions. Both report the same compiled `SAS_RUNTIME_REVISION` as `revision`, identifying their recorder implementation independently of SASGameRecord's revision.

Increment once per commit changing the loaded runtime recorder, its interfaces, configuration, call sites or shared runtime services, including implementation comments/provenance. Keep one pending revision throughout drafting and amend cycles.

Pure CI/checker, test, documentation and historical-example maintenance does not change the loaded recorder and does not require an increment or DLL rebuild.

Add a latest-first entry explaining the behavior and rationale. The diagnostics check requires consecutive history entries, agreement with the compiled value, and exactly one increment per recognized implementation commit. Shared-code detection uses changed runtime symbols; review unusual refactors manually because CI cannot infer every dependency.

## History

### Revision 5 - SAS practical 6646

- **Date:** 2026-10-19
- **Git commit:** pending
- **Change:** Add stronger event-driven menu and initialization observations where the Python/UI interfaces expose state, while keeping unavailable EXE presentation facts explicitly unresolved.
  - `MAIN_MENU_STATE_OBSERVED` records the first positive `CyInterface().isInMainMenu()` sample in a no-game interval, but only when an explicit menu-capable Python screen entry point already runs. `mainMenuStateObservedUtc` therefore proves menu state at that callback; it is not claimed as the initial rendered-menu arrival time.
  - Civilopedia visibility distinguishes `MAIN_MENU` and `IN_GAME` context in both the current JSON and history. The top-level entry covers standard Civilopedia/Sevopedia; Sevopedia's shared screen-creation hook also covers direct in-game page links.
    - Show observations require an active screen. Hides use the EXE's no-screens notification, with an inactive-screen check after close handlers as another observation path; duplicate hides are suppressed.
      - Real menu/game tests retained `civilopediaVisible=1` after closing Sevopedia: `PythonDbg.log` recorded screen `200` followed by `SCREEN OFF`, but the `onClose` path supplied no hide row. The existing `toggleSetNoScreens()` callback closes this gap without frame polling or assuming when `isActive()` changes during dispatch.
    - `civilopediaVisible` and `civilopediaContext` keep this overlay separate from the underlying activity clock.
    - `civilopediaActivePlayer` and `civilopediaActiveCivilization` identify the in-game UI owner at observation time, not the page or 3D model being previewed. Main-menu observations use `-1` for both; hide rows retain the last observed context until reset/new-session clears it.
  - BUG's EXE-driven `PreGameStart` extension records `PYTHON_PRE_GAME_START` once per current session as `pythonPreGameStartUtc`. This is a Python initialization boundary for new/load flows, not proof that any loading/slideshow frame was rendered, and its ordering relative to native-ready is not assumed for every load path.
  - These are observation-only timeline rows: they preserve the underlying `NO_GAME`, creation/load or play activity interval instead of inventing a menu-overlay/loading-screen activity. Python checks the cached native enable flag before diagnostic-only interface/screen queries; the samples occur only on existing callbacks, without frame polling.
  - The exact initial main-menu frame, exact loading-frame visibility and the EXE-owned main-menu Exit button remain unavailable through the reviewed SDK/Python callbacks. Continue to report scene/loading requests and orderly teardown as separate evidence rather than inferring those missing facts.

- **Follow-up, 2026-10-19:** The menu/loading-visibility and EXE main-menu Exit-button investigations were closed for the current callback-based recorder after live tests and SDK/Python review. Neither missing feature was added. (ChatGPT-5.6-Sol investigation, reviewed with Codex.)
  - **Initial menu/loading visibility:** reviewed getters select assets, `isInMainMenu()` samples state, and `PreGameStart` marks initialization; no callback establishing the first displayed frame was found. Polling or more asset observations would still not establish that render boundary.
    - Proactively calling `getSoundtrack()` would also change SAS's shuffle bag/ASyncRand state, while merely observing its existing call would add another asset-request proxy.
  - **EXE main-menu Exit button:** its handler is inside the EXE, unlike the in-game `CvDLLButtonPopup::OnOkClicked` path. The real button test produced orderly teardown without a Windows close-request or dedicated UI-request row.
    - Generic destruction/quit/uninitialization observations identify shutdown, not its originating button; additional hooks would not reliably distinguish that source. Existing requests, state samples and teardown remain separate evidence.

### Revision 4 - SAS practical 6645

- **Date:** 2026-10-19
- **Git commit:** `5174aae5d8ec4b2172ca4a8bd31e08f6c4429700`
- **Change:** Observe Windows close messages as distinct best-effort exit-request evidence without replacing Civ4's window procedure.
  - When runtime status is enabled, observe both sent messages (`WH_CALLWNDPROC`) and queue removal (`WH_GETMESSAGE`) on the window's thread.
    - Before a visible window is available, arm on the lifecycle caller's thread and bind a later top-level close target; later lifecycle callbacks can retarget the hooks.
    - Disabled status performs no window enumeration or hook installation; enabled ordinary messages incur only short filters.
  - Distinguish `WM_CLOSE` from the earlier `WM_SYSCOMMAND` / `SC_CLOSE` command.
    - `WINDOW_CLOSE_MESSAGE_OBSERVED` and `WINDOW_CLOSE_COMMAND_OBSERVED` identify sent-message observations; their `DEQUEUED` counterparts identify removal from the message queue, before the EXE decides whether to dispatch or handle them directly.
    - They do not identify red-X versus Alt+F4/automation, and explicit Civ4 UI exit requests remain separate evidence.
  - The first real red-cross/autoplay run exited with the observer armed but no close row. The expanded Windows fixture reproduced a blind spot in the original direct-`SendMessage` test: posted close commands need queue observation even when subsequently dispatched.
    - Ignore non-removing peeks, and leave retrieved messages unchanged. The first run establishes missing evidence, not which message the EXE handled.
    - The rebuilt autoplay/red-cross retest recorded a sent `SC_CLOSE` command, followed by orderly teardown, without a `WM_CLOSE` observation. Queue handling remains separately covered by the Windows fixture.
    - Initial-main-menu red cross also recorded `SC_CLOSE`; in-game Exit to Desktop recorded its explicit UI request instead. The tested EXE main-menu Exit button reached orderly teardown without a close-command/message or dedicated UI-request row; absence of those rows alone does not identify that button.
  - A close-message observation enters the existing `EXIT_REQUESTED` activity when no exit request is already active; when an explicit UI exit already started that activity, the close message is retained as an observation-only transition instead of restarting the interval.
  - The fixed snapshot exposes whether close observation was successfully armed in the process; `windowCloseRequestObservedUtc` retains the latest observed request time for either message or command. Orderly `CvGame` destruction publishes its final state before unhooking; abrupt process termination remains identifiable only through stale process identity, not a fabricated crash cause.
  - Both hooks chain through `CallNextHookEx`; the recorder neither changes queue messages nor replaces Civ4's window procedure. Window discovery/rearming remains confined to non-turn lifecycle observations. Roll back partial installation, and release both hooks after the final orderly snapshot.

### Revision 3 - SAS practical 6644

- **Date:** 2026-10-19
- **Git commit:** `39a7ad075bd95ce94f6bb683b3436da693ec4033`
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
