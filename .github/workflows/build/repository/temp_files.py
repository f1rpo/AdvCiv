#!/usr/bin/env python3
# AI, UI, logging, or other modifications first developed in AdvCiv-SAS (Simple Advanced Strategy)
# (c) 2026 wonderingabout & AI/LLM helpers (see Authors in AdvCiv-SAS's root README.md)
#
# Build check: CvGameCoreDLL/Project/temp_files must retain only its marker and the locally ignored Debug-opt symbol target.
#
# <!-- custom: Git only tracks files, not empty folders, so keep a zero-byte placeholder file inside temp_files.
# We suspect fast-compile temp files can produce unreliable DLLs; fail on every generated target except Debug-opt, whose matching PDB is deliberately retained for crash analysis and must be deleted before rebuilding that configuration.
# The placeholder/folder may be export-ignored from GitHub Download ZIP / git archive release archives. See KI#38 and KI#38.2. (ChatGPT-5.5 + GPT-5.5 + GPT-5.6-Sol) -->

from pathlib import Path
import argparse
import sys

# <!-- custom: Resolve repository/shared-helper paths from this subject subfolder after the workflow-tool reorganization. (GPT-6.1-Sol) -->
sys.path.insert(0, str(Path(__file__).resolve().parents[2] / "lib"))

from xml_defines import get_default_repo_root


TEMP_FILES_REL_PATH = Path("CvGameCoreDLL/Project/temp_files")
TEMP_FILES_MARKER = ".gitkeep"
RETAINED_IGNORED_TARGET = "Debug-opt"
GIT_FILTER_REL_PATHS = (
	Path(".gitignore"),
	Path(".gitattributes"),
)
ALLOWED_GITATTRIBUTES_EXPORT_IGNORE_RULES = {
	"/cvgamecoredll/project/temp_files",
	"/cvgamecoredll/project/temp_files/**",
	"cvgamecoredll/project/temp_files",
	"cvgamecoredll/project/temp_files/**",
}
ALLOWED_GITIGNORE_RULES = {
	"/cvgamecoredll/project/temp_files/debug-opt/",
	"cvgamecoredll/project/temp_files/debug-opt/",
}


def relative_posix(path: Path, repo_root: Path) -> str:
	return path.relative_to(repo_root).as_posix()


def is_allowed_gitattributes_export_ignore_rule(normalized: str) -> bool:
	parts = normalized.split()
	if len(parts) != 2:
		return False
	path, attribute = parts
	return path in ALLOWED_GITATTRIBUTES_EXPORT_IGNORE_RULES and attribute == "export-ignore"


def active_temp_files_filter_rules(repo_root: Path) -> list[str]:
	failures: list[str] = []

	for rel_path in GIT_FILTER_REL_PATHS:
		path = repo_root / rel_path
		if not path.exists():
			failures.append(f"missing repository filter file: {rel_path.as_posix()}")
			continue
		if not path.is_file():
			failures.append(f"repository filter path is not a file: {rel_path.as_posix()}")
			continue

		for line_number, line in enumerate(path.read_text(encoding="utf-8", errors="replace").splitlines(), start=1):
			stripped = line.strip()
			if not stripped or stripped.startswith("#"):
				continue
			normalized = stripped.replace("\\", "/").lower()
			if "temp_files" not in normalized:
				continue
			if rel_path == Path(".gitignore") and normalized in ALLOWED_GITIGNORE_RULES:
				continue
			if rel_path == Path(".gitattributes") and is_allowed_gitattributes_export_ignore_rule(normalized):
				continue
			failures.append(f"{rel_path.as_posix()}: line {line_number}: remove active temp_files rule: {stripped}")

	return failures


def check_temp_files_folder(repo_root: Path) -> list[str]:
	failures: list[str] = []
	temp_dir = repo_root / TEMP_FILES_REL_PATH
	marker = temp_dir / TEMP_FILES_MARKER

	if not temp_dir.exists():
		return [
			f"{TEMP_FILES_REL_PATH.as_posix()}: missing folder; commit {TEMP_FILES_REL_PATH.as_posix()}/{TEMP_FILES_MARKER} "
			"so Git preserves this sanity-check folder"
		]
	if not temp_dir.is_dir():
		return [f"{TEMP_FILES_REL_PATH.as_posix()}: exists but is not a folder"]

	if not marker.is_file():
		failures.append(f"{TEMP_FILES_REL_PATH.as_posix()}/{TEMP_FILES_MARKER}: missing zero-byte tracking marker")
	elif marker.stat().st_size != 0:
		failures.append(f"{TEMP_FILES_REL_PATH.as_posix()}/{TEMP_FILES_MARKER}: marker should be zero bytes")

	unexpected_entries = []
	retained_target = temp_dir / RETAINED_IGNORED_TARGET
	for entry in sorted(temp_dir.rglob("*"), key=lambda path: path.as_posix().lower()):
		if entry == marker:
			continue
		if entry == retained_target or retained_target in entry.parents:
			continue
		unexpected_entries.append(relative_posix(entry, repo_root))

	if unexpected_entries:
		failures.append(
			"unexpected compiler temp_files content; clean these before committing/replacing the DLL: "
			+ ", ".join(unexpected_entries)
		)

	return failures


def check_temp_files(repo_root: Path) -> list[str]:
	failures: list[str] = []
	failures.extend(check_temp_files_folder(repo_root))
	failures.extend(active_temp_files_filter_rules(repo_root))
	return failures


def main() -> int:
	parser = argparse.ArgumentParser(description="Check that CvGameCoreDLL/Project/temp_files contains only its marker and retained Debug-opt target.")
	parser.add_argument("--repo-root", type=Path, default=get_default_repo_root(), help="repository root; defaults to the root containing .github/")
	args = parser.parse_args()

	failures = check_temp_files(args.repo_root)

	if failures:
		print("FAIL compiler temp_files hygiene")
		for failure in failures:
			print(f"  - {failure}")
		return 1

	print("PASS compiler temp_files hygiene")
	print(f"  - {TEMP_FILES_REL_PATH.as_posix()} contains only {TEMP_FILES_MARKER} plus optional ignored {RETAINED_IGNORED_TARGET} symbols")
	return 0


if __name__ == "__main__":
	sys.exit(main())
