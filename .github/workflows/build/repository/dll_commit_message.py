#!/usr/bin/env python3
# AI, UI, logging, or other modifications first developed in AdvCiv-SAS (Simple Advanced Strategy)
# (c) 2026 wonderingabout & AI/LLM helpers (see Authors in AdvCiv-SAS's root README.md)
# <!-- custom: Local test DLLs, including a Debug-opt build, were accidentally shipped with source changes instead of the deliberately updated Release DLL.
# Source commits can be more frequent than distributed Release DLL updates; an installed test DLL is therefore not automatically a binary intended for Git.
# Require the explicit Update DLL phrase in each binary-changing commit's own title/body so accidental binary staging produces a CI failure rather than silently shipping it.
# A later commit's message cannot establish intent for an earlier accidental binary.
# Inspect Git trees, not a local installed DLL. (GPT-6.1-Sol) -->

import argparse
# <!-- custom: Hash committed DLL bytes and validate UTC build times when checking the matching full build report. (GPT-6.1-Sol) -->
from datetime import datetime, timezone
import hashlib
import json
import os
# <!-- custom: Drop PurePosixPath after removing the committed compiler-log requirement; Path remains for local repository/event arguments. (GPT-6.1-Sol) -->
from pathlib import Path
import re
import subprocess
import sys

# <!-- custom: Resolve repository/shared-helper paths from this subject subfolder after the workflow-tool reorganization. (GPT-6.1-Sol) -->
ROOT = Path(__file__).resolve().parents[4]
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


# <!-- custom: Share the first-parent path diff with report validation so JSON-only changes cannot bypass the DLL pair/intent rule. (GPT-6.1-Sol) -->
def changed_paths(repo, commit):
    lineage = git(repo, "rev-list", "--parents", "-n", "1", commit).decode().split()
    # <!-- custom: Compare merge commits to their first parent, just as the branch's new committed tree is compared to its prior tip.
	# This catches an unmarked binary introduced by a merge as well as by an ordinary commit.
	# Root commits include additions.
	# Disable rename folding so moving a DLL away from a .dll suffix still counts as removal. (GPT-6.1-Sol) -->
    options = ["diff-tree", "--no-commit-id", "--name-only", "--no-renames", "-r", "-z"]
    revisions = [lineage[1], commit] if len(lineage) > 1 else ["--root", commit]
    changed = git(repo, *options, *revisions).decode("utf-8", errors="surrogateescape").split("\0")
    return sorted(path for path in changed if path)


# <!-- custom: Retain the DLL-only query for existing callers; pair validation uses the complete changed-path set above. (GPT-6.1-Sol) -->
def dll_paths(repo, commit):
    return [path for path in changed_paths(repo, commit) if path.lower().endswith(".dll")]


# <!-- custom: Start build-report enforcement with trees containing the automatic report wrapper; historical DLL commits predate this requirement.
# Read reports and DLLs from each checked commit, never the working tree or a later repairing commit.
# Deletions need intent but no build evidence. (GPT-6.1-Sol) -->
def build_report_errors(repo, commit, paths):
    wrapper = "CvGameCoreDLL/Project/build_with_report.ps1"
    if not git(repo, "ls-tree", commit, "--", wrapper).strip():
        return []
    errors = []
    for path in paths:
        if not git(repo, "ls-tree", commit, "--", path).strip():
            continue
        summary_path = path + ".build.json"
        prefix = f"{commit[:12]}: {summary_path}"
        try:
            summary = json.loads(git(repo, "show", f"{commit}:{summary_path}").decode("utf-8-sig"))
            if not isinstance(summary, dict):
                raise ValueError("expected a JSON object")
            # <!-- custom: The distributed binary must use exactly Release; the explicit intent marker alone also allowed accidental Debug-opt binaries. (GPT-6.1-Sol) -->
            if summary.get("configuration") != "Release":
                raise ValueError("requires configuration Release (fast, Debug and Profile builds are not accepted)")
            if summary.get("status") != "SUCCEEDED" or summary.get("request") not in {"BUILD", "REBUILD"}:
                raise ValueError("requires a successful build/rebuild")
            for field in ("durationMilliseconds", "warningCount", "errorCount"):
                if type(summary.get(field)) is not int or summary[field] < 0:
                    raise ValueError(f"{field} must be a nonnegative integer")
            if summary["errorCount"] != 0 or summary.get("exitCode") != 0:
                raise ValueError("successful build report must report zero errors")
            # <!-- custom: A compiler can produce a usable DLL despite warnings.
            # Reject that shipping evidence so successful compilation does not hide diagnostics still needing attention. (GPT-6.1-Sol) -->
            if summary["warningCount"] != 0:
                raise ValueError(f"shipped Release build must report zero warnings (reported {summary['warningCount']}); address diagnostics and rebuild")
            times = [datetime.fromisoformat(summary[field].replace("Z", "+00:00")) for field in ("startedUtc", "endedUtc")]
            if any(time.utcoffset() != timezone.utc.utcoffset(None) for time in times) or times[1] < times[0]:
                raise ValueError("requires ordered UTC start/end timestamps")
            # <!-- custom: Commit the full JSON for DLL identity/status checks; keep full compiler output local rather than requiring an AdvCiv.log copy in Assets.
            # The JSON retains recognized coded diagnostics and build context, while the optional local log also preserves uncoded messages and all command output. (GPT-6.1-Sol) -->
            binary = git(repo, "show", f"{commit}:{path}")
            dll = summary["dllAfter"]
            if type(dll["sizeBytes"]) is not int or dll["sizeBytes"] != len(binary) or dll.get("sha256", "").lower() != hashlib.sha256(binary).hexdigest():
                raise ValueError("DLL size/SHA-256 does not match this commit's binary")
        except (RuntimeError, ValueError, KeyError, TypeError, AttributeError) as exc:
            errors.append(f"{prefix}: {exc}")
    return errors


# <!-- custom: The current shipped DLL must retain its matching JSON even on source-only commits.
# Previously evidence was checked only when a DLL changed, so missing/deleted support files escaped that guard; historical per-commit checks remain prospective. (GPT-6.1-Sol) -->
def shipped_build_errors(repo, head):
    head = commit_id(repo, head)
    for path in ("Assets/CvGameCoreDLL.dll", "CvGameCoreDLL/Project/build_with_report.ps1"):
        if not git(repo, "ls-tree", head, "--", path).strip():
            return [f"{head[:12]}: required shipped-DLL build evidence infrastructure missing: {path}"]
    return build_report_errors(repo, head, ["Assets/CvGameCoreDLL.dll"])


# <!-- custom: With build reporting available, a DLL and its JSON must change together and carry Update DLL; that marker without a pair also fails.
# Apply pair enforcement prospectively from the reporting wrapper, while older binary commits retain their original intent check. (GPT-6.1-Sol) -->
def check(repo, commits):
    errors = []
    updated = 0
    for ref in dict.fromkeys(commits):
        commit = commit_id(repo, ref)
        changed = set(changed_paths(repo, commit))
        paths = sorted(path for path in changed if path.lower().endswith(".dll"))
        reports = sorted(path for path in changed if path.lower().endswith(".dll.build.json"))
        message = git(repo, "show", "-s", "--format=%B", commit).decode("utf-8", errors="replace")
        marked = bool(MARKER.search(message))
        paired_policy = bool(git(repo, "ls-tree", commit, "--", "CvGameCoreDLL/Project/build_with_report.ps1").strip())
        if not paths and not (paired_policy and reports):
            if paired_policy and marked:
                errors.append(f"{commit[:12]}: contains 'Update DLL' but no DLL/build-JSON pair changed")
            continue
        updated += bool(paths)
        affected = sorted(set(paths) | {path[:-len(".build.json")] for path in reports}) if paired_policy else paths
        errors.extend(build_report_errors(repo, commit, affected))
        if paired_policy:
            for dll in affected:
                report = dll + ".build.json"
                if dll not in changed or report not in changed:
                    errors.append(f"{commit[:12]}: {dll} and {report} must both change in this commit (including paired additions, removals or renames)")
        if not marked:
            errors.append(f"{commit[:12]}: changes {', '.join(affected)} but its commit title/body lacks the exact phrase 'Update DLL'")
    return errors, updated


def main():
    parser = argparse.ArgumentParser(description="Require Update DLL and matching Release build evidence for tracked DLL updates.")
    parser.add_argument("--repo-root", type=Path, default=ROOT)
    parser.add_argument("--base-ref", help="explicit range base; omitted for a local single-commit check")
    parser.add_argument("--head-ref", default="HEAD")
    parser.add_argument("--event-name", default=os.environ.get("GITHUB_EVENT_NAME", "local"))
    parser.add_argument("--event-path", type=Path, default=os.environ.get("GITHUB_EVENT_PATH"), help="GitHub event JSON for complete push/PR commit selection")
    args = parser.parse_args()
    try:
        head = args.head_ref
        deleted = False
        if args.base_ref:
            commits = commits_in_range(args.repo_root, args.base_ref, args.head_ref)
        elif args.event_path:
            event = json.loads(args.event_path.read_text(encoding="utf-8"))
            deleted = args.event_name == "push" and event.get("deleted", False)
            if args.event_name == "pull_request":
                head = event["pull_request"]["head"]["sha"]
            elif args.event_name == "push" and not deleted:
                head = event["after"]
            commits = event_commits(args.repo_root, args.event_name, event, args.head_ref)
        elif args.event_name in {"push", "pull_request"}:
            raise RuntimeError("push/PR checks require --event-path or --base-ref; refusing to check only the last commit")
        else:
            commits = [commit_id(args.repo_root, args.head_ref)]
        errors, updated = check(args.repo_root, commits)
        if not deleted:
            errors = list(dict.fromkeys([*errors, *shipped_build_errors(args.repo_root, head)]))
    except (RuntimeError, OSError, ValueError, KeyError, TypeError) as exc:
        print(f"FAIL DLL commit-message check setup: {exc}")
        return 1
    for error in errors:
        print(f"FAIL {error}")
    if errors:
        return 1
    print(f"PASS DLL intent/build reports ({len(commits)} commit(s) checked, {updated} DLL-changing commit(s))")
    return 0


if __name__ == "__main__":
    sys.exit(main())
