#include "CvGameCoreDLL.h"
#include "WarEvaluator.h"
#include "UWAIAgent.h"
#include "WarUtilityAspect.h"
#include "MilitaryAnalyst.h"
#include "UWAICache.h" // <!-- custom: Naval-opportunity simulation diagnostics inspect the same cached military branches consumed by UWAI; no behavior change. See KI#53.6. (ChatGPT-5.6-Sol) -->
#include "UWAILogMuteState.h" // <!-- custom: Concrete nested-mute state is needed because WarEvaluator suppresses comparison-only sub-evaluations; emitted rows still go directly to BBAI. (ChatGPT-5.6-Sol) -->
#include "WarEvalParameters.h"
#include "CoreAI.h"
#include "CvInfo_GameOption.h"
#include "CvCity.h" // <!-- custom: Nearby-overseas WAR diagnostics compare exact nearest city distance without changing UWAI behavior. See KI#53.6. (ChatGPT-5.6-Sol) -->
#include "CvMap.h" // <!-- custom: Required for the wrap-aware plotDistance used by the nearby-overseas WAR diagnostic. See KI#53.6. (GPT-5.6-Sol) -->
#include "BBAILog.h" // <!-- custom: UWAI evaluator diagnostics use cached SAS_BBAI subsystem levels and the unified BBAI sink. See KI#505.3. (ChatGPT-5.6-Sol) -->
#include "CvGameCoreUtils.h"

using std::vector;
using std::string;
using std::ostringstream;

/*	When a player trades with the AI, the EXE asks the DLL to compute trade values
	several times in a row (usually about a dozen times), which is enough to cause
	a noticeable delay. Therefore this primitive caching mechanism. The cache is
	always used on the Trade screen; in other (async) UI contexts, it has to be enabled
	explicitly through the bUseCache param of the constructor, or the static
	enableCache function. As for the latter, it's important to call disableCache
	before returning to a synchronized context b/c the cache doesn't work in all
	situations (see WarEvalParameters::getID) and isn't stored in savegames. */
bool WarEvaluator::m_bCheckCache = false;
bool WarEvaluator::m_bCacheCleared = true;
/*	Cache size: Calls alternate between naval/ non-naval,
	limited/ total or mutual war utility of two civs,
	so caching just the last result isn't effective. */
#define iCACHE_SZ 16
namespace
{
	WarEvalParamID aiLastCallParams[iCACHE_SZ];
	int aiLastCallResult[iCACHE_SZ];
	int iLastIndex;

	// <!-- custom: Keep the naval-opportunity drill intentionally narrower than ordinary WAR level-3 logging: only a true-overseas target within the fixed investigation distance, no stronger than the fixed power threshold, and for which the agent already owns assault transports.
	// The diagnostic never changes utility or consumes RNG. See KI#53.6. (ChatGPT-5.6-Sol + GPT-5.6-Sol) -->
	int getSASBBAINavalOpportunityNearestCityDistance(TeamTypes eFrom, TeamTypes eTo)
	{
		int iBestDistance = MAX_INT;
		for (MemberAIIter itFrom(eFrom); itFrom.hasNext(); ++itFrom)
		{
			FOR_EACH_CITY(pFromCity, *itFrom)
			{
				for (MemberAIIter itTo(eTo); itTo.hasNext(); ++itTo)
				{
					FOR_EACH_CITY(pToCity, *itTo)
					{
						iBestDistance = std::min(iBestDistance, plotDistance(pFromCity->getX(), pFromCity->getY(), pToCity->getX(), pToCity->getY()));
					}
				}
			}
		}
		return (iBestDistance == MAX_INT ? -1 : iBestDistance);
	}

	int getSASBBAITeamUnitAICount(TeamTypes eTeam, UnitAITypes eUnitAI)
	{
		int iCount = 0;
		for (MemberAIIter itMember(eTeam); itMember.hasNext(); ++itMember)
			iCount += itMember->AI_totalUnitAIs(eUnitAI);
		return iCount;
	}

	int getSASBBAITeamBranchPower(TeamTypes eTeam, MilitaryBranchTypes eBranch)
	{
		int iPower = 0;
		for (MemberAIIter itMember(eTeam); itMember.hasNext(); ++itMember)
		{
			MilitaryBranch const* pBranch = itMember->uwai().getCache().getPowerValues()[eBranch];
			if (pBranch != NULL)
				iPower += pBranch->power().round();
		}
		return iPower;
	}

	int getSASBBAITeamSimulatedLostPower(MilitaryAnalyst const& kAnalysis, TeamTypes eTeam, MilitaryBranchTypes eBranch)
	{
		scaled rPower;
		for (MemberIter itMember(eTeam); itMember.hasNext(); ++itMember)
			rPower += kAnalysis.lostPower(itMember->getID(), eBranch);
		return rPower.round();
	}

	int getSASBBAITeamSimulatedGainedPower(MilitaryAnalyst const& kAnalysis, TeamTypes eTeam, MilitaryBranchTypes eBranch)
	{
		scaled rPower;
		for (MemberIter itMember(eTeam); itMember.hasNext(); ++itMember)
			rPower += kAnalysis.gainedPower(itMember->getID(), eBranch);
		return rPower.round();
	}

	int getSASBBAITeamSimulatedMilitaryProduction(MilitaryAnalyst const& kAnalysis, TeamTypes eTeam)
	{
		scaled rProduction;
		for (MemberIter itMember(eTeam); itMember.hasNext(); ++itMember)
			rProduction += kAnalysis.militaryProduction(itMember->getID());
		return rProduction.round();
	}

	int getSASBBAIConquestsFromTeam(MilitaryAnalyst const& kAnalysis, PlayerTypes eConqueror, TeamTypes eVictimTeam)
	{
		int iCount = 0;
		CitySet const& kConquests = kAnalysis.conqueredCities(eConqueror);
		for (CitySetIter itCity = kConquests.begin(); itCity != kConquests.end(); ++itCity)
		{
			for (MemberIter itVictim(eVictimTeam); itVictim.hasNext(); ++itVictim)
			{
				if (kAnalysis.lostCities(itVictim->getID()).count(*itCity) > 0)
				{
					iCount++;
					break;
				}
			}
		}
		return iCount;
	}

	int getSASBBAITeamConquestsFromPlayer(MilitaryAnalyst const& kAnalysis, TeamTypes eConquerorTeam, PlayerTypes eVictim)
	{
		int iCount = 0;
		CitySet const& kVictimLosses = kAnalysis.lostCities(eVictim);
		for (MemberIter itConqueror(eConquerorTeam); itConqueror.hasNext(); ++itConqueror)
		{
			CitySet const& kConquests = kAnalysis.conqueredCities(itConqueror->getID());
			for (CitySetIter itCity = kConquests.begin(); itCity != kConquests.end(); ++itCity)
			{
				if (kVictimLosses.count(*itCity) > 0)
					iCount++;
			}
		}
		return iCount;
	}

	int getSASBBAITeamSimulatedCityLosses(MilitaryAnalyst const& kAnalysis, TeamTypes eTeam)
	{
		int iCount = 0;
		for (MemberIter itMember(eTeam); itMember.hasNext(); ++itMember)
			iCount += kAnalysis.lostCities(itMember->getID()).size();
		return iCount;
	}

	void logSASBBAINavalOpportunitySimulation(WarEvalParameters const& kParams, MilitaryAnalyst const& kAnalysis, PlayerTypes eAgentPlayer)
	{
		TeamTypes const eAgentTeam = kParams.getAgent();
		TeamTypes const eTargetTeam = kParams.getTarget();
		logBBAI("WAR_NAVAL_SIMULATION turn=%d agentTeam=%d targetTeam=%d agentPlayer=%d total=%d prepTurns=%d simTurns=%d ourConquestsFromTarget=%d targetConquestsFromUs=%d ourCityLosses=%d targetCityLosses=%d targetCapitulates=%d ourArmyPower=%d ourFleetPower=%d ourLogisticsPower=%d targetArmyPower=%d targetFleetPower=%d targetLogisticsPower=%d ourArmyLost=%d ourFleetLost=%d ourLogisticsLost=%d targetArmyLost=%d targetFleetLost=%d targetLogisticsLost=%d ourArmyGained=%d ourFleetGained=%d ourLogisticsGained=%d targetArmyGained=%d targetFleetGained=%d targetLogisticsGained=%d ourMilitaryProduction=%d targetMilitaryProduction=%d",
			GC.getGame().getGameTurn(), eAgentTeam, eTargetTeam, eAgentPlayer, kParams.isTotal(), kParams.getPreparationTime(),
			kAnalysis.turnsSimulated(), getSASBBAIConquestsFromTeam(kAnalysis, eAgentPlayer, eTargetTeam),
			getSASBBAITeamConquestsFromPlayer(kAnalysis, eTargetTeam, eAgentPlayer), (int)kAnalysis.lostCities(eAgentPlayer).size(),
			getSASBBAITeamSimulatedCityLosses(kAnalysis, eTargetTeam), kAnalysis.getCapitulationsAccepted(eAgentTeam).count(eTargetTeam) > 0,
			getSASBBAITeamBranchPower(eAgentTeam, ARMY), getSASBBAITeamBranchPower(eAgentTeam, FLEET),
			getSASBBAITeamBranchPower(eAgentTeam, LOGISTICS), getSASBBAITeamBranchPower(eTargetTeam, ARMY),
			getSASBBAITeamBranchPower(eTargetTeam, FLEET), getSASBBAITeamBranchPower(eTargetTeam, LOGISTICS),
			getSASBBAITeamSimulatedLostPower(kAnalysis, eAgentTeam, ARMY), getSASBBAITeamSimulatedLostPower(kAnalysis, eAgentTeam, FLEET),
			getSASBBAITeamSimulatedLostPower(kAnalysis, eAgentTeam, LOGISTICS),
			getSASBBAITeamSimulatedLostPower(kAnalysis, eTargetTeam, ARMY), getSASBBAITeamSimulatedLostPower(kAnalysis, eTargetTeam, FLEET),
			getSASBBAITeamSimulatedLostPower(kAnalysis, eTargetTeam, LOGISTICS),
			getSASBBAITeamSimulatedGainedPower(kAnalysis, eAgentTeam, ARMY),
			getSASBBAITeamSimulatedGainedPower(kAnalysis, eAgentTeam, FLEET),
			getSASBBAITeamSimulatedGainedPower(kAnalysis, eAgentTeam, LOGISTICS),
			getSASBBAITeamSimulatedGainedPower(kAnalysis, eTargetTeam, ARMY),
			getSASBBAITeamSimulatedGainedPower(kAnalysis, eTargetTeam, FLEET),
			getSASBBAITeamSimulatedGainedPower(kAnalysis, eTargetTeam, LOGISTICS),
			getSASBBAITeamSimulatedMilitaryProduction(kAnalysis, eAgentTeam),
			getSASBBAITeamSimulatedMilitaryProduction(kAnalysis, eTargetTeam));
	}

	bool isSASBBAINavalOpportunityCandidate(WarEvalParameters const& kParams, CvTeamAI const& kAgent, CvTeamAI const& kTarget, bool bNaval, int& iNearestCityDistance)
	{
		if (!bNaval || kParams.isConsideringPeace() || kAgent.isAtWar(kTarget.getID()) || kParams.getSponsor() != NO_PLAYER || kAgent.AI_isLandTarget(kTarget.getID()))
			return false;
		const int iMaxTargetPowerPercent = 100;
		const int iMaxNearestCityDistance = 15;
		if (getSASBBAITeamUnitAICount(kAgent.getID(), UNITAI_ASSAULT_SEA) <= 0)
			return false;
		int const iTargetPowerPercent = (100 * kTarget.getDefensivePower(kAgent.getID())) / std::max(1, kAgent.getPower(true));
		if (iTargetPowerPercent > iMaxTargetPowerPercent)
			return false;
		iNearestCityDistance = getSASBBAINavalOpportunityNearestCityDistance(kAgent.getID(), kTarget.getID());
		return (iNearestCityDistance >= 0 && iNearestCityDistance <= iMaxNearestCityDistance);
	}

	bool hasSASBBAIHardRejectAspect(std::vector<int> const& aiUtilities)
	{
		for (size_t i = 0; i < aiUtilities.size(); i++)
		{
			if (aiUtilities[i] <= -100000)
				return true;
		}
		return false;
	}

	void appendSASBBAIWarAspectUtilities(std::ostringstream& kOut, std::vector<CvString> const& asNames, std::vector<int> const& aiUtilities)
	{
		bool bFirst = true;
		for (size_t i = 0; i < aiUtilities.size(); i++)
		{
			if (aiUtilities[i] == 0)
				continue;
			if (!bFirst)
				kOut << ",";
			kOut << asNames[i].GetCString() << ":" << aiUtilities[i];
			bFirst = false;
		}
	}

	void logSASBBAINavalOpportunity(WarEvalParameters const& kParams, CvTeamAI const& kAgent, CvTeamAI const& kTarget, WarPlanTypes eWarPlan, int iPreparationTime, int iNearestCityDistance, int iWarScenarioUtility, int iPeaceScenarioUtility, int iFinalUtility, std::vector<CvString> const& asWarAspectNames, std::vector<int> const& aiWarAspectUtilities, std::vector<CvString> const& asPeaceAspectNames, std::vector<int> const& aiPeaceAspectUtilities)
	{
		std::ostringstream warComponents;
		std::ostringstream peaceComponents;
		appendSASBBAIWarAspectUtilities(warComponents, asWarAspectNames, aiWarAspectUtilities);
		appendSASBBAIWarAspectUtilities(peaceComponents, asPeaceAspectNames, aiPeaceAspectUtilities);
		int const iOurPower = std::max(1, kAgent.getPower(true));
		int const iTargetPower = kTarget.getDefensivePower(kAgent.getID());
		logBBAI("WAR_NAVAL_OPPORTUNITY turn=%d agentTeam=%d targetTeam=%d warPlan=%s finalUtility=%d warScenarioUtility=%d peaceScenarioUtility=%d prepTurns=%d attitude=%d attitudeValue=%d closeness=%d nearestCityDistance=%d ourPower=%d targetPower=%d targetPowerPercent=%d ourCities=%d targetCities=%d ourWars=%d targetWars=%d assaultTransports=%d attackUnits=%d attackCityUnits=%d warComponents=\"%s\" peaceComponents=\"%s\"",
			GC.getGame().getGameTurn(), kAgent.getID(), kTarget.getID(), getSASWarPlanType(eWarPlan), iFinalUtility, iWarScenarioUtility,
			iPeaceScenarioUtility, iPreparationTime, kAgent.AI_getAttitude(kTarget.getID()), kAgent.AI_getAttitudeVal(kTarget.getID()),
			kAgent.AI_teamCloseness(kTarget.getID()), iNearestCityDistance, iOurPower, iTargetPower, (100 * iTargetPower) / iOurPower,
			kAgent.getNumCities(), kTarget.getNumCities(), kAgent.getNumWars(true, true), kTarget.getNumWars(true, true),
			getSASBBAITeamUnitAICount(kAgent.getID(), UNITAI_ASSAULT_SEA), getSASBBAITeamUnitAICount(kAgent.getID(), UNITAI_ATTACK),
			getSASBBAITeamUnitAICount(kAgent.getID(), UNITAI_ATTACK_CITY), warComponents.str().c_str(), peaceComponents.str().c_str());
	}

	int getSASHighWarUtilityLogThreshold()
	{
		static const int iThreshold = GC.getDefineINT("SAS_UWAI_HIGH_UTILITY_LOG_THRESHOLD");
		return iThreshold;
	}

	bool isSASHighWarUtility(int iUtility)
	{
		int const iThreshold = getSASHighWarUtilityLogThreshold();
		return (iUtility >= iThreshold || iUtility <= -iThreshold);
	}

	bool isSASSuspiciousPeaceLead(WarEvalParameters const& kParams)
	{
		if (!kParams.isConsideringPeace())
			return false;
		CvTeamAI& kAgent = GET_TEAM(kParams.getAgent());
		CvTeamAI& kTarget = GET_TEAM(kParams.getTarget());
		return (kAgent.isAtWar(kTarget.getID()) && kAgent.getNumCities() > kTarget.getNumCities() &&
			kAgent.getPower(true) * 2 >= kTarget.getPower(true) * 3 &&
			kAgent.AI_getWarSuccess(kTarget.getID()) >= kTarget.AI_getWarSuccess(kAgent.getID()) + 25);
	}

	void logSASBBAISuspiciousPeaceScenario(WarEvalParameters const& kParams, WarPlanTypes eWarPlan, bool bNaval, int iPreparationTime, char const* szScenario, int iScenarioUtility, std::vector<CvString> const& asAspectNames, std::vector<int> const& aiAspectUtilities)
	{
		CvTeamAI& kAgent = GET_TEAM(kParams.getAgent());
		CvTeamAI& kTarget = GET_TEAM(kParams.getTarget());
		ostringstream componentList;
		bool bFirstComponent = true;
		for (size_t i = 0; i < aiAspectUtilities.size(); i++)
		{
			if (aiAspectUtilities[i] == 0)
				continue;
			if (!bFirstComponent)
				componentList << ",";
			componentList << asAspectNames[i].GetCString() << ":" << aiAspectUtilities[i];
			bFirstComponent = false;
		}
		logBBAI("WAR_PEACE_UTILITY_SCENARIO turn=%d background=%d agentTeam=%d targetTeam=%d warPlan=%s scenario=%s scenarioUtility=%d naval=%d prepTurns=%d ourPower=%d targetPower=%d ourCities=%d targetCities=%d ourWarSuccess=%d targetWarSuccess=%d components=\"%s\"",
			GC.getGame().getGameTurn(), getUWAI().isEnabled(true), kAgent.getID(), kTarget.getID(), getSASWarPlanType(eWarPlan), szScenario,
			iScenarioUtility, bNaval, iPreparationTime, kAgent.getPower(true), kTarget.getPower(true), kAgent.getNumCities(),
			kTarget.getNumCities(), kAgent.AI_getWarSuccess(kTarget.getID()).round(), kTarget.AI_getWarSuccess(kAgent.getID()).round(),
			componentList.str().c_str());
	}

	void logSASBBAISuspiciousPeaceFinal(WarEvalParameters const& kParams, WarPlanTypes eWarPlan, bool bNaval, int iPreparationTime, int iWarScenarioUtility, int iPeaceScenarioUtility, int iFinalUtility)
	{
		CvTeamAI const& kAgent = GET_TEAM(kParams.getAgent());
		CvTeamAI const& kTarget = GET_TEAM(kParams.getTarget());
		logBBAI("WAR_PEACE_UTILITY turn=%d background=%d agentTeam=%d targetTeam=%d warPlan=%s finalUtility=%d warScenarioUtility=%d peaceScenarioUtility=%d naval=%d prepTurns=%d ourCities=%d targetCities=%d",
			GC.getGame().getGameTurn(), getUWAI().isEnabled(true), kAgent.getID(), kTarget.getID(), getSASWarPlanType(eWarPlan),
			iFinalUtility, iWarScenarioUtility, iPeaceScenarioUtility, bNaval, iPreparationTime, kAgent.getNumCities(),
			kTarget.getNumCities());
	}

	void logSASBBAIHighWarUtilityScenario(WarEvalParameters const& kParams, WarPlanTypes eWarPlan, bool bNaval, int iPreparationTime, char const* szScenario, int iScenarioUtility, std::vector<CvString> const& asAspectNames, std::vector<int> const& aiAspectUtilities)
	{
		CvTeamAI const& kAgent = GET_TEAM(kParams.getAgent());
		CvTeamAI const& kTarget = GET_TEAM(kParams.getTarget());
		logBBAI("WAR_UTILITY_SCENARIO_HIGH turn=%d agentTeam=%d targetTeam=%d warPlan=%s scenario=%s scenarioUtility=%d naval=%d prepTurns=%d ourCities=%d targetCities=%d ourWars=%d targetWars=%d",
			GC.getGame().getGameTurn(), kAgent.getID(), kTarget.getID(), getSASWarPlanType(eWarPlan), szScenario, iScenarioUtility, bNaval,
			iPreparationTime, kAgent.getNumCities(), kTarget.getNumCities(), kAgent.getNumWars(true, true), kTarget.getNumWars(true, true));
		for (size_t i = 0; i < aiAspectUtilities.size(); i++)
		{
			if (aiAspectUtilities[i] != 0)
			{
				logBBAI("WAR_UTILITY_SCENARIO_HIGH_COMPONENT turn=%d agentTeam=%d targetTeam=%d warPlan=%s scenario=%s aspect=%s utility=%d",
						GC.getGame().getGameTurn(), kAgent.getID(), kTarget.getID(), getSASWarPlanType(eWarPlan), szScenario, asAspectNames[i].GetCString(), aiAspectUtilities[i]);
			}
		}
	}

	void logSASBBAIHighWarUtilityFinal(WarEvalParameters const& kParams, WarPlanTypes eWarPlan, bool bNaval, int iPreparationTime, int iWarScenarioUtility, int iPeaceScenarioUtility, int iFinalUtility)
	{
		CvTeamAI const& kAgent = GET_TEAM(kParams.getAgent());
		CvTeamAI const& kTarget = GET_TEAM(kParams.getTarget());
		logBBAI("WAR_UTILITY_HIGH turn=%d agentTeam=%d targetTeam=%d warPlan=%s finalUtility=%d warScenarioUtility=%d peaceScenarioUtility=%d naval=%d prepTurns=%d ourCities=%d targetCities=%d ourWars=%d targetWars=%d",
			GC.getGame().getGameTurn(), kAgent.getID(), kTarget.getID(), getSASWarPlanType(eWarPlan), iFinalUtility, iWarScenarioUtility,
			iPeaceScenarioUtility, bNaval, iPreparationTime, kAgent.getNumCities(), kTarget.getNumCities(), kAgent.getNumWars(true, true),
			kTarget.getNumWars(true, true));
	}
}

void WarEvaluator::enableCache()
{
	m_bCheckCache = true;
	m_bCacheCleared = false;
}

void WarEvaluator::disableCache()
{
	m_bCheckCache = false;
}

void WarEvaluator::clearCache()
{
	if (m_bCacheCleared) // Just to make sure that repeated clears don't waste time
		return;
	m_bCacheCleared = true;
	for (int i = 0; i < iCACHE_SZ; i++)
		aiLastCallResult[i] = MIN_INT;
}


WarEvaluator::WarEvaluator(WarEvalParameters& kWarEvalParams, bool bUseCache)
// <!-- custom: Keep only the evaluation-wide nested mute state from the former report plumbing; all emitted diagnostics now use BBAI directly. (ChatGPT-5.6-Sol) -->
:	m_kParams(kWarEvalParams), m_kLogMuteState(m_kParams.getLogMuteState()),
	m_kAgent(GET_TEAM(m_kParams.getAgent())),
	m_kTarget(GET_TEAM(m_kParams.getTarget())),
	m_bPeaceScenario(false), m_bUseCache(bUseCache), m_bSASLogSuspiciousPeace(false), m_bSASLogNavalOpportunity(false), m_iSASNavalOpportunityNearestCityDistance(-1)
{
	static bool bInitCache = true;
	if (bInitCache)
	{
		for (int i = 0; i < iCACHE_SZ; i++)
		{
			aiLastCallParams[i] = 0;
			aiLastCallResult[i] = MIN_INT;
		}
		iLastIndex = 0;
		bInitCache = false;
	}
	FAssert(m_kAgent.getID() != m_kTarget.getID());
	FAssert(!m_kTarget.isAVassal());
	FAssert(m_kAgent.isHasMet(m_kTarget.getID()));
}


void WarEvaluator::logPreamble()
{
	FAssert(gUWAIWarUtilityLogLevel >= 2 && !m_kLogMuteState.isMuted());
	logBBAI("Evaluating %s%s war between team=%d %S%s and team=%d %S%s",
			m_kParams.isTotal() ? getSASWarPlanType(WARPLAN_TOTAL) : getSASWarPlanType(WARPLAN_LIMITED),
			m_kParams.isNaval() ? " naval" : "", m_kAgent.getID(),
			GET_TEAM(m_kAgent.getID()).getName().GetCString(), m_kAgent.isHuman() ? " (human)" : "",
			m_kTarget.getID(), GET_TEAM(m_kTarget.getID()).getName().GetCString(),
			m_kTarget.isHuman() ? " (human)" : "");
	for (MemberIter agentIt(m_kAgent.getID()); agentIt.hasNext(); ++agentIt)
		logBBAI("Agent member: player=%d name=%S", agentIt->getID(), GET_PLAYER(agentIt->getID()).getName(0));
	for (MemberIter targetIt(m_kTarget.getID()); targetIt.hasNext(); ++targetIt)
		logBBAI("Target member: player=%d name=%S", targetIt->getID(), GET_PLAYER(targetIt->getID()).getName(0));
	logBBAI("Current actual war plan: %s", getSASWarPlanType(m_kAgent.AI_getWarPlan(m_kTarget.getID())));
	if (m_kParams.isConsideringPeace()) logBBAI("Considering peace");
	logBBAI("Preparation time vs. target: %d", m_kParams.getPreparationTime());
	if (m_kParams.isImmediateDoW()) logBBAI("Immediate DoW assumed");
	if (m_kAgent.isAVassal()) logBBAI("Agent is a vassal of %S", GET_TEAM(m_kAgent.getMasterTeam()).getName().GetCString());
	if (m_kTarget.isAVassal()) logBBAI("Target is a vassal of %S", GET_TEAM(m_kTarget.getMasterTeam()).getName().GetCString());
	FOR_EACH_ENUM2(Team, eAlly)
	{
		if (m_kParams.isWarAlly(eAlly)) logBBAI("Joint DoW by %S assumed", GET_TEAM(eAlly).getName().GetCString());
	}
	FOR_EACH_ENUM2(Team, eExtraTarget)
	{
		if (m_kParams.isExtraTarget(eExtraTarget)) logBBAI("Extra target: %S", GET_TEAM(eExtraTarget).getName().GetCString());
	}
	if (m_kParams.getSponsor() != NO_PLAYER)
	{
		logBBAI("Sponsored by %S", GET_PLAYER(m_kParams.getSponsor()).getName(0));
		FAssert(m_kParams.isImmediateDoW());
	}
	if (m_kParams.isIgnoreDistraction()) logBBAI("Computation ignoring Distraction cost");
}


int WarEvaluator::defaultPreparationTime(WarPlanTypes eWarPlan)
{
	int iWPAge = 0;
	if (eWarPlan == NO_WARPLAN)
		eWarPlan = m_kAgent.AI_getWarPlan(m_kTarget.getID());
	else iWPAge = m_kAgent.AI_getWarPlanStateCounter(m_kTarget.getID());
	// Agent is past preparations
	if (eWarPlan == WARPLAN_LIMITED || eWarPlan == WARPLAN_TOTAL)
		return 0;
	int iBaseTime = -1;
	if (m_kParams.isTotal())
	{
		if (m_kParams.isNaval())
			iBaseTime = getUWAI().preparationTimeTotalNaval();
		else iBaseTime = getUWAI().preparationTimeTotal();
	}
	else
	{
		if (m_kParams.isNaval())
			iBaseTime = getUWAI().preparationTimeLimitedNaval();
		else iBaseTime = getUWAI().preparationTimeLimited();
	}
	int iR = std::max(iBaseTime - iWPAge, 0);
	iR *= GC.getInfo(GC.getGame().getGameSpeedType()).getTrainPercent();
	iR = intdiv::uround(iR, 100);
	return iR;
}


int WarEvaluator::evaluate(WarPlanTypes eWarPlan, int iPreparationTime)
{
	if (m_kParams.isImmediateDoW())
		iPreparationTime = 0;
	// Plan for war scenario
	if (eWarPlan == NO_WARPLAN)
		eWarPlan = m_kAgent.AI_getWarPlan(m_kParams.getTarget());
	if(eWarPlan == NO_WARPLAN)
	{
		if (iPreparationTime == 0)
			eWarPlan = WARPLAN_LIMITED;
		else eWarPlan = WARPLAN_PREPARING_LIMITED;
	}
	bool const bTotal = (eWarPlan == WARPLAN_TOTAL ||
			eWarPlan == WARPLAN_PREPARING_TOTAL);
	if (m_kAgent.isAtWar(m_kTarget.getID()))
	{
		/*	If already at war, MilitaryAnalyst determines whether it should be naval.
			(Doesn't currently write this into params though - should it?)
			Might as well not bother with the land target check then? */
		bool const bNaval = !m_kAgent.AI_isLandTarget(m_kTarget.getID());
		int iU = evaluateScenario(false, eWarPlan, bNaval, 0);
		m_kParams.setNaval(bNaval);
		m_kParams.setTotal(bTotal);
		m_kParams.setPreparationTime(0);
		return iU;
	}
	/*	Just for performance: Don't compute naval war utility if we have
		no transports. Do compute utility of limited naval war. May not give
		us enough time to produce transports; then war utility will be low.
		But could also already have transports e.g. from earlier wars.
		(BtS/K-Mod only considers total naval war.) */
	bool bSkipNaval = true;
	for (MemberAIIter itMember(m_kAgent.getID()); itMember.hasNext(); ++itMember)
	{
		if (itMember->AI_totalUnitAIs(UNITAI_ASSAULT_SEA) +
			itMember->AI_totalUnitAIs(UNITAI_SETTLER_SEA) > 0)
		{
			bSkipNaval = false;
		}
	}
	// <!-- custom: When both naval and non-naval scenarios are evaluated, suppress their nested level-2+ diagnostics and rerun only the selected scenario for detail. Level 1 remains compact and does not pay for this diagnostic-only third evaluation. (ChatGPT-5.6-Sol) -->
	bool const bDetailedLog = (!m_kLogMuteState.isMuted() &&
		(gUWAIWarUtilityLogLevel >= 2 || gUWAIMilitaryAnalystLogLevel >= 2 ||
		gUWAIInvasionGraphLogLevel >= 2 || gUWAIArmamentForecastLogLevel >= 2));
	bool const bExtraRun = (bDetailedLog && !bSkipNaval);
	if (bExtraRun)
		m_kLogMuteState.pushMute();
	int iNonNavalU = evaluateScenario(false, eWarPlan, false, iPreparationTime);
	// Assume non-naval war if it hardly makes a difference
	int const iAntiNavalBias = 3;
	int iNavalU = MIN_INT;
	if (!bSkipNaval)
		iNavalU = evaluateScenario(false, eWarPlan, true, iPreparationTime);
	bool const bNaval = (iNavalU > iNonNavalU + iAntiNavalBias);
	int const iU = (bNaval ? iNavalU : iNonNavalU);
	if (bExtraRun)
	{
		m_kLogMuteState.popMute();
		// <!-- custom: This third pass exists only to emit UWAI detail for the selected scenario. It bypasses WarEvaluator cache reads/writes and focused WarEvaluator structured WAR diagnostics, and its return value can never replace the gameplay utility chosen by the original comparison. (ChatGPT-5.6-Sol) -->
		int const iDiagnosticU = evaluateScenario(true, eWarPlan, bNaval, iPreparationTime);
		// <!-- custom: Cached/UI evaluations can intentionally reuse an older result; only assert direct recomputation parity when no cache path can have supplied the gameplay value. Compare outside FAssert so old MSVC /WX still sees iDiagnosticU as used when assertions compile out. (ChatGPT-5.6-Sol) -->
		if (!m_bCheckCache && !m_bUseCache && !gDLL->isDiplomacy() && iDiagnosticU != iU) FAssertMsg(false, "Diagnostic-only UWAI rerun changed utility");
	}
	/*  Calls to evaluate(WarPlanTypes,bool,int) change some members of m_kParams
		that the caller may read */
	m_kParams.setNaval(bNaval);
	m_kParams.setTotal(bTotal);
	m_kParams.setPreparationTime(defaultPreparationTime(eWarPlan));
	return iU;
}


int WarEvaluator::evaluate(WarPlanTypes eWarPlan, bool bNaval, int iPreparationTime)
{
	// <!-- custom: Preserve the inherited gameplay overload: callers such as UWAICache need the selected-scenario utility and normal cache semantics. Logging-only selected-scenario reruns use evaluateForDiagnostics instead. (ChatGPT-5.6-Sol + GPT-6.1-Sol) -->
	return evaluateScenario(false, eWarPlan, bNaval, iPreparationTime);
}


void WarEvaluator::evaluateForDiagnostics(WarPlanTypes eWarPlan, bool bNaval, int iPreparationTime)
{
	// <!-- custom: Keep the logging-only rerun observation-only by construction: no utility is returned to the caller, and evaluateScenario(true, ...) bypasses evaluator cache reads/writes and focused WAR diagnostics. Added after the remaining UWAIAgent logging rerun was identified during review. (ChatGPT-5.6-Sol + GPT-6.1-Sol) -->
	evaluateScenario(true, eWarPlan, bNaval, iPreparationTime);
}


// <!-- custom: Shared implementation for gameplay evaluation and the explicitly diagnostic-only selected-scenario rerun. bDiagnosticOnly is internal so ordinary callers cannot accidentally use a diagnostic pass as a gameplay evaluator. (ChatGPT-5.6-Sol) -->
int WarEvaluator::evaluateScenario(bool bDiagnosticOnly, WarPlanTypes eWarPlan, bool bNaval, int iPreparationTime)
{
	PROFILE_FUNC(); // All war evaluation goes through here
	m_bPeaceScenario = (eWarPlan == NO_WARPLAN); // Should only happen in recursive call
	// <!-- custom: Only the outer WAR evaluation selects and clears a naval-opportunity diagnostic candidate.
	// Its recursive PEACE evaluation below must retain that state so the two scenario decompositions can be emitted together. See KI#53.6. (ChatGPT-5.6-Sol + GPT-5.6-Sol) -->
	if (!m_bPeaceScenario)
	{
		m_iSASNavalOpportunityNearestCityDistance = -1;
		m_bSASLogNavalOpportunity = (!bDiagnosticOnly && gWarLogLevel >= 3 && isSASBBAINavalOpportunityCandidate(m_kParams, m_kAgent, m_kTarget, bNaval, m_iSASNavalOpportunityNearestCityDistance));
		if (m_bSASLogNavalOpportunity)
		{
			m_asSASNavalOpportunityPeaceAspectNames.clear();
			m_aiSASNavalOpportunityPeaceAspectUtilities.clear();
		}
	}
	m_kParams.setNaval(bNaval);
	/*  The original cause of war (dogpile, attacked etc.) has no bearing
		on war utility. */
	m_kParams.setTotal(eWarPlan == WARPLAN_TOTAL || eWarPlan == WARPLAN_PREPARING_TOTAL);
	if (iPreparationTime < 0)
	{
		FAssert(!m_bPeaceScenario); // Needs to be set in the war scenario call
		iPreparationTime = defaultPreparationTime(eWarPlan);
	}
	m_kParams.setPreparationTime(iPreparationTime);
	// Don't check cache in recursive calls (peaceScenario=true)
	if (!bDiagnosticOnly && !m_bPeaceScenario && (m_bCheckCache || m_bUseCache || gDLL->isDiplomacy()))
	{
		WarEvalParamID iParamID = m_kParams.getID();
		for (int i = 0; i < iCACHE_SZ; i++)
		{
			if (iParamID == aiLastCallParams[i] && aiLastCallResult[i] != MIN_INT)
				return aiLastCallResult[i];
		}
	}
	if (gUWAIWarUtilityLogLevel >= 2 && !m_kLogMuteState.isMuted())
	{
		if (m_bPeaceScenario)
			logBBAI("Peace scenario");
		else
		{
			/*  Normally, both are evaluated, and war goes first. Logging the preamble
				once is enough. */
			logPreamble();
			logBBAI("War scenario");
		}
	}
	vector<WarUtilityAspect*> apAspects;
	fillWithAspects(apAspects);
	for (MemberIter itMember(m_kAgent.getID()); itMember.hasNext(); ++itMember)
		evaluate(itMember->getID(), apAspects);
	bool const bSASHighUtilityLog = (!bDiagnosticOnly && gWarLogLevel >= 3 && getSASHighWarUtilityLogThreshold() > 0);
	// <!-- custom: Save-file 452 showed Mali seeking peace immediately after capturing two Maya cities despite leading heavily in cities, power and war success.
	// For similarly dominant wars, retain each utility aspect for both scenarios so a sudden reversal can be traced without enabling broad level-3 UWAI spam. (GPT-5.6-Sol) -->
	bool const bSASSuspiciousPeaceLog = (!bDiagnosticOnly && m_bSASLogSuspiciousPeace && isSASSuspiciousPeaceLead(m_kParams));
	bool const bSASAspectLog = (bSASHighUtilityLog || bSASSuspiciousPeaceLog || m_bSASLogNavalOpportunity);
	std::vector<CvString> asAspectNames;
	std::vector<int> aiAspectUtilities;
	int iU = 0;
	for (size_t i = 0; i < apAspects.size(); i++)
	{
		int iDelta = apAspects[i]->utility();
		iU += iDelta;
		if (bSASAspectLog)
		{
			asAspectNames.push_back(apAspects[i]->aspectName());
			aiAspectUtilities.push_back(iDelta);
		}
		if (gUWAIWarUtilityLogLevel >= 2 && !m_kLogMuteState.isMuted() && iDelta != 0) logBBAI("%s total: %d", apAspects[i]->aspectName(), iDelta);
		delete apAspects[i];
	}
	if (gUWAIWarUtilityLogLevel >= 1 && !m_kLogMuteState.isMuted()) logBBAI("Bottom line: %d", iU);
	// <!-- custom: Preserve the recursive PEACE scenario's aspect decomposition in members.
	// After recursion returns, the outer WAR call still has its local aspect vectors and emits both in one row. See KI#53.6. (ChatGPT-5.6-Sol + GPT-5.6-Sol) -->
	if (m_bPeaceScenario && m_bSASLogNavalOpportunity)
	{
		m_asSASNavalOpportunityPeaceAspectNames = asAspectNames;
		m_aiSASNavalOpportunityPeaceAspectUtilities = aiAspectUtilities;
	}
	if (bSASSuspiciousPeaceLog)
		logSASBBAISuspiciousPeaceScenario(m_kParams, eWarPlan, bNaval, iPreparationTime, m_bPeaceScenario ? "PEACE" : "WAR", iU, asAspectNames, aiAspectUtilities);
	if (bSASHighUtilityLog && isSASHighWarUtility(iU))
		logSASBBAIHighWarUtilityScenario(m_kParams, eWarPlan, bNaval, iPreparationTime, m_bPeaceScenario ? "PEACE" : "WAR", iU, asAspectNames, aiAspectUtilities);
	if (!m_bPeaceScenario)
	{
		int const iWarScenarioUtility = iU;
		int const iPeaceScenarioUtility = evaluateScenario(bDiagnosticOnly, NO_WARPLAN, false, iPreparationTime); // Required for final war-minus-peace utility; stored only so high-utility diagnostics can show both sides.
		iU -= iPeaceScenarioUtility;
		if (gUWAIWarUtilityLogLevel >= 1 && !m_kLogMuteState.isMuted()) logBBAI("Utility war minus peace: %d", iU);
		if (bSASSuspiciousPeaceLog)
			logSASBBAISuspiciousPeaceFinal(m_kParams, eWarPlan, bNaval, iPreparationTime, iWarScenarioUtility, iPeaceScenarioUtility, iU);
		if (bSASHighUtilityLog && isSASHighWarUtility(iU))
			logSASBBAIHighWarUtilityFinal(m_kParams, eWarPlan, bNaval, iPreparationTime, iWarScenarioUtility, iPeaceScenarioUtility, iU);
		// <!-- custom: WAR_TARGET_HARD_REJECT already explains the deliberate ~-100000 contact/lift veto.
		// Keep the naval-opportunity row for ordinary UWAI valuation only so the focused log is not dominated by duplicate hard rejects. See KI#53.6. (ChatGPT-5.6-Sol) -->
		if (m_bSASLogNavalOpportunity && !hasSASBBAIHardRejectAspect(aiAspectUtilities))
		{
			logSASBBAINavalOpportunity(m_kParams, m_kAgent, m_kTarget, eWarPlan, iPreparationTime, m_iSASNavalOpportunityNearestCityDistance, iWarScenarioUtility, iPeaceScenarioUtility, iU,
				asAspectNames, aiAspectUtilities, m_asSASNavalOpportunityPeaceAspectNames, m_aiSASNavalOpportunityPeaceAspectUtilities);
		}
		// Restore params (changed by recursive call)
		m_kParams.setNaval(bNaval);
		m_kParams.setTotal(eWarPlan == WARPLAN_TOTAL ||
				eWarPlan == WARPLAN_PREPARING_TOTAL);
		m_kParams.setPreparationTime(iPreparationTime);
		/*  Could update cache even when !m_bCheckCache, but this might
			push out just the values that are needed ... */
		if (!bDiagnosticOnly && (m_bCheckCache || m_bUseCache))
		{
			// Cache the total result after returning from the recursive call
			WarEvalParamID iParamID = m_kParams.getID();
			aiLastCallParams[iLastIndex] = iParamID;
			aiLastCallResult[iLastIndex] = iU;
			iLastIndex = (iLastIndex + 1) % iCACHE_SZ;
		}
	}
	return iU;
}


void WarEvaluator::fillWithAspects(vector<WarUtilityAspect*>& kAspects)
{
	// Abbreviate ...
	vector<WarUtilityAspect*>& v = kAspects;
	WarEvalParameters const& params = m_kParams;

	v.push_back(new GreedForAssets(params));
	v.push_back(new GreedForVassals(params));
	v.push_back(new GreedForSpace(params));
	v.push_back(new GreedForCash(params));
	v.push_back(new Loathing(params));
	v.push_back(new MilitaryVictory(params));
	v.push_back(new Assistance(params));
	v.push_back(new Reconquista(params));
	v.push_back(new Rebuke(params));
	v.push_back(new Fidelity(params));
	v.push_back(new HiredHand(params));
	v.push_back(new BorderDisputes(params));
	v.push_back(new SuckingUp(params));
	v.push_back(new PreEmptiveWar(params));
	v.push_back(new KingMaking(params));
	v.push_back(new Effort(params));
	v.push_back(new Risk(params));
	v.push_back(new IllWill(params));
	v.push_back(new Affection(params));
	if(!params.isIgnoreDistraction())
		v.push_back(new Distraction(params));
	v.push_back(new PublicOpposition(params));
	v.push_back(new Revolts(params));
	v.push_back(new UlteriorMotives(params));
	v.push_back(new TacticalSituation(params));
	v.push_back(new Bellicosity(params));
	v.push_back(new FairPlay(params));
	v.push_back(new LoveOfPeace(params));
	v.push_back(new ThirdPartyIntervention(params));
	v.push_back(new DramaticArc(params));
	FAssert(UWAI::NUM_ASPECTS - (params.isIgnoreDistraction() ? 1 : 0) == (int)v.size());
}


void WarEvaluator::evaluate(PlayerTypes eAgentPlayer, vector<WarUtilityAspect*>& kAspects)
{
	MilitaryAnalyst militaryAnalyst(eAgentPlayer, m_kParams, m_bPeaceScenario);
	if (gUWAIMilitaryAnalystLogLevel >= 1 && !m_kLogMuteState.isMuted())
	{
		for (PlayerIter<MAJOR_CIV,KNOWN_TO> it(m_kAgent.getID()); it.hasNext(); ++it)
		{
			if (!GET_TEAM(it->getID()).isCapitulated()) militaryAnalyst.logResults(it->getID());
		}
	}
	if (gUWAIWarUtilityLogLevel >= 2 && !m_kLogMuteState.isMuted()) logBBAI("Computing utility of %S",
			GET_PLAYER(eAgentPlayer).getName(0));
	int iU = 0;
	// <!-- custom: The first KI#53.6 aspect run showed that many strong/near island candidates had no GreedForAssets at all, meaning MilitaryAnalyst predicted no city gain from the target.
	// Log the underlying naval simulation only for ordinary (non-hard-rejected) opportunity candidates so we can distinguish Army, Fleet, Logistics, and projected-conquest failures before changing behavior. (ChatGPT-5.6-Sol) -->
	bool bSASNavalOpportunityHardReject = false;
	for (size_t i = 0; i < kAspects.size(); i++)
	{
		int const iDelta = kAspects[i]->evaluate(militaryAnalyst);
		iU += iDelta;
		if (iDelta <= -100000)
			bSASNavalOpportunityHardReject = true;
	}
	if (m_bSASLogNavalOpportunity && !m_bPeaceScenario && !bSASNavalOpportunityHardReject)
		logSASBBAINavalOpportunitySimulation(m_kParams, militaryAnalyst, eAgentPlayer);
	if (gUWAIWarUtilityLogLevel >= 1 && !m_kLogMuteState.isMuted()) logBBAI("Total utility for %S: %d",
			GET_PLAYER(eAgentPlayer).getName(0), iU);
}
