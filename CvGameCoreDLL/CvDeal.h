#pragma once

// CvDeal.h

#ifndef CIV4_DEAL_H
#define CIV4_DEAL_H


#include "LinkedList.h"


class CvDeal
{

public:

	CvDeal();
	virtual ~CvDeal();

	void init(int iID, PlayerTypes eFirstPlayer, PlayerTypes eSecondPlayer);
	void uninit();
	void reset(int iID = 0, PlayerTypes eFirstPlayer = NO_PLAYER, PlayerTypes eSecondPlayer = NO_PLAYER);

	DllExport void kill(bool bKillTeam = true) { // <advc.130p>
			kill(bKillTeam, NO_PLAYER); }
	void kill(bool bKillTeam, PlayerTypes eCancelPlayer); // </advc.130p>
	// advc.036:
	void killSilent(bool bKillTeam = true, bool bUpdateAttitude = true,
			PlayerTypes eCancelPlayer = NO_PLAYER); // advc.130p
	void addTrades(CLinkList<TradeData>* pFirstList, CLinkList<TradeData>* pSecondList, bool bCheckAllowed);

	void doTurn();

	void verify();

	bool isPeaceDeal() const;
	// advc.130p: BtS function; now unused.
	//bool isPeaceDealBetweenOthers(CLinkList<TradeData>* pFirstList, CLinkList<TradeData>* pSecondList) const;
	bool isVassalDeal() const;
	bool isUncancelableVassalDeal(PlayerTypes eByPlayer, CvWString* pszReason = NULL) const;
	DllExport static bool isVassalTributeDeal(const CLinkList<TradeData>* pList);
	/*  advc.003: The above checks if pList contains only TRADE_RESSOURCE items;
		need a function that checks if this deal is a tribute deal between a vassal and a master. */
	bool isVassalTributeDeal() const;
	bool isDisengage() const; // advc.034
	DllExport int getID() const;
	void setID(int iID);

	int getInitialGameTurn() const;
	void setInitialGameTurn(int iNewValue);
	int getAge() const; // advc.133
	DllExport PlayerTypes getFirstPlayer() const;
	DllExport PlayerTypes getSecondPlayer() const;
	// <advc.003>
	bool isBetween(PlayerTypes ePlayer, PlayerTypes eOtherPlayer) const;
	bool isBetween(TeamTypes eTeam, TeamTypes eOtherTeam) const;
	// </advc.003>
	void clearFirstTrades(); // advc.003j (comment): unused
	void insertAtEndFirstTrades(TradeData trade);
	DllExport CLLNode<TradeData>* nextFirstTradesNode(CLLNode<TradeData>* pNode) const;
	int getLengthFirstTrades() const;
	DllExport CLLNode<TradeData>* headFirstTradesNode() const;
	const CLinkList<TradeData>* getFirstTrades() const;

	void clearSecondTrades(); // advc.003j (comment): unused
	void insertAtEndSecondTrades(TradeData trade);
	DllExport CLLNode<TradeData>* nextSecondTradesNode(CLLNode<TradeData>* pNode) const;
	int getLengthSecondTrades() const;
	DllExport CLLNode<TradeData>* headSecondTradesNode() const;
	const CLinkList<TradeData>* getSecondTrades() const;

	DllExport bool isCancelable(PlayerTypes eByPlayer = NO_PLAYER, CvWString* pszReason = NULL);
	bool isEverCancelable(PlayerTypes eByPlayer) const; // advc.130f
	int turnsToCancel(PlayerTypes eByPlayer = NO_PLAYER);

	static bool isAnnual(TradeableItems eItem);
	DllExport static bool isDual(TradeableItems eItem, bool bExcludePeace = false);
	DllExport static bool hasData(TradeableItems eItem);
	DllExport static bool isGold(TradeableItems eItem);
	DllExport static bool isEndWar(TradeableItems eItem);
	static bool isVassal(TradeableItems eItem);
	DllExport static TradeableItems getPeaceItem();
	DllExport static TradeableItems getGoldItem();
	DllExport static TradeableItems getGoldPerTurnItem();

	void read(FDataStreamBase* pStream);
	void write(FDataStreamBase* pStream);

protected:

	bool startTrade(TradeData trade, PlayerTypes eFromPlayer, PlayerTypes eToPlayer,
			bool bPeace); // advc.122
	void endTrade(TradeData trade, PlayerTypes eFromPlayer, PlayerTypes eToPlayer, bool bTeam,
			bool bUpdateAttitude = true, // advc.036
			PlayerTypes eCancelPlayer = NO_PLAYER); // advc.130p
	// <advc.130p>
	static void addEndTradeMemory(PlayerTypes eFromPlayer, PlayerTypes eToPlayer,
			TradeableItems eItemType);
	bool recordTradeValue(CLinkList<TradeData>* pFirstList, CLinkList<TradeData>* pSecondList,
			PlayerTypes eFirstPlayer, PlayerTypes eSecondPlayer, bool bPeace,
			TeamTypes ePeaceTradeTarget = NO_TEAM, TeamTypes eWarTradeTarget = NO_TEAM);
	// </advc.130p>
	void startTeamTrade(TradeableItems eItem, TeamTypes eFromTeam, TeamTypes eToTeam, bool bDual);
	void endTeamTrade(TradeableItems eItem, TeamTypes eFromTeam, TeamTypes eToTeam);

	static bool isVassalTrade(const CLinkList<TradeData>* pFirstList);

	int m_iID;
	int m_iInitialGameTurn;

	PlayerTypes m_eFirstPlayer;
	PlayerTypes m_eSecondPlayer;

	CLinkList<TradeData> m_firstTrades;
	CLinkList<TradeData> m_secondTrades;
};

#endif
