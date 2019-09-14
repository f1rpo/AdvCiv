#include "CvGameCoreDLL.h"
#include "BBAILog.h"
// <advc.133>
#include "CvGameTextMgr.h"
#include "CvGamePlay.h" // </advc.133>

// AI decision making logging

void logBBAI(TCHAR* format, ... )
{
#ifdef LOG_AI
	static char buf[2048];
	va_list args;
	va_start(args, format);
	_vsnprintf(buf, 2048-4, format, args);
	va_end(args); // kmodx
	gDLL->logMsg("BBAI.log", buf);
#endif
}

// <advc.133>
void logBBAICancel(CvDeal const& d, PlayerTypes eCancelPlayer, wchar const* szReason)
{
	CvWStringBuffer szTmpBuffer;
	GAMETEXT.getDealString(szTmpBuffer, d, eCancelPlayer, false);
	CvWString szBuffer;
	szBuffer.Format(L"    %s cancels deal (%s): %s", GET_PLAYER(eCancelPlayer).getName(0),
			szReason, szTmpBuffer.getCString());
	// Leave it to logBBAI to narrow the string
	logBBAI("%S", szBuffer.GetCString());
} // </advc.133>
