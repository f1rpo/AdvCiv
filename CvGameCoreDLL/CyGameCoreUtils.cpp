#include "CvGameCoreDLL.h"
#include "CyGameCoreUtils.h"
#include "CvGameCoreUtils.h"
/*  <advc.make> (would be nicer to move the respective functions to the
	Python interface of CvMap) */
#include "CvMap.h"
#include "CvCity.h" // </advc.make>
// <advc> For functions moved from CvGameCoreUtils to CvInfos
#include "CvInfo_Unit.h"
#include "CvInfo_Building.h" // </advc>

int cyIntRange(int iNum, int iLow, int iHigh)
{
	return range(iNum, iLow, iHigh);
}

float cyFloatRange(float fNum, float fLow, float fHigh)
{
	return range(fNum, fLow, fHigh);
}

int cyDxWrap(int iDX)
{
	return GC.getMap().dxWrap(iDX);
}

int cyDyWrap(int iDY)
{
	return GC.getMap().dyWrap(iDY);
}

int cyPlotDistance(int iX, int iY, int iX2, int iY2)
{
	return plotDistance(iX, iY, iX2, iY2);
}

int cyStepDistance(int iX1, int iY1, int iX2, int iY2)
{
	return stepDistance(iX1, iY1, iX2, iY2);
}

CyPlot* cyPlotDirection(int iX, int iY, DirectionTypes eDirection)
{
	return new CyPlot(plotDirection(iX, iY, eDirection));
}

CyPlot* cyPlotCardinalDirection(int iX, int iY, CardinalDirectionTypes eCardDirection)
{
	return new CyPlot(plotCardinalDirection(iX, iY, eCardDirection));
}

CyPlot* cysPlotCardinalDirection(int iX, int iY, CardinalDirectionTypes eCardDirection)
{
	static CyPlot plot;
	plot.setPlot(plotCardinalDirection(iX, iY, eCardDirection));
	return &plot;
}

CyPlot* cyPlotXY(int iX, int iY, int iDX, int iDY)
{
	return new CyPlot(plotXY(iX, iY, iDX, iDY));
}

CyPlot* cysPlotXY(int iX, int iY, int iDX, int iDY)
{
	static CyPlot plot;
	plot.setPlot(plotXY(iX, iY, iDX, iDY));
	return &plot;
}

DirectionTypes cyDirectionXYFromInt(int iDX, int iDY)
{
	return directionXY(iDX, iDY);
}

DirectionTypes cyDirectionXYFromPlot(CyPlot* pFromPlot, CyPlot* pToPlot)
{
	return directionXY(pFromPlot->getPlot(), pToPlot->getPlot());
}

CyPlot* cyPlotCity(int iX, int iY, int iIndex)
{
	return new CyPlot(plotCity(iX, iY, (CityPlotTypes)iIndex));
}

int cyPlotCityXYFromInt(int iDX, int iDY)
{
	return GC.getMap().plotCityXY(iDX, iDY);
}

int cyPlotCityXYFromCity(CyCity* pCity, CyPlot* pPlot)
{
	CvCity const& kCity = *pCity->getCity(); // advc: plotCityXY(CvCity*,CvPlot*) no longer exists
	return plotCityXY(kCity.getX(), kCity.getY(), *pPlot->getPlot());
}

CardinalDirectionTypes cyGetOppositeCardinalDirection(CardinalDirectionTypes eCardDirection)
{
	return getOppositeCardinalDirection(eCardDirection);
}

DirectionTypes cyCardinalDirectionToDirection(CardinalDirectionTypes eCard)
{
	return cardinalDirectionToDirection(eCard);
}

bool cyIsCardinalDirection(DirectionTypes eDirection)
{
	return isCardinalDirection(eDirection);
}

DirectionTypes cyEstimateDirection(int iDX, int iDY)
{
	return estimateDirection(iDX, iDY);
}

bool cyAtWar(int /*TeamTypes*/ eTeamA, int /*TeamTypes*/ eTeamB)
{
	return atWar((TeamTypes)eTeamA, (TeamTypes)eTeamB);
}

bool cyIsPotentialEnemy(int /*TeamTypes*/ eOurTeam, int /*TeamTypes*/ eTheirTeam)
{
	return isPotentialEnemy((TeamTypes)eOurTeam, (TeamTypes)eTheirTeam);
}

CyCity* cyGetCity(IDInfo city)
{
	return new CyCity(getCity(city));
}

CyUnit* cyGetUnit(IDInfo unit)
{
	return new CyUnit(getUnit(unit));
}

bool cyIsPromotionValid(int /*PromotionTypes*/ ePromotion, int /*UnitTypes*/ eUnit, bool bLeader)
{
	return isPromotionValid((PromotionTypes) ePromotion, (UnitTypes) eUnit, bLeader);
}

int cyGetPopulationAsset(int iPopulation)
{
	return getPopulationAsset(iPopulation);
}

int cyGetLandPlotsAsset(int iLandPlots)
{
	return getLandPlotsAsset(iLandPlots);
}

int cyGetPopulationPower(int iPopulation)
{
	return getPopulationPower(iPopulation);
}

int cyGetPopulationScore(int iPopulation)
{
	return getPopulationScore(iPopulation);
}

int cyGetLandPlotsScore(int iPopulation)
{
	return getLandPlotsScore(iPopulation);
}

int cyGetTechScore(int /*TechTypes*/ eTech)
{
	return getTechScore((TechTypes)eTech);
}

int cyGetWonderScore(int /*BuildingClassTypes*/ eWonderClass)
{
	return getWonderScore((BuildingClassTypes)eWonderClass);
}

int /*ImprovementTypes*/ cyFinalImprovementUpgrade(int /*ImprovementTypes*/ eImprovement)
{
	return finalImprovementUpgrade((ImprovementTypes) eImprovement);
}

int cyGetWorldSizeMaxConscript(int /*CivicTypes*/ eCivic)
{
	return getWorldSizeMaxConscript((CivicTypes) eCivic);
}

bool cyIsReligionTech(int /*TechTypes*/ eTech)
{
	return CvReligionInfo::isReligionTech((TechTypes) eTech);
}

bool cyIsTechRequiredForUnit(int /*TechTypes*/ eTech, int /*UnitTypes*/ eUnit)
{
	return GC.getInfo((UnitTypes)eUnit).isTechRequired((TechTypes)eTech);
}

bool cyIsTechRequiredForBuilding(int /*TechTypes*/ eTech, int /*BuildingTypes*/ eBuilding)
{
	return GC.getInfo((BuildingTypes)eBuilding).isTechRequired((TechTypes)eTech);
}

bool cyIsTechRequiredForProject(int /*TechTypes*/ eTech, int /*ProjectTypes*/ eProject)
{
	// advc.003w: Global isTechRequiredForProject function no longer exists
	return (GC.getInfo((ProjectTypes)eProject).getTechPrereq() == eTech);
}

bool cyIsWorldUnitClass(int /*UnitClassTypes*/ eUnitClass)
{
	return GC.getInfo((UnitClassTypes)eUnitClass).isWorldUnit();
}

bool cyIsTeamUnitClass(int /*UnitClassTypes*/ eUnitClass)
{
	return GC.getInfo((UnitClassTypes)eUnitClass).isTeamUnit();
}

bool cyIsNationalUnitClass(int /*UnitClassTypes*/ eUnitClass)
{
	return GC.getInfo((UnitClassTypes)eUnitClass).isNationalUnit();
}

bool cyIsLimitedUnitClass(int /*UnitClassTypes*/ eUnitClass)
{
	return GC.getInfo((UnitClassTypes)eUnitClass).isLimited();
}

bool cyIsWorldWonderClass(int /*BuildingClassTypes*/ eBuildingClass)
{
	return GC.getInfo((BuildingClassTypes)eBuildingClass).isWorldWonder();
}

bool cyIsTeamWonderClass(int /*BuildingClassTypes*/ eBuildingClass)
{
	return GC.getInfo((BuildingClassTypes)eBuildingClass).isTeamWonder();
}

bool cyIsNationalWonderClass(int /*BuildingClassTypes*/ eBuildingClass)
{
	return GC.getInfo((BuildingClassTypes)eBuildingClass).isNationalWonder();
}

bool cyIsLimitedWonderClass(int /*BuildingClassTypes*/ eBuildingClass)
{
	return GC.getInfo((BuildingClassTypes)eBuildingClass).isLimited();
}

bool cyIsWorldProject(int /*ProjectTypes*/ eProject)
{
	return GC.getInfo((ProjectTypes)eProject).isWorldProject();
}

bool cyIsTeamProject(int /*ProjectTypes*/ eProject)
{
	return GC.getInfo((ProjectTypes)eProject).isTeamProject();
}

bool cyIsLimitedProject(int /*ProjectTypes*/ eProject)
{
	return GC.getInfo((ProjectTypes)eProject).isLimited();
}

int cyGetCombatOdds(CyUnit* pAttacker, CyUnit* pDefender)
{
	return getCombatOdds(pAttacker->getUnit(), pDefender->getUnit());
}

int cyGetEspionageModifier(int iOurTeam, int iTargetTeam)
{
	return getEspionageModifier((TeamTypes)iOurTeam, (TeamTypes)iTargetTeam);
}

