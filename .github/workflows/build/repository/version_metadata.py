#!/usr/bin/env python3
# AI, UI, logging, or other modifications first developed in AdvCiv-SAS (Simple Advanced Strategy)
# (c) 2026 wonderingabout & AI/LLM helpers (see Authors in AdvCiv-SAS's root README.md)
#
# <!-- custom: Build check: runtime archive source metadata must retain its Git export-subst version-describe/commit/date placeholders and rule. (ChatGPT-5.6-Sol) -->

from pathlib import Path
import argparse
import sys

# <!-- custom: Resolve repository/shared-helper paths from this subject subfolder after the workflow-tool reorganization. (GPT-6.1-Sol) -->
sys.path.insert(0, str(Path(__file__).resolve().parents[2] / "lib"))
from xml_defines import get_default_repo_root


METADATA_PATH = Path("Assets/SASModVersion.txt")
GITATTRIBUTES_PATH = Path(".gitattributes")
REQUIRED_LINES = {
	"versionDescribe=$Format:%(describe:tags,match=SAS_VERSION_ANCHOR_*,abbrev=10)$",
	"commit=$Format:%H$",
	"commitDate=$Format:%cI$",
}
REQUIRED_ATTRIBUTE = "/Assets/SASModVersion.txt export-subst"


def check(repo_root: Path) -> list[str]:
	failures: list[str] = []
	metadata = repo_root / METADATA_PATH
	if not metadata.is_file():
		return [f"missing runtime archive metadata file: {METADATA_PATH.as_posix()}"]
	lines = {line.strip() for line in metadata.read_text(encoding="utf-8", errors="replace").splitlines() if line.strip() and not line.lstrip().startswith("#")}
	for required in sorted(REQUIRED_LINES):
		if required not in lines:
			failures.append(f"{METADATA_PATH.as_posix()}: missing exact metadata token line {required!r}")
	attrs = (repo_root / GITATTRIBUTES_PATH).read_text(encoding="utf-8", errors="replace").splitlines() if (repo_root / GITATTRIBUTES_PATH).is_file() else []
	if REQUIRED_ATTRIBUTE not in {line.strip() for line in attrs}:
		failures.append(f"{GITATTRIBUTES_PATH.as_posix()}: missing {REQUIRED_ATTRIBUTE!r}")
	return failures


def main() -> int:
	parser = argparse.ArgumentParser(description="Check AdvCiv-SAS runtime archive version metadata wiring.")
	parser.add_argument("--repo-root", type=Path, default=get_default_repo_root())
	args = parser.parse_args()
	failures = check(args.repo_root)
	if failures:
		print("FAIL runtime source/version metadata")
		for failure in failures:
			print("  - " + failure)
		return 1
	print("PASS runtime source/version metadata")
	return 0


if __name__ == "__main__":
	raise SystemExit(main())
