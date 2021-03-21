// advc.210: New classes; see header file for description

#include "CvGameCoreDLL.h"
#include "AdvCiv4lerts.h"
#include "CvInfo_Terrain.h"
#include "CoreAI.h"
#include "CvDeal.h"
#include "CvCity.h"
#include "RiseFall.h" // advc.706
#include "UWAIAgent.h" // advc.ctr

using std::set;
using std::vector;
using std::multiset;
using std::inserter;
using std::set_difference;

AdvCiv4lert::AdvCiv4lert(PlayerTypes eOwner) : ownerId(eOwner)
{
	isSilent = false;
	/*  Set this to true in a subclass constructor in order to test or debug a
		particular alert through AI Auto Play */
	isDebug = false;
}

void AdvCiv4lert::msg(CvWString s, LPCSTR icon, int x, int y, ColorTypes colorId) const
{
	if(isSilent)
		return;
	// <advc.127>
	bool autoPlayJustEnded = GET_PLAYER(ownerId).isAutoPlayJustEnded();
	CvGame& g = GC.getGame();
	bool force = (isDebug && (GET_PLAYER(ownerId).isSpectator() ||
			/*  When Auto Play has just ended, we're no longer in spectator mode,
				but the message should still be force-delivered. */
			(autoPlayJustEnded && g.isDebugMode())));
	if(!force && (GET_PLAYER(ownerId).isHumanDisabled() || autoPlayJustEnded))
		return; // </advc.127>
	// <advc.706>
	if(g.isOption(GAMEOPTION_RISE_FALL) && g.getRiseFall().isBlockPopups())
		return; // </advc.706>
	bool arrows = (icon != NULL);
	gDLL->UI().addMessage(ownerId, false, -1, s, NULL,
			force && isDebug ? MESSAGE_TYPE_MAJOR_EVENT : MESSAGE_TYPE_INFO, // advc.127
			icon, (ColorTypes)colorId, x, y, arrows, arrows);
}

void AdvCiv4lert::check(bool silent)
{
	if(ownerId == NO_PLAYER || (!isDebug && !GET_PLAYER(ownerId).isHuman()))
	{
		/*  Normally no need to check during Auto Play. Wouldn't hurt, except
			that the checks aren't super fast. */
		return;
	}
	if(silent)
		isSilent = true;
	check();
	if(silent)
		isSilent = false;
}

// <advc.210a>
void WarTradeAlert::check()
{
	CvPlayer const& owner = GET_PLAYER(ownerId);
	for(int i = 0; i < MAX_CIV_TEAMS; i++)
	{
		CvTeamAI const& warTeam = GET_TEAM((TeamTypes)i);
		bool valid = (warTeam.isAlive() && !warTeam.isAVassal() &&
				!warTeam.isMinorCiv() && !warTeam.isHuman() &&
				warTeam.getID() != owner.getTeam() &&
				!warTeam.isAtWar(owner.getTeam()) &&
				owner.canContact(warTeam.getLeaderID(), true));
		std::vector<TeamTypes> willTradeMsgTeams;
		std::vector<TeamTypes> noLongerTradeMsgTeams;
		bool nowTooManyWars = false; // Only relevant when UWAI is disabled
		for(int j = 0; j < MAX_CIV_TEAMS; j++)
		{
			CvTeam const& victim = GET_TEAM((TeamTypes)j);
			bool otherValid = (valid && victim.isAlive() && !victim.isAVassal() &&
					!victim.isMinorCiv() && victim.getID() != owner.getTeam() &&
					victim.getID() != warTeam.getID() &&
					!warTeam.isAtWar(victim.getID()) &&
					GET_TEAM(owner.getTeam()).isHasMet(victim.getID()) &&
					// Can't suggest war trade otherwise
					warTeam.isOpenBordersTrading());
			DenialTypes tradeDenial = (otherValid ? warTeam.AI_declareWarTrade(
					victim.getID(), owner.getTeam()) : NO_DENIAL);
			bool willNowWar = (otherValid && tradeDenial == NO_DENIAL);
			if (!getUWAI.isEnabled() && otherValid && !nowTooManyWars &&
				tradeDenial == DENIAL_TOO_MANY_WARS && warTeam.getNumWars() <= 0)
			{
				nowTooManyWars = true;
			}
			if(willNowWar == willWar.get(warTeam.getID(), victim.getID()))
				continue;
			willWar.set(warTeam.getID(), victim.getID(), willNowWar);
			if(willNowWar)
				willTradeMsgTeams.push_back(victim.getID());
			/*  Obviously can't hire warTeam if it has already declared war
				or if victim has been eliminated. */
			else if(victim.isAlive() && !warTeam.isAtWar(victim.getID()))
				noLongerTradeMsgTeams.push_back(victim.getID());
		}
		msg(warTeam.getID(), willTradeMsgTeams, true);
		if(GC.getDefineBOOL("ALERT_ON_NO_LONGER_WAR_TRADE"))
			msg(warTeam.getID(), noLongerTradeMsgTeams, false);
		if (!getUWAI.isEnabled() && nowTooManyWars != tooManyWars.get(warTeam.getID()) &&
			// Willingness to start a war implies not having "too much on their hands"
			(willTradeMsgTeams.empty() || nowTooManyWars))
		{
			msg(warTeam.getID(), nowTooManyWars);
		}
		tooManyWars.set(warTeam.getID(), nowTooManyWars);
	}
}

void WarTradeAlert::msg(TeamTypes warTeamId, std::vector<TeamTypes> victims,
	bool bTrade) const
{
	if(victims.empty())
		return;
	CvTeam const& warTeam = GET_TEAM(warTeamId);
	CvWString text = gDLL->getText((bTrade ? "TXT_KEY_CIV4LERTS_TRADE_WAR" :
			"TXT_KEY_CIV4LERTS_NO_LONGER_TRADE_WAR"),
			GET_PLAYER(warTeam.getLeaderID()).getName());
	if(victims.size() > 1)
		text += L":";
	text += L" ";
	for(size_t i = 0; i < victims.size(); i++)
	{
		text += GET_TEAM(victims[i]).getName();
		if(i != victims.size() - 1)
			text += L", ";
		else text += L".";
	}
	msg(text, warTeamId);
}


void WarTradeAlert::msg(TeamTypes warTeamId, bool bNowTooManyWars) const
{
	CvTeam const& warTeam = GET_TEAM(warTeamId);
	CvWString text = gDLL->getText((bNowTooManyWars ? "TXT_KEY_CIV4LERTS_TOO_MANY_WARS" :
			"TXT_KEY_CIV4LERTS_NO_LONGER_TOO_MANY_WARS"),
			warTeam.getName().GetCString());
	msg(text, warTeamId);
}


void WarTradeAlert::msg(CvWString text, TeamTypes warTeamId) const
{
	AdvCiv4lert::msg(text, NULL,
			// <advc.127b>
			GET_TEAM(warTeamId).getCapitalX(TEAMID(ownerId)),
			GET_TEAM(warTeamId).getCapitalY(TEAMID(ownerId)), // </advc.127b>
			GC.getColorType("WAR_TRADE_ALERT"));
} // </advc.210a>

// <advc.210b>
RevoltAlert::RevoltAlert(PlayerTypes eOwner) : AdvCiv4lert(eOwner) {}

void RevoltAlert::check()
{
	set<int> updatedRevolt;
	set<int> updatedOccupation;
	CvPlayer const& owner = GET_PLAYER(ownerId);
	FOR_EACH_CITY(c, owner)
	{
		bool couldPreviouslyRevolt = revoltPossible.count(c->plotNum()) > 0;
		bool wasOccupation = occupation.count(c->plotNum()) > 0;
		double pr = c->revoltProbability();
		if(pr > 0)
		{
			updatedRevolt.insert(c->plotNum());
			/*  Report only change in revolt chance OR change in occupation status;
				the latter takes precedence. */
			if(!couldPreviouslyRevolt && wasOccupation == c->isOccupation())
			{
				wchar szTempBuffer[1024];
				swprintf(szTempBuffer, L"%.1f", (float)(100 * pr));
				msg(gDLL->getText("TXT_KEY_CIV4LERTS_REVOLT", c->getName().
						GetCString(), szTempBuffer),
						NULL // icon works, but is too distracting
						,//ARTFILEMGR.getInterfaceArtInfo("INTERFACE_RESISTANCE")->getPath(),
						c->getX(), c->getY());
						// red text also too distracting
						//(ColorTypes)GC.getInfoTypeForString("COLOR_WARNING_TEXT"));
			}
		}
		#if 0 // Disabled: Message when revolt chance becomes 0
		else if(couldPreviouslyRevolt && wasOccupation == c->isOccupation() &&
			/*  Don't report 0 revolt chance when in occupation b/c
				revolt chance will increase a bit when occupation ends. */
			!c->isOccupation())
		{
			msg(gDLL->getText("TXT_KEY_CIV4LERTS_NO_LONGER_REVOLT", c->getName().
						GetCString()), NULL
						,//ARTFILEMGR.getInterfaceArtInfo("INTERFACE_RESISTANCE")->getPath(),
						c->getX(), c->getY());
		}
		#endif
		if(c->isOccupation())
			updatedOccupation.insert(c->plotNum());
		/*  If there's no order queued, the city will come to the player's attention
			anyway when it asks for orders. */
		else if(wasOccupation && c->getNumOrdersQueued() > 0)
		{
			msg(gDLL->getText("TXT_KEY_CIV4LERTS_CITY_PACIFIED_ADVC", c->getName().
						GetCString()), NULL,
						//ARTFILEMGR.getInterfaceArtInfo("INTERFACE_RESISTANCE")->getPath(),
						c->getX(), c->getY());
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
BonusThirdPartiesAlert::BonusThirdPartiesAlert(PlayerTypes eOwner) : AdvCiv4lert(eOwner)
{
	isDebug = false;
}

void BonusThirdPartiesAlert::check()
{
	multiset<int> updatedDeals[MAX_CIV_PLAYERS];
	FOR_EACH_DEAL(d)
	{
		// This alert ignores trades of ownerId
		if(d->getFirstPlayer() == ownerId || d->getSecondPlayer() == ownerId)
			continue;
		vector<int> dealData;
		getExportData(d->getFirstList(), d->getSecondPlayer(), dealData);
		for(size_t i = 0; i < dealData.size(); i++)
			updatedDeals[d->getFirstPlayer()].insert(dealData[i]);
		dealData.clear();
		getExportData(d->getSecondList(), d->getFirstPlayer(), dealData);
		for(size_t i = 0; i < dealData.size(); i++)
			updatedDeals[d->getSecondPlayer()].insert(dealData[i]);
	}
	for(int i = 0; i < MAX_CIV_PLAYERS; i++)
	{
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
		for(size_t j = 0; j < newDeals.size(); j++)
		{
			int newCount = updatedDeals[i].count(newDeals[j]);
			int oldCount = exportDeals[i].count(newDeals[j]);
			FAssert(newCount > oldCount);
			doMsg(fromId, newDeals[j], newCount, oldCount);
		}
		for(size_t j = 0; j < missingDeals.size(); j++)
		{
			int newCount = updatedDeals[i].count(missingDeals[j]);
			int oldCount = exportDeals[i].count(missingDeals[j]);
			FAssert(newCount < oldCount);
			doMsg(fromId, missingDeals[j], newCount, oldCount);
		}
		exportDeals[i] = updatedDeals[i];
	}
}

void BonusThirdPartiesAlert::getExportData(CLinkList<TradeData> const& list,
	PlayerTypes toId, std::vector<int>& r) const
{
	FOR_EACH_TRADE_ITEM(list)
	{
		if(pItem->m_eItemType == TRADE_RESOURCES)
			r.push_back(GC.getNumBonusInfos() * toId + pItem->m_iData);
	}
}

void BonusThirdPartiesAlert::doMsg(PlayerTypes fromId, int data, int newQuantity, int oldQuantity)
{
	BonusTypes bonusId = (BonusTypes)(data % GC.getNumBonusInfos());
	PlayerTypes toId = (PlayerTypes)((data - bonusId) / GC.getNumBonusInfos());
	CvPlayerAI const& from = GET_PLAYER(fromId);
	CvPlayerAI const& to = GET_PLAYER(toId);
	// Don't report ended trade when the reason is obvious
	if(!from.isAlive() || !to.isAlive() || GET_TEAM(from.getTeam()).isAtWar(to.getTeam()) ||
		from.AI_getMemoryCount(toId, MEMORY_STOPPED_TRADING_RECENT) > 0 ||
		to.AI_getMemoryCount(fromId, MEMORY_STOPPED_TRADING_RECENT) > 0)
	{
		return;
	}
	// Don't report unseen trades
	if(!GET_PLAYER(ownerId).isSpectator() && // advc.127
		(!GET_TEAM(ownerId).isHasMet(from.getTeam()) ||
		!GET_TEAM(ownerId).isHasMet(to.getTeam())))
	{
		return;
	}
	int bonusChar = GC.getInfo(bonusId).getChar();
	CvWString msgStr;
	CvWString quantityStr;
	if(!isDebug || newQuantity == 0 || oldQuantity == 0)
		quantityStr = gDLL->getText("TXT_KEY_CIV4LERTS_BONUS_ICON", bonusChar);
	else
	{
		quantityStr = (newQuantity > oldQuantity ?
				/*  The difference should practically always be 1; if it's more,
					it's still not incorrect to claim that one more resource is
					being traded. */
				gDLL->getText("TXT_KEY_CIV4LERTS_ONE_MORE", bonusChar) :
				gDLL->getText("TXT_KEY_CIV4LERTS_ONE_FEWER", bonusChar));
	}
	if(isDebug)
	{
		msgStr = (newQuantity > 0 ?
				gDLL->getText("TXT_KEY_CIV4LERTS_NOW_EXPORTING",
				from.getNameKey(), quantityStr.GetCString(), to.getNameKey()) :
				gDLL->getText("TXT_KEY_CIV4LERTS_NO_LONGER_EXPORTING",
				from.getNameKey(), quantityStr.GetCString(), to.getNameKey()));
	}
	else
	{
		if((newQuantity > 0) == (oldQuantity > 0))
			return;
		CvBonusInfo& bi = GC.getInfo(bonusId);
		bool strategic = (bi.getHappiness() + bi.getHealth() <= 0);
		if(!strategic) // Don't bother with buildings (only need to cover Ivory)
		{
			CvCivilization const& kCiv = to.getCivilization();
			for (int i = 0; i < kCiv.getNumUnits(); i++)
			{
				UnitTypes ut = kCiv.unitAt(i);
				CvUnitInfo const& ui = GC.getInfo(ut);
				if(ui.getPrereqAndBonus() == bonusId)
				{
					// Only report Ivory while it's relevant
					TechTypes tt = ui.getPrereqAndTech();
					if(tt != NO_TECH && to.getCurrentEra() -
							GC.getInfo(tt).getEra() < 2)
						strategic = true;
				}
			}
		}
		if(strategic)
		{
			msgStr = gDLL->getText(newQuantity > 0 ?
					"TXT_KEY_CIV4LERTS_EXPORTING_STRATEGIC" :
					"TXT_KEY_CIV4LERTS_NOT_EXPORTING_STRATEGIC",
					from.getNameKey(), to.getNameKey(), quantityStr.GetCString());
		}
		else
		{
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

// <advc.ctr>
CityTradeAlert::CityTradeAlert(PlayerTypes eOwner) : AdvCiv4lert(eOwner) {}

void CityTradeAlert::check()
{
	PROFILE_FUNC();
	CvPlayer const& kAlertPlayer = GET_PLAYER(ownerId);
	for(int i = 0; i < MAX_CIV_PLAYERS; i++)
	{
		CvPlayerAI const& kPlayer = GET_PLAYER((PlayerTypes)i);
		vector<int> willCedeNow;
		vector<int> willBuyNow;
		vector<int> canLiberateNow;
		vector<CvCity const*> diffCede;
		vector<CvCity const*> diffBuy;
		vector<CvCity const*> diffLiberate;
		if(kPlayer.isAlive() && !kPlayer.isMinorCiv() && 
			kPlayer.getTeam() != kAlertPlayer.getTeam() &&
			kAlertPlayer.canContact(kPlayer.getID(), true))
		{
			bool bWar = ::atWar(kAlertPlayer.getTeam(), kPlayer.getTeam());
			/*  Don't report "will cede" when war enemy unwilling to pay for peace
				(especially not cities that kPlayer has just conquered from kAlertPlayer) */
			if(!bWar || !getUWAI.isEnabled() || GET_TEAM(kPlayer.getTeam()).
				uwai().endWarVal(kAlertPlayer.getTeam()) > 0)
			{
				vector<int>& wasWilling = willCede[kPlayer.getID()];
				FOR_EACH_CITY(pCity, kPlayer)
				{
					int iCity = pCity->getID();
					TradeData item(TRADE_CITIES, iCity);
					if(kPlayer.canTradeItem(kAlertPlayer.getID(), item, true))
					{
						// When at war, check if they'd actually cede the city for peace.
						if(bWar)
						{
							CLinkList<TradeData> alertPlayerGives;
							CLinkList<TradeData> alertPlayerReceives;
							TradeData peaceTreaty(TRADE_PEACE_TREATY);
							alertPlayerGives.insertAtEnd(peaceTreaty);
							alertPlayerReceives.insertAtEnd(peaceTreaty);
							alertPlayerReceives.insertAtEnd(item);
							if(!kPlayer.AI_considerHypotheticalOffer(kAlertPlayer.getID(),
								alertPlayerGives, alertPlayerReceives, 1))
							{
								continue;
							}
						}
						willCedeNow.push_back(iCity);
						if(std::find(wasWilling.begin(), wasWilling.end(), iCity) == wasWilling.end())
							diffCede.push_back(pCity);
					}
				}
			}
			if(!bWar)
			{
				FOR_EACH_CITY(pCity, kAlertPlayer)
				{
					int iCity = pCity->getID();
					if(kAlertPlayer.canTradeItem(kPlayer.getID(), TradeData(
						TRADE_CITIES, iCity), true))
					{
						bool bLiberate = (pCity->getLiberationPlayer() == kPlayer.getID());
						if(bLiberate)
							canLiberateNow.push_back(iCity);
						else willBuyNow.push_back(iCity);
						if( // Don't report cities right after acquisition
							//GC.getGame().getGameTurn() - pCity->getGameTurnAcquired() > 1 &&
							// Don't report possible liberation right after making peace
							/*(!bLiberate || GET_TEAM(kPlayer.getTeam()).
							AI_getAtPeaceCounter(kAlertPlayer.getTeam()) > 1)*/
							//^Try a different tack.
							(!bLiberate || pCity->getPreviousOwner() != kPlayer.getID()))
						{
							vector<int>& was = (bLiberate ? canLiberate[kPlayer.getID()] :
									willBuy[kPlayer.getID()]);
							if(std::find(was.begin(), was.end(), iCity) == was.end())
								(bLiberate ? diffLiberate : diffBuy).push_back(pCity);
						}
					}
				}
			}
			willCede[kPlayer.getID()].clear();
			willBuy[kPlayer.getID()].clear();
			canLiberate[kPlayer.getID()].clear();
			willCede[kPlayer.getID()].insert(willCede[kPlayer.getID()].begin(),
					willCedeNow.begin(), willCedeNow.end());
			willBuy[kPlayer.getID()].insert(willBuy[kPlayer.getID()].begin(),
					willBuyNow.begin(), willBuyNow.end());
			canLiberate[kPlayer.getID()].insert(canLiberate[kPlayer.getID()].begin(),
					canLiberateNow.begin(), canLiberateNow.end());
		}
		msgWilling(diffCede, kPlayer.getID(), true);
		msgWilling(diffBuy, kPlayer.getID(), false);
		msgLiberate(diffLiberate, kPlayer.getID());
	}
}

void CityTradeAlert::msgWilling(std::vector<CvCity const*> const& cities, PlayerTypes ePlayer, bool bCede) const
{
	if(cities.empty())
		return;

	CvWString szMsg(GET_PLAYER(ePlayer).getName());
	szMsg.append(L" ");
	szMsg.append(gDLL->getText(bCede ? "TXT_KEY_WILLING_TO_CEDE" : "TXT_KEY_WILLING_TO_TRADE_FOR"));
	for(size_t j = 0; j < cities.size(); j++)
	{
		if(j > 0)
			szMsg.append(L",");
		szMsg.append(L" ");
		szMsg.append(cities[j]->getName());
	}
	msg(szMsg, NULL, cities.size() == 1 ? cities[0]->getX() : - 1,
			cities.size() == 1 ? cities[0]->getY() : - 1,
			GC.getColorType("CITY_BLUE"));
}

void CityTradeAlert::msgLiberate(std::vector<CvCity const*> const& cities, PlayerTypes ePlayer) const
{
	if(cities.empty())
		return;

	CvWString szMsg;
	for(size_t j = 0; j < cities.size(); j++)
	{
		szMsg.append(cities[j]->getName());
		if(j < cities.size() - 1)
			szMsg.append(L",");
		szMsg.append(L" ");
	}
	CvWString szName(GET_PLAYER(ePlayer).getName());
	szMsg.append(gDLL->getText("TXT_KEY_CAN_LIBERATE", szName.GetCString()));
	msg(szMsg, NULL, cities.size() == 1 ? cities[0]->getX() : - 1,
			cities.size() == 1 ? cities[0]->getY() : - 1,
			GC.getColorType("CITY_BLUE"));
}
// </advc.ctr>
