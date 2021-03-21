#pragma once

#ifndef MILITARY_BRANCH_H
#define MILITARY_BRANCH_H


/*  advc.104: New class hierarchy. Breaking military power values down into
	branches such as Army and Fleet helps the AI analyze its military prospects. */


/* Containers of military branches should be laid out in this order.
   Can then access a specific branch using these enumerators. */
enum MilitaryBranchTypes {
	HOME_GUARD = 0,
	ARMY,
	FLEET,
	LOGISTICS,
	CAVALRY,
	NUCLEAR,
	NUM_BRANCHES
};

class MilitaryBranch {

public:
	MilitaryBranch(PlayerTypes ownerId);
	MilitaryBranch(MilitaryBranch const& other);
	// Can't do this in the constructor b/c virtual functions need to be called
	virtual void updateTypicalUnit();
	/* Debug output. Derived classes override the (polymorphic) enumId function. */
	  friend std::ostream& operator<<(std::ostream& os, const MilitaryBranch& mb);
	  virtual char const* str() const;
	  virtual MilitaryBranchTypes enumId() const;
	  // Debug string from enum type
	  static char const* str(MilitaryBranchTypes mb);
	/* NULL when no suitable unit can be trained.
	   Replaces CvPlayer::getTypicalUnitValue.
	   The value is computed by updateTypicalUnit; this is just a getter. */
	CvUnitInfo const* getTypicalUnit() const;
	UnitTypes getTypicalUnitType() const;
	/*  pov: The player from whose point of view the typical power/ cost is
		estimated. If NO_PLAYER or equal to ownerId, the true value is returend. */
	  double getTypicalUnitPower(PlayerTypes pov = NO_PLAYER) const;
	  double getTypicalUnitCost(PlayerTypes pov = NO_PLAYER) const;
	// Total military power of units in this branch
	inline double power() const { return pow; }
	void changePower(double delta) { pow += delta; }
	/* True iff 'u' belongs in this milit. branch (at all). The template
	   implementation is based on unitPower. */
	bool canEmploy(CvUnitInfo const& u) const;
	/* Called when a unit is created or destroyed. For tracking the total power
	   of trained units. */
	void updatePower(CvUnitInfo const& u, bool add);
	int num() const { return number; }
	/*  Whether a unit can be trained in this branch that is able to bombard city defenses.
		To be computed in updateTypicalUnit; false by default. */
	bool canBombard() const { return bombard; }
	/*  Whether a unit can be trained in this branch that deals collateral damage.
		To be computed in updateTypicalUnit; false by default. */
	bool canSoftenCityDefenders() const { return soften; }
	/*  Not virtual; all persistent data should be at the base class.
		The copy constructors also assume this. */
	void write(FDataStreamBase* stream);
	void read(FDataStreamBase* stream);

protected:
	/*  Used for selecting a typical unit. The military power of 'u' when used
		within the implementing branch. Should return a negative value if 'u'
		doesn't fit the branch at all. isValidDomain and positive combatStrength
		are ensured by the caller though.
		May want to call the generic UWAI::Civ::militaryPower. */
	virtual double unitPower(CvUnitInfo const& u, bool modify = false) const=0;
	virtual double unitUtility(CvUnitInfo const& u, double pow) const;
	bool isValidDomain(CvUnitInfo const& u) const;
	// Can a unit of this branch have domain 'd'?
	virtual bool isValidDomain(DomainTypes d) const;
	bool canKnowTypicalUnit(PlayerTypes pov) const;
	double estimateProductionCost(CvUnitInfo const& u);
	/*	Vague expectation of how many extra instances will have been produced
		when halfway through with the military buildup that this class helps predict.
		(Could get the era from ownerId, but don't want to include CvPlayer.h.) */
	inline double estimateExtraInstances(scaled eraFactor) const {
		return 1.75 + 1.25 * eraFactor.getDouble();
	}

	PlayerTypes ownerId;
	UnitTypes typicalUnitType;
	double typicalUnitPower;
	double pow;
	int number;
	bool bombard;
	bool soften;

private:
	std::ostream& out(std::ostream& os) const;
	static char const* debugStrings[];
	// Use copy-constructor instead
	MilitaryBranch& operator=(MilitaryBranch const& other);

public: // Concrete classes nested
	class Army; class HomeGuard; class Fleet; class Logistics;
	class Cavalry; class NuclearArsenal;
};


/* Includes air force. Could, at a later point, create a separate branch for
   air units, but I don't think the distinction is crucial for the AI.
   Does not include units guarding cities (HomeGuard), i.e. the Army
   can be fully deployed in an invasion.
   That said, since HomeGuard can't track units trained, Army also tracks
   guard units. Will have to call setUnitsTrained to eventually subtract
   the guard portion.
   "Artillery" may still become a branch. Currently, the military analysis
   assumes that the City AI balances Siege and non-Siege units.
   A separate branch would be helpful for WarUtilityAspect::TacticalSituation. */
class MilitaryBranch::Army : public MilitaryBranch {
public:
	Army(PlayerTypes ownerId) : MilitaryBranch(ownerId) {}
	Army(MilitaryBranch const& other) : MilitaryBranch(other) {}
	void setUnitsTrained(int number, double pow);
	void updateTypicalUnit();
	MilitaryBranchTypes enumId() const;
protected:
	virtual double unitPower(CvUnitInfo const& u, bool modify = false) const;
	virtual bool isValidDomain(DomainTypes d) const;
private:
	bool canTrainSiege() const;
	bool canTrainCollateral() const;
};

/*  City garrisons, including Fighters for air defense. The AI assumes that the
	HomeGuard is not available for invading other civs.
	HomeGuard is not to be used for tracking actual units built. Instead, use
	initUnitsTrained to have HomeGuard compute the current number of guard units
	and their power based on the number and power of all non-naval units. */
class MilitaryBranch::HomeGuard : public MilitaryBranch {
public:
	HomeGuard(PlayerTypes ownerId) : MilitaryBranch(ownerId) {}
	HomeGuard(MilitaryBranch const& other) : MilitaryBranch(other) {}
	// Returns the portion of guard units in the non-naval force
	double initUnitsTrained(int numNonNavalUnits, double powNonNavalUnits);
	MilitaryBranchTypes enumId() const;
protected:
	double unitPower(CvUnitInfo const& u, bool modify = false) const;
	bool isValidDomain(DomainTypes d) const;
};

// Any ships with combat strength, including cargo ships.
class MilitaryBranch::Fleet : public MilitaryBranch {
public:
	Fleet(PlayerTypes ownerId) : MilitaryBranch(ownerId) {}
	Fleet(MilitaryBranch const& other) : MilitaryBranch(other) {}
	void updateTypicalUnit();
	MilitaryBranchTypes enumId() const;
protected:
	double unitPower(CvUnitInfo const& u, bool modify = false) const;
	bool isValidDomain(DomainTypes d) const;
};

/*  Ships for transporting Army units. In theory, transports in other domains
	could work similarly, hence the generic name of the class. Ships transporting
	air units are also included in Logicstics (since Army units can be land- or
	air-based). More sophisticated code would count air and land cargo separately,
	i.e. through a separate AirLogistics branch.
	'Power' actually refers to cargo capacity in this class. Cargo units are also
	covered by Fleet, but, in that context, their combat strength is counted. */
class MilitaryBranch::Logistics : public MilitaryBranch {
public:
	Logistics(PlayerTypes ownerId) : MilitaryBranch(ownerId) {}
	Logistics(MilitaryBranch const& other): MilitaryBranch(other) {}
	MilitaryBranchTypes enumId() const;
protected:
	// Cargo capacity
	double unitPower(CvUnitInfo const& u, bool modify = false) const;
	/* Mix of capacity, power and strength. The pow parameter is actually
	   cargo capacity. */
	double unitUtility(CvUnitInfo const& u, double pow) const;
};

/*  Fast early attackers. Don't have to be mounted (e.g. Impi), but most are.
	Tanks aren't Cavalry; I think fast units lose their special role in the
	later eras b/c of railroads and b/c all units classes become motorized
	eventually.
	Cavalry is included in Army. */
class MilitaryBranch::Cavalry : public MilitaryBranch {
public:
	Cavalry(PlayerTypes ownerId) : MilitaryBranch(ownerId) {}
	Cavalry(MilitaryBranch const& other): MilitaryBranch(other) {}
	MilitaryBranchTypes enumId() const;
protected:
	double unitPower(CvUnitInfo const& u, bool modify = false) const;
	bool isValidDomain(DomainTypes d) const;
};

/* The usefulness of nukes for winning wars is covered by Army.
   This branch is about the strategic element that is unique to nukes. */
class MilitaryBranch::NuclearArsenal : public MilitaryBranch {
public:
	NuclearArsenal(PlayerTypes ownerId): MilitaryBranch(ownerId) {}
	NuclearArsenal(MilitaryBranch const& other): MilitaryBranch(other) {}
	MilitaryBranchTypes enumId() const;
	void updateTypicalUnit(); // override
protected:
	double unitPower(CvUnitInfo const& u, bool modify = false) const;
};

#endif
