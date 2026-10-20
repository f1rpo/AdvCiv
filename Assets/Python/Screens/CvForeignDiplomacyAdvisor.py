## Sid Meier's Civilization 4
## Copyright Firaxis Games 2005
#
# AI, UI, logging, or other modifications first developed in AdvCiv-SAS (Simple Advanced Strategy)
# (c) 2026 wonderingabout & AI/LLM helpers (see Authors in AdvCiv-SAS's root README.md)
#
from CvPythonExtensions import *
import CvScreenEnums
import CvForeignAdvisor
# <!-- custom: F3 inherits the Foreign Advisor, but its font helpers now belong to SASTextScale; the former CvForeignAdvisor star-import exports disappeared during consolidation and opening this screen raised AttributeError.
# Import the actual text-scale owner directly instead of depending on another screen to re-export its helpers. (GPT-6.1-Sol) -->
import SASTextScale

class CvForeignDiplomacyAdvisor(CvForeignAdvisor.CvForeignAdvisor):
	# <!-- custom: diplomacy/intel shell over canonical Foreign advisor codebase so F3 stays the "derived split" while F4 keeps the base trade cluster. (GPT-5.3-Codex) -->
	def __init__(self):
		CvForeignAdvisor.CvForeignAdvisor.__init__(self)
		self.SCREEN_NAME = "ForeignAdvisor"
		self.FOREIGN_TAB_KEYS = self.DIPLOMACY_TAB_KEYS

	def initText(self):
		CvForeignAdvisor.CvForeignAdvisor.initText(self)
		self.SCREEN_TITLE = SASTextScale.sasFontTagTitle.bold + CvForeignAdvisor.localText.getText("TXT_KEY_FOREIGN_ADVISOR_TITLE", ()).upper() + SASTextScale.SAS_FONT_TAG_CLOSE

	def getScreen(self):
		return CyGInterfaceScreen(self.SCREEN_NAME + str(self.iScreen), CvScreenEnums.FOREIGN_DIPLOMACY_ADVISOR)
