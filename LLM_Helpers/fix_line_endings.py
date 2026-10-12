#!/usr/bin/env python3
# AI, UI, logging, or other modifications first developed in AdvCiv-SAS (Simple Advanced Strategy)
# (c) 2026 wonderingabout & AI/LLM helpers (see Authors in AdvCiv-SAS's root README.md)
#
# External helper: report or fix mixed CRLF/LF line endings and missing final newlines.
#
# This is intentionally not run by GitHub Actions. The CI check should only report/fail; this helper is
# for local, reviewed cleanup passes.

from pathlib import Path
import argparse
import difflib
import subprocess
import sys


SCAN_RELATIVE_PATHS = (
	Path(".github"),
	Path(".gitattributes"),
	Path(".gitignore"),
	Path("AGENTS.md"),
	Path("README.md"),
	Path("ruff.toml"),
	Path("Assets/XML"),
	Path("Assets/Python"),
	Path("CvGameCoreDLL"),
	Path("LLM_Helpers"),
	Path("PrivateMaps"),
	Path("_1_AdvCiv-SAS/Docs"),
)

IGNORED_RELATIVE_DIRS = {
	Path(".git"),
	# <!-- custom: Preserve reference-example exclusions at their centralized documentation locations. (GPT-6.1-Sol) -->
	Path("_1_AdvCiv-SAS/Docs/examples/handicap"),
	Path("_1_AdvCiv-SAS/Docs/examples/source_comparison"),
	Path("_1_AdvCiv-SAS/Docs/examples/sasgamerecord"),
	Path("LLM_Helpers/outputs"),
	Path("LLM_Helpers/context/mapscript_refs"),
	Path("_0_Common_Docs"),
	Path("Assets/Python/Contrib/Sevopedia/Debug"),
	Path("CvGameCoreDLL/Project/temp_files"),
}

TEXT_FILE_SUFFIXES = {".bat", ".c", ".cfg", ".cmd", ".cpp", ".csv", ".def", ".diff", ".h", ".hpp", ".ini", ".inl", ".json", ".mak", ".md", ".mk", ".patch", ".ps1", ".py", ".pyw", ".rc", ".sh", ".sln", ".toml", ".txt", ".vcproj", ".xml", ".yaml", ".yml"}
TEXT_FILE_NAMES = {".gitattributes", ".gitignore", "Makefile"}


def get_default_repo_root() -> Path:
	return Path(__file__).resolve().parents[1]


def is_under(relative_path: Path, relative_dir: Path) -> bool:
	return relative_path == relative_dir or relative_dir in relative_path.parents


def is_ignored(relative_path: Path) -> bool:
	return any(is_under(relative_path, ignored_dir) for ignored_dir in IGNORED_RELATIVE_DIRS)


def is_text_like_path(path: Path) -> bool:
	# <!-- custom: Keep extensionless files explicit so the fixer does not rewrite future binary-like files that only pass the null-byte guard. (GPT-5.5) -->
	return path.name in TEXT_FILE_NAMES or path.suffix.lower() in TEXT_FILE_SUFFIXES


def is_probably_binary(data: bytes) -> bool:
	return b"\0" in data


def git_tracked_files(repo_root: Path) -> list[Path] | None:
	try:
		result = subprocess.run(
			["git", "ls-files", "-z"],
			cwd=repo_root,
			check=True,
			stdout=subprocess.PIPE,
			stderr=subprocess.PIPE,
		)
	except (OSError, subprocess.CalledProcessError):
		return None

	paths: list[Path] = []
	for raw_path in result.stdout.split(b"\0"):
		if not raw_path:
			continue
		paths.append(repo_root / raw_path.decode("utf-8", errors="replace"))
	return paths


def iter_candidate_files(repo_root: Path, input_paths: list[Path]) -> list[Path]:
	if input_paths:
		paths: list[Path] = []
		for input_path in input_paths:
			path = input_path if input_path.is_absolute() else repo_root / input_path
			if path.is_file():
				paths.append(path)
			elif path.is_dir():
				paths.extend(child for child in path.rglob("*") if child.is_file())
			else:
				print(f"WARN missing path: {input_path}", file=sys.stderr)
		return sorted(set(path for path in paths if is_text_like_path(path) and not is_ignored(path.relative_to(repo_root))))

	tracked_paths = git_tracked_files(repo_root)
	if tracked_paths is not None:
		return sorted(
			path
			for path in tracked_paths
			if path.is_file()
			and is_text_like_path(path)
			and not is_ignored(path.relative_to(repo_root))
			and any(is_under(path.relative_to(repo_root), scan_path) or path.relative_to(repo_root) == scan_path for scan_path in SCAN_RELATIVE_PATHS)
		)

	paths: list[Path] = []
	for scan_path in SCAN_RELATIVE_PATHS:
		path = repo_root / scan_path
		if not path.exists():
			continue
		if path.is_file():
			relative_path = path.relative_to(repo_root)
			if is_text_like_path(path) and not is_ignored(relative_path):
				paths.append(path)
			continue
		for child in path.rglob("*"):
			if not child.is_file():
				continue
			relative_path = child.relative_to(repo_root)
			if is_ignored(relative_path):
				continue
			if is_text_like_path(child):
				paths.append(child)
	return sorted(set(paths))


def line_ending_counts(data: bytes) -> tuple[int, int, int]:
	crlf_count = data.count(b"\r\n")
	without_crlf = data.replace(b"\r\n", b"")
	lf_count = without_crlf.count(b"\n")
	lone_cr_count = without_crlf.count(b"\r")
	return crlf_count, lf_count, lone_cr_count


def dominant_eol(data: bytes) -> bytes:
	crlf_count, lf_count, lone_cr_count = line_ending_counts(data)
	if crlf_count > 0 and crlf_count >= lf_count:
		return b"\r\n"
	if lf_count > 0:
		return b"\n"
	if lone_cr_count > 0:
		return b"\n"
	return b"\n"


def normalize_data(data: bytes, eol_mode: str, ensure_final_newline: bool) -> tuple[bytes, str]:
	if not data or is_probably_binary(data):
		return data, "skipped"

	target_eol = dominant_eol(data)
	if eol_mode == "lf":
		target_eol = b"\n"
	elif eol_mode == "crlf":
		target_eol = b"\r\n"

	placeholder = b"\x00SAS_CRLF_PLACEHOLDER\x00"
	normalized = data.replace(b"\r\n", placeholder)
	normalized = normalized.replace(b"\r", b"\n")
	normalized = normalized.replace(placeholder, b"\n")
	normalized = normalized.replace(b"\n", target_eol)

	if ensure_final_newline and normalized and not normalized.endswith(target_eol):
		normalized += target_eol

	return normalized, "changed" if normalized != data else "ok"


def diff_bytes(relative_path: Path, before: bytes, after: bytes) -> str:
	before_text = before.decode("utf-8", errors="replace").splitlines(keepends=True)
	after_text = after.decode("utf-8", errors="replace").splitlines(keepends=True)
	return "".join(
		difflib.unified_diff(
			before_text,
			after_text,
			fromfile=f"{relative_path.as_posix()} (before)",
			tofile=f"{relative_path.as_posix()} (after)",
		)
	)


def main() -> int:
	parser = argparse.ArgumentParser(description="Report or fix mixed line endings and missing final newlines.")
	parser.add_argument("paths", nargs="*", type=Path, help="optional files/folders to scan; defaults to active repo text files")
	parser.add_argument("--repo-root", type=Path, default=get_default_repo_root(), help="repository root; defaults to this helper's parent repo")
	parser.add_argument("--in-place", action="store_true", help="write changes; otherwise only report")
	parser.add_argument("--diff", action="store_true", help="print unified diffs for files that would change")
	parser.add_argument("--eol", choices=("preserve", "lf", "crlf"), default="preserve", help="target line ending; preserve means use each file's dominant existing style")
	parser.add_argument("--no-final-newline", action="store_true", help="do not add a final newline")
	args = parser.parse_args()

	repo_root = args.repo_root.resolve()
	changed: list[Path] = []
	scanned = 0

	for path in iter_candidate_files(repo_root, args.paths):
		scanned += 1
		data = path.read_bytes()
		new_data, status = normalize_data(data, args.eol, not args.no_final_newline)
		if status != "changed":
			continue

		relative_path = path.relative_to(repo_root)
		changed.append(relative_path)

		if args.diff:
			print(diff_bytes(relative_path, data, new_data))

		if args.in_place:
			path.write_bytes(new_data)

	print(f"Scanned {scanned} text-like file(s).")
	if not changed:
		print("No line-ending/final-newline changes needed.")
		return 0

	action = "Updated" if args.in_place else "Would update"
	print(f"{action} {len(changed)} file(s):")
	for relative_path in changed:
		print(f"  - {relative_path.as_posix()}")

	if not args.in_place:
		print("Run again with --in-place after reviewing the target list.")
	return 1 if not args.in_place else 0


if __name__ == "__main__":
	sys.exit(main())
