#!/usr/bin/env python3
# AI, UI, logging, or other modifications first developed in AdvCiv-SAS (Simple Advanced Strategy)
# (c) 2026 wonderingabout & AI/LLM helpers (see Authors in AdvCiv-SAS's root README.md)
# <!-- custom: Local test DLLs, including a Debug-opt build, were accidentally shipped with source changes instead of the deliberately updated Release DLL.
# Source commits can be more frequent than distributed Release DLL updates; an installed test DLL is therefore not automatically a binary intended for Git.
# Require the explicit Update DLL phrase in each binary-changing commit's own title/body so accidental binary staging produces a CI failure rather than silently shipping it.
# A later commit's message cannot establish intent for an earlier accidental binary.
# Inspect Git trees, not a local installed DLL. (GPT-6.1-Sol) -->

import argparse
import json
import os
from pathlib import Path
import re
import subprocess
import sys

ROOT = Path(__file__).resolve().parents[3]
MARKER = re.compile(r"(?<!\w)Update DLL(?!\w)")
PUSH_PAYLOAD_LIMIT = 2048


def git(repo, *args):
    result = subprocess.run(["git", *args], cwd=repo, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
    if result.returncode:
        raise RuntimeError(result.stderr.decode("utf-8", errors="replace").strip())
    return result.stdout


def commit_id(repo, ref):
    return git(repo, "rev-parse", "--verify", "--end-of-options", ref + "^{commit}").decode().strip()


def commits_in_range(repo, base, head):
    base = commit_id(repo, base)
    head = commit_id(repo, head)
    return git(repo, "rev-list", "--reverse", head, "--not", base).decode().splitlines()


def event_commits(repo, event_name, event, head="HEAD"):
    if event_name == "pull_request":
        pr = event["pull_request"]
        return commits_in_range(repo, pr["base"]["sha"], pr["head"]["sha"])
    if event_name == "push":
        if event.get("deleted"):
            return []
        pushed = event.get("commits")
        # <!-- custom: An amended force-push can discard github.event.before, while its new commits remain available.
		# Use the webhook's commit identities and Git for their actual changed paths/messages; Actions omits file lists from push commit objects.
		# A potentially truncated payload must use the complete Git range or fail clearly. (GPT-6.1-Sol) -->
        if isinstance(pushed, list) and len(pushed) < PUSH_PAYLOAD_LIMIT:
            if not pushed:
                return []
            commits = list(dict.fromkeys(commit_id(repo, item["id"]) for item in pushed))
            after = commit_id(repo, event["after"])
            if after not in commits:
                raise RuntimeError("push payload does not include its head commit; cannot validate the complete push")
            return commits
        before = event.get("before", "")
        if not before or set(before) == {"0"}:
            raise RuntimeError("push payload is missing or potentially truncated and has no prior commit; cannot validate the complete push")
        return commits_in_range(repo, before, event["after"])
    return [commit_id(repo, head)]


def dll_paths(repo, commit):
    lineage = git(repo, "rev-list", "--parents", "-n", "1", commit).decode().split()
    # <!-- custom: Compare merge commits to their first parent, just as the branch's new committed tree is compared to its prior tip.
	# This catches an unmarked binary introduced by a merge as well as by an ordinary commit.
	# Root commits include additions.
	# Disable rename folding so moving a DLL away from a .dll suffix still counts as removal. (GPT-6.1-Sol) -->
    options = ["diff-tree", "--no-commit-id", "--name-only", "--no-renames", "-r", "-z"]
    revisions = [lineage[1], commit] if len(lineage) > 1 else ["--root", commit]
    changed = git(repo, *options, *revisions).decode("utf-8", errors="surrogateescape").split("\0")
    return sorted(path for path in changed if path.lower().endswith(".dll"))


def check(repo, commits):
    errors = []
    updated = 0
    for ref in dict.fromkeys(commits):
        commit = commit_id(repo, ref)
        paths = dll_paths(repo, commit)
        if not paths:
            continue
        updated += 1
        message = git(repo, "show", "-s", "--format=%B", commit).decode("utf-8", errors="replace")
        if not MARKER.search(message):
            errors.append(f"{commit[:12]}: changes {', '.join(paths)} but its commit title/body lacks the exact phrase 'Update DLL'")
    return errors, updated


def main():
    parser = argparse.ArgumentParser(description="Require Update DLL in each commit that changes a tracked .dll file.")
    parser.add_argument("--repo-root", type=Path, default=ROOT)
    parser.add_argument("--base-ref", help="explicit range base; omitted for a local single-commit check")
    parser.add_argument("--head-ref", default="HEAD")
    parser.add_argument("--event-name", default=os.environ.get("GITHUB_EVENT_NAME", "local"))
    parser.add_argument("--event-path", type=Path, default=os.environ.get("GITHUB_EVENT_PATH"), help="GitHub event JSON for complete push/PR commit selection")
    args = parser.parse_args()
    try:
        if args.base_ref:
            commits = commits_in_range(args.repo_root, args.base_ref, args.head_ref)
        elif args.event_path:
            event = json.loads(args.event_path.read_text(encoding="utf-8"))
            commits = event_commits(args.repo_root, args.event_name, event, args.head_ref)
        elif args.event_name in {"push", "pull_request"}:
            raise RuntimeError("push/PR checks require --event-path or --base-ref; refusing to check only the last commit")
        else:
            commits = [commit_id(args.repo_root, args.head_ref)]
        errors, updated = check(args.repo_root, commits)
    except (RuntimeError, OSError, ValueError, KeyError, TypeError) as exc:
        print(f"FAIL DLL commit-message check setup: {exc}")
        return 1
    for error in errors:
        print(f"FAIL {error}")
    if errors:
        return 1
    print(f"PASS DLL commit messages ({len(commits)} commit(s) checked, {updated} DLL-changing commit(s))")
    return 0


if __name__ == "__main__":
    sys.exit(main())
