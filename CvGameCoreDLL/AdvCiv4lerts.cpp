// <advc.210> New classes; see header file for description

#include "CvGameCoreDLL.h"
#include "AdvCiv4lerts.h"
#include "CvDLLInterfaceIFaceBase.h"

using std::set;

AdvCiv4lert::AdvCiv4lert() {

	isSilent = false;
}

void AdvCiv4lert::init(PlayerTypes ownerId) {

	this->ownerId = ownerId;
	reset();
}

void AdvCiv4lert::msg(CvWString s, LPCSTR icon, int x, int y, int goodOrBad) const {

	if(isSilent)
		return;
	CvGame& g = GC.getGame();
	if(g.isOption(GAMEOPTION_RISE_FALL) && g.getRiseFall().isBlockPopups())
		return;
	int color = GC.getInfoTypeForString("COLOR_WHITE");
	if(goodOrBad > 0)
		color = GC.getInfoTypeForString("COLOR_GREEN");
	else if(goodOrBad < 0)
		color = GC.getInfoTypeForString("COLOR_RED");
	bool arrows = (icon != NULL);
	gDLL->getInterfaceIFace()->addHumanMessage(ownerId, true,
			GC.getEVENT_MESSAGE_TIME(), s, NULL, MESSAGE_TYPE_INFO, icon,
			(ColorTypes)color, x, y, arrows, arrows);
}

void AdvCiv4lert::check(bool silent) {

	if(silent)
		isSilent = true;
	check();
	if(silent)
		isSilent = false;
}

void AdvCiv4lert::reset() {}

// <advc.210a>
WarTradeAlert::WarTradeAlert() : AdvCiv4lert() {}

void WarTradeAlert::reset() {

	for(int i = 0; i < MAX_CIV_TEAMS; i++)
		for(int j = 0; j < MAX_CIV_TEAMS; j++)
			willWar[i][j] = false;
}

void WarTradeAlert::check() {

	CvPlayer const& owner = GET_PLAYER(ownerId);
	for(int i = 0; i < MAX_CIV_TEAMS; i++) {
		CvTeamAI const& warTeam = GET_TEAM((TeamTypes)i);
		bool valid = (warTeam.isAlive() && !warTeam.isAVassal() &&
				!warTeam.isMinorCiv() && !warTeam.isHuman() && 
				warTeam.getID() != owner.getTeam() &&
				!warTeam.isAtWar(owner.getTeam()) &&
				owner.canContactAndTalk(warTeam.getLeaderID()));
		for(int j = 0; j < MAX_CIV_TEAMS; j++) {
			CvTeam const& victim = GET_TEAM((TeamTypes)j);
			bool newValue = (valid && victim.isAlive() && !victim.isAVassal() &&
					!victim.isMinorCiv() && victim.getID() != owner.getTeam() &&
					victim.getID() != warTeam.getID() &&
					!warTeam.isAtWar(victim.getID()) &&
					GET_TEAM(owner.getTeam()).isHasMet(victim.getID()) &&
					// Can't suggest war trade otherwise
					warTeam.isOpenBordersTrading() &&
					warTeam.AI_declareWarTrade(victim.getID(), owner.getTeam()) ==
					NO_DENIAL);
			if(newValue == willWar[i][j])
				continue;
			willWar[i][j] = newValue;
			if(newValue)
				msg(gDLL->getText("TXT_KEY_CIV4LERTS_TRADE_WAR",
						warTeam.getName().GetCString(),
						victim.getName().GetCString()));
			/*  Obviously can't hire warTeam if it has already declared war
				or if victim has been eliminated. */
			else if(victim.isAlive() && !warTeam.isAtWar(victim.getID()))
				msg(gDLL->getText("TXT_KEY_CIV4LERTS_NO_LONGER_TRADE_WAR",
						warTeam.getName().GetCString(),
						victim.getName().GetCString()));
		}
	}
} // </advc.210a>

// <advc.210b>
RevoltAlert::RevoltAlert() : AdvCiv4lert() {}

void RevoltAlert::reset() {

	revoltPossible.clear();
	occupation.clear();
}

void RevoltAlert::check() {

	set<int> updatedRevolt;
	set<int> updatedOccupation;
	CvPlayer const& owner = GET_PLAYER(ownerId); int dummy;
	for(CvCity* c = owner.firstCity(&dummy); c != NULL; c = owner.nextCity(&dummy)) {
		bool couldPreviouslyRevolt = revoltPossible.count(c->plotNum()) > 0;
		bool wasOccupation = occupation.count(c->plotNum()) > 0;
		double pr = c->revoltProbability();
		if(pr > 0) {
			updatedRevolt.insert(c->plotNum());
			/*  Report only change in revolt chance OR change in occupation status;
				the latter takes precedence. */
			if(!couldPreviouslyRevolt && wasOccupation == c->isOccupation()) {
				// Looks precarious; copied this from CvDLLWidgetData::parseNationalityHelp
				wchar szTempBuffer[1024];
				swprintf(szTempBuffer, L"%.2f", (float)(100 * pr));
				msg(gDLL->getText("TXT_KEY_CIV4LERTS_REVOLT", c->getName().
						GetCString(), szTempBuffer),
						NULL // icon works, but is too distracting
						,//ARTFILEMGR.getInterfaceArtInfo("INTERFACE_RESISTANCE")->getPath(),
						c->getX_INLINE(), c->getY_INLINE(),
						// red text (-1) also too distracting
						0);
			}
		}
		else if(couldPreviouslyRevolt && wasOccupation == c->isOccupation()) {
			msg(gDLL->getText("TXT_KEY_CIV4LERTS_NO_LONGER_REVOLT", c->getName().
						GetCString()), NULL
						,//ARTFILEMGR.getInterfaceArtInfo("INTERFACE_RESISTANCE")->getPath(),
						c->getX_INLINE(), c->getY_INLINE(),
						1); // Important and rare enough to be shown in green
		}
		if(c->isOccupation())
			updatedOccupation.insert(c->plotNum());
		/*  If there's no order queued, the city will come to the player's attention
			anyway when it asks for orders. */
		else if(wasOccupation && c->getNumOrdersQueued() > 0) {
			msg(gDLL->getText("TXT_KEY_CIV4LERTS_CITY_PACIFIED_ADVC", c->getName().
						GetCString()), NULL
						,//ARTFILEMGR.getInterfaceArtInfo("INTERFACE_RESISTANCE")->getPath(),
						c->getX_INLINE(), c->getY_INLINE(),
						0);
			/*  Pretend that revolt chance is 0 after occupation ends, so that
				a spearate alert is fired on the next turn if it's actually not 0. */
			updatedRevolt.erase(c->plotNum());
		}
	}
	revoltPossible.clear();
	revoltPossible.insert(updatedRevolt.begin(), updatedRevolt.end());
	occupation.clear();
	occupation.insert(updatedOccupation.begin(), updatedOccupation.end());
} // </advc.210b>

// </advc.210>
