package wbo;
import java.awt.Point;
import java.awt.geom.Point2D;
import java.util.ArrayList;


public class Civ implements Comparable<Civ> {

	public Civ(String leader, String civ, int x, int y, String tech1, String tech2,
			String[] cities, String handicap, boolean playable, boolean test,
			String[] religions) {
		initDefault();
		this.leader = leader;
		this.civ = civ;
		this.start = new Point(x, y);
		this.tech1 = tech1;
		this.tech2 = tech2;
		for(String city : cities)
			this.cities.add(city);
		if(test)
			this.handicap = "EMPEROR";
		else this.handicap = handicap;
		this.playable = (test ? true : playable);
		this.test = test;
		for(String rel : religions)
			this.religions.add(rel);
		finalizeInit();
	}
	
	public Civ(String leader, String civ, int x, int y, String tech1, String tech2,
				String[] cities, String handicap, boolean playable, boolean test) {
		initDefault();
		this.leader = leader;
		this.civ = civ;
		this.start = new Point(x, y);
		this.tech1 = tech1;
		this.tech2 = tech2;
		for(String city : cities)
			this.cities.add(city);
		this.handicap = handicap;
		this.playable = playable;
		this.test = test;
		finalizeInit();
	}
	
	public Civ(String leader, String civ, int x, int y, String tech1, String tech2,
			String[] cities, String handicap, boolean playable) {
		initDefault();
		this.leader = leader;
		this.civ = civ;
		this.start = new Point(x, y);
		this.tech1 = tech1;
		this.tech2 = tech2;
		for(String city : cities)
			this.cities.add(city);
		this.handicap = handicap;
		this.playable = playable;
		finalizeInit();
	}
	
	public Civ(String leader, String civ, int x, int y, String tech1, String tech2,
			String[] cities, String handicap) {
		initDefault();
		this.leader = leader;
		this.civ = civ;
		this.start = new Point(x, y);
		this.tech1 = tech1;
		this.tech2 = tech2;
		for(String city : cities)
			this.cities.add(city);
		this.handicap = handicap;
		finalizeInit();
	}
	
	
	public Civ(String leader, String civ, int x, int y, String tech1, String tech2,
			String[] cities) {
		initDefault();
		this.leader = leader;
		this.civ = civ;
		this.start = new Point(x, y);
		this.tech1 = tech1;
		this.tech2 = tech2;
		for(String city : cities)
			this.cities.add(city);
		finalizeInit();
	}
	
	public Civ(String leader, String civ, int x, int y, String tech1, String tech2) {
		initDefault();
		this.leader = leader;
		this.civ = civ;
		this.start = new Point(x, y);
		this.tech1 = tech1;
		this.tech2 = tech2;
		finalizeInit();
	}
	
	public Civ(String leader, String civ, int x, int y, String tech1) {
		initDefault();
		this.leader = leader;
		this.civ = civ;
		this.start = new Point(x, y);
		this.tech1 = tech1;
		finalizeInit();
	}
	
	public Civ(String leader, String civ, int x, int y) {
		initDefault();
		this.leader = leader;
		this.civ = civ;
		this.start = new Point(x, y);
		finalizeInit();
	}
	
	public Point2D getStart() {
		return start;
	}
	
	public String teamEntry() {
		String r = "BeginTeam\n";
		if(!tech1.isEmpty())
			r += "\tTech=TECH_" + tech1 + "\n";
		if(!tech2.isEmpty())
			r += "\tTech=TECH_" + tech2 + "\n";
		if(test) {
			r += "\tTech=TECH_CALENDAR\n";
			r += "\tTech=TECH_SATELLITES\n";
		}
		for(String rel : religions) {
			String relTech = "";
			if(rel.equals("BUDDHISM"))
				relTech = "MEDITATION";
			if(rel.equals("HINDUISM"))
				relTech = "POLYTHEISM";
			if(rel.equals("JUDAISM"))
				relTech = "MONOTHEISM";
			if(rel.equals("CONFUCIANISM"))
				relTech = "CODE_OF_LAWS";
			if(rel.equals("TAOISM"))
				relTech = "PHILOSOPHY";
			if(rel.equals("CHRISTIANITY"))
				relTech = "THEOLOGY";
			if(rel.equals("ISLAM"))
				relTech = "DIVINE_RIGHT";
			if(!relTech.isEmpty())
				r += "\tTech=TECH_" + relTech + "\n";
		}
		return r + "EndTeam";
	}
	
	public String civEntry(int id) {
		String r = "BeginPlayer\n";
		r += "\tLeaderType=LEADER_" + leader + "\n";
		r += "\tCivType=CIVILIZATION_" + civ + "\n";
		r += "\tTeam=" + id + "\n";
		r += "\tPlayableCiv=" + (playable ? 1 : 0) + "\n";
		r += "\tStartingX=" + (int)start.getX() + ", StartingY=" +
				(int)start.getY() + "\n";
		r += "\tHandicap=HANDICAP_" + handicap + "\n";
		if(!cities.isEmpty()) {
			for(String city : cities) {
				if(Character.isUpperCase(city.charAt(city.length() - 1)))
					city = "TXT_KEY_CITY_NAME_" + city;
				r += "\tCityList=" + city + "\n";
			}
		}
		return r + "EndPlayer";
	}
	
	@Override
	public int compareTo(Civ other) {

		if(test && !other.test)
			return -1;
		if(!test && other.test)
			return 1;
		int thisHP = handicapPriority();
		int otherHP = other.handicapPriority();
		if(thisHP > otherHP)
			return -1;
		if(thisHP < otherHP)
			return 1;
		/*String l1 = (leader.startsWith("CHINESE") ? "MAO" : leader);
		String l2 = (other.leader.startsWith("CHINESE") ? "MAO" : other.leader);
		return l1.compareTo(l2);*/
		return civ.compareTo(other.civ);
	}
	
	private void initDefault() {
		cities = new ArrayList<String>();
		religions = new ArrayList<String>();
		start = new Point(0, 0);
		playable = true;
		leader = civ = tech1 = tech2 = handicap = "";
	}
	
	private void finalizeInit() {
		if(handicap.isEmpty())
			handicap = "NOBLE";
		WBOut.offset(start);
		int btsId = -1;
		if(civ.equals("ARABIA"))
			btsId = 0;
		else if(civ.equals("CHINA"))
			btsId = 1;
		else if(civ.equals("EGYPT"))
			btsId = 2;
		else if(civ.equals("ENGLAND"))
			btsId = 3;
		else if(civ.equals("ETHIOPIA"))
			btsId = 4;
		else if(civ.equals("GREECE"))
			btsId = 5;
		else if(civ.equals("INCA"))
			btsId = 6;
		else if(civ.equals("INDIA"))
			btsId = 7;
		else if(civ.equals("JAPAN"))
			btsId = 8;
		else if(civ.equals("KHMER"))
			btsId = 9;
		else if(civ.equals("MALI"))
			btsId = 10;
		else if(civ.equals("MAYA"))
			btsId = 11;
		else if(civ.equals("MONGOL"))
			btsId = 12;
		else if(civ.equals("PERSIA"))
			btsId = 13;
		else if(civ.equals("ROME"))
			btsId = 14;
		else if(civ.equals("RUSSIA"))
			btsId = 15;
		else if(civ.equals("ZULU"))
			btsId = 16;
		else if(civ.equals("CELT"))
			btsId = 17;
		assert(btsId >= 0);
		/* K-Mod doesn't require this, but the unmodded game doesn't use the
		 * standard city lists as a fallback. I hope overlapping city lists
		 * are OK (they are in K-Mod). */
		for(String cityName : btsCities[btsId]) {
			if(!cities.contains(cityName))
				cities.add(cityName);
		}
	}
	
	private int handicapPriority() {

		/*if(handicap.equals("NOBLE"))
			return 10;
		else */if(handicap.equals("DEITY"))
			return 1;
		else if(handicap.equals("IMMORTAL"))
			return 2;
		else if(handicap.equals("EMPEROR"))
			return 3;
		else if(handicap.equals("MONARCH"))
			return 4;
		else if(handicap.equals("PRINCE"))
			return 5;
		return 7;
	}

	private String leader, civ, tech1, tech2, handicap;
	private ArrayList<String> cities;
	private ArrayList<String> religions;
	private Point2D start;
	private boolean playable, test;
	private static final String[][] btsCities = {
	{	"MECCA",
		"MEDINA",
		"DAMASCUS",
		"BAGHDAD",
		"NAJRAN",
		"KUFAH",
		"BASRA",
		"KHURASAN",
		"ANJAR",
		"FUSTAT",
		"ADEN",
		"YAMAMA",
		"MUSCAT",
		"MANSURA",
		"BUKHARA",
		"FEZ",
		"SHIRAZ",
		"MERW",
		"BALKH",
		"MOSUL",
		"AYDAB",
		"BAYT_RAS",
		"SUHAR",
		"TAIF",
		"HAMA",
		"TABUK",
		"SANAA",
		"SHIHR",
		"TRIPOLI",
		"TUNIS",
		"KAIROUAN",
		"ALGIERS",
		"ORAN",
		"TANGIER",
		"CASABLANCA",
		"MARRAKECH",
	},
	// Indented: Potential South Cinese (Khmer) cities
	{	"BEIJING",
		 "SHANGHAI",
		"GUANGZHOU",
		"NANJING",
		"XIAN",
		"CHENGDU",
		 "HANGZHOU",
		 "TIANJIN",
		 "MACAU",
		"SHANDONG",
		"KAIFENG",
		"NINGBO",
		"BAODING",
		 "YANGZHOU",
		 "HAERBIN",
		"CHONGQING",
		"LOYANG",
		"KUNMING",
		 "TAIPEI",
		"SHENYANG",
		"TAIYUAN",
		 "TAINAN",
		"DALIAN",
		"LIJIANG",
		"WUXI",
		 "SUZHOU",
		 "MAOMING",
		 "SHAOGUAN",
		 "YANGJIANG",
		 "HEYUAN",
		"LIAONING",
		"HUBEI",
		"GANSU",
		"HUANGSHI",
		"YICHANG",
		 "YINGTAN",
		"XINYU",
		"XINZHENG",
		"HANDAN",
		"DUNHUANG",
		"GAOYU",
		"NANTONG",
		"WEIFANG",
		"XIKANG",
	},	
	{	"THEBES",
		"MEMPHIS",
		"HELIOPOLIS",
		"ELEPHANTINE",
		"ALEXANDRIA",
		"PIRAMESSES",
		"GIZA",
		"BYBLOS",
		"AKHETATEN",
		"HIERACONPOLIS",
		"ABYDOS",
		"ASYUT",
		"AVARIS",
		"LISHT",
		"BUTO",
		"EDFU",
		"PITHOM",
		"BUSIRIS",
		"KAHUN",
		"ATHRIBIS",
		"MENDES",
		"ELASHMUNEIN",
		"TANIS",
		"BUBASTIS",
		"THIS",
		"ORYX",
		"SEBENNYTUS",
		"AKHMIN",
		"KARNAK",
		"LUXOR",
		"ELKAB",
		"ARMANT",
		"BALAT",
		"ELLAHUN",
		"GHURAB",
		"HAWARA",
		"DASHUR",
		"RAQOTE",
		"DAMANHUR",
		"MERIMDE",
		"ABUSIR",
		"HERAKLEOPOLIS",
		"AKORIS",
		"BENIHASAN",
		"TASA",
		"BADARI",
		"HERMOPOLIS",
		"AMRAH",
		"NEGADE",
		"KOPTOS",
		"HERMONTHIS",
		"OMBOS",
		"ANIBA",
		"SOLEB",
		"SEMNA",
		"AMARA",
	},	
	{	"LONDON",
		"YORK",
		"NOTTHINGHAM",
		"HASTINGS",
		"CANTERBURY",
		"COVENTRY",
		"WARWICK",
		"NEWCASTLE",
		"OXFORD",
		"LIVERPOOL",
		"DOVER",
		"BRIGHTON",
		"NORWICH",
		"LEEDS",
		"READING",
		"BIRMINGHAM",
		"RICHMOND",
		"EXETER",
		"CAMBRIDGE",
		"GLOUCESTER",
		"MANCHESTER",
		"BRISTOL",
		"LEICESTER",
		"CARLISLE",
		"IPSWICH",
		"PORTSMOUTH",
		"BERWICK",
		"BATH",
		"MUMBLES",
		"SOUTHAMPTON",
		"SHEFFIELD",
		"SALISBURY",
		"COLCHESTER",
		"PLYMOUTH",
		"LANCASTER",
		"BLACKPOOL",
		"WINCHESTER",
		"HULL",
	},	
	{	"AKSUM",
		"GONDAR",
		"LALIBELA",
		"ADDIS_ABABA",
		"YEHA",
		"DEBRE_BERHAN",
		"ADULIS",
		"QOHAITO",
		"MATARA",
		"HAWULTI",
		"MASSAWA",
		"ADWA",
		"FASIL_GHEBBI",
		"ZEILA",
		"BERBERA",
		"HARAR",
		"KASKASE",
		"ADDI_GALAMO",
		"TOKONDA",
		"KUBAR",
		"TEGULAT",
		"DEBRE_TABOR",
		"MAGDALA",
		"ANKOBER",
		"ASMARA",
		"DIRE_DAWA",
		"ADAMA",
		"JIMMA",
		"MEKELE",
		"TIYA",
		"DEIRA",
		"GABAZA",
		"DAHLAK_KEBIR",
		"MELAZO",
		"FIKYA",
		"SABEA",
		"HAWILA_ASSARAW",
		"WEQRO",
	},	
	{	"ATHENS",
		"SPARTA",
		"CORINTH",
		"THEBES",
		"ARGOS",
		"KNOSSOS",
		"MYCENAE",
		"PHARSALOS",
		"EPHESUS",
		"HALICARNASSUS",
		"RHODES",
		"ERETRIA",
		"PERGAMON",
		"MILETOS",
		"MEGARA",
		"PHOCAEA",
		"SICYON",
		"TIRYNS",
		"SAMOS",
		"MYTILENE",
		"CHIOS",
		"PAROS",
		"TEGEA",
		"ELIS",
		"SYRACUSE",
		"HERAKLEIA",
		"GORTYN",
		"CHALKIS",
		"PYLOS",
		"PELLA",
		"NAXOS",
		"SICYON",
		"SMYRNA",
		"LARISSA",
		"APOLLONIA",
		"MESSENE",
		"ORCHOMENOS",
		"AMBRACIA",
		"KOS",
		"KNIDOS",
		"AMPHIPOLIS",
		"PATRAS",
		"LAMIA",
		"NAFPLION",
		"APOLYTON",
	},	
	{	"CUZCO",
		"TIWANAKU",
		"MACHU",
		"OLLANTAYTAMBO",
		"CORIHUAYACHINA",
		"HUAMANGA",
		"VILCAS",
		"VILCABAMBA",
		"VITCOS",
		"ANDAHUAYLAS",
		"ICA",
		"AREQUIPA",
		"NASCA",
		"ATICO",
		"JULI",
		"CHUITO",
		"CHUQUIAPO",
		"HUANUCOPAMPA",
		"TAMBOCCOCHA",
		"HUARAS",
		"RIOBAMBA",
		"CAXAMALCA",
		"SAUSA",
		"TAMBOCOLORADO",
		"HUACA",
		"TUMBES",
		"CHAN_CHAN",
		"SIPAN",
		"PACHACAMAC",
		"LLACTAPATA",
		"PISAC",
		"KUELAP",
		"PAJATEN",
		"CHUCUITO",
	},	
	{	"DELHI",
		"BOMBAY",
		"VIJAYANAGARA",
		"PATALIPUTRA",
		"VARANASI",
		"AGRA",
		"CALCUTTA",
		"LAHORE",				
		"MADRAS",
		"BANGALORE",
		"HYDERABAD",
		"MADURAI",
		"AHMEDABAD",
		"KOLHAPUR",
		"PRAYAGA",
		"AYODHYA",
		"INDRAPRASTHA",
		"MATHURA",
		"UJJAIN",
		"TAXILA",
		"GULBARGA",
		"JAUNPUR",
		"RAJAGRIHA",
		"SRAVASTI",
		"TIRUCHIRAPALLI",
		"THANJAVUR",
		"BODHGAYA",
		"KUSHINAGAR",
		"AMARAVATI",
		"GAUR",
		"GWALIOR",
		"JAIPUR",
		"KARACHI",
		"DHAKA",
		"LUCKNOW",
		"JODHPUR",
		"PUNE",
		"BHOPAL",
		"INDORE",
		"KANPUR",
		"NAGPUR",
		"PESHAWAR",
		"MULTAN",
		"SURAT",
		"COCHIN",
		"CUTTACK",
		"BHUBANESHWAR",
		"TAMRALIPTA",
		"CHITTAGONG",
		"GUWAHATI",
		"VIJAYAWADA",
		"MYSORE",
		"SRIRANGAPATNA",
		"AMRITSAR",
		"SRINAGAR",
		"REWALSAR",
		"VAISHALI",
		"CAMBAY",
		"ANHILWARA",
		"PAHARPUR",
		"MAHASTHANGARH",
		"GANGAI_KONDA_CHOLAPURAM",
		"KANCHI",
		"NAGAPATTINAM",
		"KAVERIPATTINAM",
		"KALYANI",
		"DHARWAD",
		"SALEM",
		"COIMBATORE",
		"MASULIPATNAM",
		"BIDAR",
		"GOLCONDA",
		"ELLICHPUR",
		"WARANGAL",
		"HALEBID",
		"PRATISHTHANA",
		"BIJAPUR",
		"SOLAPUR",
		"CALICUT",
		"BHARUCH",
		"MANGALORE",
		"DHAR",
		"AMBER",
		"CHITTOR",
		"VRINDAVAN",
		"BADORA",
		"KALPI",
		"BELUR",
		"NAINITAL",
		"JABALPUR",
		"RANCHI",
		"RAIPUR",
		"SONEPUR",
		"BOGRA",
		"DARJEELING",
		"IMPHAL",
		"CHANDIGARH",
	},	
	{	"KYOTO",
		"OSAKA",
		"TOKYO",
		"SATSUMA",
		"KAGOSHIMA",
		"NARA",
		"NAGOYA",
		"IZUMO",
		"NAGASAKI",
		"YOKOHAMA",
		"SHIMONOSEKI",
		"MATSUYAMA",
		"SAPPORO",
		"HAKODATE",
		"ISE",
		"TOYAMA",
		"FUKUSHIMA",
		"SUO",
		"BIZEN",
		"ECHIZEN",
		"IZUMI",
		"OMI",
		"ECHIGO",
		"KOZUKE",
		"SADO",
		"KOBE",
		"NAGANO",
		"HIROSHIMA",
		"TAKAYAMA",
		"AKITA",
		"FUKUOKA",
		"AOMORI",
		"KAMAKURO",
		"KOCHI",
		"NAHA",
	},	
	{	"YASODHARAPURA",
		"HARIHARALAYA",
		"ANGKOR_THOM",
		//"ANGKOR_WAT", // That's in Yasodharapura
		"NAGARA_JAYASRI",
		"RAJAVIHARA",
		"ISVARAPURA",
		"BANTEAY_KDEI",
		"JAYENDRANAGARI",
		"PRE_RUP",
		"VIMAYAPURA",
		"LINGAPURA",
		"KRONG_CHAKTOMUK",
		"CHOK_GARGYAR",
		"MAHENDRAPARVATA",
		"AMARENDRAPURA",
		"INDRAPURA",
		"LAVO",
		"PREAH_VIHEAR",
		"PHANOM_RUNG",
		"PHIMAI",
		"MUANG_TAM",
		"BENG_MELEA",
		"PRASAT_BAKAN",
		"SINGHAPURA",
		"CHALIENG",
		"SUKHOTHAI",
		"VIANGCHAN",
		"BATTAMBANG",
		"SURYAPARVATA",
		"PREY_NOKOR",
		"O_KEO",
		"ANINDITAPURA",
		"ISANAPURA",
		"SHRESTAPURA",
		"ANGKOR_BOREI",
		"VYADHAPURA",
		"BHAVAPURA",
		"SAMBHUPURA",
		"LOVEK",
		"UDONG",
	},	
	{	"TIMBUKTU",
		"DJENNE",
		"KUMBISALEH",
		"GAO",
		"WALATA",
		"NIANI",
		"AWDAGHOST",
		"TADMEKKA",
		"TEKEDDA",
		"AWLIL",
		"WADAN",
		"AGADES",
		"TEODENI",
		"TAGHAZA",
		"KATSINA",
		"KANO",
		"ZARIA",
		"BILMA",
		"NGAZARGUMU",
		"KANGABA",
		"DIA",
		"TIRAQQA",
		"AWKAR",
		"KUKIYA",
		"TAKRUR",
		"ARAWAN",
		"SYA",
		"TAOUDENNI",
		"BAMBOUK",
		"KIRINA",
	},	
	{	"MUTAL",
		"LAKAMHA",
		"CHICHEN_ITZA",
		"UXMAL",
		"MAYAPAN",
		"CALAKMUL",
		"OXHUITZA",
		"XUKPI",
		"QUIRIGUA",
		"IZANCANAC",
		"YOKIB",
		"ALTUN_HA",
		"IZAMAL",
		"ZAMA",
		"COBA",
		"DZIBILCHALTUN",
		"COZUMEL",
		"EDZNA",
		"OXKINTOK",
		"BECAN",
		"NAKBE",
		"CIVAL",
		"LAMANAIN",
		"KAMINALJUYU",
		"TAKALIK_ABAJ",
		"HOLMUL",
		"WAKA",
		"LUBAANTUM",
		"SIAAN_KAAN",
		"BONAMPAK",
		"TONINA",
		"TAZUMAL",
		"COMALCALCO",
		"NIM_LI_PUNIT",
		"EKBALAM",
		"SAYIL",
		"KABAH",
		"LABNA",
		"WAK_KABNAL",
		"XUNANTUNICH",
		"IZAPA",
		"NOJ_PETEN",
		"UTALTAN",
		"MANI",
	},	
	{	"KARAKORUM",
		"BESHBALIK",
		"TURFAN",
		"HSIA",
		"OLDSARAI",
		"NEWSARAI",
		"SAMARQAND",
		"TABRIZ",
		"TIFLIS",
		"OTRAR",
		"SANCHU",
		"KAZAN",
		"ALMARIKH",
		"ULAANBAATAR",
		"HOVD",
		"DARHAN",
		"DALANDZADGAD",
		"MANDALGOVI",
		"CHOYBALSAN",
		"ERDENET",
		"TSETSERLEG",
		"BARUUNURT",
		"EREEN",
		"BATSHIREET",
		"CHOYR",
		"ULAANGOM",
		"TOSONTSENGEL",
		"ATLAY",
		"ULIASTAY",
		"BAYANHONGOR",
		"HARAYRAG",
		"NALAYH",
		"TES",
	},	
	{	"PERSEPOLIS",
		"PASARGADAE",
		"SUSA",
		"ECBATANA",
		"TARSUS",
		"GORDIUM",
		"BACTRA",
		"SARDIS",
		"ERGILI",
		"DARIUSHKABIR",
		"GHULAMAN",
		"ZOHAK",
		"ISTAKHR",
		"JINJAN",
		"BORAZJAN",
		"HERAT",
		"DAKYANUS",
		"BAMPUR",
		"TURENGTEPE",
		"REY",
		"SHIRAZ",
		"TUSHPA",
		"HASANLU",
		"GABAE",
		"MERV",
		"BEHISTUN",
		"KANDAHAR",
		"ALTINTEPE",
		"BUNYAN",
		"CHARSADDA",
		"URATYUBE",
		"DURAEUROPOS",
		"ALEPPO",
		"QATNA",
		"KABUL",
		"CAPISA",
		"KYRESKHATA",
		"MARAKANDA",
		"PESHAWAR",
		"VAN",
		"PTEIRA",
		"ARSHADA",
		"ARTAKOANA",
		"ASPABOTA",
		"AUTIYARA",
		"BAGASTANA",
		"BAXTRI",
		"DARMASA",
		"DAPHNAI",
		"DRAPSAKA",
		"EION",
		"GANDUTAVA",
		"GAUGAMELA",
		"HARMOZEIA",
		"EKATOMPYLOS",
		"IZATA",
		"KAMPADA",
		"KAPISA",
		"KARMANA",
		"KOUNAXA",
		"KUGANAKA",
		"NAUTAKA",
		"PAISHIYAUVADA",
		"PATIGRBANA",
		"PHRADA",
	},	
	{	"ROME",
		"ANTIUM",
		"CUMAE",
		"NEAPOLIS",
		"RAVENNA",
		"ARRETIUM",
		"MEDIOLANUM",
		"ARPINUM",
		"CIRCEI",
		"SETIA",
		"SATRICUM",
		"ARDEA",
		"OSTIA",
		"VELITRAE",
		"VIROCONIUM",
		"TARENTUM",
		"BRUNDISIUM",
		"CAESARAUGUSTA",
		"CAESAREA",
		"PALMYRA",
		"SIGNIA",
		"AQUILEIA",
		"CLUSIUM",
		"SUTRIUM",
		"CREMONA",
		"PLACENTIA",
		"HISPALIS",
		"ARTAXATA",
		"AURELIANORUM",
		"NICOPOLIS",
		"LONDINIUM",
		"EBURACUM",
		"GORDION",
		"AGRIPPINA",
		"LUGDUNUM",
		"VERONA",
		"CORFINIUM",
		"TREVERI",
		"SIRMIUM",
		"AUGUSTADORUM",
		"BAGACUM",
		"LAURIACUM",
		"TEURNIA",
		"CURIA",
		"FREGELLAE",
		"ALBAFUCENS",
		"SORA",
		"INTERRAMA",
		"SUESSA",
		"SATICULA",
		"LUCERIA",
		"ARMINIUM",
		"SENAGALLICA",
		"CASTRUMNOVUM",
		"HADRIA",
	},	
	{	"MOSCOW",
		"STPETERSBURG",
		"NOVGOROD",
		"ROSTOV",
		"YAROSLAVI",
		"YEKATERINBURG",
		"YAKUTSK",
		"VLADIVOSTOK",
		"SMOLENSK",
		"ORENBURG",
		"KRASNOYARSK",
		"KHABAROVSK",
		"BRYANSK",
		"TVER",
		"NOVOSIBIRSK",
		"MAGADAN",
		"MURMANSK",
		"IRKUSTK",
		"CHITA",
		"SAMARA",
		"ARKHANGELSK",
		"CHELYABINSK",
		"TOBOLSK",
		"VOLOGDA",
		"OMSK",
		"ASTRAKHAN",
		"KURSK",
		"SARATOV",
		"TULA",
		"VLADIMIR",
		"PERM",
		"VORONEZH",
		"PSKOV",
		"STARAYARUSSA",
		"KOSTROMA",
		"NIZHNIYNOVGOROD",
		"SUZDAL",
		"MAGNITOGORSK",
	},	
	{	"ULUNDI",
		"UMGUNGUNDLOVU",
		"NOBAMBA",
		"BULAWAYO",
		"KWADUKUZA",
		"NONGOMA",
		"ONDINI",
		"NODWENGU",
		"NDONDAKUSUKA",
		"BABANANGO",
		"KHANGELA",
		"KWAHLOMENDLINI",
		"HLOBANE",
		"ETHEKWINI",
		"MLAMBONGWENYA",
		"EZIQWAQWENI",
		"EMANGWENI",
		"ISIPHEZI",
		"MASOTSHENI",
		"MTUNZINI",
		"NYAKAMUBI",
		"DUMAZULU",
		"HLATIKULU",
		"MTHONJANENI",
		"EMPANGENI",
		"PONGOLA",
		"TUNGELA",
		"KWAMASHU",
		"INGWAVUMA",
		"HLUHLUWE",
		"MTUBATUBA",
		"MHLAHLANDLELA",
		"MTHATHA",
		"MASERU",
		"LOBAMBA",
		"QUNU",
	},
	{	"BIBRACTE",
		"VIENNE",
		"TOLOSA",
		"GERGOVIA",
		"CAMULODUNUM",
		"VERLAMION",
		"DURNOVARIA",
		"ISCA",
		"DUROCORTORUM",
		"BAGACUM",
		"NEMETOCENNA",
		"CALLEVA",
		"VENTA",
		"ISURIUM",
		"RATAE",
		"LUTETIA",
		"ENTREMONT",
		"BURDIGALA",
		"NEMAUSUS",
		"SAMAROBRIVA",
		"BRIXIA",
		"MEDIOLANUM",
		"CREMONA",
		"TAURASIA",
		"SINIGAGLIA",
		"NOVIOMAGUS",
		"DUROVERNUM",
		"LUGUVALIO",
		"LINDINIS",
		"OKILIS",
		"NUMANTIA",
		"TIERNES",
		"BOTORRITA",
		"MORIDUNUM",
		"TARA",
		"EBLANA",
		"ALESIA",
		"ANDEMATUNUM",
		"UBIORUM",
		"GENEVA",
		"VINDOBONA",
		"PISONIUM",
		"ACQUINCUM",
		"TYLIS",
		"ANCYRA",
	}
	};
}
