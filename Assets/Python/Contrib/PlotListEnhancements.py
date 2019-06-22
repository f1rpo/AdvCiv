## PlotListEnhancements
##
## Utility functions for PLE by EmperorFool

def resetUnitPlotListStackedBarColors(option=None, value=None):
	import CvScreensInterface
	# advc.001, advc.069: The .PLE was missing
	CvScreensInterface.mainInterface.PLE.resetUnitPlotListStackedBarColors()
