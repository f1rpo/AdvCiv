# AI, UI, logging, or other modifications first developed in AdvCiv-SAS (Simple Advanced Strategy)
# (c) 2026 wonderingabout & AI/LLM helpers (see Authors in AdvCiv-SAS's root README.md)
from pathlib import Path
import sys
import unittest

ROOT = Path(__file__).resolve().parents[3]
# <!-- custom: Restore blocking checks in the automated subject folders; regression fixtures import those same helpers. (GPT-6.1-Sol) -->
sys.path.insert(0, str(ROOT / ".github/workflows/build/documentation"))
import markdown_structure as markdown

class MarkdownTests(unittest.TestCase):
    def test_menu_missing_title_and_hierarchy(self):
        valid = '# Doc\n\n## Menu\n\n[A](#a)\\\n&emsp;[B](#b)\n\n## A\n\n### B\n'
        self.assertEqual(markdown.menu_errors(valid), [])
        for broken in (valid.replace('&emsp;[B](#b)\n', ''), valid.replace('[A]', '[Wrong]'), valid.replace('&emsp;', ''), valid.replace('### B', '### C')):
            self.assertTrue(markdown.menu_errors(broken))

    def test_ki_stable_anchor_and_internal_subsections(self):
        valid = '# Issues\n## Menu\n[KI#457 - title](#ki-457)\n\n<a id="ki-457"></a>\n\n## KI#457 - title\n### Internal investigation\n'
        self.assertEqual(markdown.menu_errors(valid, True), [])
        self.assertIn('<a id="ki-457"></a>', markdown.render_menu(valid, True))
        self.assertTrue(markdown.menu_errors(valid.replace('[KI#457 - title](#ki-457)\n', ''), True))

    def test_refresh_preserves_external_links_prose_and_body(self):
        text = '# D\n## Menu\nHere you go:\n[A](#a)\n[Guide](guide.md)\n\n## A\nBody.\n### B\n'
        refreshed = markdown.render_menu(text)
        self.assertIn('Here you go:', refreshed)
        self.assertIn('[Guide](guide.md)', refreshed)
        self.assertTrue(refreshed.endswith('## A\nBody.\n### B\n'))
        self.assertEqual(markdown.menu_errors(refreshed), [])
        self.assertEqual(markdown.render_menu(refreshed), refreshed)

    def test_other_document_fragment_does_not_satisfy_menu_coverage(self):
        text = '# D\n## Menu\n[A](other.md#a)\n\n## A\n'
        self.assertTrue(markdown.menu_errors(text, document='README.md'))
        refreshed = markdown.render_menu(text, document='README.md')
        self.assertIn('[A](other.md#a)', refreshed)
        self.assertEqual(markdown.menu_errors(refreshed, document='README.md'), [])

    def test_bold_code_comments_and_multiline(self):
        valid = '**good**\n\n`**code`\n\n<!-- **comment -->\n\n```python\n**code\n```\n\n**multi\nline**'
        self.assertEqual(markdown.bold_errors(valid), [])
        self.assertTrue(markdown.bold_errors('Intro\n\n**unclosed'))
        self.assertEqual(markdown.bold_errors(r'\*\*literal'), [])

    def test_redundant_final_hard_break_and_menu_refresh(self):
        redundant = '# D\n\n## Menu\n\n[A](#a)\\\n\n## A\n'
        self.assertTrue(markdown.redundant_hard_break_errors(redundant))
        refreshed = markdown.render_menu(redundant)
        self.assertEqual(markdown.redundant_hard_break_errors(refreshed), [])
        self.assertIn('[A](#a)\n\n## A\n', refreshed)

    def test_hard_break_escapes_and_inline_code_content(self):
        for count in (1, 2, 3, 4):
            text = 'Literal or break: ' + '\\' * count + '\n\n'
            with self.subTest(backslashes=count):
                self.assertEqual(bool(markdown.redundant_hard_break_errors(text)), count % 2 == 1)
        for text in ('First line\\\n`second line`\n', '`inline backslash: \\`\n\n', 'First line\\`code`\n\n', '```text\nbackslash\\\n\n```\n', '<!-- backslash\\\n\n-->\n', 'First line\\ \n\n'):
            with self.subTest(text=text):
                self.assertEqual(markdown.redundant_hard_break_errors(text), [])
        self.assertTrue(markdown.redundant_hard_break_errors('Final line\\'))


if __name__ == "__main__":
    unittest.main()
