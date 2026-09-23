# GitHub workflow checks

These files are developer/repository automation, not Civ4 runtime files.

- `python-ruff.yml` shows full Ruff output and runs critical Ruff Python sanity checks.
- `python24-compile.yml` runs the real CPython 2.4 parser/bytecode compiler through Docker.
- `build.yml` runs checks that should pass for ordinary committed builds.
- `aip-predump-refresh.yml` refreshes the committed Sevopedia Leader AI Personality predump and opens/updates a PR only when the predump file changes. It can be run manually and also runs automatically after pushes that touch known AIP predump dependency files.
- `build/` contains Python 3 build-check scripts used by `build.yml`.
- `lib/` contains shared Python 3 helpers used by workflow scripts.
- Future `release.yml` and `release/` checks can be added for release-only checks such as documentation marker cleanup and archive contents.

These scripts run outside Civ4, so they may use modern Python 3. They do not need Civ4 Python 2.4 compatibility.

## Features

According to ChatGPT-5.5:

- GitHub workflow checks live under [`.github/workflows`](/.github/workflows/).
- Workflow YAML files must stay directly in that folder (e.g. `build.yml`), while Python 3 build-check scripts live under `build/` and shared workflow helpers live under `lib/`.
- `build.yml` is the authoritative list of build checks
- The `.github/` workflow folder is development infrastructure and should stay excluded from player release archives through `.gitattributes`.

For example, this helped spot [map scripts that were previously unclassified in SAS map-script heaviness defines](https://github.com/wonderingabout/AdvCiv-SAS/actions/runs/27198308080/job/80295526028); they are now listed explicitly for exhaustiveness. This also helped spot and fix duplicate parent XML keys [found by new GitHub workflow check](https://github.com/wonderingabout/AdvCiv-SAS/actions/runs/27258698041/job/80498936912) (See if needed [KI#148](/_1_AdvCiv-SAS/Docs/README_Known_Issues.md#ki-148)).

## AIP predump refresh workflow

[`aip-predump-refresh.yml`](/.github/workflows/aip-predump-refresh.yml) refreshes `SevoPediaLeaderCachePredumped.py` outside Civ4. It keeps the manual `workflow_dispatch` trigger, and also runs automatically on pushes that touch known AIP predump dependency files:

- `Assets/XML/Civilizations/CIV4LeaderHeadInfos.xml`
- `Assets/XML/AI_Variables_GlobalDefines.xml`
- `Assets/XML/GlobalDefines_advciv_sas.xml`
- `Assets/Python/Contrib/Sevopedia/ai_utils_shared_with_civ4.py`
- `Assets/Python/Contrib/Sevopedia/sas_utils_shared_with_civ4.py`
- `Assets/Python/Contrib/Sevopedia/SevoPediaLeaderAIPValues.py`
- `.github/workflows/build/aip_predump_values.py`
- `.github/workflows/aip-predump-refresh.yml`

The workflow checks out the selected or pushed branch, runs `python .github/workflows/build/aip_predump_values.py --write`, reruns the normal checker, and then inspects the predump file diff. If there is no diff, the log says the predump is already current and no PR is created. If there is a diff, it commits only `SevoPediaLeaderCachePredumped.py` to `bot/refresh-aip-predump-cache-<base-branch>` and creates or updates a PR named `Refresh AIP predump cache` targeting the branch that triggered the workflow.

The bot ignores its own refresh branches and skips runs by `github-actions[bot]`, so a generated predump PR does not recursively trigger another refresh. This uses only official checkout/setup-python actions plus GitHub's runner-provided `git`/`gh` tools. If PR creation fails with a permission error, check the repository's Actions workflow permissions: the workflow needs `contents: write`, `pull-requests: write`, and repository settings that allow GitHub Actions to create pull requests.

Validated behavior, tested in [PR #31](https://github.com/wonderingabout/AdvCiv-SAS/pull/31):

- current `main` predump already current -> no PR;
- leader XML comment-only change (Test 0) -> no PR;
- real AIP numeric XML change (e.g., Temporary test 1: increase LEADER_MOCTEZUMA's war likeliness iMaxWarRand from 50 to 10) -> predump refresh PR;
- shared AIP label/display change (e.g., Temporary test 2: "Build Unit %" -> "Build Unit Prob") -> predump refresh PR.

The [PR #31](https://github.com/wonderingabout/AdvCiv-SAS/pull/31) body keeps the detailed temporary test commits, GitHub Actions logs, and temporary bot PR links. The important result is that the bot reacts to actual generated predump drift, not merely to touched dependency files.

## Python Ruff workflow

[`python-ruff.yml`](/.github/workflows/python-ruff.yml) runs latest Ruff through `astral-sh/ruff-action` as a separate Python sanity workflow. It intentionally runs isolated from [`ruff.toml`](/ruff.toml), because `ruff.toml` is for the user's VS Code Ruff extension and local editing signal/noise, not for CI strictness. The local config keeps legacy Civ4/Python 2.4 false-positive noise manageable in the user's IDE, while GitHub should show the broader repository picture. Both configurations exclude the verbatim `LLM_Helpers/context/mapscript_refs` archaeology corpus because downloaded reference sources are not AdvCiv-SAS runtime code and should not be lint-modified.

The workflow first prints the full `ruff check . --isolated --select ALL` grouped report so all findings are visible, then fails on a practical critical gate (ignoring only those deemed non-critical and too noisy after empirical review) for syntax/parse errors and Pyflakes bug checks. The first GitHub run showed that only a few rules were clearly too noisy, so this is the first cautious narrowing step; narrow further only from actual failure output. For example, this helped spot shadowed duplicate Python callbacks in the base Civ4 `Oasis.py` map script (see [KI#164](/_1_AdvCiv-SAS/Docs/README_Known_Issues.md#ki-164)) and a dormant undefined `Point` (instead of `PointLayout`) helper in `RectLayout.py` (see [KI#165](/_1_AdvCiv-SAS/Docs/README_Known_Issues.md#ki-165)).

Note: Separate artifacts were considered, which according to ChatGPT-5.5 are as of now limited to 500MB for free users, but GitHub Actions already lets us view on the web browser/URL and download workflow logs as ZIP (as of now right click "Download Log Archive"), so we do not need and so do not use artifact storage for now.

## Python 2.4 compile workflow

[`python24-compile.yml`](/.github/workflows/python24-compile.yml) runs [`build/python24_compile.py`](#buildpython24_compilepy) inside `ghcr.io/wonderingabout/python-2.4:2.4.6`, built from the separate [`wonderingabout/python-2.4-docker`](https://github.com/wonderingabout/python-2.4-docker) Docker-image recipe. It checks `Assets/Python` and `PrivateMaps` with the real CPython 2.4 parser/bytecode compiler, which is closer to Civ4's embedded Python than Ruff or modern Python 3.

This is intentionally a syntax/compile compatibility check only: it does not launch Civ4, run gameplay code, import `CvPythonExtensions`, or validate engine-only runtime objects. It uses `docker run` after normal checkout instead of a job-level container so GitHub Actions and checkout still run on the ordinary runner, while the old image only needs to provide Python 2.4. A [test run confirmed it fails](https://github.com/wonderingabout/AdvCiv-SAS/actions/runs/27897204427) on Python 2.5+ ternary syntax such as `SAS_MAGIC_PY24_COMPILE_BREAK_TEST = 1 if True else 0` in `Assets/Python/SASMagicNumbers.py`.

## Current build checks

- [`build/temp_files.py`](#buildtemp_filespy)
- [`build/line_endings.py`](#buildline_endingspy)
- [`build/assets_dlls.py`](#buildassets_dllspy)
- [`build/art_button_paths.py`](#buildart_button_pathspy)
- [`build/markdown_images.py`](#buildmarkdown_imagespy)
- [`build/markdown_links.py`](#buildmarkdown_linkspy)
- [`build/sas_text_references.py`](#buildsas_text_referencespy)
- [`build/global_defines_nonempty.py`](#buildglobal_defines_nonemptypy)
- [`build/version_metadata.py`](#buildversion_metadatapy)
- [`build/launch_guard.py`](#buildlaunch_guardpy)
- [`build/define_int_bounds.py`](#builddefine_int_boundspy)
- [`build/turn_define_gamespeed.py`](#buildturn_define_gamespeedpy)
- [`build/xml_comments.py`](#buildxml_commentspy)
- [`build/xml_element_only_content.py`](#buildxml_element_only_contentpy)
- [`build/xml_suspicious_angle_tags.py`](#buildxml_suspicious_angle_tagspy)
- [`build/xml_suspicious_text_chars.py`](#buildxml_suspicious_text_charspy)
- [`build/xml_civ4_text_characters.py`](#buildxml_civ4_text_characterspy)
- [`build/xml_sas_text_english.py`](#buildxml_sas_text_englishpy)
- [`build/xml_text_duplicate_tags.py`](#buildxml_text_duplicate_tagspy)
- [`build/xml_parent_duplicate_keys.py`](#buildxml_parent_duplicate_keyspy)
- [`build/xml_child_duplicates.py`](#buildxml_child_duplicatespy)
- [`build/xml_redundant_defaults.py`](#buildxml_redundant_defaultspy)
- [`build/tech_columns.py`](#buildtech_columnspy)
- [`build/wonder_cost_columns.py`](#buildwonder_cost_columnspy)
- [`build/wonder_culture_gpp_columns.py`](#buildwonder_culture_gpp_columnspy)
- [`build/asset_tech_prereq_redundancy.py`](#buildasset_tech_prereq_redundancypy)
- [`build/civ_specific_assets.py`](#buildciv_specific_assetspy)
- [`build/civ_specific_not_weaker.py`](#buildciv_specific_not_weakerpy)
- [`build/define_tag_refs.py`](#builddefine_tag_refspy)
- [`build/raw_getinfotype.py`](#buildraw_getinfotypepy)
- [`build/tech_audio.py`](#buildtech_audiopy)
- [`build/leader_audio.py`](#buildleader_audiopy)
- [`build/opening_music.py`](#buildopening_musicpy)
- [`build/diagnostic_log_safety.py`](#builddiagnostic_log_safetypy)
- [`build/bbai_log.py`](#buildbbai_logpy)
- [`build/sas_game_record_log.py`](#buildsas_game_record_logpy)
- [`build/fonts.py`](#buildfontspy)
- [`build/detail_manager.py`](#builddetail_managerpy)
- [`build/aip.py`](#buildaippy)
- [`build/aip_predump_values.py`](#buildaip_predump_valuespy)
- [`build/worldsizes.py`](#buildworldsizespy)
- [`build/mapscripts.py`](#buildmapscriptspy)
- [`build/python24_compile.py`](#buildpython24_compilepy)

### `build/temp_files.py`

Verifies `CvGameCoreDLL/Project/temp_files/` exists through its zero-byte tracked placeholder and otherwise contains only the optional locally ignored `Debug-opt` target. That target is retained because its exact matching PDB makes crash dumps actionable, but must be deleted before the next Debug-opt build. Other generated targets remain visible and fail the check; only the exact Debug-opt `.gitignore` rule and the matching `.gitattributes` release-export exclusions are allowed.

This helps catch stale fast-compile intermediates and forgetting to replace the committed DLL after compiling (see also [KI#38](/_1_AdvCiv-SAS/Docs/README_Known_Issues.md#ki-38) and [KI#38.2](/_1_AdvCiv-SAS/Docs/README_Known_Issues.md#ki-38.2)).

### `build/line_endings.py`

Verifies active text-like source/config/docs files do not mix CRLF and LF line endings within the same file and that non-empty files end with a newline. This is intentionally a reporting/fail-fast build check only; for local reviewed cleanup, use [`LLM_Helpers/fix_line_endings.py`](/LLM_Helpers/fix_line_endings.py). Generated/reference helper folders such as `LLM_Helpers/outputs` and the verbatim `LLM_Helpers/context/mapscript_refs` corpus are excluded to avoid noise or rewriting downloaded originals.

The script prints its scan mode. In a normal Git checkout it lists tracked files with `git ls-files` and checks the current checked-out bytes. In a plain exported folder, such as an extracted light-source zip without `.git`, it falls back to scanning the configured active source/doc folders directly.

Before the first cleanup, this checker found 79 local worktree issues, while GitHub Actions reported only 19. Comparing local bytes with `git show :path` showed that some local mixed-EOL files were clean LF-only in Git's indexed blob (e.g., `BugInit.py`) while true CI failures stayed mixed in both (e.g., `Pangaea.py`).

Fixing all local findings updated 78 worktree files, but staging kept only the 19 stored-content changes that Git/CI saw. This makes local output useful cleanup/export guidance, while staged/GitHub output is the authoritative committed-content failure set.

Note: later on, we noticed a line endings test failed locally:

```log
FAIL text line-ending hygiene
  - .github/workflows/build/aip_predump_values.py: has mixed line endings: CRLF=560, LF=2, lone_CR=0
  - Assets/Python/Contrib/Sevopedia/SevoPediaLeaderAIPValues.py: has mixed line endings: CRLF=485, LF=1, lone_CR=0
  ```

The LF-only lines are just blank lines:

```log
.github/workflows/build/aip_predump_values.py
  LF-only line 393
  LF-only line 440

Assets/Python/Contrib/Sevopedia/SevoPediaLeaderAIPValues.py
  LF-only line 334
```

But it passed on [GitHub Actions test](https://github.com/wonderingabout/AdvCiv-SAS/pull/29/commits/c0e4c7624edafb022946267c606692071784dbf0): GitHub Actions checks its own post-checkout bytes, so local/exported mixed line endings can still need cleanup even when CI passes. When this happens in a Git checkout, the script notes failing worktree files that are already clean in the Git index. To make this clearer, the checker now prints its scan mode and can note when a failing worktree file is already clean in the Git index.

### `build/assets_dlls.py`

Recursively verifies `Assets` contains only the two expected DLL files (`Assets/CvGameCoreDLL.dll` for the main/48-civ DLL and `Assets/CvGameCoreDLL_18_civs_DLL.dll` for the 18-civ DLL), and verifies the 18-civ DLL is not larger than the main DLL by byte size.

### `build/art_button_paths.py`

Verifies local `.dds` button/image paths under `Assets/Art` contain no whitespace because a path that works through a direct Civ4 button or atlas reference can fail when reused in `<img>` markup. Paths beneath any directory named `nif` are excluded because model texture paths can be embedded in NIF files and are not safely renamed through ordinary XML changes. Base-game art paths referenced from XML are also outside this local-asset check. See [KI#118](/_1_AdvCiv-SAS/Docs/README_Known_Issues.md#ki-118), including the former `Buildings/Natya_Shastra/indian sreni.dds` example.

### `build/markdown_images.py`

Verifies every repository-local image referenced by a Markdown file resolves to an existing file. It checks normal and reference-style Markdown images plus HTML `<img src="...">` tags, while ignoring external URLs and fenced-code, inline-code, or comment examples. Repository-root paths and paths relative to each Markdown file are both supported.

This prevents documentation image links from silently breaking when image folders or files are moved or renamed.

### `build/markdown_links.py`

Verifies every repository-local file or directory referenced by a Markdown link resolves to an existing repository path, and verifies `#heading` fragments for Markdown targets against GitHub-style heading anchors and explicit HTML anchors.

It checks normal and explicit reference-style Markdown links plus HTML `<a href="...">` tags, while ignoring external URLs and fenced-code, inline-code, or comment examples. Image references are handled separately by `markdown_images.py`. Repository-root paths and paths relative to each Markdown file are both supported.

It also verifies the continuous Known Issues ledger and its standardized `KI#number` schema. Every integer through the highest body entry or stable `F### / provisional KI#number` assignment in the C++ File Audit Album must have an explicit Fixed, Pending, Merged, Rejected or Retired entry; the menu and body identifier sets must match without duplicates; and every entry uses a stable short `#ki-number` anchor. This catches both undiscoverable body entries such as the previously omitted KI#202 and wholly absent provisional/disposition entries such as KI#501.

When `_LLM_REPO_FILE_MANIFEST.txt` is present in a light-source ZIP, tracked files and directories intentionally omitted from the bundle are still recognized as existing repository paths; Markdown fragments are checked whenever the target Markdown file itself is bundled. This prevents both moved/renamed repository targets and renamed Markdown headings from silently leaving stale local links.

### `build/sas_text_references.py`

Verifies every `TXT_KEY` defined across all SAS-owned GameText files (`Assets/XML/Text/AdvCiv-SAS*.xml`) is referenced by active mod XML, BUG configuration, Python, map-script Python, or C++. It also verifies that buildings, units, civilizations, and leaders with SAS-owned descriptions reference Civilopedia keys defined in the mod's GameText files, unless the pedia text is explicitly confirmed as inherited from the base game or expansion. Other explicit exceptions cover confirmed inherited base references, empirically confirmed EXE runtime references with no static reference, and BUG option-key families constructed dynamically at runtime. Every passing run prints the exact keys in each exception group so accepted uncertainty stays visible in CI. The two sea-level recommendation labels are runtime exceptions because changing the High label changed its Custom Game list text in-game, and Low uses the matching EXE convention. Inherited AdvCiv/BUG/BULL text remains outside the blocking unused-key check because a full static scan produces hundreds of uncertain legacy and dynamic candidates; the broader manual [`audit_unused_text_keys.py`](/LLM_Helpers/README.md#audit_unused_text_keyspy) remains available for optional review audits.

### `build/global_defines_nonempty.py`

Verifies GlobalDefines XML files under `Assets/XML` have `<Define>` entries with non-empty `<DefineName>` values, unique define names within each file, and exactly one non-empty value field (normally `iDefineIntVal`, `fDefineFloatVal`, or `DefineTextVal`). This preventively catches accidental blank define values. We check this because we suspect they may possibly cause unclear errors; for intentional "empty" string-like behavior, explicit values such as `NONE` are clearer and more reliable.

### `build/version_metadata.py`

Verifies the tracked `Assets/SASModVersion.txt` archive marker retains its Git `export-subst` version-anchor describe, commit, and commit-date placeholders, and that `.gitattributes` still marks the file `export-subst`. The version placeholder uses one immutable `SAS_VERSION_ANCHOR_<exact-commit-count>` tag: Git's describe distance is the count of commits in `anchor..target`, so AdvCiv-SAS can reconstruct the same practical commit-count version in ordinary GitHub/git-archive downloads without a per-commit VERSION file. The anchor is one-time repository infrastructure and must not be moved or renamed after creation.

### `build/launch_guard.py`

Verifies `SASDefineGuard.py` exists and its int/string sentinel constants match `GlobalDefines_advciv_sas.xml`.

### `build/define_int_bounds.py`

Verifies `GlobalDefines_advciv_sas.xml` integer values stay within `[-100000, 100000]`, except specific intentional outliers such as the launch-guard sentinel checked by [`build/launch_guard.py`](#buildlaunch_guardpy) and the high bonus-Fallout scrub worker value.

### `build/turn_define_gamespeed.py`

Verifies turn-valued integer `SAS_*` defines make their game-speed semantics explicit in the define name. `TURN(S)_NORMAL_GAMESPEED` means the XML value is expressed at Normal speed and code must scale it with the appropriate game-speed percentage; `TURN(S)_UNSCALED_GAMESPEED` means literal game/path turns are intentionally kept unchanged across game speeds. The check ignores non-duration uses such as `SAS_DO_TURN_*` function-scope names, `...PER_TURN` rates, and `...PATH_TURN_VALUE...` score weights. It also recognizes legacy-looking maximum countdown, minimum/maximum age, and log-interval names so those cannot silently evade the convention.

This naming rule was added after auditing turn gates while implementing non-combat food-production allocation: the new production gate correctly needed `TrainPercent` scaling, nearby UWAI contact limits correctly represented unscaled movement/path turns, and the victory-denial countdown windows were found to need `VictoryDelayPercent` scaling.
The checker intentionally validates naming/declared intent rather than trying to infer the correct scaling formula from C++: code review still decides whether a Normal-speed value should use `TrainPercent`, `ConstructPercent`, `VictoryDelayPercent`, or another game-speed field. The explicit names make that audit grep-friendly and prevent ambiguous raw turn gates from being added silently.

### `build/xml_comments.py`

Verifies XML comments under `Assets/XML` do not contain illegal double hyphen `--` and are not left unclosed.

### `build/xml_element_only_content.py`

Verifies non-GameText XML under `Assets/XML` parses and contains no stray text between child elements. This catches XML that is well-formed but rejected by Civ4's schema loader, such as a leftover wrapped-comment line after a self-closing BuildingInfo field or a Python-style `#` before an XML comment. GameText is excluded because its language elements intentionally contain text content and inherited files can use legacy encodings.

### `build/xml_suspicious_angle_tags.py`

Verifies raw XML files under `Assets/XML` do not contain suspicious malformed-looking tag punctuation such as doubled opening angles or extra closing angles after tags, catching valid-but-wrong text like `<French>>...`. This helped [spot](https://github.com/wonderingabout/AdvCiv-SAS/actions/runs/27342682844/job/80783239087) and fix suspicious malformed-looking XML tag punctuation. See [KI#151](/_1_AdvCiv-SAS/Docs/README_Known_Issues.md#ki-151).

### `build/xml_suspicious_text_chars.py`

Verifies XML text does not contain high-confidence corrupted characters such as `?` inside a word-like token, Unicode replacement characters in active/non-ignored text, common mojibake fragments, raw control characters, or web tracking query parameters such as `?utm_source=...` in active text; inherited non-English replacement-character noise is hidden by default and can be listed with `--show-ignored`. XML comments are ignored, allowing source/reference notes to retain their original URLs. This intentionally does not enforce broader typography policy such as em dashes, curly quotes, or accented letters. This helped [spot](https://github.com/wonderingabout/AdvCiv-SAS/actions/runs/27400470104/job/80977135402) and fix the corresponding errors. See [KI#152](/_1_AdvCiv-SAS/Docs/README_Known_Issues.md#ki-152).

### `build/xml_civ4_text_characters.py`

Verifies active GameText values do not contain Unicode typography that Civ4 renders as visible artifacts, including em/en dashes, curly quotes, ellipsis characters, and non-breaking spaces. It also catches repeated question marks that indicate lost characters. XML comments are ignored, while readable accented letters remain allowed.

### `build/xml_sas_text_english.py`

Verifies AdvCiv-SAS-owned GameText XML files (`Assets/XML/Text/AdvCiv-SAS*.xml`) only use `<English>` language entries, except a narrow whitelist for inherited/renamed diplomacy text entries that intentionally keep old non-English fields.

### `build/xml_text_duplicate_tags.py`

Verifies each GameText `TXT_KEY`-style `<TEXT>/<Tag>` entry under `Assets/XML/Text` is defined only once, with source line numbers for duplicate definitions. Inspired by the old AdvCiv-SAS blind [scan_xml_duplicates-3.3.py](/LLM_Helpers/scan_xml_duplicates-3.3.py) (see [Legacy XML duplicate discovery scanner (``scan_xml_duplicates-3.3.py``)](/LLM_Helpers/README.md#legacy-xml-duplicate-discovery-scanner-scan_xml_duplicates-33py)). This helped [spot](https://github.com/wonderingabout/AdvCiv-SAS/actions/runs/27332010214/job/80746523894) and fix XML text duplicate errors. See [KI#149](/_1_AdvCiv-SAS/Docs/README_Known_Issues.md#ki-149).

### `build/xml_parent_duplicate_keys.py`

Verifies parent-style XML objects are not defined twice with the same key, such as duplicate `*Info` `<Type>` values or duplicate audio `ScriptID` / `SoundID` entries. This intentionally does not check child/list duplicate semantics, which can be more context-dependent.

### `build/xml_child_duplicates.py`

Checks suspicious duplicate child/list XML entries inside the same parent object, failing on high-confidence duplicates such as duplicate flavors, leader memory/contact keys, civilization city names, and unit/promotion lists; reports lower-confidence duplicates such as exact duplicate event trigger `Text + Era` entries without failing, prints source line numbers, and hides known allowed/noisy duplicates such as weighted goody huts and reused world-picker art paths unless run with `--show-ignored`. This helped [spot](https://github.com/wonderingabout/AdvCiv-SAS/actions/runs/27336768909/job/80762873956) and fix XML text duplicate errors. See [KI#150](/_1_AdvCiv-SAS/Docs/README_Known_Issues.md#ki-150).

### `build/xml_redundant_defaults.py`

Verifies a small set of XML representations that are known to be semantically redundant in Civ4/AdvCiv-SAS: explicit zero-valued entries inside `Flavors`, where an omitted flavor already evaluates to 0, and non-empty `TechTypes` lists containing only `NONE`/`NO_TECH`, where an empty `TechTypes` list has the same effective meaning. This was added after the flavor audit found both patterns in inherited/stale XML (see [KI#22.7](/_1_AdvCiv-SAS/Docs/README_Known_Issues.md#ki-22.7)).

The check is deliberately conservative rather than a generic "zero is bad" rule. Many scalar zero fields are mandatory or meaningful defaults, and some padded list formats may depend on loader/schema behavior. Add another pattern only after confirming that the shorter representation is accepted and semantically identical.

### `build/tech_columns.py`

Verifies all techs in the same tech-tree column (`iGridX`) share the same core column values, currently `iCost` and `iAsset`; different `iGrid` column rows in the same column are allowed, but same-timing techs should normally have identical research cost and asset/trade valuation.

### `build/wonder_cost_columns.py`

Verifies normally constructible world wonders unlocked in the same technology-cost tier have the same production cost and that world-wonder costs increase in later, more expensive technology tiers. Multiple visual columns may intentionally share a timing tier when their technologies have the same research cost. Great Person-founded shrines/corporation headquarters (`iCost=-1`) are excluded, as is the deliberately expensive United Nations diplomatic-victory wonder.

### `build/wonder_culture_gpp_columns.py`

Verifies normally constructible world wonders follow an era-relative flat-culture progression: 4 in the first two Ancient-era columns, 6 through the rest of the Ancient and Classical eras, and 8 through the Medieval and Renaissance eras. From the start of the Industrial era, culture-advisor wonders without a culture percentage modifier have 10 flat culture and other wonders have none; a wonder with a local or global culture modifier has no flat culture in every era.

World-wonder Great Person point rates (`iGreatPeopleRateChange`) must match within each column and never decrease in later populated columns; GP-producing national wonders and ordinary buildings cannot exceed the latest world-wonder rate available by their direct or shared special-building prerequisite tech column. Great Person-founded shrines/corporation headquarters (`iCost=-1`) are excluded.

### `build/asset_tech_prereq_redundancy.py`

Flags stale additional unit/building tech requirements when the primary tech already guarantees them through every valid prerequisite path. To keep nearby foundational requirements explicit when useful, it applies only when the additional tech is from an earlier era and the primary tech is at least two columns after the start of its own era. Era starts and tech-graph guarantees are derived from current XML, so moving or collapsing tech-tree columns updates the check automatically.

### `build/civ_specific_assets.py`

Verifies that civilization and leader references resolve to existing leader, art, trait, civic, and religion entries, and that each civilization's building and unit replacement class matches the class declared by the referenced asset. It also verifies civ-specific building and unit assets do not require a later starting tech-tree column (`iGridX`) or an earlier obsolete tech-tree column than the generic asset they replace; same-column parallel tech variation is allowed for flavor. For example, this helped [spot](https://github.com/wonderingabout/AdvCiv-SAS/actions/runs/27461191223/job/81175197986) and fix `BUILDING_AZTEC_SACRIFICIAL_ALTAR: building class BUILDINGCLASS_MONUMENT replacement uses later tech column than default BUILDING_MONUMENT: TECH_CALENDAR (GridX=2, GridY=11) after TECH_MYSTICISM (GridX=1, GridY=11)`, but `BUILDING_JAPAN_DOUJOU` (JAIL) requiring `TECH_MEDITATION` parallel (`iGridY=9`) to `TECH_CONSTITUTION` required by the generic `BUILDING_JAIL` is fine so it does not need a fix. Similarly for `BUILDING_EGYPTIAN_OBELISK: building class BUILDINGCLASS_MONUMENT replacement uses earlier ObsoleteTech column than default BUILDING_MONUMENT: TECH_BROADER_EDUCATION (GridX=10, GridY=11) before TECH_LIBERALISM (GridX=13, GridY=11)` being obsolete sooner and so weaker than the generic monument so fixed it (obsolete is now same `GridY` or later).

### `build/civ_specific_not_weaker.py`

Verifies civ-specific unit/building replacements are not weaker than their generic class defaults. Unit replacements fail if their `iCombat` or `iPower` is lower than the default unit for the same `UnitClass`; building replacements fail if their `iPower` is lower than the default building for the same `BuildingClass`.

### `build/define_tag_refs.py`

Verifies any SAS `DefineTextVal` token that looks like a known Civ4 XML tag (e.g. bonuses, techs, building classes, units, promotions) references a real XML `<Type>` value; successful matches are printed by default, and `--quiet` suppresses them.

### `build/raw_getinfotype.py`

Verifies runtime Python files do not add raw `getInfoTypeForString(...)` or `CvUtil.findInfoTypeNum(...)` lookups outside strict helper implementations and narrow legacy/dynamic exceptions. Use `getInfoTypeOrFail(...)` or `findInfoTypeNumOrFail(...)` for literal/static XML tag lookups so missing or renamed tags fail loudly. Python comments and string literals are ignored, so code-generation strings such as those in `savemap.py` are not flagged by this check. This allowed to [find](https://github.com/wonderingabout/AdvCiv-SAS/actions/runs/27464773794/job/81185039717) this alternative lookup and that it needed an or fail handling too: now fixed.

### `build/tech_audio.py`

Verifies every technology's normal and multiplayer audio reference resolves through the mod-local `Audio2DScripts.xml` and `AudioDefines.xml` tables to a non-empty filename, that each technology keeps a distinct spoken recording, and that no technology-specific `AS2D_TECH_*` script remains orphaned after a technology is removed or renamed. `AS2D_TECH_GENERIC` is the intentional reusable exception. Mod-local resolution matters because missing entries in these replacement audio tables are not inherited from base Civ4; this check would catch the missing Drama entries, malformed Communism multiplayer script ID, wrong-layer Aesthetics multiplayer reference, and stale removed-tech scripts found during a technology rework.

### `build/leader_audio.py`

Verifies every non-empty leader diplomacy-audio reference resolves through the mod-local `Audio2DScripts.xml` and `AudioDefines.xml` tables to a non-empty filename. This catches per-era mapping typos such as Stalin's former Renaissance `AS2D_DIPLO_STALIN_EARLY_INTROO` reference instead of silently losing the intended diplomacy intro. See [KI#1031](/_1_AdvCiv-SAS/Docs/README_Known_Issues.md#ki-1031).

### `build/opening_music.py`

Verifies main-menu opening music has a valid `Audio2DScripts.xml` trigger/fixed script, and if shuffle is enabled, at least one valid non-`NONE` shuffle script.

### `build/diagnostic_log_safety.py`

Verifies the BBAI and SASGameRecord final emission paths use the shared `logSASDiagnosticLiteralLine` wrapper instead of passing an already-formatted dynamic line directly to `gDLL->logMsg`. Civ4's EXE logger can interpret `%` in that message as printf syntax again; the wrapper doubles every remaining literal percent only at the final boundary.

The check also guards the helper's percent-doubling implementation and carries small executable-specification examples such as `75%` -> `75%%`. Producer format strings still use ordinary printf escaping (`%%` for one intended `%`). This check is intentionally scoped to the two SAS diagnostic sinks implicated by [KI#375.3](/_1_AdvCiv-SAS/Docs/README_Known_Issues.md#ki-375.3), not every inherited `gDLL->logMsg` caller in the DLL.

### `build/bbai_log.py`

Verifies BBAI logging is disabled by default in `Assets/XML/GlobalDefines_advciv_sas.xml`. Every integer `SAS_BBAI_*` define is discovered automatically and required to be `0`, so dedicated categories such as Citizen, Culture, Evacuation, Worker, and Worker-sea logging cannot be omitted from the check when categories are added. The only explicit nonzero exceptions are `SAS_BBAI_SCORE_LOG_INTERVAL_TURNS_UNSCALED_GAMESPEED=100` and `SAS_BBAI_LOG_USE_TIMESTAMPED_FILENAME=1`; these configure how enabled logging behaves but do not enable logging themselves.

### `build/sas_game_record_log.py`

Verifies the independent `SASGameRecord` report is disabled by default in `Assets/XML/GlobalDefines_advciv_sas.xml`. `SAS_GAME_RECORD_LOG_LEVEL` must stay `0`; the snapshot interval, timestamped filenames, default-on anonymous performance metrics, and default level-2 display/runtime context are also checked because they configure how enabled records behave but do not enable record logging themselves. It also checks that the public `SAS_GAME_RECORD_REVISION` matches the newest contiguous entry in the maintained revision history, that the history's current emitted `recordRevision` example is not stale, and that the emitted `GAME_RECORD_SOURCE_CONTEXT recordRevision` field remains wired to that constant.

The same check keeps readable AI-strategy diagnostics synchronized with `AIStrategies.h`: the enum must remain the contiguous power-of-two bitfield used by the recorder's shift scans, every `AIStrategy` value must map to its identical canonical raw token in `getSASAIStrategyType`, and the complete CORE/snapshot/transition scans must still reach the enum's current final strategy. This makes a future `AI_STRATEGY_*` addition or incompatible bit-layout change fail CI instead of silently disappearing from `GAME_RECORD_AI_STRATEGIES` or transition history.

The check also guards exact AreaAI and AI target-city provenance. AreaAI raw-name coverage and its normal/UWAI writer bridges must remain synchronized; target-city checks require every current setter writer plus the city-removal effective-clear path to remain covered by exact transitions and the periodic `GAME_RECORD_AI_TARGET_CITIES` checkpoint. A new target writer therefore fails CI instead of silently bypassing target history.

The check also keeps level-3 `GAME_RECORD_DIPLO_ATTITUDE_BREAKDOWN` synchronized with `CvPlayerAI::AI_updateAttitude`: the recorder component labels/order are validated against the current additive attitude getter sequence, including aggregate memories and the final partial-sum-dependent war modifier. If attitude arithmetic changes later, CI requires the diagnostic breakdown to be reviewed in the same change instead of silently becoming incomplete.

### `build/fonts.py`

Verifies shared UI font defaults are `1 / 2 / 3 / 4 / 3`.

### `build/detail_manager.py`

Verifies `Assets/XML/Misc/CIV4DetailManager.xml` `CITYBILLBOARD_SCALE` keys match the committed shared UI font/default billboard-scale defines and the base values used by `Assets/Python/SASBillboardScale.py`, catching stale rewritten billboard scales after temporary UI upscaling.

### `build/aip.py`

Verifies the Sevopedia Leader AI Personality Panel is enabled, uses the predumped cache by default, does not dump recomputed cache data to the log by default, and has a non-empty predumped cache file.

### `build/aip_predump_values.py`

Validates effective AIP predump values outside Civ4. It compares committed entries in `Assets/Python/Contrib/Sevopedia/SevoPediaLeaderCachePredumped.py` against values reconstructed from `Assets/XML/Civilizations/CIV4LeaderHeadInfos.xml`, mirroring the narrow DLL path needed for these values: `LEADER_DEFAULTS` copy behavior, `CvLeaderHeadInfo::GetChildXmlValByName` missing-tag defaults, primitive array/list loading, and `UWAI::applyPersonalityWeight` with `UWAI_PERSONALITY_PERCENT`.

This script is intentionally separate from [`build/aip.py`](#buildaippy). `aip.py` remains the lightweight release-safety check, while this deeper value mirror checks the committed predump against effective XML+DLL-style values. It currently checks full cached tuples for direct scalar getter keys, scalar attitude-threshold getters, flavors, no-war attitude probabilities, contact aggregate values, and displayed positive/negative memory aggregate values. Contact and memory aggregate formulas are shared with the in-game AIP helper code; unit AI modifiers and improvement modifiers are not checked because they are not currently displayed/predumped. The script fails by default when mismatches or missing/unparsed entries are found; use `--allow-mismatch` only for exploratory/debug runs such as `--no-uwai`.

Practical predump workflow:

1. The normal checker run reconstructs the expected AIP cache outside Civ4 and compares it with the committed `SevoPediaLeaderCachePredumped.py`. CI runs this automatically on commits/PRs, so a failure usually means the committed predump is stale relative to leader XML or shared AIP display logic.
2. The `Refresh AIP predump cache` workflow can refresh the predump from GitHub. It runs automatically after pushes to known AIP dependency files and can also be started manually from the Actions tab. When the generated file changes, it opens or updates a bot PR containing only `SevoPediaLeaderCachePredumped.py`.
3. To refresh locally without launching Civ4, run `python .github/workflows/build/aip_predump_values.py --write`, review the diff, then rerun the checker.
4. The old manual fallback still works: temporarily set `SAS_SEVOPEDIA_LEADER_AI_PERSONALITY_CACHE_USE_PREDUMPED = 0` and `SAS_SEVOPEDIA_LEADER_AI_PERSONALITY_CACHE_DUMP_TO_LOG = 1`, open the Leaders / AI Personality panel once, copy the generated `PythonDbg.log` block into `SevoPediaLeaderCachePredumped.py`, restore the defaults, and rerun this checker.

The generated file intentionally has no timestamp. This keeps no-op `--write` runs byte-identical when cache data is unchanged, avoiding false diffs and future bot/maintenance churn.

This full-tuple check is intentionally stricter than a raw-value-only comparison. During development, the earlier numeric-only check passed, but the tuple check found 394 stale predump entries after shared/runtime AIP label metadata had changed. The underlying values were mostly correct, but many committed labels were missing updated `%` text, such as `Build Unit (52)` instead of `Build Unit % (52)`. Refreshing `SevoPediaLeaderCachePredumped.py` fixed those mismatches and confirmed that this check catches display-cache drift, not just numeric-value drift.

Example local commands (Git Bash):

```bash
cd "/c/Program Files (x86)/Steam/steamapps/common/Sid Meier's Civilization IV Beyond the Sword/Beyond the Sword/Mods/AdvCiv-SAS" && python .github/workflows/build/aip_predump_values.py
cd "/c/Program Files (x86)/Steam/steamapps/common/Sid Meier's Civilization IV Beyond the Sword/Beyond the Sword/Mods/AdvCiv-SAS" && python .github/workflows/build/aip_predump_values.py --write
```

### `build/worldsizes.py`

Verifies hardcoded C++/Python `WorldSizeTypes` enum exposure matches the `Assets/XML/GameInfo/CIV4WorldInfo.xml` order, and that old `SAS_MAGIC_WORLDSIZE_*` Python runtime references have not returned.

### `build/mapscripts.py`

Verifies every playable `PrivateMaps/*.py` script is listed exactly once across the SAS map-script heaviness/coverage defines, and every listed map script exists in `PrivateMaps/`. `SAS_MAP_SCRIPT_NAMES_HEAVINESS_UNSPECIFIED` is coverage-only and is not read by the DLL; it documents map scripts intentionally left outside the explicit land-heavy / almost-all-land / naval-heavy override lists.

### `build/python24_compile.py`

Compile-checks runtime Civ4 Python files under `Assets/Python` and `PrivateMaps` with CPython 2.4 through [`python24-compile.yml`](/.github/workflows/python24-compile.yml). It uses `py_compile` on each source file but redirects bytecode to a temporary directory so the workflow catches real Python 2.4 parser/bytecode errors without writing `.pyc` files into the mounted repository. This check complements Ruff: Ruff gives modern static diagnostics, while this workflow confirms the old parser still accepts the files Civ4 can load.

## Run Locally

Note for LLMs/Users: this script can also be run locally, for example on Git Bash:

```bash
py "C:\Program Files (x86)\Steam\steamapps\common\Sid Meier's Civilization IV Beyond the Sword\Beyond the Sword\Mods\AdvCiv-SAS\.github\workflows\build\bbai_log.py"
```

## Not currently automated

The Python UI markup-escaping rule from `AGENTS.md` is intentionally not checked here yet. Raw dynamic/user text that reaches font-tagged UI renderers (`screen.setText`, `screen.setLabel`, `SASTextScale.labelText`, etc.) should keep raw text in state and escape only the display string (`&`, `<`, `>`), but broad static scanning would produce too many false positives in normal Python code, comparisons, comments, already-escaped text, and non-user-facing strings.

Only add a build check for this if it can be made targeted to a known risky path, such as a specific Sevopedia search-label rendering function or another function that displays raw user-entered text.

## Dependabot

[`.github/dependabot.yml`](/.github/dependabot.yml) enables Dependabot version updates for GitHub Actions. It is currently scheduled daily for initial testing so outdated action versions such as `actions/checkout` and `actions/setup-python` can produce an update PR quickly. If this becomes noisy, change the schedule to weekly or monthly.

For example, as of 2026-06-09, this was added to help address the following GitHub Actions maintenance warning, after which Dependabot was added to track action-version updates automatically:

```text
build-checks
Node.js 20 actions are deprecated. The following actions are running on Node.js 20 and may not work as expected: actions/checkout@v4, actions/setup-python@v5. Actions will be forced to run with Node.js 24 by default starting June 16th, 2026. Node.js 20 will be removed from the runner on September 16th, 2026. Please check if updated versions of these actions are available that support Node.js 24. To opt into Node.js 24 now, set the FORCE_JAVASCRIPT_ACTIONS_TO_NODE24=true environment variable on the runner or in your workflow file. Once Node.js 24 becomes the default, you can temporarily opt out by setting ACTIONS_ALLOW_USE_UNSECURE_NODE_VERSION=true. For more information see: https://github.blog/changelog/2025-09-19-deprecation-of-node-20-on-github-actions-runners/
```

Which it fixed here: [dependabot@PR#24](https://github.com/wonderingabout/AdvCiv-SAS/pull/24).
