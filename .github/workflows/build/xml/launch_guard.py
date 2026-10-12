#!/usr/bin/env python3
# AI, UI, logging, or other modifications first developed in AdvCiv-SAS (Simple Advanced Strategy)
# (c) 2026 wonderingabout & AI/LLM helpers (see Authors in AdvCiv-SAS's root README.md)
#
# Build check: SASDefineGuard.py and its GlobalDefines sentinels must exist and stay in sync.

from pathlib import Path
import argparse
import ast
import sys

# <!-- custom: Resolve repository/shared-helper paths from this subject subfolder after the workflow-tool reorganization. (GPT-6.1-Sol) -->
sys.path.insert(0, str(Path(__file__).resolve().parents[2] / "lib"))

from xml_defines import get_default_repo_root, read_global_define_ints, read_global_define_texts


DEFINE_GUARD_RELATIVE_PATH = Path("Assets/Python/SASDefineGuard.py")
EXPECTED_CONSTANT_NAMES = (
	"SAS_LAUNCH_GUARD_TEST_INT_DEFINE",
	"SAS_LAUNCH_GUARD_TEST_STRING_DEFINE",
	"SAS_LAUNCH_GUARD_EXPECTED_INT",
	"SAS_LAUNCH_GUARD_EXPECTED_STRING",
)


def read_define_guard_constants(repo_root: Path) -> tuple[dict[str, int | str], list[str]]:
	path = repo_root / DEFINE_GUARD_RELATIVE_PATH
	failures: list[str] = []

	if not path.exists():
		return {}, [f"{DEFINE_GUARD_RELATIVE_PATH}: missing launch guard file"]
	if not path.is_file():
		return {}, [f"{DEFINE_GUARD_RELATIVE_PATH}: exists but is not a file"]
	if path.stat().st_size == 0:
		return {}, [f"{DEFINE_GUARD_RELATIVE_PATH}: empty launch guard file"]

	tree = ast.parse(path.read_text(encoding="utf-8"), filename=str(path))
	constants: dict[str, int | str] = {}
	has_verify_or_raise = False

	for node in tree.body:
		if isinstance(node, ast.FunctionDef) and node.name == "verify_or_raise":
			has_verify_or_raise = True
			continue

		if not isinstance(node, ast.Assign) or len(node.targets) != 1 or not isinstance(node.targets[0], ast.Name):
			continue

		name = node.targets[0].id
		if name not in EXPECTED_CONSTANT_NAMES:
			continue

		if isinstance(node.value, ast.Constant) and isinstance(node.value.value, (int, str)):
			constants[name] = node.value.value
		else:
			failures.append(f"{DEFINE_GUARD_RELATIVE_PATH}: {name} must be a literal int/string constant")

	if not has_verify_or_raise:
		failures.append(f"{DEFINE_GUARD_RELATIVE_PATH}: missing verify_or_raise function")

	for name in EXPECTED_CONSTANT_NAMES:
		if name not in constants:
			failures.append(f"{DEFINE_GUARD_RELATIVE_PATH}: missing {name}")

	return constants, failures


def check_launch_guard(repo_root: Path) -> list[str]:
	failures: list[str] = []
	constants, guard_failures = read_define_guard_constants(repo_root)
	failures.extend(guard_failures)

	if guard_failures:
		return failures

	int_define_name = constants["SAS_LAUNCH_GUARD_TEST_INT_DEFINE"]
	string_define_name = constants["SAS_LAUNCH_GUARD_TEST_STRING_DEFINE"]
	expected_int = constants["SAS_LAUNCH_GUARD_EXPECTED_INT"]
	expected_string = constants["SAS_LAUNCH_GUARD_EXPECTED_STRING"]

	if not isinstance(int_define_name, str) or not isinstance(string_define_name, str) or not isinstance(expected_int, int) or not isinstance(expected_string, str):
		return failures + [f"{DEFINE_GUARD_RELATIVE_PATH}: launch guard constants have unexpected literal types"]

	int_defines = read_global_define_ints(repo_root)
	text_defines = read_global_define_texts(repo_root)

	if int_define_name not in int_defines:
		failures.append(f"GlobalDefines_advciv_sas.xml: missing int sentinel define {int_define_name}")
	elif int_defines[int_define_name] != expected_int:
		failures.append(f"GlobalDefines_advciv_sas.xml: {int_define_name} expected {expected_int} from SASDefineGuard.py, found {int_defines[int_define_name]}")

	if string_define_name not in text_defines:
		failures.append(f"GlobalDefines_advciv_sas.xml: missing string sentinel define {string_define_name}")
	elif text_defines[string_define_name] != expected_string:
		failures.append(f"GlobalDefines_advciv_sas.xml: {string_define_name} expected {expected_string!r} from SASDefineGuard.py, found {text_defines[string_define_name]!r}")

	return failures


def main() -> int:
	parser = argparse.ArgumentParser(description="Check that AdvCiv-SAS launch guard sentinels exist and match GlobalDefines.")
	parser.add_argument("--repo-root", type=Path, default=get_default_repo_root(), help="repository root; defaults to the root containing .github/")
	args = parser.parse_args()

	failures = check_launch_guard(args.repo_root)

	if failures:
		print("FAIL launch guard sentinels")
		for failure in failures:
			print(f"  - {failure}")
		return 1

	print("PASS launch guard sentinels")
	return 0


if __name__ == "__main__":
	sys.exit(main())
