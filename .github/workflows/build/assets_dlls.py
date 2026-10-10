#!/usr/bin/env python3
# AI, UI, logging, or other modifications first developed in AdvCiv-SAS (Simple Advanced Strategy)
# (c) 2026 wonderingabout & AI/LLM helpers (see Authors in AdvCiv-SAS's root README.md)
#
# Build check: the Assets folder should contain only the shipped game-core DLL.
#
# <!-- custom: Search recursively under Assets so accidentally shipped backup or test DLLs in subfolders are caught too.
# Report the shipped DLL byte size for convenient release verification. (ChatGPT-5.5; ChatGPT-5.6-Sol) -->

from pathlib import Path
import argparse
import sys

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "lib"))

from xml_defines import get_default_repo_root


ASSETS_REL_PATH = Path("Assets")
MAIN_DLL_REL_PATH = ASSETS_REL_PATH / "CvGameCoreDLL.dll"
EXPECTED_DLL_REL_PATHS = (MAIN_DLL_REL_PATH,)


def byte_size(path: Path) -> int:
	return path.stat().st_size


def relative_path(path: Path, repo_root: Path) -> Path:
	return path.relative_to(repo_root)


def dll_entries(assets_dir: Path) -> list[Path]:
	return sorted(
		(path for path in assets_dir.rglob("*") if path.is_file() and path.suffix.lower() == ".dll"),
		key=lambda path: path.as_posix().lower(),
	)


def check_assets_dlls(repo_root: Path) -> tuple[list[str], dict[Path, int]]:
	assets_dir = repo_root / ASSETS_REL_PATH
	failures: list[str] = []
	sizes: dict[Path, int] = {}

	if not assets_dir.exists():
		return [f"missing Assets folder: {assets_dir}"], sizes
	if not assets_dir.is_dir():
		return [f"Assets path is not a folder: {assets_dir}"], sizes

	expected_rel_paths = set(EXPECTED_DLL_REL_PATHS)
	actual_rel_paths = [relative_path(entry, repo_root) for entry in dll_entries(assets_dir)]

	missing_rel_paths = [path for path in EXPECTED_DLL_REL_PATHS if path not in actual_rel_paths]
	unexpected_rel_paths = [path for path in actual_rel_paths if path not in expected_rel_paths]

	for rel_path in EXPECTED_DLL_REL_PATHS:
		path = repo_root / rel_path
		if path.exists() and not path.is_file():
			failures.append(f"expected DLL path exists but is not a file: {rel_path.as_posix()}")

	if missing_rel_paths:
		failures.append("missing expected DLL file(s): " + ", ".join(path.as_posix() for path in missing_rel_paths))

	if unexpected_rel_paths:
		failures.append("unexpected DLL file(s) under Assets: " + ", ".join(path.as_posix() for path in unexpected_rel_paths))

	main_dll = repo_root / MAIN_DLL_REL_PATH
	if main_dll.is_file():
		sizes[MAIN_DLL_REL_PATH] = byte_size(main_dll)

	return failures, sizes


def main() -> int:
	parser = argparse.ArgumentParser(description="Check the expected shipped Assets DLL recursively.")
	parser.add_argument("--repo-root", type=Path, default=get_default_repo_root(), help="repository root; defaults to the root containing .github/")
	args = parser.parse_args()

	failures, sizes = check_assets_dlls(args.repo_root)

	if failures:
		print("FAIL Assets DLL files")
		for failure in failures:
			print(f"  - {failure}")
		if sizes:
			print("  - observed byte sizes:")
			for rel_path in EXPECTED_DLL_REL_PATHS:
				if rel_path in sizes:
					print(f"    - {rel_path.as_posix()}: {sizes[rel_path]} bytes")
		return 1

	print("PASS Assets DLL files")
	for rel_path in EXPECTED_DLL_REL_PATHS:
		print(f"  - {rel_path.as_posix()}: {sizes[rel_path]} bytes")
	return 0


if __name__ == "__main__":
	sys.exit(main())
