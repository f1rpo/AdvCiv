# AI, UI, logging, or other modifications first developed in AdvCiv-SAS (Simple Advanced Strategy)
# (c) 2026 wonderingabout & AI/LLM helpers (see Authors in AdvCiv-SAS's root README.md)

# <!-- custom: Locate the shared workflow library and exercise both accepted Markdown explanations and misleading metadata/example-only entries without involving a game build. (GPT-6.1-Sol) -->
from pathlib import Path
import sys
import unittest

sys.path.insert(0, str(Path(__file__).resolve().parents[4] / '.github/workflows/lib'))
# <!-- custom: Test the production parser and explanation guard shared by both recorder checks, rather than duplicating their Markdown rules in the fixture. (GPT-6.1-Sol) -->
from revision_history import change_errors, revision_entries


class RevisionExplanationTests(unittest.TestCase):
    def test_single_line_and_continued_explanations(self):
        for explanation in ('Fixed recorder timing.', '\n  Fixed recorder timing.', '\n\n  - Fixed recorder timing.'):
            with self.subTest(explanation=explanation):
                self.assertEqual(change_errors('### Revision 2\n- **Change:** ' + explanation), [])

    def test_metadata_empty_comments_and_empty_bullets_do_not_explain(self):
        for explanation in ('', '- **Change:** ', '- **Change:** <!-- later -->', '- **Change:**\n  - '):
            with self.subTest(explanation=explanation):
                text = '### Revision 2\n- **Date:** 2026-10-19\n' + explanation + '\n- **Git commit:** pending\n'
                self.assertEqual(change_errors(text), ['revision 2: missing nonempty Change explanation'])

    def test_neighboring_entry_and_template_do_not_supply_missing_text(self):
        text = '### Revision 2\n- **Change:**\n### Revision 1\n- **Change:** Earlier fix.\n## Maintenance\n```markdown\n### Revision 3\n- **Change:** Example only.\n```\n'
        self.assertEqual([number for number, _body in revision_entries(text)], [2, 1])
        self.assertEqual(change_errors(text, 2), ['revision 2: missing nonempty Change explanation'])
        self.assertEqual(change_errors(text, 3), ['revision 3: missing corresponding history entry'])
        self.assertEqual(change_errors(text, 1), [])

    def test_fenced_or_commented_entries_do_not_count(self):
        text = '<!--\n### Revision 4\n- **Change:** Hidden.\n-->\n~~~~markdown\n### Revision 3\n- **Change:** Fixture.\n~~~~\n### Revision 2\n- **Change:** Real explanation.\n'
        self.assertEqual([number for number, _body in revision_entries(text)], [2])
        self.assertEqual(change_errors(text), [])


if __name__ == '__main__':
    unittest.main()
