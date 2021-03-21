## CityUtil
##
## Collection of utility functions for dealing with cities.
##
## Copyright (c) 2009 The BUG Mod.
##
## Author: EmperorFool

from CvPythonExtensions import *


## Globals

gc = CyGlobalContext()


## Growth and Starvation

def willGrowThisTurn(city):
	"""
	Returns True if <city> will increase its population due to growth this turn.
	
	Emphasize No Growth must be off for the city, and its food rate plus storage must reach the growth threshold.
	"""
	if city.getFood() + city.foodDifference(True) < city.growthThreshold():
		return False
	return not avoidingGrowth(city) # advc: Don't hardcode emphasize id 5

# advc:
def avoidingGrowth(city):
	for i in range(gc.getNumEmphasizeInfos()):
		if gc.getEmphasizeInfo(i).isAvoidGrowth() and city.AI_isEmphasize(i):
			return True
	return False

def willShrinkThisTurn(city):
	"""
	Returns True if <city> will decrease its population due to starvation this turn.
	
	It must have at least two population, and its food rate plus storage must be negative.
	"""
	return city.getPopulation() > 1 and city.getFood() + city.foodDifference(True) < 0
