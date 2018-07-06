// <advc.210> New classes; see header file for description

#include "CvGameCoreDLL.h"
#include "AdvCiv4lerts.h"
#include "CvDLLInterfaceIFaceBase.h"

#include <iterator>

using std::set;
using std::vector;
using std::multiset;
using std::inserter;
using std::set_difference;

AdvCiv4lert::AdvCiv4lert() {

	isSilent = false;
	/*  Set this to true in a subclass constructor in order to test or debug a
		particular alert through AI Auto Play */
	isDebug = false;
}

void AdvCiv4lert::init(PlayerTypes ownerId) {

	this->ownerId = ownerId;
	reset();
}

void AdvCiv4lert::msg(CvWString s, LPCSTR icon, int x, int y, int goodOrBad) const {

	if(isSilent)
		return;
	// <advc.127>
	bool autoPlayJustEnded = GET_PLAYER(ownerId).isAutoPlayJustEnded();
	bool force = (isDebug && (GET_PLAYER(ownerId).isSpectator() ||
			/*  When Auto Play has just ended, we're no longer in spectator mode,
				but the message should still be force-delivered. */
			(autoPlayJustEnded && GC.getGameINLINE().isDebugMode())));
	if(!force && (GET_PLAYER(ownerId).isHumanDisabled() || autoPlayJustEnded))
		return; // </advc.127>
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
			GC.getEVENT_MESSAGE_TIME(), s, NULL,
			force ? MESSAGE_TYPE_MAJOR_EVENT : MESSAGE_TYPE_INFO, // advc.127
			icon, (ColorTypes)color, x, y, arrows, arrows);
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

	if(isSilent) {
		/*  Somehow WarAndPeaceAI isn't always up to date when alerts are
			checked right after loading a savegame. The calls after loading
			are silent calls; all regular calls are non-silent. */
		getWPAI.update();
	}
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
			if(newValue) {
				msg(gDLL->getText("TXT_KEY_CIV4LERTS_TRADE_WAR",
						warTeam.getName().GetCString(),
						victim.getName().GetCString()),
						// advc.127b:
						NULL, warTeam.getCapitalX(), warTeam.getCapitalY());
			}
			/*  Obviously can't hire warTeam if it has already declared war
				or if victim has been eliminated. */
			else if(victim.isAlive() && !warTeam.isAtWar(victim.getID())) {
				msg(gDLL->getText("TXT_KEY_CIV4LERTS_NO_LONGER_TRADE_WAR",
						warTeam.getName().GetCString(),
						victim.getName().GetCString()),
						// advc.127b:
						NULL, warTeam.getCapitalX(), warTeam.getCapitalY());
			}
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
		else if(couldPreviouslyRevolt && wasOccupation == c->isOccupation() &&
				/*  Don't report 0 revolt chance when in occupation b/c
					revolt chance will increase a bit when occupation ends. */
				!c->isOccupation()) {
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

// <advc.210d>
BonusThirdPartiesAlert::BonusThirdPartiesAlert() : AdvCiv4lert() {

	isDebug = false;
}

void BonusThirdPartiesAlert::reset() {

	for(int i = 0; i < MAX_CIV_PLAYERS; i++)
		exportDeals[i].clear();
}

void BonusThirdPartiesAlert::check() {

	multiset<int> updatedDeals[MAX_CIV_PLAYERS];
	CvGame& g = GC.getGame(); int dummy=-1;
	for(CvDeal* d = g.firstDeal(&dummy); d != NULL; d = g.nextDeal(&dummy)) {
		// This alert ignores trades of ownerId
		if(d->getFirstPlayer() == ownerId || d->getSecondPlayer() == ownerId)
			continue;
		vector<int> dealData;
		getExportData(d->getFirstTrades(), d->getSecondPlayer(), dealData);
		for(size_t i = 0; i < dealData.size(); i++)
			updatedDeals[d->getFirstPlayer()].insert(dealData[i]);
		dealData.clear();
		getExportData(d->getSecondTrades(), d->getFirstPlayer(), dealData);
		for(size_t i = 0; i < dealData.size(); i++)
			updatedDeals[d->getSecondPlayer()].insert(dealData[i]);
	}
	for(int i = 0; i < MAX_CIV_PLAYERS; i++) {
		if(i == ownerId)
			continue;
		PlayerTypes fromId = (PlayerTypes)i;
		if(!GET_PLAYER(fromId).isAlive())
			continue;
		vector<int> newDeals;
		set_difference(updatedDeals[i].begin(), updatedDeals[i].end(),
				exportDeals[i].begin(), exportDeals[i].end(), 
				inserter(newDeals, newDeals.begin()));
		vector<int> missingDeals;
		set_difference(exportDeals[i].begin(), exportDeals[i].end(),
				updatedDeals[i].begin(), updatedDeals[i].end(), 
				inserter(missingDeals, missingDeals.begin()));
		for(size_t j = 0; j < newDeals.size(); j++) {
			int newCount = updatedDeals[i].count(newDeals[j]);
			int oldCount = exportDeals[i].count(newDeals[j]);
			FAssert(newCount > oldCount);
			doMsg(fromId, newDeals[j], newCount, oldCount);
		}
		for(size_t j = 0; j < missingDeals.size(); j++) {
			int newCount = updatedDeals[i].count(missingDeals[j]);
			int oldCount = exportDeals[i].count(missingDeals[j]);
			FAssert(newCount < oldCount);
			doMsg(fromId, missingDeals[j], newCount, oldCount);
		}
		exportDeals[i] = updatedDeals[i];
	}
}

void BonusThirdPartiesAlert::getExportData(CLinkList<TradeData> const* list,
		PlayerTypes toId, std::vector<int>& r) const {

	if(list == NULL)
		return;
	CLinkList<TradeData> const& li = *list;
	for(CLLNode<TradeData>* node = li.head(); node != NULL; node = li.next(node)) {
		TradeData tdata = node->m_data;
		if(tdata.m_eItemType == TRADE_RESOURCES)
			r.push_back(GC.getNumBonusInfos() * toId + tdata.m_iData);
	}
}

void BonusThirdPartiesAlert::doMsg(PlayerTypes fromId, int data,
		int newQuantity, int oldQuantity) {

	BonusTypes bonusId = (BonusTypes)(data % GC.getNumBonusInfos());
	PlayerTypes toId = (PlayerTypes)((data - bonusId) / GC.getNumBonusInfos());
	CvPlayerAI const& from = GET_PLAYER(fromId);
	CvPlayerAI const& to = GET_PLAYER(toId);
	// Don't report ended trade when the reason is obvious
	if(!from.isAlive() || !to.isAlive() || GET_TEAM(from.getTeam()).isAtWar(to.getTeam()) ||
			from.AI_getMemoryCount(toId, MEMORY_STOPPED_TRADING_RECENT) > 0 ||
			to.AI_getMemoryCount(fromId, MEMORY_STOPPED_TRADING_RECENT) > 0)
		return;
	// Don't report unseen trades
	if(!GET_PLAYER(ownerId).isSpectator() && // advc.127
			(!TEAMREF(ownerId).isHasMet(from.getTeam()) ||
			!TEAMREF(ownerId).isHasMet(to.getTeam())))
		return;
	int bonusChar = GC.getBonusInfo(bonusId).getChar();
	CvWString msgStr;
	CvWString quantityStr;
	if(!isDebug || newQuantity == 0 || oldQuantity == 0)
		quantityStr = gDLL->getText("TXT_KEY_CIV4LERTS_BONUS_ICON", bonusChar);
	else {
		quantityStr = (newQuantity > oldQuantity ?
				/*  The difference should practically always be 1; if it's more,
					it's still not incorrect to claim that one more resource is
					being traded. */
				gDLL->getText("TXT_KEY_CIV4LERTS_ONE_MORE", bonusChar) :
				gDLL->getText("TXT_KEY_CIV4LERTS_ONE_FEWER", bonusChar));
	}
	if(isDebug) {
		msgStr = (newQuantity > 0 ?
				gDLL->getText("TXT_KEY_CIV4LERTS_NOW_EXPORTING",
				from.getNameKey(), quantityStr.GetCString(), to.getNameKey()) :
				gDLL->getText("TXT_KEY_CIV4LERTS_NO_LONGER_EXPORTING",
				from.getNameKey(), quantityStr.GetCString(), to.getNameKey()));
	}
	else {
		if((newQuantity > 0) == (oldQuantity > 0))
			return;
		CvBonusInfo& bi = GC.getBonusInfo(bonusId);
		bool strategic = (bi.getHappiness() + bi.getHealth() <= 0);
		if(!strategic) { // Don't bother with buildings (only need to cover Ivory)
			for(int i = 0; i < GC.getNumUnitClassInfos(); i++) {
				UnitTypes ut = (UnitTypes)(GC.getCivilizationInfo(to.
						getCivilizationType()).getCivilizationUnits(i));
				if(ut == NO_UNIT)
					continue;
				CvUnitInfo& ui = GC.getUnitInfo(ut);
				if(ui.getPrereqAndBonus() == bonusId) {
					// Only report Ivory while it's relevant
					TechTypes tt = (TechTypes)ui.getPrereqAndTech();
					if(tt != NO_TECH && to.getCurrentEra() -
							GC.getTechInfo(tt).getEra() < 2)
						strategic = true;
				}
			}
		}
		if(strategic) {
			msgStr = gDLL->getText(newQuantity > 0 ?
					"TXT_KEY_CIV4LERTS_EXPORTING_STRATEGIC" :
					"TXT_KEY_CIV4LERTS_NOT_EXPORTING_STRATEGIC",
					from.getNameKey(), to.getNameKey(), quantityStr.GetCString());
		}
		else {
			int imports = to.getNumTradeBonusImports(fromId);
			FAssert(imports >= newQuantity);
			if(newQuantity < imports)
				return;
			msgStr = gDLL->getText((newQuantity > 0 ?
					"TXT_KEY_CIV4LERTS_EXPORTING_ANY" :
					"TXT_KEY_CIV4LERTS_EXPORTING_NONE"),
					from.getNameKey(), to.getNameKey(), quantityStr.GetCString());
		}
	}
	msg(msgStr);
} // </advc.210d>

// </advc.210>