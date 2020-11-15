// advc.104: New class; see UWAIReport.h for description.

#include "CvGameCoreDLL.h"
#include "UWAIReport.h"
#include "CvGamePlay.h"
#include "CvCity.h"

using std::ostringstream;
using std::string;

UWAIReport::UWAIReport(bool silent) { // default : false

	/*  The log could be used to cheat in multiplayer. It's OK if MessageLog
		is enabled; the game will warn the other player about that. */
	if (GC.getGame().isNetworkMultiPlayer() && !GC.isLogging())
		silent = true;
	setSilent(silent);
}

UWAIReport::~UWAIReport() {

	if(!report.IsEmpty()) {
		log("_This logfile is formatted in Textile. Paste its content "
			"into the web converter on "
			"http://borgar.github.io/textile-js/#text_preview"
			" for HTML output that is easier to read. "
			"(You can download the web converter by saving the web page.)_");
		writeToFile();
	}
	deleteBuffer();
}

void UWAIReport::log(char const* fmt, ...) {

	if(muted > 0)
		return;
	va_list args;
	va_start(args, fmt);
	report += CvString::formatv(fmt, args);
	va_end(args);
	report += CvString::format("\n");
	writeToFile();
}

void UWAIReport::writeToFile() {

	if(muted > 0)
		return;
	CvGame const& g = GC.getGame();
	ostringstream logFileName;
	//if(g.isNetworkMultiPlayer()) // For OOS debugging on a single PC
		//logFileName << (int)g.getActivePlayer() << "_";
	logFileName << "uwai" << g.getGameTurn() << ".log";
	gDLL->logMsg(logFileName.str().c_str(), report, false, false);
	report.clear();
}

void UWAIReport::deleteBuffer() {

	for(size_t i = 0; i < stringBuffer.size(); i++)
		delete stringBuffer[i];
}

char const* UWAIReport::leaderName(PlayerTypes civId, int charLimit) {

	if(muted > 0)
		return "";// default: 8
	CvLeaderHeadInfo& leader = GC.getInfo(
			GET_PLAYER(civId).getLeaderType());
	return narrow(leader.getDescription(), charLimit);
}

char const* UWAIReport::unitName(CvInfoBase const& unitInfo, int charLimit) {

	if(muted > 0)
		return "";
	return narrow(unitInfo.getDescription(), charLimit);
}

char const* UWAIReport::cityName(CvCity const& c, int charLimit) {

	if(muted > 0)
		return "";
	return narrow(c.getName(), charLimit);
}

char const* UWAIReport::narrow(const wchar* ws, int charLimit) {

	CvString cvs(ws);
	/*	I'm sure the narrowing procedure is needlessly complicated,
		but I can't sort it out.
		Upd.: Should arguably return a CvString - which can be constructed from a
		CvWString. */
	string s = cvs.substr(0, charLimit);
	/*	Trailing (or leading for that matter) spaces in names of units, leaders
		etc. mess up the Textile formatting. Such spaces don't normally occur,
		but char limit can leave a space at the end. */
	while(s[s.length() - 1] == ' ')
		s = s.substr(0, s.length() - 1);
	char const* cstr = s.c_str();
	string* toBeDeleted = new string(cstr);
	stringBuffer.push_back(toBeDeleted);
	return toBeDeleted->c_str();
}

char const* UWAIReport::masterName(TeamTypes masterId, int charLimit) {
															// default: 8
	if(muted > 0)
		return "";
	CvTeam& mt = GET_TEAM(masterId);
	if(mt.getNumMembers() > 1)
		return teamName(masterId);
	return leaderName(mt.getLeaderID());
}

char const* UWAIReport::teamName(TeamTypes teamId) {

	if(muted > 0)
		return "";
	ostringstream ss;
	/*  Textile turns this into a tooltip showing the team leader's name.
		Strangely, the tooltip only results from T1(...), T2(...), ...
		not from Team1(...), ... */
	ss << "T" << (int)teamId << "(" <<
			leaderName(GET_TEAM(teamId).getLeaderID()) << ")";
	string s = ss.str();
	char const* cstr = s.c_str();
	string* toBeDeleted = new string(cstr);
	stringBuffer.push_back(toBeDeleted);
	return toBeDeleted->c_str();
}

char const* UWAIReport::techName(TechTypes techId, int charLimit) {

	if(muted > 0)
		return "";
	return narrow(GC.getInfo(techId).getDescription(), charLimit);
}

char const* UWAIReport::warPlanName(WarPlanTypes wp) const {

	if(muted > 0)
		return "";
	switch(wp) {
	case NO_WARPLAN: return "none";
	case WARPLAN_ATTACKED_RECENT: return "attacked recent";
	case WARPLAN_ATTACKED: return "attacked";
	case WARPLAN_PREPARING_LIMITED: return "preparing limited";
	case WARPLAN_PREPARING_TOTAL: return "preparing total";
	case WARPLAN_LIMITED: return "limited";
	case WARPLAN_TOTAL: return "total";
	case WARPLAN_DOGPILE: return "dogpile";
	default: return "unrecognized";
	}
}

char const* UWAIReport::prefix(int level) {

	if(muted > 0)
		return "";
	string r = "";
	for(int i = 0; i < level; i++)
		r += ">";
	if(!r.empty())
		r += " ";
	char const* cstr = r.c_str();
	string* toBeDeleted = new string(cstr);
	stringBuffer.push_back(toBeDeleted);
	return toBeDeleted->c_str();
}

void UWAIReport::setMute(bool b) {

	if(silent)
		return;
	/*  This way, a subroutine can mute and unmute the report w/o checking
		if the report has already been muted. */
	if(b)
		muted++;
	else muted--;
}

void UWAIReport::setSilent(bool b) {

	silent = b;
	if(silent)
		muted = 1;
	else muted = 0;
}
