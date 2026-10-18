# Workflow helper tests

Local platform-specific game-code tests live separately under [`tests/`](/tests/README.md).

These Python regression tests verify the workflow/tooling helpers themselves. `checks-tests.yml` runs the full suite automatically on pushes and pull requests; the same suite is also useful for focused local validation.

Many tests use small deterministic fixtures in temporary directories. A fixture is a deliberately constructed example that should either pass or fail a helper in a known way; it does not randomly alter or sample the mod's real gameplay XML. Other tests exercise the current repository where that is the behavior being protected.

Tests are named for the helper or area they exercise and grouped under the same subject folders as `build/`:

| Folder | Subject |
| --- | --- |
| `assets/` | Asset structure |
| `diagnostics/` | Recorder revisions/history |
| `documentation/` | Markdown, generated documents and long-comment references |
| `hygiene/` | Repository text/path hygiene |
| `repository/` | DLL shipping and commit-diff packaging |
| `source/` | Source conventions and related manual source-editing helpers |
| `xml/` | Define usage and asset technology rules |

`checks-tests.yml` keeps one workflow with a named step per populated subject. No empty folder is needed for categories without tests.

Run the full suite locally with:

```bash
python -m unittest discover -s .github/workflows/tests -t .github/workflows -v
```

For a focused run, select one test file, for example:

```bash
python -m unittest discover -s .github/workflows/tests/hygiene -t .github/workflows -p test_repository_hygiene.py -v
```

Keep `-t .github/workflows` in both commands so subjects import as `tests.xml`, for example, instead of conflicting with Python's standard-library `xml` package.
