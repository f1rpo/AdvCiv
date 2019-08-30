#pragma once

// CvDeal.h

#ifndef CIV4_DEAL_H
#define CIV4_DEAL_H


class CvDeal
{
public:

	CvDeal();
	~CvDeal();

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
	DllExport static bool isVassalTributeDeal(const CLinkList<TradeData>* pList);
	/*  advc.003: The above checks if pList contains only TRADE_RESSOURCE items;
		need a function that checks if this deal is a tribute deal between a vassal and a master. */
	bool isVassalTributeDeal() const;
	bool isDisengage() const; // advc.034

	DllExport inline int getID() const { return m_iID; } // advc.003f: inline
	void setID(int iID);

	int getInitialGameTurn() const;
	void setInitialGameTurn(int iNewValue);
	int getAge() const; // advc.133
	// <advc.003f> inlined
	DllExport inline PlayerTypes getFirstPlayer() const {
		return m_eFirstPlayer; }
	inline CLinkList<TradeData> const* getFirstTrades() const {
		return &(m_firstTrades); }
	DllExport inline CLLNode<TradeData>* headFirstTradesNode() const {
		return m_firstTrades.head(); }
	DllExport inline CLLNode<TradeData>* nextFirstTradesNode(CLLNode<TradeData>* pNode) const {
		return m_firstTrades.next(pNode); }
	DllExport inline PlayerTypes getSecondPlayer() const {
		return m_eSecondPlayer; }
	inline CLinkList<TradeData> const* getSecondTrades() const {
		return &(m_secondTrades); }
	DllExport inline CLLNode<TradeData>* headSecondTradesNode() const {
		return m_secondTrades.head(); }
	DllExport inline CLLNode<TradeData>* nextSecondTradesNode(CLLNode<TradeData>* pNode) const {
		return m_secondTrades.next(pNode); }
	// </advc.003f>
	// <advc.003> More convenient interface for iteration
	bool isBetween(PlayerTypes ePlayer, PlayerTypes eOtherPlayer) const;
	bool isBetween(TeamTypes eTeam, TeamTypes eOtherTeam) const;
	bool isBetween(PlayerTypes ePlayer, TeamTypes eTeam) const;
	bool involves(PlayerTypes ePlayer) const;
	bool involves(TeamTypes eTeam) const;
	PlayerTypes getOtherPlayer(PlayerTypes ePlayer) const;
	// Caller has to ensure that ePlayer/ eTeam is involved in the trade!
	CLinkList<TradeData> const& getGivesList(PlayerTypes ePlayer) const;
	CLinkList<TradeData> const& getGivesList(TeamTypes eTeam) const;
	CLLNode<TradeData>* headGivesNode(PlayerTypes ePlayer) const;
	CLLNode<TradeData>* nextGivesNode(CLLNode<TradeData>* pNode, PlayerTypes ePlayer) const;
	CLLNode<TradeData>* headGivesNode(TeamTypes eTeam) const;
	CLLNode<TradeData>* nextGivesNode(CLLNode<TradeData>* pNode, TeamTypes eTeam) const;
	CLinkList<TradeData> const& getReceivesList(PlayerTypes ePlayer) const;
	CLinkList<TradeData> const& getReceivesList(TeamTypes eTeam) const;
	CLLNode<TradeData>* headReceivesNode(PlayerTypes ePlayer) const;
	CLLNode<TradeData>* nextReceivesNode(CLLNode<TradeData>* pNode, PlayerTypes ePlayer) const;
	CLLNode<TradeData>* headReceivesNode(TeamTypes eTeam) const;
	CLLNode<TradeData>* nextReceivesNode(CLLNode<TradeData>* pNode, TeamTypes eTeam) const;
	CLLNode<TradeData>* headTradesNode() const;
	CLLNode<TradeData>* nextTradesNode(CLLNode<TradeData>* pNode) const;
	// </advc.003>

	void clearFirstTrades(); // advc.003j (comment): unused
	void insertAtEndFirstTrades(TradeData trade); // advc (comment): Currently only used internally
	int getLengthFirstTrades() const;

	void clearSecondTrades(); // advc.003j (comment): unused
	void insertAtEndSecondTrades(TradeData trade); // advc (comment): Currently only used internally
	int getLengthSecondTrades() const;

	DllExport bool isCancelable(PlayerTypes eByPlayer = NO_PLAYER, CvWString* pszReason = NULL)
	// <advc.003> Need a const version
	{
		CvDeal const& kThis = *this;
		return kThis.isCancelable(eByPlayer, pszReason);
	} bool isCancelable(PlayerTypes eByPlayer = NO_PLAYER, CvWString* pszReason = NULL) const;
	// </advc.003>
	bool isEverCancelable(PlayerTypes eByPlayer) const; // advc.130f
	int turnsToCancel(PlayerTypes eByPlayer = NO_PLAYER) /* advc.003: */ const;
	bool isAllDual() const; // advc.003

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
	void announceCancel(PlayerTypes eMsgTarget, PlayerTypes eOther, // advc.003
			bool bForce) const; // advc.106j
	bool verify(PlayerTypes eRecipient, PlayerTypes eGiver);
	// advc.003: was public
	bool isUncancelableVassalDeal(PlayerTypes eByPlayer, CvWString* pszReason = NULL) const;

	static bool isVassalTrade(const CLinkList<TradeData>* pFirstList);

	int m_iID;
	int m_iInitialGameTurn;

	PlayerTypes m_eFirstPlayer;
	PlayerTypes m_eSecondPlayer;

	CLinkList<TradeData> m_firstTrades;
	CLinkList<TradeData> m_secondTrades;
};

#endif
