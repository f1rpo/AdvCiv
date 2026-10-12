# AI, UI, logging, or other modifications first developed in AdvCiv-SAS (Simple Advanced Strategy)
# (c) 2026 wonderingabout & AI/LLM helpers (see Authors in AdvCiv-SAS's root README.md)
from pathlib import Path
import sys
import unittest
import xml.etree.ElementTree as ET

ROOT = Path(__file__).resolve().parents[3]
sys.path.insert(0, str(ROOT / ".github/workflows/build/assets"))
import art_define_structure as art

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


if __name__ == "__main__":
    unittest.main()
