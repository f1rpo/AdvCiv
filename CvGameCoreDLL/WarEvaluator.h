#pragma once

#ifndef WAR_EVALUATOR_H
#define WAR_EVALUATOR_H

class WarEvalParameters;
class WarUtilityAspect;
class UWAILogMuteState; // <!-- custom: Nested mute depth shared by UWAI diagnostic sub-evaluations; not a logger. See KI#505.3. (ChatGPT-5.6-Sol) -->

/*	advc.104: New class. Computes the utility of a war between an
	"agent" team and a "target" team from the agent's point of view. */
class WarEvaluator
{
public:
	// The cache should only be used for UI purposes; see comments in implementation file.
	WarEvaluator(WarEvalParameters& kWarEvalParams, bool bUseCache = false);

	/*	Can be called repeatedly on the same instance for different
		war plan types. If eWarPlan=NO_WARPLAN, then the current war plan
		is evaluated, LIMITED_WAR if none.
		Preparation time -1 lets WarEvaluator choose a preparation time matching
		the war plan type, the war plan age and other factors.
		(The base values are defined in UWAIAI.h.)
		Use 0 prep time for wars that have to be declared immediately
		(e.g. sponsored wars).
		Returns the war utility value. 100 is very high, 0 is no gain and no pain,
		-100 very small, but values can be (several times) higher or smaller than that.
		Will store in the WarEvalParamters instance (passed to the constructor) which
		war plan type and preparation time are assumed and whether it'll be a naval war. */
	/*	The first evaluate function lets WarEvaluator decide whether naval war
		is preferable. */
	int evaluate(WarPlanTypes eWarPlan = NO_WARPLAN, int iPreparationTime = -1);
	int evaluate(WarPlanTypes eWarPlan, bool bNaval, int iPreparationTime);
	// <!-- custom: Caller-pre-gated observer pass for the already selected UWAI scenario.
	// Unlike the inherited gameplay overload above, it emits detail without reading/writing WarEvaluator caches and cannot return a utility to gameplay.
	// Added after the remaining logging-only rerun in UWAIAgent was found during review. (ChatGPT-5.6-Sol + GPT-6.1-Sol) -->
	void evaluateForDiagnostics(WarPlanTypes eWarPlan, bool bNaval, int iPreparationTime);
	int defaultPreparationTime(WarPlanTypes eWarPlan = NO_WARPLAN);
	// <!-- custom: Enable focused aspect logging only on the evaluator that supplies an actual peace review; alternative war simulations otherwise duplicate the same diagnostic many times. (GPT-5.6-Sol) -->
	void enableSASBBAISuspiciousPeaceLog() { m_bSASLogSuspiciousPeace = true; }

private:
	// <!-- custom: Internal scenario pass shared by gameplay evaluation and the selected-scenario diagnostic rerun.
	// Keep the diagnostic-only mode first and explicit: such passes bypass WarEvaluator cache reads/writes and focused structured WAR diagnostics, and their result must never feed gameplay. (ChatGPT-5.6-Sol) -->
	int evaluateScenario(bool bDiagnosticOnly, WarPlanTypes eWarPlan, bool bNaval, int iPreparationTime);
	void logPreamble();
	/*	Utility from pov of an individual agent member. Will evaluate the aspects
		(which may modify the aspect instances). */
	void evaluate(PlayerTypes eAgentPlayer, std::vector<WarUtilityAspect*>& kAspects);
	/*  Creates the top-level war utility aspects: war gains and war costs;
		the aspect objects create their sub-aspects themselves. */
	void fillWithAspects(std::vector<WarUtilityAspect*>& kAspects);

	// Caveat: The order of the reference members is important for the ctor
	WarEvalParameters& m_kParams;
	CvTeamAI& m_kTarget;
	CvTeamAI& m_kAgent;
	// <!-- custom: Shared nested mute state from m_kParams; evaluator diagnostics otherwise use the ordinary BBAI sink directly. (ChatGPT-5.6-Sol) -->
	UWAILogMuteState& m_kLogMuteState;
	bool m_bPeaceScenario;
	bool m_bUseCache;
	bool m_bSASLogSuspiciousPeace;
	// <!-- custom: Level-3 WAR diagnostics retain the peace-scenario aspect split only while evaluating a nearby, weaker, transport-capable overseas target.
	// This lets one compact row explain why a plausible island war was rejected without enabling broad UWAI subsystem diagnostics. See KI#53.6. (ChatGPT-5.6-Sol) -->
	bool m_bSASLogNavalOpportunity;
	int m_iSASNavalOpportunityNearestCityDistance;
	std::vector<CvString> m_asSASNavalOpportunityPeaceAspectNames;
	std::vector<int> m_aiSASNavalOpportunityPeaceAspectUtilities;

	bool atTotalWarWithTarget() const;
	void gatherCivsAndTeams();

	public:
		/*  If (or while) enabled, all WarEvaluator instances use the cache, not just
			those with useCache=true. */
		static void enableCache();
		static void disableCache();
		static void clearCache(); // Invalidates the cache (disableCache does not do so)
	private:
		static bool m_bCheckCache;
		static bool m_bCacheCleared;
};

#endif
