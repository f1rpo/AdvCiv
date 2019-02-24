#include "CvGameCoreDLL.h"
#include "CvHallOfFameInfo.h"
#include "CvGlobals.h"

CvHallOfFameInfo::CvHallOfFameInfo()
{
	GC.getGameINLINE().setHallOfFame(this); // advc.106i
	m_bUninit = false; // advc.tmp
}

CvHallOfFameInfo::~CvHallOfFameInfo()
{	// <advc.106i>
	uninit();
}

void CvHallOfFameInfo::uninit(CvReplayInfo const* pReplay) {
	// <advc.tmp>
	if(m_bUninit) // (Indirect) recursive call
		return;
	m_bUninit = true;
	if(pReplay == NULL) { // </advc.tmp>
		GC.getGameINLINE().setHallOfFame(NULL);
		GC.setHoFScreenUp(false);
	}
	for(size_t i = 0; i < m_aReplays.size(); i++) {
		// <advc.tmp>
		if(pReplay != NULL) {
			if(m_aReplays[i] == pReplay) {
				FAssertMsg(false, "Shouldn't have to remove individual replays");
				m_aReplays[i] = NULL; // Caller is deleting it
				return;
			}
		} // </advc.tmp>
		else SAFE_DELETE(m_aReplays[i]);
	}
	if(pReplay == NULL) // advc.tmp
		m_aReplays.clear();
	m_bUninit = false; // advc.tmp
} // </advc.106i>

void CvHallOfFameInfo::loadReplays()
{
	GC.setHoFScreenUp(true); // advc.106i
	gDLL->loadReplays(m_aReplays);
}

int CvHallOfFameInfo::getNumGames() const
{
	return (int)m_aReplays.size();
}

CvReplayInfo* CvHallOfFameInfo::getReplayInfo(int i)
{
	return m_aReplays[i];
}