#!/usr/bin/env python3
# AI, UI, logging, or other modifications first developed in AdvCiv-SAS (Simple Advanced Strategy)
# (c) 2026 wonderingabout & AI/LLM helpers (see Authors in AdvCiv-SAS's root README.md)

# <!-- custom: Move the existing Git/base and text-reading functions out of the source-provenance checker so both source and Markdown checks share them without importing one another's scanners.
# Preserve range recovery, error handling and legacy text decoding. (GPT-6.1-Sol) -->
from pathlib import Path
import subprocess
import sys


def run_git(repo: Path, *args: str, check: bool = True) -> subprocess.CompletedProcess:
    result = subprocess.run(
        ["git", *args],
        cwd=repo,
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        text=False,
    )
    if check and result.returncode:
        raise RuntimeError(result.stderr.decode("utf-8", errors="replace").strip())
    return result


def has_commit(repo: Path, ref: str) -> bool:
    return run_git(repo, "cat-file", "-e", ref + "^{commit}", check=False).returncode == 0


def ensure_base(repo: Path, base_ref: str | None, event_name: str | None) -> str | None:
    if base_ref and set(base_ref) == {"0"}:
        return None
    if not base_ref:
        candidate = "HEAD^"
        return candidate if has_commit(repo, candidate) else None
    if has_commit(repo, base_ref):
        return base_ref
    if event_name == "push":
        fetched = run_git(repo, "fetch", "--no-tags", "origin", base_ref, check=False)
        if fetched.returncode == 0 and has_commit(repo, base_ref):
            return base_ref
        print(
            "NOTICE: push-before commit %s is unavailable; change-range checks cannot "
            "check this rewritten push range." % base_ref,
            file=sys.stderr,
        )
        return None
    raise RuntimeError("change-range base %s is unavailable; fetch the base commit first" % base_ref)


def git_text(repo: Path, ref: str, path: str) -> str | None:
    result = run_git(repo, "show", "%s:%s" % (ref, path), check=False)
    if result.returncode:
        return None
    raw = result.stdout
    for encoding in ("utf-8-sig", "utf-8", "cp1252"):
        try:
            return raw.decode(encoding).replace("\r\n", "\n").replace("\r", "\n")
        except UnicodeDecodeError:
            pass
    return raw.decode("utf-8", errors="replace").replace("\r\n", "\n").replace("\r", "\n")
