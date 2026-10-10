# AI, UI, logging, or other modifications first developed in AdvCiv-SAS (Simple Advanced Strategy)
# (c) 2026 wonderingabout & AI/LLM helpers (see Authors in AdvCiv-SAS's root README.md)
#
# <!-- custom: moved some functions that are not used in games (outside of debugging code or such, not use in a "normal" game if i may say), especially the very nice i mean debug leaderheadinfo one but not only, (another) other functions(function) as well, moving them here as it would be heavy and unclean in this case and would also clutter needlessly the other helpers. -->

# <!-- custom: modified from Claude AI's solution. -->
# <!-- custom: be careful/make sure to launch this at interfaceScreen level or later, else at __init__ this/the output is still empty, so run this later to have the actual tech info output; but also be careful to not launch/run/call this later than your error line if you have any, else the code execution would stop before you reach your debugger's call and you won't see any debug (e.g. if you have a code error at line 200, call debugger method at line 199 or sooner, not at line 201+ as it would never be reached before execution stops and you don't see debug content), a good place may be the top of interfaceScreen generally i would say but check to be sure -->
# Get all attributes of the object

from CvPythonExtensions import *
from ai_utils_shared_with_civ4 import get_aip_attitude_index_to_type

gc = CyGlobalContext()
localText = CyTranslator()

def printObjAttrs(obj):
	print ("[DEBUG] Beginning of show obj inner fields.")

	for attr in dir(obj):
		try:
			# Try to get the attribute
			attr_value = getattr(obj, attr)

			# Check if it's a method (callable)
			if callable(attr_value):
				print(u"%s\n" % attr)
			else:
				print(u"%s = %s\n" % (attr, attr_value))
		except:
			print(u"%s - Error accessing\n" % attr)

	print ("[DEBUG] End of show show obj inner fields.")

def debugPrintLeaderHeadInfoFieldsToFetch(iLeader):
	# <!-- custom: first skip these/the fields below we don't use/need in our AI personality panel for example in LEADER_DEFAULTS even though said in code sample too but to be exhaustive:
	#
	#	<Type>LEADER_DEFAULTS</Type>
	#	<Description></Description>
	#	<Civilopedia></Civilopedia>
	#	<ArtDefineTag></ArtDefineTag>
	#
	# then: -->

	print("\n\n[DEBUG] For iLeader=%d, leader head info debugged as such:" % iLeader)

	# <!-- custom: more computationally efficient to store closest pointer since we have many/multiple calls to make as chatgpt had done before and i was too dumb or rather maybe uninformed to understand/know, even if difference is minimal or not, and even if we don't use this except when debugging, no reason to not implement it as well.
	# I (had) wanted for method call for clarity, but if this is more efficient better do it as so then maybe. -->
	info = gc.getLeaderHeadInfo(iLeader)

	# <!-- custom: try to follow XML order as much as possible and to be sure we have all fields too -->
	print("\n\n==== FIRST XML FIELDS PART 1 (from XML order) ====")
	print("iWonderConstructRand: %d" % info.getWonderConstructRand())
	print("iBaseAttitude: %d" % info.getBaseAttitude())
	print("iBasePeaceWeight: %d" % info.getBasePeaceWeight())
	print("iPeaceWeightRand: %d" % info.getPeaceWeightRand())
	print("iWarmongerRespect: %d" % info.getWarmongerRespect())
	print("iEspionageWeight: %d" % info.getEspionageWeight())
	print("iRefuseToTalkWarThreshold: %d" % info.getRefuseToTalkWarThreshold())
	print("iNoTechTradeThreshold: %d" % info.getNoTechTradeThreshold())
	print("iTechTradeKnownPercent: %d" % info.getTechTradeKnownPercent())
	print("iMaxGoldTradePercent: %d" % info.getMaxGoldTradePercent())
	print("iMaxGoldPerTurnTradePercent: %d" % info.getMaxGoldPerTurnTradePercent())

	# <!-- custom: also add BBAI Victory weights now that we have exposed them / expose them to python as well, see todo docs for details and todo add .cpp code comment filename -->
	print("\n\n==== BBAI VICTORY WEIGHTS ====")
	print("Culture Victory Weight: %d" % info.getCultureVictoryWeight())
	print("Space Victory Weight: %d" % info.getSpaceVictoryWeight())
	print("Conquest Victory Weight: %d" % info.getConquestVictoryWeight())
	print("Domination Victory Weight: %d" % info.getDominationVictoryWeight())
	print("Diplomacy Victory Weight: %d" % info.getDiplomacyVictoryWeight())

	# <!-- custom: then war fields continue as per XML order -->
	print("\n\n==== WAR XML FIELDS (from XML order) ====")
	print("iMaxWarRand: %d" % info.getMaxWarRand())
	print("iMaxWarNearbyPowerRatio: %d" % info.getMaxWarNearbyPowerRatio())
	print("iMaxWarDistantPowerRatio: %d" % info.getMaxWarDistantPowerRatio())
	print("iMaxWarMinAdjacentLandPercent: %d" % info.getMaxWarMinAdjacentLandPercent())
	print("iLimitedWarRand: %d" % info.getLimitedWarRand())
	print("iLimitedWarPowerRatio: %d" % info.getLimitedWarPowerRatio())
	print("iDogpileWarRand: %d" % info.getDogpileWarRand())
	print("iMakePeaceRand: %d" % info.getMakePeaceRand())
	print("iDeclareWarTradeRand: %d" % info.getDeclareWarTradeRand())
	print("iDemandRebukedSneakProb: %d" % info.getDemandRebukedSneakProb())
	print("iDemandRebukedWarProb: %d" % info.getDemandRebukedWarProb())
	print("iRazeCityProb: %d" % info.getRazeCityProb())
	print("iBuildUnitProb: %d" % info.getBuildUnitProb())

	print("\n\n==== ATTITUDE MODIFIER FIELDS (from XML order) ====")
	print("iBaseAttackOddsChange: %d" % info.getBaseAttackOddsChange())
	print("iAttackOddsChangeRand: %d" % info.getAttackOddsChangeRand())
	print("iWorseRankDifferenceAttitudeChange: %d" % info.getWorseRankDifferenceAttitudeChange())
	print("iBetterRankDifferenceAttitudeChange: %d" % info.getBetterRankDifferenceAttitudeChange())
	print("iCloseBordersAttitudeChange: %d" % info.getCloseBordersAttitudeChange())
	print("iLostWarAttitudeChange: %d" % info.getLostWarAttitudeChange())
	print("iAtWarAttitudeDivisor: %d" % info.getAtWarAttitudeDivisor())
	print("iAtWarAttitudeChangeLimit: %d" % info.getAtWarAttitudeChangeLimit())
	print("iAtPeaceAttitudeDivisor: %d" % info.getAtPeaceAttitudeDivisor())
	print("iAtPeaceAttitudeChangeLimit: %d" % info.getAtPeaceAttitudeChangeLimit())
	print("iSameReligionAttitudeChange: %d" % info.getSameReligionAttitudeChange())
	print("iSameReligionAttitudeDivisor: %d" % info.getSameReligionAttitudeDivisor())
	print("iSameReligionAttitudeChangeLimit: %d" % info.getSameReligionAttitudeChangeLimit())
	print("iDifferentReligionAttitudeChange: %d" % info.getDifferentReligionAttitudeChange())
	print("iDifferentReligionAttitudeDivisor: %d" % info.getDifferentReligionAttitudeDivisor())
	print("iDifferentReligionAttitudeChangeLimit: %d" % info.getDifferentReligionAttitudeChangeLimit())
	print("iBonusTradeAttitudeDivisor: %d" % info.getBonusTradeAttitudeDivisor())
	print("iBonusTradeAttitudeChangeLimit: %d" % info.getBonusTradeAttitudeChangeLimit())
	print("iOpenBordersAttitudeDivisor: %d" % info.getOpenBordersAttitudeDivisor())
	print("iOpenBordersAttitudeChangeLimit: %d" % info.getOpenBordersAttitudeChangeLimit())
	print("iDefensivePactAttitudeDivisor: %d" % info.getDefensivePactAttitudeDivisor())
	print("iDefensivePactAttitudeChangeLimit: %d" % info.getDefensivePactAttitudeChangeLimit())
	print("iShareWarAttitudeChange: %d" % info.getShareWarAttitudeChange())
	print("iShareWarAttitudeDivisor: %d" % info.getShareWarAttitudeDivisor())
	print("iShareWarAttitudeChangeLimit: %d" % info.getShareWarAttitudeChangeLimit())
	print("iFavoriteCivicAttitudeChange: %d" % info.getFavoriteCivicAttitudeChange())
	print("iFavoriteCivicAttitudeDivisor: %d" % info.getFavoriteCivicAttitudeDivisor())
	print("iFavoriteCivicAttitudeChangeLimit: %d" % info.getFavoriteCivicAttitudeChangeLimit())

	# <!-- custom: there are "AttitudeThreshold" and "RefuseAttitudeThreshold", handle the most common case, they seem to all be about refusing something or not being able to do it if threshold is not met, handle them as such -->
	print("\n\n==== ATTITUDE THRESHOLDS ====")
	# <!-- custom:, based and comparing xml with debug values for leader_gandhi and leader_ragnar (for the furious value in leader_ragnar), here is the table conversion map of attitude to num: none: -1, furious: 0, annoyed: 1, cautious: 2, pleased: 3, friendly: 4
	#
	# Overall very similar to our leaders_data map, with none being last/lowest/most forgiving value (less than furious), and friendly being the highest, so we can use this map of the dll directly without needing to parse atittude str to num for our ranking of leaders in AI personality panel directly -->
	#
	# <!-- custom: Among all leaders, LEADER_ELIZABETH and LEADER_TOKUGAWA only have a value of 5 somehow, which triggers an error with our current map:
	#
	# Traceback (most recent call last):
	#   File "CvScreensInterface", line 495, in pediaJumpToLeader
	#   File "SevoPediaMain", line 313, in pediaJump
	#   File "SevoPediaLeader", line 455, in interfaceScreen
	#   File "_sevopedia_helpers", line 134, in debugPrintLeaderHeadInfoFieldsToFetch
	#
	# KeyError: 5
	# ERR: Python function pediaJumpToLeader failed, module CvScreensInterface
	#
	# however LEADER_ELIZABETH 's XML is:
	# 		<MapRefuseAttitudeThreshold>ATTITUDE_FRIENDLY</MapRefuseAttitudeThreshold>
	#
	# and LEADER_TOKUGAWA 's XML is:
	# 		<MapRefuseAttitudeThreshold>ATTITUDE_FRIENDLY</MapRefuseAttitudeThreshold>
	# Exactly the same as of LEADER_ELIZABETH, this is the field causing the key error which should map to 4, not to 5
	#
	# I have done a global search and they are the only leaders with "<MapRefuseAttitudeThreshold>ATTITUDE_FRIENDLY</MapRefuseAttitudeThreshold>", so this seems most likely intended that map trading in base advciv code at least if not other mods preceding it or and base civ4 or not handle it as such (increment one level of atittude higher than the value, to make map trading harsher, so friendly means they will always refuse map trading i assume / in my assumption, all other leaders don't trigger a key error nor in/at this field nor in any other field, so keep as is and display it accurately as DLL handles it) -->

	# <!-- custom: Map debug attitude integers back to Civ4 XML constants through the shared AIP helper; includes debug-only 5: "ALWAYS??"
	# for the AdvCiv map-trade refusal edge case explained above. (ChatGPT-5.5) -->
	DLL_ATTITUDE_MAP = get_aip_attitude_index_to_type(True)

	for attr in dir(info):
		if attr.startswith("get") and attr.endswith("AttitudeThreshold"):
			value = getattr(info, attr)()
			attitude_to_str = DLL_ATTITUDE_MAP[value]
			print("%s: %d (%s)" % (attr, value, attitude_to_str))

	print("\n\n==== VASSAL AND FREEDOM FIELDS (from XML order) ====")
	print("iVassalPowerModifier: %d" % info.getVassalPowerModifier())
	print("iFreedomAppreciation: %d" % info.getFreedomAppreciation())

	# <!-- custom: then we skip these (for example in/from LEADER_DEFAULTS):
	# 		<FavoriteCivic>NONE</FavoriteCivic>
	# 		<FavoriteReligion>NONE</FavoriteReligion>
	# 		<Traits/>
	# as we don't need them/display them in ai personality panel; then resume at fields below: -->

	print("\n\n==== FLAVORS ====")
	# <!-- custom: for flavor fields, unlike nowar (ctrl+f "attitude"), contact (ctrl+f "contact"), and memory (ctrl+f "memory") fields, it seems there are 2 occurences of methods that have "flavor" in their name, so no need to specify a number of values to loop over as in below code -->
	for i in range(gc.getNumFlavorTypes()):
		name = gc.getFlavorTypes(i)
		value = info.getFlavorValue(i)
		print("Flavor %d (%s): %d" % (i, name, value))

	print("\n\n==== CONTACTS ====")
	# <!-- custom: 15th (0 to 13 is 14 values in total) value has seemingly junk code (for example for leader gandhi Contact 14 (): Delay 1117701140, Rand 1110361188), stop at this last one -->
	NUM_CONTACT_TYPES_ASSESSED = 14
	for i in range(NUM_CONTACT_TYPES_ASSESSED):
		name = gc.getContactTypes(i)
		value_rand = info.getContactRand(i)
		value_delay = info.getContactDelay(i)
		print("Contact %d (%s): Rand %d, Delay %d" % (i, name, value_rand, value_delay))

	print("\n\n==== MEMORY ====")
	# <!-- custom: 38th (0 to 36 is 37 values in total) value results in an error:
	# AttributeError: 'NoneType' object has no attribute 'getType'
	# ERR: Python function pediaJumpToLeader failed, module CvScreensInterface
	# , so stop at this last one -->
	# <!-- custom: note: here no positive/negative different memory type or index handling, as we are simply debugging and directly displaying the XML as it is, no aggregation, no adjustment of any value, so we can use range and loop over all NUM_MEMORY_TYPES_ASSESSED instead of using positive_or_negative_memory_indexes specifically instead depending on if memory index is of a positive or of a negative memory in sevopedia leader, so here in debugging (i.e. sevopedia_helpers py file) we can respect and be consistent with the contact approach more strictly/reliably.. -->
	NUM_MEMORY_TYPES_ASSESSED = 37
	for i in range(NUM_MEMORY_TYPES_ASSESSED):
		name = gc.getMemoryInfo(i).getType()
		value_attitude_percent = info.getMemoryAttitudePercent(i)
		value_decay = info.getMemoryDecayRand(i)
		print("Memory %d (%s): AttitudePercent %d, Decay %d" % (i, name, value_attitude_percent, value_decay))

	print("\n\n==== NOWARATTITUDEPROBS ====")
	# <!-- custom: stop at 4 because there are only 5 attitudes (0 furious to 4 friendly is a total of 5 values), and trying to access the 6th non-existent in this case value returns this error:
	# AttributeError: 'NoneType' object has no attribute 'getDescription'
	# ERR: Python function pediaJumpToLeader failed, module CvScreensInterface
	# , so stop at last value value/one -->
	NUM_ATTITUDE_TYPES_ASSESSED = 5
	for i in range(NUM_ATTITUDE_TYPES_ASSESSED):
		name = gc.getAttitudeInfo(i).getDescription()
		value = info.getNoWarAttitudeProb(i)
		print("NoWarAttitudeProb %d (%s): %d" % (i, name, value))

	# <!-- custom: then skip all remaining fields, for example in LEADER_DEFAULTS 's XML:
	#
	# 		<!-- advc.104: No AdvCiv leader uses this; for mod-mods maybe. See
	# 			 UWAI_WEIGHT_LOVE_OF_PEACE in AI_Variables_GlobalDefines.xml.
	# 			 Also reduces war utility counted for "greed". -->
	# 		<iLoveOfPeace>0</iLoveOfPeace>
	# 		<UnitAIWeightModifiers/>
	# 		<ImprovementWeightModifiers/>
	# 		<DiplomacyIntroMusicPeace>
	# 			etc.
	# 		</DiplomacyIntroMusicPeace>
	# 		<DiplomacyMusicPeace>
	# 			etc.
	# 		</DiplomacyMusicPeace>
	# 		<DiplomacyIntroMusicWar>
	# 			etc.
	# 		</DiplomacyIntroMusicWar>
	# 		<DiplomacyMusicWar>
	# 			etc.
	# 		</DiplomacyMusicWar>
	#
	# and we have reached end of a leader info's xml, hopefully we parsed all we needed -->
	# <!-- custom: see as of now _sevopedia_debuggers_Leader.txt for example of output. -->
