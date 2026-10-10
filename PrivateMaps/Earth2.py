#
#   FILE:    Earth2.py
#   AUTHOR:  GRM7584 (Script adapted directly from Sirian's Terra script)
#   PURPOSE: Global map script - Simulates Randomized Earth
#-----------------------------------------------------------------------------
#   Copyright (c) 2008 Firaxis Games, Inc. All rights reserved.
#-----------------------------------------------------------------------------
#
# AI, UI, logging, or other modifications first developed in AdvCiv-SAS (Simple Advanced Strategy)
# (c) 2026 wonderingabout & AI/LLM helpers (see Authors in AdvCiv-SAS's root README.md)
#

from CvPythonExtensions import *
import CvUtil
import CvMapGeneratorUtil
from CvMapGeneratorUtil import MultilayeredFractal
from CvMapGeneratorUtil import TerrainGenerator
from CvMapGeneratorUtil import FeatureGenerator
from SAS_WorldSizeUtils import *
from SASUtils import getInfoTypeOrFail

# MULTILAYERED FRACTAL NOTES
#
# The MultilayeredFractal class was created for use with this script.
#
# I worked to make it adaptable to other scripts, though, and eventually it
# migrated in to the MapUtil file along with the other primary map classes.
#
# - Bob Thomas   July 13, 2005
#
#
# TERRA NOTES
#
# Terra turns out to be our largest size map. This is the only map script
# in the original release of Civ4 where the grids are this large!
#
# This script is also the one that got me started in to map scripting. I had 
# this idea early in the development cycle and just kept pestering until Soren 
# turned me loose on it, finally. Once I got going, I just kept on going!
#
# - Bob Thomas   September 20, 2005
#
# EARTH2 NOTES
#
# This is based purely on the Terra script, albeit with a lot more similarity
# to Earth in terms of landmasses. Rocky Climate and Normal Sea Levels strongly
# recommended for maximum earthiness.

def getDescription():
    return "TXT_KEY_MAP_SCRIPT_EARTH2_DESCR"

def isAdvancedMap():
    "This map should not show up in simple mode"
    return 1

def getGridSize(argsList):
    "Enlarge the grids! According to Soren, Earth-type maps are usually huge anyway."
    if (argsList[0] == -1): # (-1,) is passed to function on loads
        return []
    [eWorldSize] = argsList
    # <!-- custom: Use DLL WorldSizeTypes values aligned with CIV4WorldInfo.xml order.
    # Earth2's staged start distribution was designed to keep Huge and SAS24-48 at the same already-large grid, so use ordinary lookup and its intentional last-tier cap. See KI#281. (Claude code Opus 4.7; GPT-5.5; ChatGPT-5.6-Sol + GPT-5.6-Sol) -->
    grid_sizes = {
        WorldSizeTypes.WORLDSIZE_ARENA: (7,4),
        WorldSizeTypes.WORLDSIZE_DUEL: (10,6),
        WorldSizeTypes.WORLDSIZE_TINY: (15,9),
        WorldSizeTypes.WORLDSIZE_SMALL: (20,12),
        WorldSizeTypes.WORLDSIZE_STANDARD: (25,15),
        WorldSizeTypes.WORLDSIZE_LARGE: (30,18),
        WorldSizeTypes.WORLDSIZE_HUGE: (40,24),
    }
    return sas_lookup_world_size(eWorldSize, grid_sizes)

def minStartingDistanceModifier():
    return -15

def findStartingPlot(argsList):
	[playerID] = argsList

	# <!-- custom: Earth2 is already large enough; for high player counts, better distribution works better than forcing larger grids.
	# Staged starts are define-driven so tuning does not require Python edits: keep classic largest-landmass starts at low counts, then unlock Africa, then unlock full-world starts.
	# This staging uses player count only; world size affects grid dimensions separately in getGridSize. (GPT-5.3-Codex) -->
	map = CyMap()
	gcLocal = CyGlobalContext()
	iNumPlayers = gcLocal.getGame().countCivPlayersEverAlive()
	iBiggestAreaID = map.findBiggestArea(False).getID()

	iAfricaMinPlayers = gcLocal.getDefineINT("SAS_EARTH2_STARTS_POPULATE_AFRICA_MIN_PLAYERS")
	iFullWorldMinPlayers = gcLocal.getDefineINT("SAS_EARTH2_STARTS_POPULATE_FULL_WORLD_MIN_PLAYERS")

	iAfricaWestX = int(map.getGridWidth() * 0.50)
	iAfricaEastX = int(map.getGridWidth() * 0.68)
	iAfricaSouthY = int(map.getGridHeight() * 0.18)
	iAfricaNorthY = int(map.getGridHeight() * 0.64)

	def isValid(playerID, x, y):
		pPlot = map.plot(x, y)

		if iNumPlayers < iAfricaMinPlayers:
			return (pPlot.getArea() == iBiggestAreaID)

		bInAfrica = (x >= iAfricaWestX and x <= iAfricaEastX and y >= iAfricaSouthY and y <= iAfricaNorthY)
		# <!-- custom: The 16-31-player stage was intended to add Africa to the classic biggest landmass, but its Africa condition was unreachable and the remaining exclusions admitted every other Old-World island.
		# Apply the stated union directly; 32+ retains unrestricted full-world starts. See KI#280. (ChatGPT-5.6-Sol + GPT-5.6-Sol) -->
		if iNumPlayers < iFullWorldMinPlayers:
			return (pPlot.getArea() == iBiggestAreaID) or bInAfrica

		return True

	return CvMapGeneratorUtil.findStartingPlot(playerID, isValid)

class EarthMultilayeredFractal(CvMapGeneratorUtil.MultilayeredFractal):
    # Subclass. Only the controlling function overridden in this case.
    def generatePlotsByRegion(self):
        # Sirian's MultilayeredFractal class, controlling function.
        # You -MUST- customize this function for each use of the class.
        #
        # The following grain matrix is specific to Earth2.py
        sizekey = self.map.getWorldSize()
        # <!-- custom: Earth2's legacy grain table omitted Arena, so the smallest tier fell upward to Huge grain.
        # Give Arena the intended Duel/Tiny tuple; SAS24-48 intentionally retain Huge grain alongside Earth2's fixed-Huge grid policy. See KI#282. (ChatGPT-5.6-Sol + GPT-5.6-Sol) -->
        sizevalues = {
            WorldSizeTypes.WORLDSIZE_ARENA:     (3,2,1),
            WorldSizeTypes.WORLDSIZE_DUEL:      (3,2,1),
            WorldSizeTypes.WORLDSIZE_TINY:      (3,2,1),
            WorldSizeTypes.WORLDSIZE_SMALL:     (4,2,1),
            WorldSizeTypes.WORLDSIZE_STANDARD:  (4,2,1),
            WorldSizeTypes.WORLDSIZE_LARGE:     (4,2,1),
            WorldSizeTypes.WORLDSIZE_HUGE:      (5,2,1)
            }
        (ScatterGrain, BalanceGrain, GatherGrain) = sas_lookup_world_size(sizekey, sizevalues)

        # Sea Level adjustment (from user input), limited to value of 5%.
        sea = self.gc.getSeaLevelInfo(self.map.getSeaLevel()).getSeaLevelChange()
        sea = min(sea, 5)
        sea = max(sea, -5)

        # The following regions are specific to Earth2.py

        NATundraWestLon = 0.05
        NATundraEastLon = 0.21
        GreenlandWestLon = 0.26
        GreenlandEastLon = 0.39
        NAmericasWestLon = 0.10
        NAmericasEastLon = 0.29
        FloridaWestLon = 0.28
        FloridaEastLon = 0.30
        MexicoWestLon = 0.12
        MexicoEastLon = 0.20
        CAmericasWestLon = 0.13
        CAmericasEastLon = 0.25
        PanamaWestLon = 0.21
        PanamaEastLon = 0.25
        CaribWestLon = 0.17
        CaribEastLon = 0.35
        SAmericasWestLon = 0.19
        SAmericasEastLon = 0.33
        BrazilWestLon = 0.21
        BrazilEastLon = 0.37
        AndesWestLon = 0.23
        AndesEastLon = 0.26
        EmeraldWestLon = 0.54
        EmeraldEastLon = 0.58
        NEuropeWestLon = 0.60
        NEuropeEastLon = 0.68
        CEuropeWestLon = 0.54
        CEuropeEastLon = 0.68
        IberiaWestLon = 0.51
        IberiaEastLon = 0.55
        MediWestLon = 0.54
        MediEastLon = 0.68
        NumidiaWestLon = 0.50
        NumidiaEastLon = 0.58
        AfricaWestLon = 0.50
        AfricaEastLon = 0.68
        CAfricaWestLon = 0.58
        CAfricaEastLon = 0.68
        SAfricaWestLon = 0.60
        SAfricaEastLon = 0.66
        SiberiaWestLon = 0.67
        SiberiaEastLon = 0.95
        SteppeWestLon = 0.67
        SteppeEastLon = 0.92
        NearEastWestLon = 0.67
        NearEastEastLon = 0.75
        ArabiaWestLon = 0.68
        ArabiaEastLon = 0.73
        IndiaWestLon = 0.73
        IndiaEastLon = 0.81
        ChinaWestLon = 0.80
        ChinaEastLon = 0.89
        IndoChinaWestLon = 0.80
        IndoChinaEastLon = 0.94
        JapanWestLon = 0.91
        JapanEastLon = 0.94
        AustraliaWestLon = 0.84
        AustraliaEastLon = 0.96
        SouthPacificWestLon = 0.01
        SouthPacificEastLon = 0.20
        NATundraNorthLat = 0.94
        NATundraSouthLat = 0.80
        GreenlandNorthLat = 0.90
        GreenlandSouthLat = 0.78
        NAmericasNorthLat = 0.82
        NAmericasSouthLat = 0.65
        FloridaNorthLat = 0.66
        FloridaSouthLat = 0.64
        MexicoNorthLat = 0.70
        MexicoSouthLat = 0.60
        CAmericasNorthLat = 0.65
        CAmericasSouthLat = 0.54
        PanamaNorthLat = 0.55
        PanamaSouthLat = 0.54
        CaribNorthLat = 0.66
        CaribSouthLat = 0.50
        SAmericasNorthLat = 0.55
        SAmericasSouthLat = 0.25
        BrazilNorthLat = 0.50
        BrazilSouthLat = 0.40
        AndesNorthLat = 0.56
        AndesSouthLat = 0.14
        EmeraldNorthLat = 0.86
        EmeraldSouthLat = 0.81
        NEuropeNorthLat = 0.92
        NEuropeSouthLat = 0.80
        CEuropeNorthLat = 0.80
        CEuropeSouthLat = 0.72
        IberiaNorthLat = 0.74
        IberiaSouthLat = 0.68
        MediNorthLat = 0.75
        MediSouthLat = 0.60
        AfricaNorthLat = 0.60
        AfricaSouthLat = 0.44
        NumidiaNorthLat = 0.64
        NumidiaSouthLat = 0.55
        CAfricaNorthLat = 0.46
        CAfricaSouthLat = 0.25
        SAfricaNorthLat = 0.30
        SAfricaSouthLat = 0.18
        SiberiaNorthLat = 0.94
        SiberiaSouthLat = 0.82
        SteppeNorthLat = 0.85
        SteppeSouthLat = 0.70
        NearEastNorthLat = 0.75
        NearEastSouthLat = 0.59
        ArabiaNorthLat = 0.60
        ArabiaSouthLat = 0.55
        IndiaNorthLat = 0.75
        IndiaSouthLat = 0.47
        ChinaNorthLat = 0.75
        ChinaSouthLat = 0.50
        IndoChinaNorthLat = 0.55
        IndoChinaSouthLat = 0.32
        JapanNorthLat = 0.75
        JapanSouthLat = 0.65
        AustraliaNorthLat = 0.30
        AustraliaSouthLat = 0.15
        SouthPacificNorthLat = 0.45
        SouthPacificSouthLat = 0.15

        # Simulate the Western Hemisphere - North American Tundra.
        NiTextOut("Generating North America (Python Earth2) ...")
        # Set dimensions of North American Tundra
        NATundraWestX = int(self.iW * NATundraWestLon)
        NATundraEastX = int(self.iW * NATundraEastLon)
        NATundraNorthY = int(self.iH * NATundraNorthLat)
        NATundraSouthY = int(self.iH * NATundraSouthLat)
        NATundraWidth = NATundraEastX - NATundraWestX + 1
        NATundraHeight = NATundraNorthY - NATundraSouthY + 1

        NATundraWater = 35+sea

        self.generatePlotsInRegion(NATundraWater, NATundraWidth, NATundraHeight, NATundraWestX, NATundraSouthY, BalanceGrain, GatherGrain, self.iRoundFlags, self.iTerrainFlags, 5, 5, True, 5, -1, False, False)

        # Simulate the Western Hemisphere - Greenland.
        NiTextOut("Generating North America (Python Earth2) ...")
        # Set dimensions of Greenland
        GreenlandWestX = int(self.iW * GreenlandWestLon)
        GreenlandEastX = int(self.iW * GreenlandEastLon)
        GreenlandNorthY = int(self.iH * GreenlandNorthLat)
        GreenlandSouthY = int(self.iH * GreenlandSouthLat)
        GreenlandWidth = GreenlandEastX - GreenlandWestX + 1
        GreenlandHeight = GreenlandNorthY - GreenlandSouthY + 1

        GreenlandWater = 70+sea

        self.generatePlotsInRegion(GreenlandWater, GreenlandWidth, GreenlandHeight, GreenlandWestX, GreenlandSouthY, ScatterGrain, GatherGrain, self.iRoundFlags, self.iTerrainFlags, 5, 5, True, 5, 1, True, False)

        self.generatePlotsInRegion(GreenlandWater, GreenlandWidth, GreenlandHeight, GreenlandWestX, GreenlandSouthY, ScatterGrain, GatherGrain, self.iRoundFlags, self.iTerrainFlags, 5, 5, True, 5, 1, True, False)

        # Simulate the Western Hemisphere - North America Mainland.
        NiTextOut("Generating North America (Python Earth2) ...")
        # Set dimensions of North America Mainland
        NAmericasWestX = int(self.iW * NAmericasWestLon)
        NAmericasEastX = int(self.iW * NAmericasEastLon)
        NAmericasNorthY = int(self.iH * NAmericasNorthLat)
        NAmericasSouthY = int(self.iH * NAmericasSouthLat)
        NAmericasWidth = NAmericasEastX - NAmericasWestX + 1
        NAmericasHeight = NAmericasNorthY - NAmericasSouthY + 1

        NAmericasWater = 40+sea

        self.generatePlotsInRegion(NAmericasWater, NAmericasWidth, NAmericasHeight, NAmericasWestX, NAmericasSouthY, GatherGrain, BalanceGrain, self.iVertFlags, self.iTerrainFlags, 5, 5, True, 5, -1, False, False)

        self.generatePlotsInRegion(NAmericasWater, NAmericasWidth, NAmericasHeight, NAmericasWestX, NAmericasSouthY, GatherGrain, BalanceGrain, self.iVertFlags, self.iTerrainFlags, 5, 5, True, 5, -1, False, False)

        # Simulate the Western Hemisphere - Florida.
        NiTextOut("Generating North America (Python Earth2) ...")
        # Set dimensions of Florida
        FloridaWestX = int(self.iW * FloridaWestLon)
        FloridaEastX = int(self.iW * FloridaEastLon)
        FloridaNorthY = int(self.iH * FloridaNorthLat)
        FloridaSouthY = int(self.iH * FloridaSouthLat)
        FloridaWidth = FloridaEastX - FloridaWestX + 1
        FloridaHeight = FloridaNorthY - FloridaSouthY + 1

        FloridaWater = 40+sea

        self.generatePlotsInRegion(FloridaWater, FloridaWidth, FloridaHeight, FloridaWestX, FloridaSouthY, GatherGrain, ScatterGrain, self.iVertFlags, self.iTerrainFlags, 5, 5, True, 5, -1, False, False)

        # Simulate the Western Hemisphere - Mexico.
        NiTextOut("Generating Central America (Python Earth2) ...")
        # Set dimensions of Mexico
        MexicoWestX = int(self.iW * MexicoWestLon)
        MexicoEastX = int(self.iW * MexicoEastLon)
        MexicoNorthY = int(self.iH * MexicoNorthLat)
        MexicoSouthY = int(self.iH * MexicoSouthLat)
        MexicoWidth = MexicoEastX - MexicoWestX + 1
        MexicoHeight = MexicoNorthY - MexicoSouthY + 1

        MexicoWater = 30+sea

        self.generatePlotsInRegion(MexicoWater, MexicoWidth, MexicoHeight, MexicoWestX, MexicoSouthY, GatherGrain, GatherGrain, self.iRoundFlags, self.iTerrainFlags, 5, 5, True, 5, -1, False, False)

        # Simulate the Western Hemisphere - Central America.
        NiTextOut("Generating Central America (Python Earth2) ...")
        # Set dimensions of Central America
        CAmericasWestX = int(self.iW * CAmericasWestLon)
        CAmericasEastX = int(self.iW * CAmericasEastLon)
        CAmericasNorthY = int(self.iH * CAmericasNorthLat)
        CAmericasSouthY = int(self.iH * CAmericasSouthLat)
        CAmericasWidth = CAmericasEastX - CAmericasWestX + 1
        CAmericasHeight = CAmericasNorthY - CAmericasSouthY + 1

        CAmericasWater = 80+sea

        self.generatePlotsInRegion(CAmericasWater, CAmericasWidth, CAmericasHeight, CAmericasWestX, CAmericasSouthY, GatherGrain, GatherGrain, self.iVertFlags, self.iTerrainFlags, 5, 5, True, 5, -1, False, False)

        # Simulate the Western Hemisphere - Panama.
        NiTextOut("Generating Central America (Python Earth2) ...")
        # Set dimensions of Panama
        PanamaWestX = int(self.iW * PanamaWestLon)
        PanamaEastX = int(self.iW * PanamaEastLon)
        PanamaNorthY = int(self.iH * PanamaNorthLat)
        PanamaSouthY = int(self.iH * PanamaSouthLat)
        PanamaWidth = PanamaEastX - PanamaWestX + 1
        PanamaHeight = PanamaNorthY - PanamaSouthY + 1

        PanamaWater = 85+sea

        self.generatePlotsInRegion(PanamaWater, PanamaWidth, PanamaHeight, PanamaWestX, PanamaSouthY, GatherGrain, GatherGrain, self.iHorzFlags, self.iTerrainFlags, 5, 5, True, 5, -1, False, False)

        # Simulate the Western Hemisphere - The Caribbean.
        NiTextOut("Generating Central America (Python Earth2) ...")
        # Set dimensions of The Caribbean
        CaribWestX = int(self.iW * CaribWestLon)
        CaribEastX = int(self.iW * CaribEastLon)
        CaribNorthY = int(self.iH * CaribNorthLat)
        CaribSouthY = int(self.iH * CaribSouthLat)
        CaribWidth = CaribEastX - CaribWestX + 1
        CaribHeight = CaribNorthY - CaribSouthY + 1

        CaribWater = 90+sea

        self.generatePlotsInRegion(CaribWater, CaribWidth, CaribHeight, CaribWestX, CaribSouthY, ScatterGrain, BalanceGrain, self.iRoundFlags, self.iTerrainFlags, 5, 5, True, 5, -1, False, False)

        # Simulate the Western Hemisphere - South America.
        NiTextOut("Generating South America (Python Earth2) ...")
        # Set dimensions of South America
        SAmericasWestX = int(self.iW * SAmericasWestLon)
        SAmericasEastX = int(self.iW * SAmericasEastLon)
        SAmericasNorthY = int(self.iH * SAmericasNorthLat)
        SAmericasSouthY = int(self.iH * SAmericasSouthLat)
        SAmericasWidth = SAmericasEastX - SAmericasWestX + 1
        SAmericasHeight = SAmericasNorthY - SAmericasSouthY + 1

        SAmericasWater = 65+sea

        self.generatePlotsInRegion(SAmericasWater, SAmericasWidth, SAmericasHeight, SAmericasWestX, SAmericasSouthY, GatherGrain, GatherGrain, self.iVertFlags, self.iTerrainFlags, 5, 5, True, 5, -1, False, False)

        self.generatePlotsInRegion(SAmericasWater, SAmericasWidth, SAmericasHeight, SAmericasWestX, SAmericasSouthY, GatherGrain, GatherGrain, self.iRoundFlags, self.iTerrainFlags, 5, 5, True, 5, -1, False, False)

        # Simulate the Western Hemisphere - Brazil.
        NiTextOut("Generating South America (Python Earth2) ...")
        # Set dimensions of Brazil
        BrazilWestX = int(self.iW * BrazilWestLon)
        BrazilEastX = int(self.iW * BrazilEastLon)
        BrazilNorthY = int(self.iH * BrazilNorthLat)
        BrazilSouthY = int(self.iH * BrazilSouthLat)
        BrazilWidth = BrazilEastX - BrazilWestX + 1
        BrazilHeight = BrazilNorthY - BrazilSouthY + 1

        BrazilWater = 45+sea

        self.generatePlotsInRegion(BrazilWater, BrazilWidth, BrazilHeight, BrazilWestX, BrazilSouthY, GatherGrain, GatherGrain, self.iRoundFlags, self.iTerrainFlags, 5, 5, True, 5, -1, False, False)

        self.generatePlotsInRegion(BrazilWater, BrazilWidth, BrazilHeight, BrazilWestX, BrazilSouthY, GatherGrain, GatherGrain, self.iHorzFlags, self.iTerrainFlags, 5, 5, True, 5, -1, False, False)

        # Simulate the Western Hemisphere - Andes.
        NiTextOut("Generating South America (Python Earth2) ...")
        # Set dimensions of Andes
        AndesWestX = int(self.iW * AndesWestLon)
        AndesEastX = int(self.iW * AndesEastLon)
        AndesNorthY = int(self.iH * AndesNorthLat)
        AndesSouthY = int(self.iH * AndesSouthLat)
        AndesWidth = AndesEastX - AndesWestX + 1
        AndesHeight = AndesNorthY - AndesSouthY + 1

        AndesWater = 35+sea

        self.generatePlotsInRegion(AndesWater, AndesWidth, AndesHeight, AndesWestX, AndesSouthY, GatherGrain, GatherGrain, self.iVertFlags, self.iTerrainFlags, 5, 5, True, 5, -1, False, False)

        # Simulate the Eastern Hemisphere - British Isles.
        NiTextOut("Generating Europe (Python Earth2) ...")
        # Set dimensions of British Isles
        EmeraldWestX = int(self.iW * EmeraldWestLon)
        EmeraldEastX = int(self.iW * EmeraldEastLon)
        EmeraldNorthY = int(self.iH * EmeraldNorthLat)
        EmeraldSouthY = int(self.iH * EmeraldSouthLat)
        EmeraldWidth = EmeraldEastX - EmeraldWestX + 1
        EmeraldHeight = EmeraldNorthY - EmeraldSouthY + 1

        EmeraldWater = 80+sea

        self.generatePlotsInRegion(EmeraldWater, EmeraldWidth, EmeraldHeight, EmeraldWestX, EmeraldSouthY, BalanceGrain, ScatterGrain, self.iRoundFlags, self.iTerrainFlags, 5, 5, True, 5, -1, False, False)

        self.generatePlotsInRegion(EmeraldWater, EmeraldWidth, EmeraldHeight, EmeraldWestX, EmeraldSouthY, BalanceGrain, ScatterGrain, self.iVertFlags, self.iTerrainFlags, 5, 5, True, 5, 1, False, False)

        # Simulate the Eastern Hemisphere - Northern Europe.
        NiTextOut("Generating Europe (Python Earth2) ...")
        # Set dimensions of Northern Europe
        NEuropeWestX = int(self.iW * NEuropeWestLon)
        NEuropeEastX = int(self.iW * NEuropeEastLon)
        NEuropeNorthY = int(self.iH * NEuropeNorthLat)
        NEuropeSouthY = int(self.iH * NEuropeSouthLat)
        NEuropeWidth = NEuropeEastX - NEuropeWestX + 1
        NEuropeHeight = NEuropeNorthY - NEuropeSouthY + 1

        NEuropeWater = 50+sea

        self.generatePlotsInRegion(NEuropeWater, NEuropeWidth, NEuropeHeight, NEuropeWestX, NEuropeSouthY, BalanceGrain, GatherGrain, self.iRoundFlags, self.iTerrainFlags, 5, 5, True, 5, -1, False, False)

        # Simulate the Eastern Hemisphere - Central Europe.
        NiTextOut("Generating Europe (Python Earth2) ...")
        # Set dimensions of Central Europe
        CEuropeWestX = int(self.iW * CEuropeWestLon)
        CEuropeEastX = int(self.iW * CEuropeEastLon)
        CEuropeNorthY = int(self.iH * CEuropeNorthLat)
        CEuropeSouthY = int(self.iH * CEuropeSouthLat)
        CEuropeWidth = CEuropeEastX - CEuropeWestX + 1
        CEuropeHeight = CEuropeNorthY - CEuropeSouthY + 1

        CEuropeWater = 35+sea

        self.generatePlotsInRegion(CEuropeWater, CEuropeWidth, CEuropeHeight, CEuropeWestX, CEuropeSouthY, GatherGrain, GatherGrain, self.iRoundFlags, self.iTerrainFlags, 5, 5, True, 5, -1, False, False)

        self.generatePlotsInRegion(CEuropeWater, CEuropeWidth, CEuropeHeight, CEuropeWestX, CEuropeSouthY, GatherGrain, GatherGrain, self.iRoundFlags, self.iTerrainFlags, 5, 5, True, 5, -1, False, False)

        # Simulate the Eastern Hemisphere - Iberia.
        NiTextOut("Generating Europe (Python Earth2) ...")
        # Set dimensions of Iberia
        IberiaWestX = int(self.iW * IberiaWestLon)
        IberiaEastX = int(self.iW * IberiaEastLon)
        IberiaNorthY = int(self.iH * IberiaNorthLat)
        IberiaSouthY = int(self.iH * IberiaSouthLat)
        IberiaWidth = IberiaEastX - IberiaWestX + 1
        IberiaHeight = IberiaNorthY - IberiaSouthY + 1

        IberiaWater = 20+sea

        self.generatePlotsInRegion(IberiaWater, IberiaWidth, IberiaHeight, IberiaWestX, IberiaSouthY, GatherGrain, BalanceGrain, self.iRoundFlags, self.iTerrainFlags, 5, 5, True, 5, -1, False, False)

        # Simulate the Eastern Hemisphere - Mediterranean.
        NiTextOut("Generating Europe (Python Earth2) ...")
        # Set dimensions of Mediterranean
        MediWestX = int(self.iW * MediWestLon)
        MediEastX = int(self.iW * MediEastLon)
        MediNorthY = int(self.iH * MediNorthLat)
        MediSouthY = int(self.iH * MediSouthLat)
        MediWidth = MediEastX - MediWestX + 1
        MediHeight = MediNorthY - MediSouthY + 1

        MediWater = 80+sea

        self.generatePlotsInRegion(MediWater, MediWidth, MediHeight, MediWestX, MediSouthY, ScatterGrain, ScatterGrain, self.iRoundFlags, self.iTerrainFlags, 5, 5, True, 5, -1, False, False)

        # Simulate the Eastern Hemisphere - North Africa.
        NiTextOut("Generating Africa (Python Earth2) ...")
        # Set dimensions of North Africa
        AfricaWestX = int(self.iW * AfricaWestLon)
        AfricaEastX = int(self.iW * AfricaEastLon)
        AfricaNorthY = int(self.iH * AfricaNorthLat)
        AfricaSouthY = int(self.iH * AfricaSouthLat)
        AfricaWidth = AfricaEastX - AfricaWestX + 1
        AfricaHeight = AfricaNorthY - AfricaSouthY + 1

        AfricaWater = 50+sea

        self.generatePlotsInRegion(AfricaWater, AfricaWidth, AfricaHeight, AfricaWestX, AfricaSouthY, GatherGrain, ScatterGrain, self.iRoundFlags, self.iTerrainFlags, 5, 5, True, 5, -1, False, False)

        self.generatePlotsInRegion(AfricaWater, AfricaWidth, AfricaHeight, AfricaWestX, AfricaSouthY, GatherGrain, ScatterGrain, self.iRoundFlags, self.iTerrainFlags, 5, 5, True, 5, -1, False, False)

        self.generatePlotsInRegion(AfricaWater, AfricaWidth, AfricaHeight, AfricaWestX, AfricaSouthY, GatherGrain, ScatterGrain, self.iVertFlags, self.iTerrainFlags, 5, 5, True, 5, -1, False, False)

        # Simulate the Eastern Hemisphere - Numidia.
        NiTextOut("Generating Africa (Python Earth2) ...")
        # Set dimensions of Numidia
        NumidiaWestX = int(self.iW * NumidiaWestLon)
        NumidiaEastX = int(self.iW * NumidiaEastLon)
        NumidiaNorthY = int(self.iH * NumidiaNorthLat)
        NumidiaSouthY = int(self.iH * NumidiaSouthLat)
        NumidiaWidth = NumidiaEastX - NumidiaWestX + 1
        NumidiaHeight = NumidiaNorthY - NumidiaSouthY + 1

        NumidiaWater = 20+sea

        self.generatePlotsInRegion(NumidiaWater, NumidiaWidth, NumidiaHeight, NumidiaWestX, NumidiaSouthY, GatherGrain, GatherGrain, self.iRoundFlags, self.iTerrainFlags, 5, 5, True, 5, -1, False, False)

        # Simulate the Eastern Hemisphere - Central Africa.
        NiTextOut("Generating Africa (Python Earth2) ...")
        # Set dimensions of Central Africa
        CAfricaWestX = int(self.iW * CAfricaWestLon)
        CAfricaEastX = int(self.iW * CAfricaEastLon)
        CAfricaNorthY = int(self.iH * CAfricaNorthLat)
        CAfricaSouthY = int(self.iH * CAfricaSouthLat)
        CAfricaWidth = CAfricaEastX - CAfricaWestX + 1
        CAfricaHeight = CAfricaNorthY - CAfricaSouthY + 1

        CAfricaWater = 35+sea

        self.generatePlotsInRegion(CAfricaWater, CAfricaWidth, CAfricaHeight, CAfricaWestX, CAfricaSouthY, GatherGrain, BalanceGrain, self.iRoundFlags, self.iTerrainFlags, 5, 5, True, 5, -1, False, False)

        self.generatePlotsInRegion(CAfricaWater, CAfricaWidth, CAfricaHeight, CAfricaWestX, CAfricaSouthY, GatherGrain, BalanceGrain, self.iRoundFlags, self.iTerrainFlags, 5, 5, True, 5, -1, False, False)

        # Simulate the Eastern Hemisphere - South Africa.
        NiTextOut("Generating Africa (Python Earth2) ...")
        # Set dimensions of South Africa
        SAfricaWestX = int(self.iW * SAfricaWestLon)
        SAfricaEastX = int(self.iW * SAfricaEastLon)
        SAfricaNorthY = int(self.iH * SAfricaNorthLat)
        SAfricaSouthY = int(self.iH * SAfricaSouthLat)
        SAfricaWidth = SAfricaEastX - SAfricaWestX + 1
        SAfricaHeight = SAfricaNorthY - SAfricaSouthY + 1

        SAfricaWater = 45+sea

        self.generatePlotsInRegion(SAfricaWater, SAfricaWidth, SAfricaHeight, SAfricaWestX, SAfricaSouthY, GatherGrain, BalanceGrain, self.iRoundFlags, self.iTerrainFlags, 5, 5, True, 5, -1, False, False)

        self.generatePlotsInRegion(SAfricaWater, SAfricaWidth, SAfricaHeight, SAfricaWestX, SAfricaSouthY, GatherGrain, BalanceGrain, self.iRoundFlags, self.iTerrainFlags, 5, 5, True, 5, -1, False, False)

        # Simulate the Eastern Hemisphere - Siberia.
        NiTextOut("Generating Asia (Python Earth2) ...")
        # Set dimensions of Siberia
        SiberiaWestX = int(self.iW * SiberiaWestLon)
        SiberiaEastX = int(self.iW * SiberiaEastLon)
        SiberiaNorthY = int(self.iH * SiberiaNorthLat)
        SiberiaSouthY = int(self.iH * SiberiaSouthLat)
        SiberiaWidth = SiberiaEastX - SiberiaWestX + 1
        SiberiaHeight = SiberiaNorthY - SiberiaSouthY + 1

        SiberiaWater = 25+sea

        self.generatePlotsInRegion(SiberiaWater, SiberiaWidth, SiberiaHeight, SiberiaWestX, SiberiaSouthY, GatherGrain, BalanceGrain, self.iRoundFlags, self.iTerrainFlags, 5, 5, True, 5, -1, False, False)

        # Simulate the Eastern Hemisphere - Steppe.
        NiTextOut("Generating Asia (Python Earth2) ...")
        # Set dimensions of Steppe
        SteppeWestX = int(self.iW * SteppeWestLon)
        SteppeEastX = int(self.iW * SteppeEastLon)
        SteppeNorthY = int(self.iH * SteppeNorthLat)
        SteppeSouthY = int(self.iH * SteppeSouthLat)
        SteppeWidth = SteppeEastX - SteppeWestX + 1
        SteppeHeight = SteppeNorthY - SteppeSouthY + 1

        SteppeWater = 6+sea

        self.generatePlotsInRegion(SteppeWater, SteppeWidth, SteppeHeight, SteppeWestX, SteppeSouthY, GatherGrain, GatherGrain, self.iRoundFlags, self.iTerrainFlags, 5, 5, True, 5, -1, False, False)

        # Simulate the Eastern Hemisphere - Near East.
        NiTextOut("Generating Asia (Python Earth2) ...")
        # Set dimensions of Near East
        NearEastWestX = int(self.iW * NearEastWestLon)
        NearEastEastX = int(self.iW * NearEastEastLon)
        NearEastNorthY = int(self.iH * NearEastNorthLat)
        NearEastSouthY = int(self.iH * NearEastSouthLat)
        NearEastWidth = NearEastEastX - NearEastWestX + 1
        NearEastHeight = NearEastNorthY - NearEastSouthY + 1

        NearEastWater = 50+sea

        self.generatePlotsInRegion(NearEastWater, NearEastWidth, NearEastHeight, NearEastWestX, NearEastSouthY, GatherGrain, BalanceGrain, self.iRoundFlags, self.iTerrainFlags, 5, 5, True, 5, -1, False, False)

        self.generatePlotsInRegion(NearEastWater, NearEastWidth, NearEastHeight, NearEastWestX, NearEastSouthY, GatherGrain, BalanceGrain, self.iRoundFlags, self.iTerrainFlags, 5, 5, True, 5, -1, False, False)

        # Simulate the Eastern Hemisphere - Arabia.
        NiTextOut("Generating Asia (Python Earth2) ...")
        # Set dimensions of Arabia
        ArabiaWestX = int(self.iW * ArabiaWestLon)
        ArabiaEastX = int(self.iW * ArabiaEastLon)
        ArabiaNorthY = int(self.iH * ArabiaNorthLat)
        ArabiaSouthY = int(self.iH * ArabiaSouthLat)
        ArabiaWidth = ArabiaEastX - ArabiaWestX + 1
        ArabiaHeight = ArabiaNorthY - ArabiaSouthY + 1

        ArabiaWater = 50+sea

        self.generatePlotsInRegion(ArabiaWater, ArabiaWidth, ArabiaHeight, ArabiaWestX, ArabiaSouthY, GatherGrain, BalanceGrain, self.iVertFlags, self.iTerrainFlags, 5, 5, True, 5, -1, False, False)

        # Simulate the Eastern Hemisphere - India.
        NiTextOut("Generating Asia (Python Earth2) ...")
        # Set dimensions of India
        IndiaWestX = int(self.iW * IndiaWestLon)
        IndiaEastX = int(self.iW * IndiaEastLon)
        IndiaNorthY = int(self.iH * IndiaNorthLat)
        IndiaSouthY = int(self.iH * IndiaSouthLat)
        IndiaWidth = IndiaEastX - IndiaWestX + 1
        IndiaHeight = IndiaNorthY - IndiaSouthY + 1

        IndiaWater = 33+sea

        self.generatePlotsInRegion(IndiaWater, IndiaWidth, IndiaHeight, IndiaWestX, IndiaSouthY, GatherGrain, BalanceGrain, self.iRoundFlags, self.iTerrainFlags, 5, 5, True, 5, -1, False, False)

        self.generatePlotsInRegion(IndiaWater, IndiaWidth, IndiaHeight, IndiaWestX, IndiaSouthY, GatherGrain, BalanceGrain, self.iRoundFlags, self.iTerrainFlags, 5, 5, True, 5, -1, False, False)

        # Simulate the Eastern Hemisphere - China.
        NiTextOut("Generating Asia (Python Earth2) ...")
        # Set dimensions of China
        ChinaWestX = int(self.iW * ChinaWestLon)
        ChinaEastX = int(self.iW * ChinaEastLon)
        ChinaNorthY = int(self.iH * ChinaNorthLat)
        ChinaSouthY = int(self.iH * ChinaSouthLat)
        ChinaWidth = ChinaEastX - ChinaWestX + 1
        ChinaHeight = ChinaNorthY - ChinaSouthY + 1

        ChinaWater = 65+sea

        self.generatePlotsInRegion(ChinaWater, ChinaWidth, ChinaHeight, ChinaWestX, ChinaSouthY, GatherGrain, BalanceGrain, self.iRoundFlags, self.iTerrainFlags, 5, 5, True, 5, -1, False, False)

        self.generatePlotsInRegion(ChinaWater, ChinaWidth, ChinaHeight, ChinaWestX, ChinaSouthY, GatherGrain, BalanceGrain, self.iHorzFlags, self.iTerrainFlags, 5, 5, True, 5, -1, False, False)

        self.generatePlotsInRegion(ChinaWater, ChinaWidth, ChinaHeight, ChinaWestX, ChinaSouthY, GatherGrain, BalanceGrain, self.iHorzFlags, self.iTerrainFlags, 5, 5, True, 5, -1, False, False)

        # Simulate the Eastern Hemisphere - IndoChina.
        NiTextOut("Generating Asia (Python Earth2) ...")
        # Set dimensions of IndoChina
        IndoChinaWestX = int(self.iW * IndoChinaWestLon)
        IndoChinaEastX = int(self.iW * IndoChinaEastLon)
        IndoChinaNorthY = int(self.iH * IndoChinaNorthLat)
        IndoChinaSouthY = int(self.iH * IndoChinaSouthLat)
        IndoChinaWidth = IndoChinaEastX - IndoChinaWestX + 1
        IndoChinaHeight = IndoChinaNorthY - IndoChinaSouthY + 1

        IndoChinaWater = 82+sea

        self.generatePlotsInRegion(IndoChinaWater, IndoChinaWidth, IndoChinaHeight, IndoChinaWestX, IndoChinaSouthY, ScatterGrain, BalanceGrain, self.iRoundFlags, self.iTerrainFlags, 5, 5, True, 5, -1, False, False)

        self.generatePlotsInRegion(IndoChinaWater, IndoChinaWidth, IndoChinaHeight, IndoChinaWestX, IndoChinaSouthY, ScatterGrain, BalanceGrain, self.iRoundFlags, self.iTerrainFlags, 5, 5, True, 5, -1, False, False)

        # Simulate the Eastern Hemisphere - Japan.
        NiTextOut("Generating Asia (Python Earth2) ...")
        # Set dimensions of Japan
        JapanWestX = int(self.iW * JapanWestLon)
        JapanEastX = int(self.iW * JapanEastLon)
        JapanNorthY = int(self.iH * JapanNorthLat)
        JapanSouthY = int(self.iH * JapanSouthLat)
        JapanWidth = JapanEastX - JapanWestX + 1
        JapanHeight = JapanNorthY - JapanSouthY + 1

        JapanWater = 92+sea

        self.generatePlotsInRegion(JapanWater, JapanWidth, JapanHeight, JapanWestX, JapanSouthY, BalanceGrain, BalanceGrain, self.iRoundFlags, self.iTerrainFlags, 5, 5, True, 5, -1, False, False)

        self.generatePlotsInRegion(JapanWater, JapanWidth, JapanHeight, JapanWestX, JapanSouthY, BalanceGrain, BalanceGrain, self.iRoundFlags, self.iTerrainFlags, 5, 5, True, 5, -1, False, False)

        # Simulate the Eastern Hemisphere - Australia.
        NiTextOut("Generating Australia (Python Earth2) ...")
        # Set dimensions of Australia
        AustraliaWestX = int(self.iW * AustraliaWestLon)
        AustraliaEastX = int(self.iW * AustraliaEastLon)
        AustraliaNorthY = int(self.iH * AustraliaNorthLat)
        AustraliaSouthY = int(self.iH * AustraliaSouthLat)
        AustraliaWidth = AustraliaEastX - AustraliaWestX + 1
        AustraliaHeight = AustraliaNorthY - AustraliaSouthY + 1

        AustraliaWater = 45+sea

        self.generatePlotsInRegion(AustraliaWater, AustraliaWidth, AustraliaHeight, AustraliaWestX, AustraliaSouthY, GatherGrain, BalanceGrain, self.iRoundFlags, self.iTerrainFlags, 5, 5, True, 5, -1, False, False)

        self.generatePlotsInRegion(AustraliaWater, AustraliaWidth, AustraliaHeight, AustraliaWestX, AustraliaSouthY, GatherGrain, BalanceGrain, self.iRoundFlags, self.iTerrainFlags, 5, 5, True, 5, -1, False, False)

        # Simulate the South Pacific - South Pacific.
        NiTextOut("Generating Pacific (Python Earth2) ...")
        # Set dimensions of South Pacific
        SouthPacificWestX = int(self.iW * SouthPacificWestLon)
        SouthPacificEastX = int(self.iW * SouthPacificEastLon)
        SouthPacificNorthY = int(self.iH * SouthPacificNorthLat)
        SouthPacificSouthY = int(self.iH * SouthPacificSouthLat)
        SouthPacificWidth = SouthPacificEastX - SouthPacificWestX + 1
        SouthPacificHeight = SouthPacificNorthY - SouthPacificSouthY + 1

        SouthPacificWater = 94+sea

        self.generatePlotsInRegion(SouthPacificWater, SouthPacificWidth, SouthPacificHeight, SouthPacificWestX, SouthPacificSouthY, ScatterGrain, ScatterGrain, self.iRoundFlags, self.iTerrainFlags, 5, 5, True, 5, -1, False, False)

        # All regions have been processed. Plot Type generation completed.
        return self.wholeworldPlotTypes

'''
Regional Variables Key:

iWaterPercent,
iRegionWidth, iRegionHeight,
iRegionWestX, iRegionSouthY,
iRegionGrain, iRegionHillsGrain,
iRegionPlotFlags, iRegionTerrainFlags,
iRegionFracXExp, iRegionFracYExp,
bShift, iStrip,
rift_grain, has_center_rift,
invert_heights
'''

def generatePlotTypes():
    NiTextOut("Setting Plot Types (Python Earth2) ...")
    # Call generatePlotsByRegion() function, from TerraMultilayeredFractal subclass.
    global plotgen
    plotgen = EarthMultilayeredFractal()
    return plotgen.generatePlotsByRegion()

class Earth2TerrainGenerator(CvMapGeneratorUtil.TerrainGenerator):
	def __init__(self, iDesertPercent=40, iPlainsPercent=26, fSnowLatitude=0.82, fTundraLatitude=0.75, fGrassLatitude=0.1, fDesertBottomLatitude=0.1, fDesertTopLatitude=0.3, fracXExp=-1, fracYExp=-1, grain_amount=3):
		self.gc = CyGlobalContext()
		self.map = CyMap()

		grain_amount += self.gc.getWorldInfo(self.map.getWorldSize()).getTerrainGrainChange()

		self.grain_amount = grain_amount

		self.iWidth = self.map.getGridWidth()
		self.iHeight = self.map.getGridHeight()

		self.mapRand = self.gc.getGame().getMapRand()

		self.iFlags = 0  # Disallow FRAC_POLAR flag, to prevent "zero row" problems.
		if self.map.isWrapX():
			self.iFlags += CyFractal.FracVals.FRAC_WRAP_X
		if self.map.isWrapY():
			self.iFlags += CyFractal.FracVals.FRAC_WRAP_Y

		self.deserts=CyFractal()
		self.plains=CyFractal()
		self.variation=CyFractal()

		iDesertPercent += self.gc.getClimateInfo(self.map.getClimate()).getDesertPercentChange()
		iDesertPercent = min(iDesertPercent, 100)
		iDesertPercent = max(iDesertPercent, 0)

		self.iDesertPercent = iDesertPercent
		self.iPlainsPercent = iPlainsPercent

		self.iDesertTopPercent = 100
		self.iDesertBottomPercent = max(0,int(100-iDesertPercent))
		self.iPlainsTopPercent = 100
		self.iPlainsBottomPercent = max(0,int(100-iDesertPercent-iPlainsPercent))
		self.iMountainTopPercent = 75
		self.iMountainBottomPercent = 60

		fSnowLatitude += self.gc.getClimateInfo(self.map.getClimate()).getSnowLatitudeChange()
		fSnowLatitude = min(fSnowLatitude, 1.0)
		fSnowLatitude = max(fSnowLatitude, 0.0)
		self.fSnowLatitude = fSnowLatitude

		fTundraLatitude += self.gc.getClimateInfo(self.map.getClimate()).getTundraLatitudeChange()
		fTundraLatitude = min(fTundraLatitude, 1.0)
		fTundraLatitude = max(fTundraLatitude, 0.0)
		self.fTundraLatitude = fTundraLatitude

		fGrassLatitude += self.gc.getClimateInfo(self.map.getClimate()).getGrassLatitudeChange()
		fGrassLatitude = min(fGrassLatitude, 1.0)
		fGrassLatitude = max(fGrassLatitude, 0.0)
		self.fGrassLatitude = fGrassLatitude

		fDesertBottomLatitude += self.gc.getClimateInfo(self.map.getClimate()).getDesertBottomLatitudeChange()
		fDesertBottomLatitude = min(fDesertBottomLatitude, 1.0)
		fDesertBottomLatitude = max(fDesertBottomLatitude, 0.0)
		self.fDesertBottomLatitude = fDesertBottomLatitude

		fDesertTopLatitude += self.gc.getClimateInfo(self.map.getClimate()).getDesertTopLatitudeChange()
		fDesertTopLatitude = min(fDesertTopLatitude, 1.0)
		fDesertTopLatitude = max(fDesertTopLatitude, 0.0)
		self.fDesertTopLatitude = fDesertTopLatitude

		self.fracXExp = fracXExp
		self.fracYExp = fracYExp

		self.initFractals()

	def initFractals(self):
		self.deserts.fracInit(self.iWidth, self.iHeight, self.grain_amount, self.mapRand, self.iFlags, self.fracXExp, self.fracYExp)
		self.iDesertTop = self.deserts.getHeightFromPercent(self.iDesertTopPercent)
		self.iDesertBottom = self.deserts.getHeightFromPercent(self.iDesertBottomPercent)

		self.plains.fracInit(self.iWidth, self.iHeight, self.grain_amount+1, self.mapRand, self.iFlags, self.fracXExp, self.fracYExp)
		self.iPlainsTop = self.plains.getHeightFromPercent(self.iPlainsTopPercent)
		self.iPlainsBottom = self.plains.getHeightFromPercent(self.iPlainsBottomPercent)

		self.variation.fracInit(self.iWidth, self.iHeight, self.grain_amount, self.mapRand, self.iFlags, self.fracXExp, self.fracYExp)

		self.terrainDesert = getInfoTypeOrFail("TERRAIN_DESERT")
		self.terrainPlains = getInfoTypeOrFail("TERRAIN_PLAINS")
		self.terrainIce = getInfoTypeOrFail("TERRAIN_SNOW")
		self.terrainTundra = getInfoTypeOrFail("TERRAIN_TUNDRA")
		self.terrainGrass = getInfoTypeOrFail("TERRAIN_GRASS")

	def getLatitudeAtPlot(self, iX, iY):
		"""given a point (iX,iY) such that (0,0) is in the NW,
		returns a value between 0.0 (tropical) and 1.0 (polar).
		This function can be overridden to change the latitudes; for example,
		to make an entire map have temperate terrain, or to make terrain change from east to west
		instead of from north to south"""
		lat = abs((self.iHeight / 2) - iY)/float(self.iHeight/2) # 0.0 = equator, 1.0 = pole

		# Adjust latitude using self.variation fractal, to mix things up:
		lat += (128 - self.variation.getHeight(iX, iY))/(255.0 * 5.0)

		# Limit to the range [0, 1]:
		if lat < 0:
			lat = 0.0
		if lat > 1:
			lat = 1.0

		return lat

	def generateTerrain(self):		
		terrainData = [0]*(self.iWidth*self.iHeight)
		for x in range(self.iWidth):
			for y in range(self.iHeight):
				iI = y*self.iWidth + x
				terrain = self.generateTerrainAtPlot(x, y)
				terrainData[iI] = terrain
		return terrainData

	def generateTerrainAtPlot(self,iX,iY):
		lat = self.getLatitudeAtPlot(iX,iY)

		if (self.map.plot(iX, iY).isWater()):
			return self.map.plot(iX, iY).getTerrainType()

		terrainVal = self.terrainGrass

		if lat >= self.fSnowLatitude:
			terrainVal = self.terrainIce
		elif lat >= self.fTundraLatitude:
			terrainVal = self.terrainTundra
		elif lat < self.fGrassLatitude:
			terrainVal = self.terrainGrass
		else:
			desertVal = self.deserts.getHeight(iX, iY)
			plainsVal = self.plains.getHeight(iX, iY)
			if ((desertVal >= self.iDesertBottom) and (desertVal <= self.iDesertTop) and (lat >= self.fDesertBottomLatitude) and (lat < self.fDesertTopLatitude)):
				terrainVal = self.terrainDesert
			elif ((plainsVal >= self.iPlainsBottom) and (plainsVal <= self.iPlainsTop)):
				terrainVal = self.terrainPlains

		if (terrainVal == TerrainTypes.NO_TERRAIN):
			return self.map.plot(iX, iY).getTerrainType()

		return terrainVal

def generateTerrainTypes():
	NiTextOut("Generating Terrain (Python Terra) ...")
	terraingen = Earth2TerrainGenerator()
	terrainTypes = terraingen.generateTerrain()
	return terrainTypes

def addFeatures():
	NiTextOut("Adding Features (Python Earth2) ...")
	featuregen = FeatureGenerator()
	featuregen.addFeatures()
	return 0
