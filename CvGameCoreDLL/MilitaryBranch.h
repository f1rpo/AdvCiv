#pragma once

#ifndef MILITARY_BRANCH_H
#define MILITARY_BRANCH_H

#include "CvInfos.h"
#include <sstream>

/* Containers of military branches should be laid out in this order.
   Can then access a specific branch using these literals. */
enum MilitaryBranchTypes {
	HOME_GUARD = 0,
	ARMY,
	FLEET,
	LOGISTICS,
	CAVALRY,
	NUCLEAR,
	NUM_BRANCHES
};

/* <advc.104> New class. Breaking military power down into different branches
   such as Army and Fleet helps the AI in analyzing its military prospects.
   See comments about the concrete sub-classes. The base class is pure virtual. */
class MilitaryBranch {

public:
	MilitaryBranch(PlayerTypes ownerId);
	MilitaryBranch(MilitaryBranch const& other);
	/* Needs to be called after the constructor for the template pattern to work.
	   (updateTypicalUnit calls a virtual function.) A C++ quirk. */
	virtual void updateTypicalUnit();
	/* Debug output. Sub-classes override the (polymorphic) enumId function. */
	  friend std::ostream& operator<<(std::ostream& os, const MilitaryBranch& mb);
	  virtual char const* str() const;
	  virtual MilitaryBranchTypes enumId() const;
	  // Debug string from enum type
	  static char const* str(MilitaryBranchTypes mb);
	/* NULL when no suitable unit can be built. Any returned object
	   is _not_ to be deleted by the caller.
	   Replaces CvPlayer::getTypicalUnitValue.
	   The value is computed on creation; this is just a getter. */
	CvUnitInfo const* getTypicalUnit() const;
	/* Getting a UnitTypes value out of CvUnitInfo is quite an ordeal, hence the
	   extra function. */
	UnitTypes getTypicalUnitType() const;
	double getTypicalUnitPower() const;
	// Military power value of the entire branch.
	double power() const;
	void changePower(double delta);
	/* True iff 'u' belongs in this milit. branch (at all). The template
	   implementation is based on unitPower. */
	bool canEmploy(CvUnitInfo const& u) const;
	/* Called when a unit is created or destroyed. For tracking the total power
	   of trained units. */
	void updatePower(CvUnitInfo const& u, bool add);
	int num() const;
	// False unless subclass sets MilitaryBranch::bombard in updateTypicalUnit.
	bool canBombard() const;
	/*  Through collateral damage.
		False unless subclass sets MilitaryBranch::soften in updateTypicalUnit. */
	bool canSoftenCityDefenders() const;
	/*  Not virtual; the serialized data should be at the base class. The
		copy constructors also assume this. */
	void write(FDataStreamBase* stream);
	void read(FDataStreamBase* stream);

protected:
	/* Used for selecting a typical unit. The power of 'u' within the
	   implementing branch.
	   Should return a negative value if 'u' doesn't fit the branch at all.
	   A matching domain and combatStrength > 0 are ensured by the caller
	   though.
	   A generic function militaryPower(u) is currently located at
	   WarAndPeaceAI::Civ. */
	virtual double unitPower(CvUnitInfo const& u, bool modify = false) const=0;
	/* The City AI doesn't decide which units to build solely based on power. */
	virtual double unitUtility(CvUnitInfo const& u, double pow) const;
	bool isValidDomain(CvUnitInfo const& u) const;
	/* Can a unit of this branch have domain 'd'? */
	virtual bool isValidDomain(DomainTypes d) const;
	int countUnitsWithAI(std::vector<UnitAITypes> aiTypes) const;

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
	// Not implemented; use copy-constructor instead
	MilitaryBranch& operator=(MilitaryBranch const& other);

public: // Concrete sub-classes nested
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
   Artillery may still become a branch. Currently, the military analysis
   assumes that artillery will be built in sensible numbers. Since
   it's a medium-term analysis, that's a fair assumption; the City AI does
   try to balance artillery and non-artillery.
   Might need an Artillery branch for a last-minute DoW check. */
class MilitaryBranch::Army : public MilitaryBranch {
public:
	Army(PlayerTypes ownerId);
	Army(MilitaryBranch const& other);
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
	HomeGuard(PlayerTypes ownerId);
	HomeGuard(MilitaryBranch const& other);
	// Returns the portion of guard units in the non-naval force
	double initUnitsTrained(int numNonNavalUnits, double powNonNavalUnits);
	MilitaryBranchTypes enumId() const;
protected:
	double unitPower(CvUnitInfo const& u, bool modify = false) const;
	bool isValidDomain(DomainTypes d) const;
	// Unit AI types indicative of guard units
	std::vector<UnitAITypes> aiTypes();
};

/* Any ships with combat strength, including cargo ships. */
class MilitaryBranch::Fleet : public MilitaryBranch {
public:
	Fleet(PlayerTypes ownerId);
	Fleet(MilitaryBranch const& other);
	void updateTypicalUnit();
	MilitaryBranchTypes enumId() const;
protected:
	double unitPower(CvUnitInfo const& u, bool modify = false) const;
	bool isValidDomain(DomainTypes d) const;
};

/* Ships for transporting Army units. In theory, transports in other domains
   could work similarly, hence the generic name of the class. Ships
   transporting air units are also Logicstics (since Army units can be land- or
   air-based). More sophisticated code would count air and land cargo separately,
   i.e. through a separate AirLogistics branch.
   'Power' actually refers to cargo capacity in this class. Cargo units
   are also covered by Fleet, but, in this context, their combat strength
   is counted. */
class MilitaryBranch::Logistics : public MilitaryBranch {
public:
	Logistics(PlayerTypes ownerId);
	Logistics(MilitaryBranch const& other);
	MilitaryBranchTypes enumId() const;
protected:
	// Cargo capacity
	double unitPower(CvUnitInfo const& u, bool modify = false) const;
	/* Mix of capacity, power and strength. The pow parameter is actually
	   cargo capacity. */
	double unitUtility(CvUnitInfo const& u, double pow) const;
};

/* Fast early attackers. Don't have to be mounted (e.g. Impi), but most are.
   Tanks aren't Cavalry; I think fast units lose their special role in the
   later eras b/c of railroads and b/c all units classes become motorized
   eventually.
   Cavalry is included in Army. */
class MilitaryBranch::Cavalry : public MilitaryBranch {
public:
	Cavalry(PlayerTypes ownerId);
	Cavalry(MilitaryBranch const& other);
	MilitaryBranchTypes enumId() const;
protected:
	double unitPower(CvUnitInfo const& u, bool modify = false) const;
	bool isValidDomain(DomainTypes d) const;
};

/* The usefulness of nuclear arms for winning wars is covered by Army.
   This branch is about the strategic element that is unique to nuclear arms. */
class MilitaryBranch::NuclearArsenal : public MilitaryBranch {
public:
	NuclearArsenal(PlayerTypes ownerId);
	NuclearArsenal(MilitaryBranch const& other);
	MilitaryBranchTypes enumId() const;
	// The template implementation doesn't work for nukes
	void updateTypicalUnit();
protected:
	double unitPower(CvUnitInfo const& u, bool modify = false) const;
};

// </advc.104>

#endif
