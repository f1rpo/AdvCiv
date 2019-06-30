// <advc.104> New class hierarchy; see MilitaryBranch.h.

#include "CvGameCoreDLL.h"
#include "MilitaryBranch.h"
#include "WarAndPeaceAgent.h"
#include "CvGameAI.h"
#include "CvPlayerAI.h"

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
	CvPlayerAI& civ = GET_PLAYER(ownerId);
	for(int i = 0; i < GC.getNumUnitClassInfos(); i++) {
		UnitTypes ut = (UnitTypes)(GC.getCivilizationInfo(
				civ.getCivilizationType()).getCivilizationUnits(i));
		if(ut == NO_UNIT)
			continue;
		CvUnitInfo const& u = GC.getUnitInfo(ut);
		// Siege and air units count for power but aren't typical
		if(u.getCombat() == 0 || u.getCombatLimit() < 100 || !isValidDomain(u) ||
				u.getDomainType() == DOMAIN_AIR || u.getDomainType() == DOMAIN_IMMOBILE)
			continue;
		/*  I may want to give some combat unit (e.g. War Elephant) a national limit
			or an instance cost modifier at some point */
		CvUnitClassInfo const& uci = GC.getUnitClassInfo((UnitClassTypes)i);
		int nationalLimit = uci.getMaxPlayerInstances();
		if(nationalLimit >= 0 && nationalLimit <
				(GC.getGame().getCurrentEra() + 1) * 4)
			continue;
		int instanceCostModifier = uci.getInstanceCostModifier();
		if(instanceCostModifier > 5)
			continue;
		/* Could call this for land units as well, but relying on the capital for
		   those is faster, and perhaps more accurate as well. */
		if(u.getDomainType() == DOMAIN_SEA) {
			if(!GET_PLAYER(ownerId).AI_canBeExpectedToTrain(ut))
				continue;
		}
		else {
			CvCity* capital = civ.getCapitalCity();
			if(capital == NULL || !capital->canTrain(ut, false, false, false, false,
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
		/*  CvPlayer::getProductionNeeded would be more accurate, but slower,
			and shouldn't make a difference unless a unit is modded to use
			InstanceCostModifier or UnitExtraCost. */
		if(u.getProductionCost() <= 0) // Apparently acquired through special means
			continue;
		val = utility / u.getProductionCost();
		if(val > bestVal) {
			bestVal = val;
			typicalUnitPower = unitPower(u, false);
			typicalUnitType = ut;
		}
	}
}

void MilitaryBranch::NuclearArsenal::updateTypicalUnit() {

	double bestVal = 0;
	CvPlayerAI& civ = GET_PLAYER(ownerId);
	for(int i = 0; i < GC.getNumUnitClassInfos(); i++) {
		UnitTypes ut = (UnitTypes)(GC.getCivilizationInfo(
				civ.getCivilizationType()).getCivilizationUnits(i));
		if(ut == NO_UNIT) continue;
		CvCity* capital = civ.getCapitalCity();
		if(capital == NULL || !capital->canTrain(ut))
			continue;
		CvUnitInfo const& u = GC.getUnitInfo(ut);
		double unitPow = unitPower(u, true);
		if(unitPow < 0.01)
			continue;
		double utility = unitUtility(u, unitPow);
		if(u.getProductionCost() <= 0)
			continue;
		double val = utility / u.getProductionCost();
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
	return &GC.getUnitInfo(typicalUnitType);
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

int MilitaryBranch::getTypicalUnitCost(PlayerTypes pov) const {

	if(typicalUnitType == NO_UNIT)
		return -1;
	int r = GET_PLAYER(ownerId).getProductionNeeded(typicalUnitType);
	if(canKnowTypicalUnit(pov))
		return r;
	// Underestimate cost
	return ::round(r * 0.85);
}

bool MilitaryBranch::canKnowTypicalUnit(PlayerTypes pov) const {

	if(pov == NO_PLAYER || pov == ownerId || typicalUnitType == NO_UNIT)
		return true;
	if(NO_TECH != (TechTypes)getTypicalUnit()->getPrereqAndTech())
		return true; // Warrior
	if(GET_PLAYER(ownerId).getUnitClassCount((UnitClassTypes)getTypicalUnit()->
			getUnitClassType()) > 0)
		return true; // The unit's in the wild
	if(GET_PLAYER(pov).canSeeTech(ownerId))
		return true; // Tech visible on Foreign Advisor
	return false;
}

double MilitaryBranch::power() const {

	return pow;
}

void MilitaryBranch::changePower(double delta) {

	pow += delta;
}

bool MilitaryBranch::canEmploy(CvUnitInfo const& u) const {

	return u.getCombat() > 0 && isValidDomain(u) && unitPower(u) > 0;
}

bool MilitaryBranch::canBombard() const {

	return bombard;
}

bool MilitaryBranch::canSoftenCityDefenders() const {

	return soften;
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

int MilitaryBranch::num() const {

	return number;
}

bool MilitaryBranch::isValidDomain(DomainTypes d) const {

	return true;
}

double MilitaryBranch::HomeGuard::initUnitsTrained(int numNonNavalUnits,
		double powNonNavalUnits) {

	CvPlayerAI& civ = GET_PLAYER(ownerId);
	number = countUnitsWithAI(aiTypes());
	/* 1.5 per city might be more realistic, but humans tend to use especially
	   weak units as garrisons. */
	if(civ.isHuman())
		number = std::min(civ.getNumCities(), numNonNavalUnits);
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

	return isValidDomain((DomainTypes)u.getDomainType());
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
		for(int i = 0; i < GC.getNumPromotionInfos(); i++) {
			CvPromotionInfo const& prom = GC.getPromotionInfo((PromotionTypes)i);
			if(prom.getCityDefensePercent() >= 20
					&& prom.getUnitCombat(u.getUnitCombatType())) {
				defMod += 0.1;
				break;
			}
		}
		r *= defMod;
	}
	return GET_PLAYER(ownerId).warAndPeaceAI().militaryPower(u, r);
}

/* Utility equals power by default (template pattern). */
double MilitaryBranch::unitUtility(CvUnitInfo const& u, double pow) const {

	return pow;
}

double MilitaryBranch::Logistics::unitUtility(CvUnitInfo const& u, double pow) const {

	bool canEnterAllTerrain = true;
	for(int i = 0; i < GC.getNumTerrainInfos(); i++) {
		if(u.getTerrainImpassable(i)) {
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

	// (Include nukes in army )
	/*if(u.getNukeRange() >= 0)
		return -1;*/
	double r = u.getPowerValue();
	if(modify) {
		if(u.isMostlyDefensive()) // advc.315
			return -1;
		// Prefer potential city raiders
		for(int i = 0; i < GC.getNumPromotionInfos(); i++) {
			CvPromotionInfo const& prom = GC.getPromotionInfo((PromotionTypes)i);
			if(prom.getCityAttackPercent() >= 20
					&& prom.getUnitCombat(u.getUnitCombatType())) {
				r *= 1.1;
				break;
			}
		}
		/* Military power is already biased towards aggression.
		   No further adjustments needed. */
	}
	return GET_PLAYER(ownerId).warAndPeaceAI().militaryPower(u, r);
}

double MilitaryBranch::Cavalry::unitPower(CvUnitInfo const& u, bool modify) const {

	if(u.getMoves() <= 1 || u.getProductionCost() >= 150 ||
			u.isMostlyDefensive()) // advc.315
		return -1;
	return GET_PLAYER(ownerId).warAndPeaceAI().militaryPower(u);
}

double MilitaryBranch::Fleet::unitPower(CvUnitInfo const& u, bool modify) const {

	double r = GET_PLAYER(ownerId).warAndPeaceAI().militaryPower(u);
	// Avoid selecting Ironclad as typical unit (can't reach enemy cities reliably)
	if(modify) {
		for(int i = 0; i < GC.getNumTerrainInfos(); i++)
			if(u.getTerrainImpassable(i)) { r /= 2; break; }
		/* Would like to use CvPlayerAI::AI_unitImpassableCount, but the darn thing
	       requires a UnitTypes argument */
	}
	return r;
}

double MilitaryBranch::Logistics::unitPower(CvUnitInfo const& u,
		bool modify) const {

	if(u.getSpecialCargo() == NO_SPECIALUNIT)
			/*  This would include carriers and subs in Logistics. But I think
				only proper transport ships should count b/c aircraft can't
				conquer cities. */
			//|| u.getDomainCargo() == DOMAIN_AIR)
		return u.getCargoSpace();
	return -1;
}

double MilitaryBranch::NuclearArsenal::unitPower(CvUnitInfo const& u,
		bool modify) const {

	if(u.getNukeRange() < 0)
		return -1; // I.e. disregard non-nuke units
	double r = u.getPowerValue();
	/*  'modify' should only be used for picking the typical unit. Make sure
		that ICBM gets chosen over TN despite costing twice as much b/c the
		AI tends to invest more in ICBM than TN. */
	if(modify && u.getAirRange() == 0)
		r *= 2.1;
	return GET_PLAYER(ownerId).warAndPeaceAI().militaryPower(u, r);
}

int MilitaryBranch::countUnitsWithAI(vector<UnitAITypes> aiTypes) const {

	int r = 0;
	/* The numbers are precomputed. So, even when a lot of aiTypes are added up,
	   this isn't particularly expensive. */
	for(size_t i = 0; i < aiTypes.size(); i++)
		r += GET_PLAYER(ownerId).AI_getNumAIUnits(aiTypes[i]);
	return r;
}

vector<UnitAITypes> MilitaryBranch::HomeGuard::aiTypes() {

	vector<UnitAITypes> r;
	r.push_back(UNITAI_CITY_DEFENSE);
	r.push_back(UNITAI_CITY_COUNTER);
	r.push_back(UNITAI_DEFENSE_AIR);
	r.push_back(UNITAI_CITY_SPECIAL);
	r.push_back(UNITAI_RESERVE);
	return r; // by value
}

MilitaryBranch::HomeGuard::HomeGuard(PlayerTypes ownerId) : MilitaryBranch(ownerId) {}

MilitaryBranch::HomeGuard::HomeGuard(MilitaryBranch const& other) :
		MilitaryBranch(other) {}

MilitaryBranch::Army::Army(PlayerTypes ownerId) : MilitaryBranch(ownerId) {}

void MilitaryBranch::Army::setUnitsTrained(int number, double pow) {

	this->number = number;
	this->pow = pow;
}

MilitaryBranch::Army::Army(MilitaryBranch const& other) :
	MilitaryBranch(other) {}

MilitaryBranch::Fleet::Fleet(PlayerTypes ownerId) : MilitaryBranch(ownerId) {}

MilitaryBranch::Fleet::Fleet(MilitaryBranch const& other) : MilitaryBranch(other) {}

MilitaryBranch::Logistics::Logistics(PlayerTypes ownerId) : MilitaryBranch(ownerId) {}

MilitaryBranch::Logistics::Logistics(MilitaryBranch const& other)
	: MilitaryBranch(other) {}

MilitaryBranch::Cavalry::Cavalry(PlayerTypes ownerId) : MilitaryBranch(ownerId) {}

MilitaryBranch::Cavalry::Cavalry(MilitaryBranch const& other)
	: MilitaryBranch(other) {}

MilitaryBranch::NuclearArsenal::NuclearArsenal(PlayerTypes ownerId)
	: MilitaryBranch(ownerId) {}

MilitaryBranch::NuclearArsenal::NuclearArsenal(MilitaryBranch const& other)
	: MilitaryBranch(other) {}

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

	CvPlayerAI const& civ = GET_PLAYER(ownerId);
	for(int i = 0; i < GC.getNumUnitClassInfos(); i++) {
		UnitTypes ut = (UnitTypes)(GC.getCivilizationInfo(
				civ.getCivilizationType()).getCivilizationUnits(i));
		CvUnitInfo const& u = GC.getUnitInfo(ut);
		if(((u.getBombardRate() > 0 && u.getDomainType() == DOMAIN_LAND) ||
				(u.getBombardRate() > 0 && u.getDomainType() == DOMAIN_AIR)) &&
				civ.AI_canBeExpectedToTrain(ut))
			return true;
	}
	return false;
}

/* Perhaps need such a function for Cavalry as well (in case some mod ever gives
   Cav coll. dmg. or a similar ability) */
bool MilitaryBranch::Army::canTrainCollateral() const {

	CvPlayerAI const& civ = GET_PLAYER(ownerId);
	for(int i = 0; i < GC.getNumUnitClassInfos(); i++) {
		UnitTypes ut = (UnitTypes)(GC.getCivilizationInfo(
				civ.getCivilizationType()).getCivilizationUnits(i));
		CvUnitInfo const& u = GC.getUnitInfo(ut);
		if(u.getCollateralDamage() > 0 &&
				isValidDomain((DomainTypes)u.getDomainType()) &&
				civ.AI_canBeExpectedToTrain(ut))
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

// </advc.104>
