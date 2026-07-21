## BugMapOptionsTab
##
## Tab for the BUG Map Options.
##
## Copyright (c) 2009 The BUG Mod.
##
## Author: EmperorFool

import BugOptionsTab

class BugMapOptionsTab(BugOptionsTab.BugOptionsTab):
	"BUG Nap Options Screen Tab"
	
	def __init__(self, screen):
		BugOptionsTab.BugOptionsTab.__init__(self, "Map", "Map")

	def create(self, screen):
		tab = self.createTab(screen)
		panel = self.createMainPanel(screen)
		column = self.addOneColumnLayout(screen, panel)
		
		left, center, right = self.addThreeColumnLayout(screen, column, "Top", True)
		# advc.004:
		self.addLabel(screen, left, "Layers", "Layers:")
		# <advc.004h>
		self.addCheckbox(screen, left, "MainInterface__FoundingYields")
		self.addTextDropdown(screen, left, left, "MainInterface__FoundingBorder")
		# </advc.004h>  <advc.004z>
		self.addCheckbox(screen, left, "MainInterface__ScoresInGlobeView")
		self.addCheckbox(screen, left, "MainInterface__ResourceIconOptions")
		self.addCheckbox(screen, left, "MainInterface__TribalVillageIcons")
		# </advc.004z>
		# advc.004m:
		self.addCheckbox(screen, left, "MainInterface__StartWithResourceIcons")
		# <advc.092b> Plot indicator size options
		self.addTextDropdown(screen, left, left, "MainInterface__PlotIndicatorSize")
		self.addTextDropdown(screen, left, left, "MainInterface__OffScreenUnitSizeMult")
		# Not enough room here for the Strategy Overlay options anymore. Moved those
		# to the right column. Instead place misc. options here. </advc.092b>
		self.addSpacer(screen, left, "SpacerLeft1")
		self.addLabel(screen, left, "Misc", "Misc:")
		self.addCheckbox(screen, left, "EventSigns__Enabled")
		# advc.savem
		self.addCheckbox(screen, left, "MainInterface__EnableSavemap")

		#self.addLabel(screen, center, "CityBar", "CityBar:")
		#self.addCheckbox(screen, center, "CityBar__StarvationTurns")
		# <advc.011b>
		self.addLabel(screen, center, "TileHover", "Tile Hover:")
		self.addCheckbox(screen, center, "MiscHover__PartialBuildsAlways")
		# </advc.011b>
		# advc.099f: (always enabled as of AdvCiv v1.0)
		#self.addCheckbox(screen, center, "MiscHover__CultureInUnownedTiles")
		# advc.061:
		self.addCheckbox(screen, center, "MainInterface__ListUnitsPerOwner")
		#self.addCheckbox(screen, center, "MiscHover__LatLongCoords")
		#self.addCheckbox(screen, center, "MiscHover__PartialBuilds")
		self.addCheckbox(screen, center, "MainInterface__RevoltHelp") # advc.101
		self.addCheckbox(screen, center, "MainInterface__AirCapacity") # advc.187
		self.addSpacer(screen, center, "SpacerCenter1") # advc.004
		# <advc.002f>
		self.addLabel(screen, center, "CityIcons", "City Billboards:")
		centerL, centerR = self.addTwoColumnLayout(screen, center, "CityIcons", False)
		# advc.095:
		self.addCheckbox(screen, centerL, "MainInterface__WideCityBars")
		self.addCheckbox(screen, centerR, "MainInterface__AvoidGrowthIcon")
		self.addCheckbox(screen, centerL, "MainInterface__CityNetworkIcon")
		self.addCheckbox(screen, centerR, "MainInterface__AirportIcon")
		self.addCheckbox(screen, centerL, "MainInterface__TopCityIcons")
		#self.addCheckbox(screen, centerL, "MainInterface__TopProductionIcon")
		#self.addCheckbox(screen, centerR, "MainInterface__TopGoldIcon")
		#self.addCheckbox(screen, centerL, "MainInterface__TopResearchIcon")
		#self.addCheckbox(screen, centerR, "MainInterface__TopEspionageIcon")
		#self.addCheckbox(screen, centerL, "MainInterface__TopXPIcon")
		self.addCheckbox(screen, centerR, "MainInterface__NextGPIcon")
		#self.addCheckbox(screen, centerL, "MainInterface__NoUnhappyIcon")
		#self.addCheckbox(screen, centerR, "MainInterface__NoBadHealthIcon")
		# </advc.002f>
		self.addCheckbox(screen, centerL, "MainInterface__RevoltChanceIcon") # advc.101
		# <advc.186>
		# Moved from City Screen tab. But let's disable it entirely.
		#self.addCheckbox(screen, centerR, "CityBar__SelectionHelp")
		self.addSpacer(screen, centerR, "SelectionHelp")
		self.addTextDropdown(screen, centerL, centerR, "CityBar__BuildingDisplay")
		# </advc.186>
		self.addLabel(screen, right, "Camera", "Camera:") # advc.004m
		self.addCheckbox(screen, right, "MainInterface__FieldOfView")
		self.addCheckbox(screen, right, "MainInterface__FieldOfView_Remember", True)
		# advc.004m:
		self.addTextDropdown(screen, right, right, "MainInterface__DefaultCamDistance")
		# advc.102b:
		self.addTextDropdown(screen, right, right, "MainInterface__ShowFriendlyMovesThresh")
		# <advc.002a>
		# advc.102b: No room for this anymore
		#self.addSpacer(screen, right, "SpacerRight1")
		self.addLabel(screen, right, "Minimap", "Minimap:")
		self.addTextDropdown(screen, right, right, "MainInterface__MinimapWaterColor")
		self.addCheckbox(screen, right, "MainInterface__UnitsOnMinimap")
		self.addSpacer(screen, right, "SpacerRight2")
		# </advc.002a>
		# advc.092b: Moved from the left column, and made more compact.
		self.addLabel(screen, right, "StrategyOverlay", "Dot Map Overlay:")
		rightL, rightR = self.addTwoColumnLayout(screen, right, "DotMapBrightness")
		self.addCheckbox(screen, rightL, "StrategyOverlay__Enabled")
		# advc.004: Disabled; Ctrl+X should suffice.
		#self.addCheckbox(screen, right, "StrategyOverlay__ShowDotMap")
		self.addCheckbox(screen, rightR, "StrategyOverlay__DotMapDrawDots")
		#self.addTextEdit(screen, rightL, rightR, "StrategyOverlay__DotMapDotIcon")
		self.addSlider(screen, rightL, rightR, "StrategyOverlay__DotMapBrightness", False, False, False, "up", 0, 100)
		self.addSlider(screen, rightL, rightR, "StrategyOverlay__DotMapHighlightBrightness", False, False, False, "up", 0, 100)
		
		#self.addCheckbox(screen, right, "Actions__IgnoreHarmlessBarbarians")
		
		# advc.009c: Commented out
		#screen.attachHSeparator(column, column + "Sep1")
		#left, right = self.addTwoColumnLayout(screen, column, "MapFinderEnabled", True)
		#self.addLabel(screen, left, "MapFinder", "MapFinder:")
		#self.addCheckbox(screen, right, "MapFinder__Enabled")
		
		#self.addTextEdit(screen, column, column, "MapFinder__Path")
		#self.addTextEdit(screen, column, column, "MapFinder__SavePath")
		
		#left, right = self.addTwoColumnLayout(screen, column, "MapFinder", True)
		#leftL, leftR = self.addTwoColumnLayout(screen, left, "MapFinderDelays")
		#self.addFloatDropdown(screen, leftL, leftR, "MapFinder__RegenerationDelay")
		#self.addFloatDropdown(screen, leftL, leftR, "MapFinder__SkipDelay")
		#self.addFloatDropdown(screen, leftL, leftR, "MapFinder__SaveDelay")
		
		#rightL, rightR = self.addTwoColumnLayout(screen, right, "MapFinderLimits")
		#self.addTextEdit(screen, rightL, rightR, "MapFinder__RuleFile")
		#self.addTextEdit(screen, rightL, rightR, "MapFinder__RegenerationLimit")
		#self.addTextEdit(screen, rightL, rightR, "MapFinder__SaveLimit")
