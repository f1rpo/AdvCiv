#!/usr/bin/env python3
# AI, UI, logging, or other modifications first developed in AdvCiv-SAS (Simple Advanced Strategy)
# (c) 2026 wonderingabout & AI/LLM helpers (see Authors in AdvCiv-SAS's root README.md)

# <!-- custom: argparse/json read the same explicit workflow event boundaries as the SASGameRecord check; pathlib locates source/history, re recognizes labels, and subprocess inspects Git diffs. (GPT-6.1-Sol) -->
import argparse
import json
from pathlib import Path
import re
import subprocess
# <!-- custom: Reuse the established push/PR/default-branch range selection and Git readers so runtime revisions follow the same per-commit boundaries as SASGameRecord. (GPT-6.1-Sol) -->
from sas_revision_history import ROOT, changed_paths, first_parent, git_show, progression_commits, resolve_default_branch
# <!-- custom: SASGameRecord's shared-helper path setup also exposes the common entry parser; both histories require real Change text while retaining their distinct bump policies. (GPT-6.1-Sol) -->
from revision_history import change_errors, revision_entries

SOURCE = "CvGameCoreDLL/SASRuntimeStatus.cpp"
HISTORY = "_1_AdvCiv-SAS/Docs/revisions/SASRuntime.md"
DEDICATED = {SOURCE, "CvGameCoreDLL/SASRuntimeStatusReasons.h"}
MARKER = re.compile(r"SASRuntime|SAS_RUNTIME|SASDllRuntimeIdentity")


def revision(source):
    values = re.findall(r"\bSAS_RUNTIME_REVISION\s*=\s*(\d+)\b", source)
    return int(values[0]) if len(values) == 1 else None


def history_errors(source, history):
    value = revision(source)
    entries = [number for number, _body in revision_entries(history)]
    if value is None or value < 1:
        return ["missing positive compiled SAS_RUNTIME_REVISION"]
    if entries != list(range(value, 0, -1)):
        return [f"runtime history must contain revisions {value} through 1 exactly once, latest first"]
    return change_errors(history)


def read(repo, commit, path):
    if commit is None:
        file = repo / path
        return file.read_text(encoding="utf-8") if file.is_file() else ""
    data = git_show(repo, commit, path)
    return "" if data is None else data.decode("utf-8", "replace")


# <!-- custom: Dedicated recorder edits always qualify, including implementation comments; shared source/config changes qualify through changed runtime symbols.
# Pure CI, test and documentation maintenance does not change the loaded recorder, so it must not force a compiled revision change and rebuild; unusual shared-code refactors still need manual review. (GPT-6.1-Sol + ChatGPT (review)) -->
def qualifies(repo, parent, commit):
    paths = changed_paths(repo, parent, commit) if commit is not None else subprocess.check_output(["git", "diff", "--name-only", "HEAD", "--"], cwd=repo, text=True).splitlines()
    paths = set(paths)
    if commit is None:
        # <!-- custom: Git's diff handles line-ending normalization; comparing decoded blobs with text-mode working files falsely classified clean CRLF checkouts as implementation edits.
        # Add untracked paths explicitly so new draft source files are still checked before staging. (GPT-6.1-Sol) -->
        paths.update(subprocess.check_output(["git", "ls-files", "--others", "--exclude-standard"], cwd=repo, text=True).splitlines())
    if paths & DEDICATED:
        return True
    for path in paths:
        if path.startswith(("tests/", ".github/")) or Path(path).suffix.lower() not in {".cpp", ".h", ".py", ".xml"}:
            continue
        args = ["git", "diff", "--unified=0", parent]
        if commit is not None:
            args.append(commit)
        diff = subprocess.check_output(args + ["--", path], cwd=repo, text=True, encoding="utf-8", errors="replace")
        if commit is None and git_show(repo, parent, path) is None:
            diff = "\n".join("+" + line for line in read(repo, None, path).splitlines())
        if any(MARKER.search(line[1:]) for line in diff.splitlines() if line.startswith(("+", "-")) and not line.startswith(("+++", "---"))):
            return True
    return False


def transition_errors(repo, parent, commit):
    source = read(repo, commit, SOURCE)
    previous_source = read(repo, parent, SOURCE) if parent else ""
    if not source and not previous_source:
        return []
    errors = history_errors(source, read(repo, commit, HISTORY))
    previous = revision(previous_source) if previous_source else 0
    current = revision(source)
    if previous is None or current is None:
        return errors + ["cannot resolve runtime revision progression"]
    expected = previous + int(qualifies(repo, parent, commit)) if parent else 1
    if current != expected:
        errors.append(f"runtime revision {previous}->{current}: expected {expected} (exactly +1 for implementation changes, otherwise unchanged)")
    return errors


def check(repo, default_branch_ref=None, event_name="local", event_data=None):
    _, base = resolve_default_branch(repo, default_branch_ref)
    errors = []
    for commit in progression_commits(repo, base, event_name, event_data):
        errors.extend(f"commit {commit[:12]}: {error}" for error in transition_errors(repo, first_parent(repo, commit), commit))
    # <!-- custom: Check the draft separately without treating its pending revision as another committed increment; committed progression still checks each commit in the selected range. (GPT-6.1-Sol) -->
    errors.extend(f"working tree: {error}" for error in transition_errors(repo, "HEAD", None))
    return errors


def main():
    parser = argparse.ArgumentParser(description="Check runtime recorder revision/history and exactly one increment per implementation commit.")
    parser.add_argument("--repo-root", type=Path, default=ROOT)
    parser.add_argument("--default-branch-ref")
    parser.add_argument("--event-name", choices=("local", "push", "pull_request", "workflow_dispatch"), default="local")
    parser.add_argument("--event-path", type=Path)
    args = parser.parse_args()
    event = None
    if args.event_name in ("push", "pull_request"):
        if args.event_path is None:
            parser.error("push/PR checks require --event-path")
        event = json.loads(args.event_path.read_text(encoding="utf-8"))
        if not isinstance(event, dict):
            parser.error("revision-check event must be a JSON object")
    try:
        errors = check(args.repo_root, args.default_branch_ref, args.event_name, event)
    except (ValueError, OSError, subprocess.CalledProcessError) as error:
        errors = [str(error)]
    print("FAIL runtime recorder revisions" if errors else "PASS runtime recorder revisions")
    for error in errors:
        print("  - " + error)
    return int(bool(errors))


if __name__ == "__main__":
    raise SystemExit(main())
