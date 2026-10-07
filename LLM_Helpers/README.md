# External helper tools

Primary audience: coding agents (Claude Code, Codex, etc.) invoking these scripts on the user's behalf. Prefer agent-spec bullets over prose; show example output where it clarifies the contract.

This folder contains external helper scripts, mostly Python 3, for tasks outside Civ4's embedded Python runtime. These scripts are meant for LLM-assisted workflows, balancing workflows, code review, refactoring support, report generation, and other tooling.

Important distinction:

- Helper scripts may use modern Python 3.
- Output files do not automatically need to be Civ4 Python 2.4-compatible.
- Only source-rewrite helpers that modify Python files loaded by Civ4 must preserve Python 2.4-compatible target code.
- Text reports, tuning reports, XML summaries, and other external artifacts only need to be useful and accurate for their workflow.

Always review diffs before committing generated source changes.

## Menu

- [Inherited map-script reference sources](#inherited-map-script-reference-sources)
- [Python source cleanup helpers](#python-source-cleanup-helpers)
  - [`collapse_multiline_calls.py`](#collapse_multiline_callspy)
  - [`collapse_multiline_calls2.py`](#collapse_multiline_calls2py)
  - [`collapse_multiline_parens_safe.py`](#collapse_multiline_parens_safepy)
  - [`fix_matrix_indent.py`](#fix_matrix_indentpy)
  - [`collapse_flat_literals.py`](#collapse_flat_literalspy)
  - [`wrap_python2_prints_for_linting.py`](#wrap_python2_prints_for_lintingpy)
  - [`collapse_multiline_brackets.py`](#collapse_multiline_bracketspy)
  - [`fix_line_endings.py`](#fix_line_endingspy)
- [C++ source cleanup helpers](#c-source-cleanup-helpers)
  - [`reflow_cpp_logging_calls.py`](#reflow_cpp_logging_callspy)
  - [`find_cpp_dead_code_candidates.py`](#find_cpp_dead_code_candidatespy)
  - [`collapse_cpp_signatures.py`](#collapse_cpp_signaturespy)
  - [`collapse_cpp_inline_returns.py`](#collapse_cpp_inline_returnspy)
  - [`collapse_cpp_inline_statements.py`](#collapse_cpp_inline_statementspy)
- [CvMainInterface cleanup reference scripts](#cvmaininterface-cleanup-reference-scripts)
  - [`singleline_pass.py`](#singleline_passpy)
  - [`singleline_pass_comments.py`](#singleline_pass_commentspy)
  - [`singleline_pass3_comments_and_long.py`](#singleline_pass3_comments_and_longpy)
  - [`comment_cleanup_pass_v2.py`](#comment_cleanup_pass_v2py)
  - [Comparison with Base AdvCiv 1.12's Main Interface processed similarly](#comparison-with-base-advciv-112s-main-interface-processed-similarly)
  - [Notes](#notes)
    - [ChatGPT sandbox path note](#chatgpt-sandbox-path-note)
    - [Safety rule for source-rewrite helpers](#safety-rule-for-source-rewrite-helpers)
- [Game speed helper scripts](#game-speed-helper-scripts)
  - [`compare_speed_summaries.py`](#compare_speed_summariespy)
  - [`autotune_speed_from_xml.py`](#autotune_speed_from_xmlpy)
- [SASGameRecord comparison helpers](#sasgamerecord-comparison-helpers)
  - [`compare_sasgamerecord_rng.py`](#compare_sasgamerecord_rngpy)
  - [`expand_sasgamerecord_city_deltas.py`](#expand_sasgamerecord_city_deltaspy)
- [Game info comparison helpers](#game-info-comparison-helpers)
  - [`compare_handicap_infos.py`](#compare_handicap_infospy)
- [Static audit helpers](#static-audit-helpers)
  - [`audit_define_keys.py`](#audit_define_keyspy)
  - [`audit_unused_text_keys.py`](#audit_unused_text_keyspy)
- [Markdown documentation cleanup helpers](#markdown-documentation-cleanup-helpers)
  - [`reflow_markdown_prose.py`](#reflow_markdown_prosepy)
- [Legacy XML duplicate discovery scanner (``scan_xml_duplicates-3.3.py``)](#legacy-xml-duplicate-discovery-scanner-scan_xml_duplicates-33py)
- [AdvCiv manual text conversion helper](#advciv-manual-text-conversion-helper)
  - [`convert_advciv_manual_to_txt.py`](#convert_advciv_manual_to_txtpy)
- [Source packaging and generated-history helpers](#source-packaging-and-generated-history-helpers)
  - [`make_light_source_zip.py`](#make_light_source_zippy)
  - [`refresh_commit_diffs.py`](#refresh_commit_diffspy)
- [Workflow rule for timeline tuning](#workflow-rule-for-timeline-tuning)
- [General notes for future LLM helpers](#general-notes-for-future-llm-helpers)

## Inherited map-script reference sources

[`context/mapscript_refs/`](/LLM_Helpers/context/mapscript_refs/) contains downloaded, greppable, non-runtime originals used to distinguish inherited third-party map-script bugs from AdvCiv-SAS regressions. Its own README records the original package names and SHA-256 hashes, portable directory-name mappings, the source-only selection policy, and comparison guidance.

- Compare references with current `PrivateMaps` and tracked commit history; current source/Git remains authoritative.
- Do not run formatting, lint autofixes or line-ending normalization over the corpus.
- Ordinary player/GitHub Download ZIP archives and generated historical patches exclude it, while `make_light_source_zip.py` includes the current files for ChatGPT/code-agent archaeology.
- Preserve upstream credits/readmes and treat hosting/authorship/licensing metadata as upstream provenance rather than AdvCiv-SAS ownership.

## Python source cleanup helpers

### `collapse_multiline_calls.py`

Conservative source-rewrite helper for Python files.

- Collapses multiline call-like statements and simple parenthesized return expressions into one line.
- Useful for making Civ4 Python calls easier to grep/review and for reducing harmless continuation-indent noise.
- Skips statements containing comments, multiline strings, explicit backslash continuations, and large list/dict/table assignments.
- Preserves line endings and refuses to write if significant token sequence changes.
- Safer than `collapse_multiline_brackets.py` for broad cleanup because it does not collapse arbitrary bracketed dictionaries/lists/tables.
- Still review the diff before committing. The goal is searchability and reviewability, not general formatting.
- For broad passes, prefer active mod/helper source. Avoid reference/comparison folders such as `_0_Common_Docs` unless the task explicitly needs them.
- If the diff starts touching large data tables, comments, fragile indentation, or hard-to-review inherited files, stop and narrow the target set.

Single-file examples:

```powershell
python LLM_Helpers\collapse_multiline_calls.py PrivateMaps\Mirror.py
python LLM_Helpers\collapse_multiline_calls.py PrivateMaps\Mirror.py --in-place
python LLM_Helpers\collapse_multiline_calls.py PrivateMaps\Mirror.py --diff
```

Broad run workflow from PowerShell. The helper processes one file at a time, so this loop copies a temporary runner, applies it to every tracked or untracked `.py` file except the runner itself, then removes the runner.

The helper can also be run on a single file from PowerShell or Git Bash; this broad pass was tested with PowerShell for file generation, then Git Bash for staged diff output.

```powershell
cd "C:\Program Files (x86)\Steam\steamapps\common\Sid Meier's Civilization IV Beyond the Sword\Beyond the Sword\Mods\AdvCiv-SAS"; Copy-Item "LLM_Helpers\collapse_multiline_calls.py" "LLM_Helpers\collapse_multiline_calls_TEMP_RUNNER.py"; $script=(Resolve-Path "LLM_Helpers\collapse_multiline_calls_TEMP_RUNNER.py").Path; Get-ChildItem -Recurse -File -Filter *.py | Where-Object { $_.FullName -ne $script } | ForEach-Object { py $script $_.FullName --in-place }; Remove-Item $script; git diff --ignore-space-at-eol --stat; git diff --check
```

Broad staged-review workflow from Git Bash after the files were modified. This stages only `.py` files, writes a timestamped staged diff under `LLM_Helpers/outputs`, prints the staged stat, and runs the staged whitespace check:

```bash
cd "/c/Program Files (x86)/Steam/steamapps/common/Sid Meier's Civilization IV Beyond the Sword/Beyond the Sword/Mods/AdvCiv-SAS" && mkdir -p LLM_Helpers/outputs && ts=$(date +%Y%m%d_%H%M%S) && out="LLM_Helpers/outputs/staged_collapse_multiline_calls_${ts}.diff" && git ls-files -z -m -o --exclude-standard '*.py' | xargs -0 -r git add && git diff --cached --ignore-space-at-eol > "$out" && git diff --cached --ignore-space-at-eol --stat && git diff --cached --check && echo "wrote $out"
```

### `collapse_multiline_calls2.py`

Targeted second-pass source-rewrite helper.

- Separate follow-up to `collapse_multiline_calls.py`; keep the first helper as the committed known-good pass 1.
- Collapses multiline `def` headers such as `def __init__(..., ...):`.
- Collapses selected long map-generation calls such as `generatePlotsInRegion(...)`, `generateCenter(...)`, and `generateCenterPlot(...)`.
- Skips comments inside the collapsed statement, multiline strings, explicit backslash continuations, list/dict/table assignments, calls whose first argument is a list/dict literal, and long boolean return cascades beyond the conservative line cap.
- Preserves line endings and refuses to write if significant token sequence changes.
- Intended for narrow cleanup after the first collapse pass, not as a general formatter.

Tested broad run workflow from Git Bash. The helper itself processes one file at a time, but this was tested as a broad repo pass using a temporary runner.

The loop applies it to every tracked or untracked `.py` file except the runner itself, removes the runner, stages Python changes, writes a timestamped staged diff under `LLM_Helpers/outputs`, prints the staged stat, and runs the staged whitespace check:

```bash
cd "/c/Program Files (x86)/Steam/steamapps/common/Sid Meier's Civilization IV Beyond the Sword/Beyond the Sword/Mods/AdvCiv-SAS" && cp "LLM_Helpers/collapse_multiline_calls2.py" "LLM_Helpers/collapse_multiline_calls2_TEMP_RUNNER.py" && runner="LLM_Helpers/collapse_multiline_calls2_TEMP_RUNNER.py"; { git ls-files -z '*.py'; git ls-files -z -o --exclude-standard '*.py'; } | while IFS= read -r -d '' f; do [ "$f" = "$runner" ] && continue; py "$runner" "$f" --in-place; done; rm "$runner"; mkdir -p LLM_Helpers/outputs; ts=$(date +%Y%m%d_%H%M%S); out="LLM_Helpers/outputs/staged_collapse_multiline_calls2_${ts}.diff"; git ls-files -z -m -o --exclude-standard '*.py' | xargs -0 -r git add; git diff --cached --ignore-space-at-eol > "$out"; git diff --cached --ignore-space-at-eol --stat; git diff --cached --check; echo "wrote $out"
```

### `collapse_multiline_parens_safe.py`

Broad but safety-checked parenthesis-collapse helper.

- Collapses multiline parenthesized expressions to one physical line.
- Intended mainly for old Civ4 Python files where wrapped parenthesized calls/expressions create Ruff/Pylance mixed-indentation noise.
- Broader than `collapse_multiline_calls.py` because it is not limited to call-like statements.
- Safer than the legacy `collapse_multiline_brackets.py` because it defaults to parentheses only and skips risky ranges.
- Skips ranges containing comments, multiline strings, explicit backslash continuations, unmatched brackets, overly large blocks, and lines exceeding the configured maximum line length.
- Preserves strings and line endings.
- Compares significant token streams when tokenization works.
- If a file is already indentation-broken and cannot be tokenized, the helper can still apply local parenthesis-only rewrites and reports that token verification was skipped. Review those files especially carefully.
- Optional flags can include square/curly bracket ranges, but do not use those broadly unless the task explicitly needs them and the diff is easy to review.
- Always review the staged diff before committing. This is not a general formatter.

Broad Git Bash staged-review workflow. This copies a temporary runner, applies it to every tracked or untracked `.py` file except the runner itself, stages Python changes, writes the full staged diff and a check report under `LLM_Helpers/outputs`, prints the staged stat, runs `git diff --cached --check`, and reruns Ruff `E101` so the user/agent can compare the remaining mixed-indent count:

```bash
cd "/c/Program Files (x86)/Steam/steamapps/common/Sid Meier's Civilization IV Beyond the Sword/Beyond the Sword/Mods/AdvCiv-SAS" && cp "LLM_Helpers/collapse_multiline_parens_safe.py" "LLM_Helpers/collapse_multiline_parens_safe_TEMP_RUNNER.py" && runner="LLM_Helpers/collapse_multiline_parens_safe_TEMP_RUNNER.py" && mkdir -p LLM_Helpers/outputs && ts=$(date +%Y%m%d_%H%M%S) && runlog="LLM_Helpers/outputs/run_collapse_multiline_parens_safe_${ts}.txt" && diff="LLM_Helpers/outputs/staged_collapse_multiline_parens_safe_${ts}.diff" && report="LLM_Helpers/outputs/check_collapse_multiline_parens_safe_${ts}.txt" && { git ls-files -z '*.py'; git ls-files -z -o --exclude-standard '*.py'; } | while IFS= read -r -d '' f; do [ "$f" = "$runner" ] && continue; py "$runner" "$f" --in-place; done > "$runlog" 2>&1; rc=$?; rm -f "$runner"; [ $rc -eq 0 ] || { cat "$runlog"; exit $rc; }; git ls-files -z -m -o --exclude-standard '*.py' | xargs -0 -r git add --; git diff --cached --ignore-space-at-eol > "$diff"; { echo "=== collapse_multiline_parens_safe run log ==="; cat "$runlog"; echo; echo "=== git diff --cached --ignore-space-at-eol --stat -- *.py ==="; git diff --cached --ignore-space-at-eol --stat -- '*.py'; echo; echo "=== git diff --cached --check -- *.py ==="; git diff --cached --check -- '*.py'; echo; echo "=== remaining Ruff E101 ==="; py -m ruff check . --select E101 --output-format=grouped || true; echo; echo "runlog=$runlog"; echo "diff=$diff"; } > "$report" 2>&1 && cat "$report" && echo "wrote $report" && echo "wrote $diff"
```

On top of the code formatting/grepping gains, notably helped reduce ruff errors.

```md
54 files changed
816 insertions / 1875 deletions
net ~1059 physical lines removed
E101 Indentation contains mixed spaces and tabs count: ~1068 -> 259
total remaining Ruff E101 report errors: 349
```

### `fix_matrix_indent.py`

Targeted matrix/data indentation cleanup helper.

- Normalizes mixed tab/space indentation inside multiline list/dict/tuple assignment blocks.
- Intended mainly for old Civ4 map-script matrix/data blocks such as `templates = {...}`, region tables, coordinate tables, and similar static data.
- Does not collapse lines; keeps large matrices and tables multiline for readability.
- Does not touch the assignment's first line indentation, because that may be real Python block indentation.
- Converts mixed leading indentation inside the data block to spaces-only indentation.
- Uses tabsize 4 by default, matching the intended editor-style alignment for these visual matrix/data blocks better than Python's historical tabsize 8.
- Strips trailing whitespace only on lines whose leading indentation was changed.
- Preserves line endings and refuses to write if significant token sequence changes.
- Intended for Ruff `E101` cleanup in matrix/data blocks, not as a general indentation fixer.

Tested broad run workflow from Git Bash. The helper processes one file at a time, so this loop copies a temporary runner, applies it to every tracked or untracked `.py` file except the runner itself, removes the runner, stages Python changes, writes a timestamped staged diff under `LLM_Helpers/outputs`, prints the staged stat, and runs the staged whitespace check:

```bash
cd "/c/Program Files (x86)/Steam/steamapps/common/Sid Meier's Civilization IV Beyond the Sword/Beyond the Sword/Mods/AdvCiv-SAS" && cp "LLM_Helpers/fix_matrix_indent.py" "LLM_Helpers/fix_matrix_indent_TEMP_RUNNER.py" && runner="LLM_Helpers/fix_matrix_indent_TEMP_RUNNER.py"; { git ls-files -z '*.py'; git ls-files -z -o --exclude-standard '*.py'; } | while IFS= read -r -d '' f; do [ "$f" = "$runner" ] && continue; py "$runner" "$f" --in-place; done; rm "$runner"; mkdir -p LLM_Helpers/outputs; ts=$(date +%Y%m%d_%H%M%S); out="LLM_Helpers/outputs/staged_fix_matrix_indent_${ts}.diff"; git ls-files -z -m -o --exclude-standard '*.py' | xargs -0 -r git add; git diff --cached --ignore-space-at-eol > "$out"; git diff --cached --ignore-space-at-eol --stat; git diff --cached --check; echo "wrote $out"
```

In the tested broad PrivateMaps pass, this produced a high-impact Ruff cleanup: for example, `Wheel.py` dropped from roughly 400 Ruff findings to 15, while the staged diff was still mostly mechanical whitespace-only matrix indentation replacement.

```bash
LLM_Helpers/fix_matrix_indent.py     |  290 ++++++++
PrivateMaps/Archipelago.py           |  186 +++---
PrivateMaps/BTG_Lagoon.py            |  220 +++----
PrivateMaps/Balanced.py              |    4 +-
PrivateMaps/Custom_Continents.py     | 1200 +++++++++++++++++-----------------
PrivateMaps/Equal_Islands_V2_beta.py |  188 +++---
PrivateMaps/Great_Plains.py          |    4 +-
PrivateMaps/Hub.py                   |  738 ++++++++++-----------
PrivateMaps/Ice_Age.py               |    8 +-
PrivateMaps/Inland_Sea.py            |  628 +++++++++---------
PrivateMaps/Islands.py               |  188 +++---
PrivateMaps/Pangaea.py               |   24 +-
PrivateMaps/Rainforest.py            |    4 +-
PrivateMaps/Ring.py                  |  646 +++++++++---------
PrivateMaps/Tectonics.py             |   22 +-
PrivateMaps/Wheel.py                 |  786 +++++++++++-----------
16 files changed, 2713 insertions(+), 2423 deletions(-)
```

### `collapse_flat_literals.py`

Conservative flat-literal collapse helper.

- Collapses simple flat multiline Python list/tuple assignment literals to one physical line.
- Intended for small option lists, name lists, simple region-data arrays, GameFont-style flat token lists, and similar grep-friendly data.
- Does not target matrix-like map templates, nested lists/dicts/tuples, comments inside literals, or large table-style data where one row per line is clearer.
- Preserves trailing commas if they already exist; this keeps the rewrite mechanical and avoids unnecessary token/style changes.
- Refuses risky edits when significant token sequence changes.
- Default `--max-line-len` is intentionally fairly high because this helper targets grep-friendly flat data arrays, not normal prose/code wrapping.
- This is not a general formatter. Review the staged diff before committing.

Tested broad run workflow from Git Bash. The helper processes one file at a time, so this loop copies a temporary runner, applies it to every tracked or untracked `.py` file except the runner itself, removes the runner, stages Python changes, writes a timestamped staged diff under `LLM_Helpers/outputs`, prints the staged stat, and runs the staged whitespace check:

```bash
cd "/c/Program Files (x86)/Steam/steamapps/common/Sid Meier's Civilization IV Beyond the Sword/Beyond the Sword/Mods/AdvCiv-SAS" && cp "LLM_Helpers/collapse_flat_literals.py" "LLM_Helpers/collapse_flat_literals_TEMP_RUNNER.py" && runner="LLM_Helpers/collapse_flat_literals_TEMP_RUNNER.py" && mkdir -p LLM_Helpers/outputs && ts=$(date +%Y%m%d_%H%M%S) && runlog="LLM_Helpers/outputs/run_collapse_flat_literals_${ts}.txt" && diff="LLM_Helpers/outputs/staged_collapse_flat_literals_${ts}.diff" && report="LLM_Helpers/outputs/check_collapse_flat_literals_${ts}.txt" && { git ls-files -z '*.py'; git ls-files -z -o --exclude-standard '*.py'; } | while IFS= read -r -d '' f; do [ "$f" = "$runner" ] && continue; py "$runner" "$f" --in-place; done > "$runlog" 2>&1; rc=$?; rm -f "$runner"; [ $rc -eq 0 ] || { cat "$runlog"; exit $rc; }; git ls-files -z -m -o --exclude-standard '*.py' | xargs -0 -r git add --; git diff --cached --ignore-space-at-eol > "$diff"; { echo "=== collapse_flat_literals run log ==="; cat "$runlog"; echo; echo "=== staged stat ==="; git diff --cached --ignore-space-at-eol --stat -- '*.py'; echo; echo "=== staged whitespace check ==="; git diff --cached --check -- '*.py'; echo; echo "runlog=$runlog"; echo "diff=$diff"; } > "$report" 2>&1 && cat "$report" && echo "wrote $report" && echo "wrote $diff"
```

Tested broad pass result with default line length 500:

```text
19 flat literals collapsed across 15 existing source files.
One risky old vendored file, Assets\Python\BUG\configobj.py, was refused by the token-safety check.
The pass caught the longer flat GameFontDisplay character-name list in addition to shorter option/name/region-data arrays.
Run git diff --cached --check before committing.
```

Example intended rewrites:

```python
selection_names = [
	"TXT_KEY_MAP_WRAP_FLAT",
	"TXT_KEY_MAP_WRAP_CYLINDER",
	"TXT_KEY_MAP_WRAP_TOROID",
]
```

becomes:

```python
selection_names = ["TXT_KEY_MAP_WRAP_FLAT", "TXT_KEY_MAP_WRAP_CYLINDER", "TXT_KEY_MAP_WRAP_TOROID",]
```

```python
phonetic_array = [
	'Alpha', 'Bravo', 'Charlie',
	'Delta', 'Echo', 'Foxtrot',
]
```

becomes:

```python
phonetic_array = ['Alpha', 'Bravo', 'Charlie', 'Delta', 'Echo', 'Foxtrot',]
```

### `wrap_python2_prints_for_linting.py`

Conservative Python 2 print-wrapper helper.

- Converts simple active Python 2 bare prints such as `print "hello"` and `print "value = %s" % value` to Python 2.4/3-compatible single-expression calls such as `print("hello")` and `print("value = %s" % value)`.
- Skips ambiguous cases such as redirected prints, trailing-comma prints, semicolon-packed lines, and top-level comma prints; manually rewrite those with one formatted string such as `print("player %s pass %s failed" % (playerID, iPass))`.
- Does not touch commented-out prints, because comments do not block Ruff/Python 3 parsing.
- Intended to reduce parser/linter noise while preserving Civ4 Python 2.4 runtime compatibility.

Example:

```powershell
python LLM_Helpers\wrap_python2_prints_for_linting.py --recursive --diff Assets\Python PrivateMaps
python LLM_Helpers\wrap_python2_prints_for_linting.py --recursive --in-place Assets\Python PrivateMaps
python LLM_Helpers\wrap_python2_prints_for_linting.py --recursive --check Assets\Python PrivateMaps
```

### `collapse_multiline_brackets.py`

Legacy/reference helper.

- Collapses multi-line bracketed expressions (`(...)`, `[...]`, `{...}`) in a single Python source file to one-liners.
- Useful idea: make boilerplate Civ4 API calls and helper invocations greppable on one line.
- Preserves line endings, string literals, and comments outside collapsed ranges.
- Important caution: this script strips line comments and indentation inside collapsed ranges.
- Because many AdvCiv-SAS UI files contain important inline/manual-layout comments, do not use this blindly on comment-heavy files.
- Run on ONE file at a time and eyeball the diff before committing. A whole-directory sweep was tried once and reverted.

Example:

```powershell
python LLM_Helpers\collapse_multiline_brackets.py Assets\Python\Contrib\Sevopedia\_sevopedia_helpers.py
```

### `fix_line_endings.py`

External cleanup helper for line-ending hygiene.

- Reports or fixes mixed CRLF/LF line endings and missing final newlines.
- Does not run automatically in GitHub Actions; the workflow check only reports/fails.
- Defaults to active repo text-like files and excludes generated/reference helper folders such as `LLM_Helpers/outputs`, `LLM_Helpers/examples` and the verbatim `LLM_Helpers/context/mapscript_refs` corpus.
- Preserves each file's dominant existing line-ending style by default, so it does not force the whole repo to LF or CRLF.
- Adds a final newline to non-empty text files by default; use `--no-final-newline` only when deliberately preserving a missing final newline.
- Use `--eol lf` or `--eol crlf` only for deliberate normalization passes.
- Always review the diff before committing generated source changes.

Examples:

```powershell
python LLM_Helpers\fix_line_endings.py
python LLM_Helpers\fix_line_endings.py --diff
python LLM_Helpers\fix_line_endings.py --in-place
python LLM_Helpers\fix_line_endings.py Assets\Python PrivateMaps --diff
python LLM_Helpers\fix_line_endings.py Assets\Python PrivateMaps --in-place
```

## C++ source cleanup helpers

### `reflow_cpp_logging_calls.py`

Conservative readability formatter for long C++ BBAI/SASGameRecord logging calls.

- Targets only active `logBBAI(...)` and `logSASGameRecord(...)` calls; commented-out code is ignored.
- Keeps the diagnostic format string byte-for-byte intact and wraps only at top-level commas between C++ arguments.
- Does not reflow or merge existing prose comments; comment layout remains author-maintained, and calls containing comments are skipped for manual review.
- Skips preprocessor directives and continued macro bodies entirely. Physical `\` line-splicing is semantic before C++ tokenization, so macro logging calls require manual formatting.
- By default, a one-line call is considered for wrapping above 180 columns; an already-multiline call is considered only when one of its argument/continuation lines exceeds that threshold, so a deliberately long format-string line alone does not trigger churn.
- Uses a 140-column target for continuation lines while keeping individual nested expressions intact. Complex arguments that are themselves multiline or contain comments are skipped for manual review rather than reformatted speculatively.
- Preserves LF/CRLF line endings and compares non-comment C++ token streams before writing, refusing the change if significant source tokens differ.
- This intentionally complements rather than conflicts with the signature/single-line cleanup helpers: diagnostic argument lists are easier to inspect, crash-triage, and review when they are not packed into several-hundred-character physical lines.
- Always review the diff before committing. This is a narrow logging formatter, not a general C++ formatter.

Dry-run scan of the DLL source:

```bash
python LLM_Helpers/reflow_cpp_logging_calls.py CvGameCoreDLL
```

Review a single file as a unified diff:

```bash
python LLM_Helpers/reflow_cpp_logging_calls.py CvGameCoreDLL/CvUnitAI.cpp --diff
```

Apply across the DLL source:

```bash
python LLM_Helpers/reflow_cpp_logging_calls.py CvGameCoreDLL --in-place
```

### `find_cpp_dead_code_candidates.py`

Conservative C++ dead-code candidate finder for LLM/manual review.

- Does not edit files and does not prove code is dead.
- Scans the DLL source for likely review targets such as disabled macro/test infrastructure, existing `#if 0` blocks, and low-reference qualified functions.
- Use `--focus high` for the least noisy report; it skips low-reference functions and focuses on macro/test candidates and existing disabled blocks.
- Use `--include-risky-low-reference` only for deeper manual review because Civ4 DLL code can be reached through EXE callbacks, Python/Cy exports, virtual dispatch, serialization, macros, and debug-only paths.
- Writes timestamped Markdown reports under `LLM_Helpers/outputs` by default.
- First practical use: identified `ReproTest.cpp` as disabled reproducibility-test infrastructure that was still compiled in normal builds, leading to the `ENABLE_REPRO_TEST` compile-gate cleanup.
- Always review the report with an LLM/human before changing source code; prefer compile-gating disabled test/debug infrastructure over deleting inherited code.

Default report:

```powershell
python ./LLM_Helpers/find_cpp_dead_code_candidates.py
```

High-signal report:

```powershell
python ./LLM_Helpers/find_cpp_dead_code_candidates.py --focus high
```

Optional explicit output:

```powershell
python ./LLM_Helpers/find_cpp_dead_code_candidates.py --output LLM_Helpers/outputs/cpp_dead_code_candidates_manual.md
```

### `collapse_cpp_signatures.py`

Conservative signature-only source-rewrite helper for C/C++ files.

- Collapses safe multiline function declarations/definitions to one physical line, mainly to make signatures easier to scan, grep, and compare.
- Signature-only by design: ordinary multiline calls, logging statements, and constructor-like local statements are left alone. If call cleanup is wanted later, use a separate helper such as `collapse_cpp_calls.py` with separate rules.
- Skips control-flow statements, comments that would become misleading, block-comment boundaries, and candidates that do not look like function headers.
- Unindented root-scope declarations in `.cpp` files are treated like safe declarations; nested constructor-like local statements are omitted from the normal ignored report unless `--include-nonsignature-ignored` is used.
- Qualified `::` signatures are also detected when the prefix contains template commas such as `std::pair<int,int>` or `KmodPathFinder<StepMetric,Node>`.
- Indented header declarations may use qualified return types such as `std::pair<int,int>`; only indented qualified callable names are treated as likely local calls.
- Nested class/struct declarations are handled, while declaration-shaped local constructor calls inside inline function bodies stay skipped.
- Allows trailing end-of-signature `//` comments (for example `) const // advc.031`) and inline one-line `/* ... */` tail comments; comments inside the parameter list are skipped unless a trace-hoisting mode below handles them.
- Uses a generous default `--max-line-len 600` and `--max-span-lines 16` because this helper is meant to make safe signatures one-line-first; lower them for a narrower review.
- `--hoist-comments` can move whole-line comments from inside a collapsed signature above the signature, while still skipping inline comments inside the parameter list.
- `--trace-hoisted-comments` also appends a custom trace note to hoisted comments, recording whether the comment was before, after, or between specific parameters. Use `--trace-credit "GPT-5.5 (reviewed script output)"` or similar when the generated comments were externally reviewed.
- `--trace-inline-comments` similarly hoists inline `//` comments and extractable multiline `/* ... */` comments from inside the parameter list, strips them from the collapsed signature line, and appends trace metadata for their original parameter position.
- Existing whole-line `<!-- custom: ... -->` comments inside signatures receive the same hoist-position trace note as other whole-line comments.
- `--tail-exposed-to-python-comments` handles the common header-only case where `// Exposed to Python` was placed inside a multiline declaration; it moves that metadata to the final tail comment, after any existing tail comment.
- Defaults to tracked C/C++ files under `CvGameCoreDLL`; you can also pass one or more files/folders for a narrower review.
- `--diff-file` writes the review diff to a file. Without an explicit path, it creates a timestamped file under `LLM_Helpers/outputs/`.
- `--ignored-file` writes a separate review report of skipped signature-like multiline candidates and the reason they were left alone, such as unsafe comments, line length, or unsupported tail syntax.
  - Use `--include-nonsignature-ignored` only when you also want the very noisy/exhaustive ordinary-call/local-statement candidates.
- Empirically idempotent after the first full DLL pass: rerunning with the default broad scan reported `Scanned 350 C/C++ file(s). No safe C++ signature collapses found.`
- Always review the diff before committing. This is not a general C++ formatter. External review in ChatGPT or another non-agentic context can be useful for long generated diff/report files before applying the changes locally.

Example diagnostic run with output files and no source changes (Git Bash):

```Bash
cd "C:\Program Files (x86)\Steam\steamapps\common\Sid Meier's Civilization IV Beyond the Sword\Beyond the Sword\Mods\AdvCiv-SAS" && python ./LLM_Helpers/collapse_cpp_signatures.py ./CvGameCoreDLL --diff-file --ignored-file
```

This writes timestamped files like `LLM_Helpers/outputs/collapse_cpp_signatures_20260619T150412Z.diff.txt` and `LLM_Helpers/outputs/collapse_cpp_signatures_20260619T150412Z_ignored.txt`.

Example apply run (Git Bash):

```bash
cd "C:\Program Files (x86)\Steam\steamapps\common\Sid Meier's Civilization IV Beyond the Sword\Beyond the Sword\Mods\AdvCiv-SAS" && python ./LLM_Helpers/collapse_cpp_signatures.py ./CvGameCoreDLL --in-place
```

Example apply run with traceable hoisted comments (Git Bash):

```bash
cd "C:\Program Files (x86)\Steam\steamapps\common\Sid Meier's Civilization IV Beyond the Sword\Beyond the Sword\Mods\AdvCiv-SAS" && python ./LLM_Helpers/collapse_cpp_signatures.py ./CvGameCoreDLL --max-line-len 600 --max-span-lines 16 --trace-hoisted-comments --trace-credit "GPT-5.5 (reviewed script output)" --in-place
```

Example apply run with traceable whole-line and inline hoisted comments (Git Bash):

```bash
cd "C:\Program Files (x86)\Steam\steamapps\common\Sid Meier's Civilization IV Beyond the Sword\Beyond the Sword\Mods\AdvCiv-SAS" && python ./LLM_Helpers/collapse_cpp_signatures.py ./CvGameCoreDLL --max-line-len 600 --max-span-lines 16 --trace-inline-comments --trace-hoisted-comments --tail-exposed-to-python-comments --trace-credit "GPT-5.5 (reviewed script output)" --in-place
```

Example apply run for `// Exposed to Python` declaration metadata (Git Bash):

```bash
cd "C:\Program Files (x86)\Steam\steamapps\common\Sid Meier's Civilization IV Beyond the Sword\Beyond the Sword\Mods\AdvCiv-SAS" && python ./LLM_Helpers/collapse_cpp_signatures.py ./CvGameCoreDLL --max-line-len 600 --max-span-lines 16 --tail-exposed-to-python-comments --in-place
```

Example results:

- First full pass: 118 files changed, 1360 insertions, and 2662 deletions.
- A follow-up comment-hoisting pass, amended into the same commit because it seemed small and contained, collapsed 31 more signatures across 23 files.
- A later pass improved old comment text preservation and appended trace notes for where each hoisted comment had originally been placed: it was 66 insertions and 117 deletions.
- Another `// Exposed to Python` metadata pass collapsed 102 more header declarations across 13 files.
- A broad inline-comment trace pass collapsed 125 more signatures across 37 files.
- A multiline block-comment trace pass collapsed 21 more signatures across 13 files.
- A root-scope `.cpp` declaration pass collapsed 6 explicit-template-instantiation signatures in `CvGameTextMgr.cpp`.
- An existing-custom-comment pass collapsed 2 more `setUnitHelp` signatures while preserving their custom rationale comments and adding hoist-position trace notes.
- A qualified-template-prefix pass collapsed 8 more `::` signatures with template commas in the prefix.
- A qualified-return-type header pass collapsed 4 more declarations using `std::` return types.
- A nested-class declaration pass collapsed 2 more `InvasionGraph::Node` declarations while keeping inline-function local constructor calls skipped.

Example (nested simple comments) (before):

```cpp
void CvDeal::endTrade(TradeData trade, PlayerTypes eFromPlayer,
	PlayerTypes eToPlayer, bool bTeam, /* advc.036: */ bool bUpdateAttitude,
	PlayerTypes eCancelPlayer) // advc.130p
```

Example (nested simple comments) (after):

```cpp
void CvDeal::endTrade(TradeData trade, PlayerTypes eFromPlayer, PlayerTypes eToPlayer, bool bTeam, /* advc.036: */ bool bUpdateAttitude, PlayerTypes eCancelPlayer) // advc.130p
```

Example 2 (overloaded signatures) (before) :

```cpp
	/*	(Had been named "addHumanMessage" in K-Mod;
		Definition moved into CvDLLInterfaceIFaceBase.cpp.) */ // </advc.127>
	void addMessage(PlayerTypes ePlayer, bool bForce, int iLength, CvWString szString,
			LPCTSTR pszSound = NULL, InterfaceMessageTypes eType = MESSAGE_TYPE_INFO,
			LPCSTR pszIcon = NULL, ColorTypes eFlashColor = NO_COLOR,
			int iFlashX = -1, int iFlashY = -1,
			bool bShowOffScreenArrows = false, bool bShowOnScreenArrows = false);
	// advc: Wrapper for passing iFlashX, iFlashY more conveniently
	void addMessage(PlayerTypes ePlayer, bool bForce, int iLength,
			CvWString szString, CvPlot const& kPlot,
			LPCTSTR pszSound = NULL, InterfaceMessageTypes eType = MESSAGE_TYPE_INFO,
			LPCSTR pszIcon = NULL, ColorTypes eFlashColor = NO_COLOR,
			bool bShowOffScreenArrows = true, bool bShowOnScreenArrows = true);
```

Example 2 (overloaded signatures) (after):

```cpp
	/*	(Had been named "addHumanMessage" in K-Mod;
		Definition moved into CvDLLInterfaceIFaceBase.cpp.) */ // </advc.127>
	void addMessage(PlayerTypes ePlayer, bool bForce, int iLength, CvWString szString, LPCTSTR pszSound = NULL, InterfaceMessageTypes eType = MESSAGE_TYPE_INFO, LPCSTR pszIcon = NULL, ColorTypes eFlashColor = NO_COLOR, int iFlashX = -1, int iFlashY = -1, bool bShowOffScreenArrows = false, bool bShowOnScreenArrows = false);
	// advc: Wrapper for passing iFlashX, iFlashY more conveniently
	void addMessage(PlayerTypes ePlayer, bool bForce, int iLength, CvWString szString, CvPlot const& kPlot, LPCTSTR pszSound = NULL, InterfaceMessageTypes eType = MESSAGE_TYPE_INFO, LPCSTR pszIcon = NULL, ColorTypes eFlashColor = NO_COLOR, bool bShowOffScreenArrows = true, bool bShowOnScreenArrows = true);
```

Example 3 (traceable hoisted comment) (before):

```cpp
bool CvUnitAI::AI_cityAttack(int iRange, int iOddsThreshold,
	// advc (comment): No caller uses eFlags anymore (not since K-Mod 1.15)
	MovementFlags eFlags, bool bFollow)
```

Example 3 (traceable hoisted comment) (after):

```cpp
// advc (comment): No caller uses eFlags anymore (not since K-Mod 1.15) <!-- custom: hoisted from multiline signature between `iOddsThreshold` and `eFlags` by collapse_cpp_signatures.py. (GPT-5.5 (reviewed script output)) -->
bool CvUnitAI::AI_cityAttack(int iRange, int iOddsThreshold, MovementFlags eFlags, bool bFollow)
```

### `collapse_cpp_inline_returns.py`

Conservative header-only helper for simple inline C++ functions.

- Collapses inline functions whose body is exactly one `return ...;` statement to one physical line, including wrapped return expressions and signatures that already have `{` on the signature line.
- Joins a one-line `template<...>` prefix onto the collapsed inline function when the combined line stays below `--max-line-len`.
- Signature tail comments such as `// Exposed to Python` are preserved after the collapsed body.
- Opening-brace comments such as `{	// advc: ...` are hoisted above the collapsed function; closing-brace tail comments such as `} // </advc.opt>` are preserved after the collapsed body.
- Leading `// ...` and `/* ... */` body comments are hoisted above the collapsed function; multiline block comments are normalized to one line.
- Return-line tail comments such as `return x; // advc.opt` are hoisted above the collapsed function so the code line stays focused on the greppable function.
- Skips constructors/destructors, multi-statement bodies, mid-body comments after the return statement, multiline signatures, and lines exceeding `--max-line-len`.
- Separate from `collapse_cpp_signatures.py` because it rewrites function bodies, not just signatures.
- Always review the diff before committing; this is a grep/readability cleanup, not a formatter.

Example diagnostic run with output file and no source changes:

```bash
cd "C:\Program Files (x86)\Steam\steamapps\common\Sid Meier's Civilization IV Beyond the Sword\Beyond the Sword\Mods\AdvCiv-SAS" && python ./LLM_Helpers/collapse_cpp_inline_returns.py ./CvGameCoreDLL --diff-file
```

Example apply run:

```bash
cd "C:\Program Files (x86)\Steam\steamapps\common\Sid Meier's Civilization IV Beyond the Sword\Beyond the Sword\Mods\AdvCiv-SAS" && python ./LLM_Helpers/collapse_cpp_inline_returns.py ./CvGameCoreDLL --in-place
```

Reviewed passes used this to collapse:

- simple inline return helpers such as `int getGoldPerTurnByPlayer(PlayerTypes ePlayer) const { return m_aiGoldPerTurnByPlayer.get(ePlayer); } // Exposed to Python`
- wrapped-return helpers such as `template<bool bCHECK_HAS_MET> int countFreeRivals() const { return PlayerIter<FREE_MAJOR_CIV, bCHECK_HAS_MET ? KNOWN_POTENTIAL_ENEMY_OF : POTENTIAL_ENEMY_OF>::count(m_kAgentTeam.getID()); }`
- and brace-on-signature wrappers such as `CvCityAI* AI_getCapital() const { return AI_getCity(m_iCapitalCityID); }`.
- comment-bearing wrappers such as `bool AI_isAnyWaterDanger(CvPlot const& kPlot, int iRange = DANGER_RANGE) const { return (AI_getWaterDanger(kPlot, iRange, 1) >= 1); } // </advc.opt>`.

Example (before):

```cpp
	int getGoldPerTurnByPlayer(PlayerTypes ePlayer) const															// Exposed to Python
	{
		return m_aiGoldPerTurnByPlayer.get(ePlayer);
	}
```

Example (after):

```cpp
	int getGoldPerTurnByPlayer(PlayerTypes ePlayer) const { return m_aiGoldPerTurnByPlayer.get(ePlayer); } // Exposed to Python
```

### `collapse_cpp_inline_statements.py`

Conservative header-only helper for simple inline C++ functions whose body is exactly one non-return statement.

- Complements `collapse_cpp_inline_returns.py`; use this for simple setters, wrapper calls, assignment helpers, `BOOST_STATIC_ASSERT(false);` stubs, and similar one-statement bodies.
- Hoists whole-line body comments and statement tail comments above the collapsed function, and joins one-line `template<...>` prefixes when safe.
- Skips constructors/destructors, initializer-list bodies, `return` bodies, control-flow bodies, delete/throw/SAFE_DELETE bodies, block comments, multi-statement bodies, multiline signatures, and lines exceeding `--max-line-len`.
- Always review the diff before committing; this is a grep/readability cleanup, not a formatter.

Example diagnostic run with output file and no source changes:

```bash
cd "C:\Program Files (x86)\Steam\steamapps\common\Sid Meier's Civilization IV Beyond the Sword\Beyond the Sword\Mods\AdvCiv-SAS" && python ./LLM_Helpers/collapse_cpp_inline_statements.py ./CvGameCoreDLL --diff-file
```

Example apply run:

```bash
cd "C:\Program Files (x86)\Steam\steamapps\common\Sid Meier's Civilization IV Beyond the Sword\Beyond the Sword\Mods\AdvCiv-SAS" && python ./LLM_Helpers/collapse_cpp_inline_statements.py ./CvGameCoreDLL --in-place
```

Example (before):

```cpp
	template<>
	void applyOp<OP_MULT>(CompactV& cvValue, CompactV cvMultiplier)
	{
		cvValue = static_cast<CompactV>(cvValue * cvMultiplier);
	}
```

Example (after):

```cpp
	template<> void applyOp<OP_MULT>(CompactV& cvValue, CompactV cvMultiplier) { cvValue = static_cast<CompactV>(cvValue * cvMultiplier); }
```

Done with the very nice help of GPT-5.5 (on Codex) thanks.

## CvMainInterface cleanup reference scripts

These were generated during a GPT-5.5-Thinking cleanup of `CvMainInterface.py` to make the file easier for grep, VS Code review, and future LLM agents.

The generated scripts were staged working scripts, not polished repo utilities:

### `singleline_pass.py`

- First conservative pass.
- Collapsed uncommented multiline logical statements.
- Skipped statements containing comments.

### `singleline_pass_comments.py`

- Second staged pass.
- Collapsed more multiline statements.
- Preserved comments by moving them above the collapsed statement instead of deleting them.

### `singleline_pass3_comments_and_long.py`

- Third staged pass and best reference for the final single-line statement approach.
- Allowed more long-line/comment-aware collapses.
- Also handled some commented-out multiline code groups.
- This was run after earlier passes, on an already-modified file.

### `comment_cleanup_pass_v2.py`

- Fourth staged/reference pass, focused on comments rather than statements.
- Condensed simple wrapped prose comments.
- Fixed minor comment typos and wording issues.
- Intentionally left structured notes, custom XML-style comments, and commented-out code mostly alone.
- This is separate from the single-line statement passes and should be reviewed as a comment-only cleanup.

### Comparison with Base AdvCiv 1.12's Main Interface processed similarly

For comparison purposes, we also applied the same scripts to Base AdvCiv 1.12's main interface (as of 2026-06-10), with the help of ChatGPT-5.5.

The resulting file is stored in [CvMainInterface_1_12_singleline.py](/LLM_Helpers/examples/CvMainInterface_1_12_singleline.py). Before as of now adding our copyright a few heading comments, the diff was: Line count: 7279 -> 6021, so 1258 physical lines removed.

We store an example of this as of now because this file has significantly deviated due to formatting change, because it is easier to read for agents or perhaps users too, because the file is large, and because some user seemingly mentioned the comparison is harder.

### Notes

Important: `singleline_pass3_comments_and_long.py` should not be treated as guaranteed to include all behavior from pass 1 and pass 2 when run directly on a raw original file. It is the best reference implementation from the final single-line statement pass, not a clean all-in-one tool.

Likewise, `comment_cleanup_pass_v2.py` is a later comment-cleanup reference pass, not part of the statement-collapsing logic.

Future LLM/code agents should either:

- reuse the staged scripts only in the same order and review each diff, or
- preferably consolidate the useful logic into one or two clean repo-ready utilities, for example:

```text
collapse_multiline_python.py
condense_python_comments.py
```

A cleaned source-rewrite utility should support:

- command-line input path instead of hardcoded sandbox paths;
- optional `--in-place`;
- optional `--output`;
- optional `--move-comments`;
- optional `--collapse-commented-code` for statement tools;
- optional typo/comment-normalization rules for comment tools;
- line-ending preservation;
- string literal preservation;
- comment preservation;
- summary stats;
- dry-run or diff output if practical.

#### ChatGPT sandbox path note

Some generated scripts may contain paths such as:

```text
/mnt/data/CvMainInterface.singleline2.py
/mnt/data/CvMainInterface.singleline3.py
```

Those were ChatGPT sandbox paths, not repo paths. Codex, Claude Code, local PowerShell, or other agents should replace them with actual repository paths such as:

```text
Assets\Python\Screens\CvMainInterface.py
```

or accept a path from the command line.

#### Safety rule for source-rewrite helpers

For helpers that rewrite Python source files, especially files loaded by Civ4:

- preserve meaningful code tokens;
- preserve strings exactly;
- preserve comments or move them deliberately;
- never silently delete explanatory comments;
- do not rely on Python 3 `ast.parse` as proof of Civ4 Python 2.4 compatibility;
- review the git diff manually;
- smoke-test in game when the file is UI/runtime-sensitive.

The goal is not general Python formatting. The goal is to make large, LLM-maintained files easier to search, patch, and review.

Good target style for LLM-facing UI files:

```text
one semantic UI operation = one searchable line
manual layout decision = keep local and visible
large table/list = one item per line when that is clearer
commented-out code = preserve intent; do not auto-destroy it
```

## Game speed helper scripts

### `compare_speed_summaries.py`

- Reads the latest Sevopedia Game Speed chart dump from `PythonDbg.log`.
- Compares `Normal` vs one selected speed across summary rows.
- Also compares flattened increment sequences (Years and Months) index by index.
- Can focus summaries to one index (`--summary`), while still printing increment comparison.
- Automatically writes a timestamped `.txt` file.
- Output filename is short and sortable: `<UTC-ISO>_<speed>[_sXX].txt` (e.g. `20260217T110501Z_slow_s06.txt`).

Examples:

```powershell
python LLM_Helpers\compare_speed_summaries.py --speed slow
python LLM_Helpers\compare_speed_summaries.py --speed marathon --summary 6
python LLM_Helpers\compare_speed_summaries.py --log "C:\Users\PC\Documents\My Games\beyond the sword\Logs\PythonDbg.log" --speed veryslow
```

Auto-save with timestamp under `LLM_Helpers\outputs` (git-ignored):

```powershell
python LLM_Helpers\compare_speed_summaries.py --speed slow
python LLM_Helpers\compare_speed_summaries.py --speed slow --summary 6
python LLM_Helpers\compare_speed_summaries.py --speed slow --output-dir LLM_Helpers\outputs
```

### `autotune_speed_from_xml.py`

- Computes Summary rows directly from `Assets/XML/GameInfo/CIV4GameSpeedInfo.xml`.
- Does not require `PythonDbg.log`.
- Can print XML-based `Normal` vs selected speed comparison.
- Optional draft autoloop mode for `GAMESPEED_SLOW`.
- Iterates candidate timeline tweaks and reports the best-scoring result.
- Writes timestamp-first output files in `LLM_Helpers\outputs` (`<UTC-ISO>_<speed>_<mode>.txt`).

XML-native comparison and draft autoloop:

```powershell
python LLM_Helpers\autotune_speed_from_xml.py --speed slow
python LLM_Helpers\autotune_speed_from_xml.py --speed slow --summary-steps 50
python LLM_Helpers\autotune_speed_from_xml.py --speed slow --summary-steps 100 --focus-start-pct 20 --focus-end-pct 80
python LLM_Helpers\autotune_speed_from_xml.py --speed slow --autoloop --iterations 8000 --seed 1
```

## SASGameRecord comparison helpers

### `compare_sasgamerecord_rng.py`

- Reads two `SASGameRecord` `.log` files or ZIP files containing exactly one `.log` each.
- Validates every authoritative-RNG checkpoint's interval/session counter arithmetic, interval-state continuity, and Civ4 LCG progression whenever the interval contains no explicit seed replacement.
  - At record revision 71+, it also validates paired semantic CORE-state checkpoint identities, component/combined fingerprints, object counts and computation timings.
- Reports the first differing lifecycle checkpoint and its differing map/synchronized RNG or semantic-state fields, then distinguishes changed RNG consumption, changed call provenance and deterministic/non-authoritative-RNG state divergence.
  - A lone trailing `SESSION_FINALIZE` is identified as a harmless session-lifecycle difference when every comparable checkpoint matches.
- Emits recipe-sensitive combined RNG/state ordering interpretations only when both records expose the same explicit `recordRevision`. Missing or differing revision metadata leaves raw checkpoint validation/comparison available but suppresses conclusions that assume identical state-fingerprint recipes.
- Exit status is 0 when all comparable checkpoints match (including a lone trailing `SESSION_FINALIZE`), 1 for a valid gameplay/checkpoint divergence, and 2 for invalid input or failed internal invariants.
- Does not modify either record and adds no game/runtime overhead.
- [`examples/sasgamerecord_rng_compared.txt`](/LLM_Helpers/examples/sasgamerecord_rng_compared.txt) shows two valid repeated-load runs whose RNG and semantic state match through every comparable checkpoint, including measured fingerprint cost.
  - Refresh it with `--example-output`; use `--output <path>` for another retained report.

Interpretation:

- `valid` means each individual record passed interval/session counter arithmetic, state continuity, and every applicable independent LCG reconstruction before comparison.
- Different call counts mean one run consumed additional or fewer authoritative RNG advances in that interval.
- Different state or stream fingerprints mean authoritative random consumption diverged, even if the call totals happen to match.
- A call-fingerprint-only difference means the random stream still matches but its labels, data arguments, NULL-message classification or EXE-wrapper origin changed.
- A lifecycle/checkpoint mismatch usually means the records represent different boundaries or were paired from unlike run sequences.
- A semantic-state difference while authoritative RNG still matches localizes a deterministic or untracked-state divergence; the component hashes narrow it to game, team, player, city, unit, selection-group, plot or deal state.

Examples:

```powershell
python LLM_Helpers\compare_sasgamerecord_rng.py "C:\path\run_a.log" "C:\path\run_b.log"
python LLM_Helpers\compare_sasgamerecord_rng.py "C:\path\run_a.zip" "C:\path\run_b.zip"
python LLM_Helpers\compare_sasgamerecord_rng.py "C:\path\run_a.log" "C:\path\run_b.log" --example-output
```

### `expand_sasgamerecord_city_deltas.py`

- Expands revision-123+ bounded level-3 `GAME_RECORD_CITY*_DELTA` rows back into the ordinary full city-detail rows understood by older/simple consumers.
- Supports the five intentionally delta-eligible families: core city, development, happiness, health and buildings. `GAME_RECORD_CITY_TRADE_PARTNERS` remains full in the source log because delta metadata would make that one-field payload larger.
- Validates `previousTurn`, `fullBaseTurn`, `changed` and the required preceding full/delta chain instead of silently inventing missing inherited fields. A truncated excerpt that begins inside a delta chain therefore fails visibly until its preceding full checkpoint is included.
- Preserves unrelated rows and reconstructed serialized field tokens byte-for-byte, including quoted/escaped values; only the delta representation itself is replaced by its reconstructed ordinary full row.
- Reads/writes the record byte-preservingly with the Civ4-compatible single-byte mapping used by the helper, so it is suitable for lossless round-trip/reference validation rather than semantic reserialization.
- Does not modify the input record and has no game/runtime overhead.

Usage:

```bat
python LLM_Helpers\expand_sasgamerecord_city_deltas.py SASGameRecord.log SASGameRecord_expanded.log
```

## Game info comparison helpers

### `compare_handicap_infos.py`

- Report-only. Does not modify source files.
- Compares one `CIV4HandicapInfo.xml` against another; pass both XML paths explicitly.
- Compares handicap entries in target/file 2 XML order.
  - Exact `<Type>` matches are used when available.
  - Target-only entries are compared to the nearest shared boundary dynamically, so a new lowest difficulty is compared to the base file's lowest shared difficulty and a new highest difficulty is compared to the base file's highest shared difficulty without hardcoding difficulty names.
  - Left-only entries are listed at the end.
- Missing/new fields on either side are shown with the missing side blank.
- Adds a separate entry table with each compared row's left/right `Type`, `Description`, and match note, then lists changed fields by compared row.
- Flattens handicap XML by field path and writes an LLM-friendly Markdown table with flat numeric deltas and percentage deltas, computed as `(file 2 - file 1) / file 1` when both values are numeric and file 1 is nonzero.
- Repeated XML collections such as `Goodies`, `FreeTechs`, and `AIFreeTechs` are compared as compact counted lists instead of noisy index-by-index rows.
- Writes timestamped output to `LLM_Helpers\outputs\handicap_compare_<UTC-ISO>.md` by default; this folder is git-ignored.
- `--example-output` writes to `LLM_Helpers\examples\handicap_infos_compared.md` instead, useful when publishing a stable hosted example URL.
- CI reproduces the published example using the pinned [`handicap_infos_baseline.xml`](/LLM_Helpers/examples/handicap_infos_baseline.xml); use `python .github/workflows/build/generated_docs.py --refresh-handicap` after changing current handicap XML. This needs no sibling AdvCiv installation.
- The report includes its UTC run time, output path, and full input paths because XML assets can change between analysis runs.
- The same Markdown file includes a tab-separated spreadsheet matrix: one row per field, and grouped file 1/file 2/delta columns for each compared row.
  - For changed-field reports, unchanged cells are still filled when they belong to a shown field, so a row such as `iFreeUnits` shows the full handicap curve.
  - Empty cells mean no matching value on that side.
- Optional `--file1-label`/`--file2-label` labels make published examples clearer while the defaults stay generic.
- `--tsv-output` optionally writes the same matrix as a separate `.tsv` file too.
- Created with GPT-5.5/Codex and reviewed with GPT-5.5-Thinking.

Examples of use (PowerShell):

```powershell
Set-Location -LiteralPath "C:\Program Files (x86)\Steam\steamapps\common\Sid Meier's Civilization IV Beyond the Sword\Beyond the Sword\Mods\AdvCiv-SAS"; $ts=(Get-Date).ToUniversalTime().ToString("yyyyMMdd'T'HHmmss'Z'"); New-Item -ItemType Directory -Force -Path ".\LLM_Helpers\outputs" | Out-Null; python .\LLM_Helpers\compare_handicap_infos.py "..\AdvCiv\Assets\XML\GameInfo\CIV4HandicapInfo.xml" ".\Assets\XML\GameInfo\CIV4HandicapInfo.xml" --file1-label "Base AdvCiv 1.12" --file2-label "AdvCiv-SAS" --output ".\LLM_Helpers\outputs\handicap_compare_$ts.md" --tsv-output ".\LLM_Helpers\outputs\handicap_compare_$ts.tsv"; Write-Host "Output: LLM_Helpers\outputs\handicap_compare_$ts.md"
```

## Static audit helpers

### `audit_define_keys.py`

- Report-only. Does not modify source.
- Audits literal `getDefine{BOOL,FLOAT,INT,STRING}("X")` across both Python and DLL source:
  - `Assets/Python/**/*.py`
  - `CvGameCoreDLL/**/*.{cpp,h}` (skip with `--no-dll`)
- Checks each used key against `<DefineName>` declarations from:
  - mod `Assets/XML/*.xml`
  - base BTS `*.xml` (auto: `<mod-root>/../../Assets/XML/`, override `--bts-xml-dir`)
  - vanilla Civ4 `*.xml` (auto: `<mod-root>/../../../Assets/XML/`, override `--vanilla-xml-dir`)
  - any `--extra-xml <file-or-dir>` (repeatable)
- Variable-arg calls (`gc.getDefineINT(szVar)`, `GC.getDefineINT(SOME_MACRO)`) counted only, not audited.
- Use this instead of a runtime `getDefineINTOrFail` wrapper: no clean missing sentinel (returns `0`/`""` for both missing and legit zero/empty). `SASDefineGuard.verify_or_raise()` handles the wholesale "defines didn't load" case; this script handles per-key drift.
- Exit 1 on drift, 0 otherwise. Writes timestamped report to `LLM_Helpers/outputs/` by default.

```powershell
python LLM_Helpers\audit_define_keys.py
python LLM_Helpers\audit_define_keys.py --show-callsites
python LLM_Helpers\audit_define_keys.py --no-dll
python LLM_Helpers\audit_define_keys.py --no-output-file
python LLM_Helpers\audit_define_keys.py --extra-xml <path>
```

Each XML scope is scanned recursively (`rglob *.xml`), but the report only lists files that actually contributed at least one `<DefineName>`. This keeps the audit boundary auditable without dumping hundreds of unrelated entity XMLs (Buildings, Units, etc.).

Example output (clean run, exit 0):

```text
mod XML files (scanned 155, contributed): 9
  - Assets\XML\AI_Variables_GlobalDefines.xml
  - Assets\XML\BBAI_Game_Options_GlobalDefines.xml
  - Assets\XML\GlobalDefines_advc.xml
  - Assets\XML\GlobalDefines_advciv_sas.xml
  - Assets\XML\GlobalDefines_devel.xml
  - Assets\XML\GlobalDefinesAlt.xml
  - Assets\XML\LeadFromBehind_GlobalDefines.xml
  - Assets\XML\PythonCallbackDefines.xml
  - Assets\XML\TechDiffusion_GlobalDefines.xml
BTS XML files (scanned 121, contributed): 2
  - <bts>\Assets\XML\GlobalDefines.xml
  - <bts>\Assets\XML\PythonCallbackDefines.xml
vanilla XML files (scanned 129, contributed): 2
  - <civ4>\Assets\XML\GlobalDefines.xml
  - <civ4>\Assets\XML\GlobalDefinesAlt.xml
extra XML files (scanned 0, contributed): 0
mod XML defines:               697
BTS XML defines:               430
vanilla XML defines:           295
extra XML defines:             0
distinct literal keys (py):    134
distinct literal keys (dll):   658
variable-arg calls (skipped):  491

OK: every literal getDefine{BOOL,FLOAT,INT,STRING} key is declared somewhere.
wrote LLM_Helpers\outputs\20260515T201121Z_audit_define_keys_ok.txt
```

Drift output (exit 1) groups undeclared keys by source, optionally with `--show-callsites`:

```text
DRIFT: 2 key(s) used but not declared in any scanned XML:
  [python]
    SAS_RENAMED_KEY
      - Assets\Python\Screens\CvFoo.py
  [dll_cpp]
    SAS_REMOVED_KEY
      - CvGameCoreDLL\CvBar.cpp
```

### `audit_unused_text_keys.py`

- Report-only. Does not modify source.
- Flags mod GameText `<Tag>TXT_KEY_*` entries that are defined but referenced nowhere.
- DEFINITION: `<Tag>TXT_KEY_FOO</Tag>` inside an active, non-commented `<TEXT>...</TEXT>` block, in files matched by `--text-glob` (default `Assets/XML/Text/*.xml`).
- REFERENCED if the exact token appears in:
  - mod `Assets/Python/**/*.py`, `PrivateMaps/**/*.py`, `CvGameCoreDLL/**/*.{cpp,h}`, `Assets/Config/**/*.xml`, and `Assets/XML/**/*.xml` (`<Tag>` definition spans and source comments are scrubbed so definitions or commented-out uses do not count as references)
  - base BTS `Python/**/*.py` + `XML/**/*.xml` (auto: `<mod-root>/../../Assets`, override `--base-assets`)
  - vanilla Civ4 `Python/**/*.py` + `XML/**/*.xml` (auto: `<mod-root>/../../../Assets`, override `--vanilla-assets`)
  - skip the external scan with `--no-external` (mod-only; expect engine/front-end false positives)
- Scanning base+vanilla is the authoritative source for inherited engine/front-end keys (main menu, setup, Civilopedia, sealevel/worldsize Info XML) the mod does not override — confirmed used by evidence, not whitelist.
- Buckets: LIKELY-UNUSED (candidates); REVIEW (name under a known dynamic-construction prefix, e.g. `TXT_KEY_BUG_OPT_*` built via `"..."+id`); engine-derived suffixes (`_PEDIA/_STRATEGY/_HELP/...`) treated used if the base key is referenced.
  - Output is candidates, not proof — a small pure-EXE-internal residue can remain; verify before deleting.
- `--prefix TXT_KEY_SAS` to focus on mod-custom keys. Exit 1 if any LIKELY-UNUSED, 0 otherwise. Timestamped report to `LLM_Helpers/outputs/` unless `--no-output-file`.

```powershell
python LLM_Helpers\audit_unused_text_keys.py
python LLM_Helpers\audit_unused_text_keys.py --text-glob "Assets/XML/Text/AdvCiv-SAS_*.xml"
python LLM_Helpers\audit_unused_text_keys.py --prefix TXT_KEY_SAS
python LLM_Helpers\audit_unused_text_keys.py --no-external --no-output-file
```

Example output (real run, `--text-glob "Assets/XML/Text/AdvCiv-SAS_*.xml"`; exit 1 because candidates were found):

(from LLM_Helpers\outputs\unused_text_keys_20260517T162642Z.txt)

```text
AdvCiv-SAS unused GameText key audit
mod root : C:\Program Files (x86)\Steam\steamapps\common\Sid Meier's Civilization IV Beyond the Sword\Beyond the Sword\Mods\AdvCiv-SAS
text glob: Assets/XML/Text/AdvCiv-SAS_*.xml
external : C:\Program Files (x86)\Steam\steamapps\common\Sid Meier's Civilization IV Beyond the Sword\Beyond the Sword\Assets | C:\Program Files (x86)\Steam\steamapps\common\Sid Meier's Civilization IV Beyond the Sword\Assets
defined  : 1283 tags in 3 files | referenced tokens: 10510 | rescued by base/vanilla: 11
LIKELY-UNUSED: 16 | REVIEW (maybe dynamic): 28 | duplicate-defined: 0

=== LIKELY UNUSED (no reference in mod, base BTS, or vanilla; verify before deleting) ===

[Assets/XML/Text/AdvCiv-SAS_main.xml]
  TXT_KEY_MAP_SCRIPT_SAS_SIMPLE_FLAT_GRASS_DESCR
  TXT_KEY_PEDIA_BUILD_FEATURE_STRUCT
  TXT_KEY_PEDIA_BUILD_IMPROVEMENT
  TXT_KEY_PEDIA_BUILD_REMOVE
  TXT_KEY_PEDIA_BUILD_REMOVES_FEATURES
  TXT_KEY_PEDIA_CHANGES
  TXT_KEY_PEDIA_SAS_EVENT_TRIGGER_OUTCOME_COUNT
  TXT_KEY_PEDIA_SAS_IMPROVEMENTS_SHORT
  TXT_KEY_PEDIA_SAS_MUSIC_GROUPING_LEADERS_DIPLO
  TXT_KEY_PEDIA_SAS_MUSIC_GROUPING_MENUS_OPENING
  TXT_KEY_PEDIA_SAS_OPEN_PEDIA_ENTRY
  TXT_KEY_PEDIA_SAS_PLAY_MOVIE
  TXT_KEY_PEDIA_TECH_OBSOLETE_UNITS_TREE_NOTE
  TXT_KEY_PEDIA_UNTRADEABLE_TECH_REMINDER
  TXT_KEY_SEALEVEL_HIGH_RECOMMEND
  TXT_KEY_SEALEVEL_LOW_RECOMMEND

=== REVIEW: matches a known dynamic-construction prefix (likely used via string concat) ===

[Assets/XML/Text/AdvCiv-SAS_main.xml]
  TXT_KEY_BUG_OPTLABEL_FOREIGN_ADVISOR
  TXT_KEY_BUG_OPTLABEL_FOREIGN_DIPLOMACY_ADVISOR
  TXT_KEY_BUG_OPTLABEL_INFO_SCREENS
  TXT_KEY_BUG_OPTLABEL_RELIGIOUS_ADVISOR
  TXT_KEY_BUG_OPTLABEL_SEVOPEDIA
  TXT_KEY_BUG_OPTLABEL_VICTORY_CONDITIONS
  TXT_KEY_BUG_OPT_ACO__IGNOREBARBFREEWINS_HOVER
  TXT_KEY_BUG_OPT_ADVISORS__BUGRELIGIOUSTAB_HOVER
  TXT_KEY_BUG_OPT_ADVISORS__BUGRELIGIOUSTAB_TEXT
  TXT_KEY_BUG_OPT_ADVISORS__EFAGLANCETAB_HOVER
  TXT_KEY_BUG_OPT_ADVISORS__GPTECHPREFS_HOVER
  TXT_KEY_BUG_OPT_ADVISORS__GPTECHPREFS_TEXT
  TXT_KEY_BUG_OPT_AUTOSAVE__USEPLAYERNAME_HOVER
  TXT_KEY_BUG_OPT_MAININTERFACE__MODNAMEINREPLAYS_HOVER
  TXT_KEY_BUG_OPT_UNITNAMING__COMBAT_AIR_BOMBER_HOVER
  TXT_KEY_BUG_OPT_UNITNAMING__COMBAT_AIR_BOMBER_TEXT
  TXT_KEY_BUG_OPT_UNITNAMING__COMBAT_AIR_FIGHTER_HOVER
  TXT_KEY_BUG_OPT_UNITNAMING__COMBAT_AIR_FIGHTER_TEXT
  TXT_KEY_BUG_OPT_UNITNAMING__COMBAT_ARCHER_BOW_LONG_HOVER
  TXT_KEY_BUG_OPT_UNITNAMING__COMBAT_ARCHER_BOW_LONG_TEXT
  TXT_KEY_BUG_OPT_UNITNAMING__COMBAT_ARCHER_BOW_SHORT_HOVER
  TXT_KEY_BUG_OPT_UNITNAMING__COMBAT_ARCHER_BOW_SHORT_TEXT
  TXT_KEY_BUG_OPT_UNITNAMING__COMBAT_ARCHER_CROSSBOW_HOVER
  TXT_KEY_BUG_OPT_UNITNAMING__COMBAT_ARCHER_CROSSBOW_TEXT
  TXT_KEY_BUG_OPT_UNITNAMING__COMBAT_MOUNTED_MELEE_HOVER
  TXT_KEY_BUG_OPT_UNITNAMING__COMBAT_MOUNTED_MELEE_TEXT
  TXT_KEY_BUG_OPT_UNITNAMING__COMBAT_MOUNTED_RANGED_HOVER
  TXT_KEY_BUG_OPT_UNITNAMING__COMBAT_MOUNTED_RANGED_TEXT
```

`rescued by base/vanilla` counts tags a mod-only scan would have wrongly flagged that the inherited base/vanilla scan proved are used (e.g. `TXT_KEY_MAIN_MENU_*`, `TXT_KEY_PEDIA_SCREEN_TOP`).

This run helped identify (after review) the following as unused and safe to remove from `Assets/XML/Text/AdvCiv-SAS_main.xml`:

```log
TXT_KEY_PEDIA_BUILD_FEATURE_STRUCT
TXT_KEY_PEDIA_BUILD_IMPROVEMENT
TXT_KEY_PEDIA_BUILD_REMOVE
TXT_KEY_PEDIA_BUILD_REMOVES_FEATURES
TXT_KEY_PEDIA_SAS_EVENT_TRIGGER_OUTCOME_COUNT
TXT_KEY_PEDIA_SAS_IMPROVEMENTS_SHORT
TXT_KEY_PEDIA_SAS_MUSIC_GROUPING_LEADERS_DIPLO
TXT_KEY_PEDIA_SAS_MUSIC_GROUPING_MENUS_OPENING
TXT_KEY_PEDIA_SAS_OPEN_PEDIA_ENTRY
TXT_KEY_PEDIA_SAS_PLAY_MOVIE
TXT_KEY_PEDIA_UNTRADEABLE_TECH_REMINDER
```

## Markdown documentation cleanup helpers

### `reflow_markdown_prose.py`

Conservative source-rewrite helper for active Markdown documentation.

- Inserts only source line breaks at conservative sentence boundaries on already-long prose lines; it does not rewrite words, join existing lines, or wrap to a fixed width.
- Markdown normally renders those source newlines as spaces, so they improve source diff/LLM granularity without creating a visible paragraph or list break.
- That is intentionally only the mechanical first step: for rendered readability, use a blank line for a real new paragraph/thought chunk and a real nested sub-bullet (normally two-space indentation in this repository) for a distinct child point rather than plain continuation indentation.
- Do not use trailing `\` hard breaks as a substitute for paragraph structure: if prose is a separate thought, use a blank-line paragraph; if it is still one paragraph, leave it as such.
- Skips fenced/indented code, headings, blockquotes, tables, HTML-like lines, raw logs/data rows, hard-break lines, URLs, link destinations, inline code spans, decimal/version punctuation, and common abbreviations where automatic sentence detection would be risky.
- Default threshold is 320 physical characters. The threshold only decides which long lines are candidates; split positions are still semantic sentence boundaries rather than character-count boundaries.
- `--check` is available for manual/reporting use, but this helper is intentionally **not** a strict CI style gate: valid long lines exist and semantic sub-bullet decisions require context.
- Semantic visible restructuring remains a human/LLM review task. In particular, convert evidence/validation/update/part-2 material into nested sub-bullets only when it genuinely belongs to the same change; distinct changes remain peer bullets.
- Preserves the input line-ending style and is idempotent after a successful pass.
- Always review the diff before committing.

Examples:

```powershell
python LLM_Helpers\reflow_markdown_prose.py _1_AdvCiv-SAS\Docs\README_Known_Issues.md --diff
python LLM_Helpers\reflow_markdown_prose.py _1_AdvCiv-SAS\Docs\README_Known_Issues.md --in-place
python LLM_Helpers\reflow_markdown_prose.py _1_AdvCiv-SAS\Docs\README_Main_Changes_Guide.md --in-place
```

## Legacy XML duplicate discovery scanner (``scan_xml_duplicates-3.3.py``)

`scan_xml_duplicates-3.3.py` is an older broad XML duplicate scanner kept here for reference and discovery work, on [AdvCiv-SAS@python-scripts branch's GitHub repository](https://github.com/wonderingabout/AdvCiv-SAS/blob/python-scripts/scan_xml_duplicates-3.3.py).

This script scans `Assets/XML` recursively and reports repeated sibling XML entries based on a broad heuristic: if sibling tags repeat, it compares either their first child field/value or their direct text value. Because this is intentionally broad, it can find useful suspicious patterns, but it can also report valid Civ4 XML structures where repetition is intentional, weighted, or structurally normal.

It should not be treated as a release gate or as proof that every reported duplicate is wrong. Instead, it is useful as an exploratory audit tool: run it when looking for possible XML cleanup targets, review the output manually, and then move high-confidence findings into the curated GitHub workflow checks.

This legacy scanner notably helped inspire and calibrate the newer `.github/workflows/build` XML audits, including the parent duplicate-key checker, child/list duplicate checker, and duplicate text-tag checker. The newer workflow scripts are stricter and more conservative: they check reviewed XML patterns with clearer semantics, print source line numbers, and avoid known noisy cases such as weighted goody hut entries or reused world-picker UI art paths.

In short: this script is kept as a historical and practical discovery helper, while the GitHub workflow checks are the maintained release-safety layer.

## AdvCiv manual text conversion helper

### `convert_advciv_manual_to_txt.py`

- Converts the tracked base AdvCiv [`manual.odt`](/_0_Common_Docs/AdvCiv_Base_Doc/manual.odt) into [`manual.txt`](/_0_Common_Docs/AdvCiv_Base_Doc/manual.txt) for grep, VS Code/global search, LLM ingestion, and other plain-text review.
- Uses only Python 3's standard library: ODT is a ZIP container and the helper reads its `content.xml` directly. Pandoc, LibreOffice, `odt2txt`, and other external converters are not required.
- Visual fidelity is deliberately not the goal. The helper preserves normal paragraphs/headings, list headers and bullets/nesting, explicit spaces/tabs/line breaks, and simple/nested tables in a compact searchable representation.
- Refuses to overwrite `manual.txt` if extraction is suspiciously tiny, to reduce the risk of committing a broken conversion.
- Defaults to the repository's base-manual paths, so the normal refresh command from the mod root is:

```bash
python ./LLM_Helpers/convert_advciv_manual_to_txt.py
```

- After running it, review the generated text and Git diff before committing:

```bash
git diff -- "_0_Common_Docs/AdvCiv_Base_Doc/manual.txt"
```

- When base AdvCiv updates `manual.odt`, refresh `manual.txt` in the same merge/release-preparation batch so the searchable copy does not lag behind the binary manual.
- CI validates this conversion through the reusable `TEXT_CONVERSIONS` registry in [generated_docs.py](/.github/workflows/build/generated_docs.py).
  - Future source-to-text conversions can register their own source, output and converter there; the AdvCiv manual is currently the only entry.
  - Related `manual.pdf` edits also require a paired text refresh in the complete PR/push range.
- Added with help of ChatGPT-5.6-Sol.

## Source packaging and generated-history helpers

### `make_light_source_zip.py`

- Creates a timestamped light source ZIP for a Civ4 mod, mainly for compact local/LLM/code-agent review handoffs.
- Uses repo-relative archive paths and `ZIP_DEFLATED` compression by default.
  - ZIP is intentionally used instead of 7z because 7z uploads caused errors before, while ZIP is currently an as of now seemingly easily compatible format for ChatGPT/code-agent review.
  - Use `--compression-level 0` for the old `ZIP_STORED` / no-compression behavior.
- Compression applies to the whole archive, not only images: JPG/PNG screenshots are already compressed and shrink little, while XML/Python/docs shrink a lot, so whole-ZIP compression is the useful default once selected screenshot folders are included.
- Adds an archive-only `_SNAPSHOT_CONTEXT/` folder generated automatically from the local Git repository. The neutral name reflects that these files are extra context for the archive snapshot rather than repository files; they can help human reviewers, LLMs, or other tools. It contains:
  - `repo_file_manifest.txt`: every tracked path from `git ls-files` (including files intentionally omitted from the light ZIP), with the exact current working-tree byte size before each path.
    - This is deliberately only tracked-file inventory/state, so an external/ZIP-only reviewer can distinguish "not included in this light archive" from "not present in the local repository" and can still see useful size clues for omitted binaries such as `Assets/CvGameCoreDLL.dll`.
    - A tracked path missing from the working tree is marked `MISSING`.
    - Tracked paths preserve Git's canonical path spelling/casing.
    - Untracked paths are intentionally not enumerated to avoid exposing unrelated local filenames; selected untracked source files can still be included normally by the exporter.
  - `git_repository_state.txt`: current branch/HEAD, total commit count, locally known upstream plus ahead/behind counts, active `MERGE_HEAD`/matched merge target when applicable, tracked `git status --short --untracked-files=no` output, and any files already selected for the ZIP that are not tracked by Git.
    - AdvCiv-SAS commonly uses that total commit count as its practical version number in documentation (e.g. the `X` in `requires AdvCiv-SAS X+`), while `HEAD` is the exact source-state identifier.
    - Git short status uses two columns (`X` = index/staged state, `Y` = working-tree/unstaged state), e.g. `M ` for staged modification, ` M` for unstaged modification, and `MM` for a staged file modified again afterward.
    - General untracked paths are still not enumerated.
    - Upstream/ahead-behind values use the locally known upstream ref and can be stale until `git fetch`.
    - This compact state summary is kept separate from the file manifest and from the full diffs.
  - `git_ignored_paths_tree.txt`: compact ASCII tree of paths ignored by Git's effective standard ignore rules.
    - Entire ignored directories can be collapsed to one entry, so it can reveal useful local/generated/build context that is absent from the tracked manifest without exploding into a list of every file underneath those directories.
    - It is separate from `repo_file_manifest.txt` because ignored paths are local repository state, not tracked repository contents.
  - `staged_changes_no_eol.diff`: raw staged tracked changes (`HEAD -> index`) using Git's end-of-line whitespace/CR ignore options so line-ending-only noise does not dominate review. An empty file means there are no staged tracked changes.
  - `unstaged_changes_no_eol.diff`: raw unstaged tracked changes (`index -> working tree`) with the same end-of-line-noise filtering. An empty file means there are no unstaged tracked changes.
  - `git_log_since_tracked_advciv_sas_log.txt`: anonymized commit messages after the newest commit already present in [`git_log_anonymized_email_003_AdvCiv-SAS.txt`](/_1_AdvCiv-SAS/Docs/git_logs/git_log_anonymized_email_003_AdvCiv-SAS.txt) through the snapshot's `HEAD`; the already-recorded boundary commit is excluded to avoid duplication.
    - Unlike the repository's tracked AdvCiv-SAS Git log, which is newest-to-oldest, this generated gap is deliberately chronological (oldest-to-newest), so snapshot `HEAD` is at the bottom.
    - Keeping the full commit-message gap preserves detailed implemented notes that may later be removed from the temporary untracked `changes_old.md` and `changes_new.md`, without needlessly embedding every potentially huge source/XML patch in the light archive.
- Adds freshly generated canonical history context under `LLM_Helpers/context/commit_diffs/`, including `INDEX.txt`, `PATH_HISTORY_INDEX.txt`, and `<segment>_<practical-count>_<short-sha>.diff` files for every selected commit reachable from the snapshot's current `HEAD` by default.
  - This spans the inherited K-Mod -> pre-SAS AdvCiv -> AdvCiv-SAS branch history.
  - Unrelated/unmerged branch refs are not included; commits from side branches that were actually merged remain because they genuinely contribute to current `HEAD`.
  - Each commit is compared with its first parent.
  - Repository storage policy: `commit_diffs/` is generated and Git-ignored rather than tracked because it is reproducible from Git ancestry.
    - The permanent [`context/commit_rewrite_map_6164_6402.tsv`](/LLM_Helpers/context/commit_rewrite_map_6164_6402.tsv) translates pre-rewrite SHAs from the one-time cleanup that removed the formerly tracked generated mirror.
  - History segments: `KMod` is the K-Mod history; `AdvCivPreSAS` is the base AdvCiv history before the AdvCiv-SAS branch began; `SASBranch` is the later current-branch history.
  - Post-fork meaning: `SASBranch` is deliberately a history/log segment rather than an authorship label.
    - Upstream AdvCiv development continued after SAS began, so genuine later AdvCiv commits merged/imported into the SAS branch appear there alongside SAS commits.
    - This keeps the existing three anonymized logs intact while clearly distinguishing inherited pre-SAS AdvCiv from the post-fork branch history.
  - Contents/messages: each commit diff keeps a short title preview, compact change summary, patch coverage marker, and useful textual patches.
    - Full commit messages/metadata are intentionally not duplicated there: use the included anonymized K-Mod, base AdvCiv, and AdvCiv-SAS Git logs, with `_SNAPSHOT_CONTEXT/git_log_since_tracked_advciv_sas_log.txt` supplying recent branch messages not yet recorded in the tracked AdvCiv-SAS log.
  - Index/navigation: `INDEX.txt` is deliberately a lean locator (`segment`, practical count, short SHA, coverage, short title) rather than another Git log.
    - `PATH_HISTORY_INDEX.txt` provides the reverse path -> commits view, so an LLM can narrow a historical investigation to commits that touched a particular file before opening their diff files.
    - Practical counts are useful historical hints but can repeat on divergent/merged history; the full Git SHA is the canonical unique commit identifier.
  - Compactness/filtering:
    - known generated/log/binary or redundant historical payloads such as SASGameRecord logs, copied Git logs/manuals, generated Sevopedia leader caches/data, and old "files to feed ChatGPT" snapshots are summarized rather than embedded.
    - Imported reference documents under `_0_Common_Docs/` and published changelog payloads under current/former `Docs/changelogs_web/` locations are also summarized: changed paths, counts and omission reasons remain in the commit record and path-history index, while the current files remain available under the usual snapshot selection rules.
    - Maintained `README.md`/`readme.txt` indexes and source files in these folders retain their patches; a rename crossing between maintained content and reference/archive content is retained too. Ordinary maintained guides, Known Issues, revision history and helper documentation stay inspectable.
    - Existing-file patches above 10,000 changed lines are also summarized because they are usually rewrites/import churn rather than useful line-by-line history; a genuinely new functional source/config file is exempt from that line threshold (while the byte cap still applies), so e.g. adding a large real XML source file can remain inspectable.
    - Exceptionally huge individual textual patches still use generous byte caps so one file cannot dominate the handoff.
  - Diff generation: Git textconv/external diff drivers are explicitly disabled for this history, so local helpers such as `odt2txt` are never launched; redundant document formats such as ODT/PDF are summarized while text/manual equivalents remain available in the current snapshot.
    - Uncached commits use one Git process for summary+patch generation, and practical counts are derived exactly from the already-read reachable commit DAG instead of launching one `rev-list --count` process per commit, including histories with merges.
  - Cache persistence: rendered commit files are cached locally inside Git metadata under a cache-format-versioned folder and keyed by the commit's immutable full SHA.
    - Normal reruns reuse old commits and render only genuinely new hashes; amend/force-push/hard-reset needs no arbitrary "refresh last N" logic because rewritten commits automatically have new hashes, while unreachable cached hashes are simply not selected into the current ZIP.
    - The cache persists across Windows/Python restarts, and unchanged SHAs can be reused again if a later reset makes them reachable.
  - Cache migration/cleanup: existing SAS cache entries from the older message-heavy diff layout are upgraded in place to the title-only header instead of forcing Git to re-render them.
    - The cache needs no `.gitignore` entry and is never included as a repository file.
    - After a successful writable history build, superseded helper-owned cache-policy directories are pruned automatically; dry-runs never prune.
  - Privacy: consistent with the existing anonymized K-Mod/AdvCiv/AdvCiv-SAS Git-log exports, generated commit-history context does not expose Git author/committer email addresses.
    - Email-shaped strings are also redacted from index title previews and embedded historical patches, preventing old copied Git logs or other committed text from re-exposing addresses; repository source files themselves are not rewritten.
    - Older cache entries are sanitized while being upgraded in place instead of forcing Git to re-render the whole history.
  - Controls: `--commit-diff-count 0` disables current-HEAD commit-diff history, while a positive `N` limits output to the newest `N` commits reachable from current `HEAD` for an unusually small handoff; the default `-1` keeps the full reachable ancestry.
  - Canonical context/self-recursion guard: the same `LLM_Helpers/context/commit_diffs/` path is used for the Git-ignored local mirror and inside the light ZIP.
    - The directory is deliberately excluded from historical patches, ordinary light-ZIP tree selection, and raw staged/unstaged review diffs; the ZIP injects the freshly generated result once at the canonical path, then normal full-history generation can refresh the local generated copy after the archive succeeds.
    - This prevents generated context from being tracked recursively inside the history it describes or dominating a handoff's working-tree diff.
    - Use `--no-sync-context` when a full-history ZIP should deliberately leave the local generated context untouched; dry-runs and disabled/truncated history never refresh it.
  - Current map references and source-analysis records remain in the light ZIP, but their own histories are likewise omitted from generated patches so large reference/progress revisions do not obscure runtime-source history.
  - `pending_upstream/INDEX.txt`, `GIT_LOG.txt`, `PATH_HISTORY_INDEX.txt`, `UPSTREAM_REFS.txt`, plus `<sequence>_<short-sha>.diff`: separate fetched-but-unmerged base AdvCiv release history.
    - During an active merge, exact `MERGE_HEAD` wins so the ZIP describes the commit actually being merged even if `upstream/*` moves afterward.
    - Outside a merge, automatic discovery considers all locally fetched release-like refs (`upstream/X.Y[.Z]`, `upstream/vX.Y[.Z]`, `upstream/release-X.Y[.Z]`, or `upstream/release/X.Y[.Z]`), exports the deduplicated union of commits reachable from those refs but not from `HEAD`, and uses the highest detected version only as the presentation target.
    - In the normal linear case, older releases add no duplicate commits; if release lines diverge, otherwise-missed commits remain visible.
  - `UPSTREAM_REFS.txt` records what was detected/selected and lists topic or experimental refs only as awareness context rather than silently treating whichever branch was updated most recently as a release.
    - If upstream naming changes or a special maintenance line matters, repeat `--upstream-ref REF` to select explicit revisions; outside a merge these override auto selection, while an active `MERGE_HEAD` remains authoritative.
    - This folder is deliberately separate from `commit_diffs/` because pending upstream commits are review/merge context, not current-HEAD ancestry.
    - Pending diffs use the same compact filtering/privacy policy but are not written into the canonical HEAD-history SHA cache because their archive metadata is range/target-specific and the pending set is normally small.
  - `README.txt`: short explanation of the generated files for ZIP-only reviewers. If Git metadata or one of the tracked lineage logs is unavailable, the generated context keeps working where possible and reports the missing classification/message source instead of making archive creation fail.
- Uses Git's canonical `Assets/Res` casing even if Windows locally displays or accepts `Assets/res`; repository paths are case-sensitive on GitHub/Linux CI, and the generated manifest deliberately preserves the Git spelling.
- Prints final ZIP size plus total, generated-context, ZIP-write, and tracked-context-refresh durations by default. Use `--no-duration` if stable/deterministic-looking command output is preferred.
- Default output directory is the mod root. Use `--output-dir` for Downloads or another handoff folder.
- Output filename defaults to `<detected-mod-folder-name>_light_source_<timestamp>.zip`, with `UnspecifiedModName` as a fallback. Use `--mod-name` or `--prefix` only for unusual/manual labels.
- Includes small source/data/docs folders useful for review: root lone files, selected [Assets](/Assets/) folders, root helper/doc/config folders including [LLM_Helpers](/LLM_Helpers/) itself, top-level [CvGameCoreDLL](/CvGameCoreDLL/) files, top-level [CvGameCoreDLL/Project](/CvGameCoreDLL/Project/) files under 1 MB, and the tracked `CvGameCoreDLL/Project/temp_files/.gitkeep` workflow marker while excluding retained Debug-opt compiler intermediates/private symbols.
  - It also includes [_1_AdvCiv-SAS/Docs](/_1_AdvCiv-SAS/Docs/) (including the published web changelog archive and tracked AdvCiv-SAS Git log), [_1_AdvCiv-SAS/SASGameRecord_log](/_1_AdvCiv-SAS/SASGameRecord_log/) full `SASGameRecord` examples, and selected screenshot folders useful for LLM/UI/rendered-map-text review: [_1_AdvCiv-SAS/Images/advisors](/_1_AdvCiv-SAS/Images/advisors/), [_1_AdvCiv-SAS/Images/main_menu](/_1_AdvCiv-SAS/Images/main_menu/), [_1_AdvCiv-SAS/Images/SASGameRecord_map_text](/_1_AdvCiv-SAS/Images/SASGameRecord_map_text/), [_1_AdvCiv-SAS/Images/sevopedia](/_1_AdvCiv-SAS/Images/sevopedia/), and [_1_AdvCiv-SAS/Images/ui_other](/_1_AdvCiv-SAS/Images/ui_other/).
- The included `SASGameRecord` sample is an unchanged full-game log rather than a selected excerpt, which makes it reusable for external LLM analysis/review and development reference.
  - Although a level-3 record can occupy tens of megabytes raw, it compresses substantially in the light-source ZIP.
  - The selected screenshot folders similarly provide practical visual reference for understanding/reviewing the mod and for developing or modding UI changes, by showing what the mod actually looks like in advisors, Sevopedia, the main menu, common UI, and rendered `SASGameRecord` map text.
  - The `SASGameRecord_map_text` screenshots are included because an LLM may read the raw text-map characters without reconstructing the visual/geographical layout as easily.
  - Local agentic tools can inspect these folders directly, while external/ZIP-only LLMs depend on the archive contents; including every image folder currently adds 50+ MB and roughly doubles the archive, so only these key folders are included.
- Missing optional folders are skipped with warnings, so the helper can also be run on base AdvCiv or partial comparison folders.
- Skips generated/helper outputs such as `LLM_Helpers/outputs`, Python cache files, previous light-source ZIPs, heavy/binary `.dll` and `.fpk`, non-useful compact-review `.tga`, `manual.pdf`, `Assets/res/Cursors`, and large/temporary DLL project artifacts such as `.sdf` or project files over 1 MB.
  - The exact base-AdvCiv `_0_Common_Docs/AdvCiv_Base_Doc/manual.odt` is a deliberate exception: it is the canonical input consumed by `convert_advciv_manual_to_txt.py`, so ZIP-only LLM/code-agent handoffs can regenerate/test the tracked `manual.txt`; other files named `manual.odt` remain skipped.
- Does not globally exclude common image files such as `.jpg` or `.png`; small previews can be useful for LLM review, e.g. GameFont previews. Avoid heavy art/image folders by not adding those folders to the include lists instead.
- Ordinary ZIP creation stays local/network-free.
  - Use `--fetch-upstream` when the handoff should first refresh base AdvCiv with `git fetch upstream --prune`; if that explicit fetch fails, the helper aborts instead of quietly presenting stale refs as fresh.
  - During a merge this does not change the exact pending target because `MERGE_HEAD` remains authoritative.
- Use one or more `--upstream-ref REF` options when upstream release naming changes or when a nonstandard maintenance line should intentionally be included in pending context; topic/experimental branches are otherwise only listed in `pending_upstream/UPSTREAM_REFS.txt`.
- Use `--dry-run` first to review file count, size, target archive path, and included repo-relative paths without writing the ZIP. Dry-run can reuse existing commit-diff cache entries but deliberately does not persist newly rendered cache entries.
- Created/refined with help of ChatGPT-5.5, ChatGPT-5.6-Sol, and Codex.
- Commit-diff reuse/performance: the private SHA cache inside `.git` is checked first, then the local generated `LLM_Helpers/context/commit_diffs/` context is used as a read-only secondary cache when present, and Git renders a commit only when neither contains a valid entry for the current filtering policy.
  - This keeps normal local reruns fast while also preventing a populated working copy from rerendering thousands of already-generated historical patches.
  - Newly created/rewritten commits that have not reached the local generated context are still rendered once into the private cache.
  - Exact timings depend on the machine/repository, and a cache-format/history-policy change can intentionally require fresh rendering for entries that no longer validate.

Tools like here WizTree helped find which folders/files are heavy to exclude.

Example of creating a light source ZIP in Downloads folder (Git Bash):

```bash
cd "C:\Program Files (x86)\Steam\steamapps\common\Sid Meier's Civilization IV Beyond the Sword\Beyond the Sword\Mods\AdvCiv-SAS" && python ./LLM_Helpers/make_light_source_zip.py --output-dir "C:\Users\PC\Downloads"
```

Same handoff while first refreshing locally known base AdvCiv release refs:

```bash
python ./LLM_Helpers/make_light_source_zip.py --fetch-upstream --output-dir "C:\Users\PC\Downloads"
```

Example of output (Git Bash):

```text
Repo root: C:\Program Files (x86)\Steam\steamapps\common\Sid Meier's Civilization IV Beyond the Sword\Beyond the Sword\Mods\AdvCiv-SAS
Mod name:  AdvCiv-SAS
Prefix:    AdvCiv-SAS_light_source
Archive:   C:\Users\PC\Downloads\AdvCiv-SAS_light_source_20260902T225239.zip
Files:     1327 selected + 6458 generated context files
Size:      404,940,754 bytes before ZIP container overhead
Mode:      ZIP_DEFLATED / compression level 6
History:   commit diffs: 6386 included (SASBranch:2043,AdvCivPreSAS:3094,KMod:1249), 6385 private-cache hit(s), 0 local-mirror hit(s), 1 rendered, 0 not cached; versions=dag-from-one-log; cache=C:\Program Files (x86)\Steam\steamapps\common\Sid Meier's Civilization IV Beyond the Sword\Beyond the Sword\Mods\AdvCiv-SAS\.git\advciv_sas_light_source_commit_diffs\v2_1e731b350907; pending upstream: 59 commit(s) union from 16 selected ref(s); presentation=upstream/1.14; extra-vs-presentation=48
Context:   refreshed LLM_Helpers/context/commit_diffs (added=1, updated=2, removed=0, unchanged=6385)
Wrote:     7785 file(s)
ZIP size:  132,807,311 bytes
Duration:  16,586 ms total (6,765 ms generated context; 8,328 ms ZIP write; 1,228 ms local-context refresh)
```

Note: Light-source ZIP creation is usually quick because historical commit patches are cached. A first run with a missing cache, or the first run after a cache-format or history-selection policy change, can instead remain quiet for several minutes while thousands of patches are regenerated; subsequent runs reuse the rebuilt cache and become fast again.

Note 2: During development with LLMs, as of now, we do not necessarily generate compact light-source ZIP at each prompt: for example, we may use a command like `git diff --staged --ignore-space-at-eol > "uncommitted_staged_changes_no_eol_$(date +%Y%m%dT%H%M%S).diff"` to give the LLM the difference (staged e.g. with the help of VS Code's UI thanks) since a light-source ZIP, for example during an intermediate implementation/review step, saving on upload costs/time.

### `refresh_commit_diffs.py`

- Refreshes the canonical Git-ignored local generated context under `LLM_Helpers/context/commit_diffs/` from current committed Git ancestry without creating a light-source ZIP, so people, Codex and other agents can invoke it independently.
- Reuses `make_light_source_zip.py` directly rather than maintaining a second history parser/filter: segmentation, practical counts, privacy redaction, patch-size limits, `INDEX.txt`, `PATH_HISTORY_INDEX.txt`, and commit reuse policy therefore stay identical.
  - The renderer checks the private `.git` SHA cache first and the local generated context second before asking Git to render a missing commit.
- Removes stale generated commit files after rewritten/amended history while preserving the directory's explanatory `README.txt`.
- `--dry-run` reports additions/updates/removals without writing either the local generated context or history cache.
- The local generated `commit_diffs/` directory is Git-ignored and exists for local IDE grep, Codex/code agents, light-source ZIPs and LLM archaeology. The tracked `LLM_Helpers/context/` documentation and imported `mapscript_refs/` remain `export-ignore` development context rather than player content.
- The local generated copy can naturally lag by the current commit because a commit cannot have a diff for a future SHA. This is expected; current Git is authoritative and the mirror can be regenerated at any time.
- Historical rendering excludes both the canonical context path and its former paths, which is essential: otherwise refreshing or relocating the corpus would recursively make future commit diffs contain older diff files.

From the repository root:

```bash
python LLM_Helpers/refresh_commit_diffs.py
python LLM_Helpers/refresh_commit_diffs.py --dry-run
```

## Workflow rule for timeline tuning

- Always run one full 5% bird-view (`Normal` vs current target speed) before sharing a draft.
- Do not rely only on focused slices (`--focus-start-pct/--focus-end-pct`) when evaluating a candidate.
- After each local tweak, re-check the full 5% table to catch late drift and endpoint issues early.
- After each accepted candidate edit, generate a fresh human-readable review file with:

```powershell
python LLM_Helpers\compare_speed_summaries.py --speed <speed>
```

Use this `slow_*.txt` / `marathon_*.txt` style output as the primary review artifact for discussion.

## General notes for future LLM helpers

- Prefer small, reviewable, single-purpose scripts.
- Keep generated output in `LLM_Helpers\outputs` when possible.
- Do not claim a helper is safe for broad directory sweeps unless it was tested that way.
- Explain whether a script edits files, prints reports, or only suggests changes.
- When editing source files, always preserve behavior first and readability second.
- When output is only a report, optimize for clarity and usefulness rather than runtime compatibility.
