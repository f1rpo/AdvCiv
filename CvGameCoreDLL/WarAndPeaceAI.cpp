// <advc.104> New class; see WarAndPeaceAI.h for description

#include "CvGameCoreDLL.h"
#include "WarAndPeaceAI.h"
#include "WarAndPeaceAgent.h"
#include "WarEvaluator.h"
#include "CvGamePlay.h"
#include "CvMap.h"
#include "CvDLLInterfaceIFaceBase.h"
#include <iterator>

/*  advc.make: Include this for debugging with Visual Leak Detector
	(if installed). Doesn't matter which file includes it; preferrable a cpp file
	such as this b/c it doesn't cause (much) unnecessary recompilation this way. */
//#include <vld.h>

using std::vector;

WarAndPeaceAI::WarAndPeaceAI() : enabled(false), inBackgr(false) {}

void WarAndPeaceAI::invalidateUICache() {

	WarEvaluator::clearCache();
}

void WarAndPeaceAI::setUseKModAI(bool b) {

	enabled = !b;
}

void WarAndPeaceAI::setInBackground(bool b) {

	inBackgr = b;
}

vector<PlayerTypes>& WarAndPeaceAI::properCivs() {

	return _properCivs;
}

vector<TeamTypes>& WarAndPeaceAI::properTeams() {

	return _properTeams;
}

void WarAndPeaceAI::update() {

	_properTeams.clear();
	_properCivs.clear();
	for(TeamTypes tt = (TeamTypes)0; tt < MAX_CIV_TEAMS;
			tt = (TeamTypes)(tt + 1)) {
		CvTeamAI& t = GET_TEAM(tt);
		if(tt != NO_TEAM && !t.isBarbarian() && !t.isMinorCiv() && t.isAlive())
			_properTeams.push_back(tt);
	}
	for(PlayerTypes p = (PlayerTypes)0; p < MAX_CIV_PLAYERS;
			p = (PlayerTypes)(p + 1)) {
		CvPlayerAI& civ = GET_PLAYER(p);
		if(p != NO_PLAYER && civ.isAlive() && !civ.isBarbarian() &&
				!civ.isMinorCiv())
			_properCivs.push_back(p);
	}
	for(size_t i = 0; i < _properTeams.size(); i++)
		GET_TEAM(getWPAI._properTeams[i]).warAndPeaceAI().updateMembers();
	WarEvaluator::clearCache();
}

void WarAndPeaceAI::processNewCivInGame(PlayerTypes newCivId) {

	update();
	TEAMREF(newCivId).warAndPeaceAI().init(TEAMID(newCivId));
	WarAndPeaceAI::Civ& newAI = GET_PLAYER(newCivId).warAndPeaceAI();
	newAI.init(newCivId);
	// Need to set the typical units before updating the caches of the old civs
	newAI.getCache().updateTypicalUnits();
	for(size_t i = 0; i < _properTeams.size(); i++)
		GET_TEAM(_properTeams[i]).warAndPeaceAI().turnPre();
}

bool WarAndPeaceAI::isEnabled(bool inBackground) const{

	if(!enabled)
		return false;
	return (inBackground == inBackgr);
}

void WarAndPeaceAI::read(FDataStreamBase* stream) {

	stream->Read(&enabled);
}

void WarAndPeaceAI::write(FDataStreamBase* stream) {

	stream->Write(enabled);
}

int WarAndPeaceAI::maxLandDist() const {

	// Faster speed of ships now covered by estimateMovementSpeed
	return maxSeaDist(); //- 2;
}

int WarAndPeaceAI::maxSeaDist() const {

	CvMap const& m = GC.getMapINLINE();
	int r = 15;
	// That's true for Large and Huge maps
	if(m.getGridWidth() > 100 || m.getGridHeight() > 100)
		r = 18;
	if(!m.isWrapXINLINE() && !m.isWrapYINLINE())
		r = (r * 6) / 5;
	return r;
}

bool WarAndPeaceAI::isUpdated() const {

	/*  In scenarios, CvTeamAI functions aren't properly called during the first
		turn. Should skip war planning in the first two turns to make sure that
		all AI data are properly initialized and updated.
		Not sure how to tell if this is a scenario. Could track whether CvTeamAI
		functions have been called, I guess, but don't need war planning in the
		first two turns anyway. */
	return GC.getGameINLINE().getElapsedGameTurns() > 1;
}

void WarAndPeaceAI::cacheXML() {

	/*  Would be so much more elegant to store the weights in the WarUtilityAspect
		classes, but these are only initialized during war evaluation, whereas
		the caching should happen just once at game start. The way I'm implementing
		it now, the numbers returned by WarUtilityAspect::xmlId need to correspond
		to the call order in this function - which sucks. */
	xmlWeights.push_back(GC.getDefineINT("UWAI_WEIGHT_GREED_FOR_ASSETS"));
	xmlWeights.push_back(GC.getDefineINT("UWAI_WEIGHT_GREED_FOR_VASSALS"));
	xmlWeights.push_back(GC.getDefineINT("UWAI_WEIGHT_GREED_FOR_SPACE"));
	xmlWeights.push_back(GC.getDefineINT("UWAI_WEIGHT_GREED_FOR_CASH"));
	xmlWeights.push_back(GC.getDefineINT("UWAI_WEIGHT_LOATHING"));
	xmlWeights.push_back(GC.getDefineINT("UWAI_WEIGHT_MILITARY_VICTORY"));
	xmlWeights.push_back(GC.getDefineINT("UWAI_WEIGHT_PRESERVATION_OF_PARTNERS"));
	xmlWeights.push_back(GC.getDefineINT("UWAI_WEIGHT_RECONQUISTA"));
	xmlWeights.push_back(GC.getDefineINT("UWAI_WEIGHT_REBUKE"));
	xmlWeights.push_back(GC.getDefineINT("UWAI_WEIGHT_FIDELITY"));
	xmlWeights.push_back(GC.getDefineINT("UWAI_WEIGHT_HIRED_HAND"));
	xmlWeights.push_back(GC.getDefineINT("UWAI_WEIGHT_BORDER_DISPUTES"));
	xmlWeights.push_back(GC.getDefineINT("UWAI_WEIGHT_SUCKING_UP"));
	xmlWeights.push_back(GC.getDefineINT("UWAI_WEIGHT_PREEMPTIVE_WAR"));
	xmlWeights.push_back(GC.getDefineINT("UWAI_WEIGHT_KING_MAKING"));
	xmlWeights.push_back(GC.getDefineINT("UWAI_WEIGHT_EFFORT"));
	xmlWeights.push_back(GC.getDefineINT("UWAI_WEIGHT_RISK"));
	xmlWeights.push_back(GC.getDefineINT("UWAI_WEIGHT_ILL_WILL"));
	xmlWeights.push_back(GC.getDefineINT("UWAI_WEIGHT_AFFECTION"));
	xmlWeights.push_back(GC.getDefineINT("UWAI_WEIGHT_DISTRACTION"));
	xmlWeights.push_back(GC.getDefineINT("UWAI_WEIGHT_PUBLIC_OPPOSITION"));
	xmlWeights.push_back(GC.getDefineINT("UWAI_WEIGHT_REVOLTS"));
	xmlWeights.push_back(GC.getDefineINT("UWAI_WEIGHT_ULTERIOR_MOTIVES"));
	xmlWeights.push_back(GC.getDefineINT("UWAI_WEIGHT_FAIR_PLAY"));
	xmlWeights.push_back(GC.getDefineINT("UWAI_WEIGHT_BELLICOSITY"));
	xmlWeights.push_back(GC.getDefineINT("UWAI_WEIGHT_TACTICAL_SITUATION"));
}

double WarAndPeaceAI::aspectWeight(int xmlId) const {

	if(xmlId < 0 ||  xmlWeights.size() <= (size_t)xmlId)
		return 1;
	return xmlWeights[xmlId] / 100.0;
}

// </advc.104>
