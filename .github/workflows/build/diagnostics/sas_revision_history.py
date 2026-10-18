#!/usr/bin/env python3
# AI, UI, logging, or other modifications first developed in AdvCiv-SAS (Simple Advanced Strategy)
# (c) 2026 wonderingabout & AI/LLM helpers (see Authors in AdvCiv-SAS's root README.md)
# <!-- custom: Resolve repository/shared-helper paths from this subject subfolder after the workflow-tool reorganization. (GPT-6.1-Sol) -->

# <!-- custom: Validate finalized revision hashes against canonical default-branch history, then enforce the simple update-counter rule on current branch commits: a qualifying SASGameRecord implementation commit increments exactly once, unrelated commits do not increment, and the number never regresses or jumps.
# Dedicated recorder/checker/helper files are explicit; other source/config bridges qualify when their diff contains SASGameRecord/GAME_RECORD markers.
# This deliberately catches mechanically recognizable changes without pretending CI can infer every future semantic dependency; the written revision policy remains authoritative for unusual refactors. (ChatGPT-5.6-Sol) -->
import argparse
# <!-- custom: Read explicit GitHub event boundaries from its JSON payload so pushes cover every submitted commit and real merges are not guessed to be synthetic PR merges. (GPT-6.1-Sol) -->
import json
from pathlib import Path
import re
import subprocess
import sys
# <!-- custom: Parse historical/current GlobalDefines structurally so revision/config comparisons do not depend on XML whitespace or element formatting. (ChatGPT-5.6-Sol) -->
import xml.etree.ElementTree as ET

# <!-- custom: Share visible history-entry parsing and Change-field checks with the current-default and runtime guards; per-commit validation also catches an explanation deferred to a later commit. (GPT-6.1-Sol) -->
sys.path.insert(0, str(Path(__file__).resolve().parents[2] / "lib"))
from revision_history import change_errors, revision_entries

ROOT = Path(__file__).resolve().parents[4]
HISTORY = Path("_1_AdvCiv-SAS/Docs/revisions/SASGameRecord.md")
HEADER = "CvGameCoreDLL/SASGameRecordLog.h"
DEFINES = "Assets/XML/GlobalDefines_advciv_sas.xml"
DEFINE_FIRST_REVISION = 153
REVISION_DEFINE = "SAS_GAME_RECORD_REVISION"

# <!-- custom: Dedicated implementation/checker/helper files always count; tests/docs validate or describe the implementation but do not independently create a revision. (ChatGPT-5.6-Sol) -->
DEDICATED_QUALIFYING = {
    "CvGameCoreDLL/SASGameRecordLog.cpp",
    "CvGameCoreDLL/SASGameRecordLog.h",
    "Assets/Python/SASGameRecordSettings.py",
    "Assets/Python/SASGameRecordConfig.py",
    "Assets/Python/SASGameRecordActions.py",
    ".github/workflows/build/diagnostics/sas_game_record_log.py",
    ".github/workflows/build/diagnostics/sas_revision_history.py",
    "LLM_Helpers/compare_sasgamerecord_rng.py",
    "LLM_Helpers/expand_sasgamerecord_city_deltas.py",
}
IGNORED_SUFFIXES = {".md", ".txt", ".log", ".zip", ".png", ".jpg", ".jpeg", ".gif"}
MARKER = re.compile(r"SASGameRecord|SAS_GAME_RECORD|GAME_RECORD_")


def revision_define(xml_bytes):
    try:
        root = ET.fromstring(xml_bytes)
    except ET.ParseError:
        return None
    values = []
    for define in root.iter():
        if define.tag.rsplit("}", 1)[-1] != "Define":
            continue
        name = None
        value = None
        for child in list(define):
            local = child.tag.rsplit("}", 1)[-1]
            if local == "DefineName":
                name = (child.text or "").strip()
            elif local == "iDefineIntVal":
                value = (child.text or "").strip()
        if name == REVISION_DEFINE:
            try:
                values.append(int(value))
            except (TypeError, ValueError):
                return None
    return values[0] if len(values) == 1 else None


def sas_game_record_define_values(xml_bytes):
    try:
        root = ET.fromstring(xml_bytes)
    except ET.ParseError:
        return None
    values = {}
    for define in root.iter():
        if define.tag.rsplit("}", 1)[-1] != "Define":
            continue
        children = [(child.tag.rsplit("}", 1)[-1], (child.text or "").strip()) for child in list(define)]
        name = next((value for tag, value in children if tag == "DefineName"), None)
        if name and name.startswith("SAS_GAME_RECORD_") and name != REVISION_DEFINE:
            values[name] = tuple((tag, value) for tag, value in children if tag != "DefineName")
    return values


def resolve_default_branch(repo, requested_ref=None):
    candidates = [requested_ref] if requested_ref else []
    if not requested_ref:
        symbolic = subprocess.run(["git", "symbolic-ref", "--quiet", "refs/remotes/origin/HEAD"], cwd=repo, capture_output=True, text=True)
        if symbolic.returncode == 0:
            candidates.append(symbolic.stdout.strip())
        candidates.extend(("refs/remotes/origin/main", "refs/heads/main"))
    for candidate in candidates:
        resolved = subprocess.run(["git", "rev-parse", "--verify", candidate + "^{commit}"], cwd=repo, capture_output=True, text=True)
        if resolved.returncode == 0:
            return candidate, resolved.stdout.strip()
    return None, None


def git_show(repo, commit, path):
    result = subprocess.run(["git", "show", f"{commit}:{path}"], cwd=repo, capture_output=True)
    return None if result.returncode else result.stdout


def commit_revision(repo, commit):
    xml = git_show(repo, commit, DEFINES)
    if xml is not None:
        value = revision_define(xml)
        if value is not None:
            return value
    header = git_show(repo, commit, HEADER)
    if header is not None:
        marker = re.search(rb"SAS_GAME_RECORD_REVISION\s*=\s*(\d+)", header)
        if marker is not None:
            return int(marker.group(1))
    return None


def first_parent(repo, commit):
    line = subprocess.check_output(["git", "rev-list", "--parents", "-n", "1", commit], cwd=repo).decode().strip().split()
    return line[1] if len(line) > 1 else None


def changed_paths(repo, parent, commit):
    output = subprocess.check_output(["git", "diff", "--name-only", parent, commit, "--"], cwd=repo).decode("utf-8", "replace")
    return [line for line in output.splitlines() if line]


def global_define_change_qualifies(repo, parent, commit):
    before = git_show(repo, parent, DEFINES)
    after = git_show(repo, commit, DEFINES)
    if before is None or after is None:
        return False
    before_values = sas_game_record_define_values(before)
    after_values = sas_game_record_define_values(after)
    return before_values is not None and after_values is not None and before_values != after_values


def diff_mentions_recorder(repo, parent, commit, path):
    result = subprocess.run(["git", "diff", "--unified=0", parent, commit, "--", path], cwd=repo, capture_output=True, text=True, encoding="utf-8", errors="replace")
    if result.returncode != 0:
        return False
    for line in result.stdout.splitlines():
        if not line.startswith(("+", "-")) or line.startswith(("+++", "---")):
            continue
        if MARKER.search(line[1:]):
            return True
    return False


def commit_has_qualifying_change(repo, parent, commit):
    for path in changed_paths(repo, parent, commit):
        normalized = path.replace("\\", "/")
        if normalized in DEDICATED_QUALIFYING:
            return True
        if normalized == DEFINES:
            if global_define_change_qualifies(repo, parent, commit):
                return True
            continue
        # <!-- custom: Shipped DLL reports contain source paths and recorded Git status; changing that evidence falsely requested revision 157 for a runtime-only commit.
        # Exclude this artifact type from implementation detection without excluding JSON configuration files generally. (GPT-6.1-Sol) -->
        if normalized.startswith(".github/workflows/tests/") or normalized.lower().endswith(".dll.build.json") or Path(normalized).suffix.lower() in IGNORED_SUFFIXES:
            continue
        if normalized.startswith("LLM_Helpers/") and "sasgamerecord" in Path(normalized).name.lower() and Path(normalized).suffix.lower() == ".py":
            return True
        if diff_mentions_recorder(repo, parent, commit, normalized):
            return True
    return False


# <!-- custom: Previously only the tip was checked when HEAD matched main, missing earlier bad commits in a multi-commit push; ancestry guessing also treated real merges as synthetic PR merges.
# Accept explicit event data: pushes check before..after, PRs check their authored head from the base/head merge base, and local/manual checks retain the default-branch comparison. (GPT-6.1-Sol) -->
def progression_commits(repo, default_branch_commit, event_name="local", event_data=None):
    event_data = event_data or {}
    head_result = subprocess.run(["git", "rev-parse", "HEAD"], cwd=repo, capture_output=True, text=True)
    if head_result.returncode != 0:
        return []
    tip = head_result.stdout.strip()
    base = None
    if event_name == "push":
        before = event_data.get("before")
        after = event_data.get("after")
        if not before or not after:
            raise ValueError("push revision check requires the event's before and after commits")
        tip = after
        if before.strip("0"):
            base = before
        # <!-- custom: A new branch has an all-zero before SHA; use its default-branch divergence instead of asking Git to resolve a nonexistent commit. (GPT-6.1-Sol) -->
    elif event_name == "pull_request":
        pull = event_data.get("pull_request", {})
        tip = pull.get("head", {}).get("sha")
        target = pull.get("base", {}).get("sha")
        if not tip or not target:
            raise ValueError("PR revision check requires the event's base and head commits")
        result = subprocess.run(["git", "merge-base", target, tip], cwd=repo, capture_output=True, text=True)
        if result.returncode != 0:
            raise ValueError("cannot resolve the PR revision-check merge base; fetch complete base/head history")
        base = result.stdout.strip()
    if base is None and default_branch_commit and tip != default_branch_commit:
        result = subprocess.run(["git", "merge-base", default_branch_commit, tip], cwd=repo, capture_output=True, text=True)
        if result.returncode == 0 and result.stdout.strip() != tip:
            base = result.stdout.strip()
    if base is None:
        return [tip]
    # <!-- custom: Enumerate the actual push range even after a force-push; each selected commit is checked against its own first parent, not against the event's old tip. (GPT-6.1-Sol) -->
    result = subprocess.run(["git", "rev-list", "--reverse", "--first-parent", f"{base}..{tip}"], cwd=repo, capture_output=True, text=True)
    if result.returncode != 0:
        raise ValueError("cannot resolve the revision-check commit range; fetch complete before/after history")
    return result.stdout.splitlines()


# <!-- custom: Pass event boundaries into progression selection; return a diagnostic on missing Git history rather than silently checking only the last commit. (GPT-6.1-Sol) -->
def check_progression(repo, default_branch_ref=None, event_name="local", event_data=None):
    _, branch_commit = resolve_default_branch(repo, default_branch_ref)
    errors = []
    try:
        commits = progression_commits(repo, branch_commit, event_name, event_data)
    except ValueError as error:
        return [str(error)]
    for commit in commits:
        parent = first_parent(repo, commit)
        if parent is None:
            continue
        previous = commit_revision(repo, parent)
        current = commit_revision(repo, commit)
        short = commit[:12]
        if previous is None or current is None:
            errors.append(f"commit {short}: cannot resolve SASGameRecord revision in commit and first parent")
            continue
        delta = current - previous
        if delta < 0:
            errors.append(f"commit {short}: SASGameRecord revision regressed {previous}->{current}")
            continue
        if delta > 1:
            errors.append(f"commit {short}: SASGameRecord revision jumped {previous}->{current}; at most one increment is allowed per commit")
            continue
        qualifies = commit_has_qualifying_change(repo, parent, commit)
        if qualifies and delta != 1:
            errors.append(f"commit {short}: qualifying SASGameRecord implementation change requires exactly one revision increment ({previous}->{previous + 1})")
        elif not qualifies and delta != 0:
            errors.append(f"commit {short}: SASGameRecord revision advanced {previous}->{current} without an automatically recognized qualifying implementation change")
        if delta == 1:
            history = git_show(repo, commit, HISTORY.as_posix())
            errors.extend(f"commit {short}: {error}" for error in change_errors("" if history is None else history.decode("utf-8", "replace"), current))
    return errors


def check_history(repo, default_branch_ref=None):
    branch_ref, branch_commit = resolve_default_branch(repo, default_branch_ref)
    if branch_commit is None:
        print("NOTICE SASGameRecord revision default-branch ancestry unavailable; abstaining from that check")
    if branch_commit and subprocess.check_output(["git", "rev-parse", "--is-shallow-repository"], cwd=repo).strip() != b"false":
        return ["revision provenance requires full Git history (checkout fetch-depth: 0)"]
    ancestors = None if branch_commit is None else set(subprocess.check_output(["git", "rev-list", branch_commit], cwd=repo).decode().splitlines())
    text = (repo / HISTORY).read_text("utf-8")
    entries = revision_entries(text)
    if not entries:
        return ["no revision history entries"]
    errors = change_errors(text)
    for index, (revision, body) in enumerate(entries):
        value = re.search(r"^- \*\*Git commit:\*\* (.+)$", body, re.MULTILINE)
        if value is None:
            errors.append(f"revision {revision}: missing Git commit field")
            continue
        commit = value.group(1).strip().strip("`")
        if commit == "pending":
            if index:
                errors.append(f"revision {revision}: only the latest entry may remain pending")
            continue
        if not re.fullmatch(r"[0-9a-f]{40}", commit):
            errors.append(f"revision {revision}: expected a full commit hash or latest-entry pending")
            continue
        if ancestors is not None and commit not in ancestors:
            errors.append(f"revision {revision}: {commit} is absent from default-branch ancestry ({branch_ref}); reconcile after amend/rebase")
            continue
        if revision >= DEFINE_FIRST_REVISION:
            source = git_show(repo, commit, DEFINES)
            marker = None if source is None else revision_define(source)
            if marker != revision:
                errors.append(f"revision {revision}: {commit} does not contain matching {REVISION_DEFINE}={revision}")
        elif revision >= 69:
            source = git_show(repo, commit, HEADER)
            marker = None if source is None else re.search(rb"SAS_GAME_RECORD_REVISION\s*=\s*(\d+)", source)
            if marker is None or int(marker.group(1)) != revision:
                errors.append(f"revision {revision}: {commit} does not contain its matching explicit source marker")
    return errors


# <!-- custom: History provenance uses the canonical branch; progression separately receives the actual push/PR boundaries. (GPT-6.1-Sol) -->
def check(repo, default_branch_ref=None, event_name="local", event_data=None):
    return check_history(repo, default_branch_ref) + check_progression(repo, default_branch_ref, event_name, event_data)


def main():
    parser = argparse.ArgumentParser(description="Validate SASGameRecord revision history and one-increment-per-qualifying-commit progression.")
    parser.add_argument("--repo-root", type=Path, default=ROOT)
    parser.add_argument("--default-branch-ref", help="Canonical default-branch ref; ancestry/range validation is reduced when it is unavailable")
    parser.add_argument("--event-name", choices=("local", "push", "pull_request", "workflow_dispatch"), default="local", help="GitHub event type; local checks compare against the default branch")
    parser.add_argument("--event-path", type=Path, help="GitHub event JSON containing explicit before/after or PR base/head commits")
    args = parser.parse_args()
    event_data = None
    if args.event_name in ("push", "pull_request"):
        if args.event_path is None:
            parser.error("push/PR checks require --event-path")
        try:
            event_data = json.loads(args.event_path.read_text(encoding="utf-8"))
        except (OSError, ValueError) as error:
            parser.error(f"cannot read revision-check event: {error}")
        if not isinstance(event_data, dict):
            parser.error("revision-check event must be a JSON object")
    errors = check(args.repo_root, args.default_branch_ref, args.event_name, event_data)
    print("FAIL SASGameRecord revision provenance/progression" if errors else "PASS SASGameRecord revision provenance/progression")
    for error in errors:
        print("  - " + error)
    return int(bool(errors))

if __name__ == "__main__":
    sys.exit(main())
