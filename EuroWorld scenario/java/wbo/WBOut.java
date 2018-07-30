package wbo;
import java.awt.Point;
import java.awt.geom.Point2D;
import java.util.Arrays;
import java.util.Optional;
import java.util.Set;
import java.util.HashSet;
import java.util.ArrayList;

import static wbo.Direction.*;

public class WBOut {
	
	public static final int offsetX = 8, offsetY = 4;

	public static void main(String[] args) {

		final int w = 120, h = 72;
		String preamble[] = {
				"Version=11",
				"BeginGame",
				"\tSpeed=GAMESPEED_EPIC",
				"\tCalendar=CALENDAR_DEFAULT",
				//"\tOption=GAMEOPTION_RAGING_BARBARIANS",
				"\tOption=GAMEOPTION_NO_EVENTS",
				"\tGameTurn=0",
				"\tStartYear=-3600",
				"\tDescription=Eurocentric world map to replace Earth18Civs. " +
				"Custom projection with some resemblance to the Bonne projection. " +
				"Optimized for the Advanced Civ mod, but no mod is required. " +
				"Work in progress.",
				"EndGame",
		};
		for(String line : preamble)
			pr(line);
		String[] blockedReligions = {"HINDUISM","CONFUCIANISM","TAOISM"};
		final boolean placeBarbs = false;
		Civ civs[] = {
				new Civ("ASOKA", "INDIA", 82, 36, "MYSTICISM", "THE_WHEEL",
						new String[]{"PATALIPUTRA","AMARAVATI",/*"UJJAIN"*/}),
				new Civ("JULIUS_CAESAR"/*"AUGUSTUS"*/, "ROME", 46, 40, "FISHING", "MINING",
						new String[]{"ROME"/*,"SYRACUSE"*/,"LUGDUNUM"}), // MARSEILLES?
				new Civ("CHARLEMAGNE", "CELT", 47, 46, "MINING", "THE_WHEEL",
						new String[]{"PRAGUE"}),
				new Civ("DARIUS"/*"CYRUS"*/, "PERSIA", 67, 32, "THE_WHEEL", "MYSTICISM",
						new String[]{"PERSEPOLIS","ECBATANA","MERV","KANDAHAR"/*,"SAMARQAND"*/}),
				new Civ("GENGHIS_KHAN", "MONGOL", 20, 10, "MINING", "HUNTING",
						new String[]{"TARTAR"}),
				new Civ(placeBarbs ? "HATSHEPSUT" : "RAMESSES", "EGYPT", 56, 31, "AGRICULTURE", "MINING",
						new String[]{"MEMPHIS","THEBES","PHEONICIAN"}),
				new Civ("CHINESE_LEADER", "CHINA", 30, 10, "AGRICULTURE", "THE_WHEEL",
						new String[]{"XIAN"}),
				new Civ(placeBarbs ? "JUSTINIAN" : "PERICLES"/*"ALEXANDER"*/, "GREECE", 52, 37, "FISHING", "AGRICULTURE",
						new String[]{"ATHENS","THRACIAN"}),
				new Civ("PETER", "RUSSIA", 61, 47, "HUNTING", "THE_WHEEL",
						new String[]{"SAMARA","MOSCOW","ROSTOV"}),
				new Civ("RAGNAR", "ENGLAND", 45, 53, "FISHING", "HUNTING",
						new String[]{"OSLO","ROSKILDE","UPPSALA","NIDAROS","JORVIK"}),
				new Civ("SALADIN", "ARABIA", 63, 33, "AGRICULTURE", "THE_WHEEL",
						new String[]{"BABYLON","NINEVEH","MECCA","HITTITE","VAN"}),
				new Civ("SURYAVARMAN", "KHMER", 40, 10, "FISHING", "AGRICULTURE",
						new String[]{"NINGBO"}),//"GUANGZHOU"?
				new Civ("TOKUGAWA", "JAPAN", 45, 10, "FISHING", "MYSTICISM",
						new String[]{"AOMORI"}, "", true, true, blockedReligions),
				new Civ("ZARA_YAQOB", "ETHIOPIA", 59, 24, "AGRICULTURE", "",
						new String[]{"AKSUM","NUBIAN"}, "PRINCE"),
				new Civ("MANSA_MUSA", "MALI", 36, 29, "MINING", "",
						new String[]{"KUMBISALEH"}, "MONARCH"),
				new Civ("SHAKA", "ZULU", 55, 10, "HUNTING", "",
						new String[]{"BULAWAYO"}, "EMPEROR", false),
				new Civ("PACAL", "MAYA", 65, 10, "", "",
						new String[]{"XUKPI"}, "IMMORTAL", false),
				new Civ("HUAYNA_CAPAC", "INCA", 70, 10, "", "",
						new String[]{"TIWANAKU"}, "DEITY", false),
		};
		Arrays.sort(civs);
		for(Civ c : civs)
			pr(c.teamEntry());
		pr("");
		for(int i = 0; i < civs.length; i++)
			pr(civs[i].civEntry(i));
		pr("");
		int numPlots = w * h;
		// Only territory for 10 leaders so far
		String worldSz = "STANDARD"; // "LARGE" "HUGE"
		String mapStr[] = {
				"BeginMap",
				"\tgrid width=" + w,
				"\tgrid height=" + h,
				"\twrap X=1",
				"\twrap Y=0",
				"\ttop latitude=80",
				"\tbottom latitude=-70",
				"\tworld size=WORLDSIZE_" + worldSz,
				"\tclimate=CLIMATE_TEMPERATE",
				"\tsealevel=SEALEVEL_LOW",
				"\tnum plots written=" + numPlots,
				"EndMap",
		};
		for(String line : mapStr)
			pr(line);
		pr("");
		Set<Point2D> euStarts = new HashSet<Point2D>();
		for(Civ c : civs)
			euStarts.add(c.getStart());
		final String[] barbGarrison = { "ARCHER", "WARRIOR" };
		final Set<BarbCity> barbs = placeBarbs ? new HashSet<BarbCity>(
				Arrays.asList(new BarbCity[] {
				new BarbCity("SAMARQAND",71,41),
				new BarbCity("ARYAN",75,35),
				new BarbCity("PHEONICIAN",59,33,new String[]{"JUDAISM"}),
				new BarbCity("NUMIDIAN",40,35),
				//new BarbCity("GEPID",50,47),
				new BarbCity("MAGYAR",50,44),
				/* This doesn't work - civs discovering a religion tech get
				 * to found a different religion then, e.g. Christianity from
				 * Polytheism. */
				//new BarbCity("dummy",10,35,blockedReligions),
		})) : new HashSet<BarbCity>(
				Arrays.asList(new BarbCity[] {
				// To keep civs from rushing into the Indus valley 
				new BarbCity("ARYAN",73,32),
		}));
		final boolean magyars = barbs.stream().anyMatch(p -> p.getName().equals("MAGYAR") || p.getName().equals("GEPID")); 
		final Set<Point2D> desert = new HashSet<Point2D>(
				Arrays.asList(new Point[] {
				// Sinai, Negev
				new Point(60,31),
				new Point(59,31),
				new Point(58,31),
				new Point(58,30),
				// Eastern Desert
				new Point(57,31),
				new Point(57,30),
				new Point(57,29),
				new Point(58,29),
				new Point(58,28),
				new Point(56,27),
				new Point(57,27),
				new Point(58,27),
				new Point(55,26),
				new Point(56,26),
				new Point(57,26),
				new Point(58,26),
				new Point(59,26),
				new Point(57,25),
				new Point(58,25),
				new Point(59,25),
				new Point(56,24),
				new Point(57,24),
				// Sahara (rest)
				new Point(34,32),
				new Point(35,32),
				new Point(36,32),
				new Point(37,32),
				new Point(38,32),
				new Point(39,32),
				new Point(40,32),
				new Point(41,32),
				new Point(42,32),
				new Point(43,32),
				new Point(44,32),
				new Point(45,32),
				new Point(35,33),
				new Point(36,33),
				new Point(37,33),
				new Point(38,33),
				new Point(39,33),
				new Point(40,33),
				new Point(41,33),
				new Point(42,33),
				new Point(43,33),
				new Point(44,33),
				new Point(45,33),
				new Point(36,34),
				new Point(38,34),
				new Point(39,34),
				new Point(40,34),
				new Point(41,34),
				new Point(42,34),
				new Point(34,31),
				new Point(35,31),
				new Point(36,31),
				new Point(37,31),
				new Point(38,31),
				new Point(39,31),
				new Point(40,31),
				new Point(41,31),
				new Point(42,31),
				new Point(43,31),
				new Point(44,31),
				new Point(45,31),
				new Point(46,31),
				new Point(47,31),
				new Point(51,31),
				new Point(52,31),
				new Point(53,31),
				new Point(54,31),
				new Point(56,31),
				new Point(34,30),
				new Point(37,30),
				new Point(38,30),
				new Point(39,30),
				new Point(40,30),
				new Point(41,30),
				new Point(42,30),
				new Point(43,30),
				new Point(44,30),
				new Point(45,30),
				new Point(46,30),
				new Point(47,30),
				new Point(48,30),
				new Point(49,30),
				new Point(50,30),
				new Point(51,30),
				new Point(52,30),
				new Point(53,30),
				new Point(54,30),
				new Point(55,30),
				new Point(56,30),
				new Point(41,29),
				new Point(42,29),
				new Point(43,29),
				new Point(44,29),
				new Point(45,29),
				new Point(46,29),
				new Point(47,29),
				new Point(48,29),
				new Point(49,29),
				new Point(50,29),
				new Point(51,29),
				new Point(52,29),
				new Point(53,29),
				new Point(54,29),
				new Point(55,29),
				new Point(56,29),
				new Point(42,28),
				new Point(43,28),
				new Point(44,28),
				new Point(45,28),
				new Point(46,28),
				new Point(47,28),
				new Point(48,28),
				new Point(49,28),
				new Point(50,28),
				new Point(51,28),
				new Point(52,28),
				new Point(53,28),
				new Point(54,28),
				new Point(55,28),
				new Point(56,28),
				new Point(57,28),
				new Point(46,27),
				new Point(47,27),
				new Point(48,27),
				new Point(49,27),
				new Point(50,27),
				new Point(51,27),
				new Point(52,27),
				new Point(53,27),
				new Point(54,27),
				new Point(55,27),
				new Point(47,26),
				new Point(48,26),
				new Point(49,26),
				new Point(50,26),
				new Point(51,26),
				new Point(52,26),
				new Point(53,26),
				new Point(54,26),
				new Point(50,25),
				new Point(51,25),
				new Point(52,25),
				new Point(53,25),
				new Point(54,25),
				new Point(55,25),
				new Point(56,25),
				new Point(51,24),
				new Point(52,24),
				new Point(53,24),
				new Point(54,24),
				new Point(55,24),
				new Point(56,24),
				// Danakil
				new Point(60,24),
				new Point(60,23),
				// Somalia
				new Point(61,22),
				new Point(62,22),
				new Point(63,22),
				new Point(64,22),
				new Point(65,22),
				new Point(61,21),
				new Point(62,21),
				new Point(63,21),
				new Point(64,21),
				new Point(62,44), // Volga Delta
				// Caspian
				new Point(66,45),
				new Point(70,45),
				new Point(65,44),
				new Point(66,44),
				new Point(67,44),
				new Point(68,44),
				new Point(69,44),
				new Point(70,44),
				new Point(65,43),
				new Point(66,43),
				new Point(68,43),
				new Point(69,43),
				new Point(70,43),
				new Point(65,42),
				//new Point(66,42), // should be land, but need Aral to be saline
				new Point(68,42),
				new Point(67,41),
				new Point(68,41),
				new Point(69,41),
				new Point(70,41),
				new Point(66,40),
				new Point(67,40),
				new Point(68,40),
				new Point(69,40),
				new Point(69,39),
				new Point(70,39),
				new Point(71,42),
				// Taklamakan
				new Point(74,42),
				new Point(75,42),
				new Point(76,42),
				new Point(74,41),
				new Point(75,41),
				new Point(76,41),
				new Point(75,40),
				new Point(76,40),
				new Point(77,41),
				new Point(77,42),
				// Gobi
				new Point(78,43),
				new Point(79,43),
				// Iran, Afghanistan, Indus, Thar
				new Point(66,37),
				new Point(67,37),
				new Point(68,37),
				new Point(69,37),
				new Point(70,37),
				new Point(67,36),
				new Point(68,36),
				new Point(69,36),
				new Point(70,36),
				new Point(69,35),
				new Point(70,35),
				new Point(70,34),
				new Point(70,33),
				new Point(70,32),
				new Point(70,31),
				new Point(69,30),
				new Point(74,36),
				new Point(71,35),
				new Point(74,35),
				new Point(75,35),
				new Point(71,34),
				new Point(72,34),
				new Point(73,34),
				new Point(74,34),
				new Point(75,34),
				new Point(71,33),
				new Point(72,33),
				new Point(73,33),
				new Point(74,33),
				new Point(75,33),
				new Point(71,32),
				new Point(72,32),
				new Point(73,32),
				new Point(74,32),
				new Point(75,32),
				new Point(72,31),
				// Syrian and Arabian Desert
				new Point(60,34), // Damascus
				new Point(61,35),
				new Point(63,35),
				new Point(60,34),
				new Point(61,34),
				new Point(62,34),
				new Point(63,34),
				new Point(61,33),
				new Point(64,33),
				new Point(65,33),
				new Point(61,32),
				new Point(62,32),
				new Point(63,32),
				new Point(61,31),
				new Point(62,31),
				new Point(63,31),
				new Point(64,31),
				new Point(61,30),
				new Point(62,30),
				new Point(63,30),
				new Point(64,30),
				new Point(60,29),
				new Point(61,29),
				new Point(62,29),
				new Point(63,29),
				new Point(64,29),
				new Point(65,29),
				new Point(67,29),
				new Point(60,28),
				new Point(61,28),
				new Point(62,28),
				new Point(63,28),
				new Point(64,28),
				new Point(65,28),
				new Point(66,28),
				new Point(63,27),
				new Point(64,27),
				new Point(65,27),
				new Point(66,27),
				new Point(67,27),
				new Point(63,26),
				new Point(64,26),
				new Point(65,26),
				new Point(66,26),
				new Point(67,26),
				new Point(65,25),
				new Point(66,25),
		}));
		final Set<Point2D> plains = new HashSet<Point2D>(
				Arrays.asList(new Point[] {
				// NW African Savanna
				new Point(35,30),
				new Point(36,30),
				new Point(34,29),
				new Point(35,29),
				new Point(36,29),
				new Point(37,29),
				new Point(38,29),
				new Point(39,29),
				new Point(40,29),
				new Point(34,28),
				new Point(35,28),
				new Point(36,28),
				new Point(38,28),
				new Point(39,28),
				new Point(40,28),
				new Point(41,28),
				new Point(37,27),
				new Point(39,27),
				new Point(40,27),
				new Point(41,27),
				new Point(42,27),
				new Point(43,27),
				new Point(37,26),
				new Point(38,26),
				new Point(39,26),
				new Point(40,26),
				new Point(41,26),
				new Point(42,26),
				new Point(43,26),
				new Point(44,26),
				new Point(45,26),
				new Point(46,26),
				new Point(45,25),
				new Point(46,25),
				new Point(47,25),
				new Point(48,25),
				new Point(49,25),
				new Point(44,24),
				new Point(45,24),
				new Point(46,24),
				new Point(48,24),
				new Point(49,24),
				new Point(49,23),
				// Eastern Sudanic Zone, Nilotic Plain
				new Point(50,24),
				new Point(58,24),
				new Point(50,23),
				new Point(51,23),
				new Point(52,23),
				new Point(53,23),
				new Point(54,23),
				new Point(55,23),
				new Point(56,23),
				new Point(57,23),
				new Point(61,23),
				new Point(51,22),
				new Point(52,22),
				new Point(53,22),
				new Point(55,22),
				new Point(56,22),
				new Point(56,21),
				new Point(57,21),
				new Point(60,21),
				// North Africa
				new Point(37,35),
				new Point(38,35),
				new Point(39,35),
				new Point(40,35),
				new Point(42,35),
				new Point(43,35),
				new Point(41,35),
				new Point(41,36),
				new Point(42,36),
				new Point(43,34),
				new Point(46,32),
				new Point(50,31), // Cyrene
				new Point(55,31), // Egypt
				// Levante
				new Point(59,32),
				new Point(60,32),
				new Point(59,33),
				new Point(60,33),
				new Point(59,34),
				new Point(57,34), // Cyprus
				new Point(53,34), // Crete
				// Iraq
				new Point(63,36),
				new Point(62,35),
				new Point(62,33),
				new Point(63,33),
				//new Point(64,34), // Swamp toward Susiana; try water
				// Iran
				new Point(65,37),
				new Point(66,36),
				new Point(66,35),
				new Point(67,35),
				new Point(68,35),
				new Point(65,34),
				new Point(66,34),
				new Point(67,34),
				new Point(68,34),
				new Point(69,34),
				new Point(67,33),
				new Point(68,33),
				new Point(69,33),
				new Point(66,32),
				new Point(68,32),
				new Point(70,30),
				new Point(71,31),
				// Afghanistan
				new Point(71,36),
				new Point(72,36),
				new Point(73,36),
				new Point(73,35),
				// India
				new Point(74,37),
				new Point(75,37),
				new Point(75,36),
				new Point(76,36),
				new Point(76,35),
				new Point(76,34),
				new Point(76,33),
				new Point(76,32),
				new Point(75,31),
				new Point(84,37),
				new Point(77,36),
				new Point(81,36),
				new Point(82,36),
				new Point(77,35),
				new Point(78,35),
				new Point(79,35),
				new Point(80,35),
				new Point(83,35),
				new Point(77,34),
				new Point(78,34),
				new Point(82,34),
				new Point(77,33),
				new Point(82,33),
				new Point(77,32),
				new Point(78,32),
				new Point(79,32),
				new Point(80,32),
				new Point(82,32),
				new Point(77,31),
				new Point(78,31),
				new Point(79,31),
				new Point(81,31),
				new Point(78,30),
				new Point(79,30),
				new Point(80,30),
				new Point(81,30),
				new Point(79,29),
				new Point(80,28),
				new Point(82,28),
				new Point(81,27),
				new Point(82,27),
				// Burma
				new Point(85,36),
				new Point(86,36),
				// China
				new Point(77,43),
				new Point(80,43),
				new Point(81,43),
				new Point(82,43),
				new Point(83,43),
				new Point(84,43),
				new Point(86,43),
				new Point(80,42),
				new Point(81,42),
				new Point(86,42),
				new Point(82,41),
				new Point(83,41),
				new Point(86,41),
				new Point(81,40),
				new Point(86,40),
				new Point(84,39),
				new Point(86,39),
				new Point(86,38),
				// Arabian Peninsula
				new Point(62,27),
				new Point(62,26),
				new Point(64,25),
				new Point(64,24),
				// Hispania
				new Point(37,41),
				new Point(38,41),
				new Point(40,41),
				new Point(37,40),
				new Point(38,40),
				new Point(39,40),
				new Point(40,40),
				new Point(37,39),
				new Point(38,39),
				new Point(39,39),
				new Point(40,39),
				new Point(38,38),
				new Point(39,38),
				new Point(44,39), // Sardegna
				new Point(45,37), // Sicily
				// Italia
				new Point(46,36),
				new Point(48,36),
				new Point(47,37),
				new Point(48,38),
				new Point(47,39),
				new Point(47,40),
				new Point(45,41),
				new Point(46,42),
				// France
				new Point(41,43),
				new Point(42,43),
				new Point(43,42),
				new Point(40,45),
				new Point(42,45),
				// Germany
				new Point(44,45),
				new Point(46,46),
				new Point(46,47),
				new Point(47,47),
				// Poland
				new Point(49,47),
				new Point(51,47),
				// Hungary, Carpathians
				new Point(49,44),
				new Point(50,44),
				new Point(51,44),
				new Point(50,43),
				new Point(51,43),
				new Point(52,42),
				new Point(53,43),
				new Point(51,41),
				new Point(52,41),
				new Point(53,41),
				// Greece
				new Point(52,38),
				new Point(53,38),
				new Point(52,37),
				new Point(51,35),
				new Point(52,36),
				new Point(53,36),
				new Point(54,40),
				new Point(52,39),
				// Anatolia
				new Point(55,38),
				new Point(56,36),
				new Point(58,36),
				new Point(56,37),
				new Point(57,37),
				new Point(59,38),
				new Point(58,37),
				new Point(60,37),
				new Point(56,38),
				new Point(57,38),
				new Point(58,38),
				new Point(60,38),
				new Point(57,39),
				new Point(63,38),
				new Point(62,37),
				// Ukraine, Russia, Kazakhstan
				new Point(57,42),
				new Point(59,42),
				new Point(54,43),
				new Point(56,43),
				new Point(59,43),
				new Point(60,43),
				new Point(53,44),
				new Point(54,44),
				new Point(59,44),
				new Point(55,44),
				new Point(56,44),
				new Point(57,44),
				new Point(58,44),
				new Point(59,44),
				new Point(60,44),
				new Point(52,45),
				new Point(53,45),
				new Point(54,45),
				new Point(55,45),
				new Point(56,45),
				new Point(57,45),
				new Point(58,45),
				new Point(59,45),
				new Point(60,45),
				new Point(54,46),
				new Point(55,46),
				new Point(56,46),
				new Point(57,46),
				new Point(58,46),
				new Point(59,46),
				new Point(60,46),
				new Point(55,47),
				new Point(56,47),
				new Point(57,47),
				new Point(58,47),
				new Point(59,47),
				new Point(60,47),
				new Point(56,48),
				new Point(57,48),
				new Point(59,48),
				new Point(60,48),
				new Point(69,52),
				new Point(70,52),
				new Point(70,51),
				new Point(70,50),
				new Point(64,49),
				new Point(65,49),
				new Point(70,49),
				new Point(61,48),
				new Point(62,48),
				new Point(64,48),
				new Point(65,48),
				new Point(67,48),
				new Point(68,48),
				new Point(69,48),
				new Point(70,48),
				new Point(61,47),
				new Point(62,47),
				new Point(63,47),
				new Point(64,47),
				new Point(65,47),
				new Point(66,47),
				new Point(67,47),
				new Point(68,47),
				new Point(69,47),
				new Point(70,47),
				new Point(61,46),
				new Point(62,46),
				new Point(63,46),
				new Point(64,46),
				new Point(65,46),
				new Point(66,46),
				new Point(67,46),
				new Point(68,46),
				new Point(69,46),
				new Point(70,46),
				new Point(62,45),
				new Point(63,45),
				new Point(64,45),
				new Point(65,45),
				new Point(67,45),
				new Point(68,45),
				new Point(69,45),
				new Point(63,44),
				new Point(64,44),
				// Caucasus
				new Point(61,42),
				new Point(61,41),
				new Point(61,40),
				new Point(62,40),
				new Point(63,40),
				new Point(61,39),
				new Point(62,39),
				new Point(63,39),
				// Turkmenistan, Uzbekistan
				new Point(69,42),
				new Point(70,42),
				new Point(70,40),
				new Point(67,39),
				new Point(68,39),
				new Point(69,38),
				new Point(70,38),
				new Point(71,43),
				// Sweden
				new Point(47,52),
				new Point(48,51),
				new Point(49,52),
				new Point(51,52), // Finland
				new Point(46,50), // Denmark
				new Point(52,50), // Balticum
				new Point(41,50), // England
		}));
		Set<Point2D> grass = new HashSet<Point2D>(
				Arrays.asList(new Point[] {
				new Point(38,36), // Morocco
				new Point(50,32), // Cyrene
				new Point(42,38), // Baleares
				new Point(34,35), // Canaries
				// Western Sudanic Zone
				new Point(35,27),
				new Point(36,27),
				new Point(38,27),
				new Point(43,25),
				new Point(44,25),
				new Point(47,24),
				new Point(37,28), // Inner Niger Delta
				// Ethiopia
				new Point(59,24),
				new Point(58,23),
				new Point(57,22),
				new Point(58,22),
				new Point(59,22),
				new Point(60,22),
				new Point(58,21),
				// Hispania
				new Point(36,40),
				new Point(36,41),
				new Point(36,42),
				new Point(37,42),
				new Point(38,42),
				new Point(39,42),
				new Point(40,42),
				new Point(41,42),
				new Point(39,41),
				new Point(41,41),
				new Point(43,40), // Corse
				new Point(44,37), // Sicily
				// Italia
				new Point(45,43),
				new Point(46,43),
				new Point(47,43),
				new Point(48,43),
				new Point(44,42),
				new Point(45,42),
				new Point(46,41),
				new Point(47,41),
				new Point(46,40),
				new Point(46,39),
				new Point(47,38),
				new Point(48,37),
				new Point(54,34), // Crete
				// S Greece
				new Point(51,36),
				new Point(54,34),
				// Levante
				new Point(60,35),
				new Point(60,36),
				new Point(61,36),
				// Iraq
				new Point(62,36),
				new Point(63,37),
				// Iran
				new Point(66,39),
				new Point(65,38),
				new Point(67,38),
				new Point(68,38),
				new Point(64,37),
				new Point(64,36),
				new Point(65,36),
				new Point(64,35),
				new Point(65,35),
				new Point(67,32),
				new Point(69,32),
				new Point(68,31),
				// Around Hindu Kush - Himalaya
				new Point(72,43),
				new Point(76,43),
				new Point(72,42),
				new Point(71,41),
				new Point(72,41),
				new Point(73,41),
				new Point(71,40),
				new Point(72,40),
				new Point(71,39),
				new Point(71,38),
				new Point(71,37),
				new Point(72,37),
				new Point(73,37),
				new Point(73,38),
				new Point(74,38),
				new Point(76,37),
				new Point(77,37),
				new Point(78,36),
				new Point(79,36),
				new Point(80,36),
				new Point(81,37),
				new Point(82,37),
				new Point(83,38),
				new Point(82,39),
				new Point(81,39),
				// India (rest)
				new Point(83,37),
				new Point(84,38),
				new Point(83,36),
				new Point(81,35),
				new Point(82,35),
				new Point(79,34),
				new Point(80,34),
				new Point(81,34),
				new Point(83,34),
				new Point(78,33),
				new Point(79,33),
				new Point(80,33),
				new Point(81,33),
				new Point(81,32),
				new Point(80,31),
				new Point(80,29),
				new Point(81,29),
				new Point(82,29),
				new Point(81,28),
				// Sri Lanka
				new Point(85,27),
				new Point(85,26),
				// Burma
				new Point(85,38),
				new Point(85,37),
				new Point(86,37),
				// W China
				new Point(85,39),
				new Point(85,40),
				new Point(85,41),
				new Point(85,42),
				new Point(85,43),
				new Point(84,41),
				new Point(84,42),
				new Point(82,42),
				// Arabian Peninsula
				new Point(61,27),
				new Point(63,25),
				new Point(63,24),
				// Turkey, Caucasus
				new Point(55,39),
				new Point(55,37),
				new Point(56,39),
				new Point(57,40),
				new Point(58,39),
				new Point(59,39),
				new Point(59,37),
				new Point(60,40),
				new Point(60,42),
				new Point(61,37),
				new Point(61,38),
				new Point(62,38),
				// Balkan (w/o S Greece)
				new Point(49,43),
				new Point(52,44),
				new Point(52,43),
				new Point(53,42),
				new Point(49,42),
				new Point(50,42),
				new Point(51,42),
				new Point(49,41),
				new Point(50,41),
				new Point(50,40),
				new Point(51,40),
				new Point(52,40),
				new Point(50,39),
				new Point(51,39),
				new Point(51,38),
				new Point(53,40),
				// France to Königsberg
				new Point(45,50),
				new Point(45,49),
				new Point(43,48),
				new Point(44,48),
				new Point(45,48),
				new Point(46,48),
				new Point(47,48),
				new Point(49,48),
				new Point(50,48),
				new Point(39,47),
				new Point(41,47),
				new Point(42,47),
				new Point(43,47),
				new Point(44,47),
				new Point(45,47),
				new Point(48,47),
				new Point(50,47),
				new Point(38,46),
				new Point(39,46),
				new Point(40,46),
				new Point(41,46),
				new Point(42,46),
				new Point(43,46),
				new Point(44,46),
				new Point(45,46),
				new Point(47,46),
				new Point(48,46),
				new Point(49,46),
				new Point(50,46),
				new Point(39,45),
				new Point(41,45),
				new Point(43,45),
				new Point(45,45),
				new Point(46,45),
				new Point(47,45),
				new Point(48,45),
				new Point(49,45),
				new Point(50,45),
				new Point(40,44),
				new Point(41,44),
				new Point(42,44),
				new Point(43,44),
				new Point(44,44),
				new Point(47,44),
				new Point(48,44),
				new Point(40,43),
				new Point(43,43),
				// Ireland
				new Point(36,53),
				new Point(37,53),
				new Point(36,52),
				new Point(37,52),
				// Great Britain
				new Point(38,49),
				new Point(39,49),
				new Point(40,49),
				new Point(38,50),
				new Point(39,50),
				new Point(40,50),
				new Point(39,51),
				new Point(40,51),
				new Point(39,52),
				new Point(40,52),
				new Point(40,53),
				// Norway, Sweden
				new Point(44,54),
				new Point(46,54),
				new Point(44,53),
				new Point(45,53),
				new Point(46,53),
				new Point(48,53),
				new Point(44,52),
				new Point(45,52),
				new Point(47,51),
				new Point(48,50),
				// Finland
				new Point(51,53),
				new Point(53,53),
				new Point(52,52),
				// Balticum, Belarus, Russia
				new Point(51,50),
				new Point(51,49),
				new Point(51,48),
				new Point(51,46),
				new Point(51,45),
				new Point(52,49),
				new Point(52,48),
				new Point(52,47),
				new Point(52,46),
				new Point(53,51),
				new Point(53,50),
				new Point(53,49),
				new Point(53,48),
				new Point(53,46),
				new Point(54,51),
				new Point(54,50),
				new Point(54,49),
				new Point(54,48),
				new Point(54,47),
				new Point(55,51),
				new Point(55,49),
				new Point(55,48),
				new Point(56,53),
				new Point(56,50),
				new Point(56,49),
				new Point(57,50),
				new Point(57,49),
				new Point(58,48),
				new Point(58,49),
				new Point(59,49),
				new Point(60,49),
				new Point(61,49),
				// Urals
				new Point(62,49),
				new Point(63,49),
				new Point(63,48),
				new Point(61,45),
				// Lower Volga
				new Point(61,44),
				new Point(61,43),
				new Point(62,43),
		}));
		/* Dummy continents to reduce Barb city spawn on the British
		 * Isles. Shouldn't be needed anymore once America is
		 * on the map. */
		for(int x = 10; x < 25; x++)
			for(int y = 35; y <= 50; y++)
				if(y != 42 && x != 23)
					grass.add(new Point(x,y));
		final Set<Point2D> peak = new HashSet<Point2D>(
				Arrays.asList(new Point[] {
				new Point(60,39), // Ararat
				new Point(60,41), // Elbrus
				new Point(62,41), // East Caucasus
				// Elburz
				new Point(64,38),
				new Point(66,38),
				// Zagros
				new Point(66,33), // Dena
				new Point(69,31), // Hazar, Palvar
				new Point(72,35), // Central Afghanistan
				// Tian Shan
				new Point(73,42),
				new Point(74,43),
				new Point(75,43),
				// Hindu Kush - Himalaya - Kunlun
				new Point(73,40),
				new Point(74,40),
				new Point(72,39),
				new Point(73,39),
				new Point(74,39),
				new Point(76,39),
				new Point(72,38),
				new Point(75,38),
				new Point(76,38),
				new Point(78,42),
				new Point(79,42),
				new Point(83,42), // Hengduan Mts.
				new Point(78,41),
				new Point(77,40),
				new Point(78,40),
				new Point(80,40),
				new Point(82,40),
				new Point(83,40),
				new Point(84,40),
				new Point(77,39),
				new Point(83,39),
				new Point(77,38),
				new Point(78,38),
				new Point(81,38),
				new Point(82,38),
				new Point(78,37),
				new Point(79,37),
				new Point(80,37),
				//new Point(75,39), // Karakorum pass
				// Alps
				new Point(44,43),
				new Point(45,44),
				new Point(46,44),
				// Skandinavia
				new Point(45,54),
				new Point(47,55),
				new Point(48,56),
				new Point(49,57),
				new Point(37,34), // Atlas
				// Ethiopia
				new Point(59,23), // Abuna Yosef
				new Point(59,21), // Bale Mts.
		}));
		final Set<Point2D> tundra = new HashSet<Point2D>(
				Arrays.asList(new Point[] {
				new Point(53,47), // Belarus
				new Point(55,50), // Volga
				new Point(58,54), // Novaya Zemlya
				new Point(37,57), // Iceland
				// Scotland
				new Point(39,53),
				new Point(39,54),
				new Point(40,54),
				// Skandinavia, Russian taiga
				new Point(49,55),
				new Point(50,55),
				new Point(49,54),
				new Point(48,54),
				new Point(47,54),
				new Point(48,55),
				new Point(46,55),
				new Point(50,57),
				new Point(51,57),
				new Point(49,56),
				new Point(50,56),
				new Point(52,56),
				new Point(53,56),
				new Point(51,55),
				new Point(52,55),
				new Point(53,55),
				new Point(54,55),
				new Point(52,54),
				new Point(53,54),
				new Point(54,53),
				new Point(55,52),
				new Point(56,52),
				new Point(57,52),
				new Point(54,52), // had this as grass at first
				new Point(56,51), // had this as grass at first
				new Point(57,51), // had this as grass at first
				new Point(58,52),
				new Point(58,51),
				new Point(59,51),
				new Point(60,51),
				new Point(58,50),
				new Point(59,50),
				new Point(60,50),
				new Point(66,54),
				new Point(69,54),
				new Point(70,54),
				new Point(64,53),
				new Point(65,53),
				new Point(66,53),
				new Point(67,53),
				new Point(68,53),
				new Point(69,53),
				new Point(70,53),
				new Point(63,52),
				new Point(64,52),
				new Point(65,52),
				new Point(66,52),
				new Point(67,52),
				new Point(68,52),
				new Point(63,51),
				new Point(64,51),
				new Point(65,51),
				new Point(66,51),
				new Point(67,51),
				new Point(68,51),
				new Point(69,51),
				new Point(61,50),
				new Point(62,50),
				new Point(63,50),
				new Point(64,50),
				new Point(65,50),
				new Point(66,50),
				new Point(67,50),
				new Point(68,50),
				new Point(69,50),
				new Point(66,49),
				new Point(67,49),
				new Point(68,49),
				new Point(69,49),
				new Point(66,48),
				// African rainforest
				new Point(36,26),
				new Point(36,25),
				new Point(37,25),
				new Point(38,25),
				new Point(41,25),
				new Point(42,25),
				new Point(42,24),
				new Point(43,24),
				new Point(43,23),
				new Point(44,23),
				new Point(45,23),
				new Point(46,23),
				new Point(47,23),
				new Point(48,23),
				new Point(43,22),
				new Point(44,22),
				new Point(45,22),
				new Point(46,22),
				new Point(47,22),
				new Point(48,22),
				new Point(49,22),
				new Point(50,22),
				new Point(44,21),
				new Point(45,21),
				new Point(46,21),
				new Point(47,21),
				new Point(48,21),
				new Point(49,21),
				new Point(50,21),
				new Point(51,21),
				new Point(52,21),
				new Point(53,21),
				new Point(54,21),
				new Point(55,21),
				//new Point(54,22), // Sudd - try a lake instead
				// Tibetan Plateau
				new Point(79,41),
				new Point(80,41),
				new Point(81,41),
				new Point(84,27), // Sri Lanka
		}));
		final Set<Point2D> snow = new HashSet<Point2D>(
				Arrays.asList(new Point[] {
				new Point(38,57), // Iceland
				// Spitsbergen
				new Point(52,61),
				new Point(53,62),
				// Siberia
				new Point(59,52),
				new Point(60,52),
				new Point(60,53),
				new Point(65,55),
				new Point(66,55),
				new Point(69,55),
				new Point(70,55),
				new Point(63,54),
				new Point(64,54),
				new Point(65,54),
				new Point(67,54),
				new Point(68,54),
				new Point(62,53),
				new Point(63,53),
				new Point(62,52),
				new Point(61,51),
				new Point(62,51),
				new Point(57,59), // Heiss Island
				new Point(59,55), // Novaya Zemlya
				new Point(63,57), // Severnaya Zemlya
				new Point(67,57), // Lyakhovsky Islands
				// Should perhaps be a peak to prevent a city (Leh) here
				new Point(75,39),
				// Changtang
				new Point(79,40),
				new Point(78,39),
				new Point(79,39),
				new Point(80,39),
				new Point(79,38),
				new Point(80,38),
		}));
		Set<Point2D> land = new HashSet<Point2D>();
		addSafe(land, desert);
		addSafe(land, plains);
		addSafe(land, grass);
		addSafe(land, peak);
		addSafe(land, tundra);
		addSafe(land, snow);
		for(Point2D p : land)
			p.setLocation(p.getX() + offsetX, p.getY() + offsetY);
		final Set<Point2D> seaObstacles = new HashSet<Point2D>(
				Arrays.asList(new Point[] {
				new Point(50,58), // North Cape
				// Equatorial Counter
				new Point(35,25),
				new Point(35,24),
				new Point(36,24),
		}));
		for(Point2D p : seaObstacles)
			p.setLocation(p.getX() + offsetX, p.getY() + offsetY);
		final Set<River> rivers = new HashSet<River>(
				Arrays.asList(new River[] {
				new River(S,42,36,E), // Tunisia
				// Niger
				new River(S,36,28,E),
				new River(E,36,28,N),
				new River(S,37,29,E),
				new River(S,38,29,E),
				new River(E,38,29,N),
				new River(S,39,30,E),
				new River(E,39,29,S),
				new River(E,39,28,S),
				new River(S,40,28,E),
				new River(E,40,27,S),
				new River(S,41,27,E),
				new River(E,41,26,S),
				new River(E,41,25,S),
				// Benue
				new River(S,42,26,W),
				new River(S,43,26,W),
				// Chad
				new River(S,43,27,E),
				new River(S,45,26,W),
				new River(E,44,26,N),
				// W Africa (rest)
				new River(S,34,30,W), // Senegal/Gambia
				new River(E,36,25,S), // Sassandra/Bandama
				new River(E,38,26,S), // Volta
				// Ogodue
				new River(S,43,23,W),
				new River(S,44,23,W),
				// Kongo
				new River(E,47,21,S),
				new River(E,47,22,S),
				new River(E,47,23,S), // Ubangi
				new River(E,46,22,S), // Sangha
				new River(S,47,22,E), // Sangha
				new River(S,48,23,W),
				new River(S,49,23,W),
				new River(E,49,22,N),
				new River(S,50,22,W),
				new River(E,50,21,N),
				//new River(E,49,22,N), // Tbc.
				// Nile
				new River(E,55,31,N),
				new River(E,56,31,N),
				new River(S,56,31,W),
				new River(E,56,30,N),
				new River(E,56,29,N),
				new River(S,57,29,W),
				new River(E,57,28,N),
				new River(S,57,28,E),
				new River(S,56,28,E),
				new River(E,55,27,N),
				new River(S,56,26,W),
				new River(E,55,26,N),
				new River(E,56,25,N),
				new River(S,56,25,E),
				new River(E,55,24,N), // Blue+White
				// Blue Nile
				new River(S,56,24,W),
				new River(E,56,23,N),
				new River(S,57,23,W),
				// White Nile
				new River(E,55,23,N),
				new River(S,55,23,E),
				new River(S,55,22,W),
				new River(E,55,21,N),
				//new River(E,54,22,N),
				// Bahr al-Ghazal [replaced by a Sudd lake]
				//new River(S,54,23,E),
				//new River(E,53,22,N),
				// Orontes
				//new River(E,60,36,N), // Upper/ middle course superfluous
				new River(S,60,37,W),
				new River(E,59,33,S), // Jordan
				new River(E,44,37,N), // Sicily
				// Hispania
				new River(S,36,42,W),
				new River(E,36,40,S),
				new River(E,37,39,S),
				new River(S,38,40,W),
				new River(S,40,41,E),
				// Italia
				new River(S,44,43,E),
				new River(S,45,43,E),
				new River(S,46,43,E),
				new River(E,46,43,S),
				new River(S,46,41,W),
				// Turkey
				new River(E,58,39,N),
				new River(S,58,39,E),
				new River(E,57,38,N),
				new River(S,55,38,W), // Meander
				// N Greece/ Albania
				new River(S,50,40,W), // Neretva
				new River(S,50,41,W), // Drin/Seman
				new River(S,52,40,E), // Haliacmon, Vardar
				// Danube + tributaries
				new River(S,46,45,E),
				new River(S,47,45,E),
				new River(S,48,45,E),
				new River(S,49,45,E),
				new River(E,49,44,S),
				new River(E,50,44,S),
				//new River(S,49,44,E), // Drava - superfluous
				new River(E,49,43,S),
				new River(S,50,43,E),
				new River(E,50,43,S),
				new River(E,50,42,S),
				new River(S,51,42,E),
				//new River(E,51,42,S), // Olt - superfluous
				new River(E,52,42,N),
				new River(S,52,42,E),
				//new River(E,52,41,N), // Iskar - superfluous
				new River(S,53,43,E),
				//new River(E,50,41,N), // Great Morava - superfluous
				//new River(S,50,42,E), // Sava - superfluous
				// France
				//new River(E,42,44,S), // Upper/Middle Loire - superfluous 
				new River(E,42,43,S),
				new River(E,41,44,N),
				new River(S,41,45,W),
				new River(S,40,45,W),
				//new River(S,41,46,W), // Middle Seine - superfluous
				new River(E,40,46,N),
				// new River(E,42,47,N), // Meuse - superfluous
				// Rhine + tributaries
				new River(S,43,48,W),
				new River(E,43,47,N),
				new River(E,43,46,N),
				//new River(E,43,45,N), // Mosel - superfluous
				new River(S,44,46,W),
				//new River(S,45,46,W), // Main - superfluous
				new River(E,44,45,N),
				// Elbe
				new River(E,44,48,N),
				new River(S,45,48,W),
				new River(E,45,47,N),
				// Poland
				new River(S,48,47,W),
				new River(E,47,47,N),
				//new River(E,50,47,N), // Vistula tributaries - superfluous
				new River(S,50,48,W),
				new River(E,49,48,N),
				// Balticum
				new River(S,51,49,W),
				new River(E,51,48,N),
				new River(S,52,48,W),
				new River(S,51,50,W),
				new River(E,52,50,N),
				// Ukraine
				new River(S,54,44,E),
				new River(E,53,44,S),
				new River(S,53,45,E),
				new River(E,54,47,S),
				new River(E,54,46,S),
				new River(S,55,46,E),
				new River(S,56,46,E),
				new River(E,56,45,S),
				new River(S,56,45,W),
				new River(E,55,44,S),
				// Don
				new River(E,58,45,S),
				new River(S,59,45,E),
				new River(E,59,44,S),
				new River(S,59,44,W),
				new River(S,59,43,W), // Kama
				// Russia
				new River(S,53,51,W),
				new River(E,53,50,N),
				new River(S,58,43,W), // Kuban
				new River(S,54,50,W),
				new River(E,55,52,N),
				new River(E,55,51,N),
				new River(E,58,50,N),
				new River(E,58,51,N),
				new River(S,58,52,W),
				new River(E,57,52,N),
				new River(S,59,52,W),
				// Volga
				new River(E,55,49,S),
				new River(S,56,49,E),
				new River(S,57,49,E),
				new River(S,58,49,E),
				new River(S,59,49,E),
				new River(S,60,49,E),
				new River(E,60,48,S),
				new River(E,60,47,S),
				new River(S,61,47,E),
				new River(E,61,46,S),
				new River(E,61,45,S),
				new River(S,62,45,E),
				new River(E,62,44,S),
				new River(S,61,49,W),
				new River(S,62,49,W),
				new River(E,62,49,S),
				new River(E,62,48,N),
				// Oka
				new River(S,57,48,E),
				new River(S,58,48,E),
				new River(E,58,48,N),
				// Siberia
				new River(E,69,55,N),
				new River(S,69,55,W),
				new River(S,70,55,W),
				new River(S,65,52,W),
				new River(S,66,52,W),
				new River(S,67,52,W),
				new River(S,68,52,W),
				new River(S,69,52,W),
				new River(S,70,52,W),
				new River(E,64,51,S),
				new River(E,69,51,N),
				new River(E,61,51,N),
				new River(S,62,51,W),
				new River(S,63,51,W),
				new River(S,64,51,W),
				new River(E,64,50,N),
				new River(E,69,50,N),
				new River(S,65,50,W),
				new River(S,66,50,W),
				new River(S,70,50,W),
				new River(E,65,49,N),
				new River(E,66,49,N),
				new River(S,64,49,E),
				new River(S,65,49,E),
				new River(S,67,49,W),
				new River(S,68,49,W),
				new River(S,69,49,W),
				new River(E,65,48,N),
				new River(E,67,48,N),
				new River(E,69,48,N),
				new River(E,67,47,N),
				new River(E,69,47,N),
				new River(S,68,47,W),
				new River(S,70,47,W),
				// Uralsk
				new River(E,63,46,S),
				new River(E,63,45,S),
				new River(E,63,44,S),
				new River(E,45,53,S), // Norway
				// Sweden
				new River(E,46,54,S),
				new River(E,48,54,S),
				// Great Britain
				new River(S,40,50,E),
				new River(S,39,50,E),
				//new River(S,38,50,W), // No room for Severn
				//new River(S,40,51,E), // Great Ouse - superfluous
				new River(S,40,52,E),
				new River(S,39,53,W),
				new River(S,36,53,W), // Ireland
				// West Caspian
				new River(S,61,43,E),
				new River(S,61,42,E),
				new River(E,61,40,S),
				new River(S,62,40,E),
				new River(S,63,40,E),
				// East Caspian
				new River(S,66,39,W),
				new River(S,67,41,W),
				new River(S,68,41,W),
				new River(S,69,41,W),
				new River(S,70,40,W),
				new River(E,69,40,N),
				new River(E,67,41,S),
				new River(S,68,43,W),
				new River(S,69,43,W),
				new River(S,70,43,W),
				new River(E,70,42,N), // Upper Syr
				new River(S,71,42,W),
				new River(S,72,42,W),
				new River(E,70,39,N), // Upper Amu/ Herat
				new River(S,71,39,W),
				// Helmand/ Arghandab
				new River(E,71,33,S),
				new River(S,71,33,W),
				// Sutlej
				new River(S,76,37,W),
				new River(S,75,37,W),
				// Indus
				new River(E,74,37,S),
				new River(E,74,36,S),
				new River(E,74,35,S),
				new River(S,74,35,W),
				new River(E,73,34,S),
				new River(E,73,33,S),
				new River(S,74,33,E),
				new River(E,74,32,S),
				// Mesopotamia
				new River(E,61,37,S),
				new River(E,61,36,S),
				new River(E,61,35,S),
				new River(S,62,35,E),
				new River(E,62,34,S),
				new River(S,63,34,E),
				new River(E,63,33,S),
				new River(S,63,37,W), // Great Zab
				new River(E,62,36,S),
				new River(S,63,36,E),
				new River(E,63,35,S),
				new River(E,63,34,S),
				// S Iran
				new River(E,64,33,S), // Karun
				new River(S,65,35,E), // Zayandé
				new River(S,67,33,E), // Kor
				// Yangtze
				new River(S,83,44,E), // Jialing
				new River(S,84,44,E), // Jialing
				new River(E,84,43,N),
				// Mekong
				new River(S,86,40,E), // Lancang
				new River(E,86,39,S), // Lancang
				// Irawaddy
				new River(E,85,39,S),
				new River(E,85,38,S),
				new River(E,85,37,S),
				new River(E,84,38,S), // Chindwin
				new River(S,85,38,E), // Chindwin
				new River(S,86,37,E),
				new River(E,86,36,S),
				// Brahmaputra (Jamuna)
				new River(E,83,38,S),
				new River(E,83,37,S),
				// Ganges
				new River(E,77,36,S), // Ganges/Yamuna
				new River(E,77,35,N), // Chamba/Betwa
				new River(E,77,34,N), // Chamba/Betwa
				new River(S,78,36,E),
				new River(S,79,36,E),
				new River(S,80,36,E),
				new River(E,80,36,N),
				new River(S,81,37,E),
				new River(S,82,37,E),
				new River(S,83,37,E),
				// Mahanadi
				new River(S,81,35,E),
				new River(S,82,35,E),
				new River(S,83,35,E),
				// Godavari
				new River(S,79,32,E),
				new River(S,80,32,E),
				new River(S,81,32,E),
				// Krishna
				new River(S,80,31,E),
				new River(S,81,31,E),
				new River(S,82,29,E), // Kaveri
				// Narmada, Tapti
				new River(S,78,33,W),
				new River(E,77,32,S),
				new River(S,77,32,W),
		}));
		final Set<Point2D> hills = new HashSet<Point2D>(
				Arrays.asList(new Point[] {
				// African rainforest
				new Point(42,24),
				new Point(43,23),
				new Point(44,22),
				new Point(44,21),
				new Point(45,21),
				new Point(53,21), // Mitumba Mts.
				// Arabian Peninsula
				new Point(61,30),
				new Point(60,29),
				new Point(61,29),
				new Point(60,28),
				new Point(61,28),
				new Point(66,28),
				new Point(61,27),
				new Point(62,27),
				new Point(63,27),
				new Point(67,27),
				new Point(62,26),
				new Point(63,26),
				new Point(63,25),
				new Point(65,25),
				new Point(63,24),
				new Point(64,24),
				// Morocco
				new Point(36,33),
				new Point(37,33),
				new Point(36,34),
				new Point(38,34),
				new Point(38,35),
				new Point(39,35),
				new Point(50,32), // Cyrene
				new Point(42,38), // Baleares
				new Point(34,35), // Canaries
				// SW Sahara
				new Point(35,31),
				new Point(35,30),
				new Point(39,30),
				// Western Sudanic Zone
				new Point(35,27),
				new Point(36,27),
				new Point(38,27),
				new Point(43,25),
				new Point(44,25),
				new Point(47,24),
				new Point(35,28),
				new Point(49,25),
				// Inner Sahara
				new Point(41,31),
				new Point(42,31),
				new Point(41,30),
				new Point(42,30),
				new Point(43,30),
				new Point(42,29),
				new Point(43,28),
				new Point(47,29),
				new Point(48,28),
				new Point(53,28),
				new Point(52,27),
				new Point(52,25),
				new Point(55,23), // Nuba Mts.
				// Ethiopia
				new Point(59,24),
				new Point(58,23),
				new Point(57,22),
				new Point(58,22),
				new Point(59,22),
				new Point(60,22),
				new Point(57,21),
				new Point(58,21),
				new Point(60,21),
				// Somalia
				new Point(61,22),
				new Point(62,22),
				new Point(63,22),
				new Point(64,22),
				new Point(65,22),
				new Point(61,21),
				// Nile
				new Point(56,30),
				new Point(57,30),
				new Point(58,29),
				new Point(58,28),
				new Point(56,26),
				new Point(58,27),
				new Point(58,26),
				new Point(56,25),
				new Point(59,25),
				// Sinai
				new Point(59,31),
				new Point(60,31),
				new Point(58,30),
				// Levante
				new Point(60,32),
				new Point(60,35),
				new Point(61,36),
				new Point(58,34), // Cyprus
				// Anatolia
				new Point(55,37),
				new Point(56,37),
				new Point(56,39),
				new Point(56,38),
				new Point(56,36),
				new Point(57,40),
				new Point(57,39),
				new Point(57,37),
				new Point(59,38),
				new Point(58,39),
				new Point(58,36),
				new Point(59,39),
				new Point(59,38),
				new Point(60,38),
				new Point(60,37),
				new Point(61,38),
				new Point(62,38),
				new Point(63,38),
				new Point(61,37),
				new Point(63,37),
				// Crete
				new Point(53,34),
				new Point(54,34),
				// Greece
				new Point(50,39),
				new Point(51,39),
				new Point(51,38),
				new Point(52,38),
				new Point(53,40),
				new Point(51,36),
				new Point(52,37),
				new Point(51,35),
				new Point(52,36),
				new Point(53,36),
				new Point(54,34),
				new Point(43,40), // Corse
				new Point(45,37), // Sicily
				// Italia
				new Point(44,42),
				new Point(45,42),
				new Point(46,43),
				new Point(47,41),
				new Point(47,40),
				new Point(46,39),
				new Point(47,38),
				new Point(47,37),
				new Point(46,36),
				// Hispania
				new Point(37,42),
				new Point(38,42),
				new Point(40,42),
				new Point(41,42),
				new Point(37,41),
				new Point(39,41),
				new Point(41,41),
				new Point(37,40),
				new Point(38,40),
				new Point(40,40),
				new Point(37,39),
				new Point(40,39),
				new Point(39,38),
				// France
				new Point(42,44),
				new Point(42,43),
				new Point(43,43),
				// Rhine, Elbe
				new Point(44,47),
				new Point(43,46),
				new Point(44,46),
				new Point(43,45),
				new Point(44,44),
				new Point(45,47),
				// Poland, Czechoslovakia, Austria, Slovenia
				new Point(47,46),
				new Point(48,46),
				new Point(49,46),
				new Point(46,45),
				new Point(47,48),
				new Point(49,48),
				new Point(47,44),
				new Point(48,44),
				new Point(49,43),
				new Point(49,45),
				new Point(50,45),
				new Point(51,45),
				// North Balkans
				new Point(52,44),
				new Point(52,43),
				new Point(49,43),
				new Point(49,42),
				new Point(49,41),
				new Point(51,42),
				new Point(52,42),
				new Point(50,41),
				new Point(50,40),
				new Point(51,40),
				new Point(52,40),
				new Point(57,42), // Crimea
				// Caucasus
				new Point(59,42),
				new Point(60,42),
				new Point(61,41),
				new Point(61,40),
				new Point(63,40),
				new Point(61,39),
				new Point(63,39),
				// N Russia
				new Point(56,53),
				new Point(57,51),
				new Point(58,54),
				new Point(59,55),
				// Urals
				new Point(59,52),
				new Point(60,51),
				new Point(61,50),
				new Point(62,49),
				new Point(63,49),
				new Point(63,48),
				new Point(64,48),
				new Point(63,47),
				new Point(64,47),
				new Point(64,46),
				new Point(65,46),
				new Point(65,45),
				new Point(66,44),
				// Volga Uplands
				new Point(60,46),
				new Point(60,47),
				new Point(61,45),
				// east of Caspian
				new Point(68,43),
				new Point(69,43),
				new Point(70,43),
				new Point(71,43),
				new Point(72,42),
				new Point(71,41),
				new Point(73,41),
				new Point(71,40),
				new Point(72,40),
				new Point(71,39),
				new Point(76,43),
				new Point(70,40),
				new Point(65,42), // Ustyurt Plateau
				//new Point(66,42), // should be land, but need Aral to be saline
				// Taklamakan
				new Point(74,42),
				new Point(75,40),
				// W China, Burma
				new Point(75,39), // Karakorum pass
				new Point(80,43),
				new Point(81,43),
				new Point(82,43),
				new Point(85,43),
				new Point(80,42),
				new Point(81,42),
				new Point(82,42),
				new Point(84,42),
				new Point(85,42),
				new Point(82,41),
				new Point(83,41),
				new Point(84,41),
				new Point(81,40),
				new Point(85,40),
				new Point(86,40),
				new Point(78,39),
				new Point(80,39),
				new Point(81,39),
				new Point(82,39),
				new Point(84,39),
				new Point(85,39),
				new Point(86,39),
				new Point(79,38),
				new Point(80,38),
				new Point(83,38),
				new Point(84,38),
				new Point(85,38),
				new Point(86,38),
				new Point(85,37),
				new Point(85,36),
				new Point(86,36),
				// Afghanistan, SE Iran, Bugyals
				new Point(71,38),
				new Point(73,38),
				new Point(74,38),
				new Point(71,37),
				new Point(72,37),
				new Point(73,37),
				new Point(76,37),
				new Point(71,36),
				new Point(72,36),
				new Point(73,36),
				new Point(71,35),
				new Point(73,35),
				new Point(71,34),
				new Point(72,34),
				new Point(76,34),
				new Point(72,33),
				new Point(73,33),
				new Point(76,33),
				new Point(71,32),
				new Point(72,32),
				new Point(73,32),
				new Point(71,31),
				new Point(72,31),
				new Point(77,37),
				new Point(79,36),
				// India (rest)
				new Point(81,37),
				new Point(82,37),
				new Point(81,36),
				new Point(81,35),
				new Point(82,35),
				new Point(79,34),
				new Point(80,34),
				new Point(82,34),
				new Point(78,33),
				new Point(79,33),
				new Point(81,33),
				new Point(82,33),
				new Point(78,31),
				new Point(78,30),
				new Point(81,30),
				new Point(79,29),
				new Point(80,29),
				new Point(80,28),
				new Point(81,28),
				new Point(81,27),
				new Point(85,26), // Sri Lanka
				// Kola
				new Point(52,56),
				new Point(53,55),
				new Point(54,55),
				// Norway
				new Point(51,57),
				new Point(50,57),
				new Point(49,56),
				new Point(46,55),
				new Point(48,55),
				new Point(44,54),
				new Point(46,54),
				new Point(47,54),
				new Point(48,54),
				new Point(44,53),
				new Point(46,53),
				new Point(44,52),
				new Point(45,52),
				new Point(52,61), // Spitsbergen
				new Point(36,53), // Ireland
				// Great Britain
				new Point(39,49),
				new Point(38,50),
				new Point(39,52),
				new Point(39,51),
				new Point(39,53),
				new Point(39,54),
				new Point(40,53),
				new Point(40,54),
				// Iceland
				new Point(37,57),
				new Point(38,57),
				// Iran
				new Point(64,37),
				new Point(65,37),
				new Point(70,37),
				new Point(64,36),
				new Point(65,36),
				new Point(66,36),
				new Point(69,36),
				new Point(70,36),
				new Point(64,35),
				new Point(65,35),
				new Point(66,35),
				new Point(67,35),
				new Point(68,35),
				new Point(70,35),
				new Point(66,34),
				new Point(67,34),
				new Point(68,34),
				new Point(69,34),
				new Point(68,33),
				new Point(69,33),
				new Point(66,32),
				new Point(67,32),
				new Point(69,32),
				new Point(68,31),
				new Point(69,30),
				new Point(70,30),
				// Kopet Dag
				new Point(67,38),
				new Point(68,38),
				new Point(69,38),
				// Sindhar
				new Point(62,36),
				new Point(63,35),
		}));
		final Set<Point2D> forests = new HashSet<Point2D>(
				Arrays.asList(new Point[] {
				// North Africa
				new Point(38,36),
				new Point(37,35),
				new Point(38,35),
				new Point(40,35),
				new Point(41,36),
				new Point(42,36),
				new Point(42,35),
				new Point(43,34),
				new Point(50,32), // Cyrene
				// Sudanic Zone
				new Point(35,27),
				new Point(38,27),
				new Point(40,27),
				new Point(42,27),
				new Point(49,24),
				new Point(42,26),
				new Point(43,26),
				new Point(44,25),
				new Point(45,25),
				new Point(47,25),
				// Great Britain
				new Point(39,54),
				new Point(39,53),
				new Point(40,53),
				new Point(39,51),
				new Point(40,51),
				new Point(40,50),
				new Point(38,49),
				new Point(39,49),
				new Point(40,49),
				// Ireland
				new Point(36,53),
				new Point(37,53),
				new Point(57,34), // Cyprus
				// Phoenicia
				new Point(59,34),
				new Point(60,35),
				// Anatolia
				new Point(56,39),
				new Point(57,39),
				new Point(58,39),
				new Point(59,39),
				new Point(57,38),
				new Point(61,38),
				new Point(62,38),
				// Iran
				new Point(64,36),
				new Point(65,35),
				new Point(68,31),
				new Point(68,32),
				new Point(69,32),
				// Caucasus
				new Point(60,40),
				new Point(59,42),
				new Point(60,42),
				new Point(61,42),
				new Point(61,39),
				new Point(62,39),
				new Point(57,42), // Crimea
				// Balkans
				new Point(51,44),
				new Point(49,43),
				new Point(50,43),
				new Point(53,43),
				new Point(49,42),
				new Point(50,42),
				new Point(50,41),
				new Point(51,41),
				new Point(52,41),
				new Point(53,41),
				new Point(51,40),
				new Point(53,40),
				new Point(54,40),
				new Point(51,39),
				new Point(51,38),
				new Point(48,43),
				new Point(43,40), // Corse
				new Point(45,37), // Sicily
				// Italia
				new Point(45,43),
				new Point(47,39),
				new Point(47,38),
				new Point(47,37),
				// Hispania
				new Point(36,42),
				new Point(37,42),
				new Point(40,42),
				new Point(41,42),
				new Point(36,41),
				new Point(37,41),
				new Point(38,41),
				new Point(40,41),
				new Point(36,40),
				new Point(37,40),
				// France
				new Point(39,45),
				new Point(42,45),
				new Point(40,44),
				new Point(42,44),
				new Point(42,43),
				// Denmark, NW Germany
				new Point(45,50),
				new Point(45,48),
				new Point(44,48),
				new Point(44,47),
				new Point(45,47),
				new Point(46,47),
				new Point(47,47),
				new Point(44,46),
				new Point(45,46),
				// Czechoslovakia, Austria
				new Point(47,46),
				new Point(47,45),
				new Point(47,44),
				new Point(48,44),
				new Point(48,45),
				// Poland
				new Point(48,46),
				new Point(48,47),
				new Point(49,48),
				new Point(49,47),
				new Point(49,46),
				new Point(50,47),
				new Point(51,47),
				new Point(50,46),
				new Point(51,46),
				new Point(51,45),
				new Point(50,48), // Königsberg
				// Balticum
				new Point(51,48),
				new Point(51,49),
				new Point(52,49),
				new Point(51,50),
				new Point(52,50),
				// Belarus
				new Point(52,48),
				new Point(53,48),
				new Point(52,47),
				new Point(54,47),
				new Point(52,46),
				new Point(53,46),
				// N Russia 
				new Point(53,49),
				new Point(53,51),
				new Point(54,50),
				new Point(55,49),
				new Point(55,50),
				new Point(55,51),
				new Point(56,49),
				new Point(58,47),
				new Point(58,48),
				new Point(54,55),
				new Point(53,54),
				new Point(53,53),
				new Point(54,52),
				new Point(55,52),
				new Point(56,53),
				new Point(56,52),
				new Point(58,52),
				new Point(56,51),
				new Point(57,51),
				new Point(58,51),
				new Point(60,51),
				new Point(56,50),
				new Point(57,50),
				new Point(59,50),
				new Point(58,49),
				// Ukraine, S Russia
				new Point(52,45),
				new Point(53,45),
				new Point(54,46),
				new Point(56,47),
				new Point(55,47),
				new Point(56,48),
				new Point(55,48),
				new Point(60,47),
				new Point(60,49),
				new Point(60,48),
				new Point(62,48),
				new Point(63,48),
				// Finland
				new Point(51,52),
				new Point(51,53),
				new Point(52,55),
				new Point(52,56),
				new Point(53,56),
				new Point(51,55),
				new Point(51,57),
				// Norway, Sweden
				new Point(44,52),
				new Point(45,52),
				new Point(45,53),
				new Point(46,53),
				new Point(47,52),
				new Point(48,53),
				new Point(48,51),
				new Point(46,55),
				new Point(47,54),
				new Point(48,54),
				new Point(49,54),
				new Point(49,55),
				new Point(50,55),
				new Point(50,56),
				// Siberia
				new Point(69,52),
				new Point(70,52),
				new Point(66,51),
				new Point(67,51),
				new Point(65,50),
				new Point(66,50),
				new Point(69,50),
				new Point(70,50),
				new Point(65,49),
				new Point(66,49),
				new Point(67,49),
				new Point(68,49),
				new Point(67,47), // Kazakhstan
				// Around Hindu Kush - Himalaya
				new Point(71,41),
				//new Point(71,40), // not clear if logging feasible at great steepness 
				new Point(74,38),
				new Point(72,40),
				new Point(71,39),
				new Point(73,38),
				new Point(76,37),
				new Point(76,36),
				new Point(75,36),
				new Point(83,38),
				new Point(84,38),
				new Point(81,37),
				new Point(82,37),
				new Point(77,37),
				new Point(78,36),
				new Point(79,36),
				new Point(80,36),
				// W China
				new Point(82,43),
				new Point(83,43),
				new Point(84,43),
				new Point(85,43),
				new Point(86,43),
				new Point(84,42),
				new Point(85,42),
				new Point(86,42),
				new Point(84,41),
				new Point(85,41),
				new Point(84,39),
				new Point(85,39),
				new Point(86,39),
				new Point(85,38), // Burma
				// India (rest)
				new Point(84,37),
				new Point(81,36),
				new Point(83,36),
				new Point(78,35),
				new Point(81,35),
				new Point(82,35),
				new Point(79,34),
				new Point(80,34),
				new Point(82,34),
				new Point(83,34),
				new Point(79,33),
				new Point(81,33),
				new Point(82,33),
				new Point(81,32),
				new Point(78,31),
				new Point(79,31),
				new Point(79,30),
				new Point(81,30),
				new Point(81,28),
		}));
		final Set<Point2D> jungle = new HashSet<Point2D>(
				Arrays.asList(new Point[] {
				// Africa
				new Point(36,26),
				new Point(36,25),
				new Point(37,25),
				new Point(38,25),
				new Point(41,25),
				new Point(42,25),
				new Point(42,24),
				new Point(43,24),
				new Point(43,23),
				new Point(44,23),
				new Point(45,23),
				new Point(46,23),
				new Point(47,23),
				new Point(48,23),
				new Point(43,22),
				new Point(44,22),
				new Point(45,22),
				new Point(46,22),
				new Point(47,22),
				new Point(48,22),
				new Point(49,22),
				new Point(44,21),
				new Point(45,21),
				new Point(46,21),
				new Point(47,21),
				new Point(48,21),
				new Point(49,21),
				new Point(50,22),
				new Point(50,21),
				new Point(51,21),
				new Point(52,21),
				new Point(53,21),
				new Point(54,21),
				new Point(55,21),
				//new Point(54,22), // Sudd - try a lake instead
				  /* Seasonal tropical forest (set all of these to Forest
				   * at first; some are still Forests) */
				new Point(36,27),
				new Point(37,26),
				new Point(38,26),
				new Point(39,26),
				new Point(40,26),
				new Point(41,26),
				new Point(43,25),
				new Point(44,24),
				new Point(45,24),
				new Point(46,24),
				new Point(47,24),
				new Point(48,24),
				new Point(49,23),
				new Point(50,23),
				new Point(51,22),
				new Point(52,22),
				new Point(53,22),
				new Point(56,21),
				// Burma, S China
				new Point(86,38),
				new Point(85,37),
				new Point(86,37),
				new Point(85,36),
				new Point(86,36),
				// India
				new Point(77,32),
				new Point(77,31),
				new Point(78,30),
				new Point(79,29),
				new Point(80,29),
				new Point(80,28),
				new Point(82,28),
				new Point(81,27),
				new Point(82,27),
				new Point(84,27), // Sri Lanka
		}));
		final Set<Point2D> oases = new HashSet<Point2D>(
				Arrays.asList(new Point[] {
				// Algeria
				/*  Had to move these south to prevent Carthage from being
					founded too far west. But there are oases this far 
					south too. */
				new Point(39,33),  // Adrar, Timimoun
				new Point(40,32), // Tamanrasset
				new Point(41,34), // lots of oases here
				new Point(42,33),
				new Point(44,31),
				new Point(42,28), // Ingall
				new Point(60,34), // Damascus
				// Libya
				new Point(52,30), // Siwa, Qara
				new Point(50,28), // Kufra, Tazirbu
				new Point(69,39), // Merv, Tejen
				// Arabian Desert
				new Point(64,31), // Qatif
				new Point(62,30), // Al-'Una, Ha'il
				new Point(62,29), // Tayma
				new Point(62,28), // Khaybar
				new Point(64,29), // Al-Hasa
				new Point(67,29), // Al-Ain, Liwa
				// Taklamakan
				new Point(74,41), // Kaxgar
				new Point(75,41), // Yarkant
				new Point(76,42), // Kuqa
				new Point(76,40), // Hotan
		}));
		final Set<Point2D> floodPlains = new HashSet<Point2D>(
				Arrays.asList(new Point[] {
				new Point(62,44), // Volga Delta
				// BMAC
				new Point(68,42),
				new Point(67,41),
				new Point(66,40),
				new Point(71,42),
				// Indus
				new Point(74,36),
				new Point(74,35),
				new Point(73,34),
				new Point(74,33),
				new Point(74,32),
				new Point(71,33), // Zaranj
				// Mesopotamia
				new Point(62,34),
				new Point(63,34),
				new Point(64,33),
				new Point(65,33), // Susiana
				// Egyptian Nile
				new Point(56,31),
				new Point(57,31),
				new Point(55,30), // Faiyum
				new Point(56,29),
				new Point(57,29),
				new Point(56,28),
				new Point(57,28),
				new Point(57,27),
				new Point(56,27),
				new Point(55,25),
				new Point(55,24),
				new Point(56,24),
				new Point(57,24),
		}));
		Set<Point2D> ice = new HashSet<Point2D>(
				Arrays.asList(new Point[] {
				new Point(54,61),
				new Point(55,61),
				new Point(56,61),
				new Point(57,61),
				new Point(58,61),
				new Point(59,61),
				new Point(60,61),
				new Point(55,60),
				new Point(56,60),
				new Point(57,60),
				new Point(58,60),
				new Point(59,60),
				new Point(60,60),
				new Point(56,59),
				new Point(58,59),
				new Point(59,59),
				new Point(60,59),
				new Point(57,58),
				new Point(58,58),
				new Point(59,58),
				new Point(60,58),
				new Point(58,57),
				new Point(59,57),
				new Point(60,57),
				new Point(59,56),
				new Point(60,56),
				new Point(60,55),
				new Point(60,54),
				new Point(61,56),
				new Point(63,56),
				new Point(64,56),
				new Point(65,56),
				new Point(66,56),
				new Point(69,56),
				new Point(70,56),
				new Point(61,55),
				new Point(63,55),
				new Point(64,55),
		}));
		for(int x = 34; x <= 60; x++) {
			ice.add(new Point(x,63));
			if(x < 50 || x > 53) // Hole above Spitsbergen
				ice.add(new Point(x,62));
		}
		for(int x = 61; x <= 70; x++) {
			for(int y = 57; y <= 63; y++) {
				if(x == 63 && y == 57) // Severnaya Zemlya
					continue;
				if(x == 67 && y == 57) // Lyakhovsky Islands
					continue;
				ice.add(new Point(x,y));
			}
		}
		// Seal off Asia for testing
		for(int y = 56; y >= 44; y--) { // Northern Asia
			ice.add(new Point(71,y));
			ice.add(new Point(72,y));
		}
		for(int y = 44; y >= 35; y--) { // Southern Asia
			ice.add(new Point(87,y));
			ice.add(new Point(88,y));
		}
		for(int x = 73; x <= 86; x++) { // Connect the two
			ice.add(new Point(x,44));
			ice.add(new Point(x,45));
		}
		for(int x = 44; x <= 65; x++) { // Seal off Africa for testing
			ice.add(new Point(x,20));
			ice.add(new Point(x,19));
		}
		final Set<Point2D> huts = new HashSet<Point2D>(
				Arrays.asList(new Point[] {
				new Point(43,35), // Carthage
				new Point(59,33), // Jerusalem
				new Point(58,38), // Hattusa
				new Point(53,34), // Heraklion
				new Point(48,36), // Tarentum
				new Point(39,38), // Malaka
				new Point(50,44), // Budapest
				new Point(40,46), // Lutetia
				new Point(46,50), // Copenhagen
				new Point(57,42), // Sevastopol
				new Point(53,48), // Minsk
				new Point(49,56), // Tromsoe
				new Point(40,49), // Londinium
				new Point(67,27), // Magan (copper there soon depleted)
				new Point(69,39), // Merv
				//new Point(65,38), // Sari
				//new Point(64,48), // Yekaterinburg (Siberian furs)
				new Point(46,27), // Koro Toro
				new Point(63,22), // Land of Punt
				new Point(75,35), // Harappa
				new Point(74,33), // Mohenjodaro
				//new Point(73,41), // Kokand (Silk Road)
				new Point(79,43), // Golmud (salts)
				new Point(81,39), // Lhasa
		}));
		for(Point2D p : hills)
			p.setLocation(p.getX() + offsetX, p.getY() + offsetY);
		for(Point2D p : forests)
			p.setLocation(p.getX() + offsetX, p.getY() + offsetY);
		for(Point2D p : jungle)
			p.setLocation(p.getX() + offsetX, p.getY() + offsetY);
		for(Point2D p : oases)
			p.setLocation(p.getX() + offsetX, p.getY() + offsetY);
		for(Point2D p : floodPlains)
			p.setLocation(p.getX() + offsetX, p.getY() + offsetY);
		for(Point2D p : ice)
			p.setLocation(p.getX() + offsetX, p.getY() + offsetY);
		for(Point2D p : huts)
			p.setLocation(p.getX() + offsetX, p.getY() + offsetY);
		final Set<Bonus> bonuses = new HashSet<Bonus>(
				Arrays.asList(new Bonus[] {
				// Tunisia
				new Bonus("WINE",42,36),
				new Bonus("WHEAT",43,34),
				new Bonus("HORSE",41,35),
				new Bonus("IVORY",38,35), // Atlas
				// Inner Sahara
				new Bonus("URANIUM",42,29),
				new Bonus("IRON",43,28),
				new Bonus("COPPER",35,30),
				// Western Sudanic Zone
				new Bonus("GOLD",35,29),
				new Bonus("ALUMINUM",34,28),
				new Bonus("ALUMINUM",35,28),
				new Bonus("RICE",36,28),
				new Bonus("DYE",41,27),
				new Bonus("FUR",43,26),
				new Bonus("IRON",47,24),
				// Ethiopia
				new Bonus("HORSE",58,23),
				new Bonus("IVORY",57,22),
				new Bonus("COW",58,22),
				new Bonus("FUR",58,21),
				// African rainforest
				new Bonus("SPICES",36,26),
				new Bonus("GOLD",38,25),
				new Bonus("OIL",41,25),
				new Bonus("BANANA",42,25),
				new Bonus("IVORY",44,22),
				new Bonus("BANANA",54,21), // Uganda
				// Sinai
				new Bonus("COPPER",60,31),
				new Bonus("GEMS",58,30),
				// Nile
				new Bonus("WHEAT",55,31),
				new Bonus("STONE",56,30),
				new Bonus("GOLD",58,27),
				new Bonus("IRON",56,25),
				// Levante
				new Bonus("WINE",59,34),
				new Bonus("HORSE",60,33),
				new Bonus("COPPER",57,34), // Cyprus
				// Caucasus
				new Bonus("COPPER",60,40),
				new Bonus("SHEEP",61,41),
				new Bonus("WINE",61,40),
				new Bonus("OIL",63,40),
				// Anatolia
				new Bonus("COAL",56,39),
				new Bonus("MARBLE",55,37),
				new Bonus("HORSE",56,38),
				new Bonus("IRON",57,37),
				new Bonus("SHEEP",60,37),
				new Bonus("STONE",61,37),
				new Bonus("COPPER",61,38),
				new Bonus("WINE",54,34), // Crete
				// Greece
				new Bonus("GOLD",52,41), // Moved from Romania to Thrace
				new Bonus("FISH",51,34),
				new Bonus("ALUMINUM",52,38),
				new Bonus("WHEAT",53,38),
				new Bonus("MARBLE",52,37),
				new Bonus("MARBLE",54,40), // Moved to let "Thrace" act as Constantinople
				new Bonus("SILVER",53,36),
				new Bonus("WHEAT",44,39), // Sardegna
				new Bonus("WHEAT",44,37), // Moved from Belgium to Sicily
				// Italia
				new Bonus("FISH",45,39),
				new Bonus("PIG",47,38),
				new Bonus("MARBLE",45,41),
				new Bonus("WINE",45,42),
				new Bonus("WINE",48,36), // Moved from Sicily
				// Hispania
				new Bonus("FISH",37,38),
				new Bonus("SILVER",37,39),
				new Bonus("PIG",39,39),
				new Bonus("WINE",38,38),
				new Bonus("IRON",37,40),
				new Bonus("SHEEP",39,41),
				new Bonus("COPPER",37,41),
				new Bonus("WINE",41,41),
				new Bonus("GOLD",37,42),
				new Bonus("COAL",38,42),
				new Bonus("WHALE",39,43),
				// France, W Rhine
				new Bonus("CRAB",38,45),
				new Bonus("COW",39,46), // Moved from SW France to NW
				new Bonus("WINE",40,44),
				new Bonus("HORSE",41,44),
				new Bonus("DYE",41,43),
				new Bonus("MARBLE",42,43),
				new Bonus("WINE",43,45),
				new Bonus("STONE",41,46),
				new Bonus("COAL",43,46),
				new Bonus("SHEEP",42,47),
				new Bonus("PIG",43,44),
				// Germany, Denmark
				new Bonus("COW",44,48),
				new Bonus("HORSE",45,49), // Moved one up (swapped with a forest) to block HRE city in Denmark
				new Bonus("PIG",45,50),
				new Bonus("SILVER",45,46),
				new Bonus("IRON",44,46),
				new Bonus("DYE",46,46),
				new Bonus("URANIUM",47,46), // Czechia
				new Bonus("IRON",47,44), // Austria
				// Poland
				// Can place one in Hungary if barbs block it
				magyars ? new Bonus("PIG",49,44) : new Bonus("PIG",51,47),
				magyars ? new Bonus("PIG",49,47) : new Bonus("PIG",46,47),
				new Bonus("COAL",49,46),
				new Bonus("GEMS",50,48), // Königsberg
				new Bonus("COW",54,48), // Belarus/Moscow
				new Bonus("ALUMINUM",54,52), // St. Petersburg
				// Finland
				new Bonus("DEER",51,55),
				new Bonus("FUR",51,53),
				new Bonus("MARBLE",53,53),
				// Spitsbergen
				new Bonus("WHALE",51,61),
				new Bonus("WHALE",52,58),
				// Norway, Sweden
				new Bonus("FISH",43,53),
				new Bonus("FISH",47,57),
				new Bonus("HORSE",44,54),
				new Bonus("IRON",44,53),
				new Bonus("MARBLE",48,51),
				new Bonus("DEER",49,54),
				new Bonus("FISH",37,58), // Iceland
				new Bonus("COW",36,52), // Ireland
				// Great Britain
				new Bonus("CRAB",38,54),
				new Bonus("COPPER",38,49),
				new Bonus("IRON",39,49),
				new Bonus("STONE",38,50), // Moved from Hispania
				new Bonus("SHEEP",39,50),
				new Bonus("WHEAT",41,50),
				new Bonus("HORSE",40,52),
				new Bonus("URANIUM",40,54),
				new Bonus("FISH",42,50), // Dogger Bank (shifted away from Denmark)
				// N Balkans
				new Bonus("COPPER",51,42),
				//new Bonus("MARBLE",52,42), // Too much
				new Bonus("WINE",53,41),
				// Ukraine
				new Bonus("URANIUM",56,45),
				new Bonus("IRON",56,44),
				new Bonus("COAL",57,44),
				new Bonus("WINE",56,43),
				// W Russia
				new Bonus("STONE",56,48),
				new Bonus("IRON",57,47),
				new Bonus("WHEAT",57,45), // Chernozem; moved from S Denmark
				new Bonus("HORSE",59,46),
				new Bonus("COW",61,46),
				new Bonus("FUR",57,52),
				new Bonus("DEER",58,52),
				// Urals
				new Bonus("IRON",64,46),
				new Bonus("COPPER",64,47),
				new Bonus("FUR",63,48),
				// Siberia and Kazakhstan
				new Bonus("OIL",65,43),
				new Bonus("OIL",61,51),
				new Bonus("OIL",62,50),
				new Bonus("OIL",66,52),
				new Bonus("DEER",69,54),
				new Bonus("COAL",70,49),
				new Bonus("URANIUM",65,48),
				new Bonus("URANIUM",67,48),
				new Bonus("HORSE",65,47),
				new Bonus("COAL",69,47),
				new Bonus("HORSE",70,47),
				new Bonus("ALUMINUM",67,46),
				// BMAC
				new Bonus("URANIUM",70,42),
				new Bonus("SHEEP",70,40),
				new Bonus("HORSE",67,39),
				new Bonus("IRON",67,38),
				new Bonus("OIL",70,38),
				new Bonus("HORSE",72,41),
				new Bonus("IRON",73,41),
				new Bonus("GEMS",75,40), // Taklamakan
				// Afghanistan
				new Bonus("COPPER",71,37),
				new Bonus("GEMS",72,37),
				// Indus, Punjab, Thar
				new Bonus("COW",74,37),
				new Bonus("WHEAT",75,37),
				new Bonus("SUGAR",75,36),
				new Bonus("IVORY",76,36),
				new Bonus("MARBLE",76,35),
				new Bonus("SILVER",76,34),
				new Bonus("COW",76,32),
				new Bonus("DYE",75,31),
				new Bonus("OIL",76,31),
				new Bonus("SILK",74,38),
				// India (rest)
				new Bonus("DYE",84,38),
				new Bonus("SUGAR",84,37),
				new Bonus("SPICES",81,37),
				new Bonus("COPPER",81,36),
				new Bonus("RICE",83,36),
				new Bonus("IRON",79,36),
				new Bonus("SUGAR",78,35),
				new Bonus("COW",79,35),
				new Bonus("COAL",81,35),
				new Bonus("URANIUM",82,35),
				new Bonus("DYE",83,35),
				new Bonus("DYE",79,34),
				new Bonus("COAL",80,34),
				new Bonus("IRON",82,34),
				new Bonus("SHEEP",78,33),
				new Bonus("IVORY",79,33),
				new Bonus("ALUMINUM",81,33),
				new Bonus("BANANA",77,32),
				new Bonus("RICE",81,32),
				new Bonus("FISH",83,32),
				new Bonus("SILK",79,31),
				new Bonus("GEMS",81,31),
				new Bonus("IVORY",79,30),
				new Bonus("INCENSE",81,30),
				new Bonus("FISH",78,29),
				new Bonus("PIG",79,29),
				new Bonus("GOLD",81,29),
				new Bonus("DYE",82,29),
				new Bonus("SPICES",80,28),
				new Bonus("IRON",81,28),
				new Bonus("SUGAR",82,28),
				new Bonus("SPICES",81,27),
				new Bonus("BANANA",82,27),
				new Bonus("CLAM",84,26), // Moved closer to Sri Lanka to make the island more likely to be settled by the AI
				new Bonus("GEMS",84,27),
				new Bonus("IVORY",85,27),
				new Bonus("SPICES",85,26),
				// Burma
				new Bonus("GEMS",86,38),
				new Bonus("IVORY",85,36),
				// W China
				new Bonus("SILK",84,43),
				new Bonus("ALUMINUM",85,42),
				new Bonus("HORSE",83,41),
				new Bonus("SILVER",86,40),
				// Iran
				new Bonus("HORSE",65,36),
				new Bonus("COW",64,35),
				new Bonus("WINE",65,35),
				new Bonus("SHEEP",66,34),
				new Bonus("PIG",67,33),
				new Bonus("COPPER",68,33),
				new Bonus("SPICES",66,35),
				new Bonus("SHEEP",71,31),
				// Mesopotamia
				new Bonus("WHEAT",63,36),
				new Bonus("WHEAT",62,35),
				// Gulf oil
				new Bonus("OIL",65,34),
				new Bonus("OIL",64,33),
				new Bonus("OIL",64,32),
				new Bonus("OIL",65,31),
				new Bonus("OIL",66,31),
				new Bonus("OIL",65,30),
				new Bonus("OIL",66,30),
				new Bonus("OIL",65,29),
				new Bonus("OIL",66,27),
				// Arabian Peninsula
				new Bonus("HORSE",65,28),
				new Bonus("INCENSE",66,26),
				new Bonus("INCENSE",63,25),
				new Bonus("CLAM",66,29),
				new Bonus("DYE",64,24),
				// Somalia
				new Bonus("INCENSE",64,22),
				new Bonus("INCENSE",65,22),
		}));
		//Random r = new Random(0);
		for(int x = 0; x < w; x++) {
			for(int y = 0; y < h; y++) {
				final Point2D plot = new Point(x, y);
				pr("BeginPlot");
				pr("\tx=" + (int)plot.getX() + ",y=" + (int)plot.getY());
				Optional<Bonus> optBonus = bonuses.stream().filter(
						p -> p.getPlot().equals(plot)).findAny(); 
				if(optBonus.isPresent())
					pr("\tBonusType=BONUS_" + optBonus.get().getType());
				String terrain = "OCEAN";
				int plotType = 3;
				if(desert.stream().anyMatch(p -> p.equals(plot))) {
					terrain = "DESERT";
				}
				else if(plains.stream().anyMatch(p -> p.equals(plot))) {
					terrain = "PLAINS";
				}
				else if(grass.stream().anyMatch(p -> p.equals(plot))) {
					terrain = "GRASS";
				}
				else if(peak.stream().anyMatch(p -> p.equals(plot))) {
					terrain = "SNOW";
					plotType = 0;
				}
				else if(tundra.stream().anyMatch(p -> p.equals(plot))) {
					terrain = "TUNDRA";
				}
				else if(snow.stream().anyMatch(p -> p.equals(plot))) {
					terrain = "SNOW";
				}
				boolean isLand = !terrain.equals("OCEAN");
				if(!isLand) {
					if(euStarts.stream().anyMatch(p -> p.equals(plot))) {
						terrain = "GRASS";
						plotType = 2;
						isLand = true;
					}
					else if(!seaObstacles.stream().anyMatch(
							p -> p.equals(plot))) { // Automatic coasts
						for(Point2D adjPlot : adjacentPlots(plot)) {
							if(land.stream().anyMatch(p -> p.equals(adjPlot))) {
								terrain = "COAST";
								break;
							}
						}
					}
				}
				if(isLand) { // for performance
					int riverCount = 0;
					for(River ri : rivers) {
						if(!ri.getPlot().equals(plot))
							continue;
						Direction of = ri.getOf();
						if(of == S)
							pr("\tisNOfRiver");
						else if(of == E)
							pr("\tisWOfRiver");
						else assert(false);
						pr("\tRiver" + (of == S ? "WE" : "NS") + "Direction="
								+ ri.getTo().ordinal());
						riverCount++;
						if(riverCount >= 2)
							break;
					}
					if(hills.stream().anyMatch(p -> p.equals(plot)))
						plotType = 1;
					else if(plotType > 0)
						plotType = 2;
				}
				String feature = "";
				int variety = 0;
				if(forests.stream().anyMatch(p -> p.equals(plot))) {
					feature = "FOREST";
					if(terrain.equals("SNOW"))
						variety = 2;
					else if(terrain.equals("GRASS")) {
						// On flat grassland, primary coniferous forest is rare.
						if(plotType != 1)
							variety = 0;
						else {
							int adjCold = 0, adjOtherLand = 0, adjJungle = 0;
							for(Point2D adjPlot : adjacentPlots(plot)) {
								if(hills.stream().anyMatch(p -> p.equals(adjPlot)) ||
										tundra.stream().anyMatch(p -> p.equals(adjPlot)) ||
										snow.stream().anyMatch(p -> p.equals(adjPlot)) ||
										ice.stream().anyMatch(p -> p.equals(adjPlot)) ||
										peak.stream().anyMatch(p -> p.equals(adjPlot)))
									adjCold++;
								else if(land.stream().anyMatch(p -> p.equals(adjPlot)))
									adjOtherLand++;
								if(jungle.stream().anyMatch(p -> p.equals(adjPlot)))
									adjJungle++;
							}
							if(plotType == 1) // If 'plot' is a hill
								adjCold++;
							if(adjCold - adjOtherLand >= 2 && adjJungle <= 0)
								variety = 1;
						}
					}
					else if(terrain.equals("TUNDRA")) {
						variety = 2; // Boreal forest
						int adjIcy = 0;//, adjOtherLand = 0;
						for(Point2D adjPlot : adjacentPlots(plot)) {
							if(snow.stream().anyMatch(p -> p.equals(adjPlot)) ||
									ice.stream().anyMatch(p -> p.equals(adjPlot)) ||
									peak.stream().anyMatch(p -> p.equals(adjPlot)))
								adjIcy++;
							/*else if(land.stream().anyMatch(p -> p.equals(adjPlot)))
								adjOtherLand++;*/
						}
						if(plotType == 1)
							adjIcy++;
						if(adjIcy < 2)
							variety = 1;
					}
					else if(terrain.equals("PLAINS")) {
						variety = 1; // Conifers on dry soils
						int adjWet = 0, adjDry = 0;
						for(Point2D adjPlot : adjacentPlots(plot)) {
							if(grass.stream().anyMatch(p -> p.equals(adjPlot)) ||
									tundra.stream().anyMatch(p -> p.equals(adjPlot)))
								adjWet++;
							if(jungle.stream().anyMatch(p -> p.equals(adjPlot)))
								adjWet += 3;
							if(plains.stream().anyMatch(p -> p.equals(adjPlot)) ||
									desert.stream().anyMatch(p -> p.equals(adjPlot)) ||
									peak.stream().anyMatch(p -> p.equals(adjPlot)))
								adjDry++;
						}
						if(adjWet - adjDry >= 2)
							variety = 0;
					}
				}
				else if(jungle.stream().anyMatch(p -> p.equals(plot)))
					feature = "JUNGLE";
				else if(oases.stream().anyMatch(p -> p.equals(plot)))
					feature = "OASIS";
				else if(floodPlains.stream().anyMatch(p -> p.equals(plot)))
					feature = "FLOOD_PLAINS";
				else if(ice.stream().anyMatch(p -> p.equals(plot)))
					feature = "ICE";
				if(!feature.isEmpty()) {
					pr("\tFeatureType=FEATURE_" + feature
							+ ", FeatureVariety=" + variety);
					assert(feature.equals("ICE") == !isLand);
				}
				pr("\tTerrainType=TERRAIN_" + terrain);
				pr("\tPlotType=" + plotType);
				Optional<BarbCity> optBarbCity = barbs.stream().filter(
						p -> p.getPlot().equals(plot)).findAny(); 
				if(optBarbCity.isPresent()) {
					for(String barbUnit : barbGarrison) {
						pr("\tBeginUnit");
						pr("\t\tUnitType=UNIT_" + barbUnit + ", UnitOwner=18");
						//pr("\t\tLevel=1, Experience=0");
						pr("\tEndUnit");
					}
					BarbCity c = optBarbCity.get();
					pr("\tBeginCity");
					pr("\t\tCityOwner=18");
					pr("\t\tCityName=TXT_KEY_CITY_NAME_" + c.getName());
					//pr("\t\tCityPopulation=1");
					for(String religion : c.getReligions())
						pr("\t\tReligionType=RELIGION_" + religion);
					for(String religion : c.getReligions())
						pr("\t\tHolyCityReligionType=RELIGION_" + religion);
					pr("\tEndCity");
				}
				if(optBarbCity.isPresent() &&
						!huts.stream().anyMatch(p -> p.equals(plot)))
					pr("\tImprovementType=IMPROVEMENT_GOODY_HUT");
				pr("EndPlot");
			}
		}
	}
	
	private static void addSafe(Set<Point2D> to, Set<Point2D> from) {
		for(Point2D p : from) {
			if(to.contains(p)) {
				System.err.println("Multiple terrain types defined for " + p);
				assert(false);
				continue;
			}
			to.add(p);
		}
	}

	public static void pr(String s) {
		System.out.println(s);
	}
	
	public static Iterable<Point2D> adjacentPlots(Point2D center) {
		ArrayList<Point2D> r = new ArrayList<Point2D>();
		for(int dx = -1; dx <= 1; dx++) {
			for(int dy = -1; dy <= 1; dy++) {
				if(dx != 0 || dy != 0) {
					r.add(new Point((int)center.getX() + dx,
							(int)center.getY() + dy));
				}
			}
		}
		return r;
	}
}
