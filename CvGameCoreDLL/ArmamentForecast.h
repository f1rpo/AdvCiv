#pragma once

#ifndef ARMAMENT_FORECAST_H
#define ARMAMENT_FORECAST_H

#include "UWAI.h"
#include "UWAICache.h"

class MilitaryAnalyst;
class UWAIReport;
class CvArea;


/* advc.104: New class. Predicts the military build-up of a civ. Part of
   the military analysis.
   The computation happens in the constructor.
   NB: The aim is not to predict how many units a civ will _own_ on
   a future turn, but how many units will be _trained_, i.e. ignoring losses.
   Losses are handled when resolving the InvasionGraph.
   Disregards financial trouble since unit cost is rarely a limiting factor for
   AI armament. */
class ArmamentForecast {

public:
	/* 'm' belongs to the civ making the forecast ("we"), 'civId' is the civ whose
		armament is being predicted. (Not const b/c ArmamentForecast may add to
		the UWAIReport.)
	   'military': Present military of civId; power values are increased
				   by this class.
	   'peaceScenario': True iff peace is assumed beetween us and our target.
	   'partyAddedRecently': True iff this is the first forecast after adding
		any war parties. Needed in order to decide if a DoW on civId is recent
		(more build-up assumed then). */
	ArmamentForecast(PlayerTypes civId, MilitaryAnalyst& m,
			std::vector<MilitaryBranch*>& military, int timeHorizon,
			double productionPortion, // Remaining after assumed losses of cities
			UWAICache::City const* target, bool peaceScenario,
			bool partyAddedRecently, bool allPartiesKnown, bool noUpgrading);
	double getProductionInvested() const;

private:
	// Can t1 reach t2 or vice versa. Not dependent on civId or m.weId.
	bool canReachEither(TeamTypes t1, TeamTypes t2) const;
	PlayerTypes civId;
	MilitaryAnalyst const& m;
	UWAI::Civ const& uwai;
	UWAIReport& report;
	std::vector<MilitaryBranch*>& military;
	int timeHorizon;
	double productionInvested;

	enum Intensity {
		DECREASED = -1,
		NORMAL = 0,
		INCREASED,
		FULL,
	};
	static char const* strIntensity(Intensity in);
	// Both directly increase the power values in 'military'
	 void predictArmament(int turnsBuildUp, double perTurnProduction,
			/* additionalProduction: Currently, that unit upgrades converted
			   into production. */
			double additionalProduction, Intensity intensity, bool defensive,
			bool navalArmament);
	 double productionFromUpgrades();
	/* The Area AI differentiates between continents, the forecast doesn't
	   (perhaps should in the future).
	   Only considers the Area AI for the continent where the capital is --
	   which is what this function returns.
	   'civId': The civ whose Area AI is returned. Default: m.ourId(). */
	AreaAITypes getAreaAI(PlayerTypes civId = NO_PLAYER) const;
};

#endif
