# AI, UI, logging, or other modifications first developed in AdvCiv-SAS (Simple Advanced Strategy)
# (c) 2026 wonderingabout & AI/LLM helpers (see Authors in AdvCiv-SAS's root README.md)

import json
from pathlib import Path
import sys
import tempfile
import unittest
from unittest.mock import patch

# <!-- custom: Locate the repository helper before importing it; direct sys.path setup needs no lint suppression. (GPT-6.1-Sol) -->
sys.path.insert(0, str(Path(__file__).resolve().parents[4] / "LLM_Helpers"))
import light_source_python24_linux as runtime


def manifest(sha):
    tag = f"{runtime.RUNTIME_CHANNEL}-{sha}"
    return json.dumps({
        "schemaVersion": 1, "pythonVersion": runtime.PYTHON_VERSION,
        "platform": runtime.RUNTIME_PLATFORM, "archiveName": runtime.RUNTIME_ARCHIVE_NAME,
        "sha256": sha, "sizeBytes": 123, "releaseTag": tag,
        "archiveUrl": f"{runtime.RUNTIME_RELEASE_BASE}/{tag}/{runtime.RUNTIME_ARCHIVE_NAME}",
    })


# <!-- custom: Exercise metadata handoff pitfalls with fake manifests and temporary folders, without network requests, runtime downloads or another workflow. (GPT-6.1-Sol) -->
class RuntimeMetadataTests(unittest.TestCase):
    def test_updates_and_offline_fallback_keep_one_snapshot_note(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            for sha, expected in [("a" * 64, "first package check"), ("a" * 64, "unchanged since"), ("b" * 64, "NEW package")]:
                with self.subTest(status=expected), patch.object(runtime, "download_text", return_value=manifest(sha)) as fetch:
                    context, summary = runtime.build_archive_context(root)
                    fetch.assert_called_once_with(runtime.RUNTIME_MANIFEST_URL)
                    self.assertIn(expected, summary)
                    self.assertEqual(list(context), ["_SNAPSHOT_CONTEXT/python24_runtime.txt"])
                    self.assertIn(sha, context[runtime.ARCHIVE_README_NAME].decode())
            with patch.object(runtime, "download_text", side_effect=OSError("offline")):
                context, summary = runtime.build_archive_context(root)
            self.assertIn("latest package unknown", summary)
            self.assertIn("showing last known metadata", summary)
            self.assertIn("b" * 64, context[runtime.ARCHIVE_README_NAME].decode())
            files = [p.relative_to(root).as_posix() for p in root.rglob("*") if p.is_file()]
            self.assertEqual(files, ["LLM_Helpers/outputs/python24_runtime_manifest.json"])

    def test_disabled_dry_run_and_first_offline_check_write_nothing(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            with patch.object(runtime, "download_text", return_value=manifest("a" * 64)) as fetch:
                context, _ = runtime.build_archive_context(root, disabled=True)
                self.assertEqual(context, {})
                fetch.assert_not_called()
                context, _ = runtime.build_archive_context(root, dry_run=True)
                self.assertIn(runtime.ARCHIVE_README_NAME, context)
            with patch.object(runtime, "download_text", side_effect=OSError("offline")):
                _, summary = runtime.build_archive_context(root)
            self.assertIn("latest package unknown", summary)
            self.assertIn("no previous metadata available", summary)
            self.assertEqual(list(root.iterdir()), [])


if __name__ == "__main__":
    unittest.main()
