#!/usr/bin/env python3
# AI, UI, logging, or other modifications first developed in AdvCiv-SAS (Simple Advanced Strategy)
# (c) 2026 wonderingabout & AI/LLM helpers (see Authors in AdvCiv-SAS's root README.md)
#
# Report the shared Linux Python 2.4 package without downloading its binary.

from __future__ import annotations

import json
from pathlib import Path
import re
import urllib.request


PYTHON_VERSION = "2.4.6"
RUNTIME_PLATFORM = "linux-x86_64-debian-bookworm"
RUNTIME_CHANNEL = "runtime-linux-x86_64-bookworm"
RUNTIME_ARCHIVE_NAME = f"python-{PYTHON_VERSION}-{RUNTIME_PLATFORM}.tar.gz"
RUNTIME_RELEASE_BASE = "https://github.com/wonderingabout/python-2.4-docker/releases/download"
RUNTIME_MANIFEST_URL = f"{RUNTIME_RELEASE_BASE}/{RUNTIME_CHANNEL}/runtime-manifest.json"
ARCHIVE_README_NAME = "_SNAPSHOT_CONTEXT/python24_runtime.txt"


def parse_manifest(text: str) -> dict:
    try:
        data = json.loads(text)
    except ValueError as error:
        raise RuntimeError(f"invalid Linux Python 2.4 runtime manifest: {error}") from error
    if not isinstance(data, dict) or data.get("schemaVersion") != 1:
        raise RuntimeError("unsupported Linux Python 2.4 runtime manifest schema")
    for field, expected in (("pythonVersion", PYTHON_VERSION), ("platform", RUNTIME_PLATFORM), ("archiveName", RUNTIME_ARCHIVE_NAME)):
        if data.get(field) != expected:
            raise RuntimeError(f"runtime manifest {field} must be {expected!r}, got {data.get(field)!r}")
    sha = data.get("sha256")
    if not isinstance(sha, str) or re.fullmatch(r"[0-9a-f]{64}", sha) is None:
        raise RuntimeError("runtime manifest must contain a lowercase 64-digit SHA256")
    if type(data.get("sizeBytes")) is not int or data["sizeBytes"] <= 0:
        raise RuntimeError("runtime manifest must contain a positive integer sizeBytes")
    tag = f"{RUNTIME_CHANNEL}-{sha}"
    if data.get("releaseTag") != tag or data.get("archiveUrl") != f"{RUNTIME_RELEASE_BASE}/{tag}/{RUNTIME_ARCHIVE_NAME}":
        raise RuntimeError("runtime manifest must point to its checksum-named package in the shared Python repository")
    return data


def download_text(url: str) -> str:
    with urllib.request.urlopen(url, timeout=15) as response:
        return response.read().decode("utf-8")


# <!-- custom: Remember only the last reported manifest, not runtime bytes or the user's Library state; an unavailable update check must not claim the package is current or block source packaging. (GPT-6.1-Sol) -->
def build_archive_context(repo_root: Path, disabled: bool = False, dry_run: bool = False) -> tuple[dict, str]:
    if disabled:
        return {}, "Linux Python 2.4: disabled; no manifest check or reminder."
    state = repo_root / "LLM_Helpers/outputs/python24_runtime_manifest.json"
    previous = None
    try:
        previous = parse_manifest(state.read_text(encoding="utf-8"))
    except (OSError, RuntimeError):
        pass
    try:
        manifest = parse_manifest(download_text(RUNTIME_MANIFEST_URL))
        if previous is None:
            status = "first package check"
        elif previous["sha256"] == manifest["sha256"]:
            status = "unchanged since last successful check"
        else:
            status = "NEW package since last successful check; review your separately provided copy"
        if not dry_run:
            state.parent.mkdir(parents=True, exist_ok=True)
            state.write_text(json.dumps(manifest, indent=2) + "\n", encoding="utf-8")
    except (OSError, RuntimeError, ValueError) as error:
        manifest = previous
        status = f"WARNING: latest package unknown ({error}); " + ("showing last known metadata" if previous else "no previous metadata available")
    lines = [
        "Linux Python 2.4 package reminder",
        f"Update check: {status}",
        f"Manifest: {RUNTIME_MANIFEST_URL}",
        "No runtime binary was downloaded, copied or included in the source ZIP.",
        "Download the linked package manually when needed; keep it in a location available to your external tool.",
        "Tell the agent where to find it (for example, a supplied Library file). This check cannot inspect that copy.",
    ]
    if manifest:
        lines.extend([f"Python: {manifest['pythonVersion']}", f"Platform: {manifest['platform']}",
                      f"Archive: {manifest['archiveName']}", f"Bytes: {manifest['sizeBytes']}",
                      f"SHA256: {manifest['sha256']}", f"Download: {manifest['archiveUrl']}"])
    instructions = "\n".join(lines) + "\n\n" + (
        "On a compatible Linux x86_64 host, verify the archive SHA256 against the metadata above, then extract it.\n"
        "Point PYTHONHOME at the extracted python-2.4.6 directory and run from the source snapshot root:\n"
        'PYTHONHOME="/path/to/python-2.4.6" "/path/to/python-2.4.6/bin/python" .github/workflows/build/python/python24_compile.py --repo-root .\n'
        "This checks syntax without importing Civ4 engine modules; it does not replace an in-game test.\n"
        "If the package is missing or incompatible, report that the Python 2.4 check was skipped; Python 3 is not a substitute.\n"
        "The package includes CPython's upstream license; normal gameplay needs no separate Python installation.\n"
    )
    summary = f"Linux Python 2.4: metadata only\n  Update: {status}\n  ZIP note: {ARCHIVE_README_NAME}\n  Binary: supplied manually; never downloaded or attached by this helper"
    if manifest:
        summary += f"\n  Python: {manifest['pythonVersion']} / {manifest['platform']}\n  SHA256: {manifest['sha256']}\n  Download: {manifest['archiveUrl']}"
    return {ARCHIVE_README_NAME: instructions.encode("utf-8")}, summary
