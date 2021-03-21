// advc.104: New class; see WarEvalParameters.h for description.

#include "CvGameCoreDLL.h"
#include "WarEvalParameters.h"
#include "CvGameAI.h"
#include "CvTeamAI.h"
#include "AgentIterator.h"


WarEvalParameters::WarEvalParameters(TeamTypes agentId,
		TeamTypes targetId, UWAIReport& report,
		bool ignoreDistraction, PlayerTypes sponsor,
		TeamTypes capitulationTeam) :
	_agentId(agentId), _targetId(targetId), sponsor(sponsor),
	capitulationTeam(capitulationTeam), immediateDoW(false),
	report(report), ignoreDistraction(ignoreDistraction)
{
	consideringPeace = GET_TEAM(_agentId).isAtWar(_targetId);
	FAssert(capitulationTeam == NO_TEAM || consideringPeace);
	if(sponsor != NO_PLAYER)
		immediateDoW = true;
	// To be set by WarEvaluator:
	total = naval = false;
	preparationTime = -1;
}

bool WarEvalParameters::isIgnoreDistraction() const {

	return ignoreDistraction;
}

TeamTypes WarEvalParameters::agentId() const {

	return _agentId;
}

TeamTypes WarEvalParameters::targetId() const {

	return _targetId;
}

UWAIReport& WarEvalParameters::getReport() const {

	return report;
}

bool WarEvalParameters::isConsideringPeace() const {

	return consideringPeace;
}

void WarEvalParameters::setNotConsideringPeace() {

	consideringPeace = false;
}

void WarEvalParameters::addWarAlly(TeamTypes tId) {

	warAllies.insert(tId);
	warAllies.insert(GET_TEAM(tId).getMasterTeam());
	for(TeamIter<MAJOR_CIV,VASSAL_OF> it(tId); it.hasNext(); ++it)
		warAllies.insert(it->getID());
}

bool WarEvalParameters::isExtraTarget(TeamTypes tId) const {

	return extraTargets.count(tId) > 0;
}

void WarEvalParameters::addExtraTarget(TeamTypes tId) {

	extraTargets.insert(tId);
	extraTargets.insert(GET_TEAM(tId).getMasterTeam());
	for(TeamIter<MAJOR_CIV,VASSAL_OF> it(tId); it.hasNext(); ++it)
		extraTargets.insert(it->getID());
}


bool WarEvalParameters::isNoWarVsExtra() const {

	return !extraTargets.empty() && !consideringPeace &&
			!immediateDoW && !GET_TEAM(_agentId).isAtWar(_targetId);
}

bool WarEvalParameters::isWarAlly(TeamTypes tId) const {

	return warAllies.count(tId) > 0;
}

bool WarEvalParameters::isAnyWarAlly() const {

	return !warAllies.empty();
}

void WarEvalParameters::setSponsor(PlayerTypes civId) {

	sponsor = civId;
	if(civId != NO_PLAYER)
		immediateDoW = true;
}

PlayerTypes WarEvalParameters::getSponsor() const {

	return sponsor;
}

bool WarEvalParameters::isTotal() const {

	return total;
}

bool WarEvalParameters::isNaval() const {

	return naval;
}

int WarEvalParameters::getPreparationTime() const {

	return preparationTime;
}

TeamTypes WarEvalParameters::getCapitulationTeam() const {

	return capitulationTeam;
}

int WarEvalParameters::id() const {

	/*  Some 500 mio. possible combinations; fits into a single int.
		Ensure uniqueness through a mixed-base positional system: */
	long r = _targetId + 1; r *= 20;
	r += _agentId + 1; r *= 20;
	r += consideringPeace; r *= 2;
	r += ignoreDistraction; r *= 2;
	r += total; r *= 2;
	r += naval; r *= 2;
	r += preparationTime + 1; r *= 50;
	r += sponsor + 1; r *= 20;
	r += capitulationTeam + 1; r *= 20;
	r += immediateDoW;
	/*  warAllies and extraTargets matter only in situations where the cache
		should be disabled. (Also wouldn't fit into a single int, but long long
		could be used instead.) */
	FAssert(warAllies.empty() && extraTargets.empty());
	return r;
}

void WarEvalParameters::setTotal(bool b) {

	total = b;
}

void WarEvalParameters::setNaval(bool b) {

	naval = b;
}

void WarEvalParameters::setPreparationTime(int t) {

	preparationTime = t;
}

void WarEvalParameters::setImmediateDoW(bool b) {

	immediateDoW = b;
}

bool WarEvalParameters::isImmediateDoW() const {

	return immediateDoW;
}
