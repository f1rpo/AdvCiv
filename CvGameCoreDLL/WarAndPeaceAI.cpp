// <advc.104> New class; see WarAndPeaceAI.h for description

#include "CvGameCoreDLL.h"
#include "WarAndPeaceAI.h"
#include "WarAndPeaceAgent.h"
#include "WarEvaluator.h"
#include "CvGamePlay.h"
#include "CvMap.h"
#include "CvInfos.h"
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
		all AI data is properly initialized and updated. */
	CvGame const& g = GC.getGameINLINE();
	return (!g.isScenario() || g.getElapsedGameTurns() > 1);
}

void WarAndPeaceAI::doXML() {

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

	applyPersonalityWeight();
}

double WarAndPeaceAI::aspectWeight(int xmlId) const {

	if(xmlId < 0 ||  xmlWeights.size() <= (size_t)xmlId)
		return 1;
	return xmlWeights[xmlId] / 100.0;
}

void WarAndPeaceAI::applyPersonalityWeight() {

	int iWeight = GC.getDefineINT("UWAI_PERSONALITY_PERCENT");
	if(iWeight == 100)
		return;
	std::vector<std::vector<int*>*> personalityMatrix;
	int iMembers = -1;
	for(int i = 0; i < GC.getNumLeaderHeadInfos(); i++) {
		if(i == GET_PLAYER(BARBARIAN_PLAYER).getLeaderType())
			continue;
		CvLeaderHeadInfo& kLeader = GC.getLeaderHeadInfo((LeaderHeadTypes)i);
		/*  Basically serialize CvLeaderHeadInfo to avoid writing any more code
			per member variable than necessary. Tempting to use CvLeaderHeadInfo::
			write(FDataStreamBase*) for this, but some members have to be excluded. */
		int* aiPrimitiveMembers[]  = {
			&kLeader.m_iWonderConstructRand,
			&kLeader.m_iBaseAttitude, &kLeader.m_iBasePeaceWeight,
			&kLeader.m_iPeaceWeightRand, &kLeader.m_iWarmongerRespect,
			&kLeader.m_iEspionageWeight, &kLeader.m_iRefuseToTalkWarThreshold,
			&kLeader.m_iNoTechTradeThreshold, &kLeader.m_iTechTradeKnownPercent,
			&kLeader.m_iMaxGoldTradePercent, &kLeader.m_iMaxGoldPerTurnTradePercent,
			&kLeader.m_iCultureVictoryWeight, &kLeader.m_iSpaceVictoryWeight,
			&kLeader.m_iConquestVictoryWeight, &kLeader.m_iDominationVictoryWeight,
			&kLeader.m_iDiplomacyVictoryWeight, &kLeader.m_iMaxWarRand,
			&kLeader.m_iMaxWarNearbyPowerRatio, &kLeader.m_iMaxWarDistantPowerRatio,
			&kLeader.m_iMaxWarMinAdjacentLandPercent, &kLeader.m_iLimitedWarRand,
			&kLeader.m_iLimitedWarPowerRatio, &kLeader.m_iDogpileWarRand,
			&kLeader.m_iMakePeaceRand, &kLeader.m_iDeclareWarTradeRand,
			&kLeader.m_iDemandRebukedSneakProb, &kLeader.m_iDemandRebukedWarProb,
			&kLeader.m_iRazeCityProb, &kLeader.m_iBuildUnitProb,
			&kLeader.m_iBaseAttackOddsChange, &kLeader.m_iAttackOddsChangeRand,
			&kLeader.m_iWorseRankDifferenceAttitudeChange,
			&kLeader.m_iBetterRankDifferenceAttitudeChange,
			&kLeader.m_iCloseBordersAttitudeChange,
			&kLeader.m_iLostWarAttitudeChange, &kLeader.m_iAtWarAttitudeDivisor,
			&kLeader.m_iAtWarAttitudeChangeLimit, &kLeader.m_iAtPeaceAttitudeDivisor,
			&kLeader.m_iAtPeaceAttitudeChangeLimit, &kLeader.m_iSameReligionAttitudeChange,
			&kLeader.m_iSameReligionAttitudeDivisor, &kLeader.m_iSameReligionAttitudeChangeLimit,
			&kLeader.m_iDifferentReligionAttitudeChange, &kLeader.m_iDifferentReligionAttitudeDivisor,
			&kLeader.m_iDifferentReligionAttitudeChangeLimit,
			&kLeader.m_iBonusTradeAttitudeDivisor, &kLeader.m_iBonusTradeAttitudeChangeLimit,
			&kLeader.m_iOpenBordersAttitudeDivisor, &kLeader.m_iOpenBordersAttitudeChangeLimit,
			&kLeader.m_iDefensivePactAttitudeDivisor, &kLeader.m_iDefensivePactAttitudeChangeLimit,
			&kLeader.m_iShareWarAttitudeChange, &kLeader.m_iShareWarAttitudeDivisor,
			&kLeader.m_iShareWarAttitudeChangeLimit, &kLeader.m_iFavoriteCivicAttitudeChange,
			&kLeader.m_iFavoriteCivicAttitudeDivisor, &kLeader.m_iFavoriteCivicAttitudeChangeLimit,
			&kLeader.m_iDemandTributeAttitudeThreshold, &kLeader.m_iNoGiveHelpAttitudeThreshold,
			&kLeader.m_iTechRefuseAttitudeThreshold, &kLeader.m_iStrategicBonusRefuseAttitudeThreshold,
			&kLeader.m_iHappinessBonusRefuseAttitudeThreshold, &kLeader.m_iHealthBonusRefuseAttitudeThreshold,
			&kLeader.m_iMapRefuseAttitudeThreshold,
			&kLeader.m_iDeclareWarRefuseAttitudeThreshold, &kLeader.m_iDeclareWarThemRefuseAttitudeThreshold,
			&kLeader.m_iStopTradingRefuseAttitudeThreshold, &kLeader.m_iStopTradingThemRefuseAttitudeThreshold,
			&kLeader.m_iAdoptCivicRefuseAttitudeThreshold, &kLeader.m_iConvertReligionRefuseAttitudeThreshold,
			&kLeader.m_iOpenBordersRefuseAttitudeThreshold, &kLeader.m_iDefensivePactRefuseAttitudeThreshold,
			&kLeader.m_iPermanentAllianceRefuseAttitudeThreshold, &kLeader.m_iVassalRefuseAttitudeThreshold,
			&kLeader.m_iVassalPowerModifier, &kLeader.m_iFreedomAppreciation,
		};
		int const iPrimitiveMembers = sizeof(aiPrimitiveMembers) / sizeof(int);
		std::vector<int*>* paiPersonalityVector = new std::vector<int*>(
				aiPrimitiveMembers, aiPrimitiveMembers + iPrimitiveMembers);
		for(int j = 0; j < GC.getNumFlavorTypes(); j++)
			paiPersonalityVector->push_back(&kLeader.m_piFlavorValue[j]);
		for(int j = 0; j < NUM_CONTACT_TYPES; j++)
			paiPersonalityVector->push_back(&kLeader.m_piContactRand[j]);
		for(int j = 0; j < NUM_CONTACT_TYPES; j++)
			paiPersonalityVector->push_back(&kLeader.m_piContactDelay[j]);
		for(int j = 0; j < NUM_MEMORY_TYPES; j++)
			paiPersonalityVector->push_back(&kLeader.m_piMemoryDecayRand[j]);
		for(int j = 0; j < NUM_MEMORY_TYPES; j++)
			paiPersonalityVector->push_back(&kLeader.m_piMemoryAttitudePercent[j]);
		for(int j = 0; j < NUM_ATTITUDE_TYPES; j++)
			paiPersonalityVector->push_back(&kLeader.m_piNoWarAttitudeProb[j]);
		for(int j = 0; j < NUM_UNITAI_TYPES; j++)
			paiPersonalityVector->push_back(&kLeader.m_piUnitAIWeightModifier[j]);
		for(int j = 0; j < GC.getNumImprovementInfos(); j++)
			paiPersonalityVector->push_back(&kLeader.m_piImprovementWeightModifier[j]);
		personalityMatrix.push_back(paiPersonalityVector);
		FAssert(iMembers == -1 || iMembers == (int)paiPersonalityVector->size());
		iMembers = (int)paiPersonalityVector->size();
		if(iWeight == 0) { // Clear fav. civic and religion
			kLeader.m_iFavoriteCivic = NO_CIVIC;
			kLeader.m_iFavoriteReligion = NO_RELIGION;
		}
	}
	for(int j = 0; j < iMembers; j++) {
		std::vector<double> distrib;
		for(size_t i = 0; i < personalityMatrix.size(); i++)
			distrib.push_back(*personalityMatrix[i]->at(j));
		double medianValue = ::dMedian(distrib);
		for(size_t i = 0; i < personalityMatrix.size(); i++) {
			*personalityMatrix[i]->at(j) = ::round((medianValue * (100 - iWeight) +
					(*personalityMatrix[i]->at(j)) * iWeight) / 100);
		}
	}
	for(size_t i = 0; i < personalityMatrix.size(); i++)
		delete personalityMatrix[i];
}

// </advc.104>
