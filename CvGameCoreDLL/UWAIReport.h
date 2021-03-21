#pragma once

#ifndef UWAI_REPORT_H
#define UWAI_REPORT_H

/* advc.104: New class. A report about the war planning of a single team
   (cf. UWAI::Team::doWar). I'm also focusing string handling
   needed for logging in this class.
   Calls to 'log' have no effect when in silent or mute mode. DEBUG mode is not a
   prerequisite for logging output though.
   Each 'log' call is written to the log file separately (flushed). */
class UWAIReport {

public:

	/* Instead of preceding each 'log' call with a check, the client object
	   should (once) create a report in any case, but set it to be 'silent'.
	   setMute has no effect on a silent report. */
	explicit UWAIReport(bool silent = false);
	~UWAIReport();
	// Only actually logs when not set to silent or mute
	void log(char const* fmt, ...);
	/* leaderName and the next four functions use manually (de-)allocated
	   memory in order to narrow wstrings. Therefore not const.
	   All the functions returning char const* return an empty string when
	   in silent mode (for performance rasons). */
	char const* leaderName(PlayerTypes civId, int charLimit = 8);
	// Leader name if the master team has only 1 member, otherwise team name
	char const* masterName(TeamTypes masterId, int charLimit = 8);
	char const* teamName(TeamTypes teamId);
	char const* techName(TechTypes teamId, int charLimit = 8);
	char const* unitName(CvInfoBase const& unitInfo, int charLimit = 8);
	char const* cityName(CvCity const& c, int charLimit = 12);
	char const* warPlanName(WarPlanTypes wp) const;
	/* A string of '>' characters to indicate nesting through
	   indentation. */
	char const* prefix(int level);
	void setMute(bool b);
	// True if muted or if silent to begin with
	bool isMute() const { return (muted > 0); }
	void setSilent(bool b);

private:

	void writeToFile();
	void deleteBuffer();
	char const* narrow(const wchar* ws, int charLimit);

	CvString report;
	bool silent;
	int muted;
	std::vector<std::string*> stringBuffer;
};

#endif
