#ifndef CVHALLOFFAMEINFO_H
#define CVHALLOFFAMEINFO_H

#pragma once

#include "CvReplayInfo.h"

class CvHallOfFameInfo
{
public:
	CvHallOfFameInfo();
	virtual ~CvHallOfFameInfo();
	void uninit(/* advc.tmp: */ CvReplayInfo const* pReplay = NULL); // advc.106i

	void loadReplays();
	int getNumGames() const;
	CvReplayInfo* getReplayInfo(int i);

protected:
	std::vector<CvReplayInfo*> m_aReplays;
	bool m_bUninit; // advc.tmp
};

#endif