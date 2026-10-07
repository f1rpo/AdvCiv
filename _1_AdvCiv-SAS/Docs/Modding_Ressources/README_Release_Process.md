# AdvCiv-SAS Release Preparation and Release Checklist

This is a living release checklist for the user and AI/LLM helpers working on AdvCiv-SAS. It is intended to reduce release tedium and forgotten maintenance, not to force every step when it is irrelevant. Prefer verifying the current repository state over mechanically following stale version numbers or examples in this document.

## General principles

- Treat the current repository, Git history, staged diff, supporting docs, and test results as the source of truth; do not reconstruct a release from memory alone.
- Do not create, move, force-push, or publish a Git tag unless the user explicitly asks. For the preferred current tag workflow, use [the "example with more advanced version (totally not shameless...)" subsection](/_1_AdvCiv-SAS/Docs/Modding_Ressources/README.md#example-with-more-advanced-version-totally-not-shameless-or-yes-xd-but-it-might-also-help-as-well).
- Do not publish to CivFanatics, ModDB, Google Drive, GitHub Releases, or other distribution pages automatically. Prepare the files/text for user review and let the user perform publication unless explicitly asked otherwise.
- Keep transient development history out of player-facing release notes. The release changelog should describe the final net result of the release.
- Preserve historical references when they are intentionally tied to an old version, commit, screenshot, comparison file, or Known Issue. Do not globally replace old AdvCiv version strings.
- Prefer a clean working tree before final release preparation. Temporary local diagnostic settings, debug logging, generated build intermediates, and unrelated experimental changes should not leak into the release.

## 1. Establish the release boundaries

- Identify the candidate release commit/HEAD and practical AdvCiv-SAS version number (`git rev-list --count HEAD`).
- Identify the previous stable release boundary or tag that should be used for the player-facing cumulative changelog.
- Distinguish a stable release from an update to the same stable release. Existing tag conventions and the preferred advanced tag example are documented in [Modding Ressources](/_1_AdvCiv-SAS/Docs/Modding_Ressources/README.md#example-with-more-advanced-version-totally-not-shameless-or-yes-xd-but-it-might-also-help-as-well).
- Record the exact old and new SHAs before generating release comparisons, changelogs, or Git logs.
- If the release is still being developed, prepare draft material but do not freeze the changelog/version metadata prematurely.

## 2. If base AdvCiv was updated, finish that integration first

- Fetch current upstream refs when appropriate: `git fetch upstream --prune`. The light-source helper can do this explicitly with `--fetch-upstream`; ordinary archive generation intentionally remains network-free.
- After fetching, inspect the available **release-looking upstream branches**, not only the release branch SAS previously used. A newly created stable branch such as `1.15`, `2.0`, etc. may be the intended new baseline even if an older release branch such as `1.14` also received commits. `_SNAPSHOT_CONTEXT/pending_upstream/UPSTREAM_REFS.txt` and the pending-release history are useful for this check. Let the user decide which newly available release should become the SAS base when the answer is not obvious.
- Merge the intended AdvCiv release/ref on a temporary merge branch when conflicts are expected, then review conflicts semantically instead of blindly choosing "ours" or "theirs".
- Use `_SNAPSHOT_CONTEXT/pending_upstream/` from the light-source ZIP to inspect fetched-but-not-yet-merged release commits. During an active merge, exact `MERGE_HEAD` is authoritative.
- Before creating the final merge commit, fetch upstream once more and check both whether the selected release branch moved during conflict resolution and whether a newer release branch appeared. If the selected branch moved, decide whether to restart/update the merge against its new tip; if a newer release branch appeared, let the user decide whether to switch the intended base rather than silently extending the current merge.
- Compile and test the merged code before creating the final merge commit.
- After a successful merge, update current baseline wording that truly refers to the version of AdvCiv SAS now incorporates. Prefer generic wording such as "base AdvCiv" where the exact number adds maintenance without useful information.
- Do not rewrite intentionally historical references, for example old Known Issue observations, old changelog examples, frozen comparison helpers, or a file explicitly generated from AdvCiv 1.12.
- Keep numbered base-AdvCiv release refs on the AdvCiv-SAS GitHub repository when useful for historical comparisons. Prefer explicit ref pushes rather than mirroring every upstream branch, e.g. `git push origin upstream/X.Y:refs/heads/X.Y`.
- When the base release changes, let the user manage the GitHub comparison PR history. The usual SAS workflow is to keep/close the old do-not-merge comparison PR as a historical comparison, mirror the new base release branch to the SAS repository, and open a new do-not-merge comparison PR with that release as the base and `main` as the head. Then update the current README comparison link/title wording and any CI `pull_request.branches-ignore` entries that exist solely for that reference comparison. Reopening/retargeting an existing PR is also possible if the user deliberately prefers that instead.

## 3. Build and technical validation

- Follow the tested [DLL Compilation Guide](/_1_AdvCiv-SAS/Docs/Modding_Ressources/README_DLL_Compilation.md). Before every full compile attempt, delete that configuration's exact `CvGameCoreDLL/Project/temp_files/<target>` folder. A retained ignored Debug-opt target is useful for its matching PDB and does not affect a clean Release build, but it must be deleted before rebuilding Debug-opt.
- Build the Release DLL from the final candidate source. Do not accept a DLL resumed from partial intermediates after a failed compile.
- Copy/verify the generated DLL as described by the compilation guide, then clean `temp_files` back to `.gitkeep`.
- AdvCiv-SAS distributes only `Assets/CvGameCoreDLL.dll`, compiled for the supported 48-civilization-player cap. Do not maintain alternate player-cap DLL variants as release artifacts; modders who want another `MAX_CIV_PLAYERS` value can compile it from source.
- Check `git status --short`, the staged diff, and relevant generated/cache files after compilation so that the committed DLL corresponds to the final source.
- Run the repository's relevant local/GitHub checks. See [the workflow documentation](/.github/workflows/README.md). Pay particular attention to launch/debug guards, disabled default logging, XML validation, Python 2.4 compatibility, AIP predump freshness, world-size alignment, map-script coverage, and DLL/temp-file checks.
- Perform at least a practical Civ4 smoke test of the release candidate. For substantial AI/gameplay changes, use relevant autoplay/save-file regression tests rather than relying only on successful compilation.
- Review recent `SASGameRecord`/BBAI evidence when the development batch changed AI behavior enough that new mistakes could have been exposed.

## 4. Refresh base documentation copies when upstream changed them

- If the merged AdvCiv release changed its manual, keep the repository copies of `manual.odt` and `manual.pdf` synchronized with the merged upstream version.
- `manual.txt` is an AdvCiv-SAS convenience copy for grep/LLM/global-text search. Regenerate it from the ODT/source document when the manual changes, rather than extracting from the PDF unless there is a specific reason to do so.
- Regenerate the searchable copy with the repository-local standard-library helper: `python ./LLM_Helpers/convert_advciv_manual_to_txt.py`. It reads ODT `content.xml` directly and does not require Pandoc, LibreOffice, or another external converter. Inspect the resulting `manual.txt` diff for missing text, duplicated headers, encoding damage, or layout noise before committing.
- Do not assume that a binary ODT/PDF merge or copy automatically refreshes `manual.txt`.

## 5. Audit current documentation and screenshots

- Search current non-historical docs/configuration for stale release/version references after an upstream merge. Classify each occurrence before editing: current baseline wording, historical reference, or frozen generated comparison.
- Prefer removing unnecessary exact base-version numbers from general prose to reduce future release maintenance, while keeping exact versions where they identify a real comparison/source.
- Let the user perform the final screenshot audit and replacement. Small visual differences, framing/cropping choices, and whether an existing screenshot is still "good enough" are often faster and more reliable for the user to judge directly than for an LLM to infer from code/assets alone.
- The LLM should instead identify **likely screenshot candidates** from the release diff and explain briefly why each may now be stale. Examples: after a major technology-tree rework, suggest the Tech Advisor/merged Tech Advisor and Sevopedia Technology screenshots; after many civilization-specific unit/building changes, suggest Sevopedia Civilization plus relevant Unit/Building examples; after terrain/feature or unit-modifier changes, suggest the corresponding Sevopedia Terrain/Feature/Unit pages; after advisor/UI-layout changes, suggest screenshots of those specific advisors/screens.
- Do not try to exhaustively refresh every image. Give the user a short candidate list ordered by how likely/material the visual change is, then let the user inspect and decide which screenshots actually need replacement.
- Do not refresh screenshots merely to change a version number if the displayed UI is otherwise unchanged.
- Check README links, image references, install/setup instructions, Main Changes Guide links, and any player-facing version requirements affected by the release.

## 6. Build the player-facing cumulative changelog

- Use the complete published release texts in [`_1_AdvCiv-SAS/Docs/changelogs_web/`](/_1_AdvCiv-SAS/Docs/changelogs_web/) as local style/examples rather than maintaining separate example or checkpoint copies. Prefer a recent comparable release when available, while adapting sectioning to the release at hand.
- Produce the release changelog as plain text suitable for sites such as CivFanatics and ModDB where the user may add website-specific bullets/HTML formatting manually. Do not depend on Markdown-only formatting for the publishing copy.
- Before the detailed body, draft a short release overview that explains the main development arc and **why** the release matters rather than enumerating changes. Keep it substantially shorter than the body. The user may prefer to rewrite this overview personally so it expresses the release's purpose in exactly the intended voice; when that is likely, prioritize LLM effort on the body instead of over-polishing the introduction.
- Ground the overview in the actual cumulative commit/diff range and final repository state. It should describe the shipped net result, not temporary development history or a catalogue of unfamiliar asset names. Prefer one or two understandable consequences/examples over many unexplained examples.
- For AI-heavy SAS releases, it can be useful to explain the recurring empirical development loop when it materially shaped the release: add targeted BBAI/SASGameRecord diagnostics -> reproduce questionable behavior in-game/autoplay -> improve/fix the AI decision -> add or refine diagnostics -> verify and continue. When relevant, explain that SASGameRecord acts as a compact external whole-game observer designed especially for efficient LLM review, making long-game AI investigation broader and easier than isolated raw logging alone.
- Start from the full commit range between the previous stable release boundary and the candidate release. Review `git log`, cumulative `git diff`, commit diffs, `changes_old.md` and `changes_new.md`/development notes when available, Main Changes Guide, Known Issues, asset-rebalancing/supporting docs, and the final XML/code rather than relying on commit titles alone. The light-source ZIP normally carries the current post-stable Git log and commit-diff context and is a good external-LLM source.
- Treat the **previous stable release -> candidate release** comparison as the player-facing truth. Temporary development-only names, values, designs and experiments are evidence for understanding the final result, not changes that players experienced. A feature that did not exist in the previous stable release should be described as new even if it was renamed or redesigned several times before release.
- Use the Main Changes Guide as the preferred source/wording for behavior it explicitly documents, especially AI/features that were edited repeatedly and already have user-facing rationale. Version requirement markers such as `(Requires AdvCiv-SAS X+)` are useful review anchors while drafting, but can be removed from the final web copy after the release boundary has been verified.
- Treat the **position** of a Main Changes Guide version marker as evidence about what was new. A bullet that **starts** with `(Requires AdvCiv-SAS X+)` is usually wholly new at that boundary. A marker inserted **mid-bullet** usually means the bullet already existed and only the following clause/update was added later; inspect the corresponding commit diff (or stable-boundary version of the file) and extract only the genuinely new part. Do not promote the whole older bullet into the new release changelog merely because a later `Requires` marker appears inside it.
- Before finalizing the changelog, mechanically audit every post-stable `Requires AdvCiv-SAS` marker that appears mid-bullet, plus any marker-at-start bullet that reuses an older bold heading such as an `Update:` entry. Compare those cases with the last-stable guide or the exact commit diff. This catches older feature descriptions that were merely extended later (for example an existing logging category gaining more fields) and prevents old UI/AI behavior from being presented as new.
- Still inspect **every** post-stable commit, including commits whose main idea is already represented in the Main Changes Guide. Git history/commit diffs often contain orthogonal concrete release information that the guide intentionally omits: exact balance values, individual unit/building/resource changes, smaller bug fixes, tooling/diagnostics, migration work, and rationale.
- For balance/content changes, preserve useful final numerical values (production/research costs, strength, withdrawal, yields, modifiers, prerequisites, etc.) when they make the change materially clearer. Compare the previous stable value directly with the final value; do not publish intermediate tuning values. If a current technology-cost ordering merges/splits tiers, align the comparison explicitly when useful rather than implying a false one-to-one historical tier.
- Prefer purposeful wording over catalogues. When the Git history/supporting docs give a documented design reason, briefly explain the problem or strategic goal (for example, separating an overloaded technology into clearer strategic choices, or moving a civilization-specific building to a class that better supports that leader/civilization's intended play). Do **not** invent rationale when the evidence is unclear.
- Review Known Issues fixed/improved since the previous stable release and include meaningful fixes. Use concise `KI#N` references when useful, with enough plain-language explanation that the release note still makes sense by itself. Near the end, perform a final Known Issues sweep against the complete post-stable issue range: if a fixed/improved KI is not already represented by substance elsewhere, add it to a compact `Other issues fixed/improved` section rather than silently omitting it. Pending/unresolved issues remain in Known Issues rather than being presented as shipped fixes.
- If a systematic source-archeology/bug-audit tracker is active for the release, distinguish **investigation completion** from **implementation completion**. A completed historical/source sweep can legitimately ship only a first tranche of its fixes while deferred findings remain tracked for later releases. Summarize the method/results briefly when it is a meaningful release theme, link the tracker for technical detail, and do not imply that every discovered issue was fixed. Credit investigation versus implementation/validation roles accurately when documented.
- Include meaningful gameplay/balance/content, AI, UI/quality-of-life, bug-fix, performance, mapscript/map-generation, and notable tooling/diagnostic/documentation changes. Preserve more substance in the working draft when uncertain; the user can trim later. Omit tiny refactors and internal churn unless they materially affect users/modders or explain an important reliability improvement.
- Sectioning may be adapted creatively for the particular release, but the Main Changes Guide's organization is usually a good default. For civilization-specific replacements, a compact stable-to-current visual form such as `Arabia: Madrassa (University) -> Furusiyya Maydan (Stable)` is often clearer than burying the replacement inside prose.
- Consolidate iterative development into the final net change. If a technology/building/unit was changed several times during development, write one bullet describing the final release state and its meaningful difference from the previous stable release; do not narrate every temporary intermediate value.
- Likewise, omit a feature/change that was added and fully reverted before release unless the investigation itself produced a meaningful shipped fix/tool worth mentioning.
- When one commit contains several unrelated changes, extract only the release-relevant portion instead of copying the whole commit message.
- When it is unclear whether a detail is meaningful, preserve more context in the draft for user review rather than silently discarding it. The user can trim it later.
- Use the final repository diff/XML/code to verify factual values and wording. A changelog must describe what ships, not what an earlier development commit temporarily did.
- For AI changes, keep the body readable but include striking empirical before/after numbers when they convey the scale or reliability of the improvement better than abstract prose (for example Worker coverage, military-production response, pathological unit composition, or validation counts). Avoid turning every AI item into a test log.
- Do not over-detail broad terrain/feature modifier tables in the web changelog when a short design summary captures the release direction and the supporting rebalancing guide contains the exhaustive values. Preserve core rules/limits and the intended effect instead.
- CivFanatics currently has an approximately **50,000-character limit per post** and ModDB approximately **100,000 characters**. Do not prematurely delete meaningful body substance just to force one CFC post; prepare the complete body first, then compact or split it if needed.
- For the final CFC-oriented plain-text copy, keep **no blank lines between individual items within a section**. The user's website-side HTML/list formatting can provide indentation/visual separation. Blank lines may still separate major section boundaries and overview paragraphs.
- Keep cumulative compare information/SHAs near the end in the style of the existing sample when useful.
- Routine working drafts/checkpoints do not need to be committed. After publication, archive the final public release text(s) in `_1_AdvCiv-SAS/Docs/changelogs_web/<version>.txt` with release/source/retrieval provenance. When the same release has distinct CFC/ModDB/forum texts, preserve them as clearly labeled blocks in the same version file rather than flattening them together.

## 7. Prepare Git history/reference updates

- Refresh the tracked anonymized AdvCiv-SAS Git log using the incremental method documented in [Modding Ressources](/_1_AdvCiv-SAS/Docs/Modding_Ressources/README.md#git-log-with-anonymized-email-in-a-txt). Preserve the existing manually cleaned history and append only the intended new range.
- If base AdvCiv history/log copies are intentionally maintained and the merged upstream range extends them, update those copies consistently rather than replacing historical text with a new unrelated dump.
- Generate a fresh light-source ZIP with `--fetch-upstream` when a release review should include the latest upstream awareness as well as current SAS state.
- Verify that `_SNAPSHOT_CONTEXT/pending_upstream/` becomes empty for the just-merged release lineage after the merge commit, except for any newer fetched upstream commits or genuinely divergent release-line commits not contained in SAS.

## 8. Final release-candidate review

- Confirm the intended practical version number and exact HEAD SHA.
- Confirm the working tree is clean except for intentionally uncommitted local files that are excluded from the release.
- Confirm the committed `Assets/CvGameCoreDLL.dll` is current.
- Confirm default debug/BBAI/SASGameRecord settings are release-safe.
- Confirm regenerated/predumped data is current.
- Confirm the Main Changes Guide, install/setup docs, release changelog draft, manual text copy, and important screenshots are current enough for this release.
- Inspect the cumulative diff from the previous stable release for accidental binaries, local settings, debug artifacts, temporary test code, or forgotten changes.
- Run final Civ4 smoke/autoplay checks appropriate to the size of the release.
- Let the user review the final changelog and release-facing wording before publication.

## 9. Tagging and publication

- Let the user handle Git tags by default. See the preferred [advanced tag example](/_1_AdvCiv-SAS/Docs/Modding_Ressources/README.md#example-with-more-advanced-version-totally-not-shameless-or-yes-xd-but-it-might-also-help-as-well).
- The special `SAS_VERSION_ANCHOR_<version>` tag used by runtime archive-version reconstruction is **not a release tag**. It is a one-time immutable technical anchor whose numeric suffix must equal `git rev-list --count <anchor>`; do not move, rename, delete, or reuse it when updating stable release tags. For this infrastructure, anchor the known AdvCiv-SAS 6356 commit once with `git tag SAS_VERSION_ANCHOR_6356 5234f5012d745b96fd83b8bf60d3031de69c2a01` and `git push origin SAS_VERSION_ANCHOR_6356`. Normal future commits/releases require no version-anchor maintenance.
- When asked to prepare a tag command, use the actual final release SHA/version and previous stable release boundary rather than stale examples from the documentation.
- Prepare the plain-text changelog for CivFanatics/ModDB and any concise download-page summary, but leave website-specific formatting/publication to the user unless explicitly requested.
- Keep the chosen primary download link stable when that is the current release policy, while updating the hosted archive behind it as appropriate.
- After publication, verify the public tag/download/release references point to the intended final commit/files.

## 10. After release

- Update release/tag examples or version-specific documentation only when doing so adds useful future guidance; avoid turning every example into a maintenance burden.
- Update the incremental Git log and archive the newly published CFC/ModDB release text in [`Docs/changelogs_web/`](/_1_AdvCiv-SAS/Docs/changelogs_web/) if it has not already been captured.
- Record post-release hotfix/update boundaries clearly so the next cumulative changelog uses the intended previous stable release rather than accidentally starting from an intermediate development commit.
- Keep newly discovered release-process omissions or repetitive manual steps as improvements to this checklist. The goal is for future LLM/user release preparation to become more reliable and less tedious over time.
