#!/usr/bin/env python3
# AI, UI, logging, or other modifications first developed in AdvCiv-SAS (Simple Advanced Strategy)
# (c) 2026 wonderingabout & AI/LLM helpers (see Authors in AdvCiv-SAS's root README.md)
#
# Create a timestamped Civ4 mod light source ZIP for quick local/LLM review handoffs.
# Store repo-relative paths and use ZIP_DEFLATED by default so adding selected screenshot folders stays reasonably uploadable.
# Refined with ChatGPT-5.5, ChatGPT-5.6-Sol, and Codex.
# Create a timestamped light source ZIP for a Civ4 mod.

from __future__ import annotations

import argparse
import hashlib
import re
import shlex
import shutil
import subprocess
from datetime import datetime
from pathlib import Path
from time import perf_counter
from typing import Iterable, Iterator
from zipfile import ZIP_DEFLATED, ZIP_STORED, ZipFile

import refresh_commit_diffs


# <!-- custom: Keep Git's canonical `Assets/Res` casing here even on Windows, whose case-insensitive filesystem may also display/accept `Assets/res`; GitHub/Linux paths and CI are case-sensitive. (ChatGPT-5.6-Sol) -->
ASSET_SUBDIRS = (
    "Assets/Python",
    "Assets/Config",
    "Assets/Res",
    "Assets/XML",
)

ROOT_SUBDIRS = (
    "PrivateMaps",
    ".claude",
    ".github",
    "Long_Comments",
    "_0_Common_Docs",
    "LLM_Helpers",
    "Resource",
    "Settings",
    ".vscode",
)

EXTRA_SUBDIRS = (
    "_1_AdvCiv-SAS/Docs",
    "_1_AdvCiv-SAS/SASGameRecord_log",
)

# Optional screenshot-folder whitelist for visual LLM/code-agent context, e.g. advisors, main menu, Sevopedia, rendered SASGameRecord map text, and common UI.
# Keep this explicit instead of including all image folders so broad image additions do not silently bloat the archive.
IMAGE_SUBDIRS = (
    "_1_AdvCiv-SAS/Images/advisors",
    "_1_AdvCiv-SAS/Images/main_menu",
    "_1_AdvCiv-SAS/Images/SASGameRecord_map_text",
    "_1_AdvCiv-SAS/Images/sevopedia",
    "_1_AdvCiv-SAS/Images/ui_other",
)

DLL_TOP_LEVEL_DIR = "CvGameCoreDLL"
DLL_PROJECT_DIR = "CvGameCoreDLL/Project"
DLL_PROJECT_MAX_BYTES = 1 * 1024 * 1024
DEFAULT_OUTPUT_DIR = "."
DEFAULT_MOD_NAME = "UnspecifiedModName"
ARCHIVE_LABEL = "light_source"
DEFAULT_ARCHIVE_PREFIX = None
DEFAULT_COMPRESSION_LEVEL = 6
# Full reachable K-Mod -> pre-SAS AdvCiv -> AdvCiv-SAS branch history is useful for ZIP-only/LLM investigation.
# Cache each rendered commit by immutable Git SHA so normal reruns only render newly created commits.
DEFAULT_COMMIT_DIFF_COUNT = -1  # -1 = all commits reachable from current HEAD; 0 = disabled; N = newest N reachable commits
COMMIT_DIFF_CACHE_FORMAT_VERSION = 2
COMMIT_DIFF_CACHE_DIR_NAME = "advciv_sas_light_source_commit_diffs"
# <!-- custom: Keep one canonical greppable history-context path for local agents and light-source ZIPs without duplicating Git history in tracked files.
# Exclude it from rendered Git patches and ordinary tree selection; the ZIP injects the freshly generated version at this same path and can refresh the Git-ignored local copy after a successful full-history archive. (GPT-5.6-Sol) -->
COMMIT_DIFF_CONTEXT_DIR = "LLM_Helpers/context/commit_diffs"
# <!-- custom: Include current map references and resumable source analyses in the light ZIP, but do not duplicate their imported/generated history inside generated historical patches. (ChatGPT-5.6-Sol + GPT-5.6-Sol) -->
MAP_REFERENCE_DIR = "LLM_Helpers/context/mapscript_refs"
SOURCE_ANALYSIS_DIR = "_1_AdvCiv-SAS/Docs/Source_Analysis"
# <!-- custom: Exclude both former and canonical context paths so the relocation commit itself cannot inject hundreds of megabytes of generated/imported context into future historical patches.
# Existing immutable-SHA cache entries remain valid because canonical paths did not exist in those commits. (GPT-5.6-Sol) -->
LEGACY_COMMIT_DIFF_EXCLUDED_PATHS = (
    "LLM_Helpers/commit_diffs/**",
    "LLM_Helpers/map_refs/**",
    f"{SOURCE_ANALYSIS_DIR}/**",
)
COMMIT_DIFF_EXCLUDED_PATHS = (
    f"{COMMIT_DIFF_CONTEXT_DIR}/**",
    f"{MAP_REFERENCE_DIR}/**",
    *LEGACY_COMMIT_DIFF_EXCLUDED_PATHS,
)
COMMIT_DIFF_MAX_FILE_PATCH_BYTES = 2 * 1024 * 1024
COMMIT_DIFF_MAX_FILE_CHANGED_LINES = 10_000
COMMIT_DIFF_MAX_COMMIT_PATCH_BYTES = 16 * 1024 * 1024
COMMIT_DIFF_ALWAYS_SUMMARIZE_PATH_PARTS = (
    "/sasgamerecord_log/",
    "/git_logs/",
    "/_2.1_doc_files_to_feed_chatgpt_at_each_new_session_not_exhaustive/",
    "/_2.2_source_files_to_feed_chatgpt_at_each_new_session_not_exhaustive/",
    "/git_log_repository_full.txt",
    "/git_log_anonymized_email.txt",
    "/manual.txt",
    "/leaders_data.py",
    "/sevopedialeadercachepredumped.py",
    "/sevopedialead_derexamplesofoutputs.txt",
)
COMMIT_DIFF_LARGE_NEW_FUNCTIONAL_SUFFIXES = (
    ".cpp", ".h", ".py", ".xml", ".md", ".ini", ".cfg", ".json", ".csv",
    ".bat", ".cmd", ".ps1", ".sh",
)
COMMIT_DIFF_ALWAYS_SUMMARIZE_SUFFIXES = (
    ".log", ".dll", ".fpk", ".pdb", ".obj", ".lib", ".exe", ".zip",
    ".odt", ".pdf", ".doc", ".docx", ".rtf",
)
GENERATED_ARCHIVE_MARKER = "_light_source_"
# Archive-only snapshot helpers so ZIP-only reviewers can distinguish repository files from
# generated context and can recover committed/staged/unstaged changes without `.git`.
GENERATED_CONTEXT_DIR = "_SNAPSHOT_CONTEXT"
GENERATED_CONTEXT_README_NAME = f"{GENERATED_CONTEXT_DIR}/README.txt"
GENERATED_GIT_MANIFEST_NAME = f"{GENERATED_CONTEXT_DIR}/repo_file_manifest.txt"
GENERATED_GIT_STATE_NAME = f"{GENERATED_CONTEXT_DIR}/git_repository_state.txt"
GENERATED_GIT_IGNORED_TREE_NAME = f"{GENERATED_CONTEXT_DIR}/git_ignored_paths_tree.txt"
GENERATED_STAGED_DIFF_NAME = f"{GENERATED_CONTEXT_DIR}/staged_changes_no_eol.diff"
GENERATED_UNSTAGED_DIFF_NAME = f"{GENERATED_CONTEXT_DIR}/unstaged_changes_no_eol.diff"
GENERATED_INCREMENTAL_GIT_LOG_NAME = f"{GENERATED_CONTEXT_DIR}/git_log_since_tracked_advciv_sas_log.txt"
# <!-- custom: Generate commit history freshly for the archive at its canonical shared repository path rather than duplicating it under _SNAPSHOT_CONTEXT. (GPT-5.6-Sol) -->
GENERATED_COMMIT_DIFF_DIR = COMMIT_DIFF_CONTEXT_DIR
GENERATED_COMMIT_DIFF_INDEX_NAME = f"{GENERATED_COMMIT_DIFF_DIR}/INDEX.txt"
GENERATED_PATH_HISTORY_INDEX_NAME = f"{GENERATED_COMMIT_DIFF_DIR}/PATH_HISTORY_INDEX.txt"
# Keep fetched-but-unmerged base AdvCiv release history separate from current-HEAD ancestry so
# ZIP-only reviewers can inspect upcoming merge changes without mistaking them for current source.
GENERATED_PENDING_UPSTREAM_DIR = f"{GENERATED_CONTEXT_DIR}/pending_upstream"
GENERATED_PENDING_UPSTREAM_INDEX_NAME = f"{GENERATED_PENDING_UPSTREAM_DIR}/INDEX.txt"
GENERATED_PENDING_UPSTREAM_LOG_NAME = f"{GENERATED_PENDING_UPSTREAM_DIR}/GIT_LOG.txt"
GENERATED_PENDING_UPSTREAM_PATH_INDEX_NAME = f"{GENERATED_PENDING_UPSTREAM_DIR}/PATH_HISTORY_INDEX.txt"
GENERATED_PENDING_UPSTREAM_REFS_NAME = f"{GENERATED_PENDING_UPSTREAM_DIR}/UPSTREAM_REFS.txt"
# Auto-detect ordinary release-style remote refs while deliberately ignoring topic/experimental
# branches. Explicit --upstream-ref remains the escape hatch if upstream naming ever changes.
UPSTREAM_RELEASE_REF_RE = re.compile(r"^upstream/(?:(?:v)|(?:release[-/]))?(\d+)\.(\d+)(?:\.(\d+))?$", re.IGNORECASE)
TRACKED_KMOD_GIT_LOG = "_0_Common_Docs/git_logs/git_log_anonymized_email_001_K-Mod.txt"
TRACKED_BASE_ADVCIV_GIT_LOG = "_0_Common_Docs/git_logs/git_log_anonymized_email_002_Base_AdvCiv.txt"
TRACKED_ADVCIV_SAS_GIT_LOG = "_1_AdvCiv-SAS/Docs/git_logs/git_log_anonymized_email_003_AdvCiv-SAS.txt"
HISTORY_SEGMENTS = (
    ("KMod", "K-Mod history", TRACKED_KMOD_GIT_LOG),
    ("AdvCivPreSAS", "pre-SAS AdvCiv history", TRACKED_BASE_ADVCIV_GIT_LOG),
    ("SASBranch", "AdvCiv-SAS branch history", TRACKED_ADVCIV_SAS_GIT_LOG),
)
HISTORY_TITLE_PREVIEW_CHARS = 160
# Generated Git-history context is meant to preserve code/history, not personal Git identities.
# Match ordinary Internet addresses plus local Git-style identities such as user@host.
GENERATED_HISTORY_EMAIL_RE = re.compile(r"(?i)(?<![A-Z0-9._%+\-])<?[A-Z0-9._%+\-]+@[A-Z0-9][A-Z0-9.\-]*>?(?![A-Z0-9._%+\-])")
GENERATED_HISTORY_EMAIL_PRIVACY_MARKER = "# Email privacy: email-shaped addresses redacted from generated commit-history text."
GENERATED_HISTORY_LAYOUT_MARKER = "# History metadata layout: 3 (history-segment label; title-only diff header; full message redirected to anonymized Git logs)."

# Skip Python bytecode/cache folders anywhere in the tree.
# They are generated, can be heavy, and confuse LLM/code-agent reviews with stale duplicate code.
SKIP_DIR_NAMES = {".git", "__pycache__"}

# Skip whole folders that are included through a parent folder but do not help compact LLM/code-agent review.
# Civ4 cursor assets are visual/binary UI files and are usually noise for source/debugging tasks.
SKIP_REL_DIRS = {"assets/res/cursors", COMMIT_DIFF_CONTEXT_DIR.lower()}

# Skip generated/binary payloads that are too heavy or not useful for compact ChatGPT/code-agent source review.
# FPK art packs and DLL binaries should be shared separately only when specifically needed.
SKIP_SUFFIXES = {".pyc", ".pyo", ".dll", ".fpk", ".tga"}

# Preserve this build-check/workflow folder and its tracked marker in light-source archives.
# Never include retained Debug-opt compiler intermediates or private symbols.
PRESERVED_LIGHT_SOURCE_TEMP_DIR = "CvGameCoreDLL/Project/temp_files"
PRESERVED_LIGHT_SOURCE_TEMP_MARKER = ".gitkeep"

# Visual Studio database files can be very large and are regenerated locally.
# Other small lone project files are useful enough to keep.
DLL_PROJECT_SKIP_SUFFIXES = {".sdf"}

# Skip original manuals by default because converted text copies are easier to grep for compact LLM/code-agent review.
# The exact base-AdvCiv manual.odt path is re-added below as a deliberate source-input exception for the repo-local text converter.
SKIP_FILE_NAMES = {"manual.pdf", "manual.odt"}

# <!-- custom: Keep this one binary office document in the light ZIP because convert_advciv_manual_to_txt.py consumes it directly; do not broaden this into general ODT inclusion. (ChatGPT-5.6-Sol) -->
LIGHT_SOURCE_EXACT_FILE_EXCEPTIONS = (
    "_0_Common_Docs/AdvCiv_Base_Doc/manual.odt",
)

# Do not exclude common readable image files globally.
# Small previews/screenshots can be useful for LLM review, e.g. GameFont previews.
# Avoid heavy art/image folders by not adding those folders to the include lists instead.
# TGA is excluded above because ChatGPT/code-agent review generally cannot inspect it usefully in this compact source archive.


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description="Create a timestamped Civ4 mod light source ZIP."
    )
    parser.add_argument(
        "--repo-root",
        default=None,
        help="Path to the mod/repo root. Defaults to auto-detection from cwd/script path.",
    )
    parser.add_argument(
        "--output-dir",
        default=DEFAULT_OUTPUT_DIR,
        help=(
            "Output directory, relative to the mod/repo root unless absolute. "
            f"Default: {DEFAULT_OUTPUT_DIR}"
        ),
    )
    parser.add_argument(
        "--mod-name",
        default=None,
        help=(
            "Archive filename mod name. Defaults to the detected mod folder name, "
            f"with an {DEFAULT_MOD_NAME} fallback."
        ),
    )
    parser.add_argument(
        "--prefix",
        default=DEFAULT_ARCHIVE_PREFIX,
        help=(
            "Full archive filename prefix before the timestamp. Defaults to "
            "<mod-name>_light_source. Use this only for manual labels or old naming."
        ),
    )
    parser.add_argument(
        "--compression-level",
        type=int,
        default=DEFAULT_COMPRESSION_LEVEL,
        choices=range(0, 10),
        metavar="0-9",
        help=(
            "ZIP_DEFLATED compression level. Default: "
            f"{DEFAULT_COMPRESSION_LEVEL}. Use 0 for ZIP_STORED / no compression."
        ),
    )
    parser.add_argument(
        "--commit-diff-count",
        type=int,
        default=DEFAULT_COMMIT_DIFF_COUNT,
        metavar="N",
        help=(
            f"Generated committed diffs to expose under {COMMIT_DIFF_CONTEXT_DIR}. "
            "Default: -1 = every commit reachable from the current HEAD (including K-Mod, pre-SAS AdvCiv and AdvCiv-SAS branch history); "
            "0 disables; positive N keeps only the newest N reachable commits. Rendered commits are cached "
            "locally by immutable Git SHA, so normal reruns only generate new commits."
        ),
    )
    parser.add_argument(
        "--no-sync-context",
        action="store_true",
        help=(
            f"Do not refresh the canonical Git-ignored local {COMMIT_DIFF_CONTEXT_DIR} directory after a successful full-history archive. "
            "Dry runs, disabled history, and positive/truncated commit counts never refresh it."
        ),
    )
    parser.add_argument(
        "--fetch-upstream",
        action="store_true",
        help=(
            "Run `git fetch upstream --prune` before snapshot generation so pending-upstream context uses fresh remote-tracking refs. "
            "This is opt-in because ordinary archive creation remains local/network-free by default."
        ),
    )
    parser.add_argument(
        "--upstream-ref",
        action="append",
        default=[],
        metavar="REF",
        help=(
            "Explicit fetched upstream revision/ref to include as pending release context outside an active merge; may be repeated. "
            "Useful if upstream release naming changes or a nonstandard maintenance branch should be reviewed. During an active merge, exact MERGE_HEAD wins."
        ),
    )
    parser.add_argument(
        "--dry-run",
        action="store_true",
        help="Print what would be archived without writing the ZIP.",
    )
    parser.add_argument(
        "--no-duration",
        action="store_true",
        help="Do not print total/snapshot-context/ZIP-write durations. Useful when stable/deterministic-looking command output is preferred.",
    )
    return parser.parse_args()


def find_repo_root(repo_root_arg: str | None) -> Path:
    if repo_root_arg:
        root = Path(repo_root_arg).expanduser().resolve()
        if not root.is_dir():
            raise SystemExit(f"Repo root does not exist or is not a directory: {root}")
        return root

    candidates: list[Path] = []
    try:
        candidates.append(Path.cwd().resolve())
    except OSError:
        pass

    script_path = Path(__file__).resolve()
    candidates.extend([script_path.parent, *script_path.parents])

    seen: set[Path] = set()
    for candidate in candidates:
        if candidate in seen:
            continue
        seen.add(candidate)
        if (candidate / "Assets").is_dir() or (candidate / DLL_TOP_LEVEL_DIR).is_dir():
            return candidate

    raise SystemExit("Could not auto-detect mod/repo root. Run from the mod root or pass --repo-root.")


def safe_filename_part(text: str | None, fallback: str) -> str:
    if text is None:
        return fallback
    cleaned = re.sub(r"[^A-Za-z0-9._-]+", "_", text.strip()).strip("._-")
    return cleaned or fallback


def derive_mod_name(repo_root: Path, mod_name_arg: str | None) -> str:
    """Return the filename-safe mod name used in the output archive name."""
    # Normally this comes from the mod folder itself, e.g. .../Mods/AdvCiv-SAS
    # -> AdvCiv-SAS. --mod-name exists only for unusual folder names or manual labels.
    return safe_filename_part(mod_name_arg or repo_root.name, DEFAULT_MOD_NAME)


def archive_prefix(mod_name: str, prefix_arg: str | None) -> str:
    """Return the filename-safe archive prefix before the timestamp."""
    if prefix_arg:
        return safe_filename_part(prefix_arg, f"{mod_name}_{ARCHIVE_LABEL}")
    return f"{mod_name}_{ARCHIVE_LABEL}"


def output_path(repo_root: Path, output_dir_arg: str, prefix: str, create_dir: bool) -> Path:
    out_dir = Path(output_dir_arg).expanduser()
    if not out_dir.is_absolute():
        out_dir = repo_root / out_dir
    if create_dir:
        out_dir.mkdir(parents=True, exist_ok=True)
    stamp = datetime.now().strftime("%Y%m%dT%H%M%S")
    return out_dir / f"{prefix}_{stamp}.zip"


def rel_for_message(path: Path, repo_root: Path) -> str:
    try:
        return path.relative_to(repo_root).as_posix()
    except ValueError:
        return str(path)


def run_git(repo_root: Path, *args: str) -> tuple[str | None, str | None]:
    """Run Git at repo_root and return stdout or a concise error."""
    try:
        result = subprocess.run(
            ["git", *args],
            cwd=repo_root,
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            text=True,
            encoding="utf-8",
            errors="replace",
            check=False,
        )
    except OSError as exc:
        return None, str(exc)

    if result.returncode != 0:
        error = result.stderr.strip() or f"git exited with status {result.returncode}"
        return None, error
    return result.stdout, None


def fetch_upstream_or_fail(repo_root: Path) -> None:
    """Refresh locally known upstream refs only when the caller explicitly requests network access."""
    try:
        result = subprocess.run(
            ["git", "fetch", "upstream", "--prune"],
            cwd=repo_root,
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            text=True,
            encoding="utf-8",
            errors="replace",
            check=False,
        )
    except OSError as exc:
        raise SystemExit(f"Unable to run `git fetch upstream --prune`: {exc}")
    if result.returncode != 0:
        detail = result.stderr.strip() or result.stdout.strip() or f"git exited with status {result.returncode}"
        raise SystemExit(f"`git fetch upstream --prune` failed; archive not created so pending-upstream context is not silently stale:\n{detail}")
    print("Fetched:   upstream --prune")


def list_upstream_refs(repo_root: Path) -> tuple[list[tuple[str, str]], str | None]:
    """Return locally fetched upstream remote-tracking refs as (short-ref, SHA)."""
    refs_raw, refs_error = run_git(
        repo_root, "for-each-ref", "--format=%(refname:short)%09%(objectname)", "refs/remotes/upstream/"
    )
    refs: list[tuple[str, str]] = []
    if refs_raw is not None:
        for line in refs_raw.splitlines():
            parts = line.split("\t", 1)
            if len(parts) == 2 and parts[0] and parts[1] and parts[0].strip() != "upstream/HEAD":
                refs.append((parts[0].strip(), parts[1].strip()))
    return refs, refs_error


def upstream_release_version(ref: str) -> tuple[int, int, int] | None:
    match = UPSTREAM_RELEASE_REF_RE.fullmatch(ref)
    if match is None:
        return None
    values = [int(value) if value is not None else 0 for value in match.groups()]
    return values[0], values[1], values[2]


def resolve_git_commit(repo_root: Path, revision: str) -> tuple[str | None, str | None]:
    raw, error = run_git(repo_root, "rev-parse", "--verify", f"{revision}^{{commit}}")
    return (raw.strip(), None) if raw else (None, error)


def git_is_ancestor(repo_root: Path, ancestor: str, descendant: str) -> bool:
    try:
        result = subprocess.run(
            ["git", "merge-base", "--is-ancestor", ancestor, descendant],
            cwd=repo_root, stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL, check=False,
        )
    except OSError:
        return False
    return result.returncode == 0


def detect_pending_upstream_targets(repo_root: Path, explicit_refs: Iterable[str] = ()) -> dict[str, object] | None:
    """Select exact merge target or a union of fetched release-like refs not yet represented by HEAD."""
    refs, refs_error = list_upstream_refs(repo_root)
    ref_sha = {ref: sha for ref, sha in refs}
    release_refs = sorted(
        [(upstream_release_version(ref), ref, sha) for ref, sha in refs if upstream_release_version(ref) is not None],
        key=lambda item: item[0],
    )
    ignored_refs = sorted((ref, sha) for ref, sha in refs if upstream_release_version(ref) is None)

    merge_raw, _ = run_git(repo_root, "rev-parse", "-q", "--verify", "MERGE_HEAD")
    merge_heads = [value.strip() for value in (merge_raw or "").splitlines() if value.strip()]
    if merge_heads:
        target_sha = merge_heads[0]
        matching_refs = [ref for ref, sha in refs if sha.lower() == target_sha.lower()]
        release_matches = [ref for ref in matching_refs if upstream_release_version(ref) is not None]
        target_ref = release_matches[0] if release_matches else (matching_refs[0] if matching_refs else "MERGE_HEAD")
        return {
            "mode": "merge",
            "selected_refs": [target_ref],
            "selected_revisions": [target_sha],
            "presentation_ref": target_ref,
            "presentation_sha": target_sha,
            "selection": "matched current MERGE_HEAD" if matching_refs else "current MERGE_HEAD (no matching fetched upstream ref)",
            "merge_head": target_sha,
            "merge_in_progress": "yes",
            "release_refs": release_refs,
            "ignored_refs": ignored_refs,
            "refs_error": refs_error or "",
        }

    explicit = [value.strip() for value in explicit_refs if value and value.strip()]
    if explicit:
        selected_refs: list[str] = []
        selected_revisions: list[str] = []
        for ref in explicit:
            sha, error = resolve_git_commit(repo_root, ref)
            if sha is None:
                raise SystemExit(f"Unable to resolve --upstream-ref {ref!r}: {error or 'unknown Git error'}")
            selected_refs.append(ref)
            selected_revisions.append(sha)
        return {
            "mode": "explicit",
            "selected_refs": selected_refs,
            "selected_revisions": selected_revisions,
            "presentation_ref": selected_refs[-1],
            "presentation_sha": selected_revisions[-1],
            "selection": "explicit --upstream-ref override",
            "merge_head": "",
            "merge_in_progress": "no",
            "release_refs": release_refs,
            "ignored_refs": ignored_refs,
            "refs_error": refs_error or "",
        }

    if not release_refs:
        return None
    _, presentation_ref, presentation_sha = release_refs[-1]
    # Use the UNION of all detected release-looking refs. In the ordinary linear case,
    # older releases contribute no extra commits beyond the newest; if release lines ever
    # diverge, their otherwise-missed commits remain visible instead of being silently lost.
    return {
        "mode": "auto-release-union",
        "selected_refs": [ref for _, ref, _ in release_refs],
        "selected_revisions": [sha for _, _, sha in release_refs],
        "presentation_ref": presentation_ref,
        "presentation_sha": presentation_sha,
        "selection": "union of all locally fetched release-like upstream refs; highest version used as presentation target",
        "merge_head": "",
        "merge_in_progress": "no",
        "release_refs": release_refs,
        "ignored_refs": ignored_refs,
        "refs_error": refs_error or "",
    }


def build_git_manifest(repo_root: Path) -> str:
    """Build an archive-only inventory of the local Git tracked file tree with current working-tree byte sizes."""
    tracked_raw, error = run_git(repo_root, "ls-files", "-z")
    if tracked_raw is None:
        return (
            "# Local Git tracked-file manifest\n"
            "# Generated automatically by LLM_Helpers/make_light_source_zip.py.\n"
            f"# This helper exists only inside {GENERATED_CONTEXT_DIR}/ in the light-source ZIP; it is not a repository file.\n"
            "# Git tracked-file metadata was unavailable while creating this archive.\n\n"
            f"Git error: {error}\n"
        )

    tracked = sorted(path for path in tracked_raw.split("\0") if path)
    rows: list[str] = []
    missing = 0
    for path in tracked:
        try:
            size = (repo_root / path).stat().st_size
            rows.append(f"{size}\t{path}")
        except OSError:
            missing += 1
            rows.append(f"MISSING\t{path}")
    lines = [
        "# Local Git tracked-file manifest",
        "# Generated automatically by LLM_Helpers/make_light_source_zip.py.",
        f"# This helper exists only inside {GENERATED_CONTEXT_DIR}/ in the light-source ZIP; it is not a repository file.",
        "# The light archive intentionally omits many tracked assets/binaries. Use this manifest",
        "# to distinguish 'not included in this ZIP' from 'not present in the local Git repository'.",
        "# Each row is <current-working-tree-bytes><TAB><canonical-Git-path>; MISSING means the tracked path has no current working-tree file.",
        "# Exact byte sizes are useful even for intentionally omitted binaries such as CvGameCoreDLL.dll.",
        "# Tracked paths preserve Git's canonical path spelling/casing.",
        "# Untracked paths are intentionally not enumerated; selected untracked source files can",
        "# still be present normally in the ZIP, without exposing unrelated local filenames here.",
        "",
        f"Tracked files: {len(tracked)}",
        f"Tracked paths missing from working tree: {missing}",
        "",
        "[TRACKED FILES - current bytes + git ls-files path]",
        *rows,
    ]
    return "\n".join(lines) + "\n"
def build_git_repository_state(repo_root: Path, selected_files: Iterable[Path], explicit_upstream_refs: Iterable[str] = ()) -> str:
    """Build compact repository, upstream, tracked-worktree, and selected-untracked state."""
    branch_raw, branch_error = run_git(repo_root, "rev-parse", "--abbrev-ref", "HEAD")
    head_raw, head_error = run_git(repo_root, "rev-parse", "HEAD")
    count_raw, count_error = run_git(repo_root, "rev-list", "--count", "HEAD")
    upstream_raw, upstream_error = run_git(
        repo_root, "rev-parse", "--abbrev-ref", "--symbolic-full-name", "@{u}"
    )
    ahead_behind_raw, ahead_behind_error = run_git(
        repo_root, "rev-list", "--left-right", "--count", "HEAD...@{u}"
    )
    status_raw, status_error = run_git(repo_root, "status", "--short", "--untracked-files=no")
    tracked_raw, tracked_error = run_git(repo_root, "ls-files", "-z")
    pending_upstream = detect_pending_upstream_targets(repo_root, explicit_upstream_refs)

    if branch_raw is None and head_raw is None and status_raw is None:
        error = branch_error or head_error or status_error or "Git metadata unavailable"
        return (
            "# Local Git repository state\n"
            "# Generated automatically by LLM_Helpers/make_light_source_zip.py.\n"
            f"# This helper exists only inside {GENERATED_CONTEXT_DIR}/ in the light-source ZIP; it is not a repository file.\n\n"
            f"Git error: {error}\n"
        )

    branch = branch_raw.strip() if branch_raw else "unknown"
    head = head_raw.strip() if head_raw else "unknown"
    commit_count = count_raw.strip() if count_raw else f"unknown ({count_error})"
    upstream = upstream_raw.strip() if upstream_raw else None
    ahead = behind = None
    if ahead_behind_raw:
        parts = ahead_behind_raw.split()
        if len(parts) >= 2:
            ahead, behind = parts[0], parts[1]

    selected_untracked: list[str] = []
    if tracked_raw is not None:
        tracked = {path for path in tracked_raw.split("\0") if path}
        selected_untracked = sorted(
            path.relative_to(repo_root).as_posix()
            for path in selected_files
            if path.relative_to(repo_root).as_posix() not in tracked
        )

    lines = [
        "# Local Git repository state",
        "# Generated automatically by LLM_Helpers/make_light_source_zip.py.",
        f"# This helper exists only inside {GENERATED_CONTEXT_DIR}/ in the light-source ZIP; it is not a repository file.",
        "# Upstream/ahead-behind values use the locally known upstream ref and can be stale until git fetch.",
        "# AdvCiv-SAS commonly uses the total commit count as its practical version number in documentation",
        "# (for example, 'requires AdvCiv-SAS X+'); HEAD remains the exact source-state identifier.",
        "# Short-status format uses two columns: X = index/staged state, Y = working-tree/unstaged state.",
        "# Examples: 'M ' = staged modification, ' M' = unstaged modification, 'MM' = staged and modified again.",
        "# General untracked paths are intentionally omitted; only untracked files already selected for this ZIP are listed below.",
        "",
        f"Branch: {branch}",
        f"HEAD: {head}",
        f"Commit count: {commit_count}",
    ]
    if upstream:
        lines.append(f"Upstream: {upstream}")
        if ahead is not None and behind is not None:
            lines.append(f"Ahead/behind upstream: {ahead} / {behind}")
        elif ahead_behind_error:
            lines.append(f"Ahead/behind upstream: unavailable ({ahead_behind_error})")
    else:
        lines.append(f"Upstream: none ({upstream_error or 'not configured'})")

    if pending_upstream is not None:
        lines.append(f"Merge in progress: {pending_upstream['merge_in_progress']}")
        if pending_upstream["merge_head"]:
            lines.append(f"MERGE_HEAD: {pending_upstream['merge_head']}")
            lines.append(f"Merge target: {pending_upstream['presentation_ref']} ({pending_upstream['selection']})")
        else:
            lines.append(f"Pending upstream presentation target: {pending_upstream['presentation_ref']} ({pending_upstream['selection']})")
            lines.append(f"Pending upstream selected refs: {', '.join(pending_upstream['selected_refs'])}")
    else:
        lines.append("Merge in progress: no")
        lines.append("Pending upstream release context target: none (no locally fetched release-like upstream ref detected)")

    lines.extend(["", "[TRACKED WORKING TREE STATUS - git status --short --untracked-files=no]"])
    if status_raw is not None:
        lines.extend(status_raw.rstrip().splitlines() or ["(clean)"])
    else:
        lines.append(f"(unavailable: {status_error})")

    lines.extend(["", "[SELECTED ZIP FILES NOT TRACKED BY GIT]"])
    if tracked_raw is None:
        lines.append(f"(unavailable: {tracked_error})")
    else:
        lines.extend(selected_untracked or ["(none)"])
    return "\n".join(lines) + "\n"


def render_path_tree(paths: Iterable[str]) -> list[str]:
    """Render slash-separated paths as a compact ASCII tree."""
    root: dict[str, dict] = {}
    directory_paths: set[str] = set()
    for raw_path in paths:
        is_directory = raw_path.endswith("/")
        clean_path = raw_path.rstrip("/")
        if not clean_path:
            continue
        parts = [part for part in clean_path.split("/") if part]
        node = root
        built: list[str] = []
        for part in parts:
            built.append(part)
            node = node.setdefault(part, {})
        if is_directory:
            directory_paths.add("/".join(built))

    lines = ["."]

    def walk(node: dict[str, dict], prefix: str, parent_parts: list[str]) -> None:
        names = sorted(node, key=str.lower)
        for index, name in enumerate(names):
            is_last = index == len(names) - 1
            current_parts = [*parent_parts, name]
            current_path = "/".join(current_parts)
            suffix = "/" if current_path in directory_paths else ""
            lines.append(prefix + ("`-- " if is_last else "|-- ") + name + suffix)
            children = node[name]
            if children:
                walk(children, prefix + ("    " if is_last else "|   "), current_parts)

    walk(root, "", [])
    return lines


def build_git_ignored_paths_tree(repo_root: Path) -> str:
    """Build a compact tree of paths ignored by the effective local Git ignore rules."""
    ignored_raw, error = run_git(
        repo_root,
        "ls-files",
        "--others",
        "--ignored",
        "--exclude-standard",
        "--directory",
        "-z",
    )
    lines = [
        "# Local Git ignored-path tree",
        "# Generated automatically by LLM_Helpers/make_light_source_zip.py.",
        f"# This helper exists only inside {GENERATED_CONTEXT_DIR}/ in the light-source ZIP; it is not a repository file.",
        "# Uses Git's effective standard ignore rules. Entire ignored directories can be collapsed to one entry,",
        "# which keeps this useful as context without enumerating every generated/build file underneath them.",
        "# This complements repo_file_manifest.txt: ignored local paths are neither tracked paths nor necessarily ZIP contents.",
        "",
    ]
    if ignored_raw is None:
        lines.append(f"Git ignored-path metadata unavailable: {error}")
        return "\n".join(lines) + "\n"

    ignored_paths = sorted(path for path in ignored_raw.split("\0") if path)
    lines.append(f"Ignored entries: {len(ignored_paths)}")
    lines.append("")
    lines.append("[IGNORED PATHS - git ls-files --others --ignored --exclude-standard --directory]")
    lines.extend(render_path_tree(ignored_paths) if ignored_paths else ["(none)"])
    return "\n".join(lines) + "\n"

def commit_diff_pathspec_args() -> list[str]:
    """Return Git pathspecs that keep generated history context out of review/history diffs."""
    return [".", *(f":(exclude,top){path}" for path in COMMIT_DIFF_EXCLUDED_PATHS)]


def build_git_diff(repo_root: Path, cached: bool) -> bytes:
    """Return a raw review diff while ignoring line-ending/trailing-EOL-only noise."""
    args = ["diff"]
    if cached:
        args.append("--cached")
    args.extend(
        [
            "--no-ext-diff",
            "--no-textconv",
            "--no-color",
            "--ignore-space-at-eol",
            "--ignore-cr-at-eol",
            "--",
            *commit_diff_pathspec_args(),
        ]
    )
    raw, error = run_git(repo_root, *args)
    if raw is None:
        # Keep archive creation useful even when the supplied folder is not a Git checkout.
        return f"# Git diff unavailable: {error}\n".encode("utf-8")
    return raw.encode("utf-8")


def latest_commit_in_tracked_git_log(repo_root: Path) -> tuple[str | None, str | None]:
    """Return the newest commit already recorded in the tracked AdvCiv-SAS Git log."""
    log_path = repo_root / TRACKED_ADVCIV_SAS_GIT_LOG
    if not log_path.is_file():
        return None, f"tracked Git log not found: {TRACKED_ADVCIV_SAS_GIT_LOG}"
    try:
        text = log_path.read_text(encoding="utf-8", errors="replace")
    except OSError as exc:
        return None, str(exc)
    match = re.search(r"(?m)^commit ([0-9a-fA-F]{40})\s*$", text)
    if not match:
        return None, f"no full commit hash found in {TRACKED_ADVCIV_SAS_GIT_LOG}"
    return match.group(1).lower(), None


def build_incremental_git_log(repo_root: Path) -> str:
    """Build compact history from the tracked SAS Git-log boundary through this snapshot's HEAD."""
    base_commit, base_error = latest_commit_in_tracked_git_log(repo_root)
    head_raw, head_error = run_git(repo_root, "rev-parse", "HEAD")
    head_commit = head_raw.strip() if head_raw else None

    lines = [
        "# Incremental AdvCiv-SAS Git history for this light-source snapshot",
        "# Generated automatically by LLM_Helpers/make_light_source_zip.py.",
        f"# Tracked history file: {TRACKED_ADVCIV_SAS_GIT_LOG}",
        "# This archive-only file fills the gap after that tracked history through snapshot HEAD.",
        "# Unlike the tracked AdvCiv-SAS Git log (newest -> oldest), this generated gap is",
        "# chronological (oldest -> newest), so snapshot HEAD is the final commit below.",
        "# It lists every intervening commit message without duplicating potentially huge source/XML patches.",
        "# Detailed commit messages can preserve implemented notes removed later from the temporary untracked changes_old.md and changes_new.md.",
        "# Author email addresses are intentionally hidden.",
        "",
    ]

    if base_commit is None:
        lines.append(f"Unable to determine tracked-log boundary: {base_error}")
        return "\n".join(lines) + "\n"
    if head_commit is None:
        lines.append(f"Unable to determine snapshot HEAD: {head_error}")
        return "\n".join(lines) + "\n"

    base_resolved_raw, base_resolved_error = run_git(repo_root, "rev-parse", base_commit)
    if base_resolved_raw is None:
        lines.append(f"Tracked-log boundary commit is unavailable in this checkout: {base_commit}")
        lines.append(f"Git error: {base_resolved_error}")
        return "\n".join(lines) + "\n"
    base_resolved = base_resolved_raw.strip()

    merge_base_raw, merge_base_error = run_git(repo_root, "merge-base", base_resolved, head_commit)
    if merge_base_raw is None:
        lines.append(f"Unable to verify tracked-log boundary against HEAD: {merge_base_error}")
        return "\n".join(lines) + "\n"
    if merge_base_raw.strip() != base_resolved:
        lines.append(f"Tracked-log boundary is not an ancestor of snapshot HEAD: {base_resolved}")
        lines.append(f"Snapshot HEAD: {head_commit}")
        return "\n".join(lines) + "\n"

    lines.extend(
        [
            f"Tracked-log boundary already recorded: {base_resolved}",
            f"Snapshot HEAD: {head_commit}",
            f"Git range below: {base_resolved}..{head_commit}",
            "Boundary commit is excluded below because it already exists in the tracked Git log.",
            "Order: oldest newer commit -> snapshot HEAD.",
            "",
        ]
    )

    if base_resolved == head_commit:
        lines.append("(No newer committed changes after the tracked Git-log boundary.)")
        return "\n".join(lines) + "\n"

    count_raw, _ = run_git(repo_root, "rev-list", "--count", f"{base_resolved}..{head_commit}")
    if count_raw:
        lines.append(f"Newer commits: {count_raw.strip()}")
        lines.append("")

    log_raw, log_error = run_git(
        repo_root,
        "log",
        "--reverse",
        "--no-patch",
        "--no-color",
        "--pretty=format:commit %H%nAuthor: %an <hidden>%nDate:   %ai%n%n%B",
        f"{base_resolved}..{head_commit}",
    )
    if log_raw is None:
        lines.append(f"Unable to generate incremental Git log: {log_error}")
        return "\n".join(lines) + "\n"

    lines.append("[ALL NEWER COMMITS - MESSAGES | OLDEST -> NEWEST]")
    lines.append(redact_generated_history_emails(log_raw.rstrip()))
    return "\n".join(lines) + "\n"



def redact_generated_history_emails(text: str) -> str:
    """Redact email-shaped strings from archive-only Git history without modifying repository files."""
    return GENERATED_HISTORY_EMAIL_RE.sub("<hidden-email>", text)

def compact_history_title(subject: str, max_chars: int = HISTORY_TITLE_PREVIEW_CHARS) -> str:
    """Return a short single-line commit-title preview; full messages remain in the canonical Git logs."""
    title = " ".join(subject.split()) or "(no commit title)"
    title = redact_generated_history_emails(title)
    if len(title) <= max_chars:
        return title
    return title[: max_chars - 3].rstrip() + "..."


def read_tracked_history_segments(repo_root: Path) -> tuple[dict[str, tuple[str, str, str]], list[str]]:
    """Map hashes recorded in the three anonymized history logs to segment id/label/log path."""
    by_hash: dict[str, tuple[str, str, str]] = {}
    warnings: list[str] = []
    for segment_id, segment_label, log_rel in HISTORY_SEGMENTS:
        log_path = repo_root / log_rel
        if not log_path.is_file():
            warnings.append(f"missing {segment_label} Git log: {log_rel}")
            continue
        try:
            text = log_path.read_text(encoding="utf-8", errors="replace")
        except OSError as exc:
            warnings.append(f"unable to read {segment_label} Git log {log_rel}: {exc}")
            continue
        for full_hash in re.findall(r"(?m)^commit ([0-9a-fA-F]{40})\s*$", text):
            by_hash[full_hash.lower()] = (segment_id, segment_label, log_rel)
    return by_hash, warnings


def assign_commit_history_segments(repo_root: Path, commits: list[dict[str, str]]) -> list[str]:
    """Assign exact history/log segments; these describe ancestry partition, not commit authorship."""
    tracked, warnings = read_tracked_history_segments(repo_root)
    recent_sas_hashes: set[str] = set()
    newest_tracked_sas, newest_error = latest_commit_in_tracked_git_log(repo_root)
    if newest_tracked_sas:
        merge_base_raw, merge_base_error = run_git(repo_root, "merge-base", newest_tracked_sas, "HEAD")
        if merge_base_raw is not None and merge_base_raw.strip().lower() == newest_tracked_sas.lower():
            raw, error = run_git(repo_root, "rev-list", f"{newest_tracked_sas}..HEAD")
            if raw is not None:
                recent_sas_hashes = {value.lower() for value in raw.splitlines() if value.strip()}
            else:
                warnings.append(f"unable to identify recent AdvCiv-SAS Git-log gap: {error}")
        elif merge_base_raw is None:
            warnings.append(f"unable to verify recent AdvCiv-SAS Git-log boundary: {merge_base_error}")
        else:
            warnings.append("newest tracked AdvCiv-SAS Git-log commit is not an ancestor of current HEAD")
    elif newest_error:
        warnings.append(newest_error)

    for commit in commits:
        commit_hash = commit["hash"].lower()
        tracked_info = tracked.get(commit_hash)
        if tracked_info is not None:
            segment_id, segment_label, log_rel = tracked_info
            message_location = log_rel
        elif commit_hash in recent_sas_hashes:
            segment_id, segment_label = "SASBranch", "AdvCiv-SAS branch history"
            message_location = GENERATED_INCREMENTAL_GIT_LOG_NAME
        else:
            segment_id, segment_label = "Other", "Other reachable ancestry"
            message_location = "the anonymized Git logs and snapshot Git-log context included with this archive"
        commit["history_segment_id"] = segment_id
        commit["history_segment_label"] = segment_label
        commit["message_location"] = message_location
    return warnings


def commit_diff_header(commit: dict[str, str], version: str, parent_label: str) -> list[str]:
    """Build lean diff metadata; canonical full commit messages stay in the anonymized Git logs."""
    message_location = commit.get("message_location") or "the anonymized Git logs included with this archive"
    return [
        "# AdvCiv-SAS reachable-history commit diff",
        GENERATED_HISTORY_EMAIL_PRIVACY_MARKER,
        GENERATED_HISTORY_LAYOUT_MARKER,
        f"# Cache format: {COMMIT_DIFF_CACHE_FORMAT_VERSION}",
        f"# Cache policy: {commit_diff_cache_policy_key()}",
        f"# Practical commit count: {version}",
        f"# Commit: {commit['hash']}",
        f"# History segment: {commit.get('history_segment_label', 'Other reachable ancestry')}",
        f"# Parent used for diff: {parent_label}",
        f"# Title: {compact_history_title(commit.get('subject', ''))}",
        f"# Full commit message/metadata: see {message_location}.",
        "# History segments describe ancestry/log partition, not authorship: SASBranch includes SAS work plus later upstream AdvCiv commits merged/imported into this branch.",
        "# Practical commit counts can repeat on divergent/merged history; the full Git SHA is the canonical unique commit identifier.",
        "# Diff policy: preserve textual source/docs/config history; summarize known generated/binary/noisy or exceptionally huge file patches.",
        "# Git textconv/external diff drivers are disabled; redundant document formats such as ODT/PDF are summarized rather than converted.",
    ]


def migrate_cached_commit_diff(cache_path: Path, commit: dict[str, str], write_cache: bool) -> tuple[bytes | Path, bool]:
    """Upgrade older SHA cache text in place without asking Git to re-render its unchanged patch."""
    try:
        with cache_path.open("rb") as handle:
            header = handle.read(4096).decode("utf-8", errors="replace")
    except OSError:
        return cache_path, False
    if GENERATED_HISTORY_LAYOUT_MARKER in header and GENERATED_HISTORY_EMAIL_PRIVACY_MARKER in header:
        return cache_path, False
    try:
        text = cache_path.read_text(encoding="utf-8", errors="replace")
    except OSError:
        return cache_path, False

    version_match = re.search(r"(?m)^# (?:Practical version|Practical commit count): (\d+)$", text)
    parent_match = re.search(r"(?m)^# Parent used for diff: (.+)$", text)
    summary_pos = text.find("[CHANGE SUMMARY - NUMSTAT + RENAME/MODE SUMMARY]")
    if not version_match or summary_pos < 0:
        # Unknown/very old helper cache layout: let normal validation/rendering recover it instead of guessing.
        return cache_path, False
    version = version_match.group(1)
    parent_label = parent_match.group(1).strip() if parent_match else ((commit.get("parents") or "").split() or ["(root commit)"])[0]
    body = commit_diff_header(commit, version, parent_label) + ["", text[summary_pos:].rstrip()]
    sanitized = (redact_generated_history_emails("\n".join(body)).rstrip() + "\n").encode("utf-8")
    if not write_cache:
        return sanitized, True
    temp_path = cache_path.with_suffix(".metadata.tmp")
    try:
        temp_path.write_bytes(sanitized)
        temp_path.replace(cache_path)
        return cache_path, True
    except OSError:
        try:
            if temp_path.exists():
                temp_path.unlink()
        except OSError:
            pass
        return sanitized, True


def commit_diff_cache_policy_key() -> str:
    """Fingerprint cache-affecting patch policy constants so tuning caps/exclusions does not reuse stale renderings."""
    # <!-- custom: Key relocation-equivalent exclusions by the former paths so the existing multi-thousand-commit SHA cache remains reusable.
	# Cached commits predate the canonical paths; new commits are rendered with both old/new exclusions above, while any future substantive filtering change still changes this policy key normally. (GPT-5.6-Sol) -->
    payload = repr((COMMIT_DIFF_CACHE_FORMAT_VERSION, COMMIT_DIFF_MAX_FILE_PATCH_BYTES, COMMIT_DIFF_MAX_FILE_CHANGED_LINES, COMMIT_DIFF_MAX_COMMIT_PATCH_BYTES, COMMIT_DIFF_ALWAYS_SUMMARIZE_PATH_PARTS, COMMIT_DIFF_ALWAYS_SUMMARIZE_SUFFIXES, COMMIT_DIFF_LARGE_NEW_FUNCTIONAL_SUFFIXES, LEGACY_COMMIT_DIFF_EXCLUDED_PATHS)).encode("utf-8")
    return hashlib.sha1(payload).hexdigest()[:12]


def commit_diff_cache_dir(repo_root: Path) -> tuple[Path | None, str | None]:
    """Return a local cache inside Git metadata, avoiding repository/.gitignore churn."""
    git_dir_raw, error = run_git(repo_root, "rev-parse", "--git-dir")
    if git_dir_raw is None:
        return None, error
    git_dir = Path(git_dir_raw.strip())
    if not git_dir.is_absolute():
        git_dir = (repo_root / git_dir).resolve()
    return git_dir / COMMIT_DIFF_CACHE_DIR_NAME / f"v{COMMIT_DIFF_CACHE_FORMAT_VERSION}_{commit_diff_cache_policy_key()}", None


def local_commit_diff_candidates(repo_root: Path) -> dict[str, list[Path]]:
    """Index valid-looking local generated mirror files by short SHA so a populated working copy can reuse them as a read-only cache."""
    local_dir = repo_root / COMMIT_DIFF_CONTEXT_DIR
    candidates: dict[str, list[Path]] = {}
    if not local_dir.is_dir():
        return candidates
    for path in local_dir.glob("*.diff"):
        match = re.search(r"_([0-9a-fA-F]{10})\.diff$", path.name)
        if match:
            candidates.setdefault(match.group(1).lower(), []).append(path)
    return candidates


def prune_superseded_commit_diff_cache_dirs(cache_dir: Path) -> tuple[int, list[str]]:
    """Remove obsolete helper-owned cache-policy directories after a successful writable build."""
    removed = 0
    errors: list[str] = []
    cache_root = cache_dir.parent
    try:
        siblings = list(cache_root.iterdir())
    except OSError as exc:
        return 0, [str(exc)]
    for sibling in siblings:
        if sibling == cache_dir or not sibling.is_dir() or not re.fullmatch(r"v\d+_[0-9a-f]{12}", sibling.name):
            continue
        try:
            shutil.rmtree(sibling)
            removed += 1
        except OSError as exc:
            errors.append(f"{sibling.name}: {exc}")
    return removed, errors


def parse_commit_history_metadata(repo_root: Path, revision: str = "HEAD", reverse: bool = False) -> tuple[list[dict[str, str]], str | None]:
    """Read hashes/parents/titles for one revision/range in a single Git process."""
    args = ["log", "--no-color", "--format=%H%x1f%P%x1f%s%x1e"]
    if reverse:
        args.append("--reverse")
    args.append(revision)
    raw, error = run_git(repo_root, *args)
    if raw is None:
        return [], error
    commits: list[dict[str, str]] = []
    for record in raw.split("\x1e"):
        record = record.strip("\r\n")
        if not record:
            continue
        parts = record.split("\x1f", 2)
        if len(parts) != 3:
            continue
        full_hash, parents, subject = parts
        commits.append({"hash": full_hash.strip(), "parents": parents.strip(), "subject": subject.strip()})
    return commits, None


def parse_commit_history_metadata_multi(repo_root: Path, revisions: Iterable[str], exclude_revision: str = "HEAD") -> tuple[list[dict[str, str]], str | None]:
    """Read one deduplicated union of commits reachable from revisions but not from exclude_revision."""
    revisions = [value for value in revisions if value]
    if not revisions:
        return [], None
    args = ["log", "--no-color", "--reverse", "--topo-order", "--format=%H%x1f%P%x1f%s%x1e", *revisions, f"^{exclude_revision}"]
    raw, error = run_git(repo_root, *args)
    if raw is None:
        return [], error
    commits: list[dict[str, str]] = []
    for record in raw.split("\x1e"):
        record = record.strip("\r\n")
        if not record:
            continue
        parts = record.split("\x1f", 2)
        if len(parts) == 3:
            full_hash, parents, subject = parts
            commits.append({"hash": full_hash.strip(), "parents": parents.strip(), "subject": subject.strip()})
    return commits, None


def historical_patch_label(patch_chunk: str) -> str:
    first_line = patch_chunk.splitlines()[0] if patch_chunk else "diff --git (unknown)"
    return first_line[len("diff --git "):] if first_line.startswith("diff --git ") else first_line


def historical_patch_changed_lines(patch_chunk: str) -> int:
    """Count real added/deleted hunk lines, excluding diff metadata such as +++/---."""
    return sum(1 for line in patch_chunk.splitlines() if (line.startswith("+") and not line.startswith("+++")) or (line.startswith("-") and not line.startswith("---")))


def should_summarize_historical_patch(patch_label: str, patch_bytes: int, patch_chunk: str) -> tuple[bool, str, int]:
    """Suppress generated/noisy or huge-history payloads while retaining substantial new functional files."""
    normalized = "/" + patch_label.lower().replace("\\", "/") + "/"
    changed_lines = historical_patch_changed_lines(patch_chunk)
    if any(part in normalized for part in COMMIT_DIFF_ALWAYS_SUMMARIZE_PATH_PARTS):
        return True, "known-generated/noisy-history", changed_lines
    label_clean = patch_label.lower().replace('"', "")
    label_tokens = label_clean.split()
    if any(token.endswith(COMMIT_DIFF_ALWAYS_SUMMARIZE_SUFFIXES) for token in label_tokens):
        return True, "binary/generated-file", changed_lines
    if changed_lines > COMMIT_DIFF_MAX_FILE_CHANGED_LINES:
        is_new_file = "new file mode " in patch_chunk
        is_functional_source = label_clean.endswith(COMMIT_DIFF_LARGE_NEW_FUNCTIONAL_SUFFIXES)
        if not (is_new_file and is_functional_source):
            return True, "file-patch-too-many-changed-lines", changed_lines
    if patch_bytes > COMMIT_DIFF_MAX_FILE_PATCH_BYTES:
        return True, "file-patch-too-large", changed_lines
    return False, "", changed_lines


def cached_commit_info(cache_path: Path, expected_hash: str) -> tuple[str, str] | None:
    """Validate a cache entry cheaply and return practical version plus patch coverage."""
    try:
        with cache_path.open("rb") as handle:
            header = handle.read(2048).decode("utf-8", errors="replace")
            handle.seek(0, 2)
            size = handle.tell()
            handle.seek(max(0, size - 1024))
            tail = handle.read().decode("utf-8", errors="replace")
    except OSError:
        return None
    hash_match = re.search(r"(?m)^# Commit: ([0-9a-fA-F]{40})$", header)
    version_match = re.search(r"(?m)^# (?:Practical version|Practical commit count): (\d+)$", header)
    cache_match = re.search(r"(?m)^# Cache format: (\d+)$", header)
    policy_match = re.search(r"(?m)^# Cache policy: ([0-9a-f]+)$", header)
    if not hash_match or hash_match.group(1).lower() != expected_hash.lower():
        return None
    if not cache_match or int(cache_match.group(1)) != COMMIT_DIFF_CACHE_FORMAT_VERSION or not policy_match or policy_match.group(1) != commit_diff_cache_policy_key() or not version_match:
        return None
    coverage_match = re.search(r"patchFiles=(\d+) embeddedFiles=(\d+) omittedFiles=(\d+)", tail)
    if coverage_match:
        embedded = int(coverage_match.group(2))
        omitted = int(coverage_match.group(3))
        coverage = "full" if omitted == 0 else ("partial" if embedded > 0 else "summary-only")
    else:
        coverage = "unknown"
    return version_match.group(1), coverage


def render_commit_diff(repo_root: Path, commit: dict[str, str]) -> tuple[bytes, str, str]:
    """Render one immutable commit patch plus compact metadata, returning bytes/version/status."""
    full_hash = commit["hash"]
    parents = [value for value in commit["parents"].split() if value]
    parent = parents[0] if parents else None
    version = commit.get("version", "")
    version_error = None
    if not version:
        version_raw, version_error = run_git(repo_root, "rev-list", "--count", full_hash)
        version = version_raw.strip() if version_raw else "unknown"

    # One Git process supplies both compact change summary and patches. Explicitly disable
    # textconv as well as external diffs: local/global Git configs can otherwise launch tools
    # such as odt2txt for historical documents that the light-source archive does not need.
    common_args = ["--numstat", "--summary", "--patch", "--no-ext-diff", "--no-textconv", "--no-color", "--ignore-space-at-eol", "--ignore-cr-at-eol"]
    if parent is None:
        combined_raw, combined_error = run_git(repo_root, "show", "--format=", *common_args, full_hash, "--", *commit_diff_pathspec_args())
        parent_label = "(root commit)"
    else:
        combined_raw, combined_error = run_git(repo_root, "diff", *common_args, "--find-renames", parent, full_hash, "--", *commit_diff_pathspec_args())
        parent_label = parent

    summary_raw = None
    patch_all_raw = None
    if combined_raw is not None:
        patch_start = combined_raw.find("diff --git ")
        if patch_start >= 0:
            summary_raw = combined_raw[:patch_start].rstrip()
            patch_all_raw = combined_raw[patch_start:]
        else:
            summary_raw = combined_raw.rstrip()
            patch_all_raw = ""

    body = commit_diff_header(commit, version, parent_label) + [
        "",
        "[CHANGE SUMMARY - NUMSTAT + RENAME/MODE SUMMARY]",
        (summary_raw if summary_raw is not None else f"(unavailable: {combined_error})").rstrip(),
        "",
        "[PATCHES]",
    ]

    patch_chunks: list[str] = []
    if patch_all_raw:
        starts = [match.start() for match in re.finditer(r"(?m)^diff --git ", patch_all_raw)]
        for i, chunk_start in enumerate(starts):
            chunk_end = starts[i + 1] if i + 1 < len(starts) else len(patch_all_raw)
            patch_chunks.append(patch_all_raw[chunk_start:chunk_end].rstrip())

    included = 0
    omitted = 0
    embedded_bytes = 0
    if combined_raw is None:
        body.append(f"(Patch unavailable: {combined_error})")
    for patch_chunk in patch_chunks:
        label = historical_patch_label(patch_chunk)
        patch_size = len(patch_chunk.encode("utf-8"))
        summarize, reason, changed_lines = should_summarize_historical_patch(label, patch_size, patch_chunk)
        if not summarize and embedded_bytes + patch_size > COMMIT_DIFF_MAX_COMMIT_PATCH_BYTES:
            summarize, reason = True, "commit-patch-budget"
        if summarize:
            omitted += 1
            body.extend(["", f"[PATCH OMITTED] file={label} reason={reason} changedLines={changed_lines} bytes={patch_size}"])
            continue
        included += 1
        embedded_bytes += patch_size
        body.extend(["", patch_chunk])

    if not patch_chunks:
        body.extend(["", "(No textual patch after end-of-line-noise filtering; this may be a metadata-only/merge commit or an EOL-only change.)"])
    status = "full" if omitted == 0 else ("partial" if included > 0 else "summary-only")
    body.extend(["", "[PATCH COVERAGE]", f"patchFiles={len(patch_chunks)} embeddedFiles={included} omittedFiles={omitted} embeddedPatchBytes={embedded_bytes}"])
    if version_error and version == "unknown":
        body.extend(["", f"[VERSION WARNING] {version_error}"])
    return (redact_generated_history_emails("\n".join(body)).rstrip() + "\n").encode("utf-8"), version, status



def historical_patch_paths(patch_label: str) -> list[str]:
    """Return old/new repo-relative paths encoded by one `diff --git` label, including ordinary spaces."""
    label = patch_label.strip()
    tokens: list[str]
    if label.startswith('"'):
        try:
            tokens = shlex.split(label)
        except ValueError:
            tokens = []
    elif label.startswith("a/") and " b/" in label:
        # Git does not quote ordinary spaces in `diff --git` labels. The final ` b/` normally
        # separates old/new paths; quoted unusual paths use the shlex branch above.
        split_at = label.rfind(" b/")
        tokens = [label[:split_at], label[split_at + 1:]]
    else:
        tokens = label.split(maxsplit=1)
    paths: list[str] = []
    for token in tokens[:2]:
        if token.startswith("a/") or token.startswith("b/"):
            token = token[2:]
        if token and token != "/dev/null" and token not in paths:
            paths.append(token)
    return paths


def changed_paths_from_commit_diff(value: bytes | Path) -> list[str]:
    """Extract every included/omitted patch path from one generated first-parent commit diff."""
    try:
        text = value.read_text(encoding="utf-8", errors="replace") if isinstance(value, Path) else value.decode("utf-8", errors="replace")
    except OSError:
        return []
    labels = re.findall(r"(?m)^diff --git (.+)$", text)
    labels.extend(re.findall(r"(?m)^\[PATCH OMITTED\] file=(.*?) reason=", text))
    paths: list[str] = []
    for label in labels:
        for path in historical_patch_paths(label):
            if path not in paths:
                paths.append(path)
    return paths


def build_path_history_index(path_history: dict[str, list[tuple[str, str, str]]]) -> bytes:
    """Build compact path -> commits navigation without duplicating patch text or commit messages."""
    lines = [
        "# Reachable path history index",
        "# Generated automatically by LLM_Helpers/make_light_source_zip.py.",
        "# Maps each historical path to commits whose generated first-parent diff touched that path.",
        "# Scope is current HEAD ancestry only; unrelated/unmerged branch refs are not included.",
        "# Each indented row: <segment> <practical-count> <short-sha>, newest -> oldest.",
        "# Practical counts can repeat on divergent/merged history; SHA is canonical. Inspect the matching .diff for rename/status details.",
        "",
    ]
    for path in sorted(path_history, key=str.casefold):
        lines.append(path)
        for segment_id, version, short_hash in path_history[path]:
            lines.append(f"  {segment_id} {version} {short_hash}")
        lines.append("")
    return ("\n".join(lines).rstrip() + "\n").encode("utf-8")


def render_pending_upstream_diff(
    repo_root: Path,
    commit: dict[str, str],
    presentation_ref: str,
    available_refs: Iterable[str],
    sequence: int,
    total: int,
) -> tuple[bytes, str]:
    """Render one fetched-but-unmerged upstream commit with explicitly non-HEAD metadata."""
    pending_commit = dict(commit)
    pending_commit["version"] = "0"
    pending_commit["history_segment_label"] = f"Pending upstream release history ({presentation_ref})"
    pending_commit["message_location"] = GENERATED_PENDING_UPSTREAM_LOG_NAME
    rendered, _, coverage = render_commit_diff(repo_root, pending_commit)
    text = rendered.decode("utf-8", errors="replace")
    summary_pos = text.find("[CHANGE SUMMARY - NUMSTAT + RENAME/MODE SUMMARY]")
    if summary_pos < 0:
        return rendered, coverage
    parents = [value for value in commit.get("parents", "").split() if value]
    parent_label = parents[0] if parents else "(root commit)"
    refs_text = ", ".join(available_refs) or presentation_ref
    header = [
        "# AdvCiv-SAS pending upstream release commit diff",
        GENERATED_HISTORY_EMAIL_PRIVACY_MARKER,
        f"# Presentation target: {presentation_ref}",
        f"# Reachable from selected ref(s): {refs_text}",
        "# IMPORTANT: this commit is fetched upstream context and is NOT reachable from snapshot HEAD yet.",
        f"# Pending sequence: {sequence} of {total} (oldest -> newest)",
        f"# Commit: {commit['hash']}",
        f"# Parent used for diff: {parent_label}",
        f"# Title: {compact_history_title(commit.get('subject', ''))}",
        f"# Full commit message/metadata: see {GENERATED_PENDING_UPSTREAM_LOG_NAME}.",
        "# Diff policy matches reachable commit history, but pending diffs are rendered archive-only and are not stored in the HEAD-history SHA cache.",
        "# Git textconv/external diff drivers are disabled; generated/binary/noisy or exceptionally huge payloads are summarized.",
        "",
        text[summary_pos:].rstrip(),
    ]
    return (redact_generated_history_emails("\n".join(header)).rstrip() + "\n").encode("utf-8"), coverage


def build_pending_upstream_path_history_index(path_history: dict[str, list[tuple[int, str]]], presentation_ref: str) -> bytes:
    """Build reverse path navigation for fetched-but-unmerged upstream release commits."""
    lines = [
        "# Pending upstream path history index",
        "# Generated automatically by LLM_Helpers/make_light_source_zip.py.",
        f"# Presentation target: {presentation_ref}",
        "# Scope: union of selected fetched upstream release/override refs, excluding commits already reachable from snapshot HEAD.",
        "# These commits are context for review/merge work and are NOT current source ancestry yet.",
        "# Each indented row: <sequence> <short-sha>, oldest -> newest.",
        "",
    ]
    for path in sorted(path_history, key=str.casefold):
        lines.append(path)
        for sequence, short_hash in path_history[path]:
            lines.append(f"  {sequence} {short_hash}")
        lines.append("")
    return ("\n".join(lines).rstrip() + "\n").encode("utf-8")


def pending_ref_ahead_count(repo_root: Path, revision: str) -> str:
    raw, error = run_git(repo_root, "rev-list", "--count", f"HEAD..{revision}")
    return raw.strip() if raw else f"unavailable ({error})"


def build_pending_upstream_refs_report(repo_root: Path, target: dict[str, object] | None) -> bytes:
    """Explain detected release refs, selected refs, and ignored topic/experimental refs."""
    lines = [
        "# Locally fetched upstream ref classification",
        "# Generated automatically by LLM_Helpers/make_light_source_zip.py.",
        "# This is discovery/context only. Non-release/topic refs are NOT dumped into pending release diffs by default.",
        "# Remote-tracking refs can be stale until `git fetch upstream --prune` or `--fetch-upstream` is used.",
        "# Auto release patterns currently accept upstream/X.Y[.Z], upstream/vX.Y[.Z], upstream/release-X.Y[.Z], and upstream/release/X.Y[.Z].",
        "# If upstream naming changes or a special maintenance line matters, pass one or more explicit --upstream-ref REF options.",
        "",
    ]
    if target is None:
        refs, refs_error = list_upstream_refs(repo_root)
        lines.append("Selected pending refs: (none)")
        if refs_error:
            lines.append(f"Ref discovery warning: {refs_error}")
        lines.append("")
        lines.append("[LOCALLY FETCHED UPSTREAM REFS]")
        for ref, sha in refs:
            lines.append(f"{ref}\t{sha[:10]}\tahead={pending_ref_ahead_count(repo_root, ref)}")
        return ("\n".join(lines).rstrip() + "\n").encode("utf-8")

    selected_refs = list(target["selected_refs"])
    presentation_ref = str(target["presentation_ref"])
    presentation_sha = str(target["presentation_sha"])
    lines.extend([
        f"Selection mode: {target['mode']}",
        f"Selection reason: {target['selection']}",
        f"Presentation target: {presentation_ref} ({presentation_sha[:10]})",
        f"Selected pending refs: {', '.join(selected_refs)}",
        "",
        "[AUTO-DETECTED RELEASE-LIKE REFS]",
    ])
    release_refs = list(target["release_refs"])
    if release_refs:
        for version, ref, sha in release_refs:
            if ref == presentation_ref:
                relation = "presentation-target"
            elif git_is_ancestor(repo_root, ref, presentation_ref):
                relation = "contained-in-presentation-target"
            else:
                relation = "divergent-or-not-contained; union preserves any extra commits"
            selected = "selected" if ref in selected_refs else "not-selected"
            lines.append(f"{ref}\t{sha[:10]}\tversion={'.'.join(map(str, version))}\tahead={pending_ref_ahead_count(repo_root, ref)}\t{selected}\t{relation}")
    else:
        lines.append("(none)")

    lines.extend(["", "[OTHER/TOPIC UPSTREAM REFS - awareness only, not pending release diff input]"])
    ignored_refs = list(target["ignored_refs"])
    if ignored_refs:
        for ref, sha in ignored_refs:
            selected = "explicitly-selected" if ref in selected_refs else "ignored-by-auto-release-policy"
            lines.append(f"{ref}\t{sha[:10]}\tahead={pending_ref_ahead_count(repo_root, ref)}\t{selected}")
    else:
        lines.append("(none)")
    return ("\n".join(lines).rstrip() + "\n").encode("utf-8")


def build_pending_upstream_context(repo_root: Path, explicit_refs: Iterable[str] = ()) -> tuple[dict[str, bytes | Path], str]:
    """Expose fetched base-AdvCiv release commits that have not yet entered current HEAD."""
    target = detect_pending_upstream_targets(repo_root, explicit_refs)
    generated: dict[str, bytes | Path] = {}
    generated[GENERATED_PENDING_UPSTREAM_REFS_NAME] = build_pending_upstream_refs_report(repo_root, target)
    if target is None:
        text = (
            "# Pending upstream release commit index\n"
            "# Generated automatically by LLM_Helpers/make_light_source_zip.py.\n"
            "# No locally fetched release-like upstream ref was detected. Run `git fetch upstream --prune`, use --fetch-upstream, or pass --upstream-ref REF before relying on this context.\n"
        )
        generated[GENERATED_PENDING_UPSTREAM_INDEX_NAME] = text.encode("utf-8")
        generated[GENERATED_PENDING_UPSTREAM_LOG_NAME] = b"# Pending upstream Git history\n# No selected pending release refs.\n"
        generated[GENERATED_PENDING_UPSTREAM_PATH_INDEX_NAME] = build_pending_upstream_path_history_index({}, "(none)")
        return generated, "pending upstream: no fetched release-like ref detected"

    presentation_ref = str(target["presentation_ref"])
    selected_refs = list(target["selected_refs"])
    selected_revisions = list(dict.fromkeys(str(value) for value in target["selected_revisions"]))
    head_raw, head_error = run_git(repo_root, "rev-parse", "HEAD")
    head = head_raw.strip() if head_raw else "unknown"
    commits, history_error = parse_commit_history_metadata_multi(repo_root, selected_revisions, "HEAD")

    # One rev-list per selected ref provides compact commit->release membership without
    # duplicating commits in the union. This remains cheap for normal numbered release sets.
    membership: dict[str, list[str]] = {}
    for ref, revision in zip(selected_refs, target["selected_revisions"]):
        raw, _ = run_git(repo_root, "rev-list", str(revision), "^HEAD")
        for sha in (raw or "").splitlines():
            membership.setdefault(sha.strip().lower(), []).append(ref)

    newest_count_raw, _ = run_git(repo_root, "rev-list", "--count", f"HEAD..{target['presentation_sha']}")
    newest_count = int(newest_count_raw.strip()) if newest_count_raw and newest_count_raw.strip().isdigit() else 0
    extra_union = max(0, len(commits) - newest_count)
    selected_text = ", ".join(selected_refs)
    conceptual_range = f"union({selected_text}) minus HEAD"

    index_lines = [
        "# Pending upstream release commit index",
        "# Generated automatically by LLM_Helpers/make_light_source_zip.py.",
        "# This folder is deliberately separate from commit_diffs/: its commits are NOT reachable from snapshot HEAD yet.",
        "# During a merge, exact MERGE_HEAD overrides release discovery so the archive describes the merge actually being resolved.",
        "# Outside a merge, auto mode unions ALL locally fetched release-like refs and deduplicates by Git SHA.",
        "# Ordinarily older releases are ancestors of the highest version and add nothing; if release lines diverge, otherwise-missed commits remain included.",
        "# Topic/experimental refs are awareness-only in UPSTREAM_REFS.txt unless explicitly selected with --upstream-ref.",
        "# Local remote-tracking refs can be stale until `git fetch upstream --prune` or --fetch-upstream is used.",
        f"# Snapshot HEAD: {head}",
        f"# Presentation target: {presentation_ref}",
        f"# Presentation target SHA: {target['presentation_sha']}",
        f"# Selected refs: {selected_text}",
        f"# Target selection: {target['selection']}",
        f"# Merge in progress: {target['merge_in_progress']}",
        f"# Conceptual range: {conceptual_range}",
        f"# Additional union commits not reachable from presentation target: {extra_union}",
        "# Files are named <sequence>_<short-sha>.diff in chronological/topological oldest -> newest order.",
        "# Columns: sequence<TAB>short-sha<TAB>coverage<TAB>release-ref-membership<TAB>title-preview",
        "",
    ]
    if head_raw is None:
        index_lines.append(f"# HEAD warning: {head_error}")
    if history_error:
        index_lines.append(f"# History warning: {history_error}")

    total = len(commits)
    index_lines.append(f"Pending commits (deduplicated union): {total}")
    if total == 0:
        index_lines.append("(Snapshot HEAD already contains all commits from every selected pending ref.)")
        generated[GENERATED_PENDING_UPSTREAM_INDEX_NAME] = ("\n".join(index_lines).rstrip() + "\n").encode("utf-8")
        generated[GENERATED_PENDING_UPSTREAM_LOG_NAME] = (
            f"# Pending upstream Git history\n# No commits in {conceptual_range}.\n"
        ).encode("utf-8")
        generated[GENERATED_PENDING_UPSTREAM_PATH_INDEX_NAME] = build_pending_upstream_path_history_index({}, presentation_ref)
        return generated, f"pending upstream: 0 commit(s); presentation={presentation_ref}; selected={len(selected_refs)} ref(s)"

    log_args = [
        "log", "--reverse", "--topo-order", "--no-patch", "--no-color",
        "--pretty=format:commit %H%nAuthor: %an <hidden>%nDate:   %ai%n%n%B",
        *selected_revisions, "^HEAD",
    ]
    log_raw, log_error = run_git(repo_root, *log_args)
    log_lines = [
        f"# Pending upstream Git history; presentation target {presentation_ref}",
        "# Generated automatically by LLM_Helpers/make_light_source_zip.py.",
        "# These commits are fetched upstream context and are NOT reachable from snapshot HEAD yet.",
        f"# Snapshot HEAD: {head}",
        f"# Selected refs: {selected_text}",
        f"# Conceptual range: {conceptual_range}",
        "# Order: oldest/topological -> newest. Duplicate commits reachable from multiple release refs appear once. Author email addresses are intentionally hidden.",
        "",
    ]
    if log_raw is None:
        log_lines.append(f"Unable to generate pending upstream Git log: {log_error}")
    else:
        log_lines.append("[PENDING UPSTREAM COMMITS - MESSAGES | OLDEST -> NEWEST]")
        log_lines.append(redact_generated_history_emails(log_raw.rstrip()))
    generated[GENERATED_PENDING_UPSTREAM_LOG_NAME] = ("\n".join(log_lines).rstrip() + "\n").encode("utf-8")

    path_history: dict[str, list[tuple[int, str]]] = {}
    width = max(3, len(str(total)))
    for sequence, commit in enumerate(commits, 1):
        commit_refs = membership.get(commit["hash"].lower(), [])
        value, coverage = render_pending_upstream_diff(repo_root, commit, presentation_ref, commit_refs, sequence, total)
        short_hash = commit["hash"][:10]
        rel_name = f"{GENERATED_PENDING_UPSTREAM_DIR}/{sequence:0{width}d}_{short_hash}.diff"
        generated[rel_name] = value
        refs_preview = ",".join(commit_refs) if commit_refs else "(selected-union)"
        index_lines.append(
            f"{sequence:0{width}d}\t{short_hash}\t{coverage}\t{refs_preview}\t{compact_history_title(commit.get('subject', ''))}"
        )
        for path in changed_paths_from_commit_diff(value):
            path_history.setdefault(path, []).append((sequence, short_hash))

    generated[GENERATED_PENDING_UPSTREAM_INDEX_NAME] = ("\n".join(index_lines).rstrip() + "\n").encode("utf-8")
    generated[GENERATED_PENDING_UPSTREAM_PATH_INDEX_NAME] = build_pending_upstream_path_history_index(path_history, presentation_ref)
    return generated, (
        f"pending upstream: {total} commit(s) union from {len(selected_refs)} selected ref(s); "
        f"presentation={presentation_ref}; extra-vs-presentation={extra_union}"
    )
def assign_dag_practical_versions(commits: list[dict[str, str]]) -> bool:
    """Assign exact `git rev-list --count <commit>` values from the already-read reachable DAG."""
    if not commits:
        return False
    index_by_hash = {commit["hash"].lower(): i for i, commit in enumerate(commits)}
    reachable_bits: dict[str, int] = {}
    for commit in reversed(commits):
        commit_hash = commit["hash"].lower()
        bits = 1 << index_by_hash[commit_hash]
        for parent in (value.lower() for value in commit["parents"].split() if value):
            parent_bits = reachable_bits.get(parent)
            if parent_bits is None:
                return False
            bits |= parent_bits
        reachable_bits[commit_hash] = bits
        commit["version"] = str(bits.bit_count())
    return True

def build_commit_diff_history_context(repo_root: Path, commit_count: int, write_cache: bool) -> tuple[dict[str, bytes | Path], str]:
    """Build current-HEAD reachable commit-patch ancestry, reusing immutable SHA-keyed local cache entries."""
    if commit_count == 0:
        return {}, "commit diff history disabled"

    commits, history_error = parse_commit_history_metadata(repo_root)
    if not commits:
        message = f"Git history unavailable: {history_error or 'no commits found'}"
        index = "# Reachable commit diff index\n# Generated automatically by LLM_Helpers/make_light_source_zip.py.\n\n" + message + "\n"
        return {GENERATED_COMMIT_DIFF_INDEX_NAME: index.encode("utf-8")}, message

    dag_versions = assign_dag_practical_versions(commits)
    segment_warnings = assign_commit_history_segments(repo_root, commits)
    if commit_count > 0:
        commits = commits[:commit_count]

    cache_dir, cache_error = commit_diff_cache_dir(repo_root)
    cache_writable = bool(cache_dir and write_cache)
    if cache_writable:
        try:
            cache_dir.mkdir(parents=True, exist_ok=True)
        except OSError as exc:
            cache_error = str(exc)
            cache_writable = False

    generated: dict[str, bytes | Path] = {}
    local_candidates = local_commit_diff_candidates(repo_root)
    index_lines = [
        "# Reachable commit diff index",
        "# Generated automatically by LLM_Helpers/make_light_source_zip.py.",
        "# Scope: selected commits reachable from the snapshot's current HEAD only; unrelated/unmerged branch refs are excluded.",
        "# Merged side-branch commits remain included because they genuinely contribute to current HEAD ancestry; each commit is diffed against its first parent.",
        "# Segments: KMod = K-Mod history; AdvCivPreSAS = base AdvCiv history before the SAS branch; SASBranch = post-fork AdvCiv-SAS branch history.",
        "# SASBranch is a history/log segment, not an authorship label: it includes SAS commits plus later upstream AdvCiv commits merged/imported into the branch.",
        f"# Full commit messages/metadata: {TRACKED_KMOD_GIT_LOG}; {TRACKED_BASE_ADVCIV_GIT_LOG}; {TRACKED_ADVCIV_SAS_GIT_LOG}.",
        f"# Recent AdvCiv-SAS messages not yet in the tracked SAS log: {GENERATED_INCREMENTAL_GIT_LOG_NAME}.",
        "# Practical commit counts can repeat on divergent/merged history; the full Git SHA is the canonical unique commit identifier.",
        "# Files are named <segment>_<practical-count>_<short-sha>.diff; coverage is full, partial, summary-only, or unknown.",
        f"# Self-recursion guard: {COMMIT_DIFF_CONTEXT_DIR}/ is excluded from these historical patches because it is only a generated local mirror of this same data.",
        "# Columns: segment<TAB>practical-count<TAB>short-sha<TAB>coverage<TAB>title-preview",
    ]
    if segment_warnings:
        index_lines.append("# History-segment warning: " + " | ".join(segment_warnings))
    index_lines.append("")

    path_history: dict[str, list[tuple[str, str, str]]] = {}
    reused = mirror_reused = rendered = in_memory = cache_migrations = 0
    segment_counts: dict[str, int] = {}
    for commit in commits:
        full_hash = commit["hash"]
        segment_id = commit.get("history_segment_id", "Other")
        segment_counts[segment_id] = segment_counts.get(segment_id, 0) + 1
        cache_path = cache_dir / f"{full_hash}.diff" if cache_dir else None
        cached_info = cached_commit_info(cache_path, full_hash) if cache_path and cache_path.is_file() else None
        coverage = cached_info[1] if cached_info else "unknown"
        version = cached_info[0] if cached_info else None
        value: bytes | Path
        if version is not None and cache_path is not None:
            value, cache_migrated = migrate_cached_commit_diff(cache_path, commit, write_cache=cache_writable)
            cache_migrations += int(cache_migrated)
            reused += 1
        else:
            # <!-- custom: A populated local mirror can reuse validated history immediately; keep the private cache for new/unsynced SHAs. (ChatGPT-5.6-Sol) -->
            mirror_path = None
            mirror_info = None
            for candidate in local_candidates.get(full_hash[:10].lower(), []):
                candidate_info = cached_commit_info(candidate, full_hash)
                if candidate_info is not None:
                    mirror_path = candidate
                    mirror_info = candidate_info
                    break
            if mirror_path is not None and mirror_info is not None:
                value = mirror_path
                version, coverage = mirror_info
                mirror_reused += 1
            else:
                data, version, coverage = render_commit_diff(repo_root, commit)
                rendered += 1
                if cache_writable and cache_path is not None:
                    temp_path = cache_path.with_suffix(".tmp")
                    try:
                        temp_path.write_bytes(data)
                        temp_path.replace(cache_path)
                        value = cache_path
                    except OSError:
                        try:
                            if temp_path.exists():
                                temp_path.unlink()
                        except OSError:
                            pass
                        value = data
                        in_memory += 1
                else:
                    value = data
                    in_memory += 1
        rel_name = f"{GENERATED_COMMIT_DIFF_DIR}/{segment_id}_{version}_{full_hash[:10]}.diff"
        generated[rel_name] = value
        safe_subject = compact_history_title(commit.get("subject", ""))
        index_lines.append(f"{segment_id}\t{version}\t{full_hash[:10]}\t{coverage}\t{safe_subject}")
        for path in changed_paths_from_commit_diff(value):
            path_history.setdefault(path, []).append((segment_id, version, full_hash[:10]))

    pruned_cache_dirs = 0
    prune_errors: list[str] = []
    if cache_writable and cache_dir is not None and in_memory == 0:
        pruned_cache_dirs, prune_errors = prune_superseded_commit_diff_cache_dirs(cache_dir)

    generated[GENERATED_COMMIT_DIFF_INDEX_NAME] = ("\n".join(index_lines).rstrip() + "\n").encode("utf-8")
    generated[GENERATED_PATH_HISTORY_INDEX_NAME] = build_path_history_index(path_history)
    cache_note = str(cache_dir) if cache_dir else f"unavailable ({cache_error})"
    version_mode = "dag-from-one-log" if dag_versions else "per-commit-fallback"
    prune_note = f"; pruned={pruned_cache_dirs} old cache dir(s)" if pruned_cache_dirs else ""
    if prune_errors:
        prune_note += f"; prune warnings={len(prune_errors)}"
    migration_note = f"; cache-migrated={cache_migrations}" if cache_migrations else ""
    segment_note = ",".join(f"{key}:{segment_counts[key]}" for key in ("SASBranch", "AdvCivPreSAS", "KMod", "Other") if key in segment_counts)
    if segment_warnings:
        segment_note += f"; segment warnings={len(segment_warnings)}"
    summary = f"commit diffs: {len(commits)} included ({segment_note}), {reused} private-cache hit(s), {mirror_reused} local-mirror hit(s), {rendered} rendered, {in_memory} not cached; versions={version_mode}; cache={cache_note}{migration_note}{prune_note}"
    return generated, summary


def build_snapshot_context_readme() -> str:
    """Explain the generated archive-only snapshot context files."""
    return (
        "AdvCiv-SAS light-source snapshot context\n"
        "========================================\n\n"
        "This folder is generated only inside the light-source ZIP. It is not part of the repository.\n\n"
        "repo_file_manifest.txt\n"
        "  Canonical tracked Git paths from git ls-files, each prefixed by its exact current working-tree byte size.\n"
        "  This includes size context for tracked binaries/assets intentionally omitted from the light ZIP; missing tracked\n"
        "  working-tree paths are marked MISSING.\n\n"
        "git_repository_state.txt\n"
        "  Current branch/HEAD, commit count, locally known upstream/ahead-behind state, active MERGE_HEAD/target\n"
        "  when merging, tracked git status --short output, and ZIP-selected files that are not tracked by Git. X is\n"
        "  staged/index state; Y is unstaged/working-tree state (for example M<space>, <space>M, and MM).\n"
        "  AdvCiv-SAS commonly uses Commit count as its practical version number (for example X in\n"
        "  'requires AdvCiv-SAS X+'); HEAD is the exact source-state identifier.\n\n"
        "git_ignored_paths_tree.txt\n"
        "  Compact ASCII tree of paths ignored by Git's effective standard ignore rules. Entire ignored\n"
        "  directories can be collapsed to one entry, so this adds useful local context without listing\n"
        "  every generated/build file beneath them.\n\n"
        "staged_changes_no_eol.diff\n"
        "  Raw staged diff (HEAD -> index), with end-of-line whitespace/CR-only noise ignored. Changes to the generated\n"
        f"  history context at {COMMIT_DIFF_CONTEXT_DIR}/ are omitted here so hundreds of MB of patch text do not recur;\n"
        "  git_ignored_paths_tree.txt can still show that the local generated context directory exists. An empty file means no other staged changes.\n\n"
        "unstaged_changes_no_eol.diff\n"
        "  Raw unstaged diff (index -> working tree), with the same EOL-noise and generated-history-context exclusions.\n"
        "  An empty file means there were no other unstaged tracked changes.\n\n"
        "git_log_since_tracked_advciv_sas_log.txt\n"
        f"  Commit messages after the newest commit already recorded in {TRACKED_ADVCIV_SAS_GIT_LOG}\n"
        "  through this snapshot's HEAD. The boundary commit is not duplicated. Unlike the tracked\n"
        "  AdvCiv-SAS Git log (newest -> oldest), this generated gap is ordered oldest -> newest,\n"
        "  so snapshot HEAD appears at the bottom. Together with the tracked K-Mod, AdvCiv and\n"
        "  AdvCiv-SAS anonymized Git logs, these are the canonical full commit messages for history review.\n"
        f"\n../{COMMIT_DIFF_CONTEXT_DIR}/INDEX.txt + <segment>_<practical-count>_<short-sha>.diff\n"
        "  Filtered first-parent textual diffs for every selected commit reachable from current HEAD by default,\n"
        "  spanning K-Mod -> pre-SAS AdvCiv -> AdvCiv-SAS branch history. The last segment includes SAS work plus\n"
        "  later upstream AdvCiv commits merged/imported after SAS began. Unrelated/unmerged branch refs are not\n"
        "  included; merged side-branch commits remain because they genuinely contribute to current HEAD. Diff files\n"
        "  keep a short title, change summary and useful patches, while full messages/metadata redirect to the tracked\n"
        "  anonymized Git logs above (or the generated recent SAS gap). This avoids duplicating long commit messages.\n"
        "  Known generated/log/binary or exceptionally huge historical payloads are summarized instead of embedded.\n"
        f"  This canonical generated context lives at {COMMIT_DIFF_CONTEXT_DIR}/ locally for code agents, and the light ZIP\n"
        "  injects a freshly generated copy at that same path rather than duplicating it under _SNAPSHOT_CONTEXT. Normal\n"
        "  full-history ZIP creation refreshes the Git-ignored local directory after the archive succeeds. The directory is\n"
        "  excluded from its generated patches and ordinary tree selection, preventing recursive history-of-history growth.\n"
        "  Practical commit counts can repeat on divergent/merged history; the full Git SHA is always canonical.\n"
        f"\n../{COMMIT_DIFF_CONTEXT_DIR}/PATH_HISTORY_INDEX.txt\n"
        "  Compact reverse navigation from a historical repository path to the commits whose generated first-parent\n"
        "  diffs touched it. Use it to narrow investigation before opening the matching commit diff files.\n"
        "\npending_upstream/INDEX.txt + GIT_LOG.txt + PATH_HISTORY_INDEX.txt + UPSTREAM_REFS.txt + <sequence>_<short-sha>.diff\n"
        "  Separate fetched-but-unmerged base-AdvCiv context. During an active merge, exact MERGE_HEAD is authoritative.\n"
        "  Outside a merge, all locally fetched release-like refs are considered and their HEAD-external commits are\n"
        "  unioned/deduplicated by SHA; the highest detected version is only a presentation target, so divergent older\n"
        "  release lines cannot silently lose unique commits. Topic/experimental refs are listed in UPSTREAM_REFS.txt but\n"
        "  are not treated as releases merely because they are newer; --upstream-ref can explicitly select unusual naming\n"
        f"  or special maintenance lines. These commits are deliberately NOT mixed into {COMMIT_DIFF_CONTEXT_DIR}/ because they are not\n"
        "  current-HEAD ancestry. Remote refs can be stale until `git fetch upstream --prune`; use --fetch-upstream when\n"
        "  archive creation should explicitly refresh upstream first (normal generation remains network-free).\n"
        "\nHistory cache/privacy\n"
        "  Generated history follows the existing anonymized Git-log email policy and also redacts email-shaped strings\n"
        "  embedded in historical patch text. Reuse checks the private immutable-SHA cache inside Git metadata first, then\n"
        f"  the local generated {COMMIT_DIFF_CONTEXT_DIR}/ mirror as a read-only secondary cache when present, so populated working copies do not rerender\n"
        "  already-generated history. Git renders only missing/new entries; amend/force-push/reset needs no arbitrary last-N refresh.\n"
        "  Use --commit-diff-count 0 to disable, or a positive N to include only the newest N reachable commits.\n"
    )


def build_generated_context(repo_root: Path, selected_files: Iterable[Path], commit_diff_count: int, write_cache: bool, explicit_upstream_refs: Iterable[str] = ()) -> tuple[dict[str, bytes | Path], str]:
    """Return snapshot-only metadata plus freshly generated canonical history keyed by ZIP-relative path."""
    selected_files = list(selected_files)
    context: dict[str, bytes | Path] = {
        GENERATED_CONTEXT_README_NAME: build_snapshot_context_readme().encode("utf-8"),
        GENERATED_GIT_MANIFEST_NAME: build_git_manifest(repo_root).encode("utf-8"),
        GENERATED_GIT_STATE_NAME: build_git_repository_state(repo_root, selected_files, explicit_upstream_refs).encode("utf-8"),
        GENERATED_GIT_IGNORED_TREE_NAME: build_git_ignored_paths_tree(repo_root).encode("utf-8"),
        GENERATED_STAGED_DIFF_NAME: build_git_diff(repo_root, cached=True),
        GENERATED_UNSTAGED_DIFF_NAME: build_git_diff(repo_root, cached=False),
        GENERATED_INCREMENTAL_GIT_LOG_NAME: build_incremental_git_log(repo_root).encode("utf-8"),
    }
    commit_context, commit_diff_summary = build_commit_diff_history_context(repo_root, commit_diff_count, write_cache)
    context.update(commit_context)
    pending_context, pending_summary = build_pending_upstream_context(repo_root, explicit_upstream_refs)
    context.update(pending_context)
    return context, f"{commit_diff_summary}; {pending_summary}"


def should_skip_dir(path: Path, repo_root: Path) -> bool:
    if path.name in SKIP_DIR_NAMES:
        return True
    rel = rel_for_message(path, repo_root).lower()
    if rel in SKIP_REL_DIRS:
        return True
    if rel == "llm_helpers/outputs" or rel.startswith("llm_helpers/outputs/"):
        return True
    return False


def should_skip_file(path: Path) -> bool:
    name = path.name.lower()
    if name in SKIP_FILE_NAMES:
        return True
    if path.suffix.lower() in SKIP_SUFFIXES:
        return True
    # Avoid recursively bundling earlier archives created by this script when
    # the default output folder is the mod root.
    if path.suffix.lower() == ".zip" and GENERATED_ARCHIVE_MARKER in name:
        return True
    return False


def iter_tree_files(root: Path, repo_root: Path) -> Iterator[Path]:
    if not root.exists():
        print(f"Warning: missing folder skipped: {rel_for_message(root, repo_root)}")
        return
    if not root.is_dir():
        print(f"Warning: not a folder, skipped: {rel_for_message(root, repo_root)}")
        return

    for child in sorted(root.iterdir(), key=lambda p: p.name.lower()):
        if child.is_dir():
            if should_skip_dir(child, repo_root):
                continue
            yield from iter_tree_files(child, repo_root)
        elif child.is_file() and not should_skip_file(child):
            yield child


def iter_root_files(repo_root: Path) -> Iterator[Path]:
    for child in sorted(repo_root.iterdir(), key=lambda p: p.name.lower()):
        if child.is_file() and not should_skip_file(child):
            yield child


def iter_dll_top_level_files(repo_root: Path) -> Iterator[Path]:
    dll_dir = repo_root / DLL_TOP_LEVEL_DIR
    if not dll_dir.is_dir():
        print(f"Warning: missing folder skipped: {DLL_TOP_LEVEL_DIR}")
        return
    for child in sorted(dll_dir.iterdir(), key=lambda p: p.name.lower()):
        if child.is_file() and not should_skip_file(child):
            yield child


def iter_dll_project_top_level_files(repo_root: Path) -> Iterator[Path]:
    project_dir = repo_root / DLL_PROJECT_DIR
    if not project_dir.is_dir():
        print(f"Warning: missing folder skipped: {DLL_PROJECT_DIR}")
        return
    for child in sorted(project_dir.iterdir(), key=lambda p: p.name.lower()):
        if not child.is_file() or should_skip_file(child):
            continue
        if child.suffix.lower() in DLL_PROJECT_SKIP_SUFFIXES:
            continue
        size = child.stat().st_size
        if size >= DLL_PROJECT_MAX_BYTES:
            print(
                "Warning: DLL project file over 1 MB skipped: "
                f"{rel_for_message(child, repo_root)} ({size:,} bytes)"
            )
            continue
        yield child


def iter_preserved_temp_files(repo_root: Path) -> Iterator[Path]:
    temp_dir = repo_root / PRESERVED_LIGHT_SOURCE_TEMP_DIR
    if not temp_dir.is_dir():
        print(f"Warning: missing folder skipped: {PRESERVED_LIGHT_SOURCE_TEMP_DIR}")
        return
    marker = temp_dir / PRESERVED_LIGHT_SOURCE_TEMP_MARKER
    if marker.is_file():
        yield marker
    else:
        print(f"Warning: missing temp-files marker skipped: {PRESERVED_LIGHT_SOURCE_TEMP_DIR}/{PRESERVED_LIGHT_SOURCE_TEMP_MARKER}")


def iter_exact_file_exceptions(repo_root: Path) -> Iterator[Path]:
    """Yield narrowly whitelisted files that intentionally bypass ordinary light-ZIP file skips."""
    for rel in LIGHT_SOURCE_EXACT_FILE_EXCEPTIONS:
        path = repo_root / rel
        if path.is_file():
            yield path
        else:
            print(f"Warning: exact light-source file exception missing: {rel}")


def collect_files(repo_root: Path) -> list[Path]:
    files: list[Path] = []
    seen: set[str] = set()

    def add(paths: Iterable[Path]) -> None:
        for path in paths:
            rel = path.relative_to(repo_root).as_posix()
            if rel not in seen:
                seen.add(rel)
                files.append(path)

    add(iter_root_files(repo_root))

    for rel_dir in ASSET_SUBDIRS:
        add(iter_tree_files(repo_root / rel_dir, repo_root))

    for rel_dir in ROOT_SUBDIRS:
        add(iter_tree_files(repo_root / rel_dir, repo_root))

    add(iter_dll_top_level_files(repo_root))
    add(iter_dll_project_top_level_files(repo_root))
    add(iter_preserved_temp_files(repo_root))
    add(iter_exact_file_exceptions(repo_root))

    for rel_dir in EXTRA_SUBDIRS:
        add(iter_tree_files(repo_root / rel_dir, repo_root))

    for rel_dir in IMAGE_SUBDIRS:
        add(iter_tree_files(repo_root / rel_dir, repo_root))

    return sorted(files, key=lambda p: p.relative_to(repo_root).as_posix().lower())


def write_zip(zip_path: Path, repo_root: Path, files: Iterable[Path], compression_level: int, generated_context: dict[str, bytes | Path]) -> int:
    count = 0
    temp_path = zip_path.with_suffix(zip_path.suffix + ".tmp")
    if temp_path.exists():
        temp_path.unlink()

    compression_method = ZIP_STORED if compression_level <= 0 else ZIP_DEFLATED
    with ZipFile(temp_path, "w", compression=compression_method, compresslevel=(None if compression_method == ZIP_STORED else compression_level), allowZip64=True) as archive:
        preserved_temp_dir = repo_root / PRESERVED_LIGHT_SOURCE_TEMP_DIR
        if preserved_temp_dir.is_dir():
            archive.writestr(PRESERVED_LIGHT_SOURCE_TEMP_DIR.rstrip("/") + "/", b"")
        for path in files:
            rel = path.relative_to(repo_root).as_posix()
            archive.write(path, rel)
            count += 1
        for rel, data in sorted(generated_context.items()):
            if isinstance(data, Path):
                archive.write(data, rel)
            else:
                archive.writestr(rel, data)
            count += 1

    temp_path.replace(zip_path)
    return count


def main() -> int:
    total_start_time = perf_counter()
    args = parse_args()
    repo_root = find_repo_root(args.repo_root)
    if args.fetch_upstream:
        fetch_upstream_or_fail(repo_root)
    mod_name = derive_mod_name(repo_root, args.mod_name)
    prefix = archive_prefix(mod_name, args.prefix)
    zip_path = output_path(repo_root, args.output_dir, prefix, not args.dry_run)
    files = collect_files(repo_root)
    context_start_time = perf_counter()
    generated_context, commit_diff_summary = build_generated_context(repo_root, files, args.commit_diff_count, write_cache=not args.dry_run, explicit_upstream_refs=args.upstream_ref)
    context_duration_ms = int((perf_counter() - context_start_time) * 1000)
    total_bytes = sum(path.stat().st_size for path in files) + sum(data.stat().st_size if isinstance(data, Path) else len(data) for data in generated_context.values())
    compression_mode = "ZIP_STORED / no compression" if args.compression_level <= 0 else f"ZIP_DEFLATED / compression level {args.compression_level}"

    print(f"Repo root: {repo_root}")
    print(f"Mod name:  {mod_name}")
    print(f"Prefix:    {prefix}")
    print(f"Archive:   {zip_path}")
    print(f"Files:     {len(files)} selected + {len(generated_context)} generated context files")
    print(f"Size:      {total_bytes:,} bytes before ZIP container overhead")
    print(f"Mode:      {compression_mode}")
    print(f"History:   {commit_diff_summary}")

    if args.dry_run:
        for path in files:
            print(path.relative_to(repo_root).as_posix())
        for rel in sorted(generated_context):
            print(f"(generated) {rel}")
        print("Dry run only; no archive written.")
        return 0

    zip_start_time = perf_counter()
    count = write_zip(zip_path, repo_root, files, args.compression_level, generated_context)
    zip_duration_ms = int((perf_counter() - zip_start_time) * 1000)

    # <!-- custom: A normal full-history archive and local/code-agent context should expose the same generated ancestry.
	# Reuse the already-built results after the ZIP succeeds; dry-run, disabled/truncated history and --no-sync-context never mutate the Git-ignored local context. (GPT-5.6-Sol) -->
    refresh_duration_ms = 0
    if args.commit_diff_count == DEFAULT_COMMIT_DIFF_COUNT and not args.no_sync_context:
        refresh_start_time = perf_counter()
        refresh_result = refresh_commit_diffs.refresh_commit_diff_context(repo_root, generated_context, COMMIT_DIFF_CONTEXT_DIR)
        refresh_duration_ms = int((perf_counter() - refresh_start_time) * 1000)
        print(f"Context:   refreshed {COMMIT_DIFF_CONTEXT_DIR} ({refresh_commit_diffs.format_refresh_summary(refresh_result)})")
    elif args.no_sync_context:
        print("Context:   local commit-diff refresh disabled by --no-sync-context")
    else:
        print("Context:   local commit-diff refresh skipped for disabled/truncated history")

    total_duration_ms = int((perf_counter() - total_start_time) * 1000)
    print(f"Wrote:     {count} file(s)")
    print(f"ZIP size:  {zip_path.stat().st_size:,} bytes")
    if not args.no_duration:
        print(f"Duration:  {total_duration_ms:,} ms total ({context_duration_ms:,} ms generated context; {zip_duration_ms:,} ms ZIP write; {refresh_duration_ms:,} ms local-context refresh)")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
