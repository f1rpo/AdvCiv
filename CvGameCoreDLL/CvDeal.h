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
	void addTrades(CLinkList<TradeData> const& kFirstList, CLinkList<TradeData> const& kSecondList, bool bCheckAllowed);

	void doTurn();
	void verify();

	bool isPeaceDeal() const;
	// advc.130p: BtS function; now unused.
	//bool isPeaceDealBetweenOthers(CLinkList<TradeData>* pFirstList, CLinkList<TradeData>* pSecondList) const;
	bool isVassalDeal() const;
	DllExport static bool isVassalTributeDeal(const CLinkList<TradeData>* pList);
	/*  advc: The above checks if pList contains only TRADE_RESSOURCE items;
		need a function that checks if this deal is a tribute deal between a vassal and a master. */
	bool isVassalTributeDeal() const;
	bool isDisengage() const; // advc.034

	DllExport inline int getID() const { return m_iID; } // advc.inl
	void setID(int iID);

	int getInitialGameTurn() const;
	void setInitialGameTurn(int iNewValue);
	int getAge() const; // advc.133
	// <advc.inl>
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
	// </advc.inl>
	// <advc> More convenient interface for iteration
	/*  Want to make all the CLLNodes const - should generally not modify deal lists
		while traversing them. (Though this means that CLLNode::m_data also can't be
		changed; make that mutable?) */
	inline CLLNode<TradeData> const* nextFirstTradesNode(CLLNode<TradeData> const* pNode) const {
		return m_firstTrades.next(pNode); }
	inline CLLNode<TradeData> const* nextSecondTradesNode(CLLNode<TradeData> const* pNode) const {
		return m_secondTrades.next(pNode); }
	bool isBetween(PlayerTypes ePlayer, PlayerTypes eOtherPlayer) const;
	bool isBetween(TeamTypes eTeam, TeamTypes eOtherTeam) const;
	bool isBetween(PlayerTypes ePlayer, TeamTypes eTeam) const;
	bool involves(PlayerTypes ePlayer) const;
	bool involves(TeamTypes eTeam) const;
	PlayerTypes getOtherPlayer(PlayerTypes ePlayer) const;
	// Caller has to ensure that ePlayer/ eTeam is involved in the trade!
	CLinkList<TradeData> const& getGivesList(PlayerTypes ePlayer) const;
	CLinkList<TradeData> const& getGivesList(TeamTypes eTeam) const;
	CLLNode<TradeData> const* headGivesNode(PlayerTypes ePlayer) const;
	CLLNode<TradeData> const* nextGivesNode(CLLNode<TradeData> const* pNode, PlayerTypes ePlayer) const;
	CLLNode<TradeData> const* headGivesNode(TeamTypes eTeam) const;
	CLLNode<TradeData> const* nextGivesNode(CLLNode<TradeData> const* pNode, TeamTypes eTeam) const;
	CLinkList<TradeData> const& getReceivesList(PlayerTypes ePlayer) const;
	CLinkList<TradeData> const& getReceivesList(TeamTypes eTeam) const;
	CLLNode<TradeData> const* headReceivesNode(PlayerTypes ePlayer) const;
	CLLNode<TradeData> const* nextReceivesNode(CLLNode<TradeData> const* pNode, PlayerTypes ePlayer) const;
	CLLNode<TradeData> const* headReceivesNode(TeamTypes eTeam) const;
	CLLNode<TradeData> const* nextReceivesNode(CLLNode<TradeData> const* pNode, TeamTypes eTeam) const;
	CLLNode<TradeData> const* headTradesNode() const;
	CLLNode<TradeData> const* nextTradesNode(CLLNode<TradeData> const* pNode) const;
	// </advc>

	void clearFirstTrades(); // advc.003j (comment): unused
	void insertAtEndFirstTrades(TradeData trade); // advc (comment): Currently only used internally
	int getLengthFirstTrades() const;

	void clearSecondTrades(); // advc.003j (comment): unused
	void insertAtEndSecondTrades(TradeData trade); // advc (comment): Currently only used internally
	int getLengthSecondTrades() const;

	DllExport bool isCancelable(PlayerTypes eByPlayer = NO_PLAYER, CvWString* pszReason = NULL)
	// <advc> Need a const version
	{
		CvDeal const& kThis = *this;
		return kThis.isCancelable(eByPlayer, pszReason);
	} bool isCancelable(PlayerTypes eByPlayer = NO_PLAYER, CvWString* pszReason = NULL) const;
	// </advc>
	bool isEverCancelable(PlayerTypes eByPlayer) const; // advc.130f
	int turnsToCancel(PlayerTypes eByPlayer = NO_PLAYER) /* advc: */ const;
	bool isAllDual() const; // advc

	static bool isAnnual(TradeableItems eItem);
	DllExport static bool isDual(TradeableItems eItem, bool bExcludePeace = false);
	DllExport static bool hasData(TradeableItems eItem);
	DllExport static bool isEndWar(TradeableItems eItem);
	// advc.inl: Inlined x5
	DllExport static bool isGold(TradeableItems eItem)
	{
		return (eItem == getGoldItem() || eItem == getGoldPerTurnItem());
	}
	static bool isVassal(TradeableItems eItem)
	{
		return (eItem == TRADE_VASSAL || eItem == TRADE_SURRENDER);
	}
	DllExport static inline TradeableItems getPeaceItem() { return TRADE_PEACE_TREATY; }
	DllExport static inline TradeableItems getGoldItem() { return TRADE_GOLD; }
	DllExport static inline TradeableItems getGoldPerTurnItem() { return TRADE_GOLD_PER_TURN; }

	void read(FDataStreamBase* pStream);
	void write(FDataStreamBase* pStream);

protected:
	bool startTrade(TradeData trade, PlayerTypes eFromPlayer, PlayerTypes eToPlayer,
			bool bPeace); // advc.ctr
	void endTrade(TradeData trade, PlayerTypes eFromPlayer, PlayerTypes eToPlayer, bool bTeam,
			bool bUpdateAttitude = true, // advc.036
			PlayerTypes eCancelPlayer = NO_PLAYER); // advc.130p
	// <advc.130p>
	static void addEndTradeMemory(PlayerTypes eFromPlayer, PlayerTypes eToPlayer,
			TradeableItems eItemType);
	bool recordTradeValue(CLinkList<TradeData> const& kFirstList, CLinkList<TradeData> const& kSecondList,
			PlayerTypes eFirstPlayer, PlayerTypes eSecondPlayer, bool bPeace,
			TeamTypes ePeaceTradeTarget = NO_TEAM, TeamTypes eWarTradeTarget = NO_TEAM,
			bool bAIRequest = false); // advc.ctr
	// </advc.130p>
	void startTeamTrade(TradeableItems eItem, TeamTypes eFromTeam, TeamTypes eToTeam, bool bDual);
	void endTeamTrade(TradeableItems eItem, TeamTypes eFromTeam, TeamTypes eToTeam);
	void announceCancel(PlayerTypes eMsgTarget, PlayerTypes eOther, // advc
			bool bForce) const; // advc.106j
	bool verify(PlayerTypes eRecipient, PlayerTypes eGiver);
	// advc: was public
	bool isUncancelableVassalDeal(PlayerTypes eByPlayer, CvWString* pszReason = NULL) const;

	static bool isVassalTrade(CLinkList<TradeData> const& kList);

	int m_iID;
	int m_iInitialGameTurn;

	PlayerTypes m_eFirstPlayer;
	PlayerTypes m_eSecondPlayer;

	CLinkList<TradeData> m_firstTrades;
	CLinkList<TradeData> m_secondTrades;
};

#endif
