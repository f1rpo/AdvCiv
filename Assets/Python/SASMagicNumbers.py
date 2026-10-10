# AI, UI, logging, or other modifications first developed in AdvCiv-SAS (Simple Advanced Strategy)
# (c) 2026 wonderingabout & AI/LLM helpers (see Authors in AdvCiv-SAS's root README.md)

# <!-- custom: Magic numbers and numeric constants only.
# Values do not need to be shared to live here; a named home is useful when the number would otherwise be obscure, collision-prone, or likely to drift.
# Keep this module import-free so maps, advisors, Sevopedia, and utility modules can import it without pulling in CvPythonExtensions, CyGlobalContext, screen helpers, or SASUtils runtime state.
# Prefix exports with SAS_MAGIC_ to reduce accidental global-name collisions in files that use from-module import-star. (GPT-5.5) -->

# <!-- custom: WIDGET_PYTHON data1 routing IDs used by Sevopedia and cross-screen pedia jumps such as Tech Chooser Build links.
# Keep these unique across pedia handlers; data1 selects the route and data2 carries the actual item/action payload.
# These were originally split between SevoPediaMain.py and SevoPediaLeader.py; centralizing them here avoids local magic IDs and duplicate aliases. (Claude Opus 4.5 / Claude code Sonnet 4.6 / Claude code Opus 4.7 / GPT-5.5) -->
SAS_MAGIC_PEDIA_PYTHON_BUILD = 6798
SAS_MAGIC_PEDIA_PYTHON_TRAIT = 6799
SAS_MAGIC_PEDIA_PYTHON_MOVIE_ENTRY = 6800
SAS_MAGIC_PEDIA_PYTHON_MOVIE_PLAY = 6801
SAS_MAGIC_PEDIA_PYTHON_MUSIC_ENTRY = 6802
SAS_MAGIC_PEDIA_PYTHON_MUSIC_PLAY = 6803
SAS_MAGIC_PEDIA_PYTHON_CHART_LOG = 6804
SAS_MAGIC_PEDIA_PYTHON_LEADER_ATTITUDE = 6805
SAS_MAGIC_PEDIA_PYTHON_LEADER_ACTION = 6806
SAS_MAGIC_PEDIA_PYTHON_HISTORY_EXPAND = 6807
SAS_MAGIC_PEDIA_PYTHON_CONTENT_EXPAND = 6808
SAS_MAGIC_PEDIA_PYTHON_CONTENT_RELOAD = 6809
# <!-- custom: Sevopedia leader era art preview buttons (default/"D" + per-era index buttons).
# Routed via SevoPediaMain.handleInput -> applyLeaderEra. (Claude code Sonnet 4.6 / GPT-5.5) -->
SAS_MAGIC_PEDIA_PYTHON_LEADER_ERA = 6810
# <!-- custom: Votes and Event Triggers have no native engine jump widgets, so their Sevopedia left-list entries route through WIDGET_PYTHON like the custom Build/Trait pages. (Claude code Opus 4.7 / GPT-5.5) -->
SAS_MAGIC_PEDIA_PYTHON_VOTE_ENTRY = 6811
SAS_MAGIC_PEDIA_PYTHON_EVENT_TRIGGER_ENTRY = 6812
SAS_MAGIC_PEDIA_PYTHON_GAME_PLAYER_ID_PREV = 6813
SAS_MAGIC_PEDIA_PYTHON_GAME_PLAYER_ID_NEXT = 6814
SAS_MAGIC_PEDIA_PYTHON_SEARCH_KEY = 6815
# <!-- custom: Sevopedia expanded-text playground controls use separate IDs for panel style, background DDS, and text color so their signed iData2 cycle deltas don't collide with expand/close dispatch. See KI#128. (Claude code Opus 4.7 + GPT-5.5) -->
SAS_MAGIC_PEDIA_PYTHON_PANEL_STYLE_CYCLE = 6816
SAS_MAGIC_PEDIA_PYTHON_BACKGROUND_CYCLE = 6817
SAS_MAGIC_PEDIA_PYTHON_TEXT_COLOR_CYCLE = 6818

# <!-- custom: Sevopedia Movie/Music packed-key type IDs.
# They are payload discriminators used by SevoPediaMain pack/unpack helpers and by Movie/Music pages, so keep them beside the WIDGET_PYTHON routing IDs instead of as SevoPediaMain instance fields. (GPT-5.5) -->
SAS_MAGIC_PEDIA_MOVIE_TYPE_VICTORY = 1
SAS_MAGIC_PEDIA_MOVIE_TYPE_WONDER = 2
SAS_MAGIC_PEDIA_MOVIE_TYPE_PROJECT = 3
SAS_MAGIC_PEDIA_MOVIE_TYPE_RELIGION = 4
SAS_MAGIC_PEDIA_MOVIE_TYPE_ERA = 5
SAS_MAGIC_PEDIA_MOVIE_TYPE_CORPORATION = 6
SAS_MAGIC_PEDIA_MUSIC_TYPE_TECH = 1
SAS_MAGIC_PEDIA_MUSIC_TYPE_ERA = 2
SAS_MAGIC_PEDIA_MUSIC_TYPE_LEADER = 3
SAS_MAGIC_PEDIA_MUSIC_TYPE_CIV = 4
SAS_MAGIC_PEDIA_MUSIC_TYPE_SCRIPT = 5
SAS_MAGIC_PEDIA_MUSIC_TYPE_SCRIPT_3D = 6
