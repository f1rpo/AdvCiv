// advc.104: New class hierarchy; see MilitaryBranch.h.

#include "CvGameCoreDLL.h"
#include "MilitaryBranch.h"
#include "UWAIAgent.h"
#include "CvGameAI.h"
#include "CvPlayerAI.h"
#include "CvCity.h"
#include "CvCivilization.h"

using std::ostream;
using std::vector;

MilitaryBranch::MilitaryBranch(PlayerTypes ownerId) : ownerId(ownerId) {

	typicalUnitType = NO_UNIT;
	typicalUnitPower = 0;
	bombard = soften = false;
	pow = 0;
	number = 0;
}

MilitaryBranch::MilitaryBranch(MilitaryBranch const& other) :
		ownerId(other.ownerId), typicalUnitType(other.typicalUnitType),
		typicalUnitPower(other.typicalUnitPower), pow(other.pow),
		number(other.number), bombard(other.bombard), soften(other.soften) {}

void MilitaryBranch::write(FDataStreamBase* stream) {

	stream->Write(typicalUnitType);
	stream->Write(typicalUnitPower);
	stream->Write(pow);
	stream->Write(number);
	stream->Write(bombard);
	stream->Write(soften);
}

void MilitaryBranch::read(FDataStreamBase* stream) {

	int tmp;
	stream->Read(&tmp);
	typicalUnitType = (UnitTypes)tmp;
	stream->Read(&typicalUnitPower);
	stream->Read(&pow);
	stream->Read(&number);
	stream->Read(&bombard);
	stream->Read(&soften);
}

void MilitaryBranch::updateTypicalUnit() {

	PROFILE_FUNC();
	double val = 0;
	double bestVal = 0;
	CvPlayerAI& owner = GET_PLAYER(ownerId);
	CvCivilization const& civ = owner.getCivilization();
	for(int i = 0; i < civ.getNumUnits(); i++) {
		UnitTypes const ut = civ.unitAt(i);
		CvUnitInfo const& u = GC.getInfo(ut);
		// Siege and air units count for power but aren't typical
		if(u.getCombat() == 0 || u.getCombatLimit() < 100 || !isValidDomain(u) ||
				u.getDomainType() == DOMAIN_AIR || u.getDomainType() == DOMAIN_IMMOBILE)
			continue;
		UnitClassTypes const uct = CvCivilization::unitClass(ut);
		/*  I may want to give some combat unit (e.g. War Elephant) a national limit
			or an instance cost modifier at some point */
		CvUnitClassInfo const& uci = GC.getInfo(uct);
		int nationalLimit = uci.getMaxPlayerInstances();
		if(nationalLimit >= 0 && nationalLimit <
				(GC.AI_getGame().AI_getCurrEraFactor() + 1) * 4)
			continue;
		if(uci.getInstanceCostModifier() >= 20)
			continue;
		/* Could call this for land units as well, but relying on the capital for
		   those is faster, and perhaps more accurate as well. */
		if(u.getDomainType() == DOMAIN_SEA) {
			if(!GET_PLAYER(ownerId).AI_canBeExpectedToTrain(ut))
				continue;
		}
		else {
			if(!owner.hasCapital() ||
					!owner.getCapital()->canTrain(ut, false, false, false, false,
					true)) // Ignore air unit cap
				continue;
		}
		/* Normally apply situational modifiers only in simulation steps;
		   use them here only in order to determine the most suitable unit
		   for a given job. */
		double unitPow = unitPower(u, true);
		if(unitPow < 0.01)
			continue;
		double utility = unitUtility(u, unitPow);
		double productionCost = estimateProductionCost(u);
		if(productionCost <= 0)
			continue;
		val = utility / productionCost;
		if(val > bestVal) {
			bestVal = val;
			typicalUnitPower = unitPower(u, false);
			typicalUnitType = ut;
		}
	}
}

void MilitaryBranch::NuclearArsenal::updateTypicalUnit() {

	CvPlayerAI& owner = GET_PLAYER(ownerId);
	if(!owner.hasCapital())
		return;
	double bestVal = 0;
	CvCivilization const& civ = owner.getCivilization();
	for(int i = 0; i < civ.getNumUnits(); i++) {
		UnitTypes ut = civ.unitAt(i);
		if(!owner.getCapitalCity()->canTrain(ut))
			continue;
		CvUnitInfo const& u = GC.getInfo(ut);
		double unitPow = unitPower(u, true);
		if(unitPow < 0.01)
			continue;
		double utility = unitUtility(u, unitPow);
		double productionCost = estimateProductionCost(u);
		if(productionCost <= 0)
			continue;
		double val = utility / productionCost;
		if(val > bestVal) {
			bestVal = val;
			typicalUnitPower = unitPower(u, false);
			typicalUnitType = ut;
		}
	}
}

CvUnitInfo const* MilitaryBranch::getTypicalUnit() const {

	if(typicalUnitType == NO_UNIT)
		return NULL;
	return &GC.getInfo(typicalUnitType);
}

UnitTypes MilitaryBranch::getTypicalUnitType() const {

	return typicalUnitType;
}

double MilitaryBranch::getTypicalUnitPower(PlayerTypes pov) const {

	if(canKnowTypicalUnit(pov))
		return typicalUnitPower;
	// Underestimate power
	return typicalUnitPower * 0.8;
}

double MilitaryBranch::getTypicalUnitCost(PlayerTypes pov) const {

	if(typicalUnitType == NO_UNIT)
		return -1;
	CvPlayerAI const& owner = GET_PLAYER(ownerId);
	double r = owner.getProductionNeeded(typicalUnitType,
			::round(estimateExtraInstances(owner.AI_getCurrEraFactor())));
	if(canKnowTypicalUnit(pov))
		return r;
	// Underestimate cost
	return r * 0.85;
}

bool MilitaryBranch::canKnowTypicalUnit(PlayerTypes pov) const {

	if(pov == NO_PLAYER || pov == ownerId || typicalUnitType == NO_UNIT)
		return true;
	if(getTypicalUnit()->getPrereqAndTech() != NO_TECH)
		return true; // Warrior
	/*	(Wouldn't be difficult to let CvTeamAI keep track of units ever encountered
		through a EnumMap<UnitClassTypes,bool> that gets updated by CvUnit::setXY -
		tbd. maybe.) */
	if(GET_PLAYER(ownerId).getUnitClassCount(getTypicalUnit()->getUnitClassType()) > 0)
		return true; // The unit's in the wild
	if(GET_PLAYER(pov).canSeeTech(ownerId))
		return true; // Tech visible on Foreign Advisor
	return false;
}

double MilitaryBranch::estimateProductionCost(CvUnitInfo const& u) {

	double r = u.getProductionCost();
	CvPlayerAI const& owner = GET_PLAYER(ownerId);
	UnitClassTypes const uct = u.getUnitClassType();
	/*  CvPlayer::getProductionNeeded would be needlessly slow. Don't need all
		those modifiers, and we need a projection for InstanceCostModifier anyway. */
	int instanceCostMod = GC.getInfo(uct).getInstanceCostModifier();
	if(instanceCostMod > 0) {
		r *= 1 + (instanceCostMod * 0.01 *
					(owner.getUnitClassCount(uct) +
					estimateExtraInstances(owner.AI_getCurrEraFactor())));
	}
	return r;
}

bool MilitaryBranch::canEmploy(CvUnitInfo const& u) const {

	return u.getCombat() > 0 && isValidDomain(u) && unitPower(u) > 0;
}

void MilitaryBranch::updatePower(CvUnitInfo const& u, bool add) {

	int sign = 1;
	if(!add)
		sign = -1;
	// Calling canEmploy may, unfortunately, call unitPower twice
	if((u.getCombat() > 0 || u.getNukeRange() >= 0) && isValidDomain(u)) {
		double powChange = unitPower(u);
		if(powChange > 0) {
			powChange *= sign;
			pow += powChange;
			number += sign;
		}
	}
}

bool MilitaryBranch::isValidDomain(DomainTypes d) const {

	return true;
}

double MilitaryBranch::HomeGuard::initUnitsTrained(int numNonNavalUnits,
		double powNonNavalUnits) {

	// Akin to list in CvUnitAI::AI_isCityAIType
	CvPlayerAI const& owner = GET_PLAYER(ownerId);
	UnitAITypes guardAITypes[] = {
		UNITAI_CITY_DEFENSE,
		UNITAI_CITY_COUNTER,
		UNITAI_DEFENSE_AIR,
		UNITAI_CITY_SPECIAL,
		UNITAI_RESERVE,
	};
	number = 0;
	for(int i = 0; i < ARRAY_LENGTH(guardAITypes); i++)
		number += owner.AI_getNumAIUnits(guardAITypes[i]);
	/* 1.5 per city might be more realistic, but humans tend to use especially
	   weak units as garrisons. */
	if(owner.isHuman())
		number = owner.getNumCities();
	/*	Units with aggressive AI types can be temporarily tied down defending cities.
		So the count based on AI types isn't reliable. */
	else number = scaled::max(number, fixp(10/7.) * owner.getNumCities()).round();
	number = std::min(number, numNonNavalUnits);
	double r = 0;
	/* Splitting nonNavyPower up based on counted units tends to overestimate
	   the power of garrisons because these tend to be cheaper units. On the
	   other hand, the AI doesn't put every single non-garrison into its invasion
	   stacks, so this may even out. */
	if(numNonNavalUnits > 0)
		r = number / (double)numNonNavalUnits;
	FAssert(r <= 1);
	pow = powNonNavalUnits * r;
	return r;
}

bool MilitaryBranch::isValidDomain(CvUnitInfo const& u) const {

	return isValidDomain(u.getDomainType());
}

bool MilitaryBranch::HomeGuard::isValidDomain(DomainTypes d) const {

	return d == DOMAIN_LAND || d == DOMAIN_AIR;
}

bool MilitaryBranch::Army::isValidDomain(DomainTypes d) const {

	return d == DOMAIN_LAND || d == DOMAIN_AIR || d == DOMAIN_IMMOBILE;
}

bool MilitaryBranch::Fleet::isValidDomain(DomainTypes d) const {

	return d == DOMAIN_SEA;
}

bool MilitaryBranch::Cavalry::isValidDomain(DomainTypes d) const {

	return d == DOMAIN_LAND;
}

double MilitaryBranch::HomeGuard::unitPower(CvUnitInfo const& u,
		bool modify) const {

	double r = u.getPowerValue();
	if(modify) {
		if(u.isNoDefensiveBonus())
			r *= 0.66;
		double defMod = 1 + u.getCityDefenseModifier() / 100.0;
		// Prefer potential garrisons
		FOR_EACH_ENUM(Promotion) {
			CvPromotionInfo const& promo = GC.getInfo(eLoopPromotion);
			if(promo.getCityDefensePercent() >= 20
					&& promo.getUnitCombat(u.getUnitCombatType())) {
				defMod += 0.1;
				break;
			}
		}
		r *= defMod;
	}
	return GET_PLAYER(ownerId).uwai().militaryPower(u, r);
}

/* Utility equals power by default (template pattern). */
double MilitaryBranch::unitUtility(CvUnitInfo const& u, double pow) const {

	return pow;
}

double MilitaryBranch::Logistics::unitUtility(CvUnitInfo const& u, double pow) const {

	bool canEnterAllTerrain = true;
	FOR_EACH_ENUM(Terrain) {
		if(u.getTerrainImpassable(eLoopTerrain)) {
			canEnterAllTerrain = false;
			break;
		}
	}
	/* Better to use GET_PLAYER(ownerId).AI_unitImpassableCount(UnitTypes) == 0?
	   But how does one get a UnitTypes from CvUnitInfo again? */
	/* Can't rely solely on cargo space: Galleys are most efficient in that
	   regard, but the City AI doesn't build Galleys once it has Galleons.
	   Could use pow instead of getCargoSpace, but that's a bit confusing. */
	return u.getCargoSpace() + u.getCombat() + u.getMoves()
			+ (canEnterAllTerrain ? 5 : 0);
}

double MilitaryBranch::Army::unitPower(CvUnitInfo const& u, bool modify) const {

	// (Include nukes in army)
	/*if(u.getNukeRange() >= 0)
		return -1;*/
	double r = u.getPowerValue();
	if(modify) {
		if(u.isMostlyDefensive()) // advc.315
			return -1;
		// Prefer potential city raiders
		FOR_EACH_ENUM(Promotion) {
			CvPromotionInfo const& promo = GC.getInfo(eLoopPromotion);
			if(promo.getCityAttackPercent() >= 20 && promo.getUnitCombat(u.getUnitCombatType())) {
				r *= 1.1;
				break;
			}
		}
		/* Military power is already biased towards aggression.
		   No further adjustments needed. */
	}
	return GET_PLAYER(ownerId).uwai().militaryPower(u, r);
}

double MilitaryBranch::Cavalry::unitPower(CvUnitInfo const& u, bool modify) const {

	if(u.getMoves() <= 1 || u.getProductionCost() >= 150 ||
			u.isMostlyDefensive()) // advc.315
		return -1;
	return GET_PLAYER(ownerId).uwai().militaryPower(u);
}

double MilitaryBranch::Fleet::unitPower(CvUnitInfo const& u, bool modify) const {

	double r = GET_PLAYER(ownerId).uwai().militaryPower(u);
	// Avoid selecting Ironclad as typical unit (can't reach enemy cities reliably)
	if(modify) {
		FOR_EACH_ENUM(Terrain) {
			if(u.getTerrainImpassable(eLoopTerrain)) {
				r /= 2;
				break;
			}
		}
		/*  Would like to use CvPlayerAI::AI_unitImpassableCount, but that
			requires a UnitTypes argument. */
	}
	return r;
}

double MilitaryBranch::Logistics::unitPower(CvUnitInfo const& u, bool modify) const {

	if(u.getSpecialCargo() == NO_SPECIALUNIT)
			/*  This would include carriers and subs in Logistics. But I think
				only proper transport ships should count b/c aircraft can't
				conquer cities. */
			//|| u.getDomainCargo() == DOMAIN_AIR)
		return u.getCargoSpace();
	return -1;
}

double MilitaryBranch::NuclearArsenal::unitPower(CvUnitInfo const& u, bool modify) const {

	if(u.getNukeRange() < 0)
		return -1; // I.e. disregard non-nuke units
	double r = u.getPowerValue();
	/*  'modify' should only be used for picking the typical unit. Make sure
		that ICBM gets chosen over TN despite costing twice as much b/c the
		AI tends to invest more in ICBM than TN. */
	if(modify && u.getAirRange() == 0)
		r *= 2.1;
	return GET_PLAYER(ownerId).uwai().militaryPower(u, r);
}

void MilitaryBranch::Army::setUnitsTrained(int number, double pow) {

	this->number = number;
	this->pow = pow;
}

void MilitaryBranch::Army::updateTypicalUnit() {

	MilitaryBranch::updateTypicalUnit();
	bombard = canTrainSiege();
	soften = canTrainCollateral();
}

void MilitaryBranch::Fleet::updateTypicalUnit() {

	MilitaryBranch::updateTypicalUnit();
	if(typicalUnitType != NO_UNIT)
		bombard = getTypicalUnit()->getBombardRate() > 0;
}

bool MilitaryBranch::Army::canTrainSiege() const {

	CvPlayerAI const& owner = GET_PLAYER(ownerId);
	CvCivilization const& civ = owner.getCivilization();
	for(int i = 0; i < civ.getNumUnits(); i++) {
		UnitTypes ut = civ.unitAt(i);
		CvUnitInfo const& u = GC.getInfo(ut);
		if(((u.getBombardRate() > 0 && u.getDomainType() == DOMAIN_LAND) ||
				(u.getBombardRate() > 0 && u.getDomainType() == DOMAIN_AIR)) &&
				owner.AI_canBeExpectedToTrain(ut))
			return true;
	}
	return false;
}

/* Perhaps need such a function for Cavalry as well (in case some mod ever gives
   Cav coll. dmg. or a similar ability) */
bool MilitaryBranch::Army::canTrainCollateral() const {

	CvPlayerAI const& owner = GET_PLAYER(ownerId);
	CvCivilization const& civ = owner.getCivilization();
	for(int i = 0; i < civ.getNumUnits(); i++) {
		UnitTypes ut = civ.unitAt(i);
		CvUnitInfo const& u = GC.getInfo(ut);
		if(u.getCollateralDamage() > 0 &&
				isValidDomain(u.getDomainType()) &&
				owner.AI_canBeExpectedToTrain(ut))
			return true;
	}
	return false;
}

ostream& operator<<(ostream& os, const MilitaryBranch& mb) {

	return mb.out(os);
}

ostream& MilitaryBranch::out(ostream& os) const {

	return os << str();
}

char const* MilitaryBranch::str() const {

	return debugStrings[enumId()];
}

char const* MilitaryBranch::debugStrings[] = {
	"Guard", "Army", "Fleet", "Logistics", "Cavalry",
	"Nuclear", "(unknown branch)"
};

MilitaryBranchTypes MilitaryBranch::enumId() const {

	return NUM_BRANCHES;
}

MilitaryBranchTypes MilitaryBranch::Army::enumId() const {

	return ARMY;
}

MilitaryBranchTypes MilitaryBranch::HomeGuard::enumId() const {

	return HOME_GUARD;
}

MilitaryBranchTypes MilitaryBranch::Fleet::enumId() const {

	return FLEET;
}

MilitaryBranchTypes MilitaryBranch::Logistics::enumId() const {

	return LOGISTICS;
}

MilitaryBranchTypes MilitaryBranch::Cavalry::enumId() const {

	return CAVALRY;
}

MilitaryBranchTypes MilitaryBranch::NuclearArsenal::enumId() const {

	return NUCLEAR;
}

char const* MilitaryBranch::str(MilitaryBranchTypes mb) {

	if(mb < 0)
		mb = NUM_BRANCHES;
	return debugStrings[mb];
}
