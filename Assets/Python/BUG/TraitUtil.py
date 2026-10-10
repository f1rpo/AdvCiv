## TraitUtil
##
## Utilities for dealing with Traits and TraitInfos.
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

GENERIC_ICON = "*"
TRAIT_ICONS = {}

GENERIC_BUTTON = "Art/Interface/Buttons/TechTree/"
TRAIT_BUTTONS = {}

gc = CyGlobalContext()

def init():
	"Performs one-time initialization after the game starts up."
	game = gc.getGame()
	global GENERIC_ICON
	GENERIC_ICON = u"%c" % game.getSymbolID(FontSymbols.MAP_CHAR)

	addTrait("AGGRESSIVE", game.getSymbolID(FontSymbols.STRENGTH_CHAR), "Art/Interface/Buttons/Promotions/Combat1.dds")
	addTrait("CHARISMATIC", game.getSymbolID(FontSymbols.HAPPY_CHAR), "Art/Interface/Buttons/TechTree/MassMedia.dds")
	addTrait("CREATIVE", gc.getCommerceInfo(CommerceTypes.COMMERCE_CULTURE).getChar(), "Art/Interface/Buttons/TechTree/Music.dds")
	# <!-- custom: TRAIT_EXPANSIVE removed; keep entry disabled. (GPT-5.2-Codex (summarized)) -->
	#addTrait("EXPANSIVE", game.getSymbolID(FontSymbols.HEALTHY_CHAR), "Art/Interface/Buttons/Actions/Heal.dds")
	addTrait("FINANCIAL", gc.getCommerceInfo(CommerceTypes.COMMERCE_GOLD).getChar(), "Art/Interface/Buttons/TechTree/Banking.dds")
	# <!-- custom: change trait char (was OCCUPATION_CHAR). (GPT-5.2-Codex (summarized)) -->
	addTrait("IMPERIALIST", game.getSymbolID(FontSymbols.TRADE_CHAR), "Art/Interface/Buttons/Actions/FoundCity.dds")
	addTrait("INDUSTRIOUS", gc.getYieldInfo(YieldTypes.YIELD_PRODUCTION).getChar(), "Art/Interface/Buttons/TechTree/Industrialism.dds")
	# <!-- custom: change trait char (was TRADE_CHAR). (GPT-5.2-Codex (summarized)) -->
	addTrait("ORGANIZED", game.getSymbolID(FontSymbols.OCCUPATION_CHAR), "Art/Interface/Buttons/Buildings/Courthouse.dds")
	addTrait("PHILOSOPHICAL", game.getSymbolID(FontSymbols.GREAT_PEOPLE_CHAR), "Art/Interface/Buttons/TechTree/Philosophy.dds")
	addTrait("PROTECTIVE", game.getSymbolID(FontSymbols.DEFENSE_CHAR), "Art/Interface/Buttons/Promotions/CityGarrison1.dds")
	addTrait("SPIRITUAL", game.getSymbolID(FontSymbols.RELIGION_CHAR), "Art/Interface/Buttons/TechTree/Meditation.dds")

def addTrait(trait, icon, button):
	eTrait = getInfoTypeOrFail("TRAIT_" + trait)
	if eTrait != -1:
		if icon is not None:
			TRAIT_ICONS[eTrait] = u"%c" % icon
		if button is not None:
			TRAIT_BUTTONS[eTrait] = button

def getIcon(eTrait):
	if eTrait in TRAIT_ICONS:
		return TRAIT_ICONS[eTrait]
	else:
		return GENERIC_ICON

# <!-- custom: Format CyGameTextMgr leader-trait text with each trait name preceded by its TraitUtil icon.
# Dawn of Man and Victory Screen both use this compact "icon Trait" display, while CyGameTextMgr keeps the normal trait wording/order. (GPT-5.5) -->
def getLeaderTraitsWithIcons(eLeader, eCiv):
	szTraits = CyGameTextMgr().parseLeaderTraits(eLeader, eCiv, True, False)
	leaderInfo = gc.getLeaderHeadInfo(eLeader)
	for iTrait in range(gc.getNumTraitInfos()):
		if leaderInfo.hasTrait(iTrait):
			traitDesc = gc.getTraitInfo(iTrait).getDescription()
			szTraits = szTraits.replace(traitDesc, getIcon(iTrait) + u" " + traitDesc, 1)
	return szTraits

def getButton(eTrait):
	if eTrait in TRAIT_BUTTONS:
		return TRAIT_BUTTONS[eTrait]
	else:
		return GENERIC_BUTTON
