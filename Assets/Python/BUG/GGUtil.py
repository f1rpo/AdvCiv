## GGUtil
##
## Utilities for dealing with Great Generals.
##
## Notes
##   - Must be initialized externally by calling init()
##
## Copyright (c) 2008 The BUG Mod.
##
## Author: EmperorFool
#
# AI, UI, logging, or other modifications first developed in AdvCiv-SAS (Simple Advanced Strategy)
# (c) 2026 wonderingabout & AI/LLM helpers (see Authors in AdvCiv-SAS's root README.md)
#
# <!-- custom: AdvCiv-SAS does not actively maintain this third-party BUG library file.
# Edits here are limited to repo-wide consistency passes (e.g. getInfoTypeOrFail for fail-loud XML lookups). (Claude code Opus 4.7) -->

from CvPythonExtensions import *
from SASUtils import getInfoTypeOrFail
import BugUtil
import FontUtil

gc = CyGlobalContext()

g_ePromo = -1
g_promoButton = ""
g_cGreatGeneral = ""

def init():
	global g_ePromo
	g_ePromo = getInfoTypeOrFail("PROMOTION_LEADER")
	global g_promoButton
	g_promoButton = gc.getPromotionInfo(g_ePromo).getButton()
	global g_cGreatGeneral
	g_cGreatGeneral = FontUtil.getChar(FontSymbols.GREAT_GENERAL_CHAR)

def getPromotionId():
	return g_ePromo

def getPromotion():
	return gc.getPromotionInfo(g_ePromo)

def getGreatGeneralText(iNeededExp):
	return BugUtil.getText("INTERFACE_NEXT_GREAT_GENERAL_XP", (g_cGreatGeneral, iNeededExp))
