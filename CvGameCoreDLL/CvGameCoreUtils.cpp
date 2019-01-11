#include "CvGameCoreDLL.h"
#include "CvGameCoreUtils.h"
#include <algorithm>
#include "CvUnit.h"
#include "CvPlayerAI.h"
#include "CvMap.h"
#include "CvPlot.h"
#include "CvRandom.h"
#include "FAStarNode.h"
#include "CvCity.h"
#include "CvTeamAI.h"
#include "CvInfos.h"
#include "CvGlobals.h"
#include "FProfiler.h"

#include "CvDLLInterfaceIFaceBase.h"
#include "CvDLLEntityIFaceBase.h"
#include "CvDLLFAStarIFaceBase.h"


#define PATH_MOVEMENT_WEIGHT    (1000)
//#define PATH_RIVER_WEIGHT     (100)
#define PATH_RIVER_WEIGHT       (20) // K-Mod ( * river crossing penalty)
//#define PATH_CITY_WEIGHT      (100)
#define PATH_CITY_WEIGHT        (200) // K-Mod
//#define PATH_DEFENSE_WEIGHT   (10)
#define PATH_DEFENSE_WEIGHT     (4) // K-Mod. ( * defence bonus)
#define PATH_TERRITORY_WEIGHT   (5) // was 3
#define PATH_STEP_WEIGHT        (4) // was 2
#define PATH_STRAIGHT_WEIGHT    (2) // was 1
//#define PATH_ASYMMETRY_WEIGHT   (1) // K-Mod

// #define PATH_DAMAGE_WEIGHT      (500) // K-Mod (disabled because it isn't used)
#define PATH_COMBAT_WEIGHT      (300) // K-Mod. penalty for having to fight along the way.
// Note: there will also be other combat penalties added, for example from defence weight and city weight.


// <advc.003g>
using std::vector;

int roundToMultiple(double d, int iMultiple) {

	int r = (int)(d + 0.5 * iMultiple);
	return r - r % iMultiple;
}

bool bernoulliSuccess(double pr, char const* pszLog) {

    int chancePerMyriad = round(pr * 10000.0);
	// These two checks are just for better performance
	if(chancePerMyriad >= 10000)
		return true;
	if(chancePerMyriad <= 0)
		return false;
	if(strlen(pszLog) <= 0)
		pszLog = "bs";
    return GC.getGameINLINE().getSorenRandNum(10000, pszLog) < chancePerMyriad;
}

double median(vector<double>& distribution, bool bSorted) {

	FAssert(!distribution.empty());
	if(!bSorted)
		std::sort(distribution.begin(), distribution.end());
	int medianIndex = distribution.size() / 2;
	if(distribution.size() % 2 != 0)
		return distribution[medianIndex];
	return (distribution[medianIndex] + distribution[medianIndex - 1]) / 2;
}

double mean(vector<double>& distribution) {

	FAssert(!distribution.empty());
	double r = 0;
	for(size_t i = 0; i < distribution.size(); i++)
		r += distribution[i];
	return r / distribution.size();
}

double max(vector<double>& distribution) {

	FAssert(!distribution.empty());
	double r = distribution[0];
	for(size_t i = 1; i < distribution.size(); i++)
		if(distribution[i] > r)
			r = distribution[i];
	return r;
}

double min(vector<double>& distribution) {

	FAssert(!distribution.empty());
	double r = distribution[0];
	for(size_t i = 1; i < distribution.size(); i++)
		if(distribution[i] < r)
			r = distribution[i];
	return r;
}

double percentileRank(vector<double>& distribution, double score,
		bool bSorted, bool bScorePartOfDistribution) {
  // default: false, true

	if(!bSorted)
		std::sort(distribution.begin(), distribution.end());
	int n = (int)distribution.size();
	int nLEq = 0; // less or equal
	for(int i = 0; i < n; i++) {
		if(distribution[i] <= score)
			nLEq++;
		else break;
	}
	if(bScorePartOfDistribution) {
		nLEq++;
		n++;
	}
	else if(n == 0)
		return 1;
	return nLEq / (double)n;
} // </advc.003g>
// <advc.003>
float hash(vector<long> const& x, PlayerTypes civId) {

	int const prime = 31;
	long hashVal = 0;
	for(size_t i = 0; i < x.size(); i++) {
		hashVal += x[i];
		hashVal *= prime;
	}
	int capIndex = -1;
	if(civId != NO_PLAYER) {
		CvCity* cap = GET_PLAYER(civId).getCapitalCity();
		if(cap != NULL)
			capIndex = GC.getMapINLINE().plotNumINLINE(cap->plot()->getX_INLINE(),
					cap->plot()->getY_INLINE());
	}
	if(capIndex >= 0) {
		hashVal += capIndex;
		hashVal *= prime;
	}
	CvRandom rng;
	rng.init(hashVal);
	return rng.getFloat();
}

float hash(long x, PlayerTypes civId) {

	vector<long> v;
	v.push_back(x);
	return hash(v, civId);
}

void fatCross(CvPlot const& p, vector<CvPlot*>& r) {

	FAssert(r.empty());
	r.reserve(21);
	for(int i = 0; i < 21; i++)
		r.push_back(NULL);
	r[0] = const_cast<CvPlot*>(&p);
	int pos = 1;
	CvMap& map = GC.getMap();
	for(int dx = -CITY_PLOTS_RADIUS; dx <= CITY_PLOTS_RADIUS; dx++) {
		for(int dy = -CITY_PLOTS_RADIUS; dy <= CITY_PLOTS_RADIUS; dy++) {
			// Skip corners and center
			if(std::abs(dx) + std::abs(dy) == 4 || (dx == 0 && dy == 0))
				continue;
			// that's NULL if off the map
			r[pos] = map.plot(r[0]->getX_INLINE() + dx, r[0]->getY_INLINE() + dy);
			pos++;
		}
	}
	FAssert(pos == 21);
} // </advc.003>
// <advc.035>
void contestedPlots(std::vector<CvPlot*>& r, TeamTypes t1, TeamTypes t2) {

	if(GC.getOWN_EXCLUSIVE_RADIUS() <= 0)
		return;
	// Enough to check plots around the teams' cities
	std::vector<CvCity*> cities;
	for(int i = 0; i < MAX_CIV_PLAYERS; i++) {
		CvPlayer& member = GET_PLAYER((PlayerTypes)i);
		if(!member.isAlive() || (member.getTeam() != t1 && member.getTeam() != t2))
			continue;
		int dummy=-1;
		for(CvCity* c = member.firstCity(&dummy); c != NULL; c = member.nextCity(&dummy))
			cities.push_back(c);
	}
	std::set<int> seenPlots; // To avoid duplicates
	for(size_t i = 0; i < cities.size(); i++) {
		CvCity const& c = *cities[i];
		std::vector<CvPlot*> cross;
		fatCross(*c.plot(), cross);
		for(size_t j = 1; j < cross.size(); j++) {
			if(cross[j] == NULL) continue;
			CvPlot& p = *cross[j];
			if(p.isCity())
				continue;
			PlayerTypes secondOwner = p.getSecondOwner();
			PlayerTypes owner = p.getOwnerINLINE();
			if(owner == NO_PLAYER || secondOwner == NO_PLAYER)
				continue;
			TeamTypes team = TEAMID(owner);
			TeamTypes secondTeam = TEAMID(secondOwner);
			if(team == secondTeam)
				continue;
			if((team == t1 && secondTeam == t2) || (team == t2 && secondTeam == t1)) {
				int plotId = p.getX_INLINE() * 1000 + p.getY_INLINE();
				if(seenPlots.count(plotId) <= 0) {
					seenPlots.insert(plotId);
					r.push_back(&p);
				}
			}
		}
	}
} // </advc.035>

// <advc.008e>
bool isArticle(BuildingTypes bt) {

	CvBuildingInfo const& bi = GC.getBuildingInfo(bt);
	if(!isWorldWonderClass((BuildingClassTypes)bi.getBuildingClassType()))
		return false; // Should only be called for wonders really
	CvWString txtKey = bi.getTextKeyWide();
	CvWString txt = gDLL->getText(txtKey + L"_NA");
	/*  If an _NA key exists, then gDLL will return a dot. If it doesn't, then
		an article should be used. */
	return (txt.compare(L".") != 0);
}
bool isArticle(ProjectTypes pt) {

	CvProjectInfo const& pi = GC.getProjectInfo(pt);
	if(!isLimitedProject(pt))
		return false;
	CvWString txtKey = pi.getTextKeyWide();
	CvWString txt = gDLL->getText(txtKey + L"_NA");
	return (txt.compare(L".") != 0);
} // </advc.008e>

CvPlot* plotCity(int iX, int iY, int iIndex)
{
	return GC.getMapINLINE().plotINLINE((iX + GC.getCityPlotX()[iIndex]), (iY + GC.getCityPlotY()[iIndex]));
}

int plotCityXY(int iDX, int iDY)
{
	if ((abs(iDX) > CITY_PLOTS_RADIUS) || (abs(iDY) > CITY_PLOTS_RADIUS))
	{
		return -1;
	}
	else
	{
		return GC.getXYCityPlot((iDX + CITY_PLOTS_RADIUS), (iDY + CITY_PLOTS_RADIUS));
	}
}

int plotCityXY(const CvCity* pCity, const CvPlot* pPlot)
{
	return plotCityXY(dxWrap(pPlot->getX_INLINE() - pCity->getX_INLINE()), dyWrap(pPlot->getY_INLINE() - pCity->getY_INLINE()));
}

/* <advc.303> Has to include the city tile in order to be compatible with
   CvPlayer::AI_foundValue_bulk */
bool isInnerRing(CvPlot const* pl, CvPlot const* cityPl) {
	return pl != NULL && cityPl != NULL && plotDistance(pl, cityPl) <= 1;
} // </advc.303>

CardinalDirectionTypes getOppositeCardinalDirection(CardinalDirectionTypes eDir)
{
	return (CardinalDirectionTypes)((eDir + 2) % NUM_CARDINALDIRECTION_TYPES);
}

DirectionTypes cardinalDirectionToDirection(CardinalDirectionTypes eCard)
{
	switch (eCard)
	{
	case CARDINALDIRECTION_NORTH:
		return DIRECTION_NORTH;
	case CARDINALDIRECTION_EAST:
		return DIRECTION_EAST;
	case CARDINALDIRECTION_SOUTH:
		return DIRECTION_SOUTH;
	case CARDINALDIRECTION_WEST:
		return DIRECTION_WEST;
	}
	return NO_DIRECTION;
}

bool isCardinalDirection(DirectionTypes eDirection)
{
	switch( eDirection )
	{
	case DIRECTION_EAST:
	case DIRECTION_NORTH:
	case DIRECTION_SOUTH:
	case DIRECTION_WEST:
		return true;
	}
	return false;
}

DirectionTypes estimateDirection(int iDX, int iDY)
{
	const int displacementSize = 8;
	static float sqrt2 = 1 / sqrt(2.0f);
	//													N			NE			E			SE				S			SW				W			NW
	static float displacements[displacementSize][2] = {{0, 1}, {sqrt2, sqrt2}, {1, 0}, {sqrt2, -sqrt2}, {0, -1}, {-sqrt2, -sqrt2}, {-1, 0}, {-sqrt2, sqrt2}};
	float maximum = 0;
	int maximumIndex = -1;
	for(int i=0;i<displacementSize;i++)
	{
		float dotProduct = iDX * displacements[i][0] + iDY * displacements[i][1];
		if(dotProduct > maximum)
		{
			maximum = dotProduct;
			maximumIndex = i;
		}
	}

	return (DirectionTypes) maximumIndex;
}

DirectionTypes estimateDirection(const CvPlot* pFromPlot, const CvPlot* pToPlot)
{
	return estimateDirection(dxWrap(pToPlot->getX_INLINE() - pFromPlot->getX_INLINE()), dyWrap(pToPlot->getY_INLINE() - pFromPlot->getY_INLINE()));
}


float directionAngle( DirectionTypes eDirection )
{
	switch( eDirection )
	{
	case DIRECTION_NORTHEAST:	return fM_PI * 0.25f;
	case DIRECTION_EAST:			return fM_PI * 0.5f;
	case DIRECTION_SOUTHEAST:	return fM_PI * 0.75f;
	case DIRECTION_SOUTH:			return fM_PI * 1.0f;
	case DIRECTION_SOUTHWEST:	return fM_PI * 1.25f;
	case DIRECTION_WEST:			return fM_PI * 1.5f;
	case DIRECTION_NORTHWEST:	return fM_PI * 1.75f;
	default:
	case DIRECTION_NORTH:			return 0.0f;
	}
}

bool atWar(TeamTypes eTeamA, TeamTypes eTeamB)
{
	if(eTeamA == NO_TEAM || eTeamB == NO_TEAM)
		return false;

	// advc.134a: Use internal function. (And removed some assertions.)
	return GET_TEAM(eTeamA).isAtWarInternal(eTeamB);
}

bool isPotentialEnemy(TeamTypes eOurTeam, TeamTypes eTheirTeam)
{
	FAssert(eOurTeam != NO_TEAM);

	if (eTheirTeam == NO_TEAM)
	{
		return false;
	}
	return (atWar(eOurTeam, eTheirTeam) || (GET_TEAM(eOurTeam).AI_isSneakAttackReady(
			//eTheirTeam)
			GET_TEAM(eTheirTeam).getMasterTeam()) // advc.104j
/************************************************************************************************/
/* UNOFFICIAL_PATCH                       05/05/09                                jdog5000      */
/*                                                                                              */
/* Bugfix, General AI                                                                           */
/************************************************************************************************/
			// Fixes bug where AI would launch invasion while unable to declare war
			// which caused units to be bumped once forced peace expired
			&& GET_TEAM(GET_TEAM(eOurTeam).getMasterTeam()). // advc.104j
					canDeclareWar(eTheirTeam)));
/************************************************************************************************/
/* UNOFFICIAL_PATCH                        END                                                  */
/************************************************************************************************/
}

CvCity* getCity(IDInfo city)
{
	if ((city.eOwner >= 0) && city.eOwner < MAX_PLAYERS)
	{
		return (GET_PLAYER((PlayerTypes)city.eOwner).getCity(city.iID));
	}

	return NULL;
}

CvUnit* getUnit(IDInfo unit)
{
	if ((unit.eOwner >= 0) && unit.eOwner < MAX_PLAYERS)
	{
		return (GET_PLAYER((PlayerTypes)unit.eOwner).getUnit(unit.iID));
	}

	return NULL;
}

bool isBeforeUnitCycle(const CvUnit* pFirstUnit, const CvUnit* pSecondUnit)
{
	FAssert(pFirstUnit != NULL);
	FAssert(pSecondUnit != NULL);
	FAssert(pFirstUnit != pSecondUnit);

	if (pFirstUnit->getOwnerINLINE() != pSecondUnit->getOwnerINLINE())
	{
		return (pFirstUnit->getOwnerINLINE() < pSecondUnit->getOwnerINLINE());
	}

	if (pFirstUnit->getDomainType() != pSecondUnit->getDomainType())
	{
		return (pFirstUnit->getDomainType() < pSecondUnit->getDomainType());
	}

	/* if (pFirstUnit->baseCombatStr() != pSecondUnit->baseCombatStr())
	{
		return (pFirstUnit->baseCombatStr() > pSecondUnit->baseCombatStr());
	} */ // disabled by K-Mod

	if (pFirstUnit->getUnitType() != pSecondUnit->getUnitType())
	{
		return (pFirstUnit->getUnitType() > pSecondUnit->getUnitType());
	}

	if (pFirstUnit->getLevel() != pSecondUnit->getLevel())
	{
		return (pFirstUnit->getLevel() > pSecondUnit->getLevel());
	}

	if (pFirstUnit->getExperience() != pSecondUnit->getExperience())
	{
		return (pFirstUnit->getExperience() > pSecondUnit->getExperience());
	}

	return (pFirstUnit->getID() < pSecondUnit->getID());
}

// K-Mod
// return true if the first unit in the first group comes before the first unit in the second group.
// (note: the purpose of this function is to return _false_ when the groupCycleDistance should include a pentalty.)
bool isBeforeGroupOnPlot(const CvSelectionGroup* pFirstGroup, const CvSelectionGroup* pSecondGroup)
{
	FAssert(pFirstGroup && pSecondGroup);
	FAssert(pFirstGroup != pSecondGroup);
	FAssert(pFirstGroup->plot() == pSecondGroup->plot());

	CvPlot* pPlot = pFirstGroup->plot();
	//int iGroup2Units = pSecondGroup->getNumUnits();

	CLLNode<IDInfo>* pUnitNode = pPlot->headUnitNode();
	while (pUnitNode)// && iGroup2Units > 0)
	{
		CvUnit* pLoopUnit = ::getUnit(pUnitNode->m_data);

		if (pLoopUnit->getGroup() == pFirstGroup)
			return true;
		if (pLoopUnit->getGroup() == pSecondGroup)
			return false;
			//iGroup2Units--;

		pUnitNode = pPlot->nextUnitNode(pUnitNode);
	}

	FAssert(false);
	return false;
}

// return the 'cost' of cycling from pFirstGroup to pSecondGroup. (eg. a big jump to a differnet type of unit, then it should be a high cost.)
int groupCycleDistance(const CvSelectionGroup* pFirstGroup, const CvSelectionGroup* pSecondGroup)
{
	FAssert(pFirstGroup && pSecondGroup && pFirstGroup != pSecondGroup);

	CvUnit* pFirstHead = pFirstGroup->getHeadUnit();
	CvUnit* pSecondHead = pSecondGroup->getHeadUnit();

	FAssert(pFirstHead && pSecondHead);

	const int iBaseScale = 4;
	int iPenalty = 0;
	if (pFirstHead->getUnitType() != pSecondHead->getUnitType())
	{
		if (pFirstHead->canFight() != pSecondHead->canFight())
			iPenalty += 4;
		else
		{
			if (pFirstHead->canFight())
			{
				if (pFirstHead->getUnitCombatType() != pSecondHead->getUnitCombatType())
					iPenalty += 2;
				if (pFirstHead->canAttack() != pSecondHead->canAttack()
						// <advc.315>
						|| ::isMostlyDefensive(pFirstHead->getUnitInfo()) !=
						::isMostlyDefensive(pSecondHead->getUnitInfo()))
						// </advc.315>
					iPenalty += 1;
			}
			else
				iPenalty += 2;
		}
	}

	int iDistance = plotDistance(pFirstHead->getX_INLINE(), pFirstHead->getY_INLINE(), pSecondHead->getX_INLINE(), pSecondHead->getY_INLINE());
	iPenalty = std::min(5, iPenalty * (1+iDistance) / iBaseScale);

	// For human players, use the unit order that the plot actually has, not the order it _should_ have.
	// For AI players, use the preferred ordering, because it's slightly faster.
	if (iDistance == 0 && !(pFirstHead->isHuman() ? isBeforeGroupOnPlot(pFirstGroup, pSecondGroup) : isBeforeUnitCycle(pFirstHead, pSecondHead)))
		iPenalty += iPenalty > 0 ? 1 : 5;

	return iDistance + iPenalty;
}
// K-Mod end

bool isPromotionValid(PromotionTypes ePromotion, UnitTypes eUnit, bool bLeader)
{
	CvUnitInfo& kUnit = GC.getUnitInfo(eUnit);
	CvPromotionInfo& kPromotion = GC.getPromotionInfo(ePromotion);

	if (kUnit.getFreePromotions(ePromotion))
	{
		return true;
	}

	if (kUnit.getUnitCombatType() == NO_UNITCOMBAT)
	{
		return false;
	}

	if (!bLeader && kPromotion.isLeader())
	{
		return false;
	}

	if (!(kPromotion.getUnitCombat(kUnit.getUnitCombatType())))
	{
		return false;
	}

	if (kUnit.isOnlyDefensive())
	{
		if (kPromotion.getCityAttackPercent() != 0 ||
			  kPromotion.getWithdrawalChange() != 0 ||
			  kPromotion.getCollateralDamageChange() != 0 ||
			  kPromotion.//isBlitz()
				getBlitz() != 0 || // advc.164
			  kPromotion.isAmphib() ||
			  kPromotion.isRiver() ||
			  kPromotion.getHillsAttackPercent() != 0)
		{
			return false;
		}
	}

	if (kUnit.isIgnoreTerrainCost())
	{
		if (kPromotion.getMoveDiscountChange() != 0)
		{
			return false;
		}
	}
	// advc.164: Check this in CvUnit::isPromotionValid instead
	/*if (kUnit.getMoves() == 1)
	{
		if (kPromotion.isBlitz())
		{
			return false;
		}
	}*/

	if ((kUnit.getCollateralDamage() == 0) || (kUnit.getCollateralDamageLimit() == 0) || (kUnit.getCollateralDamageMaxUnits() == 0))
	{
		if (kPromotion.getCollateralDamageChange() != 0)
		{
			return false;
		}
	}

	if (kUnit.getInterceptionProbability() == 0)
	{
		if (kPromotion.getInterceptChange() != 0)
		{
			return false;
		}
	}

	if (NO_PROMOTION != kPromotion.getPrereqPromotion())
	{
		if (!isPromotionValid((PromotionTypes)kPromotion.getPrereqPromotion(), eUnit, bLeader))
		{
			return false;
		}
	}

	PromotionTypes ePrereq1 = (PromotionTypes)kPromotion.getPrereqOrPromotion1();
	PromotionTypes ePrereq2 = (PromotionTypes)kPromotion.getPrereqOrPromotion2();
/*
** K-Mod, 14/jan/11, karadoc
** third optional preq.
*/
	PromotionTypes ePrereq3 = (PromotionTypes)kPromotion.getPrereqOrPromotion3();
	/* original bts code
	if (NO_PROMOTION != ePrereq1 || NO_PROMOTION != ePrereq2)
	{
		bool bValid = false;
		if (!bValid)
		{
			if (NO_PROMOTION != ePrereq1 && isPromotionValid(ePrereq1, eUnit, bLeader))
			{
				bValid = true;
			}
		}

		if (!bValid)
		{
			if (NO_PROMOTION != ePrereq2 && isPromotionValid(ePrereq2, eUnit, bLeader))
			{
				bValid = true;
			}
		}*/
	if (ePrereq1 != NO_PROMOTION || ePrereq2 != NO_PROMOTION || ePrereq3 != NO_PROMOTION)
	{
		bool bValid = false;

		if (ePrereq1 != NO_PROMOTION && isPromotionValid(ePrereq1, eUnit, bLeader))
			bValid = true;
		if (ePrereq2 != NO_PROMOTION && isPromotionValid(ePrereq2, eUnit, bLeader))
			bValid = true;
		if (ePrereq3 != NO_PROMOTION && isPromotionValid(ePrereq3, eUnit, bLeader))
			bValid = true;

		if (!bValid)
		{
			return false;
		}
	}
/*
** K-Mod end
*/

	return true;
}

int getPopulationAsset(int iPopulation)
{
	return (iPopulation * 2);
}

int getLandPlotsAsset(int iLandPlots)
{
	return iLandPlots;
}

int getPopulationPower(int iPopulation)
{
	return (iPopulation / 2);
}

int getPopulationScore(int iPopulation)
{
	return iPopulation;
}

int getLandPlotsScore(int iLandPlots)
{
	return iLandPlots;
}

int getTechScore(TechTypes eTech)
{
	return (GC.getTechInfo(eTech).getEra() + 1);
}

int getWonderScore(BuildingClassTypes eWonderClass)
{
	if (isLimitedWonderClass(eWonderClass))
	{
		return 5;
	}
	else
	{
		return 0;
	}
}

/* original bts code
ImprovementTypes finalImprovementUpgrade(ImprovementTypes eImprovement, int iCount)
{
	FAssertMsg(eImprovement != NO_IMPROVEMENT, "Improvement is not assigned a valid value");

	if (iCount > GC.getNumImprovementInfos())
	{
		return NO_IMPROVEMENT;
	}

	if (GC.getImprovementInfo(eImprovement).getImprovementUpgrade() != NO_IMPROVEMENT)
	{
		return finalImprovementUpgrade(((ImprovementTypes)(GC.getImprovementInfo(eImprovement).getImprovementUpgrade())), (iCount + 1));
	}
	else
	{
		return eImprovement;
	}
} */
// K-Mod
ImprovementTypes finalImprovementUpgrade(ImprovementTypes eImprovement)
{
	if (eImprovement == NO_IMPROVEMENT)
		return NO_IMPROVEMENT;

	FAssert(eImprovement < GC.getNumImprovementInfos());

	int iLoopDetector = GC.getNumImprovementInfos();

	while (GC.getImprovementInfo(eImprovement).getImprovementUpgrade() != NO_IMPROVEMENT && --iLoopDetector > 0)
		eImprovement = (ImprovementTypes)GC.getImprovementInfo(eImprovement).getImprovementUpgrade();

	return iLoopDetector == 0 ? NO_IMPROVEMENT : eImprovement;
}
// K-Mod end

int getWorldSizeMaxConscript(CivicTypes eCivic)
{
	int iMaxConscript;

	iMaxConscript = GC.getCivicInfo(eCivic).getMaxConscript();

	iMaxConscript *= std::max(0, (GC.getWorldInfo(GC.getMapINLINE().getWorldSize()).getMaxConscriptModifier() + 100));
	iMaxConscript /= 100;

	return iMaxConscript;
}

bool isReligionTech(TechTypes eTech)
{
	int iI;

	for (iI = 0; iI < GC.getNumReligionInfos(); iI++)
	{
		if (GC.getReligionInfo((ReligionTypes)iI).getTechPrereq() == eTech)
		{
			return true;
		}
	}

	return false;
}

bool isCorporationTech(TechTypes eTech)
{
	int iI;

	for (iI = 0; iI < GC.getNumCorporationInfos(); iI++)
	{
		if (GC.getCorporationInfo((CorporationTypes)iI).getTechPrereq() == eTech)
		{
			return true;
		}
	}

	return false;
}

bool isTechRequiredForUnit(TechTypes eTech, UnitTypes eUnit)
{
	int iI;
	CvUnitInfo& info = GC.getUnitInfo(eUnit);

	if (info.getPrereqAndTech() == eTech)
	{
		return true;
	}

	for (iI = 0; iI < GC.getNUM_UNIT_AND_TECH_PREREQS(); iI++)
	{
		if (info.getPrereqAndTechs(iI) == eTech)
		{
			return true;
		}
	}

	return false;
}

bool isTechRequiredForBuilding(TechTypes eTech, BuildingTypes eBuilding)
{
	int iI;
	CvBuildingInfo& info = GC.getBuildingInfo(eBuilding);

	if (info.getPrereqAndTech() == eTech)
	{
		return true;
	}

	for (iI = 0; iI < GC.getNUM_BUILDING_AND_TECH_PREREQS(); iI++)
	{
		if (info.getPrereqAndTechs(iI) == eTech)
		{
			return true;
		}
	}

	SpecialBuildingTypes eSpecial = (SpecialBuildingTypes)info.getSpecialBuildingType();
	if (NO_SPECIALBUILDING != eSpecial && GC.getSpecialBuildingInfo(eSpecial).getTechPrereq() == eTech)
	{
		return true;
	}

	return false;
}

bool isTechRequiredForProject(TechTypes eTech, ProjectTypes eProject)
{
	if (GC.getProjectInfo(eProject).getTechPrereq() == eTech)
	{
		return true;
	}

	return false;
}

bool isWorldUnitClass(UnitClassTypes eUnitClass)
{
	return (GC.getUnitClassInfo(eUnitClass).getMaxGlobalInstances() != -1);
}

bool isTeamUnitClass(UnitClassTypes eUnitClass)
{
	return (GC.getUnitClassInfo(eUnitClass).getMaxTeamInstances() != -1);
}

bool isNationalUnitClass(UnitClassTypes eUnitClass)
{
	return (GC.getUnitClassInfo(eUnitClass).getMaxPlayerInstances() != -1);
}

bool isLimitedUnitClass(UnitClassTypes eUnitClass)
{
	return (isWorldUnitClass(eUnitClass) || isTeamUnitClass(eUnitClass) || isNationalUnitClass(eUnitClass));
}
// <advc.104>
bool isMundaneBuildingClass(int buildingClass) {

	BuildingClassTypes bc = (BuildingClassTypes)buildingClass;
	return !isWorldWonderClass(bc) && !isTeamWonderClass(bc) &&
			!isNationalWonderClass(bc) && !isLimitedWonderClass(bc);
} // </advc.104>

bool isWorldWonderClass(BuildingClassTypes eBuildingClass)
{
	return (GC.getBuildingClassInfo(eBuildingClass).getMaxGlobalInstances() != -1);
}

bool isTeamWonderClass(BuildingClassTypes eBuildingClass)
{
	return (GC.getBuildingClassInfo(eBuildingClass).getMaxTeamInstances() != -1);
}

bool isNationalWonderClass(BuildingClassTypes eBuildingClass)
{
	return (GC.getBuildingClassInfo(eBuildingClass).getMaxPlayerInstances() != -1);
}

bool isLimitedWonderClass(BuildingClassTypes eBuildingClass)
{
	return (isWorldWonderClass(eBuildingClass) || isTeamWonderClass(eBuildingClass) || isNationalWonderClass(eBuildingClass));
}

int limitedWonderClassLimit(BuildingClassTypes eBuildingClass)
{
	int iMax;
	int iCount = 0;
	bool bLimited = false;

	iMax = GC.getBuildingClassInfo(eBuildingClass).getMaxGlobalInstances();
	if (iMax != -1)
	{
		iCount += iMax;
		bLimited = true;
	}

	iMax = GC.getBuildingClassInfo(eBuildingClass).getMaxTeamInstances();
	if (iMax != -1)
	{
		iCount += iMax;
		bLimited = true;
	}

	iMax = GC.getBuildingClassInfo(eBuildingClass).getMaxPlayerInstances();
	if (iMax != -1)
	{
		iCount += iMax;
		bLimited = true;
	}

	return bLimited ? iCount : -1;
}

bool isWorldProject(ProjectTypes eProject)
{
	return (GC.getProjectInfo(eProject).getMaxGlobalInstances() != -1);
}

bool isTeamProject(ProjectTypes eProject)
{
	return (GC.getProjectInfo(eProject).getMaxTeamInstances() != -1);
}

bool isLimitedProject(ProjectTypes eProject)
{
	return (isWorldProject(eProject) || isTeamProject(eProject));
}

// FUNCTION: getBinomialCoefficient
// Needed for getCombatOdds
// Returns int value, being the possible number of combinations 
// of k draws out of a population of n
// Written by DeepO 
// Modified by Jason Winokur to keep the intermediate factorials small
__int64 getBinomialCoefficient(int iN, int iK)
{
	__int64 iTemp = 1;
	//take advantage of symmetry in combination, eg. 15C12 = 15C3
	iK = std::min(iK, iN - iK);
	
	//eg. 15C3 = (15 * 14 * 13) / (1 * 2 * 3) = 15 / 1 * 14 / 2 * 13 / 3 = 455
	for(int i=1;i<=iK;i++)
		iTemp = (iTemp * (iN - i + 1)) / i;

	// Make sure iTemp fits in an integer (and thus doesn't overflow)
	FAssert(iTemp < MAX_INT);

	return iTemp;
}

// FUNCTION: getCombatOdds
// Calculates combat odds, given two units
// Returns value from 0-1000
// Written by DeepO
int getCombatOdds(const CvUnit* pAttacker, const CvUnit* pDefender)
{
	float fOddsEvent;
	float fOddsAfterEvent;
	int iAttackerStrength;
	int iAttackerFirepower;
	int iDefenderStrength;
	int iDefenderFirepower;
	int iDefenderOdds;
	int iAttackerOdds;
	int iStrengthFactor;
	int iDamageToAttacker;
	int iDamageToDefender;
	int iNeededRoundsAttacker;
	int iNeededRoundsDefender;
	int iMaxRounds;
	int iAttackerLowFS;
	int iAttackerHighFS;
	int iDefenderLowFS;
	int iDefenderHighFS;
	int iFirstStrikes;
	int iDefenderHitLimit;
	int iI;
	int iJ;
	int iI3;
	int iI4;
	int iOdds = 0;

	// setup battle, calculate strengths and odds
	//////

	//Added ST
	iAttackerStrength = pAttacker->currCombatStr(NULL, NULL);
	iAttackerFirepower = pAttacker->currFirepower(NULL, NULL);

	iDefenderStrength = pDefender->currCombatStr(pDefender->plot(), pAttacker);
	iDefenderFirepower = pDefender->currFirepower(pDefender->plot(), pAttacker);

	FAssert((iAttackerStrength + iDefenderStrength) > 0);
	FAssert((iAttackerFirepower + iDefenderFirepower) > 0);

	iDefenderOdds = ((GC.getCOMBAT_DIE_SIDES() * iDefenderStrength) / (iAttackerStrength + iDefenderStrength));

	if (iDefenderOdds == 0)
	{
		return 1000;
	}

	iAttackerOdds = GC.getCOMBAT_DIE_SIDES() - iDefenderOdds;	

	if (iAttackerOdds == 0)
	{
		return 0;
	}

	iStrengthFactor = ((iAttackerFirepower + iDefenderFirepower + 1) / 2);

	// calculate damage done in one round
	//////

	iDamageToAttacker = std::max(1,((GC.getCOMBAT_DAMAGE() * (iDefenderFirepower + iStrengthFactor)) / (iAttackerFirepower + iStrengthFactor)));
	iDamageToDefender = std::max(1,((GC.getCOMBAT_DAMAGE() * (iAttackerFirepower + iStrengthFactor)) / (iDefenderFirepower + iStrengthFactor)));

	// calculate needed rounds.
	// Needed rounds = round_up(health/damage)
	//////

	iDefenderHitLimit = pDefender->maxHitPoints() - pAttacker->combatLimit();

	iNeededRoundsAttacker = (std::max(0, pDefender->currHitPoints() - iDefenderHitLimit) + iDamageToDefender - 1 ) / iDamageToDefender;
	iNeededRoundsDefender = (pAttacker->currHitPoints() + iDamageToAttacker - 1 ) / iDamageToAttacker;
	iMaxRounds = iNeededRoundsAttacker + iNeededRoundsDefender - 1;

	// calculate possible first strikes distribution.
	// We can't use the getCombatFirstStrikes() function (only one result,
	// no distribution), so we need to mimic it.
	//////

	iAttackerLowFS = (pDefender->immuneToFirstStrikes()) ? 0 : pAttacker->firstStrikes();
	iAttackerHighFS = (pDefender->immuneToFirstStrikes()) ? 0 : (pAttacker->firstStrikes() + pAttacker->chanceFirstStrikes());

	iDefenderLowFS = (pAttacker->immuneToFirstStrikes()) ? 0 : pDefender->firstStrikes();
	iDefenderHighFS = (pAttacker->immuneToFirstStrikes()) ? 0 : (pDefender->firstStrikes() + pDefender->chanceFirstStrikes());

	// UncutDragon
	if (GC.getLFBEnable())
		return LFBgetCombatOdds(iAttackerLowFS, iAttackerHighFS, iDefenderLowFS, iDefenderHighFS, iNeededRoundsAttacker, iNeededRoundsDefender, iAttackerOdds);
	// /UncutDragon

	// For every possible first strike event, calculate the odds of combat.
	// Then, add these to the total, weighted to the chance of that first 
	// strike event occurring
	//////

	for (iI = iAttackerLowFS; iI < iAttackerHighFS + 1; iI++)
	{
		for (iJ = iDefenderLowFS; iJ < iDefenderHighFS + 1; iJ++)
		{
			// for every possible combination of fs results, calculate the chance

			if (iI >= iJ)
			{
				// Attacker gets more or equal first strikes than defender

				iFirstStrikes = iI - iJ;

				// For every possible first strike getting hit, calculate both
				// the chance of that event happening, as well as the rest of 
				// the chance assuming the event has happened. Multiply these 
				// together to get the total chance (Bayes rule). 
				// iI3 counts the number of successful first strikes
				//////

				for (iI3 = 0; iI3 < (iFirstStrikes + 1); iI3++)
				{
					// event: iI3 first strikes hit the defender

					// calculate chance of iI3 first strikes hitting: fOddsEvent
					// f(k;n,p)=C(n,k)*(p^k)*((1-p)^(n-k)) 
					// this needs to be in floating point math
					//////

					fOddsEvent = ((float)getBinomialCoefficient(iFirstStrikes, iI3)) * pow((((float)iAttackerOdds) / GC.getCOMBAT_DIE_SIDES()), iI3) * pow((1.0f - (((float)iAttackerOdds) / GC.getCOMBAT_DIE_SIDES())), (iFirstStrikes - iI3));

					// calculate chance assuming iI3 first strike hits: fOddsAfterEvent
					//////

					if (iI3 >= iNeededRoundsAttacker)
					{
						fOddsAfterEvent = 1;
					}
					else
					{
						fOddsAfterEvent = 0;

						// odds for _at_least_ (iNeededRoundsAttacker - iI3) (the remaining hits 
						// the attacker needs to make) out of (iMaxRounds - iI3) (the left over 
						// rounds) is the sum of each _exact_ draw
						//////

						for (iI4 = (iNeededRoundsAttacker - iI3); iI4 < (iMaxRounds - iI3 + 1); iI4++)
						{
							// odds of exactly iI4 out of (iMaxRounds - iI3) draws.
							// f(k;n,p)=C(n,k)*(p^k)*((1-p)^(n-k)) 
							// this needs to be in floating point math
							//////

							fOddsAfterEvent += ((float)getBinomialCoefficient((iMaxRounds - iI3), iI4)) * pow((((float)iAttackerOdds) / GC.getCOMBAT_DIE_SIDES()), iI4) * pow((1.0f - (((float)iAttackerOdds) / GC.getCOMBAT_DIE_SIDES())), ((iMaxRounds - iI3) - iI4));
						}
					}

					// Multiply these together, round them properly, and add 
					// the result to the total iOdds
					//////

					iOdds += ((int)(1000.0 * (fOddsEvent*fOddsAfterEvent + 0.0005)));
				}
			}
			else // (iI < iJ)
			{
				// Attacker gets less first strikes than defender

				iFirstStrikes = iJ - iI;

				// For every possible first strike getting hit, calculate both
				// the chance of that event happening, as well as the rest of 
				// the chance assuming the event has happened. Multiply these 
				// together to get the total chance (Bayes rule). 
				// iI3 counts the number of successful first strikes
				//////

				for (iI3 = 0; iI3 < (iFirstStrikes + 1); iI3++)
				{
					// event: iI3 first strikes hit the defender

					// First of all, check if the attacker is still alive.
					// Otherwise, no further calculations need to occur 
					/////

					if (iI3 < iNeededRoundsDefender)
					{
						// calculate chance of iI3 first strikes hitting: fOddsEvent
						// f(k;n,p)=C(n,k)*(p^k)*((1-p)^(n-k)) 
						// this needs to be in floating point math
						//////

						fOddsEvent = ((float)getBinomialCoefficient(iFirstStrikes, iI3)) * pow((((float)iDefenderOdds) / GC.getCOMBAT_DIE_SIDES()), iI3) * pow((1.0f - (((float)iDefenderOdds) / GC.getCOMBAT_DIE_SIDES())), (iFirstStrikes - iI3));

						// calculate chance assuming iI3 first strike hits: fOddsAfterEvent
						//////

						fOddsAfterEvent = 0;

						// odds for _at_least_ iNeededRoundsAttacker (the remaining hits 
						// the attacker needs to make) out of (iMaxRounds - iI3) (the left over 
						// rounds) is the sum of each _exact_ draw
						//////

						for (iI4 = iNeededRoundsAttacker; iI4 < (iMaxRounds - iI3 + 1); iI4++)
						{

							// odds of exactly iI4 out of (iMaxRounds - iI3) draws.
							// f(k;n,p)=C(n,k)*(p^k)*((1-p)^(n-k)) 
							// this needs to be in floating point math
							//////

							fOddsAfterEvent += ((float)getBinomialCoefficient((iMaxRounds - iI3), iI4)) * pow((((float)iAttackerOdds) / GC.getCOMBAT_DIE_SIDES()), iI4) * pow((1.0f - (((float)iAttackerOdds) / GC.getCOMBAT_DIE_SIDES())), ((iMaxRounds - iI3) - iI4));
						}

						// Multiply these together, round them properly, and add 
						// the result to the total iOdds
						//////

						iOdds += ((int)(1000.0 * (fOddsEvent*fOddsAfterEvent + 0.0005)));
					}
				}				
			}
		}
	}

	// Weigh the total to the number of possible combinations of first strikes events
	// note: the integer math breaks down when #FS > 656 (with a die size of 1000)
	//////

	iOdds /= (((pDefender->immuneToFirstStrikes()) ? 0 : pAttacker->chanceFirstStrikes()) + 1) * (((pAttacker->immuneToFirstStrikes()) ? 0 : pDefender->chanceFirstStrikes()) + 1); 

	// finished!
	//////

	return iOdds;
}

// K-Mod
int estimateCollateralWeight(const CvPlot* pPlot, TeamTypes eAttackTeam, TeamTypes eDefenceTeam)
{
	int iBaseCollateral = GC.getDefineINT("COLLATERAL_COMBAT_DAMAGE"); // note: the default xml value is "10"

	// Collateral damage does not depend on any kind of strength bonus - so when a unit takes collateral damage, their bonuses are effectively wasted.
	// Therefore, I'm going to inflate the value of collateral damage based on a rough estimate of the defenders bonuses might be.
	if (pPlot == NULL)
	{
		iBaseCollateral *= 110;
	}
	else
	{
		TeamTypes ePlotBonusTeam = eDefenceTeam;
		if (ePlotBonusTeam == NO_TEAM)
			ePlotBonusTeam = pPlot->getTeam() == eAttackTeam ? NO_TEAM : pPlot->getTeam();

		iBaseCollateral *= (pPlot->isCity() ? 130 : 110) + pPlot->defenseModifier(ePlotBonusTeam, false
				, eAttackTeam); // advc.012

		// Estimate the average collateral damage reduction of the units on the plot
		int iResistanceSum = 0;
		int iUnits = 0;

		CLLNode<IDInfo>* pUnitNode = pPlot->headUnitNode();

		while (pUnitNode)
		{
			CvUnit* pLoopUnit = ::getUnit(pUnitNode->m_data);
			pUnitNode = pPlot->nextUnitNode(pUnitNode);

			if (!pLoopUnit->canDefend(pPlot))
				continue;
			if (eDefenceTeam != NO_TEAM && pLoopUnit->getTeam() != eDefenceTeam)
				continue;
			if (eAttackTeam != NO_TEAM && pLoopUnit->getTeam() == eAttackTeam)
				continue;

			iUnits++;
			// Kludge! I'm only checking for immunity against the unit's own combat type.
			// Ideally we'd know what kind of collateral damage we're expecting to be hit by, and check for immunity vs that.
			// Or we could check all types... But the reality is, there are always going to be mods and fringe cases where
			// the esitmate is inaccurate. And currently in K-Mod, all instances of immunity are to the units own type anyway.
			// Whichever way we do the estimate, cho-ku-nu is going to mess it up anyway. (Unless I change the game mechanics.)
			if ( // advc.001: Animals have no unit combat type (K146 also fixes this)
				pLoopUnit->getUnitCombatType() != NO_UNITCOMBAT &&
				pLoopUnit->getUnitInfo().getUnitCombatCollateralImmune(pLoopUnit->getUnitCombatType()))
				iResistanceSum += 100;
			else
				iResistanceSum += pLoopUnit->getCollateralDamageProtection();
		}
		if (iUnits > 0)
			iBaseCollateral = iBaseCollateral * (iUnits * 100 - iResistanceSum)/(iUnits * 100);
	}
	return iBaseCollateral; // note, a factor of 100 is included in the result.
}
// K-Mod end

int getEspionageModifier(TeamTypes eOurTeam, TeamTypes eTargetTeam)
{
	FAssert(eOurTeam != eTargetTeam);
	FAssert(eOurTeam != BARBARIAN_TEAM);
	// FAssert(eTargetTeam != BARBARIAN_TEAM); // K-Mod note. This is possible for legitimate reasons (although, the result is never important...)

	/* original bts code
	int iTargetPoints = GET_TEAM(eTargetTeam).getEspionagePointsEver();
	int iOurPoints = GET_TEAM(eOurTeam).getEspionagePointsEver();

	int iModifier = GC.getDefineINT("ESPIONAGE_SPENDING_MULTIPLIER") * (2 * iTargetPoints + iOurPoints);
	iModifier /= std::max(1, iTargetPoints + 2 * iOurPoints);
	return iModifier; */
	// K-Mod. Scale the points modifier based on the teams' population. (Note ESPIONAGE_SPENDING_MULTIPLIER is 100 in the default xml.)
	const CvTeam& kOurTeam = GET_TEAM(eOurTeam);
	const CvTeam& kTargetTeam = GET_TEAM(eTargetTeam);

	int iPopScale = 5 * GC.getWorldInfo(GC.getMapINLINE().getWorldSize()).getTargetNumCities();
	int iTargetPoints = 10 * kTargetTeam.getEspionagePointsEver() / std::max(1, iPopScale + kTargetTeam.getTotalPopulation(false));
	int iOurPoints = 10 * kOurTeam.getEspionagePointsEver() / std::max(1, iPopScale + kOurTeam.getTotalPopulation(false));

	return GC.getDefineINT("ESPIONAGE_SPENDING_MULTIPLIER") * std::max(1, 2 * iTargetPoints + iOurPoints) / std::max(1, iTargetPoints + 2 * iOurPoints);
	// K-Mod end
}

void setTradeItem(TradeData* pItem, TradeableItems eItemType, int iData)
{
	pItem->m_eItemType = eItemType;
	pItem->m_iData = iData;
	pItem->m_bOffering = false;
	pItem->m_bHidden = false;
}

bool isPlotEventTrigger(EventTriggerTypes eTrigger)
{
	CvEventTriggerInfo& kTrigger = GC.getEventTriggerInfo(eTrigger);

	if (kTrigger.getNumPlotsRequired() > 0)
	{
		if (kTrigger.getPlotType() != NO_PLOT)
		{
			return true;
		}

		if (kTrigger.getNumFeaturesRequired() > 0)
		{
			return true;
		}

		if (kTrigger.getNumTerrainsRequired() > 0)
		{
			return true;
		}

		if (kTrigger.getNumImprovementsRequired() > 0)
		{
			return true;
		}

		if (kTrigger.getNumBonusesRequired() > 0)
		{
			return true;
		}

		if (kTrigger.getNumRoutesRequired() > 0)
		{
			return true;
		}

		if (kTrigger.isUnitsOnPlot() && kTrigger.getNumUnitsRequired() > 0)
		{
			return true;
		}

		if (kTrigger.isPrereqEventCity() && !kTrigger.isPickCity())
		{
			return true;
		}
	}

	return false;
}

TechTypes getDiscoveryTech(UnitTypes eUnit, PlayerTypes ePlayer)
{
	TechTypes eBestTech = NO_TECH;
	int iBestValue = 0;

	for (int iI = 0; iI < GC.getNumTechInfos(); iI++)
	{
		if (GET_PLAYER(ePlayer).canResearch((TechTypes)iI))
		{
			int iValue = 0;

			for (int iJ = 0; iJ < GC.getNumFlavorTypes(); iJ++)
			{
				iValue += (GC.getTechInfo((TechTypes) iI).getFlavorValue(iJ) * GC.getUnitInfo(eUnit).getFlavorValue(iJ));
			}

			if (iValue > iBestValue)
			{
				iBestValue = iValue;
				eBestTech = ((TechTypes)iI);
			}
		}
	}

	return eBestTech;
}


void setListHelp(wchar* szBuffer, const wchar* szStart, const wchar* szItem, const wchar* szSeparator, bool bFirst)
{
	if (bFirst)
	{
		wcscat(szBuffer, szStart);
	}
	else
	{
		wcscat(szBuffer, szSeparator);
	}

	wcscat(szBuffer, szItem);
}

void setListHelp(CvWString& szBuffer, const wchar* szStart, const wchar* szItem, const wchar* szSeparator, bool bFirst)
{
	if (bFirst)
	{
		szBuffer += szStart;
	}
	else
	{
		szBuffer += szSeparator;
	}

	szBuffer += szItem;
}

void setListHelp(CvWStringBuffer& szBuffer, const wchar* szStart, const wchar* szItem, const wchar* szSeparator, bool bFirst)
{
	if (bFirst)
	{
		szBuffer.append(szStart);
	}
	else
	{
		szBuffer.append(szSeparator);
	}

	szBuffer.append(szItem);
}

bool PUF_isGroupHead(const CvUnit* pUnit, int iData1, int iData2)
{
	return (pUnit->isGroupHead());
}

bool PUF_isPlayer(const CvUnit* pUnit, int iData1, int iData2)
{
	FAssertMsg(iData1 != -1, "Invalid data argument, should be >= 0");
	// <advc.061>
	TeamTypes eForTeam = (TeamTypes)iData2;
	PlayerTypes eOwner = (PlayerTypes)iData1;
	if(eForTeam == NO_TEAM || eOwner == NO_PLAYER || eForTeam == TEAMID(eOwner)) {
		// </advc.061>
		return (pUnit->getOwnerINLINE() == iData1);
	} // <advc.061>
	return (pUnit->getOwnerINLINE() == iData1 && !pUnit->isInvisible(eForTeam, false) &&
			!pUnit->getUnitInfo().isHiddenNationality()); // </advc.061>
}

bool PUF_isTeam(const CvUnit* pUnit, int iData1, int iData2)
{
	FAssertMsg(iData1 != -1, "Invalid data argument, should be >= 0");
	return (pUnit->getTeam() == iData1);
}

bool PUF_isCombatTeam(const CvUnit* pUnit, int iData1, int iData2)
{
	FAssertMsg(iData1 != -1, "Invalid data argument, should be >= 0");
	FAssertMsg(iData2 != -1, "Invalid data argument, should be >= 0");

	return (GET_PLAYER(pUnit->getCombatOwner((TeamTypes)iData2, pUnit->plot())).getTeam() == iData1 && !pUnit->isInvisible((TeamTypes)iData2, false, false));
}

bool PUF_isOtherPlayer(const CvUnit* pUnit, int iData1, int iData2)
{
	FAssertMsg(iData1 != -1, "Invalid data argument, should be >= 0");
	return (pUnit->getOwnerINLINE() != iData1);
}

bool PUF_isOtherTeam(const CvUnit* pUnit, int iData1, int iData2)
{
	FAssertMsg(iData1 != -1, "Invalid data argument, should be >= 0");
	TeamTypes eTeam = GET_PLAYER((PlayerTypes)iData1).getTeam();
	if (pUnit->canCoexistWithEnemyUnit(eTeam))
	{
		return false;
	}

	return (pUnit->getTeam() != eTeam);
}

bool PUF_isEnemy(const CvUnit* pUnit, int iData1, int iData2)
{
	FAssertMsg(iData1 != -1, "Invalid data argument, should be >= 0");
	FAssertMsg(iData2 != -1, "Invalid data argument, should be >= 0");

	TeamTypes eOtherTeam = GET_PLAYER((PlayerTypes)iData1).getTeam();
	TeamTypes eOurTeam = GET_PLAYER(pUnit->getCombatOwner(eOtherTeam, pUnit->plot())).getTeam();

	if (pUnit->canCoexistWithEnemyUnit(eOtherTeam))
	{
		return false;
	}

	return (iData2 ? eOtherTeam != eOurTeam : atWar(eOtherTeam, eOurTeam));
}
// <advc.122>
bool PUF_isEnemyCityAttacker(const CvUnit* pUnit, int iData1, int iData2) {

	CvUnitInfo& u = pUnit->getUnitInfo();
	if(u.getCargoSpace() <= 0 || u.getSpecialCargo() != NO_SPECIALUNIT) {
		if(u.getDomainType() != DOMAIN_LAND)
			return false;
		if(u.isOnlyDefensive() || u.getCombat() <= 0)
			return false;
	}
	return PUF_isEnemy(pUnit, iData1, iData2);
} // </advc.122>

bool PUF_isVisible(const CvUnit* pUnit, int iData1, int iData2)
{
	FAssertMsg(iData1 != -1, "Invalid data argument, should be >= 0");
	return !(pUnit->isInvisible(GET_PLAYER((PlayerTypes)iData1).getTeam(), false));
}

bool PUF_isVisibleDebug(const CvUnit* pUnit, int iData1, int iData2)
{
	FAssertMsg(iData1 != -1, "Invalid data argument, should be >= 0");
	return !(pUnit->isInvisible(GET_PLAYER((PlayerTypes)iData1).getTeam(), true));
}

bool PUF_canSiege(const CvUnit* pUnit, int iData1, int iData2)
{
	FAssertMsg(iData1 != -1, "Invalid data argument, should be >= 0");
	return pUnit->canSiege(GET_PLAYER((PlayerTypes)iData1).getTeam());
}

bool PUF_isPotentialEnemy(const CvUnit* pUnit, int iData1, int iData2)
{
	FAssertMsg(iData1 != -1, "Invalid data argument, should be >= 0");
	FAssertMsg(iData2 != -1, "Invalid data argument, should be >= 0");

	TeamTypes eOtherTeam = GET_PLAYER((PlayerTypes)iData1).getTeam();
	TeamTypes eOurTeam = GET_PLAYER(pUnit->getCombatOwner(eOtherTeam, pUnit->plot())).getTeam();

	if (pUnit->canCoexistWithEnemyUnit(eOtherTeam))
	{
		return false;
	}
	return (iData2 ? eOtherTeam != eOurTeam : isPotentialEnemy(eOtherTeam, eOurTeam));
}

bool PUF_canDeclareWar( const CvUnit* pUnit, int iData1, int iData2)
{
	FAssertMsg(iData1 != -1, "Invalid data argument, should be >= 0");
	FAssertMsg(iData2 != -1, "Invalid data argument, should be >= 0");

	TeamTypes eOtherTeam = GET_PLAYER((PlayerTypes)iData1).getTeam();
	TeamTypes eOurTeam = GET_PLAYER(pUnit->getCombatOwner(eOtherTeam, pUnit->plot())).getTeam();

	if (pUnit->canCoexistWithEnemyUnit(eOtherTeam))
	{
		return false;
	}

	return (iData2 ? false : GET_TEAM(eOtherTeam).canDeclareWar(eOurTeam));
}

bool PUF_canDefend(const CvUnit* pUnit, int iData1, int iData2)
{
	return pUnit->canDefend();
}

bool PUF_cannotDefend(const CvUnit* pUnit, int iData1, int iData2)
{
	return !(pUnit->canDefend());
}

bool PUF_canDefendGroupHead(const CvUnit* pUnit, int iData1, int iData2)
{
	return (PUF_canDefend(pUnit, iData1, iData2) && PUF_isGroupHead(pUnit, iData1, iData2));
}

bool PUF_canDefendEnemy(const CvUnit* pUnit, int iData1, int iData2)
{
	FAssertMsg(iData1 != -1, "Invalid data argument, should be >= 0");
	FAssertMsg(iData2 != -1, "Invalid data argument, should be >= 0");
	return (PUF_canDefend(pUnit, iData1, iData2) && PUF_isEnemy(pUnit, iData1, iData2));
}

bool PUF_canDefendPotentialEnemy(const CvUnit* pUnit, int iData1, int iData2)
{
	FAssertMsg(iData1 != -1, "Invalid data argument, should be >= 0");
	return (PUF_canDefend(pUnit, iData1, iData2) && PUF_isPotentialEnemy(pUnit, iData1, iData2));
}

bool PUF_canAirAttack(const CvUnit* pUnit, int iData1, int iData2)
{
	return pUnit->canAirAttack();
}

bool PUF_canAirDefend(const CvUnit* pUnit, int iData1, int iData2)
{
	return pUnit->canAirDefend();
}

bool PUF_isFighting(const CvUnit* pUnit, int iData1, int iData2)
{
	return pUnit->isFighting();
}

bool PUF_isAnimal( const CvUnit* pUnit, int iData1, int iData2)
{
	return pUnit->isAnimal();
}

bool PUF_isMilitaryHappiness(const CvUnit* pUnit, int iData1, int iData2)
{
	return pUnit->isMilitaryHappiness();
}

bool PUF_isInvestigate(const CvUnit* pUnit, int iData1, int iData2)
{	// <advc.103>
	if(pUnit->hasMoved())
		return false; // </advc.103>
	return pUnit->isInvestigate();
}

bool PUF_isCounterSpy(const CvUnit* pUnit, int iData1, int iData2)
{
	return pUnit->isCounterSpy();
}

bool PUF_isSpy(const CvUnit* pUnit, int iData1, int iData2)
{
	return pUnit->isSpy();
}

bool PUF_isDomainType(const CvUnit* pUnit, int iData1, int iData2)
{
	FAssertMsg(iData1 != -1, "Invalid data argument, should be >= 0");
	return (pUnit->getDomainType() == iData1);
}

bool PUF_isUnitType(const CvUnit* pUnit, int iData1, int iData2)
{
	FAssertMsg(iData1 != -1, "Invalid data argument, should be >= 0");
	return (pUnit->getUnitType() == iData1);
}

bool PUF_isUnitAIType(const CvUnit* pUnit, int iData1, int iData2)
{
	FAssertMsg(iData1 != -1, "Invalid data argument, should be >= 0");
	return (pUnit->AI_getUnitAIType() == iData1);
}

bool PUF_isCityAIType(const CvUnit* pUnit, int iData1, int iData2)
{
	return pUnit->AI_isCityAIType();
}

bool PUF_isNotCityAIType(const CvUnit* pUnit, int iData1, int iData2)
{
	return !(PUF_isCityAIType(pUnit, iData1, iData2));
}
// advc.003j (comment): unused
bool PUF_isSelected(const CvUnit* pUnit, int iData1, int iData2)
{
	return pUnit->IsSelected();
}

bool PUF_makeInfoBarDirty(CvUnit* pUnit, int iData1, int iData2)
{
	pUnit->setInfoBarDirty(true);
	return true;
}

bool PUF_isNoMission(const CvUnit* pUnit, int iData1, int iData2)
{
	/* original bts code
	return (pUnit->getGroup()->getActivityType() != ACTIVITY_MISSION); */
	// K-Mod
	return (pUnit->getGroup()->AI_getMissionAIType() == NO_MISSIONAI);
	// K-Mod end
}

bool PUF_isFiniteRange(const CvUnit* pUnit, int iData1, int iData2)
{
	return ((pUnit->getDomainType() != DOMAIN_AIR) || (pUnit->getUnitInfo().getAirRange() > 0));
}

/************************************************************************************************/
/* BETTER_BTS_AI_MOD                      01/15/09                                jdog5000      */
/*                                                                                              */
/* General AI                                                                                   */
/************************************************************************************************/
bool PUF_isAvailableUnitAITypeGroupie(const CvUnit* pUnit, int iData1, int iData2)
{
	return ((PUF_isUnitAITypeGroupie(pUnit,iData1,iData2)) && !(pUnit->isCargo()));
}

bool PUF_isUnitAITypeGroupie(const CvUnit* pUnit, int iData1, int iData2)
{
	CvUnit* pGroupHead = pUnit->getGroup()->getHeadUnit();
	return (PUF_isUnitAIType(pGroupHead,iData1,iData2));
}

bool PUF_isFiniteRangeAndNotJustProduced(const CvUnit* pUnit, int iData1, int iData2)
{
	return (PUF_isFiniteRange(pUnit,iData1,iData2) && ((GC.getGameINLINE().getGameTurn() - pUnit->getGameTurnCreated()) > 1));
}
/************************************************************************************************/
/* BETTER_BTS_AI_MOD                       END                                                  */
/************************************************************************************************/
// K-Mod
bool PUF_isMissionAIType(const CvUnit* pUnit, int iData1, int iData2)
{
	return pUnit->getGroup()->AI_getMissionAIType() == iData1;
}

bool PUF_isAirIntercept(const CvUnit* pUnit, int iData1, int iData2)
{
	return pUnit->getDomainType() == DOMAIN_AIR && pUnit->getGroup()->getActivityType() == ACTIVITY_INTERCEPT;
}
// K-Mod end

int potentialIrrigation(FAStarNode* parent, FAStarNode* node, int data, const void* pointer, FAStar* finder)
{
	if (parent == NULL)
	{
		return TRUE;
	}

	return ((GC.getMapINLINE().plotSorenINLINE(node->m_iX, node->m_iY)->isPotentialIrrigation()) ? TRUE : FALSE);
}


int checkFreshWater(FAStarNode* parent, FAStarNode* node, int data, const void* pointer, FAStar* finder)
{
	if (data == ASNL_ADDCLOSED)
	{
		if (GC.getMapINLINE().plotSorenINLINE(node->m_iX, node->m_iY)->isFreshWater())
		{
			*((bool *)pointer) = true;
		}
	}

	return 1;
}


int changeIrrigated(FAStarNode* parent, FAStarNode* node, int data, const void* pointer, FAStar* finder) 
{
	if (data == ASNL_ADDCLOSED)
	{
		GC.getMapINLINE().plotSorenINLINE(node->m_iX, node->m_iY)->setIrrigated(*((bool *)pointer));
	}

	return 1;
}

// (edited by K-Mod)
int pathDestValid(int iToX, int iToY, const void* pointer, FAStar* finder)
{
	PROFILE_FUNC();

	CvPlot* pToPlot = GC.getMapINLINE().plotSorenINLINE(iToX, iToY);
	FAssert(pToPlot);

	//pSelectionGroup = ((CvSelectionGroup *)pointer);
	// K-Mod
	CvSelectionGroup* pSelectionGroup = finder ? (CvSelectionGroup*)pointer : ((CvPathSettings*)pointer)->pGroup;
	int iFlags = finder ? gDLL->getFAStarIFace()->GetInfo(finder) : ((CvPathSettings*)pointer)->iFlags;
	// K-Mod end

	if (pSelectionGroup->atPlot(pToPlot))
		return TRUE;

	if (pSelectionGroup->getDomainType() == DOMAIN_IMMOBILE)
		return FALSE;

	bool bAIControl = pSelectionGroup->AI_isControlled();

	if (bAIControl)
	{
/************************************************************************************************/
/* BETTER_BTS_AI_MOD                      11/04/09                                jdog5000      */
/*                                                                                              */
/* Efficiency                                                                                   */
/************************************************************************************************/
		// BBAI efficiency: switch order, getPlotDanger is more expensive
		if (pSelectionGroup->getDomainType() == DOMAIN_LAND)
		{
			int iGroupAreaID = pSelectionGroup->getArea();
			if (pToPlot->getArea() != iGroupAreaID)
			{
				if( !(pSelectionGroup->canMoveAllTerrain()) )
				{
					if (!(pToPlot->isAdjacentToArea(iGroupAreaID)))
					{
						return FALSE;
					}
				}
			}
		}	

		if (!(iFlags & MOVE_IGNORE_DANGER))
		{
			if (!(pSelectionGroup->canFight()) && !(pSelectionGroup->alwaysInvisible()))
			{
				//if (GET_PLAYER(pSelectionGroup->getHeadOwner()).AI_getPlotDanger(pToPlot) > 0)
				if (GET_PLAYER(pSelectionGroup->getHeadOwner()).AI_getAnyPlotDanger(pToPlot))
				{
					return FALSE;
				}
			}
		}
/************************************************************************************************/
/* BETTER_BTS_AI_MOD                       END                                                  */
/************************************************************************************************/
	}

	if (bAIControl || pToPlot->isRevealed(pSelectionGroup->getHeadTeam(), false))
	{
		if (pSelectionGroup->isAmphibPlot(pToPlot))
		{
			CLLNode<IDInfo>* pUnitNode1 = pSelectionGroup->headUnitNode();

			while (pUnitNode1 != NULL)
			{
				CvUnit* pLoopUnit1 = ::getUnit(pUnitNode1->m_data);
				pUnitNode1 = pSelectionGroup->nextUnitNode(pUnitNode1);

				if ((pLoopUnit1->getCargo() > 0) && (pLoopUnit1->domainCargo() == DOMAIN_LAND))
				{
					bool bValid = false;

					CLLNode<IDInfo>* pUnitNode2 = pLoopUnit1->plot()->headUnitNode();

					while (pUnitNode2 != NULL)
					{
						CvUnit* pLoopUnit2 = ::getUnit(pUnitNode2->m_data);
						pUnitNode2 = pLoopUnit1->plot()->nextUnitNode(pUnitNode2);

						if (pLoopUnit2->getTransportUnit() == pLoopUnit1)
						{
							if (pLoopUnit2->isGroupHead())
							{
								//if (pLoopUnit2->getGroup()->canMoveOrAttackInto(pToPlot, (pSelectionGroup->AI_isDeclareWar(pToPlot) || (iFlags & MOVE_DECLARE_WAR))))
								if (pLoopUnit2->getGroup()->canMoveOrAttackInto(pToPlot, iFlags & MOVE_DECLARE_WAR, false, bAIControl)) // K-Mod. The new AI must be explicit about declaring war.
								{
									bValid = true;
									break;
								}
							}
						}
					}

					if (bValid)
					{
						return TRUE;
					}
				}
			}

			return FALSE;
		}
		else
		{
			//if (!(pSelectionGroup->canMoveOrAttackInto(pToPlot, (pSelectionGroup->AI_isDeclareWar(pToPlot) || (iFlags & MOVE_DECLARE_WAR)))))
			if (!pSelectionGroup->canMoveOrAttackInto(pToPlot, iFlags & MOVE_DECLARE_WAR, false, bAIControl)) // K-Mod. The new AI must be explicit about declaring war.
			{
				return FALSE;
			}
		}
	}

	return TRUE;
}


int pathHeuristic(int iFromX, int iFromY, int iToX, int iToY)
{
	return (stepDistance(iFromX, iFromY, iToX, iToY) * PATH_MOVEMENT_WEIGHT);
}

// This function has been completely rewritten for K-Mod. (the rewrite includes some bug fixes as well as some new features)
int pathCost(FAStarNode* parent, FAStarNode* node, int data, const void* pointer, FAStar* finder)
{
	PROFILE_FUNC();

	CvPlot* pFromPlot = GC.getMapINLINE().plotSorenINLINE(parent->m_iX, parent->m_iY);
	CvPlot* pToPlot = GC.getMapINLINE().plotSorenINLINE(node->m_iX, node->m_iY);

	FAssert(pFromPlot != NULL);
	FAssert(pToPlot != NULL);

	//CvSelectionGroup* pSelectionGroup = ((CvSelectionGroup *)pointer);
	// K-Mod
	CvSelectionGroup* pSelectionGroup = finder ? (CvSelectionGroup*)pointer : ((CvPathSettings*)pointer)->pGroup;
	int iFlags = finder ? gDLL->getFAStarIFace()->GetInfo(finder) : ((CvPathSettings*)pointer)->iFlags;
	// K-Mod end


	int iWorstCost = 0;
	int iWorstMovesLeft = MAX_INT;
	int iWorstMaxMoves = MAX_INT;

	TeamTypes eTeam = pSelectionGroup->getHeadTeam();
	// <advc.035>
	int const iFlipModifierDiv = 7;
	int iFlipModifier = iFlipModifierDiv;
	// </advc.035>
	// K-Mod
	int iExploreModifier = 3; // (in thirds)
	if (!pToPlot->isRevealed(eTeam, false))
	{
		if (pSelectionGroup->getAutomateType() == AUTOMATE_EXPLORE ||
			(!pSelectionGroup->isHuman() && (pSelectionGroup->getHeadUnitAI() == UNITAI_EXPLORE || pSelectionGroup->getHeadUnitAI() == UNITAI_EXPLORE_SEA)))
		{
			iExploreModifier = 2; // lower cost to encourage exploring unrevealed areas
		}
		else if (!pFromPlot->isRevealed(eTeam, false))
		{
			iExploreModifier = 4; // higher cost to discourage pathfinding deep into the unknown
		}
	}
	// K-Mod end
	// <advc.035>
	else if(GC.getOWN_EXCLUSIVE_RADIUS() > 0 && (iFlags & MOVE_DECLARE_WAR) &&
			eTeam != BARBARIAN_TEAM) {
		PlayerTypes const secondOwner = pToPlot->getSecondOwner();
		PlayerTypes const firstOwner = pToPlot->getOwnerINLINE();
		if(secondOwner != NO_PLAYER && firstOwner != NO_PLAYER &&
				((pSelectionGroup->getDomainType() == DOMAIN_SEA) == pToPlot->isWater())) {
			// Avoid tiles that flip from us to the enemy upon DoW
			if(TEAMID(firstOwner) == eTeam && (GET_TEAM(eTeam).isHuman() ?
					(!GET_TEAM(eTeam).isFriendlyTerritory(TEAMID(secondOwner)) &&
					!GET_TEAM(eTeam).isAtWar(TEAMID(secondOwner))) :
					GET_TEAM(eTeam).AI_isSneakAttackReady(TEAMID(secondOwner))))
				iFlipModifier++;
			// Seek out enemy tiles that will flip to us upon DoW
			if(TEAMID(secondOwner) == eTeam && (GET_TEAM(eTeam).isHuman() ?
					(!GET_TEAM(eTeam).isFriendlyTerritory(TEAMID(firstOwner)) &&
					!GET_TEAM(eTeam).isAtWar(TEAMID(firstOwner))) :
					GET_TEAM(eTeam).AI_isSneakAttackReady(TEAMID(firstOwner))))
				iFlipModifier--;
			/*  This could be done much more accurately, taking into account
				vassal agreements, defensive pacts, and going through the entire
				selection group, but I worry about the performance, and it's OK
				if it doesn't always work correctly. */
		}
	} // </advc.035>
	{
		CLLNode<IDInfo>* pUnitNode = pSelectionGroup->headUnitNode();
		while (pUnitNode != NULL)
		{
			CvUnit* pLoopUnit = ::getUnit(pUnitNode->m_data);
			pUnitNode = pSelectionGroup->nextUnitNode(pUnitNode);
			FAssert(pLoopUnit->getDomainType() != DOMAIN_AIR);

			int iMaxMoves = parent->m_iData1 > 0 ? parent->m_iData1 : pLoopUnit->maxMoves();
			int iMoveCost = pToPlot->movementCost(pLoopUnit, pFromPlot,
					false); // advc.001i
			int iMovesLeft = std::max(0, (iMaxMoves - iMoveCost));

			iWorstMovesLeft = std::min(iWorstMovesLeft, iMovesLeft);
			iWorstMaxMoves = std::min(iWorstMaxMoves, iMaxMoves);

			int iCost = PATH_MOVEMENT_WEIGHT * (iMovesLeft == 0 ? iMaxMoves : iMoveCost);
			iCost = (iCost * iExploreModifier) / 3;
			iCost = (iCost * iFlipModifier) / iFlipModifierDiv; // advc.035
			if (iCost > iWorstCost)
			{
				iWorstCost = iCost;
				iWorstMovesLeft = iMovesLeft;
				iWorstMaxMoves = iMaxMoves;
			}
		}
	}

	iWorstCost += PATH_STEP_WEIGHT;

	// symmetry breaking. This is meant to prevent two paths from having equal cost.
	// (If two paths have equal cost, sometimes the interface shows one path and the units follow the other. This is bad.)
	/* original K-Mod symmetry breaking. (extra cost for turning a corner)
	if (parent->m_pParent)
	{
		const int map_width = GC.getMapINLINE().getGridWidthINLINE();
		const int map_height = GC.getMapINLINE().getGridHeightINLINE();

#define WRAP_X(x) ((x) - ((x) > map_width/2 ? map_width : 0) + ((x) < -map_width/2 ? map_width : 0))
#define WRAP_Y(y) ((y) - ((y) > map_height/2 ? map_height : 0) + ((y) < -map_height/2 ? map_height : 0))

		int start_x = parent->m_pParent->m_iX;
		int start_y = parent->m_pParent->m_iY;

		int dx1 = WRAP_X(pFromPlot->getX_INLINE() - start_x);
		int dy1 = WRAP_Y(pFromPlot->getY_INLINE() - start_y);
		int dx2 = WRAP_X(pToPlot->getX_INLINE() - start_x);
		int dy2 = WRAP_Y(pToPlot->getY_INLINE() - start_y);

		// cross product. (greater than zero => sin(angle) > 0 => angle > 0)
		int cross = dx1 * dy2 - dx2 * dy1;
		if (cross > 0)
			iWorstCost += PATH_ASYMMETRY_WEIGHT; // turning left
		else if (cross < 0)
			iWorstCost -= PATH_ASYMMETRY_WEIGHT; // turning right

		// woah - hang on. Does that say /minus/ asym weight?
		// Doesn't this guy know that bad things happen if the total weight is negative?
		// ...
		// take a breath.
#if PATH_STEP_WEIGHT < PATH_ASYMMETRY_WEIGHT
#error "I'm sorry, but I must demand that step weight be greater than asym weight."
#endif
		// I think we're going to be ok.

#undef WRAP_X
#undef WRAP_Y
	} */

	// Unfortunately, the above code is not sufficient to fix the symmetry problem.
	// Here's a new method, which combines symmetry breaking with the old "straight path" effect.
	// Note: symmetry breaking is not important for AI controlled units.

	// It's actually marginally better strategy to move diagonally - for mapping reasons.
	// So let the AI prefer diagonal movement.
	// However, diagonal zig-zags will probably seem unnatural and weird to humans who are just trying to move in a straight line.
	// So let the pathfinding for human groups prefer cardinal movement.
	if (pSelectionGroup->AI_isControlled())
	{
		if (pFromPlot->getX_INLINE() == pToPlot->getX_INLINE() || pFromPlot->getY_INLINE() == pToPlot->getY_INLINE())
			iWorstCost += PATH_STRAIGHT_WEIGHT;
	}
	else
	{
		if ((pFromPlot->getX_INLINE() != pToPlot->getX_INLINE()) && (pFromPlot->getY_INLINE() != pToPlot->getY_INLINE()))
			iWorstCost += PATH_STRAIGHT_WEIGHT * (1+(node->m_iX + node->m_iY)%2);
		iWorstCost += (node->m_iX + node->m_iY+1)%3;
	}
	// unfortunately, this simple method may have problems at the world-wrap boundries.
	// It's difficult to tell when to correct for wrap effects and when not to, because as soon as the
	// unit starts moving, the start position of the path changes, and so it's no longer posible to tell
	// whether or not the unit started on the other side of the boundry.  Drat.

	// end symmetry breaking.

	if (!pToPlot->isRevealed(eTeam, false))
		return iWorstCost;

	// the cost of battle...
	if (iFlags & MOVE_ATTACK_STACK)
	{
		FAssert(pSelectionGroup->AI_isControlled()); // only the AI uses MOVE_ATTACK_STACK
		FAssert(pSelectionGroup->getDomainType() == DOMAIN_LAND);

		int iEnemyDefence = 0;

		if (pToPlot->isVisible(eTeam, false))
		{
			/*  <advc.001> In the rare case that the AI plans war while animals
				still roam the map, the DefenceStrength computation will crash
				when it gets to the point where the UnitCombatType is accessed.
				(Actually, not so exotic b/c I'm allowing animals to survive
				in continents w/o civ cities (advc.300). */
			CvUnit* up = pToPlot->getUnitByIndex(0);
			if(up != NULL && !up->isAnimal()) { // </advc.001>
				iEnemyDefence = GET_PLAYER(pSelectionGroup->getOwnerINLINE()).
						AI_localDefenceStrength(pToPlot, NO_TEAM,
						pSelectionGroup->getDomainType(), 0, true, false,
						pSelectionGroup->isHuman());
			}
		}
		else
		{
			// plot not visible. use memory
			iEnemyDefence = GET_TEAM(eTeam).AI_getStrengthMemory(pToPlot->getX_INLINE(), pToPlot->getY_INLINE());
		}

		if (iEnemyDefence > 0)
		{
			iWorstCost += PATH_COMBAT_WEIGHT;
			int iAttackRatio = std::max(10, 100 * pSelectionGroup->AI_sumStrength(pToPlot) / iEnemyDefence);
			// Note. I half intend to have pathValid return false whenever the above ratio is less than 100.
			// I just haven't done that yet, mostly because I'm worried about performance.
			if (iAttackRatio < 400)
			{
				iWorstCost += PATH_MOVEMENT_WEIGHT * GC.getMOVE_DENOMINATOR() * (400-iAttackRatio)/std::min(150, iAttackRatio);
			}
			// else, don't worry about it too much.
		}
	}
	//

	if (iWorstMovesLeft <= 0)
	{
		if (pToPlot->getTeam() != eTeam)
		{
			iWorstCost += PATH_TERRITORY_WEIGHT;
		}

		// Damage caused by features (for mods)
		if (0 != GC.getPATH_DAMAGE_WEIGHT())
		{
			if (pToPlot->getFeatureType() != NO_FEATURE)
			{
				iWorstCost += (GC.getPATH_DAMAGE_WEIGHT() * std::max(0, GC.getFeatureInfo(pToPlot->getFeatureType()).getTurnDamage())) / GC.getMAX_HIT_POINTS();
			}

			if (pToPlot->getExtraMovePathCost() > 0)
			{
				iWorstCost += (PATH_MOVEMENT_WEIGHT * pToPlot->getExtraMovePathCost());
			}
		}

		// defence modifiers
		CLLNode<IDInfo>* pUnitNode = pSelectionGroup->headUnitNode();
		int iDefenceMod = 0;
		int iDefenceCount = 0;
		int iFromDefenceMod = 0; // defence bonus for our attacking units left behind
		int iAttackWeight = 0;
		int iAttackCount = 0;
		int iEnemies = pToPlot->getNumVisibleEnemyDefenders(pSelectionGroup->getHeadUnit());

		while (pUnitNode != NULL)
		{
			CvUnit* pLoopUnit = ::getUnit(pUnitNode->m_data);
			pUnitNode = pSelectionGroup->nextUnitNode(pUnitNode);

			if (pLoopUnit->canFight())
			{
				iDefenceCount++;
				if (pLoopUnit->canDefend(pToPlot))
					iDefenceMod += pLoopUnit->noDefensiveBonus() ? 0 :
							// advc.012:
							GET_TEAM(eTeam).AI_plotDefense(*pToPlot);
							//pToPlot->defenseModifier(eTeam, false);
				else
					iDefenceMod -= 100; // we don't want to be here.

				// K-Mod note. the above code doesn't count all defensive bonuses, unfortunately.
				// We could count everything like this:
				/*
				CombatDetails combat_details;
				pLoopUnit->maxCombatStr(pToPlot, NULL, &combat_details);
				iDefenceMod += combat_details.iModifierTotal;
				*/
				// but that seems like overkill. I'm worried it would be too slow.

				// defence for units who stay behind after attacking an enemy.
				if (iEnemies > 0)
				{
					// For human-controlled units, only apply the following effects for multi-step moves.
					// (otherwise this might prevent the user from attacking from where they want to attack from.)
					if (pSelectionGroup->AI_isControlled() || parent->m_iKnownCost != 0 || iFlags & MOVE_HAS_STEPPED)
					{
						iAttackCount++;
						iFromDefenceMod += pLoopUnit->noDefensiveBonus() ? 0 : pFromPlot->defenseModifier(eTeam, false);

						if (!pFromPlot->isCity())
						{
							iAttackWeight += PATH_CITY_WEIGHT;
							// it's done this way around rather than subtracting when in a city so that the overall adjustment can't be negative.
						}

						if (pLoopUnit->canAttack() && !pLoopUnit->isRiver() && pFromPlot->isRiverCrossing(directionXY(pFromPlot, pToPlot)))
						{
							iAttackWeight -= PATH_RIVER_WEIGHT * GC.getRIVER_ATTACK_MODIFIER(); // Note, river modifier will be negative.
							//iAttackMod -= (PATH_MOVEMENT_WEIGHT * iMovesLeft);
						}
					}
					// If this is a direct attack move from a human player, make sure it is the best value move possible. (This allows humans to choose which plot they attack from.)
					// (note: humans have no way of ordering units to attack units en-route, so the fact that this is an attack move means we are at the destination.)
					else if (pLoopUnit->canAttack()) // parent->m_iKnownCost == 0 && !(iFlags & MOVE_HAS_STEPPED) && !pSelectionGroup->AI_isControlled()
						return PATH_STEP_WEIGHT; // DONE!
				}
			}
		}
		//
		if (iAttackCount > 0)
		{
			// scale attack weights down if not all our units will need to fight.
			iAttackWeight *= std::min(iAttackCount, iEnemies);
			iAttackWeight /= iAttackCount;
			iFromDefenceMod *= std::min(iAttackCount, iEnemies);
			iFromDefenceMod /= iAttackCount;
			iAttackCount = std::min(iAttackCount, iEnemies);
		}
		//
		iWorstCost += PATH_DEFENSE_WEIGHT * std::max(0, (iDefenceCount*200 - iDefenceMod) / std::max(1, iDefenceCount));
		iWorstCost += PATH_DEFENSE_WEIGHT * std::max(0, (iAttackCount*200 - iFromDefenceMod) / std::max(1, iAttackCount));
		iWorstCost += std::max(0, iAttackWeight) / std::max(1, iAttackCount);
		// if we're in enemy territory, consider the sum of our defensive bonuses as well as the average
		if (pToPlot->isOwned() && atWar(pToPlot->getTeam(), eTeam))
		{
			iWorstCost += PATH_DEFENSE_WEIGHT * std::max(0, (iDefenceCount*200 - iDefenceMod)/5);
			iWorstCost += PATH_DEFENSE_WEIGHT * std::max(0, (iAttackCount*200 - iFromDefenceMod)/5);
			iWorstCost += std::max(0, iAttackWeight) / 5;
		}

		// additional cost for ending turn in or adjacent to enemy territory based on flags (based on BBAI)
		if (iFlags & (MOVE_AVOID_ENEMY_WEIGHT_2 | MOVE_AVOID_ENEMY_WEIGHT_3))
		{
			if (pToPlot->isOwned() && GET_TEAM(eTeam).AI_getWarPlan(pToPlot->getTeam()) != NO_WARPLAN)
			{
				iWorstCost *= (iFlags & MOVE_AVOID_ENEMY_WEIGHT_3) ? 3 : 2;
			}
			else
			{
				CvPlot* pAdjacentPlot;
				for (int iI = 0; iI < NUM_DIRECTION_TYPES; iI++)
				{
					pAdjacentPlot = plotDirection(pToPlot->getX_INLINE(), pToPlot->getY_INLINE(), ((DirectionTypes)iI));

					if( pAdjacentPlot != NULL )
					{
						if (pAdjacentPlot->isOwned() && atWar(pAdjacentPlot->getTeam(), pSelectionGroup->getHeadTeam()))
						{
							if (iFlags & MOVE_AVOID_ENEMY_WEIGHT_3)
							{
								iWorstCost *= 3;
								iWorstCost /= 2;
							}
							else
							{
								iWorstCost *= 4;
								iWorstCost /= 3;
							}
						}
					}
				}
			}
		}
	}

	FAssert(iWorstCost > 0);

	return iWorstCost;
}

int pathValid_join(FAStarNode* parent, FAStarNode* node, CvSelectionGroup* pSelectionGroup, int iFlags)
{
	CvPlot* pFromPlot = GC.getMapINLINE().plotSorenINLINE(parent->m_iX, parent->m_iY);
	CvPlot* pToPlot = GC.getMapINLINE().plotSorenINLINE(node->m_iX, node->m_iY);

	FAssert(pFromPlot != NULL);
	FAssert(pToPlot != NULL);

	if (pSelectionGroup->getDomainType() == DOMAIN_SEA)
	{
		if (pFromPlot->isWater() && pToPlot->isWater())
		{
			if (!(GC.getMapINLINE().plotINLINE(pFromPlot->getX_INLINE(), pToPlot->getY_INLINE())->isWater()) && !(GC.getMapINLINE().plotINLINE(pToPlot->getX_INLINE(), pFromPlot->getY_INLINE())->isWater()))
			{
				if( !(pSelectionGroup->canMoveAllTerrain()) )
				{
					return FALSE;
				}
			}
		}
	}
	return TRUE;
}

int pathValid_source(FAStarNode* parent, CvSelectionGroup* pSelectionGroup, int iFlags)
{
	PROFILE_FUNC();
	CvPlot* pFromPlot = GC.getMapINLINE().plotSorenINLINE(parent->m_iX, parent->m_iY);
	//CvPlot* pToPlot = GC.getMapINLINE().plotSorenINLINE(node->m_iX, node->m_iY);

	if (pSelectionGroup->atPlot(pFromPlot))
	{
		return TRUE;
	}

	if (iFlags & MOVE_SAFE_TERRITORY)
	{
		if (pFromPlot->isOwned())
		{
			if (pFromPlot->getTeam() != pSelectionGroup->getHeadTeam())
			{
				return FALSE;
			}
		}

		if (!(pFromPlot->isRevealed(pSelectionGroup->getHeadTeam(), false)))
		{
			return FALSE;
		}
	}
	// <advc.049> No new AI routes in human territory (but upgrade to railroad OK)
	if(iFlags & MOVE_ROUTE_TO) {
		if(pFromPlot->getRevealedRouteType(pSelectionGroup->getHeadTeam(), false) ==
			NO_ROUTE && !pSelectionGroup->isHuman()) {
			PlayerTypes eOwner = pFromPlot->getOwnerINLINE();
			if(eOwner != NO_PLAYER && GET_PLAYER(eOwner).isHuman())
				return FALSE;
		}
	} // </advc.049>

	if (iFlags & MOVE_NO_ENEMY_TERRITORY)
	{
		if (pFromPlot->isOwned())
		{
			if (atWar(pFromPlot->getTeam(), pSelectionGroup->getHeadTeam()))
			{
				return FALSE;
			}
		}
	}

	bool bAIControl = pSelectionGroup->AI_isControlled();

	if (bAIControl)
	{
		if ((parent->m_iData2 > 1) || (parent->m_iData1 == 0))
		{
			if (!(iFlags & MOVE_IGNORE_DANGER))
			{
				if (!(pSelectionGroup->canFight()) && !(pSelectionGroup->alwaysInvisible()))
				{
					if (GET_PLAYER(pSelectionGroup->getHeadOwner()).AI_getAnyPlotDanger(pFromPlot))
					{
						return FALSE;
					}
				}
			}
		}
	}

	if (bAIControl || pFromPlot->isRevealed(pSelectionGroup->getHeadTeam(), false))
	{
		//if (iFlags & MOVE_THROUGH_ENEMY)
		if (iFlags & (MOVE_THROUGH_ENEMY | MOVE_ATTACK_STACK)) // K-Mod
		{
			//if (!(pSelectionGroup->canMoveOrAttackInto(pFromPlot)))
			if (!pSelectionGroup->canMoveOrAttackInto(pFromPlot, iFlags & MOVE_DECLARE_WAR && !pSelectionGroup->isHuman())) // K-Mod
			{
				return FALSE;
			}
		}
		else
		{
			//if (!(pSelectionGroup->canMoveThrough(pFromPlot)))
			if (!pSelectionGroup->canMoveThrough(pFromPlot, iFlags & MOVE_DECLARE_WAR && !pSelectionGroup->isHuman(), iFlags & MOVE_ASSUME_VISIBLE || !pSelectionGroup->isHuman())) // K-Mod
			{
				return FALSE;
			}
		}
	}
	// K-Mod. Note: it's currently difficult to extract the vision-cheating part of this AI,
	// because the AI needs to cheat inside canMoveOrAttackInto for its other cheating parts to work...
	//  .. anyway, here is the beginnings of what the code might look like without the cheats. (it's unfinished)
#if 0
	if (pFromPlot->isRevealed(pSelectionGroup->getHeadTeam(), false))
	{
		PROFILE("pathValid move through");
		CvTeamAI& kTeam = GET_TEAM(pSelectionGroup->getHeadTeam());

		int iEnemyDefence;
		if (pFromPlot->isVisible(pSelectionGroup->getHeadTeam(), false))
		{
			iEnemyDefence = GET_PLAYER(pSelectionGroup->getOwnerINLINE()).AI_localDefenceStrength(pToPlot, NO_TEAM, pSelectionGroup->getDomainType(), 0, true, false, pSelectionGroup->isHuman());
		}
		else
		{
			iEnemyDefence = kTeam.AI_getStrengthMemory(pFromPlot);
		}

		if (kTeam.AI_getStrengthMemory(pFromPlot) > 0 && iFlags & (MOVE_THROUGH_ENEMY | MOVE_ATTACK_STACK))
		{
			if (!pSelectionGroup->canMoveOrAttackInto(pFromPlot) ||
				(iFlags & MOVE_ATTACK_STACK && pSelectionGroup->AI_sumStrength(pFromPlot) < iEnemyDefence))
			{
				return FALSE;
			}
		}
		else
		{
			if (!pSelectionGroup->canMoveThrough(pFromPlot))
			{
				return FALSE;
			}
		}
	}
#endif
	// K-Mod end

	return TRUE;
}


int pathValid(FAStarNode* parent, FAStarNode* node, int data, const void* pointer, FAStar* finder)
{
	PROFILE_FUNC();

	if(parent == NULL)
		return TRUE;
	// <advc.003> Was unused apart from the assert
	/*CvPlot* pFromPlot = ...;
	CvPlot* pToPlot = ...; */
	FAssert(GC.getMapINLINE().plotSorenINLINE(parent->m_iX, parent->m_iY) != NULL);
	FAssert(GC.getMapINLINE().plotSorenINLINE(node->m_iX, node->m_iY) != NULL);
	// </advc.003>
	//pSelectionGroup = ((CvSelectionGroup *)pointer);
	// K-Mod
	CvSelectionGroup* pSelectionGroup = finder ? (CvSelectionGroup*)pointer :
			((CvPathSettings*)pointer)->pGroup;
	int iFlags = finder ? gDLL->getFAStarIFace()->GetInfo(finder) :
			((CvPathSettings*)pointer)->iFlags;
	// K-Mod end

	if (!pathValid_join(parent, node, pSelectionGroup, iFlags))
		return FALSE;

	//bResult = pathValidInternal(parent, node, data, pPathSettings, finder);
	return pathValid_source(parent, pSelectionGroup, iFlags);
}


int pathAdd(FAStarNode* parent, FAStarNode* node, int data, const void* pointer, FAStar* finder)
{
	PROFILE_FUNC();

	//CvSelectionGroup* pSelectionGroup = ((CvSelectionGroup *)pointer);
	// K-Mod
	CvSelectionGroup* pSelectionGroup = finder ? (CvSelectionGroup*)pointer : ((CvPathSettings*)pointer)->pGroup;
	int iFlags = finder ? gDLL->getFAStarIFace()->GetInfo(finder) : ((CvPathSettings*)pointer)->iFlags;
	// K-Mod end
	FAssert(pSelectionGroup->getNumUnits() > 0);

	int iTurns = 1;
	int iMoves = MAX_INT;

	if (data == ASNC_INITIALADD)
	{
		bool bMaxMoves = (iFlags & MOVE_MAX_MOVES);
		// K-Mod. I've moved the code from here into separate functions.
		iMoves = bMaxMoves ? pSelectionGroup->maxMoves() : pSelectionGroup->movesLeft();
		// K-Mod end
	}
	else
	{
		CvPlot* pFromPlot = GC.getMapINLINE().plotSorenINLINE(parent->m_iX, parent->m_iY);
		FAssertMsg(pFromPlot != NULL, "FromPlot is not assigned a valid value");
		CvPlot* pToPlot = GC.getMapINLINE().plotSorenINLINE(node->m_iX, node->m_iY);
		FAssertMsg(pToPlot != NULL, "ToPlot is not assigned a valid value");

		int iStartMoves = parent->m_iData1;
		iTurns = parent->m_iData2;
		/* original bts code
		if (iStartMoves == 0)
		{
			iTurns++;
		}

		for (CLLNode<IDInfo>* pUnitNode = pSelectionGroup->headUnitNode(); pUnitNode != NULL; pUnitNode = pSelectionGroup->nextUnitNode(pUnitNode))
		{
			CvUnit* pLoopUnit = ::getUnit(pUnitNode->m_data);

			int iUnitMoves = (iStartMoves == 0 ? pLoopUnit->maxMoves() : iStartMoves);
			iUnitMoves -= pToPlot->movementCost(pLoopUnit, pFromPlot);
			iUnitMoves = std::max(iUnitMoves, 0);
			
			iMoves = std::min(iMoves, iUnitMoves);
		} */
		// K-Mod. The original code would give incorrect results for groups where one unit had more moves but also had higher move cost.
		// (eg. the most obvious example is when a group with 1-move units and 2-move units is moving on a railroad. - In this situation,
		//  the original code would consistently underestimate the remaining moves at every step.)
		bool bNewTurn = iStartMoves == 0;

		if (bNewTurn)
		{
			iTurns++;
			iStartMoves = pSelectionGroup->maxMoves();
		}

		CLLNode<IDInfo>* pUnitNode = pSelectionGroup->headUnitNode();
		int iMoveCost = pToPlot->movementCost(::getUnit(pUnitNode->m_data), pFromPlot,
				false); // advc.001i
		bool bUniformCost = true;

		for (pUnitNode = pSelectionGroup->nextUnitNode(pUnitNode); pUnitNode && bUniformCost; pUnitNode = pSelectionGroup->nextUnitNode(pUnitNode))
		{
			CvUnit* pLoopUnit = ::getUnit(pUnitNode->m_data);

			int iLoopCost = pToPlot->movementCost(pLoopUnit, pFromPlot,
					false); // advc.001i
			if (iLoopCost != iMoveCost)
			{
				bUniformCost = false;
			}
		}

		if (bUniformCost)
		{
			// the simple, normal case
			iMoves = std::max(0, iStartMoves - iMoveCost);
		}
		else
		{
			PROFILE("pathAdd - non-uniform cost");
			// Move costs are uneven for units in this group.
			// To be sure of the true movement cost for the group, we need to calculate the movement cost for each unit for every step in this turn.
			std::vector<const CvPlot*> plot_list; // will be traversed in reverse order
			plot_list.push_back(pToPlot);
			plot_list.push_back(pFromPlot);
			FAStarNode* pStartNode = parent;
			while (pStartNode->m_iData2 == iTurns && pStartNode->m_pParent)
			{
				pStartNode = pStartNode->m_pParent;
				plot_list.push_back(GC.getMapINLINE().plotSorenINLINE(pStartNode->m_iX, pStartNode->m_iY));
			}
			iMoves = INT_MAX;
			bool bMaxMoves = pStartNode->m_iData1 == 0 || iFlags & MOVE_MAX_MOVES;

			for (pUnitNode = pSelectionGroup->headUnitNode(); pUnitNode != NULL; pUnitNode = pSelectionGroup->nextUnitNode(pUnitNode))
			{
				CvUnit* pLoopUnit = ::getUnit(pUnitNode->m_data);

				int iUnitMoves = bMaxMoves ? pLoopUnit->maxMoves() : pLoopUnit->movesLeft();
				for (size_t i = plot_list.size()-1; i > 0; i--)
				{
					iUnitMoves -= plot_list[i-1]->movementCost(pLoopUnit, plot_list[i],
							false); // advc.001i
					FAssert(iUnitMoves > 0 || i == 1);
				}

				iUnitMoves = std::max(iUnitMoves, 0);
				iMoves = std::min(iMoves, iUnitMoves);
			}
		}
		// K-Mod end
	}

	FAssertMsg(iMoves >= 0, "iMoves is expected to be non-negative (invalid Index)");

	node->m_iData1 = iMoves;
	node->m_iData2 = iTurns;

	return 1;
}


int stepDestValid(int iToX, int iToY, const void* pointer, FAStar* finder)
{
	PROFILE_FUNC();

	CvPlot* pFromPlot;
	CvPlot* pToPlot;

	pFromPlot = GC.getMapINLINE().plotSorenINLINE(gDLL->getFAStarIFace()->GetStartX(finder), gDLL->getFAStarIFace()->GetStartY(finder));
	FAssert(pFromPlot != NULL);
	pToPlot = GC.getMapINLINE().plotSorenINLINE(iToX, iToY);
	FAssert(pToPlot != NULL);

	if (pFromPlot->area() != pToPlot->area())
	{
		return FALSE;
	}

	return TRUE;
}

// <advc.104b> Rule out (basically) no destinations; let teamStepValid_advc decide
int stepDestValid_advc(int iToX, int iToY, const void* pointer, FAStar* finder) {

	CvPlot* pToPlot = GC.getMapINLINE().plotSorenINLINE(iToX, iToY);
	if(pToPlot == NULL || pToPlot->isImpassable())
		return FALSE;
	return TRUE;
}

// Can handle sea paths. Based on teamStepValid.
int teamStepValid_advc(FAStarNode* parent, FAStarNode* node, int data,
		const void* pointer, FAStar* finder) {

	if(parent == NULL)
		return TRUE;
	CvMap const& m = GC.getMapINLINE();
	CvPlot* pNewPlot = m.plotSorenINLINE(node->m_iX, node->m_iY);
	if(pNewPlot->isImpassable())
		return FALSE;
	CvPlot* pFromPlot = m.plotSorenINLINE(parent->m_iX, parent->m_iY);
	if(pFromPlot->isWater() && pNewPlot->isWater() &&
			!m.plotINLINE(parent->m_iX, node->m_iY)->isWater() &&
			!m.plotINLINE(node->m_iX, parent->m_iY)->isWater())
		return FALSE;
	TeamTypes ePlotTeam = pNewPlot->getTeam();
	int* v = (int*)pointer;
	int iMaxPath = v[5];
	/*  As far as I understand the code, node (the pToPlot) is still set to 0
		cost if it's visited for the first time, so we should look at parent
		(pFromPlot) when enforcing the upper bound (iMaxPath). But it doesn't
		hurt to check node's cost too. */
	if(iMaxPath > 0 && (parent->m_iHeuristicCost + parent->m_iKnownCost > iMaxPath ||
			node->m_iHeuristicCost + node->m_iKnownCost > iMaxPath))
		return FALSE;
	TeamTypes eTeam = (TeamTypes)v[0];
	TeamTypes eTargetTeam = (TeamTypes)v[1];
	DomainTypes dom = (DomainTypes)v[2];
	if(dom == NO_DOMAIN)
		dom = DOMAIN_LAND;
	// Check domain legality:
	if(dom == DOMAIN_LAND && pNewPlot->isWater())
		return FALSE;
	bool coastalCity = pNewPlot->isCity(true) && pNewPlot->isCoastalLand();
	// Use DOMAIN_IMMOBILE to encode sea units with impassable terrain
	bool impassableTerrain = false;
	if(dom == DOMAIN_IMMOBILE) {
		impassableTerrain = true;
		dom = DOMAIN_SEA;
	}
	if(dom == DOMAIN_SEA && !coastalCity && !pNewPlot->isWater() &&
			// Allow non-city land tile as cargo destination
			(pNewPlot->getX_INLINE() != v[3] || pNewPlot->getY_INLINE() != v[4]))
		return FALSE;
	/*  This handles only Coast, and no other terrain types that a mod might make
		impassable */
	if(!coastalCity && ePlotTeam != eTeam && impassableTerrain &&
			pNewPlot->getTerrainType() != (TerrainTypes)(GC.getDefineINT("SHALLOW_WATER_TERRAIN")))
		return FALSE;
	// Don't check isRevealed; caller ensures that destination city is deducible
	if(ePlotTeam == NO_TEAM)
		return TRUE;
	if(GET_TEAM(ePlotTeam).getMasterTeam() == GET_TEAM(eTargetTeam).getMasterTeam())
		return TRUE;
	CvTeamAI const& kTeam = GET_TEAM(eTeam);
	if(kTeam.isFriendlyTerritory(ePlotTeam) ||
			/*  A war plan isn't enough; war against eTargetTeam could replace
				that plan. */
			kTeam.isAtWar(ePlotTeam) || kTeam.isOpenBorders(ePlotTeam))
		return TRUE;
	return FALSE;
}
// </advc.104b>

int stepHeuristic(int iFromX, int iFromY, int iToX, int iToY)
{
	return stepDistance(iFromX, iFromY, iToX, iToY);
}


int stepCost(FAStarNode* parent, FAStarNode* node, int data, const void* pointer, FAStar* finder)
{
	return 1;
}


int stepValid(FAStarNode* parent, FAStarNode* node, int data, const void* pointer, FAStar* finder)
{
	CvPlot* pNewPlot;

	if (parent == NULL)
	{
		return TRUE;
	}

	pNewPlot = GC.getMapINLINE().plotSorenINLINE(node->m_iX, node->m_iY);

	if (pNewPlot->isImpassable())
	{
		return FALSE;
	}

/********************************************************************************/
/* 	BETTER_BTS_AI_MOD					12/12/08				jdog5000	*/
/* 																			*/
/* 	Bugfix																	*/
/********************************************************************************/
/* original BTS code
	if (GC.getMapINLINE().plotSorenINLINE(parent->m_iX, parent->m_iY)->area() != pNewPlot->area())
	{
		return FALSE;
	}
*/
	CvPlot* pFromPlot = GC.getMapINLINE().plotSorenINLINE(parent->m_iX, parent->m_iY);
	if (pFromPlot->area() != pNewPlot->area())
	{
		return FALSE;
	}

	// Don't count diagonal hops across land isthmus
	if (pFromPlot->isWater() && pNewPlot->isWater())
	{
		if (!(GC.getMapINLINE().plotINLINE(parent->m_iX, node->m_iY)->isWater()) && !(GC.getMapINLINE().plotINLINE(node->m_iX, parent->m_iY)->isWater()))
		{
			return FALSE;
		}
	}
/********************************************************************************/
/* 	BETTER_BTS_AI_MOD						END								*/
/********************************************************************************/

	return TRUE;
}

/********************************************************************************/
/* 	BETTER_BTS_AI_MOD					02/02/09				jdog5000	*/
/* 																			*/
/* 																			*/
/********************************************************************************/
// Find paths that a team's units could follow without declaring war
// advc.003 (comment): This does assume a DoW on pointer[1] (eTargetTeam)
int teamStepValid(FAStarNode* parent, FAStarNode* node, int data, const void* pointer, FAStar* finder)
{
	CvPlot* pNewPlot;

	if (parent == NULL)
	{
		return TRUE;
	}

	pNewPlot = GC.getMapINLINE().plotSorenINLINE(node->m_iX, node->m_iY);

	if (pNewPlot->isImpassable())
	{
		return FALSE;
	}

	CvPlot* pFromPlot = GC.getMapINLINE().plotSorenINLINE(parent->m_iX, parent->m_iY);

	if (pFromPlot->area() != pNewPlot->area())
	{
		return FALSE;
	}

	// Don't count diagonal hops across land isthmus
	if (pFromPlot->isWater() && pNewPlot->isWater())
	{
		if (!(GC.getMapINLINE().plotINLINE(parent->m_iX, node->m_iY)->isWater()) && !(GC.getMapINLINE().plotINLINE(node->m_iX, parent->m_iY)->isWater()))
		{
			return FALSE;
		}
	}

	TeamTypes ePlotTeam = pNewPlot->getTeam();

	std::vector<TeamTypes> teamVec = *((std::vector<TeamTypes> *)pointer);
	TeamTypes eTeam = teamVec[0];
	TeamTypes eTargetTeam = teamVec[1];
	CvTeamAI& kTeam = GET_TEAM(eTeam);

	if (ePlotTeam == NO_TEAM)
	{
		return TRUE;
	}

	// advc.001: Was just ePlotTeam == eTargetTeam; anticipate DoW on/ by vassals.
	if(eTargetTeam != NO_TEAM && GET_TEAM(ePlotTeam).getMasterTeam() == GET_TEAM(eTargetTeam).getMasterTeam())
	{
		return TRUE;
	}

	if (kTeam.isFriendlyTerritory(ePlotTeam))
	{
		return TRUE;
	}

	if (kTeam.AI_getWarPlan(ePlotTeam) != NO_WARPLAN)
	{
		return TRUE;
	}

	if (kTeam.isOpenBorders(ePlotTeam)
			|| kTeam.isDisengage(ePlotTeam)) // advc.034
	{
		return TRUE;
	}



	return FALSE;
}
/********************************************************************************/
/* 	BETTER_BTS_AI_MOD						END								*/
/********************************************************************************/

int stepAdd(FAStarNode* parent, FAStarNode* node, int data, const void* pointer, FAStar* finder)
{
	if (data == ASNC_INITIALADD)
	{
		node->m_iData1 = 0;
	}
	else
	{
		node->m_iData1 = (parent->m_iData1 + 1);
	}

	FAssertMsg(node->m_iData1 >= 0, "node->m_iData1 is expected to be non-negative (invalid Index)");

	return 1;
}


int routeValid(FAStarNode* parent, FAStarNode* node, int data, const void* pointer, FAStar* finder)
{
	CvPlot* pNewPlot;
	PlayerTypes ePlayer;

	if (parent == NULL)
	{
		return TRUE;
	}

	pNewPlot = GC.getMapINLINE().plotSorenINLINE(node->m_iX, node->m_iY);

	ePlayer = ((PlayerTypes)(gDLL->getFAStarIFace()->GetInfo(finder)));

	if (!(pNewPlot->isOwned()) || (pNewPlot->getTeam() == GET_PLAYER(ePlayer).getTeam()))
	{
		if (pNewPlot->getRouteType() == GET_PLAYER(ePlayer).getBestRoute(pNewPlot))
		{
			return TRUE;
		}
	}

	return FALSE;
}


int borderValid(FAStarNode* parent, FAStarNode* node, int data, const void* pointer, FAStar* finder)
{
	CvPlot* pNewPlot;
	PlayerTypes ePlayer;

	if (parent == NULL)
	{
		return TRUE;
	}

	pNewPlot = GC.getMapINLINE().plotSorenINLINE(node->m_iX, node->m_iY);

	ePlayer = ((PlayerTypes)(gDLL->getFAStarIFace()->GetInfo(finder)));

	if (pNewPlot->getTeam() == GET_PLAYER(ePlayer).getTeam())
	{
		return TRUE;
	}

	return FALSE;
}


int areaValid(FAStarNode* parent, FAStarNode* node, int data, const void* pointer, FAStar* finder)
{
	if (parent == NULL)
	{
		return TRUE;
	}

/************************************************************************************************/
/* BETTER_BTS_AI_MOD                      10/02/09                                jdog5000      */
/*                                                                                              */
/* General AI                                                                                   */
/************************************************************************************************/
// original BTS code
	return ((GC.getMapINLINE().plotSorenINLINE(parent->m_iX, parent->m_iY)->isWater() == GC.getMapINLINE().plotSorenINLINE(node->m_iX, node->m_iY)->isWater()) ? TRUE : FALSE);

	// BBAI TODO: Why doesn't this work to break water and ice into separate area?
/*
	if( GC.getMapINLINE().plotSorenINLINE(parent->m_iX, parent->m_iY)->isWater() != GC.getMapINLINE().plotSorenINLINE(node->m_iX, node->m_iY)->isWater() )
	{
		return FALSE;
	}

	// Ice blocks become their own area
	if( GC.getMapINLINE().plotSorenINLINE(parent->m_iX, parent->m_iY)->isWater() && GC.getMapINLINE().plotSorenINLINE(node->m_iX, node->m_iY)->isWater() )
	{
		if( GC.getMapINLINE().plotSorenINLINE(parent->m_iX, parent->m_iY)->isImpassable() != GC.getMapINLINE().plotSorenINLINE(node->m_iX, node->m_iY)->isImpassable() )
		{
			return FALSE;
		}
	}

	return TRUE;
/************************************************************************************************/
/* BETTER_BTS_AI_MOD                       END                                                  */
/************************************************************************************************/
}


int joinArea(FAStarNode* parent, FAStarNode* node, int data, const void* pointer, FAStar* finder) 
{
	if (data == ASNL_ADDCLOSED)
	{
		GC.getMapINLINE().plotSorenINLINE(node->m_iX, node->m_iY)->setArea(gDLL->getFAStarIFace()->GetInfo(finder));
	}

	return 1;
}


int plotGroupValid(FAStarNode* parent, FAStarNode* node, int data, const void* pointer, FAStar* finder)
{
	CvPlot* pOldPlot;
	CvPlot* pNewPlot;
	PlayerTypes ePlayer;

	if (parent == NULL)
	{
		return TRUE;
	}

	pOldPlot = GC.getMapINLINE().plotSorenINLINE(parent->m_iX, parent->m_iY);
	pNewPlot = GC.getMapINLINE().plotSorenINLINE(node->m_iX, node->m_iY);

	ePlayer = ((PlayerTypes)(gDLL->getFAStarIFace()->GetInfo(finder)));
	TeamTypes eTeam = GET_PLAYER(ePlayer).getTeam();

	if (pOldPlot->getPlotGroup(ePlayer) == pNewPlot->getPlotGroup(ePlayer))
	{
		if (pNewPlot->isTradeNetwork(eTeam))
		{
			if (pNewPlot->isTradeNetworkConnected(pOldPlot, eTeam))
			{
				return TRUE;
			}
		}
	}

	return FALSE;
}


int countPlotGroup(FAStarNode* parent, FAStarNode* node, int data, const void* pointer, FAStar* finder)
{
	if (data == ASNL_ADDCLOSED)
	{
		(*((int*)pointer))++;
	}

	return 1;
}

// advc.003j (comment): Unused
int baseYieldToSymbol(int iNumYieldTypes, int iYieldStack)
{
	int iReturn;	// holds the return value we will be calculating

	// get the base value for the iReturn value
	iReturn = iNumYieldTypes * GC.getDefineINT("MAX_YIELD_STACK");
	// then add the offset to the return value
	iReturn += iYieldStack;

	// return the value we have calculated
	return iReturn;
}

/*  advc.003j: Vanilla Civ 4 function that used to be a DLLExport; certainly unused
	since BtS, and doesn't sound too useful. */
/*bool isPickableName(const TCHAR* szName)
{
	if (szName)
	{
		int iLen = _tcslen(szName);

		if (!_tcsicmp(&szName[iLen-6], "NOPICK"))
		{
			return false;
		}
	}

	return true;
}*/


// create an array of shuffled numbers
int* shuffle(int iNum, CvRandom& rand)
{
	int* piShuffle = new int[iNum];
	shuffleArray(piShuffle, iNum, rand);
	return piShuffle;
}


void shuffleArray(int* piShuffle, int iNum, CvRandom& rand)
{
	int iI, iJ;

	for (iI = 0; iI < iNum; iI++)
	{
		piShuffle[iI] = iI;
	}

	for (iI = 0; iI < iNum; iI++)
	{
		iJ = (rand.get(iNum - iI, NULL) + iI);

		if (iI != iJ)
		{
			int iTemp = piShuffle[iI];
			piShuffle[iI] = piShuffle[iJ];
			piShuffle[iJ] = iTemp;
		}
	}
}

int getTurnYearForGame(int iGameTurn, int iStartYear, CalendarTypes eCalendar, GameSpeedTypes eSpeed)
{
	return (getTurnMonthForGame(iGameTurn, iStartYear, eCalendar, eSpeed) / GC.getNumMonthInfos());
}


int getTurnMonthForGame(int iGameTurn, int iStartYear, CalendarTypes eCalendar, GameSpeedTypes eSpeed)
{
	int iTurnMonth;
	int iTurnCount;
	int iI;

	iTurnMonth = iStartYear * GC.getNumMonthInfos();

	switch (eCalendar)
	{
	case CALENDAR_DEFAULT:
		iTurnCount = 0;

		for (iI = 0; iI < GC.getGameSpeedInfo(eSpeed).getNumTurnIncrements(); iI++)
		{
			if (iGameTurn > (iTurnCount + GC.getGameSpeedInfo(eSpeed).getGameTurnInfo(iI).iNumGameTurnsPerIncrement))
			{
				iTurnMonth += (GC.getGameSpeedInfo(eSpeed).getGameTurnInfo(iI).iMonthIncrement * GC.getGameSpeedInfo(eSpeed).getGameTurnInfo(iI).iNumGameTurnsPerIncrement);
				iTurnCount += GC.getGameSpeedInfo(eSpeed).getGameTurnInfo(iI).iNumGameTurnsPerIncrement;
			}
			else
			{
				iTurnMonth += (GC.getGameSpeedInfo(eSpeed).getGameTurnInfo(iI).iMonthIncrement * (iGameTurn - iTurnCount));
				iTurnCount += (iGameTurn - iTurnCount);
				break;
			}
		}

		if (iGameTurn > iTurnCount)
		{
			iTurnMonth += (GC.getGameSpeedInfo(eSpeed).getGameTurnInfo(GC.getGameSpeedInfo(eSpeed).getNumTurnIncrements() - 1).iMonthIncrement * (iGameTurn - iTurnCount));
		}
		break;

	case CALENDAR_BI_YEARLY:
		iTurnMonth += (2 * iGameTurn * GC.getNumMonthInfos());
		break;

	case CALENDAR_YEARS:
	case CALENDAR_TURNS:
		iTurnMonth += iGameTurn * GC.getNumMonthInfos();
		break;

	case CALENDAR_SEASONS:
		iTurnMonth += (iGameTurn * GC.getNumMonthInfos()) / GC.getNumSeasonInfos();
		break;

	case CALENDAR_MONTHS:
		iTurnMonth += iGameTurn;
		break;

	case CALENDAR_WEEKS:
		iTurnMonth += iGameTurn / GC.getDefineINT("WEEKS_PER_MONTHS");
		break;

	default:
		FAssert(false);
	}

	return iTurnMonth;
}

// these string functions should only be used under chipotle cheat code (not internationalized)

void getDirectionTypeString(CvWString& szString, DirectionTypes eDirectionType)
{
	switch (eDirectionType)
	{
	case NO_DIRECTION: szString = L"NO_DIRECTION"; break;

	case DIRECTION_NORTH: szString = L"north"; break;
	case DIRECTION_NORTHEAST: szString = L"northeast"; break;
	case DIRECTION_EAST: szString = L"east"; break;
	case DIRECTION_SOUTHEAST: szString = L"southeast"; break;
	case DIRECTION_SOUTH: szString = L"south"; break;
	case DIRECTION_SOUTHWEST: szString = L"southwest"; break;
	case DIRECTION_WEST: szString = L"west"; break;
	case DIRECTION_NORTHWEST: szString = L"northwest"; break;

	default: szString = CvWString::format(L"UNKNOWN_DIRECTION(%d)", eDirectionType); break;
	}
}

void getCardinalDirectionTypeString(CvWString& szString, CardinalDirectionTypes eDirectionType)
{
	getDirectionTypeString(szString, cardinalDirectionToDirection(eDirectionType));
}

// advc.007: Removed the "ACTIVITY_" prefix from the strings b/c it takes up too much space.
void getActivityTypeString(CvWString& szString, ActivityTypes eActivityType)
{
	switch (eActivityType)
	{
	case NO_ACTIVITY: szString = L"NO_ACTIVITY"; break;

	case ACTIVITY_AWAKE: szString = L"AWAKE"; break;
	case ACTIVITY_HOLD: szString = L"HOLD"; break;
	case ACTIVITY_SLEEP: szString = L"SLEEP"; break;
	case ACTIVITY_HEAL: szString = L"HEAL"; break;
	case ACTIVITY_SENTRY: szString = L"SENTRY"; break;
	case ACTIVITY_INTERCEPT: szString = L"INTERCEPT"; break;
	case ACTIVITY_MISSION: szString = L"MISSION"; break;
// K-Mod. There were some missing activity strings...
/*#define case_string(x) case x: szString = L#x; break;
	case_string(ACTIVITY_PATROL)
	case_string(ACTIVITY_PLUNDER)
#undef case_string*/
// K-Mod end
	// <advc.007>
	case ACTIVITY_PATROL: szString = L"PATROL"; break;
	case ACTIVITY_PLUNDER: szString = L"PLUNDER"; break;
	// </advc.007>
	default: szString = CvWString::format(L"UNKNOWN_ACTIVITY(%d)", eActivityType); break;
	}
}

void getMissionTypeString(CvWString& szString, MissionTypes eMissionType)
{
	switch (eMissionType)
	{
	case NO_MISSION: szString = L"NO_MISSION"; break;

	case MISSION_MOVE_TO: szString = L"MISSION_MOVE_TO"; break;
	case MISSION_ROUTE_TO: szString = L"MISSION_ROUTE_TO"; break;
	case MISSION_MOVE_TO_UNIT: szString = L"MISSION_MOVE_TO_UNIT"; break;
	case MISSION_SKIP: szString = L"MISSION_SKIP"; break;
	case MISSION_SLEEP: szString = L"MISSION_SLEEP"; break;
	case MISSION_FORTIFY: szString = L"MISSION_FORTIFY"; break;
	case MISSION_PLUNDER: szString = L"MISSION_PLUNDER"; break;
	case MISSION_AIRPATROL: szString = L"MISSION_AIRPATROL"; break;
	case MISSION_SEAPATROL: szString = L"MISSION_SEAPATROL"; break;
	case MISSION_HEAL: szString = L"MISSION_HEAL"; break;
	case MISSION_SENTRY_HEAL: szString = L"MISSION_SENTRY_HEAL"; break; // advc.004l
	case MISSION_SENTRY: szString = L"MISSION_SENTRY"; break;
	case MISSION_AIRLIFT: szString = L"MISSION_AIRLIFT"; break;
	case MISSION_NUKE: szString = L"MISSION_NUKE"; break;
	case MISSION_RECON: szString = L"MISSION_RECON"; break;
	case MISSION_PARADROP: szString = L"MISSION_PARADROP"; break;
	case MISSION_AIRBOMB: szString = L"MISSION_AIRBOMB"; break;
	case MISSION_BOMBARD: szString = L"MISSION_BOMBARD"; break;
	case MISSION_PILLAGE: szString = L"MISSION_PILLAGE"; break;
	case MISSION_SABOTAGE: szString = L"MISSION_SABOTAGE"; break;
	case MISSION_DESTROY: szString = L"MISSION_DESTROY"; break;
	case MISSION_STEAL_PLANS: szString = L"MISSION_STEAL_PLANS"; break;
	case MISSION_FOUND: szString = L"MISSION_FOUND"; break;
	case MISSION_SPREAD: szString = L"MISSION_SPREAD"; break;
	case MISSION_SPREAD_CORPORATION: szString = L"MISSION_SPREAD_CORPORATION"; break;
	case MISSION_JOIN: szString = L"MISSION_JOIN"; break;
	case MISSION_CONSTRUCT: szString = L"MISSION_CONSTRUCT"; break;
	case MISSION_DISCOVER: szString = L"MISSION_DISCOVER"; break;
	case MISSION_HURRY: szString = L"MISSION_HURRY"; break;
	case MISSION_TRADE: szString = L"MISSION_TRADE"; break;
	case MISSION_GREAT_WORK: szString = L"MISSION_GREAT_WORK"; break;
	case MISSION_INFILTRATE: szString = L"MISSION_INFILTRATE"; break;
	case MISSION_GOLDEN_AGE: szString = L"MISSION_GOLDEN_AGE"; break;
	case MISSION_BUILD: szString = L"MISSION_BUILD"; break;
	case MISSION_LEAD: szString = L"MISSION_LEAD"; break;
	case MISSION_ESPIONAGE: szString = L"MISSION_ESPIONAGE"; break;
	case MISSION_DIE_ANIMATION: szString = L"MISSION_DIE_ANIMATION"; break;

	case MISSION_BEGIN_COMBAT: szString = L"MISSION_BEGIN_COMBAT"; break;
	case MISSION_END_COMBAT: szString = L"MISSION_END_COMBAT"; break;
	case MISSION_AIRSTRIKE: szString = L"MISSION_AIRSTRIKE"; break;
	case MISSION_SURRENDER: szString = L"MISSION_SURRENDER"; break;
	case MISSION_CAPTURED: szString = L"MISSION_CAPTURED"; break;
	case MISSION_IDLE: szString = L"MISSION_IDLE"; break;
	case MISSION_DIE: szString = L"MISSION_DIE"; break;
	case MISSION_DAMAGE: szString = L"MISSION_DAMAGE"; break;
	case MISSION_MULTI_SELECT: szString = L"MISSION_MULTI_SELECT"; break;
	case MISSION_MULTI_DESELECT: szString = L"MISSION_MULTI_DESELECT"; break;

	default: szString = CvWString::format(L"UNKOWN_MISSION(%d)", eMissionType); break;
	}
}

// advc.007: Removed the "MISSIONAI_" prefix from the strings b/c it takes up too much space.
void getMissionAIString(CvWString& szString, MissionAITypes eMissionAI)
{
	switch (eMissionAI)
	{
	case NO_MISSIONAI: szString = L"NO_MISSIONAI"; break;

	case MISSIONAI_SHADOW: szString = L"SHADOW"; break;
	case MISSIONAI_GROUP: szString = L"GROUP"; break;
	case MISSIONAI_LOAD_ASSAULT: szString = L"LOAD_ASSAULT"; break;
	case MISSIONAI_LOAD_SETTLER: szString = L"LOAD_SETTLER"; break;
	case MISSIONAI_LOAD_SPECIAL: szString = L"LOAD_SPECIAL"; break;
	case MISSIONAI_GUARD_CITY: szString = L"GUARD_CITY"; break;
	case MISSIONAI_GUARD_BONUS: szString = L"GUARD_BONUS"; break;
	case MISSIONAI_GUARD_SPY: szString = L"GUARD_SPY"; break;
	case MISSIONAI_ATTACK_SPY: szString = L"ATTACK_SPY"; break;
	case MISSIONAI_SPREAD: szString = L"SPREAD"; break;
	case MISSIONAI_CONSTRUCT: szString = L"CONSTRUCT"; break;
	case MISSIONAI_HURRY: szString = L"HURRY"; break;
	case MISSIONAI_GREAT_WORK: szString = L"GREAT_WORK"; break;
	case MISSIONAI_EXPLORE: szString = L"EXPLORE"; break;
	case MISSIONAI_BLOCKADE: szString = L"BLOCKADE"; break;
	case MISSIONAI_PILLAGE: szString = L"PILLAGE"; break;
	case MISSIONAI_FOUND: szString = L"FOUND"; break;
	case MISSIONAI_BUILD: szString = L"BUILD"; break;
	case MISSIONAI_ASSAULT: szString = L"ASSAULT"; break;
	case MISSIONAI_CARRIER: szString = L"CARRIER"; break;
	case MISSIONAI_PICKUP: szString = L"PICKUP"; break;
// K-Mod
#define mission_string(x) case x: szString = L#x; break;
	mission_string(MISSIONAI_GUARD_COAST)
	mission_string(MISSIONAI_REINFORCE)
	mission_string(MISSIONAI_SPREAD_CORPORATION)
	mission_string(MISSIONAI_RECON_SPY)
	mission_string(MISSIONAI_JOIN_CITY)
	mission_string(MISSIONAI_TRADE)
	mission_string(MISSIONAI_INFILTRATE)
	mission_string(MISSIONAI_CHOKE)
	mission_string(MISSIONAI_HEAL)
	mission_string(MISSIONAI_RETREAT)
	mission_string(MISSIONAI_PATROL)
	mission_string(MISSIONAI_DEFEND)
	mission_string(MISSIONAI_COUNTER_ATTACK)
	mission_string(MISSIONAI_UPGRADE)
	mission_string(MISSIONAI_STRANDED)
#undef mission_string
// K-Mod end

	default: szString = CvWString::format(L"UNKOWN_MISSION_AI(%d)", eMissionAI); break;
	}
}

void getUnitAIString(CvWString& szString, UnitAITypes eUnitAI)
{
	// note, GC.getUnitAIInfo(eUnitAI).getDescription() is a international friendly way to get string (but it will be longer)
	
	switch (eUnitAI)
	{
	case NO_UNITAI: szString = L"no unitAI"; break;

	case UNITAI_UNKNOWN: szString = L"unknown"; break;
	case UNITAI_ANIMAL: szString = L"animal"; break;
	case UNITAI_SETTLE: szString = L"settle"; break;
	case UNITAI_WORKER: szString = L"worker"; break;
	case UNITAI_ATTACK: szString = L"attack"; break;
	case UNITAI_ATTACK_CITY: szString = L"attack city"; break;
	case UNITAI_COLLATERAL: szString = L"collateral"; break;
	case UNITAI_PILLAGE: szString = L"pillage"; break;
	case UNITAI_RESERVE: szString = L"reserve"; break;
	case UNITAI_COUNTER: szString = L"counter"; break;
	case UNITAI_CITY_DEFENSE: szString = L"city defense"; break;
	case UNITAI_CITY_COUNTER: szString = L"city counter"; break;
	case UNITAI_CITY_SPECIAL: szString = L"city special"; break;
	case UNITAI_EXPLORE: szString = L"explore"; break;
	case UNITAI_MISSIONARY: szString = L"missionary"; break;
	case UNITAI_PROPHET: szString = L"prophet"; break;
	case UNITAI_ARTIST: szString = L"artist"; break;
	case UNITAI_SCIENTIST: szString = L"scientist"; break;
	case UNITAI_GENERAL: szString = L"general"; break;
	case UNITAI_MERCHANT: szString = L"merchant"; break;
	case UNITAI_ENGINEER: szString = L"engineer"; break;
	case UNITAI_GREAT_SPY: szString = L"great spy"; break; // K-Mod
	case UNITAI_SPY: szString = L"spy"; break;
	case UNITAI_ICBM: szString = L"icbm"; break;
	case UNITAI_WORKER_SEA: szString = L"worker sea"; break;
	case UNITAI_ATTACK_SEA: szString = L"attack sea"; break;
	case UNITAI_RESERVE_SEA: szString = L"reserve sea"; break;
	case UNITAI_ESCORT_SEA: szString = L"escort sea"; break;
	case UNITAI_EXPLORE_SEA: szString = L"explore sea"; break;
	case UNITAI_ASSAULT_SEA: szString = L"assault sea"; break;
	case UNITAI_SETTLER_SEA: szString = L"settler sea"; break;
	case UNITAI_MISSIONARY_SEA: szString = L"missionary sea"; break;
	case UNITAI_SPY_SEA: szString = L"spy sea"; break;
	case UNITAI_CARRIER_SEA: szString = L"carrier sea"; break;
	case UNITAI_MISSILE_CARRIER_SEA: szString = L"missile carrier"; break;
	case UNITAI_PIRATE_SEA: szString = L"pirate sea"; break;
	case UNITAI_ATTACK_AIR: szString = L"attack air"; break;
	case UNITAI_DEFENSE_AIR: szString = L"defense air"; break;
	case UNITAI_CARRIER_AIR: szString = L"carrier air"; break;
	case UNITAI_MISSILE_AIR: szString = L"missile air"; break; // K-Mod (this string was missing)
	case UNITAI_PARADROP: szString = L"paradrop"; break;
	case UNITAI_ATTACK_CITY_LEMMING: szString = L"attack city lemming"; break;

	default: szString = CvWString::format(L"unknown(%d)", eUnitAI); break;
	}
}

// Lead From Behind by UncutDragon
typedef std::vector<int> LFBoddsAttOdds;
typedef std::vector<LFBoddsAttOdds> LFBoddsDefRounds;
typedef std::vector<LFBoddsDefRounds> LFBoddsAttRounds;
typedef std::vector<LFBoddsAttRounds> LFBoddsFirstStrike;
int LFBlookupCombatOdds(int iFirstStrikes, int iNeededRoundsAttacker, int iNeededRoundsDefender, int iAttackerOdds);
int LFBlookupCombatOdds(LFBoddsFirstStrike* pOdds, int iFSIndex, int iFirstStrikes, int iNeededRoundsAttacker, int iNeededRoundsDefender, int iAttackerOdds);
int LFBlookupCombatOdds(LFBoddsAttOdds* pOdds, int iOddsIndex, int iFirstStrikes, int iNeededRoundsAttacker, int iNeededRoundsDefender, int iAttackerOdds);
int LFBcalculateCombatOdds(int iFirstStrikes, int iNeededRoundsAttacker, int iNeededRoundsDefender, int iAttackerOdds);

const int LFB_ODDS_INTERVAL_SIZE = 16;
const int LFB_ODDS_EXTRA_ACCURACY = 32;
LFBoddsFirstStrike pOddsCacheFSPos;
LFBoddsFirstStrike pOddsCacheFSNeg;

// gets the combat odds using precomputed attacker/defender values instead of unit pointers
int LFBgetCombatOdds(int iAttackerLowFS,	int iAttackerHighFS, int iDefenderLowFS, int iDefenderHighFS, int iNeededRoundsAttacker, int iNeededRoundsDefender, int iAttackerOdds)
{
	int iDefenderOdds;
	bool bFlip = false;
	int iFirstStrikes;
	int iI;
	int iJ;
	int iOdds = 0;

	// Essentially, this means we're attacking with a seige engine and the defender is already at or below the max combat limit
	// We're not allowed to attack regardless, since we can't do any damage - just return 100%
	if (iNeededRoundsAttacker == 0)
		return 1000;
	// Because the best defender code calls us from the defender's perspective, we also need to check 'defender' rounds zero
	if (iNeededRoundsDefender == 0)
		return 0;

	// If attacker has better than even chance to hit, we just flip it and calculate defender's chance to win
	// This reduces how much we cache considerably (by half just from the fact we're only dealing with half the odds
	// - but additionally, iNeededRounds'Defender' is guaranteed to stay low - at most 5 with standard settings).
	iDefenderOdds = GC.getCOMBAT_DIE_SIDES() - iAttackerOdds;	
	if (iAttackerOdds > iDefenderOdds)
		bFlip = true;

	// This is basically the two outside loops at the end of the standard getCombatOdds
	// We just call our cache lookup in the middle (flipped if necessary) instead of the actual computation
	for (iI = iAttackerLowFS; iI < iAttackerHighFS + 1; iI++)
	{
		for (iJ = iDefenderLowFS; iJ < iDefenderHighFS + 1; iJ++)
		{
			iFirstStrikes = iI - iJ;
			if (bFlip)
				iOdds += LFBlookupCombatOdds(-iFirstStrikes, iNeededRoundsDefender, iNeededRoundsAttacker, iDefenderOdds);
			else
				iOdds += LFBlookupCombatOdds(iFirstStrikes, iNeededRoundsAttacker, iNeededRoundsDefender, iAttackerOdds);
		}
	}

	// Odds are a straight average of all the FS combinations (since all are equally possible)
	iOdds /= ((iAttackerHighFS - iAttackerLowFS + 1) * (iDefenderHighFS - iDefenderLowFS + 1));

	// Now that we have the final odds, we can remove the extra accuracy, rounding off
	iOdds = (iOdds + (LFB_ODDS_EXTRA_ACCURACY/2)) / LFB_ODDS_EXTRA_ACCURACY;

	// If we flipped the perspective in the computation/lookup, need to flip it back now
	if (bFlip)
		iOdds = 1000 - iOdds;

	return iOdds;
}

// lookup the combat odds in the cache for a specific sub-result
int LFBlookupCombatOdds(int iFirstStrikes, int iNeededRoundsAttacker, int iNeededRoundsDefender, int iAttackerOdds)
{
	int iOdds = 0;

	// We actually maintain two caches - one for positive first strikes (plus zero), and one for negative
	// This just makes the indices (and growing the array as needed) easy
	if (iFirstStrikes < 0)
		iOdds = LFBlookupCombatOdds(&pOddsCacheFSNeg, (-iFirstStrikes)-1, iFirstStrikes, iNeededRoundsAttacker, iNeededRoundsDefender, iAttackerOdds);
	else
		iOdds = LFBlookupCombatOdds(&pOddsCacheFSPos, iFirstStrikes, iFirstStrikes, iNeededRoundsAttacker, iNeededRoundsDefender, iAttackerOdds);

	return iOdds;
}

int LFBlookupCombatOdds(LFBoddsFirstStrike* pOdds, int iFSIndex, int iFirstStrikes, int iNeededRoundsAttacker, int iNeededRoundsDefender, int iAttackerOdds)
{
	// Grow the arrays as needed
	// First dimension is the first strikes
	int iInsert = iFSIndex - (int)(*pOdds).size() + 1;
	if (iInsert > 0)
	{
		LFBoddsAttRounds pAdd;
		(*pOdds).insert((*pOdds).end(), iInsert, pAdd);
	}

	// Second dimension is the attacker rounds (starting at 1)
	LFBoddsAttRounds* pAttRounds = &((*pOdds)[iFSIndex]);
	iInsert = iNeededRoundsAttacker - (int)(*pAttRounds).size();
	if (iInsert > 0)
	{
		LFBoddsDefRounds pAdd;
		(*pAttRounds).insert((*pAttRounds).end(), iInsert, pAdd);
	}

	// Third dimension is the defender rounds (starting at 1)
	LFBoddsDefRounds* pDefRounds = &((*pAttRounds)[iNeededRoundsAttacker-1]);
	iInsert = iNeededRoundsDefender - (int)(*pDefRounds).size();
	if (iInsert > 0)
	{
		LFBoddsAttOdds pAdd;
		(*pDefRounds).insert((*pDefRounds).end(), iInsert, pAdd);
	}

	// Fourth (last) dimension is the odds index (odds/16)
	LFBoddsAttOdds* pAttOdds = &((*pDefRounds)[iNeededRoundsDefender-1]);

	// Round down to the nearest interval
	int iMinOddsIndex = iAttackerOdds / LFB_ODDS_INTERVAL_SIZE;
	int iMinOddsValue = iMinOddsIndex * LFB_ODDS_INTERVAL_SIZE;

	// Lookup the odds for the rounded down value
	int iOdds = LFBlookupCombatOdds(pAttOdds, iMinOddsIndex, iFirstStrikes, iNeededRoundsAttacker, iNeededRoundsDefender, iMinOddsValue);

	// If we happened to hit an interval exactly, we're done
	if (iMinOddsValue < iAttackerOdds)
	{
		// 'Round up' to the nearest interval - we don't actually need to compute it, we know
		// it's one more than the rounded down interval
		int iMaxOddsIndex = iMinOddsIndex+1;
		int iMaxOddsValue = iMinOddsValue+LFB_ODDS_INTERVAL_SIZE;

		// Lookup the odds for the rounded up value
		int iMaxOdds = LFBlookupCombatOdds(pAttOdds, iMaxOddsIndex, iFirstStrikes, iNeededRoundsAttacker, iNeededRoundsDefender, iMaxOddsValue);
		
		// Do a simple weighted average on the two odds
		//iOdds += (((iAttackerOdds - iMinOddsValue) * (iMaxOdds - iOdds)) / LFB_ODDS_INTERVAL_SIZE);
		iOdds += ((iAttackerOdds - iMinOddsValue) * (iMaxOdds - iOdds) + LFB_ODDS_INTERVAL_SIZE/2) / LFB_ODDS_INTERVAL_SIZE; // K-Mod. (rounded rather than truncated)
	}

	return iOdds;
}

int LFBlookupCombatOdds(LFBoddsAttOdds* pOdds, int iOddsIndex, int iFirstStrikes, int iNeededRoundsAttacker, int iNeededRoundsDefender, int iAttackerOdds)
{
	int iNotComputed = -1;

	// Index 0 -> AttackerOdds 0 -> no chance to win
	if (iOddsIndex == 0)
		return 0;

	// We don't store all possible indices, just what we need/use
	// So use position 0 to keep track of what index we start with
	int iFirstIndex = iOddsIndex;
	if ((*pOdds).size() == 0)
		(*pOdds).push_back(iFirstIndex);
	else
		iFirstIndex = (*pOdds)[0];

	int iRealIndex = iOddsIndex - iFirstIndex + 1;

	// Index is before the start of our array
	int iInsert = -iRealIndex+1;
	if (iInsert > 0)
	{
		(*pOdds).insert((*pOdds).begin()+1, iInsert, iNotComputed);
		iFirstIndex -= iInsert;
		iRealIndex = 1;
		(*pOdds)[0] = iFirstIndex;
	}

	// Index is past the end of our array
	iInsert = iRealIndex - (int)(*pOdds).size() + 1;
	if (iInsert > 0)
		(*pOdds).insert((*pOdds).end(), iInsert, iNotComputed);

	// Retrieve the odds from the array
	int iOdds = (*pOdds)[iRealIndex];

	// Odds aren't cached yet - need to actually calculate them
	if (iOdds == iNotComputed)
	{
		iOdds = LFBcalculateCombatOdds(iFirstStrikes, iNeededRoundsAttacker, iNeededRoundsDefender, iAttackerOdds);
		(*pOdds)[iRealIndex] = iOdds;
	}

	return iOdds;
}

// Perform the actual odds calculation (basically identical to the default algorithm, except that we retain a little more accuracy)
int LFBcalculateCombatOdds(int iFirstStrikes, int iNeededRoundsAttacker, int iNeededRoundsDefender, int iAttackerOdds)
{
	float fOddsEvent;
	float fOddsAfterEvent;
	int iMaxRounds = iNeededRoundsAttacker + iNeededRoundsDefender - 1;
	int iOdds = 0;
	int iI3;
	int iI4;

	// This part is basically the inside of the outer two loops at the end of the standard getCombatOdds
	if (iFirstStrikes > 0)
	{
		// Attacker gets more or equal first strikes than defender

		// For every possible first strike getting hit, calculate both
		// the chance of that event happening, as well as the rest of 
		// the chance assuming the event has happened. Multiply these 
		// together to get the total chance (Bayes rule). 
		// iI3 counts the number of successful first strikes
		//////

		for (iI3 = 0; iI3 < (iFirstStrikes + 1); iI3++)
		{
			// event: iI3 first strikes hit the defender

			// calculate chance of iI3 first strikes hitting: fOddsEvent
			// f(k;n,p)=C(n,k)*(p^k)*((1-p)^(n-k)) 
			// this needs to be in floating point math
			//////

			fOddsEvent = ((float)getBinomialCoefficient(iFirstStrikes, iI3)) * std::pow((((float)iAttackerOdds) / GC.getCOMBAT_DIE_SIDES()), iI3) * std::pow((1.0f - (((float)iAttackerOdds) / GC.getCOMBAT_DIE_SIDES())), (iFirstStrikes - iI3));

			// calculate chance assuming iI3 first strike hits: fOddsAfterEvent
			//////

			if (iI3 >= iNeededRoundsAttacker)
			{
				fOddsAfterEvent = 1;
			}
			else
			{
				fOddsAfterEvent = 0;

				// odds for _at_least_ (iNeededRoundsAttacker - iI3) (the remaining hits 
				// the attacker needs to make) out of (iMaxRounds - iI3) (the left over 
				// rounds) is the sum of each _exact_ draw
				//////

				for (iI4 = (iNeededRoundsAttacker - iI3); iI4 < (iMaxRounds - iI3 + 1); iI4++)
				{
					// odds of exactly iI4 out of (iMaxRounds - iI3) draws.
					// f(k;n,p)=C(n,k)*(p^k)*((1-p)^(n-k)) 
					// this needs to be in floating point math
					//////

					fOddsAfterEvent += ((float)getBinomialCoefficient((iMaxRounds - iI3), iI4)) * std::pow((((float)iAttackerOdds) / GC.getCOMBAT_DIE_SIDES()), iI4) * std::pow((1.0f - (((float)iAttackerOdds) / GC.getCOMBAT_DIE_SIDES())), ((iMaxRounds - iI3) - iI4));
				}
			}

			// Multiply these together, round them properly, and add 
			// the result to the total iOdds
			//////

			iOdds += ((int)((1000.0 * fOddsEvent * fOddsAfterEvent * (float)LFB_ODDS_EXTRA_ACCURACY) + 0.5));
		}
	}
	else // (iI < iJ)
	{
		// Attacker gets less first strikes than defender
		int iDefenderOdds = GC.getCOMBAT_DIE_SIDES() - iAttackerOdds;
		iFirstStrikes *= -1;

		// For every possible first strike getting hit, calculate both
		// the chance of that event happening, as well as the rest of 
		// the chance assuming the event has happened. Multiply these 
		// together to get the total chance (Bayes rule). 
		// iI3 counts the number of successful first strikes
		//////

		for (iI3 = 0; iI3 < (iFirstStrikes + 1); iI3++)
		{
			// event: iI3 first strikes hit the defender

			// First of all, check if the attacker is still alive.
			// Otherwise, no further calculations need to occur 
			/////

			if (iI3 < iNeededRoundsDefender)
			{
				// calculate chance of iI3 first strikes hitting: fOddsEvent
				// f(k;n,p)=C(n,k)*(p^k)*((1-p)^(n-k)) 
				// this needs to be in floating point math
				//////

				fOddsEvent = ((float)getBinomialCoefficient(iFirstStrikes, iI3)) * std::pow((((float)iDefenderOdds) / GC.getCOMBAT_DIE_SIDES()), iI3) * std::pow((1.0f - (((float)iDefenderOdds) / GC.getCOMBAT_DIE_SIDES())), (iFirstStrikes - iI3));

				// calculate chance assuming iI3 first strike hits: fOddsAfterEvent
				//////

				fOddsAfterEvent = 0;

				// odds for _at_least_ iNeededRoundsAttacker (the remaining hits 
				// the attacker needs to make) out of (iMaxRounds - iI3) (the left over 
				// rounds) is the sum of each _exact_ draw
				//////

				for (iI4 = iNeededRoundsAttacker; iI4 < (iMaxRounds - iI3 + 1); iI4++)
				{

					// odds of exactly iI4 out of (iMaxRounds - iI3) draws.
					// f(k;n,p)=C(n,k)*(p^k)*((1-p)^(n-k)) 
					// this needs to be in floating point math
					//////

					fOddsAfterEvent += ((float)getBinomialCoefficient((iMaxRounds - iI3), iI4)) * std::pow((((float)iAttackerOdds) / GC.getCOMBAT_DIE_SIDES()), iI4) * std::pow((1.0f - (((float)iAttackerOdds) / GC.getCOMBAT_DIE_SIDES())), ((iMaxRounds - iI3) - iI4));
				}

				// Multiply these together, round them properly, and add 
				// the result to the total iOdds
				//////

				iOdds += ((int)((1000.0 * fOddsEvent * fOddsAfterEvent * (float)LFB_ODDS_EXTRA_ACCURACY)+0.5));
			}
		}				
	}

	return iOdds;
}
// lfb end
