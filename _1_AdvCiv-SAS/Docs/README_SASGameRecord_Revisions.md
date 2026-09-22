# SASGameRecord Revision History

> Historical reconstruction from the AdvCiv-SAS commit-diff corpus, reconciled through practical version 6439 (`03b44b8932a0715be234e8c5332fcb8a1d1cf3b2`).

## Revision policy

`SASGameRecord` uses one monotonically increasing **revision** number as a downstream-update signal, not as a compatibility promise. The purpose is simple: an external mod/parser/tool that supports revision N and encounters N+1 knows that the official SASGameRecord implementation changed and should review/update its copy.

- Increment the revision for every intentional SASGameRecord-related code change that a downstream copy may want to review: emitted data/semantics, row/field names, recording conditions, recorder bug fixes, relevant bridges/call sites/configuration/checkers, implementation/refactors/optimizations, and code comments or provenance headers in those implementation files. The goal is deliberately simple: if the official SASGameRecord-related code changed, a copied implementation can notice that there is something new to review.
- Do **not** spend time deciding whether a change is backward-compatible or behavior-changing. When in doubt, increment.
- Do not increment for standalone documentation/history edits, refreshed example logs/screenshots/packages, or unrelated gameplay commits that merely used SASGameRecord as validation evidence.
- Multiple SASGameRecord changes deliberately combined in one Git commit are one revision; this is an update counter, not a hunk/field counter.
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
GAME_RECORD_SOURCE_CONTEXT recordRevision=124 ...
```

After this, each qualifying SASGameRecord update increments `SAS_GAME_RECORD_REVISION` by one and adds one short latest-first entry to this file in the same commit. The revision is only a downstream-update signal; exact runtime source identity remains in `GAME_RECORD_SOURCE_CONTEXT`.

## Consumer/parser notes

- **Runtime rows are the dynamic game record, not an exhaustive static schema manifest.** The build's exact revision/source identity and `GAME_RECORD_LOG_SETTINGS` tell a consumer which implementation/settings produced the file; loaded-XML/rules rows preserve runtime-dependent mappings. An exhaustive per-run list of every row type the source *could* emit would repeat gameplay-independent implementation metadata and still require maintenance for conditional/dynamic `GAME_RECORD_ACTION type=...` families. Keep such schema/consumer guidance in source/docs/checkers rather than enlarging every record with a static manifest unless a concrete downstream consumer later proves that worthwhile.
- **Object IDs are namespace-local unless a row explicitly says otherwise.** In particular, Civ4 `cityId` and `unitId` values are player-local; trace them as `(player, cityId)` and `(player, unitId)`, not as globally unique integers. Team/player slot IDs remain their ordinary game slot identities.
- **Missing-value interpretation is field-specific.** `-` is the common textual/not-applicable token, while numeric `-1` is also used by native Civ4 `NO_*` IDs and by some unavailable numeric fields. Do not globally rewrite every `-1` as null; parse the documented field meaning/context.
- **Text-map body rows deliberately remain raw pipe-framed art.** Structured `BEGIN`, `LEGEND`, `LAYER_BEGIN`, `LAYER_END`, and `END` framing rows carry the parseable metadata; revision 122 makes every structured legend/layer frame self-locating by repeating `turn`, while the fixed-width `|...|` drawing rows remain undecorated.
- **Level-3 periodic city detail is checkpoint-plus-delta encoded from revision 123 onward.** Full `GAME_RECORD_CITY`, `GAME_RECORD_CITY_DEVELOPMENT`, `GAME_RECORD_CITY_HAPPINESS`, `GAME_RECORD_CITY_HEALTH`, and `GAME_RECORD_CITY_BUILDINGS` rows establish complete state. Matching `*_DELTA` rows inherit every omitted field from the immediately preceding observation of the same `(player, cityId)` and base family, apply only explicitly present changed fields, and expose `previousTurn`/`fullBaseTurn` so missing chains are detectable. Every new/load log session and every tenth observation returns to a full row. `GAME_RECORD_CITY_TRADE_PARTNERS` remains full. The reference `LLM_Helpers/expand_sasgamerecord_city_deltas.py` can restore these five families to ordinary full rows while validating the chain.
- **Detailed AI-intent coverage follows AdvCiv-SAS-supported gameplay rather than every inherited compatibility mode.** War-plan causal provenance targets foreground UWAI. `GAME_RECORD_WAR_AI_SETTINGS` still identifies inherited K-Mod/Legacy configurations and factual `WAR_PLAN_CHANGED` history remains available, but SASGameRecord does not promise equivalent deep causal provenance for intentionally unsupported Legacy war/peace planning. Deeper rejected-candidate/value decomposition remains BBAI/source-investigation territory.

## Reconstruction method

The history below counts commits that changed the recorder implementation itself (`SASGameSummaryLog.*` / `SASGameRecordLog.*`) or changed runtime/configuration call sites directly participating in the structured record.

It intentionally excludes broad text-search hits that only mentioned the record in documentation/comments, used it as test evidence, refreshed example logs, or changed packaging/tooling without changing the recorder implementation.

This is why the original reconstruction produced 67 revisions through practical 6438 rather than the much noisier ~166 broad-history-hit figure. The stricter current policy additionally counts later intentional code-comment/header-only updates in SASGameRecord-related implementation/checker files.

Because this numbering is reconstructed after the fact, the descriptions are concise summaries of the canonical commit diffs/messages rather than claims that these revision numbers were emitted by historical builds.

## History (latest first)

### Revision 124 - SAS practical 6565

- **Date:** 2026-09-23
- **Git commit:** pending
- **Change:** Generalized periodic territory irrigation reporting from hardcoded Farms to every improvement whose loaded XML sets `bCarriesIrrigation`, and added compact city-network component rows.

`GAME_RECORD_TERRITORY_DEVELOPMENT` now reports irrigation carriers and their irrigated/dry, bonus, BFC, suburb and by-improvement breakdowns. This matches the Worker evaluator's XML-driven irrigation model, so mod-added food, production or commerce carriers remain visible without new C++ type branches. The same existing territory scan also splits routes on Worker-developable land into periodic BFC and suburb counts, deltas, percentages and route-type lists, making useful city development distinguishable from outer-territory routing.

New `GAME_RECORD_CITY_NETWORK` rows directly list all known cities sharing each `CvPlotGroup` that contains one of the player's cities, along with component plot, route, river-network and terrain-network composition. The implementation reuses the existing expansion map scan and performs no diagnostic pathfinding; conduit counts describe infrastructure present in the connected component rather than claiming an exact mode for every city pair.

### Revision 123 - SAS practical 6552

- **Date:** 2026-09-21
- **Git commit:** `37448599d5f14a155645fe08838df74f41f9db82`
- **Change:** Added bounded level-3 checkpoint-plus-delta encoding for the five largest repetitive periodic city-detail families while retaining ordinary full rows as self-contained recovery points.

`GAME_RECORD_CITY`, `GAME_RECORD_CITY_DEVELOPMENT`, `GAME_RECORD_CITY_HAPPINESS`, `GAME_RECORD_CITY_HEALTH`, and `GAME_RECORD_CITY_BUILDINGS` now emit a full row on the first observation of each city/family in a log session and every tenth observation thereafter. Intermediate `*_DELTA` rows keep `(player, cityId)`, `previousTurn`, `fullBaseTurn`, `changed`, and only serialized fields that changed. New/load log rolls clear every chain; a lifecycle generation guard prevents reused/reacquired city identities from inheriting stale state. `GAME_RECORD_CITY_TRADE_PARTNERS` deliberately remains full because its small payload became larger with delta metadata.

The level-3-only compressor runs after the ordinary city snapshot is already computed/formatted, so levels 0-2 add no city scan, AI query, pathfinding, or RNG work. `LLM_Helpers/expand_sasgamerecord_city_deltas.py` is the strict reference expander: it validates `previousTurn`/`fullBaseTurn`, fails closed on missing bases/links, copies unrelated rows byte-for-byte, and restores the five encoded families to ordinary full rows for older/simple consumers.

Runtime validation used the actual revision-123 C++ output: 14,835 delta rows and 2,200 full city-detail checkpoints compressed a 59,726,600-byte expanded record to 53,805,598 bytes, saving 5,921,002 bytes (about 9.9% overall). Against the revision-122 same-parent control, all 425 state checkpoints, all 425 gameplay-significant RNG checkpoints, and all 37,458 pre-existing factual actions matched after ignoring timing/sequence-only fields. Expansion reproduced the logical city-detail history; its byte-level comparison also exposed a separate pre-existing exact-boundary `CvString::formatv` termination defect, left for a dedicated source fix rather than mixed into this format revision. See also [KI#375.2](/_1_AdvCiv-SAS/Docs/README_Known_Issues.md#ki-375.2)\

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

The row distinguishes turtle-strategy drafting, local-danger drafting when nearby enemy attack strength exceeds local defense, the final non-critical randomized/value route, and the existing forced-caller API exit. Ordinary decisions preserve the already-computed conscript unit/population cost, danger and land-war state, good-value/excess-population context, poor worked plots, happiness allowance, unit-cost pressure, and the trigger-specific local-strength or `AI_buildUnitProb` input.

Recording is reached only immediately before an actual `conscript()` call. Rejected/no-draft city turns remain intentionally unlogged; the live poor-plot count is cached once and reused by gameplay/BBAI/recorder consumers; and no draft valuation, local-strength query or synchronized RNG is repeated solely for recording.

Validation also exposed an older factual-source ambiguity: `CvPlayer::splitEmpire` creates free colony defenders through the same `initConscriptedUnit()` helper, so they had been counted and labeled as ordinary conscription. Unit-completion provenance now uses explicit `PRODUCTION`, `CONSCRIPT` and `COLONY_FREE_DEFENDER` sources; level-2 production flow keeps separate colony-defender count/material/type buckets, while true `AI_doDraft` rows continue to pair with `UNIT_COMPLETED source=CONSCRIPT`.

### Revision 117 - SAS practical 6522

- **Date:** 2026-09-20
- **Git commit:** `5aa5122bee1e089e723de0926a0572209673b0a3`
- **Change:** Added compact level-2 `GAME_RECORD_AI_HURRY_DECISION` provenance for realized AI/automated-governor city hurries.

The row distinguishes panic-forced hurries, population whipping that immediately relieves unhappiness or food loss, and ordinary unit/building value decisions. Value-based rows preserve the already-computed subjective population/gold costs, final target value, valued overflow and value-minus-cost margin together with live happiness/food context; the following `CITY_HURRIED` action remains authoritative for the actual production/population/treasury/anger mutation.

Recording is reached only immediately before an actual `hurry()` call. Rejected hurry types remain intentionally unlogged, no target/building valuation or synchronized RNG is repeated solely for recording, and ordinary no-hurry city turns pay no recorder-only state collection.

### Revision 116 - SAS practical 6521

- **Date:** 2026-09-20
- **Git commit:** `7b12ec59733e99164ea2badfeac4d35420a97be3`
- **Change:** Added compact level-2 `GAME_RECORD_AI_VASSAL_RESOURCE_TRIBUTE` provenance for the master's automatic vassal-resource chooser.

The row preserves the selected missing resource and its already-computed `AI_bonusTradeVal` together with the runner-up/value margin and eligible-candidate count. This chooser is separate from ordinary negotiated resource trading: after other diplomacy gates, the master scans resources its vassal can provide, requires the master to have none, and deterministically chooses the highest-valued tradeable resource.

For an AI vassal, the row is emitted immediately before the existing one-sided resource deal. For a human vassal, `delivery=HUMAN_POPUP` records the selected tribute request before `BUTTONPOPUP_VASSAL_GRANT_TRIBUTE`; a later transfer still depends on the human response. Candidate/runner-up bookkeeping is level-2-only, every `AI_bonusTradeVal` result is reused from the native scan, and no resource valuation, tradeability query, counterproposal or synchronized RNG is added solely for recording.

### Revision 115 - SAS practical 6518

- **Date:** 2026-09-20
- **Git commit:** `f3cf0b1a0a8f07110e2850188f8b25d93402e136`
- **Change:** Added compact level-2 realized `GAME_RECORD_AI_MAP_TRADE_DECISION` provenance for proactive AI map swaps.

The row preserves the two live pre-exchange `AI_mapTradeVal` results: how much the proposer values the target map and how much the target values the proposer map. It also records their value advantage and the strict `DIPLOMACY_VALUE_REMAINDER` threshold applied to the target's received-map value.

A final `MAPS <-> MAPS` package cannot reconstruct those inputs because implementing the deal reveals both maps and changes subsequent map valuation. Existing generic `CONTACT_TRADE_MAP`/deal actions remain authoritative for delivery and payload. The native contact roll, bilateral trade-denial checks, two map valuations and acceptance inequalities remain unchanged, and no map valuation or synchronized RNG is repeated solely for recording.

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

Tribute rows preserve the same caller origin, `AIDemandTypes` route, minimum/final deal value, and the already-computed technology/map chooser context where meaningful; revision 87 remains authoritative for detailed resource-candidate selection. Optional recorder contexts are supplied only at level 2+, and the helpers reuse existing live valuations/results without repeating candidate scans or synchronized RNG.

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

Ordinary deals also preserve the final two live balance values used by the existing 75% acceptance test; city fallback rows preserve war-success rating, selected city and its existing acquire-minus-keep fitness instead. The existing generic `GAME_RECORD_AI_DIPLO_CONTACT` row remains authoritative for the final trade package, so revision 106 adds no duplicate trade-list serialization, target valuation, balancing search or synchronized RNG.

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
- **Change:** Added compact level-2 realized proactive AI diplomatic-contact provenance: contact class, human-contact versus immediate AI-deal delivery, current attitude, optional non-package subject, and the final AI-gives/AI-receives package. Existing deeper peace and proactive resource-trade rows remain authoritative rather than being duplicated.

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
- **Change:** Added exact level-2 per-player/per-area AI target-city history for normal area search, K-Mod random clearing, diplomacy coordination, area reassignment and effective clears when a referenced city is removed, plus compact periodic target checkpoints with the city-target refresh timer. Normal search exposes its already-computed winning target value through an optional diagnostic output; old-target provenance remains level-2 gated without duplicating the refresh loop.

### Revision 96 - SAS practical 6486

- **Date:** 2026-09-18
- **Git commit:** `86485bdd6be96fcb0c3bb7d92ad6885a0fc197a2`
- **Change:** Added exact level-2 AreaAI theater-posture transitions from the two authoritative writers (`CALCULATED` and UWAI land/naval alignment), plus compact periodic non-neutral AreaAI checkpoints and a shared canonical raw enum-name helper. Initial `NO_AREAAI` setup assignment is omitted from transition history; disabled logging keeps the original inner calculation loop.

### Revision 95 - SAS practical 6485

- **Date:** 2026-09-18
- **Git commit:** `084b2bc93ce3734416f2a205eac6b035b3fe7c2d`
- **Change:** Extended the opt-in level-3 active-viewer trade-market snapshot with `GAME_RECORD_TRADE_STRATEGIC`: exact bilateral denial state for maps/vassalage/capitulation/open borders/defensive pacts/permanent alliances/disengagement, city ids the rival will currently cede or refuse with denial reasons, and Glance-style third-party war-for-hire targets/denials.

The scan reuses the existing Foreign-Advisor market gate and viewer scope rather than evaluating every possible observer. Derived willingness is periodic rather than presented as an exact transition because Civ4 computes these queries on demand instead of storing one authoritative willingness state.

### Revision 94 - SAS practical 6484

- **Date:** 2026-09-18
- **Git commit:** `da80e04a653cfed2ec20621d9b8c6185acf33903`
- **Change:** Added complete periodic level-3 AI attitude-component provenance. `GAME_RECORD_DIPLO_ATTITUDE_BREAKDOWN` reconstructs each known cross-team major-AI relationship from the same additive components and order used by `CvPlayerAI::AI_updateAttitude` (first impression, rank/team size, close borders, peace/war, religion, resource/open-border/defensive-pact effects, rival pacts/vassals, expansionism, shared war, favorite civic, trade/rival trade, aggregate memories and attitude-extra), plus the pre-clamp component sum, its normal -100..100 clamped value, cached raw value and gameplay-effective value.

This makes fluctuating diplomacy such as different religion ("heathen"), open borders and fair trade explainable from the record rather than exposing only the final attitude number.

The existing `GAME_RECORD_DIPLO_MEMORIES` row remains the per-memory detail and now shares the same memory scan with the broader breakdown instead of recomputing it separately. Full breakdowns are level-3-only and limited to gameplay-active major AI players (including AI Auto Play); ordinary human outgoing pseudo-attitudes and same-team forced relations are not presented as additive AI reasoning. No extra attitude work is added when level 3 is disabled.

### Revision 93 - SAS practical 6483

- **Date:** 2026-09-18
- **Git commit:** `56f4bce56342897f452363f8f6a52a31c8e52b1c`
- **Change:** Added exact level-2 team worst-enemy transition history. `GAME_RECORD_WORST_ENEMY_CHANGE` records only the final committed `AI_updateWorstEnemy` replacement as `oldEnemyTeam` -> `newEnemyTeam`, together with the old/new enmity values already computed by the real selection pass plus `recursiveRecheck=0/1`. AdvCiv's optional recursive reevaluation after enemy-trade-memory decay therefore cannot emit its tentative first-pass candidate; the recursive final pass commits and records the authoritative result.

The bridge adds no enemy scan, attitude/enmity evaluation, RNG or pathfinding solely for logging. Existing periodic `GAME_RECORD_DIPLO_STATUS worstEnemyTeam=` fields remain the checkpoint for loaded saves and truncated logs.

### Revision 92 - SAS practical 6482

- **Date:** 2026-09-18
- **Git commit:** `d4a67ae0effbeaf879061e9f24c4382da83a7efe`
- **Change:** Added exact readable AI victory-route stage history at level 2. `GAME_RECORD_AI_VICTORY_STAGE_CHANGE` records each final Culture/Space/Conquest/Domination/Diplomacy 0..4 stage transition after `AI_updateVictoryStageHash` finishes, including authoritative resets caused by invalid/capitulated/no-capital state; the existing periodic `GAME_RECORD_AI_VICTORY_STAGES` row remains the loaded-save/truncated-log checkpoint. The bridge compares only the already-computed before/after victory-stage bitfields and repeats no victory evaluation, RNG or pathfinding.

The periodic and exact rows now share one AI/AI-Auto-Play eligibility helper. The revision checker also verifies the revision history's current emitted `recordRevision` example against the source constant so that descriptive marker cannot silently lag after a revision bump.

### Revision 91 - SAS practical 6481

- **Date:** 2026-09-18
- **Git commit:** `9a2348b4029ec5e4bc1c609e00018235d62d0123`
- **Change:** Added complete readable AI-strategy history at level 2. `GAME_RECORD_AI_STRATEGY_CHANGE` records each final gameplay-active non-default `AIStrategy` bit that starts/stops after `AI_updateStrategyHash` finishes its validity cleanup, while periodic `GAME_RECORD_AI_STRATEGIES` checkpoints preserve the complete active set without requiring replay from turn 0. Existing specialized decision rows remain for local context and BBAI retains its detailed K-Mod transition diagnostics/trigger values.

A shared `getSASAIStrategyType` helper in `CvGameCoreUtils` now provides canonical raw enum names to both logs, replacing the fragile BBAI stringification macros without changing their emitted strategy names. Existing SASGameRecord strategy predicates share one AI-Auto-Play-aware wrapper, and the build check guards the contiguous strategy-bit layout, enum-name coverage, and all complete strategy scans against future `AIStrategy` additions. No AI strategy evaluation, RNG or pathfinding is repeated solely for recording.

### Revision 90 - SAS practical 6480

- **Date:** 2026-09-17
- **Git commit:** `e25c3e02b18e1252d873e5b5eb978edf40225d37`
- **Change:** Source-readability maintenance only. Oversized `logSASGameRecord(...)` argument lists are wrapped at top-level argument boundaries while their format strings remain unchanged. A conservative helper now preserves this argument-layout convention alongside the matching BBAI cleanup while deliberately leaving existing prose-comment layout untouched. Emitted SASGameRecord row text, fields, conditions, and gameplay behavior are unchanged.

### Revision 89 - SAS practical 6477

- **Date:** 2026-09-17
- **Git commit:** `18e8765652b15b1726e3c97724f2a150656ef6ad`
- **Change:** Tightened disabled-recorder overhead after the expanded provenance work. `SAS_GAME_RECORD_LOG_LEVEL` is now cached once after all GlobalDefines/module overrides load, making the widespread level gates direct integer reads instead of out-of-line getter calls in ordinary non-LTCG Release builds.

Caller-gated plot/random-event placeholder states no longer initialize unused fields at level 0/1, the plot-owner cause scope avoids an unused disabled-path member write, and unit-event before/after snapshots now live wholly inside the enabled branch. Tiny empty vectors that must span gameplay mutation remain intentionally accepted and locally documented; they perform no recorder scan/allocation/population while logging is disabled. Emitted record semantics are unchanged.

### Revision 88 - SAS practical 6476

- **Date:** 2026-09-17
- **Git commit:** `d8a8df2f2dc046fa57ba8d4679e7d98cf130ab33`
- **Change:** Added compact AI civic/revolution provenance. `GAME_RECORD_AI_CIVIC_DECISION` records live `AI_doCivics` switch candidates from the already-computed current/best civic values, percent threshold, absolute/anarchy slack, bundle-anarchy state and first/recheck pass; accepted candidates are level 2 while rejected/wanted-but-rejected candidates are level 3. `GAME_RECORD_AI_CIVIC_OUTCOME` records meaningful accepted-bundle outcomes: actual revolution, delaying for an imminent researched civic, insufficient-gold blocking, or an unexpected final `canRevolution` block, including the pending/final civic bundle and real timer/anarchy/research/gold context; `BLOCKED_GOLD` also records the net deal gold-per-turn actually used by its reserve formula. No civic valuation or revolution eligibility search is repeated solely for recording.

The same call-site review also gates older BBAI-only financial-trouble/war-state diagnostic lookups so ordinary player-log-disabled gameplay no longer performs that diagnostic work.

### Revision 87 - SAS practical 6475

- **Date:** 2026-09-17
- **Git commit:** `727e094fdc31b3e785825433407b47bacdeccaf8`
- **Change:** Added compact AI resource-trade choice provenance. `GAME_RECORD_AI_BONUS_TRADE_DECISION` records the real proactive `AI_proposeResourceTrade` receive/give chooser winners and runner-ups from already-computed buyer trade value, seller keep-value estimate, bias, probability gate and random final score, then joins them to the actual counterproposal terms that are about to be shown to a human or implemented between AIs. Winner-only factual resource/strategic context is gathered only when an event is emitted; the shared `AI_bonusTradeVal` function itself remains uninstrumented.

`GAME_RECORD_AI_BONUS_DEMAND_DECISION` separately records the different tribute chooser's randomized sorted winner/runner-up, non-surplus sorting adjustment, selected resource bundle/value, minimum threshold and final deal valuation. Existing periodic `GAME_RECORD_TRADE_MARKET` remains the resolved availability/denial/price view. The new rows repeat no bonus valuation, counterproposal search or RNG solely for recording.

### Revision 86 - SAS practical 6474

- **Date:** 2026-09-17
- **Git commit:** `318c4c070c954a548fd44ae819b59819981d3e1f`
- **Change:** Added compact AI research/free-technology choice provenance. `GAME_RECORD_AI_RESEARCH_DECISION` identifies teammate coordination, Python overrides and the real `AI_bestTech` fallback; the latter preserves the chosen/requested technology, actual current research after queueing, deeper strategic aim, chooser/path values, distinct runner-up and chooser breadth from values already computed by gameplay. Level 3 additionally serializes each distinct returned-tech candidate with its chooser/path value and deeper aim, while free-tech choices use the same depth-1 machinery. No technology valuation or chooser RNG is repeated for recording.

The technology-path diagnostic also recovers the actual deepest endpoint after K-Mod backfills/reorders paths with unrelated depth-0 technologies, rather than assuming `path.front()` is always the long-term aim; AdvCiv's rare scenario fallback likewise reports the path actually selected rather than always path 0. These are diagnostic/provenance corrections only and do not change research selection.

### Revision 85 - SAS practical 6473

- **Date:** 2026-09-17
- **Git commit:** `f94667171d4523a595a9153e253ad68c5486680b`
- **Change:** Added compact AI espionage mission-choice and mission-phase interception provenance. `GAME_RECORD_AI_ESPIONAGE_DECISION` retains the scored chooser winner/runner-up and both candidates' already-computed valuation/randomization/overhead/cost components plus chooser risk/EP/strategy context without rerunning valuation or RNG; the separate K-Mod tactical city-revolt path emits `GAME_RECORD_AI_ESPIONAGE_TACTICAL_DECISION` with its factual remaining-defense gate instead of fabricated scores.

`GAME_RECORD_SPY_INTERCEPTION_CHECK` records every real before/after-mission base/final chance and authoritative result/draw plus counterespionage, counter-Spy-defense/Spy, city-defense and recent-mission context; successful non-interceptions are therefore visible alongside existing completed-mission and caught-Spy outcomes. The ordinary logging-disabled/travel-interception path keeps the original `SyncRandSuccess10000` behavior, and logging-only plot/unit scans remain level-2 gated.

### Revision 84 - SAS practical 6472

- **Date:** 2026-09-17
- **Git commit:** `b93cfcd8ae7a6279729c067373532f66d884ea8d`
- **Change:** Added compact AI corporation operational provenance. `GAME_RECORD_AI_CORPORATION_DECISION` records actual Executive production commits, new/retargeted `AI_spreadCorporation` destinations and deliberate Executive airlift/sea-transport routing with only already-computed chooser scores/path context; no corporation valuation or pathfinding is repeated for logging. Periodic `GAME_RECORD_CORPORATION_POSTURE` separates policy-enabled corporations from actually present corporation types/city instances/headquarters and preserves persistent Executive spread missions, including Executive cargo carried by sea transports, at-target/waiting-gold/ready state and level-3 compact targets across loaded saves; it stays absent before meaningful corporation/Executive state exists.

### Revision 83 - SAS practical 6471

- **Date:** 2026-09-17
- **Git commit:** `23a3c7abe45b7f775a7c03f4ab904fe8a31bb1a3`
- **Change:** Added explicit AI fog-control / anti-Barbarian map-control provenance. `GAME_RECORD_AI_MAP_CONTROL_DECISION` records real `AI_guardCitySite` and `AI_guardYield` assignment/retarget changes with their already-computed site/guard/yield, defense/sight/fog-distance, path and action context, without extra target search, pathfinding or fog scans. Periodic `GAME_RECORD_BARBARIAN_PRESSURE` also identifies persistent city-site `MISSIONAI_GUARD_CITY` posture, including level-3 site/guard/current coordinates, so loaded saves retain ongoing fog-control state.

### Revision 82 - SAS practical 6470

- **Date:** 2026-09-17
- **Git commit:** `a864aa13593c9e80df49d4ff8967b7139bd8d00d`
- **Change:** Added compact AI religion-choice provenance at the real `AI_doReligion` boundary. Level 2 records meaningful switch/spread-block decisions with the pre-spread best and runner-up post-bias scores, current/selected religions, favorite/missionary/flavor context, best-religion spread/holy-city state, the raw values actually used by gameplay for switch probability, the real conversion probability/roll and realized outcome. Level 3 additionally serializes every eligible religion's already-computed chooser score. The recorder performs no extra religion valuation or RNG and suppresses routine keep-current evaluations to avoid per-turn noise.

### Revision 81 - SAS practical 6469

- **Date:** 2026-09-16
- **Git commit:** `3bcaa9af2c70561ee62b2c60e938cce216949849`
- **Change:** Added compact peace-decision provenance and knowledge-limited barbarian pressure. `GAME_RECORD_AI_PEACE_DECISION` records the real shared peace-negotiation boundary at level 2: UWAI versus legacy source, war age, initial/final end-war benefit values, provisional reparations, human-counterproposal use, final trade lists when available, and resolved outcomes such as victory-denial block, insufficient reparations/counterproposal, offer sent or AI-AI peace implemented/rejected. It reuses the live negotiation values/package rather than rerunning UWAI or trade valuation.

`GAME_RECORD_BARBARIAN_PRESSURE` complements the omniscient global barbarian rows with what each player's team can actually know: visible/non-invisible barbarian units and city proximity, revealed/visible/actionable-known barbarian cities, explore/city-attack UnitAI state, `MISSIONAI_EXPLORE` groups, and the AI's existing barbarian-defense-focus/attacker-need predicates. `BARBARIAN_UNIT_SPAWNED` also records plot ownership/terrain/feature, revealed/visible major-team counts and the nearest major-civilization city. The new observations add no spawn reevaluation, pathfinding or barbarian target scoring.

### Revision 80 - SAS practical 6468

- **Date:** 2026-09-16
- **Git commit:** `1cb708ab8dfc36d7200bd0b4e02cd70c4d6f62b1`
- **Change:** Added privacy-tiered runtime/install provenance and compact naval operational intent. `GAME_RECORD_EXE_CONTEXT` fingerprints the loaded `Civ4BeyondSword.exe`; level 2 records Windows/Wine runtime/build/update and process/native architecture plus coarse distribution/Steam AppID-build-beta metadata, while level 3 additionally records coarse CPU/GPU vendors, logical processors/RAM and exact EXE/mod paths.

`GAME_RECORD_DISPLAY_CONTEXT` now retries after loaded-save graphics initialization so short sessions do not omit display/GPU context. Level-3 naval posture also records each assault group's existing MissionAI destination, target team/city, cargo/capacity, support/base and ocean capability without new pathfinding or AI evaluation; war-start context now reuses the shared spaceship-parts-percent helper.
Note: Steam detection/path/build metadata are runtime-validated; GOG, Firaxis retail/other and Wine/Proton branches are source-reviewed but not yet runtime-tested.

### Revision 79 - SAS practical 6465

- **Date:** 2026-09-16
- **Git commit:** `439081be51b7ec481f72cda74688fce4ca8b5b33`
- **Change:** Added explicit territory/BFC/suburb unimproved-development totals and level-3 `GAME_RECORD_CITY_DEVELOPMENT` rows with owned/assigned-radius gaps, development/improvement coverage, remaining feature/resource types and bounded plot coordinates. Level 3 also serializes each already-maintained AI city-site shortlist as rank/coordinates/current found value. These are descriptive/cached-state additions only: no Worker build-legality/value scan, city-site reevaluation or pathfinding.

### Revision 78 - SAS practical 6464

- **Date:** 2026-09-16
- **Git commit:** `fb4c3dcb0b8d8fe9f085620a7bdcaf2ea059bc63`
- **Change:** Added compact strategic AI intent: current team war plans/ages, remaining high-level military-strategy flags, selected UWAI war-target context, and level-3 best-target `DEFERRED_ROLL` events without logging exhaustive rejected candidates. Espionage snapshots now distinguish foreign Spy locations from stored attack/recon/guard MissionAI intent. The RNG/state comparator also requires equal explicit `recordRevision` values before recipe-sensitive combined interpretation.

### Revision 77 - SAS practical 6463

- **Date:** 2026-09-16
- **Git commit:** `989eb50ce43e551708d7f573db4b140e28902307`
- **Change:** Added compact expansion/city-site intent: maintained shortlist/threshold/primary-site context, settler `MISSIONAI_FOUND` targets/ranks/cached values, normal founding choice plus two cached alternatives, and exact barbarian chooser winner/two runner-ups with raw/area/final values and random/site-spacing context. All reuse already-computed state/scans, adding no map-wide rescoring or pathfinding; also removed a duplicate `CvInfo_Symbol.h` include.

### Revision 76 - SAS practical 6461

- **Date:** 2026-09-14
- **Git commit:** `66f72899345d54e552c3a40da59b26d7cbdac270`
- **Change:** Strengthened runtime provenance with a compact tracked code/rules/scripts diff fingerprint and byte count in `GAME_RECORD_SOURCE_CONTEXT`, scoped to `CvGameCoreDLL`, `Assets/Config`, `Assets/Python`, `Assets/XML`, and `PrivateMaps`. This distinguishes different edits to the same runtime-relevant dirty paths and, together with the independent exact-byte loaded-DLL fingerprint, makes stale/unrebuilt DLL mistakes directly visible without hashing docs/LLM helpers or adding per-file timestamp noise.

### Revision 75 - SAS practical 6460

- **Date:** 2026-09-14
- **Git commit:** `40b3580cfa826150dfdb659452ec9f9fcf85cbd3`
- **Change:** Added compact periodic naval-assault posture covering assault lift/capacity/loading, current open-ocean capability, fleet-group loading/base-location/mission/support state and exact transported unit composition; also recorded the SAS UWAI naval-logistics deployment gameplay toggle in the canonical war-AI settings row.

### Revision 74 - SAS practical 6455

- **Date:** 2026-09-14
- **Git commit:** `320e73215cf08fa5b5af9fb4231c612a7cebf8f8`
- **Change:** Added one compact all-city `CITY_PRODUCTION_NO_TARGET` outcome at control-path-aware turn boundaries. Manual human cities are sampled before end-turn production processing, while AI-controlled and production-automated cities are sampled afterward; disorder is suppressed, and detailed legality/chooser-path evidence is kept in the dedicated BBAI category.

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
- **Change:** Standardized provenance headers across active AdvCiv-SAS files, including the SASGameRecord implementation, comparison helper and dedicated build checker. Runtime recorder behavior was unchanged, but the SASGameRecord-related code artifacts changed and therefore count under the current strict revision policy.

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

For the next qualifying committed update, increment the public source constant and prepend one entry like this:

```markdown
### Revision N - SAS practical XXXX

- **Date:** YYYY-MM-DD
- **Change:** Briefly state what changed in SASGameRecord-related code and why a downstream copy should review/update.
```

Do not try to place the final SHA of the same not-yet-created commit into its own history entry: changing the document would change that SHA again. Historical entries can retain already-known SHAs, while current runtime Git/source identity is recorded independently by `GAME_RECORD_SOURCE_CONTEXT`.

Standalone docs/history/example-log/package-only updates do not bump the revision. If a qualifying code change and those maintenance files are committed together, they remain one revision.
