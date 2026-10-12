# Automated check helpers

Everything under this directory is used by automated GitHub Actions. Subject folders match the corresponding `checks-<subject>.yml` workflow wherever there is a category workflow.

- `repository/`: shipped-file layout, DLL-update intent, temporary-file policy and version metadata.
- `hygiene/`: BOM/encoding signatures, line endings, filename lengths and Windows installation-path limits.
- `documentation/`: maintained Markdown links/images, long-comment references, menu/prose structure and generated-document freshness.
- `xml/`: XML structure/text/defaults, SAS define/text references, technology/resource timing, wonder-policy tables and civilization replacement checks.
- `assets/`: ArtDefine/button references, audio and shared font configuration.
- `diagnostics/`: BBAI/SASGameRecord disabled defaults, log-sink safety and SASGameRecord revision/history synchronization.
- `runtime/`: AIP defaults/predumps, Detail Manager configuration, world-size alignment and map-script classification.
- `source/`: changed-source provenance/rationale checks and fail-fast XML identity lookup policy.
- `python/`: the helper used by the specialized real CPython 2.4 compile workflow.

`checks-source.yml` also runs `LLM_Helpers/reflow_custom_comments.py --check`, and `checks-tests.yml` runs the helper regression suite under `../tests/`. Markdown lint, Ruff, Python 2.4 compilation and AIP predump refresh remain specialized workflows.

See the [workflow README](../README.md) for individual descriptions, workflow names and local commands.
