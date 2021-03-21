#pragma once

#ifndef ADVCIV4LERTS_H
#define ADVCIV4LERTS_H

/*  advc.210: I can't write this in Python, so I'm having Civ4lerts.py call
	this class. Will need to make changes in Civ4lerts.py, Civ4lerts.xml,
	BugAlertsOptionsTab.py, CvPlayer.cpp and some file for game text
	(like CIV4GameText_advc.xml) to add more alerts. */
class AdvCiv4lert {

public:
	AdvCiv4lert(PlayerTypes eOwner);
	virtual ~AdvCiv4lert() {}
	// Silent calls are for initializing data after loading a savegame
	void check(bool silent);

protected:
	virtual void check()=0;
	void msg(CvWString s, LPCSTR icon = NULL, int x = -1, int y = -1,
			ColorTypes colorId = NO_COLOR) const;
	PlayerTypes ownerId;
	bool isSilent;
	bool isDebug;
};

// <advc.210a>
class WarTradeAlert : public AdvCiv4lert {
public:
	WarTradeAlert(PlayerTypes eOwner) : AdvCiv4lert(eOwner) {}
protected:
	void check();
private:
	void msg(TeamTypes warTeamId, std::vector<TeamTypes> victims, bool bTrade) const;
	void msg(TeamTypes warTeamId, bool bNowTooManyWars) const;
	void msg(CvWString text, TeamTypes warTeamId) const;
	EnumMap2D<TeamTypes,TeamTypes,bool> willWar;
	EnumMap<TeamTypes,bool> tooManyWars;
}; // </advc.210a>

// <advc.210b>
class RevoltAlert : public AdvCiv4lert {
public:
	RevoltAlert(PlayerTypes eOwner);
protected:
	void check();
private:
	std::set<int> revoltPossible;
	std::set<int> occupation;
}; // </advc.210b>

// <advc.210d>
class BonusThirdPartiesAlert : public AdvCiv4lert {
public:
	BonusThirdPartiesAlert(PlayerTypes eOwner);
protected:
	void check();
private:
	void getExportData(CLinkList<TradeData> const& list, PlayerTypes toId,
			std::vector<int>& r) const;
	void doMsg(PlayerTypes fromId, int data, int newQuantity,
			int oldQuantity);
	std::multiset<int> exportDeals[MAX_CIV_PLAYERS];
}; // </advc.210d>

// <advc.ctr>
class CvCity;

class CityTradeAlert : public AdvCiv4lert
{
public:
	CityTradeAlert(PlayerTypes eOwner);
protected:
	void check();
private:
	void msgWilling(std::vector<CvCity const*> const& cities, PlayerTypes ePlayer, bool bCede) const;
	void msgLiberate(std::vector<CvCity const*> const& cities, PlayerTypes ePlayer) const;
	std::vector<int> willCede[MAX_CIV_PLAYERS];
	std::vector<int> willBuy[MAX_CIV_PLAYERS];
	std::vector<int> canLiberate[MAX_CIV_PLAYERS];
}; // </advc.ctr>

#endif
