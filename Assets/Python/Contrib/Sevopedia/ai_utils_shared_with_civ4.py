# AI Utilities for normalization and general helpers
# AI, UI, logging, or other modifications first developed in AdvCiv-SAS (Simple Advanced Strategy)
# (c) 2026 wonderingabout & AI/LLM helpers (see Authors in AdvCiv-SAS's root README.md)

from sas_utils_shared_with_civ4 import get_sas_python_file_header_lines


# <!-- custom: This module is shared by Civ4 runtime Python and GitHub workflow Python.
# Keep it pure-helper only: no CvPythonExtensions imports, no game-context globals, and no syntax newer than Python 2.4 unless guarded outside runtime paths. (ChatGPT-5.5) -->
# <!-- custom: Shared AIP enum/type metadata used by both the in-game Sevopedia AIP code and workflow predump validation.
# These specs mirror the compact subset currently assessed by the AIP panel, not every possible future field. (ChatGPT-5.5) -->
def get_aip_excluded_leader_types():
	# <!-- custom: AIP-only excluded leaders shared by the runtime Sevopedia AIP cache and the workflow predump checker.
	# Keep separate from EXCLUDED_LEADER_TYPES_FROM_SEVOPEDIA in _sevopedia_helpers.py, which is the broader Sevopedia leader/civ association list used by Traits, Improvements, Religions, Civics, and groupings. (ChatGPT-5.5) -->
	return ("LEADER_BARBARIAN",)

def get_aip_flavor_types_assessed():
	return (
		"FLAVOR_MILITARY",
		"FLAVOR_RELIGION",
		"FLAVOR_PRODUCTION",
		"FLAVOR_GOLD",
		"FLAVOR_SCIENCE",
		"FLAVOR_CULTURE",
		"FLAVOR_GROWTH",
		"FLAVOR_ESPIONAGE",
	)

def get_aip_no_war_attitude_types_assessed():
	return (
		"ATTITUDE_FURIOUS",
		"ATTITUDE_ANNOYED",
		"ATTITUDE_CAUTIOUS",
		"ATTITUDE_PLEASED",
		"ATTITUDE_FRIENDLY",
	)

def get_aip_contact_specs():
	# <!-- custom: Central contact metadata tuple: (debug/index order, XML/DLL contact type, compact AIP UI label).
	# Example: index 0 maps CONTACT_RELIGION_PRESSURE to "Relig Press", preserving the old 0: "Relig Press" clarity without a separate duplicated label dict. (ChatGPT-5.5) -->
	return (
		(0, "CONTACT_RELIGION_PRESSURE", "Relig Press"),
		(1, "CONTACT_CIVIC_PRESSURE", "Civic Press"),
		(2, "CONTACT_JOIN_WAR", "Join W"),
		(3, "CONTACT_STOP_TRADING", "Stop Tr"),
		(4, "CONTACT_GIVE_HELP", "Gave Help"),
		(5, "CONTACT_ASK_FOR_HELP", "Help"),
		(6, "CONTACT_DEMAND_TRIBUTE", "Trib"),
		(7, "CONTACT_OPEN_BORDERS", "Open Borders"),
		(8, "CONTACT_DEFENSIVE_PACT", "DefensPact"),
		(9, "CONTACT_PERMANENT_ALLIANCE", "PermAlliance"),
		(10, "CONTACT_PEACE_TREATY", "PeaceTreaty"),
		(11, "CONTACT_TRADE_TECH", "Tr Tech"),
		(12, "CONTACT_TRADE_BONUS", "Tr Bonus"),
		(13, "CONTACT_TRADE_MAP", "Tr Map"),
	)


def get_aip_contact_types_assessed():
	return tuple([contact_type for _index, contact_type, _label in get_aip_contact_specs()])

def get_aip_memory_specs():
	# <!-- custom: indexes based on real ingame sevopedia leader debug output, see sevopedia_helpers py file code comments for details -->
	# <!-- custom: Central memory metadata tuple: (debug/index order, XML/DLL memory type, compact AIP UI label, is_positive_memory).
	# Positive/negative aggregate helpers derive from this single structure so type lists, labels, and polarity cannot drift separately. (ChatGPT-5.5) -->
	return (
		(0, "MEMORY_DECLARED_WAR", "D.W", False),
		(1, "MEMORY_DECLARED_WAR_ON_FRIEND", "D.W onFr", False),
		(2, "MEMORY_HIRED_WAR_ALLY", "HirWAlly", False),
		(3, "MEMORY_NUKED_US", "Nuked Us", False),
		(4, "MEMORY_NUKED_FRIEND", "Nuked Fr", False),
		(5, "MEMORY_RAZED_CITY", "RazC", False),
		(6, "MEMORY_RAZED_HOLY_CITY", "RazHolyC", False),
		(7, "MEMORY_SPY_CAUGHT", "Spy Caught", False),
		(8, "MEMORY_GIVE_HELP", "Gave Help", True),
		(9, "MEMORY_REFUSED_HELP", "RefHelpUs", False),
		(10, "MEMORY_ACCEPT_DEMAND", "AcD", True),
		(11, "MEMORY_REJECTED_DEMAND", "Rej D", False),
		(12, "MEMORY_ACCEPTED_RELIGION", "AcRelig", True),
		(13, "MEMORY_DENIED_RELIGION", "Dn Relig", False),
		(14, "MEMORY_ACCEPTED_CIVIC", "AcCivic", True),
		(15, "MEMORY_DENIED_CIVIC", "Dn Civic", False),
		(16, "MEMORY_ACCEPTED_JOIN_WAR", "AcJoin W", True),
		(17, "MEMORY_DENIED_JOIN_WAR", "Dn JoinW", False),
		(18, "MEMORY_ACCEPTED_STOP_TRADING", "AcStop Tr", True),
		(19, "MEMORY_DENIED_STOP_TRADING", "Dn StopTr", False),
		(20, "MEMORY_STOPPED_TRADING", "StoppedTr", False),
		(21, "MEMORY_STOPPED_TRADING_RECENT", "RecStoppedTr", False),
		(22, "MEMORY_HIRED_TRADE_EMBARGO", "TrEmbargo", False),
		(23, "MEMORY_MADE_DEMAND", "Made D", False),
		(24, "MEMORY_CANCELLED_VASSAL_AGREEMENT", "CancVassal", False),
		(25, "MEMORY_MADE_DEMAND_RECENT", "RecentMadeD", False),
		(26, "MEMORY_CANCELLED_OPEN_BORDERS", "CancelledOB", False),
		(27, "MEMORY_CANCELLED_DEFENSIVE_PACT", "CancelledDP", False),
		(28, "MEMORY_TRADED_TECH_TO_US", "Tr Tech", True),
		# <!-- custom: for MEMORY_RECEIVED_TECH_FROM_ANY in particular, it seems less clear if this is negative or not, i found this info for example in kujira's website in (translate to english with your web browser or such): https://gforestshade.github.io/kujira/post/civ4leaderheadinfos/#memory_received_tech_from_any ; Long_Comments_py.txt #7 -->
		(29, "MEMORY_RECEIVED_TECH_FROM_ANY", "RecentTechAny", False),
		(30, "MEMORY_VOTED_AGAINST_US", "VotedAgUs", False),
		(31, "MEMORY_VOTED_FOR_US", "VotedForUs", True),
		(32, "MEMORY_EVENT_GOOD_TO_US", "Event Good", True),
		(33, "MEMORY_EVENT_BAD_TO_US", "Event Bad", False),
		(34, "MEMORY_LIBERATED_CITIES", "LiberatedC", True),
		(35, "MEMORY_INDEPENDENCE", "Indep", True),
		(36, "MEMORY_DECLARED_WAR_RECENT", "Recent W", False),
	)


def get_aip_memory_types_assessed():
	return tuple([memory_type for _index, memory_type, _label, _is_positive in get_aip_memory_specs()])

def get_aip_attitude_type_to_index():
	return {
		"NO_ATTITUDE": -1,
		"NONE": -1,
		"ATTITUDE_FURIOUS": 0,
		"ATTITUDE_ANNOYED": 1,
		"ATTITUDE_CAUTIOUS": 2,
		"ATTITUDE_PLEASED": 3,
		"ATTITUDE_FRIENDLY": 4,
	}


# <!-- custom: Shared AIP direct scalar field specs.
# Each direct-int tuple is (getter name, XML tag, CvLeaderHeadInfo::read explicit default, display label, invert-for-ranking flag).
# Each attitude-threshold tuple uses the same shape, with the default matching the constructor/XML-loader default integer.
# These central specs let the Sevopedia AIP panel and workflow predump checker use the same field list instead of maintaining parallel dictionaries. (ChatGPT-5.5) -->
def get_aip_direct_int_field_specs():
	return (
		('getWonderConstructRand', 'iWonderConstructRand', 0, 'Wonder C.R', False),
		('getBaseAttitude', 'iBaseAttitude', 0, 'Base Attitude', False),
		('getBasePeaceWeight', 'iBasePeaceWeight', 0, 'Base Peace Weig', False),
		('getPeaceWeightRand', 'iPeaceWeightRand', 0, 'Peace Weig Rand', False),
		('getWarmongerRespect', 'iWarmongerRespect', 0, 'Warmonger Resp', False),
		('getEspionageWeight', 'iEspionageWeight', 0, 'EspionageWeig', False),
		('getRefuseToTalkWarThreshold', 'iRefuseToTalkWarThreshold', 0, 'Ref2TalkWSpan', False),
		('getNoTechTradeThreshold', 'iNoTechTradeThreshold', 0, 'NoTech2AdvT', True),
		('getTechTradeKnownPercent', 'iTechTradeKnownPercent', 0, 'NoTechYetRdy%', False),
		('getMaxGoldTradePercent', 'iMaxGoldTradePercent', 0, 'Max Gold Tr%', False),
		('getMaxGoldPerTurnTradePercent', 'iMaxGoldPerTurnTradePercent', 0, 'Max GPT Tr%', False),
		('getCultureVictoryWeight', 'iCultureVictoryWeight', 30, 'Culture', False),
		('getSpaceVictoryWeight', 'iSpaceVictoryWeight', 30, 'Space', False),
		('getConquestVictoryWeight', 'iConquestVictoryWeight', 30, 'Conquest', False),
		('getDominationVictoryWeight', 'iDominationVictoryWeight', 30, 'Domination', False),
		('getDiplomacyVictoryWeight', 'iDiplomacyVictoryWeight', 30, 'Diplomacy', False),
		('getMaxWarRand', 'iMaxWarRand', 0, 'T.W Likely', True),
		('getMaxWarNearbyPowerRatio', 'iMaxWarNearbyPowerRatio', 0, 'T.W NearPR', False),
		('getMaxWarDistantPowerRatio', 'iMaxWarDistantPowerRatio', 0, 'T.W DistPR', False),
		('getMaxWarMinAdjacentLandPercent', 'iMaxWarMinAdjacentLandPercent', 0, 'T.W MinNearPR', True),
		('getLimitedWarRand', 'iLimitedWarRand', 0, 'Lim.W Likely', True),
		('getLimitedWarPowerRatio', 'iLimitedWarPowerRatio', 0, 'Lim.W PR', False),
		('getDogpileWarRand', 'iDogpileWarRand', 0, 'Dogpile Likely', True),
		('getMakePeaceRand', 'iMakePeaceRand', 0, 'MakePeaceLikely', True),
		('getDeclareWarTradeRand', 'iDeclareWarTradeRand', 0, 'WAllianceMaker', False),
		('getDemandRebukedSneakProb', 'iDemandRebukedSneakProb', 0, 'TribRSneakW%', False),
		('getDemandRebukedWarProb', 'iDemandRebukedWarProb', 0, 'TribRef W%', False),
		('getRazeCityProb', 'iRazeCityProb', 0, 'Raz C %', False),
		('getBuildUnitProb', 'iBuildUnitProb', 0, 'Build Unit %', False),
		('getBaseAttackOddsChange', 'iBaseAttackOddsChange', 0, 'Risky Aggr', False),
		('getAttackOddsChangeRand', 'iAttackOddsChangeRand', 0, 'Risky AggrRand+', False),
		('getWorseRankDifferenceAttitudeChange', 'iWorseRankDifferenceAttitudeChange', 0, 'Worse Rank AC', False),
		('getBetterRankDifferenceAttitudeChange', 'iBetterRankDifferenceAttitudeChange', 0, 'Better Rank AC', False),
		('getCloseBordersAttitudeChange', 'iCloseBordersAttitudeChange', 0, 'CloseBordSpark', True),
		('getLostWarAttitudeChange', 'iLostWarAttitudeChange', 0, 'Lost W AC', False),
		('getAtWarAttitudeDivisor', 'iAtWarAttitudeDivisor', 0, 'At W AD', False),
		('getAtWarAttitudeChangeLimit', 'iAtWarAttitudeChangeLimit', 0, 'At W ACL', False),
		('getAtPeaceAttitudeDivisor', 'iAtPeaceAttitudeDivisor', 0, 'At Peace AD', False),
		('getAtPeaceAttitudeChangeLimit', 'iAtPeaceAttitudeChangeLimit', 0, 'At Peace ACL', False),
		('getSameReligionAttitudeChange', 'iSameReligionAttitudeChange', 0, 'Same Relig AC', False),
		('getSameReligionAttitudeDivisor', 'iSameReligionAttitudeDivisor', 0, 'Same Relig AD', False),
		('getSameReligionAttitudeChangeLimit', 'iSameReligionAttitudeChangeLimit', 0, 'Same Relig ACL', False),
		('getDifferentReligionAttitudeChange', 'iDifferentReligionAttitudeChange', 0, 'Diff Relig AC', False),
		('getDifferentReligionAttitudeDivisor', 'iDifferentReligionAttitudeDivisor', 0, 'Diff Relig AD', False),
		('getDifferentReligionAttitudeChangeLimit', 'iDifferentReligionAttitudeChangeLimit', 0, 'Diff Relig ACL', True),
		('getBonusTradeAttitudeDivisor', 'iBonusTradeAttitudeDivisor', 0, 'Bonus Tr AD', False),
		('getBonusTradeAttitudeChangeLimit', 'iBonusTradeAttitudeChangeLimit', 0, 'Bonus Tr ACL', False),
		('getOpenBordersAttitudeDivisor', 'iOpenBordersAttitudeDivisor', 0, 'Open Bord AD', False),
		('getOpenBordersAttitudeChangeLimit', 'iOpenBordersAttitudeChangeLimit', 0, 'Open Bord ACL', False),
		('getDefensivePactAttitudeDivisor', 'iDefensivePactAttitudeDivisor', 0, 'Defens Pact AD', False),
		('getDefensivePactAttitudeChangeLimit', 'iDefensivePactAttitudeChangeLimit', 0, 'Defens Pact ACL', False),
		('getShareWarAttitudeChange', 'iShareWarAttitudeChange', 0, 'Share W AC', False),
		('getShareWarAttitudeDivisor', 'iShareWarAttitudeDivisor', 0, 'Share W AD', False),
		('getShareWarAttitudeChangeLimit', 'iShareWarAttitudeChangeLimit', 0, 'Share W ACL', False),
		('getFavoriteCivicAttitudeChange', 'iFavoriteCivicAttitudeChange', 0, 'Fav Civic AC', False),
		('getFavoriteCivicAttitudeDivisor', 'iFavoriteCivicAttitudeDivisor', 0, 'Fav Civic AD', False),
		('getFavoriteCivicAttitudeChangeLimit', 'iFavoriteCivicAttitudeChangeLimit', 0, 'Fav Civic ACL', False),
		('getVassalPowerModifier', 'iVassalPowerModifier', 0, 'ResistCapitulPM', False),
		('getFreedomAppreciation', 'iFreedomAppreciation', 0, 'FreedomApprec', False),
	)

def get_aip_attitude_threshold_field_specs():
	return (
		('getDemandTributeAttitudeThreshold', 'DemandTributeAttitudeThreshold', -1, 'ScaryNoTrib', True),
		('getNoGiveHelpAttitudeThreshold', 'NoGiveHelpAttitudeThreshold', -1, 'PrideNoHelp', False),
		('getTechRefuseAttitudeThreshold', 'TechRefuseAttitudeThreshold', -1, 'Tech', False),
		('getCityRefuseAttitudeThreshold', 'CityRefuseAttitudeThreshold', 2, 'C', False),
		('getNativeCityRefuseAttitudeThreshold', 'NativeCityRefuseAttitudeThreshold', 3, 'Native C', False),
		('getStrategicBonusRefuseAttitudeThreshold', 'StrategicBonusRefuseAttitudeThreshold', -1, 'Strategic Bonus', False),
		('getHappinessBonusRefuseAttitudeThreshold', 'HappinessBonusRefuseAttitudeThreshold', -1, 'Happiness Bonus', False),
		('getHealthBonusRefuseAttitudeThreshold', 'HealthBonusRefuseAttitudeThreshold', -1, 'Health Bonus', False),
		('getMapRefuseAttitudeThreshold', 'MapRefuseAttitudeThreshold', -1, 'Map', False),
		('getDeclareWarRefuseAttitudeThreshold', 'DeclareWarRefuseAttitudeThreshold', -1, 'D.W', False),
		# <!-- custom: inverted according to: https://gforestshade.github.io/kujira/post/civ4leaderheadinfos/#declarewarthemrefuseattitudethreshold -->
		('getDeclareWarThemRefuseAttitudeThreshold', 'DeclareWarThemRefuseAttitudeThreshold', -1, 'LoyaltyNoD.W', True),
		('getStopTradingRefuseAttitudeThreshold', 'StopTradingRefuseAttitudeThreshold', -1, 'StopTr', False),
		# <!-- custom: inverted according to: https://gforestshade.github.io/kujira/post/civ4leaderheadinfos/#stoptradingthemrefuseattitudethreshold -->
		('getStopTradingThemRefuseAttitudeThreshold', 'StopTradingThemRefuseAttitudeThreshold', -1, 'LoyaltyNoStopTr', True),
		('getAdoptCivicRefuseAttitudeThreshold', 'AdoptCivicRefuseAttitudeThreshold', -1, 'Adopt Civic', False),
		('getConvertReligionRefuseAttitudeThreshold', 'ConvertReligionRefuseAttitudeThreshold', -1, 'Convert Religion', False),
		('getOpenBordersRefuseAttitudeThreshold', 'OpenBordersRefuseAttitudeThreshold', -1, 'Open Bord', False),
		('getDefensivePactRefuseAttitudeThreshold', 'DefensivePactRefuseAttitudeThreshold', -1, 'DefensPact', False),
		('getPermanentAllianceRefuseAttitudeThreshold', 'PermanentAllianceRefuseAttitudeThreshold', -1, 'PermAlliance', False),
		('getVassalRefuseAttitudeThreshold', 'VassalRefuseAttitudeThreshold', -1, 'Vassal', False),
	)

def get_aip_extra_uwai_only_int_field_specs():
	# <!-- custom: UWAI::applyPersonalityWeight also mutates iLoveOfPeace even though the current AIP panel does not display it.
	# Keep it in the workflow median vector so XML+UWAI validation mirrors the DLL primitive-member list. (ChatGPT-5.5) -->
	return (
		('getLoveOfPeace', 'iLoveOfPeace', 0),
	)

def get_aip_fields_directly_parsed():
	# <!-- custom: Return the runtime display dictionaries expected by SevoPediaLeaderAIPValues.py: getter_name -> (label, b_invert). Attributes with b_invert=True are ones where high raw values are worse and low raw values are better. (ChatGPT-5.5) -->
	fields_with_direct_getters = {}
	for getter_name, _xml_tag, _xml_default, label, b_invert in get_aip_direct_int_field_specs():
		fields_with_direct_getters[getter_name] = (label, b_invert)
	fields_attitude_thresholds = {}
	for getter_name, _xml_tag, _xml_default, label, b_invert in get_aip_attitude_threshold_field_specs():
		fields_attitude_thresholds[getter_name] = (label, b_invert)
	return fields_with_direct_getters, fields_attitude_thresholds

def get_aip_contact_index_labels():
	labels = {}
	for index, _contact_type, label in get_aip_contact_specs():
		labels[index] = label
	return labels


def get_aip_positive_memory_index_labels():
	labels = {}
	for index, _memory_type, label, is_positive in get_aip_memory_specs():
		if is_positive:
			labels[index] = label
	return labels


def get_aip_negative_memory_index_labels():
	labels = {}
	for index, _memory_type, label, is_positive in get_aip_memory_specs():
		if not is_positive:
			labels[index] = label
	return labels


def get_aip_memory_index_labels_assessed():
	labels = {}
	for index, _memory_type, label, _is_positive in get_aip_memory_specs():
		labels[index] = label
	return labels

def get_aip_attitude_index_to_type(include_debug_always):
	index_to_type = {}
	for attitude_type, attitude_index in get_aip_attitude_type_to_index().items():
		# <!-- custom: Prefer NONE over NO_ATTITUDE for the shared reverse/debug label because it is shorter and matches existing debug output. (ChatGPT-5.5) -->
		if attitude_index == -1 and attitude_type != "NONE":
			continue
		index_to_type[attitude_index] = attitude_type
	if include_debug_always:
		# <!-- custom: AdvCiv can expose attitude threshold 5 in debug output for harsh map-trade refusal behavior; it is not a normal XML attitude enum. (ChatGPT-5.5) -->
		index_to_type[5] = "ALWAYS??"
	return index_to_type


# <!-- custom: Shared AIP primitive array/list specs.
# These are used by the workflow checker to parse LeaderHeadInfo XML and by comments/helpers in the runtime AIP code to keep field-family metadata in one place.
# Each tuple is (xml parent tag, xml enum key tag, xml value tag, assessed enum/type names, synthetic AIP key prefix). (ChatGPT-5.5) -->
def get_aip_display_array_field_specs():
	return (
		("Flavors", "FlavorType", "iFlavor", get_aip_flavor_types_assessed(), "iFlavor"),
		("NoWarAttitudeProbs", "AttitudeType", "iNoWarProb", get_aip_no_war_attitude_types_assessed(), "iNoWarAttitudeProb"),
	)

def get_aip_hidden_array_field_specs():
	# These are not displayed directly in the AIP predump. They feed shared synthetic contact/memory aggregate fields below.
	return (
		("ContactRands", "ContactType", "iContactRand", get_aip_contact_types_assessed(), "iContactRand"),
		("ContactDelays", "ContactType", "iContactDelay", get_aip_contact_types_assessed(), "iContactDelay"),
		("MemoryDecays", "MemoryType", "iMemoryRand", get_aip_memory_types_assessed(), "iMemoryDecay"),
		("MemoryAttitudePercents", "MemoryType", "iMemoryAttitudePercent", get_aip_memory_types_assessed(), "iMemoryAttitudePercent"),
	)

def get_aip_array_field_specs():
	return get_aip_display_array_field_specs() + get_aip_hidden_array_field_specs()

def get_aip_memory_type_by_index(iMemoryIndex):
	return get_aip_memory_types_assessed()[iMemoryIndex]

def get_aip_contact_type_by_index(iContactIndex):
	return get_aip_contact_types_assessed()[iContactIndex]

# <!-- custom: 11 positive entries total -->
def get_positive_memory_indexes_to_types():
	memory_indexes_to_types = {}
	for index, memory_type, _label, is_positive in get_aip_memory_specs():
		if is_positive:
			memory_indexes_to_types[index] = memory_type
	return memory_indexes_to_types


def get_negative_memory_indexes_to_types():
	memory_indexes_to_types = {}
	for index, memory_type, _label, is_positive in get_aip_memory_specs():
		if not is_positive:
			memory_indexes_to_types[index] = memory_type
	return memory_indexes_to_types

def get_shifted_values(min_val, value, max_val):
	# <!-- custom:
	# example 1:
	# 	min_val = -19	->	shifted_min = 0
	#	value = -7		->	shifted_value = -7 - (-19) = -7 + 19 = 12
	#	max_val = -3	->	shifted_max = -3 - (-19) = -3 + 19 = 16
	#
	# example 2:
	# 	min_val = -19	->	shifted_min = 0
	#	value = -2		->	shifted_value = -2 - (-19) = -2 + 19 = 17
	#	max_val = 3		->	shifted_max = 3 - (-19) = 3 + 19 = 22
	#
	# example 3:
	# 	min_val = -19	->	shifted_min = 0
	#	value = 2		->	shifted_value = 2 - (-19) = 2 + 19 = 21
	#	max_val = 9		->	shifted_max = 9 - (-19) = 9 + 19 = 28
	#
	# example 4:
	# 	min_val = -19	->	shifted_min = 0
	#	value = -11		->	shifted_value = -11 - (-19) = -11 + 19 = 8
	#	max_val = -9	->	shifted_max = -9 - (-19) = -9 + 19 = 10
	#
	# example 5:
	# 	min_val = -19	->	shifted_min = 0
	#	value = -11		->	shifted_value = -11 - (-19) = -11 + 19 = 8
	#	max_val = 152	->	shifted_max = 152 - (-19) = 152 + 19 = 171
	#
	# example (1.)6:
	# 	min_val = -72	->	shifted_min = 0
	#	value = -55		->	shifted_value = -55 - (-72) = -55 + 72 = 17
	#	max_val = 37	->	shifted_max = 37 - (-72) = 37 + 72 = 109
	#
	# example 7:
	# 	min_val = 19	->	shifted_min = 0
	#	value = 24		->	shifted_value = 24 - 19 = 5
	#	max_val = 53	->	shifted_max = 53 - 19 = 34
	#
	# example 8 (sanity check):
	# 	min_val = 0		->	shifted_min = 0
	#	value = 24		->	shifted_value = 24
	#	max_val = 53	->	shifted_max = 53
	#
	# -->

	# <!-- custom: if min_val == 0, we return same values (min_val, value, max_val) as received -->
	shifted_min = 0
	shifted_value = value - min_val
	shifted_max = max_val - min_val

	return shifted_min, shifted_value, shifted_max

def test_expected_shifting_pre_normalize_to_100():
	# <!-- custom: examples taken from get_shifted_values's code comments -->
	vals_to_test = (
		(-19, -7, -3, 0, 12, 16),	# 1
		(-19, -2, 3, 0, 17, 22),	# 2
		(-19, 2, 9, 0, 21, 28),		# 3
		(-19, -11, -9, 0, 8, 10),	# 4
		(-19, -11, 152, 0, 8, 171),	# 5
		(-72, -55, 37, 0, 17, 109),	# (1.)6
		(19, 24, 53, 0, 5, 34),		# 7
		(0, 24, 53, 0, 24, 53),		# 8
	)

	for min_val, value, max_val, expected_shifted_min, expected_shifted_value, expected_shifted_max in vals_to_test:
		shifted_min, shifted_value, shifted_max = get_shifted_values(min_val, value, max_val)

		assert(shifted_min == expected_shifted_min)
		assert(shifted_value == expected_shifted_value)
		assert(shifted_max == expected_shifted_max)

# Attribute normalization
def round_half_away_from_zero(value):
	# <!-- custom: deterministic alternate rounder for the memory aggregate edge case described below. -->
	if value >= 0:
		return int(value + 0.5)
	return int(value - 0.5)


def round_current_runtime(value):
	# <!-- custom: Built-in round() differs at exact half ties between Civ4's Python 2.4 runtime and Python 3 predump tooling.
	# Implement ties-to-even explicitly so both paths preserve the committed contact aggregate value 52 instead of Python 2.4 returning 53 for 52.5. See KI#322. (ChatGPT-5.6-Sol + GPT-5.6-Sol) -->
	iTruncated = int(value)
	fFraction = value - iTruncated
	if fFraction > 0.5 or (fFraction == 0.5 and iTruncated % 2 != 0):
		return iTruncated + 1
	if fFraction < -0.5 or (fFraction == -0.5 and iTruncated % 2 != 0):
		return iTruncated - 1
	return iTruncated


def test_expected_rounding():
	# <!-- custom: Cover positive/negative half ties and ordinary fractions so the shared Python 2.4/Python 3 rounding contract cannot silently regress. See KI#322. (ChatGPT-5.6-Sol + GPT-5.6-Sol) -->
	assert(round_current_runtime(52.5) == 52)
	assert(round_current_runtime(53.5) == 54)
	assert(round_current_runtime(-52.5) == -52)
	assert(round_current_runtime(-53.5) == -54)
	assert(round_current_runtime(52.4) == 52)
	assert(round_current_runtime(52.6) == 53)


def normalize_to_100_with_rounder(value, min_val, max_val, B_WARN, invert, attr_name, rounder):
	# Normalizes an AI attribute value to a 0-100 integer scale.
	#
	# - First checks if min_val > max_val or if value outside [min_val, max_val].
	# - If min_val == max_val, issues a warning and returns 50.
	# - Shifts for non-zero minimums:
	#   - If min_val < 0, shifts range upwards.
	#   - If min_val > 0, shifts range downwards.
	# - Normalizes shifted_value / shifted_max.
	# - Optionally inverts the normalized value.
	# - Converts final normalized value to 0-100 integer using the supplied rounder.
	#
	# Warnings:
	# - <!-- custom: if B_WARN is set to true, --> Uniform min==max -> All normalized scores will be 50.
	#
	# Errors:
	# - <!-- custom: min_val or max_val are None -> Raises an error. -->
	# - min_val higher value than max_val -> Raises an error.
	# - value out of bounds -> Raises an error.
	# - Shifted min not equal to 0 -> Raises an error.
	# - Shifted value negative -> Raises an error.
	# - Normalized value out of bounds -> Raises an error.
	#
	# Returns:
	# - final_score (int): Normalized integer 0-100.

	# Pre-checks
	if (min_val is None) or (max_val is None):
		raise ValueError(u"[FATAL] min_val=%s or max_val=%s cannot be None, failed to fetch the real value if any exist, check your XML or fetching code." % (str(min_val), str(max_val)))

	if min_val > max_val:
		raise ValueError("Invalid min/max for attribute '%s': min_val=%d > max_val=%d" % (attr_name, min_val, max_val))

	if value < min_val or value > max_val:
		raise ValueError("Value out of range for attribute '%s': value=%d not in [%d, %d]" % (attr_name, value, min_val, max_val))

	if min_val == max_val:
		if B_WARN:
			print("[WARNING] Attribute %s has an identical min and max value (%d). All normalized values will be 50." % (attr_name, min_val))
		return 50

	# Shift min_val, value, and max_val, if needed, before we normalize
	shifted_value = None
	shifted_min, shifted_value, shifted_max = get_shifted_values(min_val, value, max_val)

	if (shifted_value is None):
		raise ValueError("[FATAL] In attr_name=%s, shifted_value=%s has not been initialized properly and is still None: shifted_min=%d, shifted_value=%d, shifted_max=%d, min_val=%d, value=%d, max_val=%d" % (attr_name, shifted_value, shifted_min, shifted_value, shifted_max, min_val, value, max_val))

	# Normalize
	if shifted_min != 0:
		raise ValueError("[FATAL] For attr_name=%s, distribution has not shifted to a shifted_min of 0 before normalization: shifted_min=%d, shifted_value=%d, shifted_max=%d, min_val=%d, value=%d, max_val=%d" % (attr_name, shifted_min, shifted_value, shifted_max, min_val, value, max_val))
	if shifted_value < 0:
		raise ValueError("[FATAL] For attr_name=%s, shifted_value cannot be negative (as shifted_min should always be 0) before normalization: shifted_min=%d, shifted_value=%d, shifted_max=%d, min_val=%d, value=%d, max_val=%d" % (attr_name, shifted_min, shifted_value, shifted_max, min_val, value, max_val))
	# <!-- custom: no need to handle division by 0 as, if shifted_max is 0, this would mean that shifted_min is 0 too since we shifted_min to 0, so in other words that min == 50 case that we already covered before so no need to cover again here but mention for exhaustiveness -->

	norm = float(shifted_value) / float(shifted_max)

	final_score = rounder(norm * 100)
	if (final_score < 0) or (final_score > 100):
		raise ValueError("Norm of %s out of range (0-100) during normalization: final_score=%.3f, shifted_min=%d, shifted_max=%d, min_val=%d, max_val=%d" % (attr_name, final_score, shifted_min, shifted_max, min_val, max_val))

	if invert:
		final_score = 100 - final_score

	return final_score


def normalize_to_100(value, min_val, max_val, B_WARN, invert, attr_name):
	return normalize_to_100_with_rounder(value, min_val, max_val, B_WARN, invert, attr_name, round_current_runtime)


def normalize_to_100_half_away_from_zero(value, min_val, max_val, B_WARN, invert, attr_name):
	# <!-- custom: The existing committed predump has one known .5-sensitive memory aggregate family where normal Python 3 round() under-validates compared with the cache: e.g. MEMORY_TRADED_TECH_TO_US affection raw 36 in range 10..90 normalizes to 32.5, but the predump stores 33.
	# Keep this explicit rather than silently changing the default contact behavior above. (ChatGPT-5.5) -->
	return normalize_to_100_with_rounder(value, min_val, max_val, B_WARN, invert, attr_name, round_half_away_from_zero)


# <!-- custom: Shared final AIP tuple formatting.
# The runtime Sevopedia cache and workflow predump checker both call these pure helpers so labels, normalized numeric values, and scale strings cannot drift independently.
# Keep this section free of CvPythonExtensions/game-context dependencies. (ChatGPT-5.5) -->
def get_aip_scale_symbols():
	return {
		"RAW_SCALE_SYMBOL": "+",
		"AGGREGATED_SCALE_SYMBOL": "#",
		"EQUAL_SCALE_SYMBOL": "=",
	}


def get_aip_symbol_scale(score, symbol):
	# <!-- custom: examples: score 64 with "+" returns "++++++"; score 39 with "#" returns "###". -->
	return symbol * (score // 10)


def get_aip_labels_as_keys_or_suffixes_with_abbreviated_tail(key_or_suffix, tail_to_trim, abbreviated_tail, label_raw, max_length):
	# <!-- custom: Shared raw-XML-name label compactor used when SAS_SEVOPEDIA_LEADER_AI_PERSONALITY_PANEL_SHOW_RAW_XML_FIELD_NAMES_INSTEAD is enabled.
	# Kept byte-for-byte behavior-equivalent to the old runtime-local helper. (ChatGPT-5.5) -->
	if tail_to_trim:
		key_or_suffix_with_tail_trimmed = key_or_suffix[:-len(tail_to_trim)]
	else:
		key_or_suffix_with_tail_trimmed = key_or_suffix

	total_tail_length = len(abbreviated_tail) + len(" ") + len(label_raw)
	room_for_first = max_length - total_tail_length
	if room_for_first <= 0:
		raise ValueError(u"[ERROR] Unexpected label_raw=%s + ' ' + abbreviated_tail=%s total_tail_length=%d, too long to fit key_or_suffix_with_tail_trimmed=%s within max_length = %d in the final label. This should not happen, please make sure abbreviated_tail + ' ' + label_raw are short enough relative to max_length, or that max_length is high enough." % (label_raw, abbreviated_tail, total_tail_length, key_or_suffix_with_tail_trimmed, max_length))
	key_or_suffix_with_tail_trimmed_further_trimmed = key_or_suffix_with_tail_trimmed[:max(1, room_for_first)]

	return key_or_suffix_with_tail_trimmed_further_trimmed + abbreviated_tail + " " + label_raw


def get_aip_labels_as_keys_or_suffixes_max_length_label(key_or_suffix, label_raw, max_length):
	# <!-- custom: Returns key_or_suffix + label_raw, trimmed so total length <= max_length.
	# This is shared with the predump checker so raw-name-mode labels are checked with the same compacting rules as runtime. (ChatGPT-5.5) -->
	if key_or_suffix.startswith("get"):
		key_or_suffix_without_front = key_or_suffix[len("get"):]

		if key_or_suffix_without_front.endswith("VictoryWeight"):
			return get_aip_labels_as_keys_or_suffixes_with_abbreviated_tail(key_or_suffix_without_front, "VictoryWeight", "", label_raw, max_length)
		elif key_or_suffix_without_front.endswith("RefuseAttitudeThreshold"):
			return get_aip_labels_as_keys_or_suffixes_with_abbreviated_tail(key_or_suffix_without_front, "RefuseAttitudeThreshold", "", label_raw, max_length)
		elif key_or_suffix_without_front.endswith("AttitudeThreshold"):
			return get_aip_labels_as_keys_or_suffixes_with_abbreviated_tail(key_or_suffix_without_front, "AttitudeThreshold", "AT", label_raw, max_length)
		elif key_or_suffix_without_front.endswith("AttitudeChangeLimit"):
			return get_aip_labels_as_keys_or_suffixes_with_abbreviated_tail(key_or_suffix_without_front, "AttitudeChangeLimit", "ACL", label_raw, max_length)
		elif key_or_suffix_without_front.endswith("AttitudeChangeDivisor"):
			return get_aip_labels_as_keys_or_suffixes_with_abbreviated_tail(key_or_suffix_without_front, "AttitudeChangeDivisor", "ACD", label_raw, max_length)
		elif key_or_suffix_without_front.endswith("AttitudeChange"):
			return get_aip_labels_as_keys_or_suffixes_with_abbreviated_tail(key_or_suffix_without_front, "AttitudeChange", "AC", label_raw, max_length)
		else:
			return get_aip_labels_as_keys_or_suffixes_with_abbreviated_tail(key_or_suffix_without_front, "", "", label_raw, max_length)
	else:
		return get_aip_labels_as_keys_or_suffixes_with_abbreviated_tail(key_or_suffix, "", "", label_raw, max_length)


def get_aip_label_with_raw_value(key_or_suffix, display_label, label_raw, max_length, is_show_raw_xml_field_names_instead):
	if is_show_raw_xml_field_names_instead:
		return get_aip_labels_as_keys_or_suffixes_max_length_label(key_or_suffix, label_raw, max_length)
	return "%s %s" % (display_label, label_raw)


def build_aip_cached_tuple(raw_value, min_value, max_value, b_invert, symbol, all_symbols, cache_key, label, B_WARN, normalizer=None):
	if normalizer is None:
		normalizer = normalize_to_100
	norm_value = normalizer(raw_value, min_value, max_value, B_WARN, b_invert, cache_key)

	if min_value == max_value:
		symbol = all_symbols["EQUAL_SCALE_SYMBOL"]

	if not label:
		raise ValueError(u"Unexpected label=%s tested false as a boolean in cache_key=%s, please check label is not empty or missing or some other kind of invalid format" % (str(label), cache_key))

	if symbol not in all_symbols.values():
		raise ValueError(u"Unexpected symbol=%s not in all_symbols=%s in cache_key=%s." % (symbol, str(all_symbols), cache_key))

	return (label, norm_value, get_aip_symbol_scale(norm_value, symbol))


def get_positive_negative(is_positive):
	if (is_positive):
		return "Positive"
	else:
		return "Negative"

def get_affection_resentment(is_affection):
	if (is_affection):
		return "Affection"
	else:
		return "Resentment"

def get_pascal_case_suffix(enumType):
	# Converts an enum constant like 'MEMORY_DECLARED_WAR' or 'CONTACT_STOP_TRADING' into a PascalCase suffix like 'DeclaredWar' or 'StopTrading'.
	#
	# This version automatically removes the first prefix (e.g., 'MEMORY_' or 'CONTACT_') instead of requiring it to be passed as a parameter.
	#
	# Args:
	# 	enumType (str): An enum string with an uppercase prefix (e.g., 'MEMORY_', 'CONTACT_').
	#
	# Returns:
	# 	str: PascalCase string of the remaining part (e.g., 'DeclaredWar').

	# Remove everything before the first underscore (e.g., 'MEMORY_DECLARED_WAR' → 'DECLARED_WAR')
	suffix = enumType.split("_", 1)[1]
	# Convert to lowercase, split on underscores, then capitalize each part
	parts = suffix.lower().split("_")
	return "".join([part.capitalize() for part in parts])

def get_aip_array_value_key(key_prefix, enum_type):
	return "%s%s" % (key_prefix, get_pascal_case_suffix(enum_type))

def get_aip_adjusted_contact_rand_key(contact_type):
	return get_aip_array_value_key("iAdjustedContactRand", contact_type)

def get_aip_adjusted_contact_delay_key(contact_type):
	return get_aip_array_value_key("iAdjustedContactDelay", contact_type)

def get_aip_aggregated_raw_contact_prob_key(contact_type):
	return get_aip_array_value_key("iAggregatedRawContactProb", contact_type)

def get_aip_aggregated_contact_prob_key(contact_type):
	return get_aip_array_value_key("iAggregatedContactProb", contact_type)

def get_aip_adjusted_memory_attitude_key(memory_type, is_affection):
	return "iAdjustedMemoryAttitudePercent%s%s" % (get_pascal_case_suffix(memory_type), get_affection_resentment(is_affection))

def get_aip_adjusted_memory_decay_key(memory_type, is_affection):
	return "iAdjustedMemoryDecay%s%s" % (get_pascal_case_suffix(memory_type), get_affection_resentment(is_affection))

def get_aip_aggregated_raw_memory_key(memory_type, is_positive, is_affection):
	return "iAggregatedRaw%sMemory%s%s" % (get_positive_negative(is_positive), get_pascal_case_suffix(memory_type), get_affection_resentment(is_affection))

def get_aip_aggregated_memory_key(memory_type, is_positive, is_affection):
	return "iAggregated%sMemory%s%s" % (get_positive_negative(is_positive), get_pascal_case_suffix(memory_type), get_affection_resentment(is_affection))


def get_aip_raw_contact_aggregate_specs(contact_types=None):
	# <!-- custom: Central synthetic raw contact specs.
	# Each tuple is (contact index, contact type, synthetic raw key); e.g. CONTACT_RELIGION_PRESSURE creates iAggregatedRawContactProbReligionPressure before min/max normalization creates iAggregatedContactProbReligionPressure. (ChatGPT-5.5) -->
	if contact_types is None:
		contact_types = get_aip_contact_types_assessed()
	specs = []
	for i in range(len(contact_types)):
		contact_type = contact_types[i]
		specs.append((i, contact_type, get_aip_aggregated_raw_contact_prob_key(contact_type)))
	return tuple(specs)


def get_aip_displayed_contact_aggregate_specs(contact_types=None):
	# <!-- custom: Displayed synthetic contact specs extend the raw specs with the normalized display/predump key and compact label, so AIP runtime and workflow checker agree on the same iAggregatedRaw* -> iAggregated* mapping. (ChatGPT-5.5) -->
	if contact_types is None:
		contact_types = get_aip_contact_types_assessed()
	contact_index_labels = get_aip_contact_index_labels()
	specs = []
	for i, contact_type, raw_key in get_aip_raw_contact_aggregate_specs(contact_types):
		specs.append((i, contact_type, raw_key, get_aip_aggregated_contact_prob_key(contact_type), contact_index_labels[i]))
	return tuple(specs)


def get_aip_raw_memory_aggregate_specs():
	# <!-- custom: Central synthetic raw memory specs.
	# This intentionally returns the full positive/negative x affection/resentment matrix even though the current panel displays only positive affections and negative resentments, keeping future UI expansion data-driven. (ChatGPT-5.5) -->
	specs = []
	for is_positive in (True, False):
		for is_affection in (True, False):
			for i in get_positive_or_negative_memory_indexes(is_positive):
				memory_type = get_aip_memory_type_by_index(i)
				specs.append((i, memory_type, is_positive, is_affection, get_aip_aggregated_raw_memory_key(memory_type, is_positive, is_affection)))
	return tuple(specs)


def get_aip_displayed_memory_aggregate_specs():
	# <!-- custom: Current displayed synthetic memory specs: positive memory affections and negative memory resentments.
	# Each tuple includes both the synthetic raw key and normalized display/predump key, e.g. iAggregatedRawPositiveMemoryTradedTechToUsAffection -> iAggregatedPositiveMemoryTradedTechToUsAffection. (ChatGPT-5.5) -->
	memory_index_labels = get_aip_memory_index_labels_assessed()
	specs = []
	for is_positive in (True, False):
		is_affection = is_positive
		for i in get_positive_or_negative_memory_indexes(is_positive):
			memory_type = get_aip_memory_type_by_index(i)
			specs.append((i, memory_type, is_positive, is_affection, get_aip_aggregated_raw_memory_key(memory_type, is_positive, is_affection), get_aip_aggregated_memory_key(memory_type, is_positive, is_affection), memory_index_labels[i]))
	return tuple(specs)


def get_aip_displayed_aggregate_value_keys():
	keys = []
	for _i, _contact_type, _raw_key, display_key, _label in get_aip_displayed_contact_aggregate_specs():
		keys.append(display_key)
	for _i, _memory_type, _is_positive, _is_affection, _raw_key, display_key, _label in get_aip_displayed_memory_aggregate_specs():
		keys.append(display_key)
	return tuple(keys)

def get_adjusted_contact_values(contact_rand_raw, contact_delay_raw, is_debug, contact_type):
	# Adjusts contact rand and contact delay values according to standard rules.
	# Returns: (adjusted_rand, adjusted_delay, aggregated_prob_force_zero_flag)
	#
	force_zero_adjusted_values = None

	if contact_delay_raw < 0:
		# <!-- custom: detail: if delay < 0 (rand is meaningless/irrelevant but we still store it (for exhaustiveness and ui display of raw value anyways)), delay is infinite, probability of contact is 0 -->
		adjusted_delay = 999 # Infinite delay
		if is_debug:
			print(u"[INFO] In contact contact_type=%s Delay < 0: adjusted delay is considered infinite, forced to zero aggregation as well. Values of these are contact_delay_raw=%d, adjusted_delay=%d, force_zero_adjusted_values=%s." % (contact_type, contact_delay_raw, adjusted_delay, str(force_zero_adjusted_values)))

		if contact_rand_raw <= 0:
			adjusted_rand = 0
			force_zero_adjusted_values = True # Forced 0 aggregation
			if is_debug:
				print(u"[INFO] In contact contact_type=%s Delay < 0, adjusted delay is considered infinite, so Rand is irrelevant, as regardless of its value we would force aggregate to zero. But for exhaustiveness, adjusting the rand as well, here to 0 since Rand <= 0. Values of these are contact_delay_raw=%d, adjusted_delay=%d, contact_rand_raw=%d, adjusted_rand=%d, force_zero_adjusted_values=%s." % (contact_type, contact_delay_raw, adjusted_delay, contact_rand_raw, adjusted_rand, str(force_zero_adjusted_values)))

			return adjusted_rand, adjusted_delay, force_zero_adjusted_values

		else:
			adjusted_rand = contact_rand_raw
			force_zero_adjusted_values = True # Forced 0 aggregation
			if is_debug:
				print(u"[INFO] In contact contact_type=%s Delay < 0, adjusted delay is considered infinite, so Rand is irrelevant, as regardless of its value we would force aggregate to zero. But for exhaustiveness, adjusting the rand as well, here to a real value equal to raw rand value, as Rand > 0 which is valid even though delay makes it eventually irrelevant and we still force aggregate in the end for this contact. Values of these are contact_delay_raw=%d, adjusted_delay=%d, contact_rand_raw=%d, adjusted_rand=%d, force_zero_adjusted_values=%s." % (contact_type, contact_delay_raw, adjusted_delay, contact_rand_raw, adjusted_rand, str(force_zero_adjusted_values)))

		return adjusted_rand, adjusted_delay, force_zero_adjusted_values

	else:
		# <!-- custom: the higher the delay the worse/lower the contact prob (example gandhi's data/values vs montezuma so we should (i think) invert both) -->
		adjusted_delay = contact_delay_raw
		if is_debug:
			print(u"[INFO] In contact contact_type=%s Delay >=0 which is valid: adjusted delay is equal to delay raw, forced to zero aggregation has yet to be determined. Values of these are contact_delay_raw=%d, adjusted_delay=%d, force_zero_adjusted_values=%s." % (contact_type, contact_delay_raw, adjusted_delay, str(force_zero_adjusted_values)))

		if contact_rand_raw <= 0:
			# <!-- custom: (and else,) if rand is <=0, AI has a compatible delay but still never engages due to rand, so probability of contact is still 0.
			# Only outside of these edge cases can the contact probabiltiy be computed if i'm not mistaken, else should be 0 as in this code block/check. -->
			# Can try, but refuses to ever engage → Aggregated ContactProb = 0
			adjusted_rand = 0
			force_zero_adjusted_values = True # Forced 0 aggregation
			if is_debug:
				print(u"[INFO] In contact contact_type=%s Delay >=0 which is valid: adjusted delay is equal to delay raw. As for forced aggregation value, Rand <= 0, so AI has a compatible delay but still never engages due to rand if i (advciv-sas mod maker based on trying to understand how the base advciv or similar or before behaves) am not mistaken, and so probability of contact is still 0. So we still force aggregate to 0 for this contact. Values of these are contact_delay_raw=%d, adjusted_delay=%d, contact_rand_raw=%d, adjusted_rand=%d, force_zero_adjusted_values=%s." % (contact_type, contact_delay_raw, adjusted_delay, contact_rand_raw, adjusted_rand, str(force_zero_adjusted_values)))

			return adjusted_rand, adjusted_delay, force_zero_adjusted_values

		else:
			# <!-- custom: (and else,), if both delay and rand are compatible with havig a scaling and acutal contact probability (that we can compute too maybe here indeed), then (to compute this we propose this formula that) the higher the delay the worse/lower the contact prob, and the higher the rand the worse/lower the contact prob (example gandhi's data/values vs montezuma so we should (i think) invert both) -->
			adjusted_rand = contact_rand_raw
			force_zero_adjusted_values = False # Normal aggregation
			if is_debug:
				print(u"[INFO] In contact contact_type=%s Delay >=0 which is valid: adjusted delay is equal to delay raw. As for forced aggregation value, Rand > 0 which is valid, so probability of contact is highest when rand is low, and decreases the higher rand is. For example a rand of 5 gives 1/5 i.e. better odds whatever the actual value is than say 1000 which would be 1 / 1000 or some transformed value that would indicate or lead to a much lower contact chance if i (advciv-sas mod maker based on trying to understand how the base advciv or similar or before behaves) am not mistaken in my understanding. So we do not force aggregate to 0 for this contact. Values of these are contact_delay_raw=%d, adjusted_delay=%d, contact_rand_raw=%d, adjusted_rand=%d, force_zero_adjusted_values=%s." % (contact_type, contact_delay_raw, adjusted_delay, contact_rand_raw, adjusted_rand, str(force_zero_adjusted_values)))

			return adjusted_rand, adjusted_delay, force_zero_adjusted_values

def get_contact_rand_and_delay_invert_flags():
	# <!-- custom: the higher the contact rand (say 200 > 50), the lower the 1/n = 1/200 vs 1/50 chance of contact event / prob from my memory of the terminology if i may say or words used in kujira about memory fields or such, so we invert -->
	b_invert_contact_rands = True
	# <!-- custom: also, the higher the delay (say 100 > 5 (turns?
	# )), the longer until next contact, so the lower the contact event / prob, so we invert delays too if i may say or not or yes or etc -->
	b_invert_contact_delays = True
	return b_invert_contact_rands, b_invert_contact_delays

def get_aggregated_raw_contact_score_from_adjusted_values(adjusted_value_rand_norm_score, adjusted_value_delay_norm_score, force_zero_adjusted_values):
	if force_zero_adjusted_values:
		return 0
	else:
		# Weight configuration
		# MAIN_WEIGHT represents the primary importance (e.g. randomness of contact)
		# The secondary weight (e.g. delay) is auto-balanced to ensure the total is 1.0
		MAIN_WEIGHT = 0.8
		SECONDARY_WEIGHT = 1.0 - MAIN_WEIGHT

		# Sanity check: weights must sum to exactly 1.0
		weight_sum = MAIN_WEIGHT + SECONDARY_WEIGHT
		if not abs(weight_sum - 1.0) < 1e-6:
			raise ValueError(u"[VALUE ERROR] Weights must sum to 1.0: got MAIN = %f, SECONDARY = %f (sum = %f)" % (MAIN_WEIGHT, SECONDARY_WEIGHT, weight_sum))

		# Aggregate score: round for consistency, since we later normalize as int 0–100 anyway
		raw_aggregated = MAIN_WEIGHT * adjusted_value_rand_norm_score + SECONDARY_WEIGHT * adjusted_value_delay_norm_score
		# <!-- custom: no reason to strictly round the raw values since they will be normalized later anyways which would/should be an int and as of now the raw aggregated contact prob is only stored before that at min max storage stage (before their normalization as said before in this sentence), but no reason not to, since most if not indeed all fields are int, and it is an approximation (aggregation) to begin with, values close enough like 78.123456 vs 78.234567 could be considered to be the same 78 for example in my understanding, so round them now even though makes data a bit more inaccurate. -->
		return int(round(raw_aggregated))

# <!-- custom: the adjust formula for memory fields is different than the contact adjust system we use in advciv-sas too for contacts, because memory fields use a positive/negative memory field while there are no positive/negative contacts (all contacts are handled the same way computationally (only displayed as offers or demands but the values are computationally treated the same between all contact types, unlike memory types that are aggregated differently based on whether they fit in positive or in negative memory types, so the adjust formula is different read after bracket(s) for rest of the explanation)), so we don't hardcode aggregated values to extremes like 0 or 999 in contact fields, and use instead a different kind of adjustment, of raw memory attitude percents and memory decays, -->
def get_adjusted_memory_values(raw_attitude_percent, raw_decay, is_affection, is_debug, mem_type):
	# Step 1: Clamp invalid affection/resentment signs to 0
	adjusted_attitude_percent = raw_attitude_percent
	if is_affection:
		# Affection must be non-negative
		if raw_attitude_percent < 0:
			adjusted_attitude_percent = 0
			if is_debug:
				print(u"[INFO] Affection memory mem_type=%s has adjusted attitude: adjusted_attitude_percent=%d. Rounded to 0." % (mem_type, adjusted_attitude_percent))
	else:
		# Resentment must be non-positive
		if raw_attitude_percent > 0:
			adjusted_attitude_percent = 0
			if is_debug:
				print(u"[INFO] Resentment memory mem_type=%s has adjusted attitude adjusted_attitude_percent=%d. Rounded to 0." % (mem_type, adjusted_attitude_percent))

	# Step 2: Adjust decay (decay must be non-negative)
	adjusted_decay = max(0, raw_decay)

	# <!-- custom: Step 3: (also was it my code comment originally or not) we --> never force aggregation to 0 for memories unlike in contact code, despite their similarities, so just one False here should be enough hopefully maybe anyways.
	force_zero_adjusted_values = False

	return adjusted_attitude_percent, adjusted_decay, force_zero_adjusted_values

def get_memory_attitude_percent_and_decay_invert_flags(is_positive, is_affection):
	if is_positive:
		if is_affection:
			# <!-- custom: higher attitude score (ex: + 350 > + 200) means more intense positive feeling (affection), closer to 0 means AI cares less (0 should be		- minimum -		attitudes after normalization (where again AI cares the least)), so we don't invert.
			# -->
			# As for decay it should remain the same as it seems to just be some time unit or span. -->
			return False, False
		else:
			# <!-- custom: lower attitude score (ex: -350 < -200) means more intense negative feeling (resentment) (resentful and (more) especially spiteful AI even for (presumably) good deeds), closer to 0 means AI cares less (0 should be		- maximum -		attitudes after normalization (where again AI also, as in positive affection, for this value 0 (after adjustment) attitude, cares the least (at least we model it a such))), so we should invert indeed.
			# More detail on why and to be careful since these are negative values unlike most civ4 data (a good fail check too maybe to review or learn for me at least anyways), is that since our normalize_to_100 function shifts to 0 distribution before normalizing (i.e. -350 is now -350 + 350 = 0, and -200 is now -200 + 350 = 150) then the lower the score is (0 vs 150 before normalization, which is (after normalization) 0 / 150 * 100 = 0 vs 100 / 150 * 100 = 100, then -350 (now 0) which was more intense(ly negative feeling (resentment)) is the lowest, while -200 (now 100) with the lowest (in comparison relatively) feeling is now the highest in score, so the atititude score should indeed be inverted, hopefully safe nowanyways.
			# -->
			return True, False

	else:
		if is_affection:
			# <!-- custom: similarly but in negative memories, higher attitude score (ex: + 350 > + 200) means more affection for them (more and more masochistic or similar AI anyways..
			# which i don't dislike but anyways...
			# Not necessarily especially like but anyways (either/too)(or why not?)
			# but anyways...), so we don't invert. -->
			return False, False
		else:
			# # <!-- custom: similarly but in negative memories, lower attitude score (ex: -350 < -200) means more intense negative feeling (resentment) (resentful and (more) especially spiteful AI but this time in an (seemingly) expected way if (conventionally) harm(ful behaviour or other thing or similar thing) is done to it), closer to 0 means AI cares less (0 should be		- maximum -		attitudes after normalization), so we invert.. -->
			return True, False

def get_aggregated_raw_positive_or_negative_memory_affection_or_resentment_score_from_adjusted_values(adjusted_value_attitude_percent_norm_score, adjusted_value_decay_norm_score, force_zero_adjusted_values):
	if force_zero_adjusted_values:
		return 0
	else:
		# <!-- custom: see the same/similar function but for contact types' code code comments for details; approximation of the ratios based on kujira's website (translate(d?)
		# to english with chrome web browser or such) https://gforestshade.github.io/kujira/post/civ4leaderheadinfos/#%e5%a4%96%e4%ba%a4%e7%9a%84%e5%87%ba%e6%9d%a5%e4%ba%8b%e3%81%ab%e3%82%88%e3%82%8b%e6%85%8b%e5%ba%a6%e8%a3%9c%e6%ad%a3 and https://gforestshade.github.io/kujira/post/civ4leaderheadinfos/#memoryattitudepercents and https://gforestshade.github.io/kujira/post/civ4leaderheadinfos/#memorydecays quite similarly to contact code -->

		# Weight configuration
		# MAIN_WEIGHT represents the primary importance (e.g. randomness of contact)
		# The secondary weight (e.g. delay) is auto-balanced to ensure the total is 1.0
		MAIN_WEIGHT = 0.8
		SECONDARY_WEIGHT = 1.0 - MAIN_WEIGHT

		# Sanity check: weights must sum to exactly 1.0
		weight_sum = MAIN_WEIGHT + SECONDARY_WEIGHT
		if not abs(weight_sum - 1.0) < 1e-6:
			raise ValueError(u"[VALUE ERROR] Weights must sum to 1.0: got MAIN = %f, SECONDARY = %f (sum = %f)"% (MAIN_WEIGHT, SECONDARY_WEIGHT, weight_sum))

		# Aggregate score: round for consistency, since we later normalize as int 0–100
		raw_aggregated = MAIN_WEIGHT * adjusted_value_attitude_percent_norm_score + SECONDARY_WEIGHT * adjusted_value_decay_norm_score
		# <!-- custom: similarly round, see the aggregated contact prob similar function to this. -->
		return int(round(raw_aggregated))


# <!-- custom: Shared pre-normalization AIP derived-value builders.
# These helpers create synthetic raw AIP fields from already-effective LeaderHeadInfo values before the normal Sevopedia min/max normalization and display-cache step.
# They are deliberately free of gc/UI imports so the in-game AIP code and the Python 3 workflow checker can reuse the same contact/memory aggregation logic with different value providers. (ChatGPT-5.5) -->
def get_positive_or_negative_memory_indexes(is_positive):
	if is_positive:
		positive_or_negative_memory_indexes = tuple(sorted(get_positive_memory_indexes_to_types().keys()))
	else:
		positive_or_negative_memory_indexes = tuple(sorted(get_negative_memory_indexes_to_types().keys()))

	if not positive_or_negative_memory_indexes:
		raise ValueError("[VALUE ERROR] memory indexes missing; please check positive/negative memory type helpers")

	return positive_or_negative_memory_indexes

def compute_leaders_info_aggregated_raw_contact_probs(non_excluded_leaders, contact_types, get_contact_rand, get_contact_delay, B_WARN, is_debug):
	# Returns leaders_info_aggregated_raw_contact_probs[iLeader][parsed_contact_key] = aggregated_raw_score.
	# parsed_contact_key is e.g. "iAggregatedRawContactProbStopTrading".
	contact_count = len(contact_types)
	parsed_adjusted_rand_names = [get_aip_adjusted_contact_rand_key(contact_type) for contact_type in contact_types]
	parsed_adjusted_delay_names = [get_aip_adjusted_contact_delay_key(contact_type) for contact_type in contact_types]
	parsed_aggregated_raw_names = [get_aip_aggregated_raw_contact_prob_key(contact_type) for contact_type in contact_types]

	temp_by_leader = {}
	min_adj_rand = [None] * contact_count
	max_adj_rand = [None] * contact_count
	min_adj_delay = [None] * contact_count
	max_adj_delay = [None] * contact_count

	for iLeader in non_excluded_leaders:
		leader_rows = [None] * contact_count

		for i in range(contact_count):
			value_1_rand_raw = get_contact_rand(iLeader, i)
			value_1_delay_raw = get_contact_delay(iLeader, i)
			adjusted_rand, adjusted_delay, b_force_zero = get_adjusted_contact_values(value_1_rand_raw, value_1_delay_raw, is_debug, contact_types[i])
			leader_rows[i] = (adjusted_rand, adjusted_delay, b_force_zero)

			if min_adj_rand[i] is None:
				min_adj_rand[i] = adjusted_rand
				max_adj_rand[i] = adjusted_rand
				min_adj_delay[i] = adjusted_delay
				max_adj_delay[i] = adjusted_delay
			else:
				if adjusted_rand < min_adj_rand[i]:
					min_adj_rand[i] = adjusted_rand
				if adjusted_rand > max_adj_rand[i]:
					max_adj_rand[i] = adjusted_rand
				if adjusted_delay < min_adj_delay[i]:
					min_adj_delay[i] = adjusted_delay
				if adjusted_delay > max_adj_delay[i]:
					max_adj_delay[i] = adjusted_delay

		temp_by_leader[iLeader] = leader_rows

	if is_debug:
		print("[DEBUG] Contact aggregation pass 1 done. min_adj_rand=%s max_adj_rand=%s min_adj_delay=%s max_adj_delay=%s" % (str(min_adj_rand), str(max_adj_rand), str(min_adj_delay), str(max_adj_delay)))

	leaders_info_aggregated_raw_contact_probs = {}
	b_invert_contact_rands, b_invert_contact_delays = get_contact_rand_and_delay_invert_flags()

	for iLeader in non_excluded_leaders:
		leaders_info_aggregated_raw_contact_probs[iLeader] = {}
		leader_rows = temp_by_leader[iLeader]
		for i in range(contact_count):
			adjusted_rand, adjusted_delay, b_force_zero = leader_rows[i]
			adjusted_rand_norm_score = normalize_to_100(adjusted_rand, min_adj_rand[i], max_adj_rand[i], B_WARN, b_invert_contact_rands, parsed_adjusted_rand_names[i])
			adjusted_delay_norm_score = normalize_to_100(adjusted_delay, min_adj_delay[i], max_adj_delay[i], B_WARN, b_invert_contact_delays, parsed_adjusted_delay_names[i])
			aggregated_value = get_aggregated_raw_contact_score_from_adjusted_values(adjusted_rand_norm_score, adjusted_delay_norm_score, b_force_zero)
			leaders_info_aggregated_raw_contact_probs[iLeader][parsed_aggregated_raw_names[i]] = aggregated_value

	if is_debug:
		print("[DEBUG] leaders_info_aggregated_raw_contact_probs after pass 2: %s" % str(leaders_info_aggregated_raw_contact_probs))

	return leaders_info_aggregated_raw_contact_probs

def compute_leaders_info_aggregated_raw_memory_family(non_excluded_leaders, memory_indexes, memory_types, get_memory_attitude_percent, get_memory_decay_rand, is_positive, is_affection, B_WARN, is_debug):
	# Returns leaders_info_aggregated_raw_memory_family[iLeader][parsed_memory_key] = aggregated_raw_score.
	positive_negative = get_positive_negative(is_positive)
	affection_resentment = get_affection_resentment(is_affection)
	parsed_adjusted_attitude_names = [get_aip_adjusted_memory_attitude_key(memory_type, is_affection) for memory_type in memory_types]
	parsed_adjusted_decay_names = [get_aip_adjusted_memory_decay_key(memory_type, is_affection) for memory_type in memory_types]
	parsed_aggregated_raw_names = [get_aip_aggregated_raw_memory_key(memory_type, is_positive, is_affection) for memory_type in memory_types]

	count = len(memory_indexes)
	temp_by_leader = {}
	min_adj_attitude = [None] * count
	max_adj_attitude = [None] * count
	min_adj_decay = [None] * count
	max_adj_decay = [None] * count

	for iLeader in non_excluded_leaders:
		leader_rows = [None] * count
		for j in range(count):
			iMemoryIndex = memory_indexes[j]
			memory_type = memory_types[j]
			attitude_percent_raw = get_memory_attitude_percent(iLeader, iMemoryIndex)
			decay_rand_raw = get_memory_decay_rand(iLeader, iMemoryIndex)
			adjusted_attitude_percent, adjusted_decay, b_force_zero = get_adjusted_memory_values(attitude_percent_raw, decay_rand_raw, is_affection, is_debug, memory_type)
			leader_rows[j] = (adjusted_attitude_percent, adjusted_decay, b_force_zero)

			if min_adj_attitude[j] is None:
				min_adj_attitude[j] = adjusted_attitude_percent
				max_adj_attitude[j] = adjusted_attitude_percent
				min_adj_decay[j] = adjusted_decay
				max_adj_decay[j] = adjusted_decay
			else:
				if adjusted_attitude_percent < min_adj_attitude[j]:
					min_adj_attitude[j] = adjusted_attitude_percent
				if adjusted_attitude_percent > max_adj_attitude[j]:
					max_adj_attitude[j] = adjusted_attitude_percent
				if adjusted_decay < min_adj_decay[j]:
					min_adj_decay[j] = adjusted_decay
				if adjusted_decay > max_adj_decay[j]:
					max_adj_decay[j] = adjusted_decay

		temp_by_leader[iLeader] = leader_rows

	if is_debug:
		print("[DEBUG] Memory aggregation pass 1 done for %s/%s" % (positive_negative, affection_resentment))
		print("[DEBUG] min_adj_attitude=%s max_adj_attitude=%s" % (str(min_adj_attitude), str(max_adj_attitude)))
		print("[DEBUG] min_adj_decay=%s max_adj_decay=%s" % (str(min_adj_decay), str(max_adj_decay)))

	leaders_info_aggregated_raw_memory_family = {}
	b_invert_attitude_percent, b_invert_decay = get_memory_attitude_percent_and_decay_invert_flags(is_positive, is_affection)

	for iLeader in non_excluded_leaders:
		leaders_info_aggregated_raw_memory_family[iLeader] = {}
		leader_rows = temp_by_leader[iLeader]
		for j in range(count):
			adjusted_attitude_percent, adjusted_decay, b_force_zero = leader_rows[j]
			adjusted_attitude_norm_score = normalize_to_100(adjusted_attitude_percent, min_adj_attitude[j], max_adj_attitude[j], B_WARN, b_invert_attitude_percent, parsed_adjusted_attitude_names[j])
			adjusted_decay_norm_score = normalize_to_100(adjusted_decay, min_adj_decay[j], max_adj_decay[j], B_WARN, b_invert_decay, parsed_adjusted_decay_names[j])
			aggregated_value = get_aggregated_raw_positive_or_negative_memory_affection_or_resentment_score_from_adjusted_values(adjusted_attitude_norm_score, adjusted_decay_norm_score, b_force_zero)
			leaders_info_aggregated_raw_memory_family[iLeader][parsed_aggregated_raw_names[j]] = aggregated_value

	if is_debug:
		print("[DEBUG] leaders_info_aggregated_raw_memory_family after pass 2 for %s/%s: %s" % (positive_negative, affection_resentment, str(leaders_info_aggregated_raw_memory_family)))

	return leaders_info_aggregated_raw_memory_family

def compute_leaders_info_aggregated_raw_memory_affections_and_resentments(non_excluded_leaders, get_memory_type, get_memory_attitude_percent, get_memory_decay_rand, B_WARN, is_debug):
	leaders_info_aggregated_raw_memory_affections_and_resentments = {}
	for is_positive in (True, False):
		for is_affection in (True, False):
			memory_indexes = get_positive_or_negative_memory_indexes(is_positive)
			memory_types = [get_memory_type(iMemoryIndex) for iMemoryIndex in memory_indexes]
			family_values = compute_leaders_info_aggregated_raw_memory_family(non_excluded_leaders, memory_indexes, memory_types, get_memory_attitude_percent, get_memory_decay_rand, is_positive, is_affection, B_WARN, is_debug)
			for iLeader in family_values.keys():
				if iLeader not in leaders_info_aggregated_raw_memory_affections_and_resentments:
					leaders_info_aggregated_raw_memory_affections_and_resentments[iLeader] = {}
				leaders_info_aggregated_raw_memory_affections_and_resentments[iLeader].update(family_values[iLeader])

	return leaders_info_aggregated_raw_memory_affections_and_resentments

# <!-- custom: Shared final numeric values for AIP-derived contact/memory fields.
# This still does not build UI labels/scales; it only turns shared raw aggregate formulas into the normalized numbers stored in the predumped cache, so workflow validation and in-game AIP can share the math without importing UI formatting.
# Example: contact rand/delay first create a pre-normalization synthetic key such as iAggregatedRawContactProbReligionPressure; this function then normalizes that raw aggregate across leaders into iAggregatedContactProbReligionPressure, which is the displayed/predumped value.
# For memories, it similarly turns iAggregatedRawPositiveMemoryTradedTechToUsAffection into iAggregatedPositiveMemoryTradedTechToUsAffection. (ChatGPT-5.5) -->
def compute_leaders_info_aip_synthetic_raw_values(non_excluded_leaders, contact_types, get_contact_rand, get_contact_delay, get_memory_type, get_memory_attitude_percent, get_memory_decay_rand, B_WARN, is_debug):
	# <!-- custom: Shared synthetic raw field creator.
	# It combines contact and memory aggregate builders into one per-leader flat dict so callers do not need to know which synthetic raw family created each iAggregatedRaw* key. (ChatGPT-5.5) -->
	contact_raw = compute_leaders_info_aggregated_raw_contact_probs(non_excluded_leaders, contact_types, get_contact_rand, get_contact_delay, B_WARN, is_debug)
	memory_raw = compute_leaders_info_aggregated_raw_memory_affections_and_resentments(non_excluded_leaders, get_memory_type, get_memory_attitude_percent, get_memory_decay_rand, B_WARN, is_debug)
	synthetic_raw_values = {}
	for iLeader in non_excluded_leaders:
		synthetic_raw_values[iLeader] = {}
		synthetic_raw_values[iLeader].update(contact_raw[iLeader])
		synthetic_raw_values[iLeader].update(memory_raw[iLeader])
	return synthetic_raw_values


def compute_leaders_info_aip_aggregate_display_values(non_excluded_leaders, contact_types, get_contact_rand, get_contact_delay, get_memory_type, get_memory_attitude_percent, get_memory_decay_rand, B_WARN, is_debug):
	synthetic_raw_values = compute_leaders_info_aip_synthetic_raw_values(non_excluded_leaders, contact_types, get_contact_rand, get_contact_delay, get_memory_type, get_memory_attitude_percent, get_memory_decay_rand, B_WARN, is_debug)
	display_values = {}
	for iLeader in non_excluded_leaders:
		display_values[iLeader] = {}

	for _contact_index, _contact_type, raw_key, display_key, _label in get_aip_displayed_contact_aggregate_specs(contact_types):
		raw_values = [synthetic_raw_values[iLeader][raw_key] for iLeader in non_excluded_leaders]
		min_value = min(raw_values)
		max_value = max(raw_values)
		for iLeader in non_excluded_leaders:
			display_values[iLeader][display_key] = normalize_to_100(synthetic_raw_values[iLeader][raw_key], min_value, max_value, B_WARN, False, display_key)

	for _memory_index, _memory_type, _is_positive, _is_affection, raw_key, display_key, _label in get_aip_displayed_memory_aggregate_specs():
		raw_values = [synthetic_raw_values[iLeader][raw_key] for iLeader in non_excluded_leaders]
		min_value = min(raw_values)
		max_value = max(raw_values)
		for iLeader in non_excluded_leaders:
			# <!-- custom: preserve the current predump's memory aggregate .5 behavior; see normalize_to_100_half_away_from_zero for the concrete MEMORY_TRADED_TECH_TO_US example. (ChatGPT-5.5) -->
			display_values[iLeader][display_key] = normalize_to_100_half_away_from_zero(synthetic_raw_values[iLeader][raw_key], min_value, max_value, B_WARN, False, display_key)

	return display_values

# <!-- custom: Shared provider-based AIP cache builder.
# Runtime Civ4 code supplies a provider backed by gc/DLL getters; workflow validation/generation supplies a provider backed by parsed XML+UWAI values.
# This keeps tuple creation, labels, normalization, scale strings, and displayed aggregate selection in one pure helper path. (ChatGPT-5.5) -->
def compute_leaders_info_aip_cache_from_provider(non_excluded_leaders, provider, is_show_raw_xml_field_names_instead, B_WARN, is_debug):
	contact_types = get_aip_contact_types_assessed()
	memory_types = get_aip_memory_types_assessed()
	all_symbols = get_aip_scale_symbols()
	raw_symbol = all_symbols["RAW_SCALE_SYMBOL"]
	aggregated_symbol = all_symbols["AGGREGATED_SCALE_SYMBOL"]

	synthetic_raw_values = compute_leaders_info_aip_synthetic_raw_values(non_excluded_leaders, contact_types, provider.get_contact_rand, provider.get_contact_delay, provider.get_memory_type, provider.get_memory_attitude_percent, provider.get_memory_decay_rand, B_WARN, is_debug)

	def get_provider_value(iLeader, key):
		if key in synthetic_raw_values[iLeader]:
			return synthetic_raw_values[iLeader][key]
		return provider.get_value(iLeader, key)

	def get_min_max_for_key(key):
		values = [get_provider_value(iLeader, key) for iLeader in non_excluded_leaders]
		return min(values), max(values)

	def get_min_max_from_values(values):
		return min(values), max(values)

	def build_label(key_or_suffix, display_label, label_raw, max_length):
		return get_aip_label_with_raw_value(key_or_suffix, display_label, label_raw, max_length, is_show_raw_xml_field_names_instead)

	leaders_info_cached = {}
	for iLeader in non_excluded_leaders:
		leader_info_cached = {}

		for getter_key, _xml_tag, _xml_default, display_label, b_invert in get_aip_direct_int_field_specs():
			raw_value = provider.get_value(iLeader, getter_key)
			min_value, max_value = get_min_max_for_key(getter_key)
			label = build_label(getter_key, display_label, "(%d)" % raw_value, 18)
			leader_info_cached[getter_key] = build_aip_cached_tuple(raw_value, min_value, max_value, b_invert, raw_symbol, all_symbols, getter_key, label, B_WARN)

		for getter_key, _xml_tag, _xml_default, display_label, b_invert in get_aip_attitude_threshold_field_specs():
			raw_value = provider.get_value(iLeader, getter_key)
			min_value, max_value = get_min_max_for_key(getter_key)
			label = build_label(getter_key, display_label, "(%d)" % raw_value, 18)
			leader_info_cached[getter_key] = build_aip_cached_tuple(raw_value, min_value, max_value, b_invert, raw_symbol, all_symbols, getter_key, label, B_WARN)

		for i, flavor_type in enumerate(get_aip_flavor_types_assessed()):
			suffix = get_pascal_case_suffix(flavor_type)
			cache_key = get_aip_array_value_key("iFlavor", flavor_type)
			raw_value = provider.get_flavor_value(iLeader, i)
			min_value, max_value = get_min_max_from_values([provider.get_flavor_value(loopLeader, i) for loopLeader in non_excluded_leaders])
			label = build_label(suffix, suffix, "(%d)" % raw_value, 19)
			leader_info_cached[cache_key] = build_aip_cached_tuple(raw_value, min_value, max_value, False, raw_symbol, all_symbols, cache_key, label, B_WARN)

		for i, contact_type, raw_key, display_key, display_label in get_aip_displayed_contact_aggregate_specs(contact_types):
			suffix = get_pascal_case_suffix(contact_type)
			raw_value = synthetic_raw_values[iLeader][raw_key]
			min_value, max_value = get_min_max_for_key(raw_key)
			label_raw = "(%d/%d)" % (provider.get_contact_rand(iLeader, i), provider.get_contact_delay(iLeader, i))
			label = build_label(suffix, display_label, label_raw, 19)
			leader_info_cached[display_key] = build_aip_cached_tuple(raw_value, min_value, max_value, False, aggregated_symbol, all_symbols, display_key, label, B_WARN)

		for i, memory_type, _is_positive, _is_affection, raw_key, display_key, display_label in get_aip_displayed_memory_aggregate_specs():
			suffix = get_pascal_case_suffix(memory_type)
			raw_value = synthetic_raw_values[iLeader][raw_key]
			min_value, max_value = get_min_max_for_key(raw_key)
			label_raw = "(%d/%d)" % (provider.get_memory_attitude_percent(iLeader, i), provider.get_memory_decay_rand(iLeader, i))
			label = build_label(suffix, display_label, label_raw, 19)
			# <!-- custom: Preserve the current predump's memory aggregate .5 behavior; see normalize_to_100_half_away_from_zero for the concrete MEMORY_TRADED_TECH_TO_US example. (ChatGPT-5.5) -->
			leader_info_cached[display_key] = build_aip_cached_tuple(raw_value, min_value, max_value, False, aggregated_symbol, all_symbols, display_key, label, B_WARN, normalize_to_100_half_away_from_zero)

		for i, attitude_type in enumerate(get_aip_no_war_attitude_types_assessed()):
			suffix = get_pascal_case_suffix(attitude_type)
			cache_key = get_aip_array_value_key("iNoWarAttitudeProb", attitude_type)
			raw_value = provider.get_no_war_attitude_prob(iLeader, i)
			min_value, max_value = get_min_max_from_values([provider.get_no_war_attitude_prob(loopLeader, i) for loopLeader in non_excluded_leaders])
			label = build_label(suffix, suffix, "(%d)" % raw_value, 19)
			leader_info_cached[cache_key] = build_aip_cached_tuple(raw_value, min_value, max_value, False, raw_symbol, all_symbols, cache_key, label, B_WARN)

		leaders_info_cached[iLeader] = leader_info_cached
		if is_debug:
			print(u"[DEBUG] Leader info cached for iLeader=%d is leader_info_cached=%s" % (iLeader, str(leader_info_cached)))

	return leaders_info_cached

# <!-- custom: Shared predump module serializer for both the in-game PythonDbg.log fallback and the outside-Civ4 workflow writer.
# The callers provide only source-specific header lines and leader-count text; markers, common generation metadata, and deterministic cache literal formatting stay in one place.
# Keep this Python 2.4-safe because the in-game AIP code imports this module.
# No real generation timestamp is written: the writer should be safe to rerun as a no-op without creating false diffs or future bot/maintenance churn. (ChatGPT-5.5) -->
def format_aip_leader_index_type_pairs(index_type_pairs):
	return ", ".join(["%d: %s" % (iLeader, leader_type) for iLeader, leader_type in index_type_pairs])



def build_aip_predump_module_lines(leaders_info_cached, leader_count_line, excluded_leader_types, leader_index_type_line, is_emoji_enabled, is_raw_xml_names, source_header_lines, b_include_file_header):
	lines = []
	if b_include_file_header:
		lines.extend(get_sas_python_file_header_lines())

	lines.append("# === SAS_LEADER_AI_CACHE_PYMODULE_BEGIN ===")
	for line in source_header_lines:
		lines.append(line)
	lines.append("# No timestamp is stored; no-op --write runs stay byte-identical when cache data is unchanged.")
	lines.append("#")
	lines.append("# Generation info:")
	lines.append(leader_count_line)
	lines.append("#   - Excluded leader types: %r" % (excluded_leader_types,))
	lines.append("#   - Leader index:type pairs: %s" % leader_index_type_line)
	lines.append("#   - Emoji headers enabled: %s" % str(is_emoji_enabled))
	lines.append("#   - Raw XML field names: %s" % str(is_raw_xml_names))
	lines.append("LEADERS_INFO_CACHED = {")
	for iLeader in sorted(leaders_info_cached.keys()):
		lines.append("	%r: {" % iLeader)
		leader_info_cached = leaders_info_cached[iLeader]
		for cache_key in sorted(leader_info_cached.keys()):
			lines.append("		%r: %r," % (cache_key, leader_info_cached[cache_key]))
		lines.append("	},")
	lines.append("}")
	lines.append("# === SAS_LEADER_AI_CACHE_PYMODULE_END ===")
	return lines

