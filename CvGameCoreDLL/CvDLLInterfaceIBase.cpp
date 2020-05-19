/*  <advc.127> Cut and pasted from CvDLLInterfaceIFaceBase.h; this function
	is getting too large for an inline definition. */

#include "CvGameCoreDLL.h"
#include "CvDLLInterfaceIFaceBase.h" // </advc.127>
#include "CvGame.h"
#include "CvPlayer.h"
#include "CvPlot.h"
#include "RiseFall.h" // advc.700

void CvDLLInterfaceIFaceBase::addMessage(PlayerTypes ePlayer, bool bForce,
	int iLength, CvWString szString, LPCTSTR pszSound,
	InterfaceMessageTypes eType, LPCSTR pszIcon, ColorTypes eFlashColor,
	int iFlashX, int iFlashY, bool bShowOffScreenArrows,
	bool bShowOnScreenArrows)
{
	// <advc>
	if (iLength == -1)
		iLength = GC.getEVENT_MESSAGE_TIME();
	// Perhaps the EXE does that anyway; let's make sure.
	if (eFlashColor == NO_COLOR)
		eFlashColor = GC.getColorType("WHITE"); // </advc>
	CvPlayer& kPlayer = GET_PLAYER(ePlayer);
	if(kPlayer.isHuman() ||
		/*  <advc.700> Want message archive to be available when human
			takes over. AI messages expire just like human messages. */
		(GC.getGame().isOption(GAMEOPTION_RISE_FALL) &&
		!kPlayer.isHumanDisabled() &&
		GC.getGame().getRiseFall().isDeliverMessages(ePlayer)))
	{	// </advc.700>
		addMessageExternal(ePlayer, bForce
				&& !gDLL->getEngineIFace()->isGlobeviewUp(), // advc.106
				iLength, szString, pszSound, eType, pszIcon, eFlashColor, iFlashX, iFlashY,
				bShowOffScreenArrows, bShowOnScreenArrows);
	}
	//else if (GC.getGame().getActivePlayer() == ePlayer)
	// advc.700: Replacing the above
	else if(!kPlayer.isHuman() && kPlayer.isHumanDisabled())
	{
		// this means ePlayer is human, but currently using auto-play (K-Mod)
		if(eType == MESSAGE_TYPE_MAJOR_EVENT || eType == MESSAGE_TYPE_CHAT ||
				eType == MESSAGE_TYPE_MAJOR_EVENT_LOG_ONLY) // advc.106b
		{
			addMessageExternal(ePlayer,
				/*  advc.127: bForce=true causes the event to be announced immediately,
					whereas bForce=false delays the announcement until the start of the next turn.
					bForce=true should make Auto Play easier to follow.
					Also set pszSound to NULL; no sounds during Auto Play. */
					true, iLength, szString, NULL, eType, NULL,
					//NO_COLOR, -1, -1, // (K-Mod)
					/*  advc.127: Don't want stuff to flash during Auto Play,
						but pszIcon=NULL is enough to prevent this. No need to
						discard the color and coordinates. */
					eFlashColor, iFlashX, iFlashY,
					false, false);
		}
	}
}

void CvDLLInterfaceIFaceBase::addMessage(PlayerTypes ePlayer, bool bForce,
	int iLength, CvWString szString, CvPlot const& kPlot,
	LPCTSTR pszSound, InterfaceMessageTypes eType, LPCSTR pszIcon,
	ColorTypes eFlashColor, bool bShowOffScreenArrows, bool bShowOnScreenArrows)
{
	addMessage(ePlayer, bForce, iLength, szString, pszSound, eType, pszIcon,
			eFlashColor, kPlot.getX(), kPlot.getY(),
			bShowOffScreenArrows, bShowOnScreenArrows);
}
