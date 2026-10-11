// AI, UI, logging, or other modifications first developed in AdvCiv-SAS (Simple Advanced Strategy)
// (c) 2026 wonderingabout & AI/LLM helpers (see Authors in AdvCiv-SAS's root README.md)

#pragma once
#ifndef SAS_SAVE_STREAM_FORMAT_H
#define SAS_SAVE_STREAM_FORMAT_H

// <!-- custom: One list supplies the existing gameplay/replay writer markers and their compact diagnostic names.
// Each entry supplies its diagnostic label as well as its constant/value; stringify that label instead of reverse-looking-up numeric values, which repeat across components.
// These are current writer capabilities, not values captured from loaded objects or a complete save-schema compatibility identifier; XML-info caches are outside this list.
// Loaded markers are stream input; historical reader thresholds describe particular past layouts and must not be replaced by these current writer constants.
// Keep the existing marker when only centralizing it; change it deliberately when that subsystem's serialized layout changes.
// The list generates compile-time constants and the recorder's cold-path descriptor array, with no runtime registry initialization or gameplay lookup. (GPT-6.1-Sol) -->
#define SAS_SAVE_STREAM_FORMATS(DO) \
	DO(CV_AREA, CvArea, 4) \
	DO(CV_CITY, CvCity, 17) \
	DO(CV_CITY_AI, CvCityAI, 9) \
	DO(CV_DEAL, CvDeal, 0) \
	DO(CV_DIPLO_PARAMETERS, CvDiploParameters, 0) \
	DO(CV_GAME, CvGame, 27) \
	DO(CV_GAME_AI, CvGameAI, 1) \
	DO(CV_INIT_CORE, CvInitCore, 7) \
	DO(CV_MAP, CvMap, 7) \
	DO(CV_PLOT, CvPlot, 13) \
	DO(CV_PLOT_GROUP, CvPlotGroup, 1) \
	DO(CV_PLAYER, CvPlayer, 22) \
	DO(CV_PLAYER_AI, CvPlayerAI, 22) \
	DO(CV_SELECTION_GROUP, CvSelectionGroup, 3) \
	DO(CV_SELECTION_GROUP_AI, CvSelectionGroupAI, 0) \
	DO(CV_TEAM, CvTeam, 19) \
	DO(CV_TEAM_AI, CvTeamAI, 9) \
	DO(CV_UNIT, CvUnit, 8) \
	DO(CV_UNIT_AI, CvUnitAI, 0) \
	DO(CV_GAME_RECORD, CvGameRecord, 0) \
	DO(CV_PLAYER_RECORD, CvPlayerRecord, 0) \
	DO(UWAI_CACHE, UWAICache, 9) \
	DO(UWAI_CACHE_CITY, UWAICache::City, 5) \
	DO(MILITARY_BRANCH, MilitaryBranch, 1) \
	DO(RISE_FALL, RiseFall, 1) \
	DO(RF_CHAPTER, RFChapter, 0) \
	DO(RF_CHAPTER_SCORE, RFChapterScore, 0) \
	DO(RF_TOTAL_SCORE, RFTotalScore, 0) \
	DO(CV_REPLAY_INFO, CvReplayInfo, 6)

namespace SASSaveStreamFormat
{
#define SAS_SAVE_STREAM_MARKER_ENUM(ID, LABEL, VALUE) ID = VALUE,
	enum CurrentWriteMarker
	{
		SAS_SAVE_STREAM_FORMATS(SAS_SAVE_STREAM_MARKER_ENUM)
	};
#undef SAS_SAVE_STREAM_MARKER_ENUM
}

#endif
