#!/usr/bin/env python3
# AI, UI, logging, or other modifications first developed in AdvCiv-SAS (Simple Advanced Strategy)
# (c) 2026 wonderingabout & AI/LLM helpers (see Authors in AdvCiv-SAS's root README.md)
# <!-- custom: Python can call a newly registered method while the distributed DLL still lacks it; SASGameRecord's rev154's unconditional logging bridges made this an installation failure rather than merely stale native behavior.
# Compare literal Boost.Python .def names/targets at default-branch integration and require a changed Release DLL whose build manifest includes the affected registration sources.
# This is a focused registration guard, not a C++ parser or proof of every Python call's compatibility. (GPT-6.1-Sol) -->

import argparse
import hashlib
import json
import os
from pathlib import Path
import re
import sys

from dll_commit_message import ROOT, build_report_errors, commit_id, commits_in_range, event_commits, git
from dll_define_changes import DLL, blob_id


TOKENS = re.compile(r'//[^\n]*|/\*.*?\*/|"(?:\\.|[^"\\])*"|\'(?:\\.|[^\'\\])*\'|[A-Za-z_]\w*|[^\s]', re.S)


# <!-- custom: Tokenize strings and comments before looking for registrations so commented-out calls and .def text inside documentation cannot request rebuilds.
# Only the literal Python name and normalized target expression matter here; reflow, registration order and trailing help-string edits do not change this contract. (GPT-6.1-Sol) -->
def registrations(text):
    tokens = [m.group() for m in TOKENS.finditer(text) if not m.group().startswith(("//", "/*"))]
    found = set()
    for i in range(len(tokens) - 5):
        # <!-- custom: Module-level python::def functions need the same rebuilt DLL as class .def methods; the Python timestamp bridge uses this global-function form.
        # Keep the build-report parser aligned. (GPT-6.1-Sol) -->
        is_method = tokens[i:i + 3] == [".", "def", "("]
        is_function = i >= 2 and tokens[i - 2:i + 3] == ["python", ":", ":", "def", "("]
        if not (is_method or is_function) or not tokens[i + 3].startswith('"') or tokens[i + 4] != ",":
            continue
        target = []
        depth = 0
        for token in tokens[i + 5:]:
            if depth == 0 and token in (",", ")"):
                break
            target.append(token)
            if token in ("(", "[", "<"):
                depth += 1
            elif token in (")", "]", ">"):
                depth -= 1
        if target:
            found.add((tokens[i + 3][1:-1], "".join(target)))
    return found


# <!-- custom: Match the build wrapper's sorted hex-encoded UTF-8 name/NUL/target records; only the registration contract enters this fingerprint, while exact file bytes retain their separate provenance hash. (GPT-6.1-Sol) -->
def binding_fingerprint(text):
    records = sorted((name + "\0" + target).encode("utf-8").hex().upper() for name, target in registrations(text))
    return hashlib.sha256("\n".join(records).encode("ascii")).hexdigest()


def binding_sources(repo, revision):
    if revision is None:
        return {}
    # <!-- custom: git ls-tree rejected :(glob) pathspec magic in the regression fixture; enumerate the source subtree and filter interface filenames locally. (GPT-6.1-Sol) -->
    paths = git(repo, "ls-tree", "-r", "--name-only", revision, "--", "CvGameCoreDLL").decode().splitlines()
    result = {}
    for path in paths:
        if "Interface" not in Path(path).name or not path.endswith(".cpp"):
            continue
        for binding in registrations(git(repo, "show", f"{revision}:{path}").decode("utf-8-sig", errors="replace")):
            result.setdefault(binding, set()).add(path)
    return result


# <!-- custom: A different DLL/report pair can still predate the new binding; compare affected registrations with the build's captured names/targets, not merely the binary/report hash pair.
# Exact file hashes wrongly rejected a post-build rationale comment in CyGameInterface.cpp despite unchanged registrations; keep hashes as byte provenance and accept the captured binding contract when only that file identity differs.
# Reports without binding snapshots still need exact source evidence; deleted sources must be absent, and unchanged registrations moved between files do not request a rebuild. (GPT-6.1-Sol) -->
def manifest_errors(repo, head, paths):
    report = json.loads(git(repo, "show", f"{head}:{DLL}.build.json").decode("utf-8-sig"))
    recorded = {row["path"]: row for row in report.get("buildInputsBefore", [])}
    errors = []
    for path in sorted(paths):
        if blob_id(repo, head, path) is None:
            if path in recorded:
                errors.append(f"Release build manifest still includes deleted binding source {path}")
        else:
            # <!-- custom: The real Windows Release manifest hashes CRLF working files while Git stores LF; comparing raw blob hashes rejected the matching build.
            # Accept either uniform LF/CRLF representation, without normalizing other source content. (GPT-6.1-Sol) -->
            data = git(repo, "show", f"{head}:{path}")
            lf = data.replace(b"\r\n", b"\n")
            hashes = {hashlib.sha256(form).hexdigest() for form in (data, lf, lf.replace(b"\n", b"\r\n"))}
            row = recorded.get(path, {})
            if row.get("sha256", "").lower() in hashes:
                continue
            fingerprint = row.get("pythonBindingsSha256")
            if not isinstance(fingerprint, str) or fingerprint.lower() != binding_fingerprint(data.decode("utf-8-sig", errors="replace")):
                errors.append(f"Release build manifest does not match binding source {path}; rebuild from the integration sources")
    return errors


def check(repo, commits, head):
    head = commit_id(repo, head)
    shipped = blob_id(repo, head, DLL)
    cached = {}
    affected = set()
    paths = set()
    errors = []
    for commit in dict.fromkeys(commits):
        lineage = git(repo, "rev-list", "--parents", "-n", "1", commit).decode().split()
        parent = lineage[1] if len(lineage) > 1 else None
        for revision in (parent, commit):
            if revision not in cached:
                cached[revision] = binding_sources(repo, revision)
        before, after = cached[parent], cached[commit]
        changed = before.keys() ^ after.keys()
        if not changed:
            continue
        affected.update(changed)
        for binding in changed:
            paths.update(before.get(binding, set()) | after.get(binding, set()))
        if shipped is None or shipped == blob_id(repo, parent, DLL):
            names = ", ".join(sorted({name for name, _target in changed}))
            errors.append(f"{commit[:12]}: Python DLL bindings changed ({names}); update {DLL} with a rebuilt Release DLL by the integration head")
    if affected and not errors:
        if blob_id(repo, head, "CvGameCoreDLL/Project/build_with_report.ps1") is None:
            errors.append("Python binding changes require the Release build-report infrastructure")
        else:
            errors.extend(build_report_errors(repo, head, [DLL]))
        if not errors:
            errors.extend(manifest_errors(repo, head, paths))
    return errors, affected


# <!-- custom: Match the structural-define guard's integration policy: source-only feature commits remain possible, but default-branch pushes/PRs must provide the matching DLL at their final head. (GPT-6.1-Sol) -->
def main():
    parser = argparse.ArgumentParser(description="Require matching Release DLL evidence for literal Boost.Python method/function registration changes.")
    parser.add_argument("--repo-root", type=Path, default=ROOT)
    parser.add_argument("--base-ref")
    parser.add_argument("--head-ref", default="HEAD")
    parser.add_argument("--event-name", default=os.environ.get("GITHUB_EVENT_NAME", "local"))
    parser.add_argument("--event-path", type=Path, default=os.environ.get("GITHUB_EVENT_PATH"))
    args = parser.parse_args()
    try:
        head = args.head_ref
        if args.base_ref:
            commits = commits_in_range(args.repo_root, args.base_ref, head)
        elif args.event_name == "workflow_dispatch":
            commits = [commit_id(args.repo_root, head)]
        elif args.event_path:
            event = json.loads(args.event_path.read_text(encoding="utf-8"))
            if args.event_name == "push" and event.get("deleted"):
                print("PASS Python DLL bindings (deleted branch)")
                return 0
            default = event["repository"]["default_branch"]
            target = event["pull_request"]["base"]["ref"] if args.event_name == "pull_request" else event.get("ref", "").removeprefix("refs/heads/")
            if target != default:
                print("PASS Python DLL bindings (deferred until integration into the repository default branch)")
                return 0
            head = event["pull_request"]["head"]["sha"] if args.event_name == "pull_request" else event["after"]
            commits = event_commits(args.repo_root, args.event_name, event, head)
        elif args.event_name != "local":
            raise RuntimeError("GitHub checks require an event payload or explicit base")
        else:
            commits = [commit_id(args.repo_root, head)]
        errors, affected = check(args.repo_root, commits, head)
    except (RuntimeError, OSError, ValueError, KeyError, TypeError) as exc:
        print(f"FAIL Python DLL binding setup: {exc}")
        return 1
    for error in errors:
        print(f"FAIL {error}")
    if errors:
        return 1
    print(f"PASS Python DLL bindings ({len(affected)} registration changes)")
    return 0


if __name__ == "__main__":
    sys.exit(main())
