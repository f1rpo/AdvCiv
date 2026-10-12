#!/usr/bin/env python3
# AI, UI, logging, or other modifications first developed in AdvCiv-SAS (Simple Advanced Strategy)
# (c) 2026 wonderingabout & AI/LLM helpers (see Authors in AdvCiv-SAS's root README.md)
# <!-- custom: Adding/removing a DLL-referenced XML define name must not silently leave the distributed DLL behind.
# Require a changed binary and matching existing Release build evidence at the integration head; intermediate feature-branch commits may remain source-only.
# Compare name membership, not general XML modifications: a rename is one removed name plus one added name, reported separately without guessing a pairing.
# Value/comment edits and Python-only names are excluded. (GPT-6.1-Sol) -->

import argparse
import json
import os
from pathlib import Path
import re
import subprocess
import sys
import xml.etree.ElementTree as ET

from dll_commit_message import ROOT, build_report_errors, commit_id, commits_in_range, event_commits, git

XML_PATHS = [":(glob,icase)Assets/XML/**/*.xml"]
DLL = "Assets/CvGameCoreDLL.dll"
CPP_PATHS = [f":(glob)CvGameCoreDLL/**/*{suffix}" for suffix in (".cpp", ".h", ".hpp", ".inl")]
CPP_TOKENS = re.compile(r'//[^\n]*|/\*.*?\*/|"(?:\\.|[^"\\])*"|\'(?:\\.|[^\'\\])*\'|[A-Za-z_]\w*', re.S)
DEFINE_NAME = re.compile(r"\b[A-Za-z_]\w*\b")


def blob_id(repo, revision, path):
    if revision is None:
        return None
    row = git(repo, "ls-tree", revision, "--", path).split()
    return row[2] if row else None


# <!-- custom: Discover DefineName declarations by XML content under Assets/XML, without restricting filenames or the SAS_ prefix.
# Combine names across all files so moving a declaration between files does not imply an interface change. (GPT-6.1-Sol) -->
def declared_names(repo, revision):
    if revision is None:
        return set()
    result = subprocess.run(["git", "grep", "-I", "-l", "-z", "-F", "DefineName", revision, "--", *XML_PATHS], cwd=repo, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
    if result.returncode not in (0, 1):
        raise RuntimeError(result.stderr.decode(errors="replace"))
    names = set()
    for entry in result.stdout.decode("utf-8", errors="surrogateescape").split("\0"):
        if not entry:
            continue
        root = ET.fromstring(git(repo, "show", entry))
        names.update(node.text.strip() for node in root.iter() if node.tag.split("}")[-1] == "DefineName" and node.text and node.text.strip())
    return names


# <!-- custom: Inspect both source trees so a removed/renamed consumer cannot disappear from the rebuild decision.
# Ignore comments; this is a conservative static name-reference check, not proof of preprocessor reachability or dynamically constructed names. (GPT-6.1-Sol) -->
def cpp_references(repo, revision, names):
    if revision is None or not names:
        return set()
    patterns = [arg for name in sorted(names) for arg in ("-e", name)]
    result = subprocess.run(["git", "grep", "-I", "-l", "-z", "-w", "-F", *patterns, revision, "--", *CPP_PATHS], cwd=repo, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
    if result.returncode not in (0, 1):
        raise RuntimeError(result.stderr.decode(errors="replace"))
    found = set()
    for entry in result.stdout.decode("utf-8", errors="surrogateescape").split("\0"):
        if not entry:
            continue
        text = git(repo, "show", entry).decode("utf-8-sig", errors="replace")
        for token in CPP_TOKENS.finditer(text):
            if not token.group().startswith(("//", "/*")):
                found.update(names.intersection(DEFINE_NAME.findall(token.group())))
    return found


def check(repo, commits, head):
    head = commit_id(repo, head)
    affected = set()
    errors = []
    shipped = blob_id(repo, head, DLL)
    name_cache = {}
    for commit in dict.fromkeys(commits):
        lineage = git(repo, "rev-list", "--parents", "-n", "1", commit).decode().split()
        parent = lineage[1] if len(lineage) > 1 else None
        if blob_id(repo, parent, "Assets/XML") == blob_id(repo, commit, "Assets/XML"):
            continue
        for revision in (parent, commit):
            if revision not in name_cache:
                name_cache[revision] = declared_names(repo, revision)
        before = name_cache[parent]
        after = name_cache[commit]
        changed = before ^ after
        used = cpp_references(repo, parent, changed) | cpp_references(repo, commit, changed)
        if not used:
            continue
        affected.update(used)
        if shipped is None or shipped == blob_id(repo, parent, DLL):
            added = ", ".join(sorted(used & (after - before))) or "none"
            removed = ", ".join(sorted(used & (before - after))) or "none"
            errors.append(f"{commit[:12]}: DLL-referenced XML define name changes\n  added: {added}\n  removed: {removed}\n  Note: this check compares define-name sets, so renaming a define is reported as removing its old name and adding its new name; value/comment edits are excluded.\n  Update {DLL} with a rebuilt Release DLL by the integration head.")
    if affected and not errors:
        if blob_id(repo, head, "CvGameCoreDLL/Project/build_with_report.ps1") is None:
            errors.append("DLL define changes require the Release build-report infrastructure")
        else:
            errors.extend(build_report_errors(repo, head, [DLL]))
    return errors, affected


def main():
    parser = argparse.ArgumentParser(description="Require rebuilt Release DLL evidence for added/removed DLL-referenced XML define names at integration.")
    parser.add_argument("--repo-root", type=Path, default=ROOT)
    parser.add_argument("--base-ref", help="explicit local range base; otherwise check the selected head commit")
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
                print("PASS DLL define changes (deleted branch)")
                return 0
            default = event["repository"]["default_branch"]
            target = event["pull_request"]["base"]["ref"] if args.event_name == "pull_request" else event.get("ref", "").removeprefix("refs/heads/")
            if target != default:
                print("PASS DLL define changes (deferred until integration into the repository default branch)")
                return 0
            if args.event_name == "pull_request":
                head = event["pull_request"]["head"]["sha"]
            elif args.event_name == "push":
                head = event["after"]
            commits = event_commits(args.repo_root, args.event_name, event, head)
        elif args.event_name != "local":
            raise RuntimeError("GitHub checks require an event payload or explicit base")
        else:
            commits = [commit_id(args.repo_root, head)]
        errors, affected = check(args.repo_root, commits, head)
    except (RuntimeError, OSError, ValueError, KeyError, TypeError, ET.ParseError) as exc:
        print(f"FAIL DLL define change setup: {exc}")
        return 1
    for error in errors:
        print(f"FAIL {error}")
    if errors:
        return 1
    print(f"PASS DLL define changes ({len(affected)} DLL-referenced name changes)")
    return 0


if __name__ == "__main__":
    sys.exit(main())
