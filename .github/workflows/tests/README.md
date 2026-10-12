# Workflow helper tests

These Python regression tests verify the workflow/tooling helpers themselves. `checks-tests.yml` runs the full suite automatically on pushes and pull requests; the same suite is also useful for focused local validation.

Many tests use small deterministic fixtures in temporary directories. A fixture is a deliberately constructed example that should either pass or fail a helper in a known way; it does not randomly alter or sample the mod's real gameplay XML. Other tests exercise the current repository where that is the behavior being protected.

Tests are named for the helper or area they exercise instead of being collected in a generic backlog file.

Run the full suite locally with:

```bash
python -m unittest discover -s .github/workflows/tests -v
```

For a focused run, select one test file, for example:

```bash
python -m unittest discover -s .github/workflows/tests -p test_repository_hygiene.py -v
```
