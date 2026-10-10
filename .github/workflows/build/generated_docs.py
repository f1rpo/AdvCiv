#!/usr/bin/env python3
# AI, UI, logging, or other modifications first developed in AdvCiv-SAS (Simple Advanced Strategy)
# (c) 2026 wonderingabout & AI/LLM helpers (see Authors in AdvCiv-SAS's root README.md)

# <!-- custom: Rebuild generated reports into a temporary directory and compare actual content, rather than merely requiring that a stale report was touched in the same commit.
# The pinned handicap baseline reproduces the published comparison without a sibling mod installation. (GPT-6.1-Sol) -->
import argparse
import datetime
from dataclasses import dataclass
from pathlib import Path
import re
import subprocess
import sys
import tempfile
from typing import Callable

ROOT = Path(__file__).resolve().parents[3]
sys.path.insert(0, str(ROOT / "LLM_Helpers"))
import compare_handicap_infos as handicap
import convert_advciv_manual_to_txt as manual

BASELINE = Path("LLM_Helpers/examples/handicap_infos_baseline.xml")
REPORT = Path("LLM_Helpers/examples/handicap_infos_compared.md")
HANDICAP = Path("Assets/XML/GameInfo/CIV4HandicapInfo.xml")
MANUAL = Path("_0_Common_Docs/AdvCiv_Base_Doc")

@dataclass(frozen=True)
class TextConversion:
    source: Path
    output: Path
    converter: Callable[[Path, Path], object]
    refresh_command: str
    related_sources: tuple[Path, ...] = ()

# <!-- custom: Register each canonical source, searchable text output and its converter together.
# Related source artifacts also require a paired text refresh; the AdvCiv manual is currently the only registered conversion.
# Future conversions use the same checks without manual-specific branches. (GPT-6.1-Sol) -->
TEXT_CONVERSIONS = (
    TextConversion(MANUAL / "manual.odt", MANUAL / "manual.txt", manual.convert, "python LLM_Helpers/convert_advciv_manual_to_txt.py", (MANUAL / "manual.pdf",)),
)

def stable_report(text):
    header, separator, body = text.replace("\r\n", "\n").partition("\n## ")
    header = re.sub(r"^- (?:Run time|Output path|.+ path):.*\n", "", header, flags=re.MULTILINE)
    return header + separator + body

def render_handicap(repo, output):
    title = (repo / REPORT).read_text("utf-8").splitlines()[0]
    labels = re.fullmatch(r"# Handicap Info comparison: (.+) vs (.+)", title)
    if labels is None:
        raise ValueError("handicap report lacks its comparison-title labels")
    left_label, right_label = labels.groups()
    left = handicap.parse_handicaps(repo / BASELINE)
    right = handicap.parse_handicaps(repo / HANDICAP)
    pairs = handicap.collect_entry_pairs(left, right)
    rows = handicap.collect_diffs(pairs, left, right, False)
    matrix = handicap.collect_diffs(pairs, left, right, True)
    stats, changed, total = handicap.collect_diff_stats(pairs, left, right)
    tsv = handicap.build_tsv_text(left_label, right_label, pairs, rows, False, matrix)
    handicap.write_markdown(str(output), str(repo / BASELINE), str(repo / HANDICAP), left_label, right_label, pairs, rows, False, datetime.datetime.now(datetime.timezone.utc), tsv, stats, changed, total)

def has_commit(repo, ref):
    return subprocess.run(["git", "cat-file", "-e", ref + "^{commit}"], cwd=repo, stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL).returncode == 0

def changed_conversion_errors(repo, base_ref, conversions=TEXT_CONVERSIONS, event_name=None):
    # <!-- custom: Compare the whole PR/push range, not only its last commit, so a later paired text update satisfies an earlier source edit.
    # Root pushes have no prior tree. (GPT-6.1-Sol) -->
    if not base_ref or set(base_ref) == {"0"}:
        return []
    if not has_commit(repo, base_ref):
        if event_name != "push":
            return [f"change-range base {base_ref} is unavailable; fetch the base commit before checking paired source/text updates"]
        # <!-- custom: A force-push after amending left github.event.before pointing to a commit absent from the runner's full-history checkout, causing fatal bad object.
        # Try fetching that exact commit first; if it is no longer served, explicitly report the unavailable push-range check while retaining current-content validation.
        # PR/ordinary local missing bases remain errors. (GPT-6.1-Sol) -->
        fetched = subprocess.run(["git", "fetch", "--no-tags", "origin", base_ref], cwd=repo, stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True)
        if fetched.returncode != 0 or not has_commit(repo, base_ref):
            print(f"NOTICE: push-before commit {base_ref} is unavailable even after fetching origin; paired source/text change-range validation cannot run (rewritten push history). Current generated-content validation still runs.", file=sys.stderr)
            print(f"  Git fetch result: {fetched.stderr.strip()}", file=sys.stderr)
            return []
    comparison = subprocess.run(["git", "diff", "--name-only", base_ref, "HEAD"], cwd=repo, stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True)
    if comparison.returncode != 0:
        return [f"could not compare change-range base {base_ref}: {comparison.stderr.strip()}"]
    changed = set(comparison.stdout.splitlines())
    errors = []
    for conversion in conversions:
        sources = {path.as_posix() for path in (conversion.source, *conversion.related_sources)}
        changed_sources = sorted(sources & changed)
        if changed_sources and conversion.output.as_posix() not in changed:
            errors.append("%s changed without a %s refresh in this change range" % (", ".join(changed_sources), conversion.output.as_posix()))
    return errors

def text_conversion_errors(repo, conversions=TEXT_CONVERSIONS):
    errors = []
    with tempfile.TemporaryDirectory() as temporary:
        for index, conversion in enumerate(conversions):
            missing = [path.as_posix() for path in (conversion.source, conversion.output) if not (repo / path).is_file()]
            if missing:
                errors.append("registered text conversion is missing: " + ", ".join(missing))
                continue
            target = Path(temporary) / str(index) / conversion.output.name
            target.parent.mkdir(parents=True)
            conversion.converter(repo / conversion.source, target)
            if target.read_text("utf-8") != (repo / conversion.output).read_text("utf-8"):
                errors.append("%s is stale; run %s" % (conversion.output.as_posix(), conversion.refresh_command))
    return errors

def check(repo, base_ref=None, conversions=TEXT_CONVERSIONS, event_name=None):
    errors = changed_conversion_errors(repo, base_ref, conversions, event_name)
    errors.extend(text_conversion_errors(repo, conversions))
    with tempfile.TemporaryDirectory() as temporary:
        target = Path(temporary)
        render_handicap(repo, target / "handicap.md")
        if stable_report((target / "handicap.md").read_text("utf-8")) != stable_report((repo / REPORT).read_text("utf-8")):
            errors.append("handicap_infos_compared.md is stale; run this checker with --refresh-handicap")
    return errors

def main():
    parser = argparse.ArgumentParser(description="Verify registered source-to-text conversions and the published handicap comparison.")
    parser.add_argument("--repo-root", type=Path, default=ROOT)
    parser.add_argument("--base-ref", help="PR base or push-before commit for registered source/text paired-update validation")
    parser.add_argument("--event-name", choices=("push", "pull_request", "workflow_dispatch"), help="Workflow event; only pushes may report an unavailable rewritten-history base without failing current-content checks")
    parser.add_argument("--refresh-handicap", action="store_true")
    args = parser.parse_args()
    if args.refresh_handicap:
        render_handicap(args.repo_root, args.repo_root / REPORT)
    errors = check(args.repo_root, args.base_ref, event_name=args.event_name)
    print("FAIL generated docs" if errors else "PASS generated docs")
    for error in errors:
        print("  - " + error)
    return int(bool(errors))

if __name__ == "__main__":
    sys.exit(main())
