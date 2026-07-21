# advc.009b: New module for initialization/ reloading of modified BtS screens

# Calling these CvScreensInterface functions directly through BugUtil.lookupFunction doesn't work reliably (race condition) when reloading scripts. I don't understand why; perhaps some circular dependency.
# initBugAdvisors may still fail sometimes.
# BUG - Options - start  (moved from CvScreensInterface.py)
def init(): # called when parsing 'BUG Core.xml'
	import CvScreensInterface
	try:
		CvScreensInterface.initBugAdvisors()
	except AttributeError:
		import BugUtil
		BugUtil.error("Failed to call CvScreensInterface.initBugAdvisors")
# BUG - Options - end

def deleteTechSplash(option=None, value=None): # called when parsing TechWindow.xml
	import CvScreensInterface
	CvScreensInterface.deleteTechSplash(option, value)
