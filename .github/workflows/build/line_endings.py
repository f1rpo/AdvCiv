#!/usr/bin/env python3
# AI, UI, logging, or other modifications first developed in AdvCiv-SAS (Simple Advanced Strategy)
# (c) 2026 wonderingabout & AI/LLM helpers (see Authors in AdvCiv-SAS's root README.md)
#
# Build check: active text files should not have mixed CRLF/LF line endings and should end with a newline.
# <!-- custom: Before the first cleanup, this checker found 79 local worktree issues, while GitHub Actions reported only 19.
# Comparing local bytes with `git show :path` showed that some local mixed-EOL files were clean LF-only in Git's indexed blob (e.g., BugInit.py) while true CI failures stayed mixed in both (e.g., Pangaea.py).
# Fixing all local findings updated 78 worktree files, but staging kept only the 19 stored-content changes that Git/CI saw.
# This makes local output useful cleanup guidance, while staged/GitHub output is the authoritative committed-content failure set. (GPT-5.5) -->

from pathlib import Path
import argparse
import subprocess
import sys

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "lib"))

from xml_defines import get_default_repo_root


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
	Path("LLM_Helpers/examples"),
	Path("LLM_Helpers/outputs"),
	Path("LLM_Helpers/context/mapscript_refs"),
	Path("_0_Common_Docs"),
	Path("Assets/Python/Contrib/Sevopedia/Debug"),
	Path("CvGameCoreDLL/Project/temp_files"),
}

TEXT_FILE_SUFFIXES = {".bat", ".c", ".cfg", ".cmd", ".cpp", ".csv", ".def", ".diff", ".h", ".hpp", ".ini", ".inl", ".json", ".mak", ".md", ".mk", ".patch", ".ps1", ".py", ".pyw", ".rc", ".sh", ".sln", ".toml", ".txt", ".vcproj", ".xml", ".yaml", ".yml"}
TEXT_FILE_NAMES = {".gitattributes", ".gitignore", "Makefile"}


class EolIssue:
	def __init__(self, relative_path: Path, message: str):
		self.relative_path = relative_path
		self.message = message

	def format(self) -> str:
		return f"{self.relative_path.as_posix()}: {self.message}"


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


def iter_candidate_files(repo_root: Path) -> tuple[list[Path], str]:
	tracked_paths = git_tracked_files(repo_root)
	if tracked_paths is not None:
		return (
			sorted(
				path
				for path in tracked_paths
				if path.is_file()
				and is_text_like_path(path)
				and not is_ignored(path.relative_to(repo_root))
				and any(is_under(path.relative_to(repo_root), scan_path) or path.relative_to(repo_root) == scan_path for scan_path in SCAN_RELATIVE_PATHS)
			),
			"git-tracked working-tree files",
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
	return sorted(set(paths)), "plain directory fallback scan (no Git index available)"


def line_ending_counts(data: bytes) -> tuple[int, int, int]:
	crlf_count = data.count(b"\r\n")
	without_crlf = data.replace(b"\r\n", b"")
	lf_count = without_crlf.count(b"\n")
	lone_cr_count = without_crlf.count(b"\r")
	return crlf_count, lf_count, lone_cr_count


def check_data(relative_path: Path, data: bytes) -> list[EolIssue]:
	failures: list[EolIssue] = []

	if not data or is_probably_binary(data):
		return failures

	crlf_count, lf_count, lone_cr_count = line_ending_counts(data)
	used_styles = sum(1 for count in (crlf_count, lf_count, lone_cr_count) if count > 0)

	if lone_cr_count:
		failures.append(EolIssue(relative_path, f"contains {lone_cr_count} lone CR line ending(s)"))
	if used_styles > 1:
		failures.append(EolIssue(relative_path, f"has mixed line endings: CRLF={crlf_count}, LF={lf_count}, lone_CR={lone_cr_count}"))
	if not data.endswith(b"\n"):
		failures.append(EolIssue(relative_path, "missing final newline"))

	return failures


def check_file(repo_root: Path, path: Path) -> list[EolIssue]:
	return check_data(path.relative_to(repo_root), path.read_bytes())


def read_git_index_bytes(repo_root: Path, relative_path: Path) -> bytes | None:
	try:
		result = subprocess.run(
			["git", "show", ":%s" % relative_path.as_posix()],
			cwd=repo_root,
			check=True,
			stdout=subprocess.PIPE,
			stderr=subprocess.PIPE,
		)
	except (OSError, subprocess.CalledProcessError):
		return None
	return result.stdout


def get_index_clean_worktree_failures(repo_root: Path, failures: list[EolIssue]) -> list[Path]:
	clean_in_index: list[Path] = []
	for relative_path in sorted({failure.relative_path for failure in failures}):
		data = read_git_index_bytes(repo_root, relative_path)
		if data is not None and not check_data(relative_path, data):
			clean_in_index.append(relative_path)
	return clean_in_index


def check_line_endings(repo_root: Path) -> tuple[list[EolIssue], str]:
	failures: list[EolIssue] = []
	candidate_files, scan_mode = iter_candidate_files(repo_root)

	for path in candidate_files:
		failures.extend(check_file(repo_root, path))

	return failures, scan_mode


def main() -> int:
	parser = argparse.ArgumentParser(description="Check active text files for mixed line endings and missing final newlines.")
	parser.add_argument("--repo-root", type=Path, default=get_default_repo_root(), help="repository root; defaults to the root containing .github/")
	parser.add_argument("--max-failures", type=int, default=80, help="maximum number of failures to print before summarizing")
	args = parser.parse_args()

	failures, scan_mode = check_line_endings(args.repo_root)

	if failures:
		print("FAIL text line-ending hygiene")
		print(f"  scan mode: {scan_mode}")
		for failure in failures[: args.max_failures]:
			print(f"  - {failure.format()}")
		remaining = len(failures) - args.max_failures
		if remaining > 0:
			print(f"  - ... {remaining} more failure(s) not shown")
		if scan_mode.startswith("git-tracked"):
			clean_in_index = get_index_clean_worktree_failures(args.repo_root, failures)
			if clean_in_index:
				print("  note: %d failing worktree file(s) are clean in the Git index; GitHub Actions may pass if its checkout uses those normalized/indexed bytes." % len(clean_in_index))
				for relative_path in clean_in_index[:10]:
					print(f"    index-clean: {relative_path.as_posix()}")
				if len(clean_in_index) > 10:
					print("    ... %d more index-clean worktree failure(s) not shown" % (len(clean_in_index) - 10))
		return 1

	print("PASS text line-ending hygiene")
	print(f"  scan mode: {scan_mode}")
	return 0


if __name__ == "__main__":
	sys.exit(main())
