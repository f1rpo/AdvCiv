# AI, UI, logging, or other modifications first developed in AdvCiv-SAS (Simple Advanced Strategy)
# (c) 2026 wonderingabout & AI/LLM helpers (see Authors in AdvCiv-SAS's root README.md)

# <!-- custom: pathlib/sys locate the production checker and unittest exercises its paragraph boundaries and chronology decisions, including the KI#979 replay placement that motivated the rule. (GPT-6.1-Sol) -->
from pathlib import Path
import sys
import unittest

sys.path.insert(0, str(Path(__file__).resolve().parents[2] / "build/documentation"))
# <!-- custom: Import known_issues_chronology itself so the fixtures exercise the production parser and decisions rather than a copied implementation. (GPT-6.1-Sol) -->
import known_issues_chronology as checker


class KnownIssuesChronologyTests(unittest.TestCase):
    baseline = ('<a id="ki-979"></a>\n\n## KI#979 - Finding\n\n'
                'Original defect.\n\nArchaeology and rationale.\n\nOriginal credits.\n\n'
                '<a id="ki-980"></a>\n\n## KI#980 - Other finding\n\nOther history.\n')

    def test_replay_inserted_before_archaeology_or_credits_fails(self):
        note = '2026-10-21 post-fix replay: the crash still reproduced.\n\n'
        for retained in ('Original defect.', 'Archaeology and rationale.', 'Original credits.'):
            with self.subTest(retained=retained):
                new = self.baseline.replace(retained, note + retained)
                errors = checker.chronology_errors(self.baseline, new)
                self.assertEqual(len(errors), 1)
                self.assertIn('KI#979', errors[0])

    def test_appends_new_entries_and_corrections_pass(self):
        variants = (
            self.baseline,
            self.baseline.replace('<a id="ki-980">', '2026-10-21 replay result.\n\n<a id="ki-980">'),
            self.baseline + '\n2026-10-21 final-entry update.\n',
            self.baseline.replace('Original defect.', 'Corrected defect description.'),
            self.baseline.replace('Archaeology and rationale.', 'Archaeology and\nrationale.'),
            self.baseline.replace('## KI#979 - Finding', '## KI#979 - Fixed finding'),
            self.baseline.replace('<a id="ki-980">', '<a id="ki-979.2"></a>\n\n## KI#979.2 - New follow-up\n\nNew finding.\n\n<a id="ki-980">'),
            self.baseline.replace('Original credits.\n\n', ''),
        )
        for new in variants:
            with self.subTest(new=new):
                self.assertEqual(checker.chronology_errors(self.baseline, new), [])

    def test_relocation_to_end_passes(self):
        note = '2026-10-21 replay result.\n\n'
        old = self.baseline.replace('Archaeology and rationale.', note + 'Archaeology and rationale.')
        new = self.baseline.replace('<a id="ki-980">', note + '<a id="ki-980">')
        self.assertEqual(checker.chronology_errors(old, new), [])


if __name__ == '__main__':
    unittest.main()
