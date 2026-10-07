# AI, UI, logging, or other modifications first developed in AdvCiv-SAS (Simple Advanced Strategy)
# (c) 2026 wonderingabout & AI/LLM helpers (see Authors in AdvCiv-SAS's root README.md)
# <!-- custom: Exercise real malformed wrappers, changed XML/report content, menu omissions, encoding signatures and amended-history objects so the new checks prove they reject regressions, not just today's clean tree. (GPT-6.1-Sol) -->
from pathlib import Path
from contextlib import redirect_stderr
import io
import shutil
import re
import subprocess
import sys
import tempfile
import unittest
import xml.etree.ElementTree as ET

ROOT = Path(__file__).resolve().parents[3]
sys.path.insert(0, str(ROOT / ".github/workflows/build"))
import art_define_structure as art
import asset_primary_tech as tech
import generated_docs as docs
import markdown_structure as markdown
import repository_hygiene as hygiene
import sas_revision_history as history
import sas_game_record_log as recorder


def git(repo, *args):
    return subprocess.check_output(["git", *args], cwd=repo, stderr=subprocess.DEVNULL).decode().strip()


def fixture_git(repo):
    git(repo, "init", "-q")
    git(repo, "config", "user.name", "CI fixture")
    git(repo, "config", "user.email", "fixture@example.invalid")
    git(repo, "config", "commit.gpgsign", "false")
    git(repo, "config", "core.autocrlf", "false")


class ArtStructureTests(unittest.TestCase):
    def test_valid_and_missing_wrapper(self):
        valid = '<Civ4ArtDefines><BuildingArtInfos><BuildingArtInfo><Type>TIPI</Type><NIF>x</NIF></BuildingArtInfo></BuildingArtInfos></Civ4ArtDefines>'
        self.assertEqual(art.check_tree(ET.fromstring(valid)), [])
        broken = valid.replace('<BuildingArtInfo>', '').replace('</BuildingArtInfo>', '')
        self.assertTrue(any('loose Type' in e for e in art.check_tree(ET.fromstring(broken))))

    def test_filename_collection_mismatch(self):
        root = ET.fromstring("<Civ4ArtDefines><UnitArtInfos/></Civ4ArtDefines>")
        self.assertTrue(art.check_tree(root, "BuildingArtInfos"))

    def test_loose_text_and_unknown_collection(self):
        for xml in ('<Civ4ArtDefines><BuildingArtInfos>stray</BuildingArtInfos></Civ4ArtDefines>', '<Civ4ArtDefines><WrongArtInfos/></Civ4ArtDefines>'):
            self.assertTrue(art.check_tree(ET.fromstring(xml)))


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


class HygieneTests(unittest.TestCase):
    def test_all_bom_signatures_and_long_paths(self):
        with tempfile.TemporaryDirectory() as temp:
            repo = Path(temp)
            fixture_git(repo)
            (repo / 'clean.txt').write_bytes(b'clean\n')
            git(repo, 'add', 'clean.txt')
            self.assertEqual(hygiene.check(repo, 'C:/Mods/SAS'), [])
            for prefix, _ in hygiene.BOMS:
                (repo / 'clean.txt').write_bytes(prefix + b'content')
                self.assertTrue(any('BOM' in e for e in hygiene.check(repo, 'C:/Mods/SAS')))
            (repo / 'clean.txt').write_bytes(b'clean\n')
            self.assertTrue(any('installed path' in e for e in hygiene.check(repo, 'C:/' + 'x' * 250)))
            self.assertTrue(hygiene.check(repo, 'C:/' + '\U0001f600' * 126))


    def test_filename_limit_counts_extensions_utf16_and_collects_all_failures(self):
        with tempfile.TemporaryDirectory() as temp:
            repo = Path(temp)
            fixture_git(repo)
            accepted = ('a' * 52 + '.txt', '\U0001f600' * 26 + '.txt')
            rejected = ('b' * 53 + '.txt', '\U0001f600' * 27 + '.txt')
            for name in accepted:
                (repo / name).write_bytes(b'clean\n')
                git(repo, 'add', name)
            self.assertEqual(hygiene.check(repo, 'C:/Mods/SAS'), [])
            for name in rejected:
                (repo / name).write_bytes(b'clean\n')
                git(repo, 'add', name)
            errors = hygiene.check(repo, 'C:/Mods/SAS')
            self.assertEqual(len(errors), 2)
            for name in rejected:
                self.assertTrue(any(error.startswith(name + ': filename uses ') for error in errors))
            self.assertTrue(all('including extension (maximum 56)' in error for error in errors))


    def test_visual_studio_utf8_boms_are_preserved(self):
        with tempfile.TemporaryDirectory() as temp:
            repo = Path(temp)
            fixture_git(repo)
            for name in ('OtherSolution.sln', 'OtherProject.vcxproj'):
                path = repo / name
                path.write_bytes(b'\xef\xbb\xbfcontent')
                git(repo, 'add', name)
            self.assertEqual(hygiene.check(repo, 'C:/Mods/SAS'), [])
            for signature, encoding in hygiene.BOMS:
                if encoding == 'UTF-8':
                    continue
                (repo / 'OtherProject.vcxproj').write_bytes(signature + b'content')
                self.assertTrue(any('BOM' in error for error in hygiene.check(repo, 'C:/Mods/SAS')))


class XmlFixture(unittest.TestCase):
    def setUp(self):
        self.temporary = tempfile.TemporaryDirectory()
        self.addCleanup(self.temporary.cleanup)
        self.repo = Path(self.temporary.name)
        # <!-- custom: Use invented resources/technologies with controlled graph relationships. Real resource renames and balance changes must not invalidate these checker regressions. (GPT-6.1-Sol) -->
        fixtures = {
            'Technologies/CIV4TechInfos.xml': '<Civ4TechInfos><TechInfos><TechInfo><Type>TECH_CI_PRIMARY</Type><Era>ERA_CI</Era><iGridX>1</iGridX><AndPreReqs/><OrPreReqs/></TechInfo><TechInfo><Type>TECH_CI_PARALLEL</Type><Era>ERA_CI</Era><iGridX>1</iGridX><AndPreReqs/><OrPreReqs/></TechInfo><TechInfo><Type>TECH_CI_LATER</Type><Era>ERA_CI</Era><iGridX>4</iGridX><AndPreReqs><PrereqTech>TECH_CI_PRIMARY</PrereqTech></AndPreReqs><OrPreReqs/></TechInfo></TechInfos></Civ4TechInfos>',
            'GameInfo/CIV4EraInfos.xml': '<Civ4EraInfos><EraInfos><EraInfo><Type>ERA_CI</Type></EraInfo></EraInfos></Civ4EraInfos>',
            'Units/CIV4UnitInfos.xml': '<Civ4UnitInfos><UnitInfos><UnitInfo><Type>UNIT_CI</Type><PrereqTech>TECH_CI_PRIMARY</PrereqTech><TechTypes/></UnitInfo></UnitInfos></Civ4UnitInfos>',
            'Buildings/CIV4BuildingInfos.xml': '<Civ4BuildingInfos><BuildingInfos><BuildingInfo><Type>BUILDING_CI</Type><SpecialBuildingType>SPECIALBUILDING_CI</SpecialBuildingType><PrereqTech>NONE</PrereqTech><TechTypes/></BuildingInfo></BuildingInfos></Civ4BuildingInfos>',
            'Buildings/CIV4SpecialBuildingInfos.xml': '<Civ4SpecialBuildingInfos><SpecialBuildingInfos><SpecialBuildingInfo><Type>SPECIALBUILDING_CI</Type><TechPrereq>TECH_CI_LATER</TechPrereq></SpecialBuildingInfo></SpecialBuildingInfos></Civ4SpecialBuildingInfos>',
            'Units/CIV4BuildInfos.xml': '<Civ4BuildInfos><BuildInfos><BuildInfo><Type>BUILD_CI</Type><PrereqTech>TECH_CI_PRIMARY</PrereqTech><ImprovementType>IMPROVEMENT_CI</ImprovementType></BuildInfo></BuildInfos></Civ4BuildInfos>',
            'Terrain/CIV4ImprovementInfos.xml': '<Civ4ImprovementInfos><ImprovementInfos><ImprovementInfo><Type>IMPROVEMENT_CI</Type><bActsAsCity>0</bActsAsCity><BonusTypeStructs><BonusTypeStruct><BonusType>BONUS_CI_ALPHA</BonusType><bBonusTrade>1</bBonusTrade></BonusTypeStruct><BonusTypeStruct><BonusType>BONUS_CI_BETA</BonusType><bBonusTrade>1</bBonusTrade></BonusTypeStruct></BonusTypeStructs></ImprovementInfo></ImprovementInfos></Civ4ImprovementInfos>',
            'Terrain/CIV4BonusInfos.xml': '<Civ4BonusInfos><BonusInfos><BonusInfo><Type>BONUS_CI_ALPHA</Type><TechReveal>NONE</TechReveal><TechCityTrade>TECH_CI_PRIMARY</TechCityTrade></BonusInfo><BonusInfo><Type>BONUS_CI_BETA</Type><TechReveal>TECH_CI_LATER</TechReveal><TechCityTrade>TECH_CI_LATER</TechCityTrade></BonusInfo></BonusInfos></Civ4BonusInfos>',
        }
        for relative, text in fixtures.items():
            target = self.repo / 'Assets/XML' / relative
            target.parent.mkdir(parents=True, exist_ok=True)
            target.write_text(text, 'utf-8')

    def replace(self, relative, old, new):
        path = self.repo / relative
        text = path.read_text('utf-8')
        self.assertTrue(old in text, "missing fixture text: " + old)
        path.write_text(text.replace(old, new, 1), 'utf-8')


class TechTests(XmlFixture):
    def test_generic_shared_and_reveal_requirements(self):
        self.assertEqual(tech.check(self.repo), [])

    def test_current_mod_rules(self):
        self.assertEqual(tech.check(ROOT), [])

    def test_later_additional_unit_requirement(self):
        self.replace('Assets/XML/Units/CIV4UnitInfos.xml', '<TechTypes/>', '<TechTypes><PrereqTech>TECH_CI_LATER</PrereqTech></TechTypes>')
        self.assertTrue(any('precedes required TECH_CI_LATER' in e for e in tech.check(self.repo)))

    def test_same_column_independent_trade_requirement(self):
        self.replace('Assets/XML/Terrain/CIV4BonusInfos.xml', '<TechCityTrade>TECH_CI_PRIMARY</TechCityTrade>', '<TechCityTrade>TECH_CI_PARALLEL</TechCityTrade>')
        self.assertTrue(any('not guaranteed' in e for e in tech.check(self.repo)))

    def test_delayed_trade_requirement_for_any_resource(self):
        self.replace('Assets/XML/Terrain/CIV4BonusInfos.xml', '<TechCityTrade>TECH_CI_PRIMARY</TechCityTrade>', '<TechCityTrade>TECH_CI_LATER</TechCityTrade>')
        self.assertTrue(any('BONUS_CI_ALPHA' in e and 'not guaranteed' in e for e in tech.check(self.repo)))

    def test_build_unlock_changes_are_followed(self):
        self.replace('Assets/XML/Units/CIV4BuildInfos.xml', '<PrereqTech>TECH_CI_PRIMARY</PrereqTech>', '<PrereqTech>TECH_CI_PARALLEL</PrereqTech>')
        self.assertTrue(tech.check(self.repo))
        self.replace('Assets/XML/Terrain/CIV4BonusInfos.xml', '<TechCityTrade>TECH_CI_PRIMARY</TechCityTrade>', '<TechCityTrade>TECH_CI_PARALLEL</TechCityTrade>')
        self.assertEqual(tech.check(self.repo), [])

    def test_renamed_resources_and_technologies_need_no_allowlist(self):
        for path in self.repo.rglob('*.xml'):
            path.write_text(path.read_text('utf-8').replace('_CI', '_RENAMED_TEST'), 'utf-8')
        self.assertEqual(tech.check(self.repo), [])

    def test_every_alternative_connection_path_must_satisfy_gate(self):
        self.replace('Assets/XML/Units/CIV4BuildInfos.xml', '</BuildInfos>', '<BuildInfo><Type>BUILD_CI_ALTERNATIVE</Type><PrereqTech>TECH_CI_PARALLEL</PrereqTech><ImprovementType>IMPROVEMENT_CI</ImprovementType></BuildInfo></BuildInfos>')
        self.assertTrue(any('BONUS_CI_ALPHA' in e and 'not guaranteed' in e for e in tech.check(self.repo)))
        self.replace('Assets/XML/Terrain/CIV4BonusInfos.xml', '<TechCityTrade>TECH_CI_PRIMARY</TechCityTrade>', '<TechCityTrade>NONE</TechCityTrade>')
        self.assertEqual(tech.check(self.repo), [])

    def test_missing_tech_fails(self):
        self.replace('Assets/XML/Units/CIV4BuildInfos.xml', '<PrereqTech>TECH_CI_PRIMARY</PrereqTech>', '<PrereqTech>UNKNOWN_SYNTHETIC_XML_TAG</PrereqTech>')
        # <!-- custom: Build prerequisite validation must reject unknown tags before graph comparisons; an empty guaranteed set alone can silently accept a resource with no trade gate. (GPT-6.1-Sol) -->
        self.assertTrue(any('UNKNOWN_SYNTHETIC_XML_TAG' in e for e in tech.check(self.repo)))


class GeneratedDocsTests(unittest.TestCase):
    def setUp(self):
        self.temporary = tempfile.TemporaryDirectory()
        self.addCleanup(self.temporary.cleanup)
        self.repo = Path(self.temporary.name)
        for relative in (docs.BASELINE, docs.REPORT, docs.HANDICAP, docs.MANUAL / 'manual.odt', docs.MANUAL / 'manual.txt'):
            target = self.repo / relative
            target.parent.mkdir(parents=True, exist_ok=True)
            shutil.copyfile(ROOT / relative, target)

    def test_current_and_changed_xml(self):
        self.assertEqual(docs.check(self.repo), [])
        path = self.repo / docs.HANDICAP
        tree = ET.parse(path)
        field = next(node for node in tree.getroot().iter() if tech.tag(node) == 'iAIResearchPercent')
        field.text = str(int(field.text) + 1)
        tree.write(path, encoding='utf-8', xml_declaration=True)
        self.assertTrue(any('handicap' in e for e in docs.check(self.repo)))

    def test_changed_report(self):
        path = self.repo / docs.REPORT
        text = path.read_text('utf-8')
        changed = re.sub(r'(Changed fields: )(\d+)', lambda match: match.group(1) + str(int(match.group(2)) + 1), text, count=1)
        self.assertNotEqual(changed, text)
        path.write_text(changed, 'utf-8')
        self.assertTrue(any('handicap' in e for e in docs.check(self.repo)))

    def test_stale_manual(self):
        with (self.repo / docs.MANUAL / 'manual.txt').open('a', encoding='utf-8') as stream:
            stream.write('stale extraction\n')
        self.assertTrue(any('manual.txt' in e for e in docs.check(self.repo)))

    def second_conversion(self):
        def convert(source, output):
            output.write_text(source.read_text('utf-8').upper(), 'utf-8')
        conversion = docs.TextConversion(Path('Other Docs/notes.source'), Path('Search Copies/notes.txt'), convert, 'python other_converter.py', (Path('Other Docs/notes.export'),))
        for relative, text in ((conversion.source, 'converted notes\n'), (conversion.output, 'CONVERTED NOTES\n'), (conversion.related_sources[0], 'related artifact')):
            target = self.repo / relative
            target.parent.mkdir(parents=True, exist_ok=True)
            target.write_text(text, 'utf-8')
        return conversion

    def test_second_conversion_uses_its_own_converter_and_paths(self):
        conversion = self.second_conversion()
        conversions = (*docs.TEXT_CONVERSIONS, conversion)
        self.assertEqual(docs.check(self.repo, conversions=conversions), [])
        (self.repo / conversion.source).write_text('updated notes\n', 'utf-8')
        errors = docs.check(self.repo, conversions=conversions)
        self.assertEqual(errors, ['Search Copies/notes.txt is stale; run python other_converter.py'])
        conversion.converter(self.repo / conversion.source, self.repo / conversion.output)
        self.assertEqual(docs.check(self.repo, conversions=conversions), [])

    def test_registered_conversion_requires_source_and_output(self):
        conversion = self.second_conversion()
        for relative in (conversion.source, conversion.output):
            (self.repo / relative).unlink()
        errors = docs.text_conversion_errors(self.repo, (conversion,))
        self.assertEqual(len(errors), 1)
        self.assertIn(conversion.source.as_posix(), errors[0])
        self.assertIn(conversion.output.as_posix(), errors[0])

    def test_second_conversion_pairs_related_source_over_entire_range(self):
        conversion = self.second_conversion()
        fixture_git(self.repo)
        git(self.repo, 'add', '.')
        git(self.repo, 'commit', '-qm', 'initial conversions fixture')
        before = git(self.repo, 'rev-parse', 'HEAD')
        (self.repo / conversion.related_sources[0]).write_text('updated related artifact', 'utf-8')
        git(self.repo, 'add', '.')
        git(self.repo, 'commit', '-qm', 'related source update')
        self.assertEqual(docs.changed_conversion_errors(self.repo, before, (conversion,)), ['Other Docs/notes.export changed without a Search Copies/notes.txt refresh in this change range'])
        (self.repo / conversion.output).write_text('reviewed paired update', 'utf-8')
        git(self.repo, 'add', '.')
        git(self.repo, 'commit', '-qm', 'later converted text update')
        self.assertEqual(docs.changed_conversion_errors(self.repo, before, (conversion,)), [])

    def test_missing_base_fails_for_pr_and_local_checks(self):
        fixture_git(self.repo)
        for event in (None, 'pull_request'):
            errors = docs.changed_conversion_errors(self.repo, 'f' * 40, event_name=event)
            self.assertEqual(len(errors), 1)
            self.assertIn('change-range base', errors[0])
            self.assertIn('unavailable', errors[0])

    def test_missing_push_base_reports_gap_but_still_checks_current_content(self):
        fixture_git(self.repo)
        diagnostics = io.StringIO()
        with redirect_stderr(diagnostics):
            self.assertEqual(docs.check(self.repo, 'f' * 40, event_name='push'), [])
        self.assertIn('paired source/text change-range validation cannot run', diagnostics.getvalue())
        with (self.repo / docs.MANUAL / 'manual.txt').open('a', encoding='utf-8') as stream:
            stream.write('stale extraction')
        with redirect_stderr(io.StringIO()):
            errors = docs.check(self.repo, 'f' * 40, event_name='push')
        self.assertTrue(any('manual.txt is stale' in error for error in errors))

    def test_missing_push_base_is_fetched_and_pairing_is_enforced(self):
        fixture_git(self.repo)
        pdf = self.repo / docs.MANUAL / 'manual.pdf'
        pdf.write_bytes(b'old PDF fixture')
        git(self.repo, 'add', '.')
        git(self.repo, 'commit', '-qm', 'initial fixture')
        with tempfile.TemporaryDirectory() as temporary:
            clone = Path(temporary) / 'runner'
            subprocess.run(['git', 'clone', '-q', str(self.repo), str(clone)], check=True)
            pdf.write_bytes(b'changed PDF fixture')
            git(self.repo, 'add', '.')
            git(self.repo, 'commit', '-qm', 'commit absent from runner checkout')
            before = git(self.repo, 'rev-parse', 'HEAD')
            self.assertFalse(docs.has_commit(clone, before))
            errors = docs.changed_conversion_errors(clone, before, event_name='push')
            self.assertTrue(docs.has_commit(clone, before))
            self.assertTrue(any('manual.pdf changed without' in error for error in errors))

    def test_root_push_without_previous_tree_needs_no_range(self):
        self.assertEqual(docs.changed_conversion_errors(self.repo, '0' * 40, event_name='push'), [])

    def test_pdf_pairing_over_entire_range(self):
        fixture_git(self.repo)
        pdf = self.repo / docs.MANUAL / 'manual.pdf'
        pdf.write_bytes(b'old PDF fixture')
        git(self.repo, 'add', '.')
        git(self.repo, 'commit', '-qm', 'initial fixture')
        before = git(self.repo, 'rev-parse', 'HEAD')
        pdf.write_bytes(b'new PDF fixture')
        git(self.repo, 'add', '.')
        git(self.repo, 'commit', '-qm', 'PDF fixture update')
        self.assertTrue(docs.changed_conversion_errors(self.repo, before))
        with (self.repo / docs.MANUAL / 'manual.txt').open('a') as stream:
            stream.write('paired fixture update')
        git(self.repo, 'add', '.')
        git(self.repo, 'commit', '-qm', 'later text fixture update')
        self.assertEqual(docs.changed_conversion_errors(self.repo, before), [])


class RevisionDocumentationTests(unittest.TestCase):
    def setUp(self):
        self.temporary = tempfile.TemporaryDirectory()
        self.addCleanup(self.temporary.cleanup)
        self.repo = Path(self.temporary.name)
        for relative in (recorder.REVISION_HEADER, recorder.REVISION_SOURCE, recorder.REVISION_HISTORY):
            target = self.repo / relative
            target.parent.mkdir(parents=True, exist_ok=True)
            shutil.copyfile(ROOT / relative, target)
        self.history = self.repo / recorder.REVISION_HISTORY
        self.text = self.history.read_text('utf-8')

    def test_current_revision_documentation(self):
        self.assertEqual(recorder.check_revision(self.repo), [])

    def test_source_bump_without_history_update(self):
        path = self.repo / recorder.REVISION_HEADER
        text = path.read_text('utf-8')
        updated = re.sub(r'(SAS_GAME_RECORD_REVISION\s*=\s*)(\d+)', lambda match: match.group(1) + str(int(match.group(2)) + 1), text, count=1)
        path.write_text(updated, 'utf-8')
        self.assertTrue(any('revision mismatch' in error for error in recorder.check_revision(self.repo)))

    def test_removed_latest_history_entry(self):
        entries = list(re.finditer(r'^### Revision ', self.text, flags=re.MULTILINE))
        self.history.write_text(self.text[:entries[0].start()] + self.text[entries[1].start():], 'utf-8')
        self.assertTrue(any('revision mismatch' in error for error in recorder.check_revision(self.repo)))

    def test_stale_documented_emitted_marker(self):
        text = re.sub(r'(GAME_RECORD_SOURCE_CONTEXT recordRevision=)(\d+)', lambda match: match.group(1) + str(int(match.group(2)) - 1), self.text, count=1)
        self.history.write_text(text, 'utf-8')
        self.assertTrue(any('current emitted recordRevision example' in error for error in recorder.check_revision(self.repo)))

    def test_malformed_extra_heading_cannot_be_skipped(self):
        self.history.write_text(self.text + '\n### Revision malformed\n', 'utf-8')
        self.assertTrue(any('malformed revision heading' in error for error in recorder.check_revision(self.repo)))

    def test_latest_history_metadata_is_required(self):
        for field in ('Date', 'Git commit', 'Change'):
            with self.subTest(field=field):
                text = re.sub(r'^- \*\*' + re.escape(field) + r':\*\*[^\n]*', '- **' + field + ':** ', self.text, count=1, flags=re.MULTILINE)
                self.history.write_text(text, 'utf-8')
                self.assertTrue(any('completed ' + field in error for error in recorder.check_revision(self.repo)))


class RevisionHistoryTests(unittest.TestCase):
    def test_feature_rewrite_does_not_replace_default_branch_commit(self):
        with tempfile.TemporaryDirectory() as temp:
            repo = Path(temp)
            fixture_git(repo)
            header = repo / history.HEADER
            header.parent.mkdir(parents=True)
            header.write_text('enum { SAS_GAME_RECORD_REVISION = 69 };\n')
            git(repo, 'add', '.')
            git(repo, 'commit', '-qm', 'original fixture revision')
            original = git(repo, 'rev-parse', 'HEAD')
            git(repo, 'branch', 'main')
            git(repo, 'switch', '-qc', 'feature')
            git(repo, 'commit', '--amend', '-qm', 'amended fixture revision')
            amended = git(repo, 'rev-parse', 'HEAD')
            target = repo / history.HISTORY
            target.parent.mkdir(parents=True)
            def record(commit):
                target.write_text('### Revision 70 - fixture\n- **Git commit:** pending\n\n### Revision 69 - fixture\n- **Git commit:** `' + commit + '`\n', 'utf-8')
            record(original)
            self.assertEqual(history.check(repo, "main"), [])
            record(amended)
            git(repo, 'cat-file', '-e', amended)
            self.assertTrue(any('ancestry' in e for e in history.check(repo, "main")))
            record(original)
            git(repo, 'cat-file', '-e', original)
            record('pending')
            self.assertTrue(any('latest entry' in e for e in history.check(repo, "main")))
            record(original)
            target.write_text(target.read_text().replace('Revision 69', 'Revision 71'))
            self.assertTrue(any('matching explicit source marker' in e for e in history.check(repo, "main")))

    def test_missing_default_branch_abstains_from_ancestry(self):
        with tempfile.TemporaryDirectory() as temp:
            repo = Path(temp)
            fixture_git(repo)
            target = repo / history.HISTORY
            target.parent.mkdir(parents=True)
            target.write_text('### Revision 68 - fixture\n- **Git commit:** `0000000000000000000000000000000000000000`\n', 'utf-8')
            self.assertEqual(history.check(repo, "refs/remotes/origin/missing"), [])


if __name__ == '__main__':
    unittest.main()
