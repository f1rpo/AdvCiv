// CvDeal.cpp

#include "CvGameCoreDLL.h"
#include "CvGlobals.h"
#include "CvGameAI.h"
#include "CvTeamAI.h"
#include "CvPlayerAI.h"
#include "CvMap.h"
#include "CvPlot.h"
#include "CvGameCoreUtils.h"
#include "CvGameTextMgr.h"
#include "CvDLLInterfaceIFaceBase.h"
#include "CvEventReporter.h"

/************************************************************************************************/
/* BETTER_BTS_AI_MOD                      10/02/09                                jdog5000      */
/*                                                                                              */
/* AI logging                                                                                   */
/************************************************************************************************/
#include "BetterBTSAI.h"
/************************************************************************************************/
/* BETTER_BTS_AI_MOD                       END                                                  */
/************************************************************************************************/

// Public Functions...

CvDeal::CvDeal()
{
	reset();
}


CvDeal::~CvDeal()
{
	uninit();
}


void CvDeal::init(int iID, PlayerTypes eFirstPlayer, PlayerTypes eSecondPlayer)
{
	//--------------------------------
	// Init saved data
	reset(iID, eFirstPlayer, eSecondPlayer);

	//--------------------------------
	// Init non-saved data

	//--------------------------------
	// Init other game data
	setInitialGameTurn(GC.getGameINLINE().getGameTurn());
}


void CvDeal::uninit()
{
	m_firstTrades.clear();
	m_secondTrades.clear();
}


// FUNCTION: reset()
// Initializes data members that are serialized.
void CvDeal::reset(int iID, PlayerTypes eFirstPlayer, PlayerTypes eSecondPlayer)
{
	//--------------------------------
	// Uninit class
	uninit();

	m_iID = iID;
	m_iInitialGameTurn = 0;

	m_eFirstPlayer = eFirstPlayer;
	m_eSecondPlayer = eSecondPlayer;
}


void CvDeal::kill(bool bKillTeam)
{
	if ((getLengthFirstTrades() > 0) || (getLengthSecondTrades() > 0))
	{
		CvWString szString;
		CvWStringBuffer szDealString;
		CvWString szCancelString = gDLL->getText("TXT_KEY_POPUP_DEAL_CANCEL");
		if (GET_TEAM(GET_PLAYER(getFirstPlayer()).getTeam()).isHasMet(GET_PLAYER(getSecondPlayer()).getTeam()))
		{
			szDealString.clear();
			GAMETEXT.getDealString(szDealString, *this, getFirstPlayer());
			szString.Format(L"%s: %s", szCancelString.GetCString(), szDealString.getCString());
			gDLL->getInterfaceIFace()->addHumanMessage((PlayerTypes)getFirstPlayer(), true, GC.getEVENT_MESSAGE_TIME(), szString, "AS2D_DEAL_CANCELLED");
		}

		if (GET_TEAM(GET_PLAYER(getSecondPlayer()).getTeam()).isHasMet(GET_PLAYER(getFirstPlayer()).getTeam()))
		{
			szDealString.clear();
			GAMETEXT.getDealString(szDealString, *this, getSecondPlayer());
			szString.Format(L"%s: %s", szCancelString.GetCString(), szDealString.getCString());
			gDLL->getInterfaceIFace()->addHumanMessage((PlayerTypes)getSecondPlayer(), true, GC.getEVENT_MESSAGE_TIME(), szString, "AS2D_DEAL_CANCELLED");
		}
	}

	CLLNode<TradeData>* pNode;

	for (pNode = headFirstTradesNode(); (pNode != NULL); pNode = nextFirstTradesNode(pNode))
	{
		endTrade(pNode->m_data, getFirstPlayer(), getSecondPlayer(), bKillTeam);
	}

	for (pNode = headSecondTradesNode(); (pNode != NULL); pNode = nextSecondTradesNode(pNode))
	{
		endTrade(pNode->m_data, getSecondPlayer(), getFirstPlayer(), bKillTeam);
	}

	GC.getGameINLINE().deleteDeal(getID());
}


void CvDeal::addTrades(CLinkList<TradeData>* pFirstList, CLinkList<TradeData>* pSecondList, bool bCheckAllowed)
{
	if (isVassalTrade(pFirstList) && isVassalTrade(pSecondList))
	{
		return;
	}

	if (pFirstList != NULL)
	{
		for (CLLNode<TradeData>* pNode = pFirstList->head(); pNode; pNode = pFirstList->next(pNode))
		{
			if (bCheckAllowed)
			{
				if (!(GET_PLAYER(getFirstPlayer()).canTradeItem(getSecondPlayer(), pNode->m_data)))
				{
					return;
				}
			}
		}
	}

	if (pSecondList != NULL)
	{
		for (CLLNode<TradeData>* pNode = pSecondList->head(); pNode; pNode = pSecondList->next(pNode))
		{
			if (bCheckAllowed && !(GET_PLAYER(getSecondPlayer()).canTradeItem(getFirstPlayer(), pNode->m_data)))
			{
				return;
			}
		}
	}

	TeamTypes eFirstTeam = GET_PLAYER(getFirstPlayer()).getTeam();
	TeamTypes eSecondTeam = GET_PLAYER(getSecondPlayer()).getTeam();
	bool bBumpUnits = false; // K-Mod

	if (atWar(eFirstTeam, eSecondTeam))
	{
		// free vassals of capitulating team before peace is signed
		/*  advc.130y: Commented out; let CvTeam::setVassal free the vassals
			after signing peace */
		/*if (isVassalTrade(pSecondList))
		{
			for (int iI = 0; iI < MAX_TEAMS; iI++)
			{
				TeamTypes eLoopTeam = (TeamTypes) iI;
				CvTeam& kLoopTeam = GET_TEAM(eLoopTeam);
				if ((eLoopTeam != eFirstTeam) && (eLoopTeam != eSecondTeam))
				{
					if (kLoopTeam.isAlive() && kLoopTeam.isVassal(eSecondTeam))
					{
						GET_TEAM(eSecondTeam).freeVassal(eLoopTeam);
						int iSecondSuccess = GET_TEAM(eFirstTeam).AI_getWarSuccess(eSecondTeam) + GC.getWAR_SUCCESS_CITY_CAPTURING() * GET_TEAM(eSecondTeam).getNumCities();
						GET_TEAM(eFirstTeam).AI_setWarSuccess(eLoopTeam, std::max(iSecondSuccess, GET_TEAM(eFirstTeam).AI_getWarSuccess(eLoopTeam)));
					}
				}
			}
		}
		else if (isVassalTrade(pFirstList)) // K-Mod added 'else'
		{
			for (int iI = 0; iI < MAX_TEAMS; iI++)
			{
				TeamTypes eLoopTeam = (TeamTypes) iI;
				CvTeam& kLoopTeam = GET_TEAM(eLoopTeam);
				if ((eLoopTeam != eFirstTeam) && (eLoopTeam != eSecondTeam))
				{
					if (kLoopTeam.isAlive() && kLoopTeam.isVassal(eFirstTeam))
					{
						GET_TEAM(eFirstTeam).freeVassal(eLoopTeam);
						int iFirstSuccess = GET_TEAM(eSecondTeam).AI_getWarSuccess(eFirstTeam) + GC.getWAR_SUCCESS_CITY_CAPTURING() * GET_TEAM(eFirstTeam).getNumCities();
						GET_TEAM(eSecondTeam).AI_setWarSuccess(eLoopTeam, std::max(iFirstSuccess, GET_TEAM(eSecondTeam).AI_getWarSuccess(eLoopTeam)));
					}
				}
			}
		}*/

		//GET_TEAM(eFirstTeam).makePeace(eSecondTeam, !isVassalTrade(pFirstList) && !isVassalTrade(pSecondList));

		// K-Mod. Bump units only after all trades are completed, because some deals (such as city gifts) may affect which units get bumped.
		// (originally, units were bumped automatically while executing the peace deal trades)
		// Note: the original code didn't bump units for vassal trades. This is can erroneously allow the vassal's units to stay in the master's land.
		GET_TEAM(eFirstTeam).makePeace(eSecondTeam, false);
		bBumpUnits = true;
		// K-Mod end
	}
	else
	{
		if (!isPeaceDealBetweenOthers(pFirstList, pSecondList))
		{
			if ((pSecondList != NULL) && (pSecondList->getLength() > 0))
			{
				int iValue =
				/*  <advc.550a> Ignore discounts when it comes to fair-trade
					diplo bonuses? Hard to decide, apply half the discount for now. */
						::round((GET_PLAYER(getFirstPlayer()).AI_dealVal(
						getSecondPlayer(), pSecondList, true, 1, true) + // </advc.550a>
						GET_PLAYER(getFirstPlayer()).AI_dealVal(
						getSecondPlayer(), pSecondList, true)
						/ 2.0)); // advc.550a

				if ((pFirstList != NULL) && (pFirstList->getLength() > 0))
				{
					GET_PLAYER(getFirstPlayer()).AI_changePeacetimeTradeValue(getSecondPlayer(), iValue);
				}
				else
				{
					GET_PLAYER(getFirstPlayer()).AI_changePeacetimeGrantValue(getSecondPlayer(), iValue);
				}
			}
			if ((pFirstList != NULL) && (pFirstList->getLength() > 0))
			{
				int iValue = // <advc.550a>
						::round((GET_PLAYER(getSecondPlayer()).AI_dealVal(
						getFirstPlayer(), pFirstList, true, 1, true) + // </advc.550a>
						GET_PLAYER(getSecondPlayer()).AI_dealVal(
						getFirstPlayer(), pFirstList, true)
						/ 2.0)); // advc.550a

				if ((pSecondList != NULL) && (pSecondList->getLength() > 0))
				{
					GET_PLAYER(getSecondPlayer()).AI_changePeacetimeTradeValue(getFirstPlayer(), iValue);
				}
				else
				{
					GET_PLAYER(getSecondPlayer()).AI_changePeacetimeGrantValue(getFirstPlayer(), iValue);
				}
			}
			// K-Mod
			GET_PLAYER(getFirstPlayer()).AI_updateAttitudeCache(getSecondPlayer());
			GET_PLAYER(getSecondPlayer()).AI_updateAttitudeCache(getFirstPlayer());
			// K-Mod end
		}
	}

	bool bAlliance = false;

	if (pFirstList != NULL)
	{
		// K-Mod. Vassal deals need to be implemented last, so that master/vassal power is set correctly.
		for (CLLNode<TradeData>* pNode = pFirstList->head(); pNode; pNode = pFirstList->next(pNode))
		{
			if (isVassal(pNode->m_data.m_eItemType))
			{
				pFirstList->moveToEnd(pNode);
				break;
			}
		}
		// K-Mod end
		for (CLLNode<TradeData>* pNode = pFirstList->head(); pNode; pNode = pFirstList->next(pNode))
		{
			// <advc.104> Allow UWAI to record the value of the sponsorship
			if(pNode->m_data.m_eItemType == TRADE_WAR)
				GET_PLAYER(getFirstPlayer()).warAndPeaceAI().getCache().
						reportSponsoredWar(*pSecondList, getSecondPlayer(),
						(TeamTypes)pNode->m_data.m_iData); // </advc.104>
			bool bSave = startTrade(pNode->m_data, getFirstPlayer(), getSecondPlayer());
			bBumpUnits = bBumpUnits || pNode->m_data.m_eItemType == TRADE_PEACE; // K-Mod

			if (bSave)
				insertAtEndFirstTrades(pNode->m_data);
			if (pNode->m_data.m_eItemType == TRADE_PERMANENT_ALLIANCE)
				bAlliance = true;
		}
	}

	if (pSecondList != NULL)
	{
		// K-Mod. Vassal deals need to be implemented last, so that master/vassal power is set correctly.
		for (CLLNode<TradeData>* pNode = pFirstList->head(); pNode; pNode = pFirstList->next(pNode))
		{
			if (isVassal(pNode->m_data.m_eItemType))
			{
				pFirstList->moveToEnd(pNode);
				break;
			}
		}
		// K-Mod end
		for (CLLNode<TradeData>* pNode = pSecondList->head(); pNode; pNode = pSecondList->next(pNode))
		{
			//  <advc.104> As above
			if(pNode->m_data.m_eItemType == TRADE_WAR)
				GET_PLAYER(getSecondPlayer()).warAndPeaceAI().getCache().
						reportSponsoredWar(*pFirstList, getFirstPlayer(),
						(TeamTypes)pNode->m_data.m_iData); // </advc.104>
			bool bSave = startTrade(pNode->m_data, getSecondPlayer(), getFirstPlayer());
			bBumpUnits = bBumpUnits || pNode->m_data.m_eItemType == TRADE_PEACE; // K-Mod

			if (bSave)
				insertAtEndSecondTrades(pNode->m_data);

			if (pNode->m_data.m_eItemType == TRADE_PERMANENT_ALLIANCE)
				bAlliance = true;
		}
	}

	if (bAlliance)
	{
		if (eFirstTeam < eSecondTeam)
		{
			GET_TEAM(eFirstTeam).addTeam(eSecondTeam);
		}
		else if (eSecondTeam < eFirstTeam)
		{
			GET_TEAM(eSecondTeam).addTeam(eFirstTeam);
		}
	}

	// K-Mod
	if (bBumpUnits)
	{
		GC.getMapINLINE().verifyUnitValidPlot();
	}
	// K-Mod end
}


void CvDeal::doTurn()
{
	int iValue;

	if (!isPeaceDeal()
		/*  <advc.130p> Open Borders and Defensive Pact have very small
			AI_dealVals. In (most?) other places, this doesn't matter b/c the AI
			never pays for these deals, but, here, it means that OB and DP
			have practically no impact on Grant and Trade values. This could be
			rectified by multiplying the return values of CvPlayerAI::
			AI_openBordersTradeVal and AI_defensivePactTradeVal by
			PEACE_TREATY_LENGTH, but I prefer to handle the issue entirely in the
			CvPlayerAI::AI_get...Attitude functions, and skip all dual deals here.
			In multiplayer, dual deals can be mixed with e.g. gold per turn,
			which is why I only skip single-item deals. For mixed multiplayer
			deals, OB and DP will be counted as some value between 0 and 5, which is
			a bit messy, not really a problem. */
			&& (getLengthSecondTrades() != getLengthFirstTrades() ||
			getLengthSecondTrades() > 1 ||
			!isDual(getFirstTrades()->head()->m_data.m_eItemType))) // </advc.130p>
	{
		if (getLengthSecondTrades() > 0)
		{
			iValue = (GET_PLAYER(getFirstPlayer()).AI_dealVal(getSecondPlayer(), getSecondTrades()) / GC.getDefineINT("PEACE_TREATY_LENGTH"));

			if (getLengthFirstTrades() > 0)
			{
				GET_PLAYER(getFirstPlayer()).AI_changePeacetimeTradeValue(getSecondPlayer(), iValue);
			}
			else
			{
				GET_PLAYER(getFirstPlayer()).AI_changePeacetimeGrantValue(getSecondPlayer(), iValue);
			}
		}

		if (getLengthFirstTrades() > 0)
		{
			iValue = (GET_PLAYER(getSecondPlayer()).AI_dealVal(getFirstPlayer(), getFirstTrades()) / GC.getDefineINT("PEACE_TREATY_LENGTH"));

			if (getLengthSecondTrades() > 0)
			{
				GET_PLAYER(getSecondPlayer()).AI_changePeacetimeTradeValue(getFirstPlayer(), iValue);
			}
			else
			{
				GET_PLAYER(getSecondPlayer()).AI_changePeacetimeGrantValue(getFirstPlayer(), iValue);
			}
		}
		// K-Mod note: for balance reasons this function should probably be called at the boundry of some particular player's turn,
		// rather than at the turn boundry of the game itself. -- Unfortunately, the game currently doesn't work like this.
		// Also, note that we do not update attitudes of particular players here, but instead update all of them at the game turn boundry.
	}
}


// XXX probably should have some sort of message for the user or something...
void CvDeal::verify()
{
	bool bCancelDeal = false;

	CvPlayer& kFirstPlayer = GET_PLAYER(getFirstPlayer());
	CvPlayer& kSecondPlayer = GET_PLAYER(getSecondPlayer());

	for (CLLNode<TradeData>* pNode = headFirstTradesNode(); (pNode != NULL); pNode = nextFirstTradesNode(pNode))
	{
		if (pNode->m_data.m_eItemType == TRADE_RESOURCES)
		{
			// XXX embargoes?
			if ((kFirstPlayer.getNumTradeableBonuses((BonusTypes)(pNode->m_data.m_iData)) < 0) ||
				  !(kFirstPlayer.canTradeNetworkWith(getSecondPlayer())) || 
				  GET_TEAM(kFirstPlayer.getTeam()).isBonusObsolete((BonusTypes) pNode->m_data.m_iData) || 
				  GET_TEAM(kSecondPlayer.getTeam()).isBonusObsolete((BonusTypes) pNode->m_data.m_iData))
			{
				bCancelDeal = true;
			}
		}
	}

	for (CLLNode<TradeData>* pNode = headSecondTradesNode(); (pNode != NULL); pNode = nextSecondTradesNode(pNode))
	{
		if (pNode->m_data.m_eItemType == TRADE_RESOURCES)
		{
			// XXX embargoes?
			if ((GET_PLAYER(getSecondPlayer()).getNumTradeableBonuses((BonusTypes)(pNode->m_data.m_iData)) < 0) ||
				  !(GET_PLAYER(getSecondPlayer()).canTradeNetworkWith(getFirstPlayer())) || 
				  GET_TEAM(kFirstPlayer.getTeam()).isBonusObsolete((BonusTypes) pNode->m_data.m_iData) || 
				  GET_TEAM(kSecondPlayer.getTeam()).isBonusObsolete((BonusTypes) pNode->m_data.m_iData))
			{
				bCancelDeal = true;
			}
		}
	}

	if (isCancelable(NO_PLAYER))
	{
		if (isPeaceDeal())
		{
			bCancelDeal = true;
		}
	}

	if (bCancelDeal)
	{
		kill();
	}
}


bool CvDeal::isPeaceDeal() const
{
	CLLNode<TradeData>* pNode;

	for (pNode = headFirstTradesNode(); (pNode != NULL); pNode = nextFirstTradesNode(pNode))
	{
		if (pNode->m_data.m_eItemType == getPeaceItem())
		{
			return true;
		}
	}

	for (pNode = headSecondTradesNode(); (pNode != NULL); pNode = nextSecondTradesNode(pNode))
	{
		if (pNode->m_data.m_eItemType == getPeaceItem())
		{
			return true;
		}
	}

	return false;
}

bool CvDeal::isVassalDeal() const
{
	return (isVassalTrade(&m_firstTrades) || isVassalTrade(&m_secondTrades));
}

bool CvDeal::isVassalTrade(const CLinkList<TradeData>* pList)
{
	if (pList)
	{
		for (CLLNode<TradeData>* pNode = pList->head(); pNode != NULL; pNode = pList->next(pNode))
		{
			if (isVassal(pNode->m_data.m_eItemType))
			{
				return true;
			}
		}
	}

	return false;
}


bool CvDeal::isUncancelableVassalDeal(PlayerTypes eByPlayer, CvWString* pszReason) const
{
	CLLNode<TradeData>* pNode;
	CvWStringBuffer szBuffer;

	for (pNode = headFirstTradesNode(); (pNode != NULL); pNode = nextFirstTradesNode(pNode))
	{
		if (isVassal(pNode->m_data.m_eItemType))
		{
			if (eByPlayer == getSecondPlayer())
			{
				if (pszReason)
				{
					*pszReason += gDLL->getText("TXT_KEY_MISC_DEAL_NO_CANCEL_EVER");
				}

				return true;
			}
		}

		if (pNode->m_data.m_eItemType == TRADE_SURRENDER)
		{
			CvTeam& kVassal = GET_TEAM(GET_PLAYER(getFirstPlayer()).getTeam());
			TeamTypes eMaster = GET_PLAYER(getSecondPlayer()).getTeam();
			
			if (!kVassal.canVassalRevolt(eMaster))
			{
				if (pszReason)
				{
					szBuffer.clear();
					GAMETEXT.setVassalRevoltHelp(szBuffer, eMaster, GET_PLAYER(getFirstPlayer()).getTeam());
					*pszReason = szBuffer.getCString();
				}

				return true;
			}
		}
	}

	for (pNode = headSecondTradesNode(); (pNode != NULL); pNode = nextSecondTradesNode(pNode))
	{
		if (isVassal(pNode->m_data.m_eItemType))
		{
			if (eByPlayer == getFirstPlayer())
			{
				if (pszReason)
				{
					*pszReason += gDLL->getText("TXT_KEY_MISC_DEAL_NO_CANCEL_EVER");
				}

				return true;
			}
		}

		if (pNode->m_data.m_eItemType == TRADE_SURRENDER)
		{
			CvTeam& kVassal = GET_TEAM(GET_PLAYER(getSecondPlayer()).getTeam());
			TeamTypes eMaster = GET_PLAYER(getFirstPlayer()).getTeam();
			
			if (!kVassal.canVassalRevolt(eMaster))
			{
				// kmodx: Redundant code removed
				if (pszReason)
				{
					szBuffer.clear();
					GAMETEXT.setVassalRevoltHelp(szBuffer, eMaster, GET_PLAYER(getFirstPlayer()).getTeam());
					*pszReason = szBuffer.getCString();
				}

				return true;
			}
		}
	}

	return false;
}

bool CvDeal::isVassalTributeDeal(const CLinkList<TradeData>* pList)
{
	for (CLLNode<TradeData>* pNode = pList->head(); pNode != NULL; pNode = pList->next(pNode))
	{
		if (pNode->m_data.m_eItemType != TRADE_RESOURCES)
		{
			return false;
		}
	}

	return true;
}

bool CvDeal::isPeaceDealBetweenOthers(CLinkList<TradeData>* pFirstList, CLinkList<TradeData>* pSecondList) const
{
	CLLNode<TradeData>* pNode;

	if (pFirstList != NULL)
	{
		for (pNode = pFirstList->head(); pNode; pNode = pFirstList->next(pNode))
		{
			if (pNode->m_data.m_eItemType == TRADE_PEACE)
			{
				return true;
			}
		}
	}

	if (pSecondList != NULL)
	{
		for (pNode = pSecondList->head(); pNode; pNode = pSecondList->next(pNode))
		{
			if (pNode->m_data.m_eItemType == TRADE_PEACE)
			{
				return true;
			}
		}
	}

	return false;
}


int CvDeal::getID() const
{
	return m_iID;
}


void CvDeal::setID(int iID)
{
	m_iID = iID;
}


int CvDeal::getInitialGameTurn() const
{
	return m_iInitialGameTurn;
}


void CvDeal::setInitialGameTurn(int iNewValue)
{
	m_iInitialGameTurn = iNewValue;
}


PlayerTypes CvDeal::getFirstPlayer() const
{
	return m_eFirstPlayer;
}


PlayerTypes CvDeal::getSecondPlayer() const
{
	return m_eSecondPlayer;
}


void CvDeal::clearFirstTrades()
{
	m_firstTrades.clear();
}


void CvDeal::insertAtEndFirstTrades(TradeData trade)
{
	m_firstTrades.insertAtEnd(trade);
}


CLLNode<TradeData>* CvDeal::nextFirstTradesNode(CLLNode<TradeData>* pNode) const
{
	return m_firstTrades.next(pNode);
}


int CvDeal::getLengthFirstTrades() const
{
	return m_firstTrades.getLength();
}


CLLNode<TradeData>* CvDeal::headFirstTradesNode() const
{
	return m_firstTrades.head();
}


const CLinkList<TradeData>* CvDeal::getFirstTrades() const
{
	return &(m_firstTrades);
}


void CvDeal::clearSecondTrades()
{
	m_secondTrades.clear();
}


void CvDeal::insertAtEndSecondTrades(TradeData trade)
{
	m_secondTrades.insertAtEnd(trade);
}


CLLNode<TradeData>* CvDeal::nextSecondTradesNode(CLLNode<TradeData>* pNode) const
{
	return m_secondTrades.next(pNode);
}


int CvDeal::getLengthSecondTrades() const
{
	return m_secondTrades.getLength();
}


CLLNode<TradeData>* CvDeal::headSecondTradesNode() const
{
	return m_secondTrades.head();
}


const CLinkList<TradeData>* CvDeal::getSecondTrades() const
{
	return &(m_secondTrades);
}


void CvDeal::write(FDataStreamBase* pStream)
{
	uint uiFlag=0;
	pStream->Write(uiFlag);		// flag for expansion

	pStream->Write(m_iID);
	pStream->Write(m_iInitialGameTurn);

	pStream->Write(m_eFirstPlayer);
	pStream->Write(m_eSecondPlayer);

	m_firstTrades.Write(pStream);
	m_secondTrades.Write(pStream);
}

void CvDeal::read(FDataStreamBase* pStream)
{
	uint uiFlag=0;
	pStream->Read(&uiFlag);	// flags for expansion

	pStream->Read(&m_iID);
	pStream->Read(&m_iInitialGameTurn);

	pStream->Read((int*)&m_eFirstPlayer);
	pStream->Read((int*)&m_eSecondPlayer);

	m_firstTrades.Read(pStream);
	m_secondTrades.Read(pStream);
}

// Protected Functions...

// Returns true if the trade should be saved...
bool CvDeal::startTrade(TradeData trade, PlayerTypes eFromPlayer, PlayerTypes eToPlayer)
{
	CivicTypes* paeNewCivics;
	CvCity* pCity;
	CvPlot* pLoopPlot;
	bool bSave;
	int iI;

	bSave = false;

	switch (trade.m_eItemType)
	{
	case TRADE_TECHNOLOGIES:
	{
		// K-Mod only adjust tech_from_any memory if this is a tech from a recent era
		// and the team receiving the tech isn't already more than 2/3 of the way through.
		// (This is to prevent the AI from being crippled by human players selling them lots of tech scraps.)
		// Note: the current game era is the average of all the player eras, rounded down. (It no longer includes barbs.)
		bool bSignificantTech =
			GC.getTechInfo((TechTypes)trade.m_iData).getEra() >= GC.getGame().getCurrentEra()-1 &&
			GET_TEAM(GET_PLAYER(eToPlayer).getTeam()).getResearchLeft((TechTypes)trade.m_iData) > GET_TEAM(GET_PLAYER(eToPlayer).getTeam()).getResearchCost((TechTypes)trade.m_iData) / 3;
		// K-Mod end
		GET_TEAM(GET_PLAYER(eToPlayer).getTeam()).setHasTech(((TechTypes)trade.m_iData), true, eToPlayer, true, true);
		GET_TEAM(GET_PLAYER(eToPlayer).getTeam()).setNoTradeTech(((TechTypes)trade.m_iData), true);

		if( gTeamLogLevel >= 2 )
		{
			logBBAI("    Player %d (%S) trades tech %S to player %d (%S)", eFromPlayer, GET_PLAYER(eFromPlayer).getCivilizationDescription(0), GC.getTechInfo((TechTypes)trade.m_iData).getDescription(), eToPlayer, GET_PLAYER(eToPlayer).getCivilizationDescription(0) );
		}

		for (iI = 0; iI < MAX_PLAYERS; iI++)
		{
			if (GET_PLAYER((PlayerTypes)iI).isAlive())
			{
				if (GET_PLAYER((PlayerTypes)iI).getTeam() == GET_PLAYER(eToPlayer).getTeam())
				{   // advc.130j:
					GET_PLAYER((PlayerTypes)iI).AI_rememberEvent(eFromPlayer, MEMORY_TRADED_TECH_TO_US);
				}
				//else
				else if (bSignificantTech) // K-Mod
				{
					if (GET_TEAM(GET_PLAYER((PlayerTypes)iI).getTeam()).isHasMet(GET_PLAYER(eToPlayer).getTeam()))
					{   // advc.130j:
						GET_PLAYER((PlayerTypes)iI).AI_rememberEvent(eToPlayer, MEMORY_RECEIVED_TECH_FROM_ANY);
					}
				}
			}
		}
		break;
	}

	case TRADE_RESOURCES:
		GET_PLAYER(eFromPlayer).changeBonusExport(((BonusTypes)trade.m_iData), 1);
		GET_PLAYER(eToPlayer).changeBonusImport(((BonusTypes)trade.m_iData), 1);
		if( gTeamLogLevel >= 2 )
		{
			logBBAI("    Player %d (%S) trades bonus type %S due to TRADE_RESOURCES with %d (%S)", eFromPlayer, GET_PLAYER(eFromPlayer).getCivilizationDescription(0), GC.getBonusInfo((BonusTypes)trade.m_iData).getDescription(), eToPlayer, GET_PLAYER(eToPlayer).getCivilizationDescription(0) );
		}
		bSave = true;
		break;

	case TRADE_CITIES:
		pCity = GET_PLAYER(eFromPlayer).getCity(trade.m_iData);
		if (pCity != NULL)
		{
			if( gTeamLogLevel >= 2 )
			{
				logBBAI("    Player %d (%S) gives a city due to TRADE_CITIES with %d (%S)", eFromPlayer, GET_PLAYER(eFromPlayer).getCivilizationDescription(0), eToPlayer, GET_PLAYER(eToPlayer).getCivilizationDescription(0) );
			}
			pCity->doTask(TASK_GIFT, eToPlayer);
		}
		break;

	case TRADE_GOLD:
		GET_PLAYER(eFromPlayer).changeGold(-(trade.m_iData));
		GET_PLAYER(eToPlayer).changeGold(trade.m_iData);
		GET_PLAYER(eFromPlayer).AI_changeGoldTradedTo(eToPlayer, trade.m_iData);

		if( gTeamLogLevel >= 2 )
		{
			logBBAI("    Player %d (%S) trades gold %d due to TRADE_GOLD with player %d (%S)", eFromPlayer, GET_PLAYER(eFromPlayer).getCivilizationDescription(0), trade.m_iData, eToPlayer, GET_PLAYER(eToPlayer).getCivilizationDescription(0) );
		}

		// Python Event
		CvEventReporter::getInstance().playerGoldTrade(eFromPlayer, eToPlayer, trade.m_iData);

		break;

	case TRADE_GOLD_PER_TURN:
		GET_PLAYER(eFromPlayer).changeGoldPerTurnByPlayer(eToPlayer, -(trade.m_iData));
		GET_PLAYER(eToPlayer).changeGoldPerTurnByPlayer(eFromPlayer, trade.m_iData);

		if( gTeamLogLevel >= 2 )
		{
			logBBAI("    Player %d (%S) trades gold per turn %d due to TRADE_GOLD_PER_TURN with player %d (%S)", eFromPlayer, GET_PLAYER(eFromPlayer).getCivilizationDescription(0), trade.m_iData, eToPlayer, GET_PLAYER(eToPlayer).getCivilizationDescription(0) );
		}

		bSave = true;
		break;

	case TRADE_MAPS:
		for (iI = 0; iI < GC.getMapINLINE().numPlotsINLINE(); iI++)
		{
			pLoopPlot = GC.getMapINLINE().plotByIndexINLINE(iI);

			if (pLoopPlot->isRevealed(GET_PLAYER(eFromPlayer).getTeam(), false))
			{
				pLoopPlot->setRevealed(GET_PLAYER(eToPlayer).getTeam(), true, false, GET_PLAYER(eFromPlayer).getTeam(), false);
			}
		}

		for (iI = 0; iI < MAX_PLAYERS; iI++) 
		{ 
			if (GET_PLAYER((PlayerTypes)iI).isAlive()) 
			{ 
				if (GET_PLAYER((PlayerTypes)iI).getTeam() == GET_PLAYER(eToPlayer).getTeam()) 
				{ 
					GET_PLAYER((PlayerTypes)iI).updatePlotGroups(); 
				} 
			} 
		} 

		if( gTeamLogLevel >= 2 )
		{
			logBBAI("    Player %d (%S) trades maps due to TRADE_MAPS with player %d (%S)", eFromPlayer, GET_PLAYER(eFromPlayer).getCivilizationDescription(0), eToPlayer, GET_PLAYER(eToPlayer).getCivilizationDescription(0) );
		}
		break;

	case TRADE_SURRENDER:
	case TRADE_VASSAL:
		if (trade.m_iData == 0)
		{
			startTeamTrade(trade.m_eItemType, GET_PLAYER(eFromPlayer).getTeam(), GET_PLAYER(eToPlayer).getTeam(), false);
			GET_TEAM(GET_PLAYER(eFromPlayer).getTeam()).setVassal(GET_PLAYER(eToPlayer).getTeam(), true, TRADE_SURRENDER == trade.m_eItemType);
			if( gTeamLogLevel >= 2 )
			{
				if( TRADE_SURRENDER == trade.m_eItemType )
				{
					logBBAI("    Player %d (%S) trades themselves as vassal due to TRADE_SURRENDER with player %d (%S)", eFromPlayer, GET_PLAYER(eFromPlayer).getCivilizationDescription(0), eToPlayer, GET_PLAYER(eToPlayer).getCivilizationDescription(0) );
				}
				else
				{
					logBBAI("    Player %d (%S) trades themselves as vassal due to TRADE_VASSAL with player %d (%S)", eFromPlayer, GET_PLAYER(eFromPlayer).getCivilizationDescription(0), eToPlayer, GET_PLAYER(eToPlayer).getCivilizationDescription(0) );
				}
			}
		}
		else
		{
			bSave = true;
		}


		break;

	case TRADE_PEACE:
		if( gTeamLogLevel >= 2 )
		{
			logBBAI("    Team %d (%S) makes peace with team %d due to TRADE_PEACE with %d (%S)", GET_PLAYER(eFromPlayer).getTeam(), GET_PLAYER(eFromPlayer).getCivilizationDescription(0), trade.m_iData, eToPlayer, GET_PLAYER(eToPlayer).getCivilizationDescription(0) );
		}
		//GET_TEAM(GET_PLAYER(eFromPlayer).getTeam()).makePeace((TeamTypes)trade.m_iData);
		// K-Mod. (units will be bumped after the rest of the trade deals are completed.)
		// <advc.100b>
		TEAMREF(eFromPlayer).makePeaceBulk((TeamTypes)trade.m_iData, false, TEAMID(eToPlayer));
		TEAMREF(eFromPlayer).signPeaceTreaty((TeamTypes)trade.m_iData); // K-Mod. Use a standard peace treaty rather than a simple cease-fire.
		// </advc.100b>
		// K-Mod todo: this team should offer something fair to the peace-team if this teams endWarVal is higher.
		break;

	case TRADE_WAR:
		if( gTeamLogLevel >= 2 )
		{
			logBBAI("    Team %d (%S) declares war on team %d due to TRADE_WAR with %d (%S)", GET_PLAYER(eFromPlayer).getTeam(), GET_PLAYER(eFromPlayer).getCivilizationDescription(0), trade.m_iData, eToPlayer, GET_PLAYER(eToPlayer).getCivilizationDescription(0) );
		}
		TEAMREF(eFromPlayer).declareWar(((TeamTypes)trade.m_iData), true, NO_WARPLAN,
				true, eToPlayer); // advc.100
		// advc.146:
		TEAMREF(eFromPlayer).signPeaceTreaty(TEAMID(eToPlayer));
		for (iI = 0; iI < MAX_CIV_PLAYERS; iI++)
		{
			CvPlayerAI& attacked = GET_PLAYER((PlayerTypes)iI); // <advc.003>
			if(!attacked.isAlive() || attacked.getTeam() != (TeamTypes)trade.m_iData)
				continue; // </advc.003>
			// advc.130j:
			attacked.AI_rememberEvent(eToPlayer, MEMORY_HIRED_WAR_ALLY);
			// advc.104i: Same code as in CvPlayer::handleDiploEvent
			attacked.AI_changeMemoryCount(eFromPlayer, MEMORY_STOPPED_TRADING_RECENT, 1);
		}
		break;

	case TRADE_EMBARGO:
		GET_PLAYER(eFromPlayer).stopTradingWithTeam((TeamTypes)trade.m_iData);
		/*  <advc.130f> The instigator needs to stop too. (If an AI suggested
			the embargo to a human, that's not handled here but by
			CvPlayer::handleDiploEvent.) */
		if(!TEAMREF(eFromPlayer).isCapitulated() || !TEAMREF(eFromPlayer).isVassal(TEAMID(eToPlayer)))
			GET_PLAYER(eToPlayer).stopTradingWithTeam((TeamTypes)trade.m_iData, false);
		// </advc,130f>
		for (iI = 0; iI < MAX_PLAYERS; iI++)
		{
			if (GET_PLAYER((PlayerTypes)iI).isAlive())
			{
				if (GET_PLAYER((PlayerTypes)iI).getTeam() == ((TeamTypes)trade.m_iData))
				{   // advc.130j:
					GET_PLAYER((PlayerTypes)iI).AI_rememberEvent(eToPlayer, MEMORY_HIRED_TRADE_EMBARGO);
				}
			}
		}
		if( gTeamLogLevel >= 2 )
		{
			logBBAI("    Player %d (%S) signs embargo against team %d due to TRADE_EMBARGO with player %d (%S)", eFromPlayer, GET_PLAYER(eFromPlayer).getCivilizationDescription(0), (TeamTypes)trade.m_iData, eToPlayer, GET_PLAYER(eToPlayer).getCivilizationDescription(0) );
		}
		// advc.130f:
		GET_PLAYER(eToPlayer).stopTradingWithTeam((TeamTypes)trade.m_iData);
		break;

	case TRADE_CIVIC:
		paeNewCivics = new CivicTypes[GC.getNumCivicOptionInfos()];

		for (iI = 0; iI < GC.getNumCivicOptionInfos(); iI++)
		{
			paeNewCivics[iI] = GET_PLAYER(eFromPlayer).getCivics((CivicOptionTypes)iI);
		}

		paeNewCivics[GC.getCivicInfo((CivicTypes)trade.m_iData).getCivicOptionType()] = ((CivicTypes)trade.m_iData);

		GET_PLAYER(eFromPlayer).revolution(paeNewCivics, true);

		if (GET_PLAYER(eFromPlayer).AI_getCivicTimer() < GC.getDefineINT("PEACE_TREATY_LENGTH"))
		{
			GET_PLAYER(eFromPlayer).AI_setCivicTimer(GC.getDefineINT("PEACE_TREATY_LENGTH"));
		}
		if( gTeamLogLevel >= 2 )
		{
			logBBAI("    Player %d (%S) switched civics due to TRADE_CIVICS with player %d (%S)", eFromPlayer, GET_PLAYER(eFromPlayer).getCivilizationDescription(0), eToPlayer, GET_PLAYER(eToPlayer).getCivilizationDescription(0) );
		}

		SAFE_DELETE_ARRAY(paeNewCivics);
		break;

	case TRADE_RELIGION:
		GET_PLAYER(eFromPlayer).convert((ReligionTypes)trade.m_iData);

		if (GET_PLAYER(eFromPlayer).AI_getReligionTimer() < GC.getDefineINT("PEACE_TREATY_LENGTH"))
		{
			GET_PLAYER(eFromPlayer).AI_setReligionTimer(GC.getDefineINT("PEACE_TREATY_LENGTH"));
		}
		if( gTeamLogLevel >= 2 )
		{
			logBBAI("    Player %d (%S) switched religions due to TRADE_RELIGION with player %d (%S)", eFromPlayer, GET_PLAYER(eFromPlayer).getCivilizationDescription(0), eToPlayer, GET_PLAYER(eToPlayer).getCivilizationDescription(0) );
		}
		break;

	case TRADE_OPEN_BORDERS:
		if (trade.m_iData == 0)
		{
			startTeamTrade(TRADE_OPEN_BORDERS, GET_PLAYER(eFromPlayer).getTeam(), GET_PLAYER(eToPlayer).getTeam(), true);
			GET_TEAM(GET_PLAYER(eFromPlayer).getTeam()).setOpenBorders(((TeamTypes)(GET_PLAYER(eToPlayer).getTeam())), true);
			if( gTeamLogLevel >= 2 )
			{
				logBBAI("    Player %d (%S_1) signs open borders due to TRADE_OPEN_BORDERS with player %d (%S_2)", eFromPlayer, GET_PLAYER(eFromPlayer).getCivilizationDescription(0), eToPlayer, GET_PLAYER(eToPlayer).getCivilizationDescription(0) );
			}
		}
		else
		{
			bSave = true;
		}
		break;

	case TRADE_DEFENSIVE_PACT:
		if (trade.m_iData == 0)
		{
			startTeamTrade(TRADE_DEFENSIVE_PACT, GET_PLAYER(eFromPlayer).getTeam(), GET_PLAYER(eToPlayer).getTeam(), true);
			GET_TEAM(GET_PLAYER(eFromPlayer).getTeam()).setDefensivePact(((TeamTypes)(GET_PLAYER(eToPlayer).getTeam())), true);
			if( gTeamLogLevel >= 2 )
			{
				logBBAI("    Player %d (%S) signs defensive pact due to TRADE_DEFENSIVE_PACT with player %d (%S)", eFromPlayer, GET_PLAYER(eFromPlayer).getCivilizationDescription(0), eToPlayer, GET_PLAYER(eToPlayer).getCivilizationDescription(0) );
			}
		}
		else
		{
			bSave = true;
		}
		break;

	case TRADE_PERMANENT_ALLIANCE:
		break;

	case TRADE_PEACE_TREATY:
		GET_TEAM(GET_PLAYER(eFromPlayer).getTeam()).setForcePeace(((TeamTypes)(GET_PLAYER(eToPlayer).getTeam())), true);
		if( gTeamLogLevel >= 2 )
		{
			logBBAI("    Player %d (%S) signs peace treaty due to TRADE_PEACE_TREATY with player %d (%S)", eFromPlayer, GET_PLAYER(eFromPlayer).getCivilizationDescription(0), eToPlayer, GET_PLAYER(eToPlayer).getCivilizationDescription(0) );
		}
		bSave = true;
		break;

	default:
		FAssert(false);
		break;
	}

	return bSave;
}

// <advc.003> Refactored this function
void CvDeal::endTrade(TradeData trade, PlayerTypes eFromPlayer,
		PlayerTypes eToPlayer, bool bTeam) {

	bool teamTradeEnded = false; // advc.133
	switch(trade.m_eItemType) {
	case TRADE_RESOURCES:
		GET_PLAYER(eToPlayer).changeBonusImport(((BonusTypes)trade.m_iData), -1);
		GET_PLAYER(eFromPlayer).changeBonusExport(((BonusTypes)trade.m_iData), -1);
		break;

	case TRADE_GOLD_PER_TURN:
		GET_PLAYER(eFromPlayer).changeGoldPerTurnByPlayer(eToPlayer, trade.m_iData);
		GET_PLAYER(eToPlayer).changeGoldPerTurnByPlayer(eFromPlayer, -(trade.m_iData));
		break;

	// <advc.143>
	case TRADE_VASSAL:
	case TRADE_SURRENDER: {
		bool bSurrender = (trade.m_eItemType == TRADE_SURRENDER);
		// Canceled b/c of failure to protect vassal?
		bool deniedHelp = false;
		if(bSurrender)
			deniedHelp = (TEAMREF(eFromPlayer).isLossesAllowRevolt(TEAMID(eToPlayer))
					// Doesn't count if losses obviously only from cultural borders
					&& TEAMREF(eFromPlayer).getAnyWarPlanCount(true) > 0);
		else {
			DenialTypes reason = TEAMREF(eFromPlayer).
					AI_surrenderTrade(TEAMID(eToPlayer));
			deniedHelp = (reason == DENIAL_POWER_YOUR_ENEMIES);
		}
		TEAMREF(eFromPlayer).setVassal(TEAMID(eToPlayer), false, bSurrender);
		if(bTeam) {
			if(bSurrender)
				endTeamTrade(TRADE_SURRENDER, TEAMID(eFromPlayer), TEAMID(eToPlayer));
			else endTeamTrade(TRADE_VASSAL, TEAMID(eFromPlayer), TEAMID(eToPlayer));
			teamTradeEnded = true; // advc.133
		}
		addEndTradeMemory(eFromPlayer, eToPlayer, TRADE_VASSAL);
		if(!deniedHelp) // Master remembers for 2x10 turns
			addEndTradeMemory(eFromPlayer, eToPlayer, TRADE_VASSAL);
		else { /* Vassal remembers for 3x10 turns (and master for 1x10 turns,
				  which could matter when a human becomes a vassal) */
			for(int i = 0; i < 3; i++)
				addEndTradeMemory(eToPlayer, eFromPlayer, TRADE_VASSAL);
		}
		break;
	} // </advc.143>
	case TRADE_OPEN_BORDERS:
		TEAMREF(eFromPlayer).setOpenBorders(TEAMID(eToPlayer), false);
		if(bTeam) {
			endTeamTrade(TRADE_OPEN_BORDERS, TEAMID(eFromPlayer), TEAMID(eToPlayer));
			teamTradeEnded = true; // advc.133
			// advc.130p:
			addEndTradeMemory(eFromPlayer, eToPlayer, TRADE_OPEN_BORDERS);
		}
		break;

	case TRADE_DEFENSIVE_PACT:
		TEAMREF(eFromPlayer).setDefensivePact(TEAMID(eToPlayer), false);
		if(bTeam) {
			endTeamTrade(TRADE_DEFENSIVE_PACT, TEAMID(eFromPlayer), TEAMID(eToPlayer));
			teamTradeEnded = true; // advc.133
			// advc.130p:
			addEndTradeMemory(eFromPlayer, eToPlayer, TRADE_DEFENSIVE_PACT);
		}
		break;

	case TRADE_PEACE_TREATY:
		TEAMREF(eFromPlayer).setForcePeace(TEAMID(eToPlayer), false);
		break;

	default: FAssert(false);
	}
	// </advc.003>
	// <advc.133> (I think this is needed even w/o change 133 canceling more deals)
	if(!teamTradeEnded)
		GET_PLAYER(eFromPlayer).AI_updateAttitudeCache(eToPlayer);
	else TEAMREF(eFromPlayer).AI_updateAttitudeCache(TEAMID(eToPlayer));
	// </advc.133>
}

// <advc.130p> Cut-and-pasted from CvDeal::endTrade
void CvDeal::addEndTradeMemory(PlayerTypes eFromPlayer, PlayerTypes eToPlayer,
		TradeableItems dealType) {

	for(int i = 0; i < MAX_CIV_PLAYERS; i++) {
		PlayerTypes eToTeamMember = (PlayerTypes)i;
		if(!GET_PLAYER(eToTeamMember).isAlive() ||
				TEAMID(eToTeamMember) != TEAMID(eToPlayer))
			continue;
		for(int j = 0; j < MAX_CIV_PLAYERS; j++) {
			PlayerTypes eFromTeamMember = (PlayerTypes)j;
			if(!GET_PLAYER(eFromTeamMember).isAlive() ||
				TEAMID(eFromTeamMember) != TEAMID(eFromPlayer))
			continue;
			MemoryTypes mem = MEMORY_CANCELLED_OPEN_BORDERS;
			if(dealType == TRADE_DEFENSIVE_PACT)
				mem = MEMORY_CANCELLED_DEFENSIVE_PACT;
			else if(dealType == TRADE_VASSAL)
				mem = MEMORY_CANCELLED_VASSAL_AGREEMENT;
			// advc.130j:
			GET_PLAYER(eToTeamMember).AI_rememberEvent(eFromTeamMember, mem);
		}
	}
}
// </advc.130p>

void CvDeal::startTeamTrade(TradeableItems eItem, TeamTypes eFromTeam, TeamTypes eToTeam, bool bDual)
{
	for (int iI = 0; iI < MAX_PLAYERS; iI++)
	{
		CvPlayer& kLoopFromPlayer = GET_PLAYER((PlayerTypes)iI);
		if (kLoopFromPlayer.isAlive() )
		{
			if (kLoopFromPlayer.getTeam() == eFromTeam)
			{
				for (int iJ = 0; iJ < MAX_PLAYERS; iJ++)
				{
					CvPlayer& kLoopToPlayer = GET_PLAYER((PlayerTypes)iJ);
					if (kLoopToPlayer.isAlive())
					{
						if (kLoopToPlayer.getTeam() == eToTeam)
						{
							TradeData item;
							setTradeItem(&item, eItem, 1);
							CLinkList<TradeData> ourList;
							ourList.insertAtEnd(item);
							CLinkList<TradeData> theirList;
							if (bDual)
							{
								theirList.insertAtEnd(item);
							}
							GC.getGame().implementDeal((PlayerTypes)iI, (PlayerTypes)iJ, &ourList, &theirList);
						}
					}
				}
			}
		}
	}
}

void CvDeal::endTeamTrade(TradeableItems eItem, TeamTypes eFromTeam, TeamTypes eToTeam)
{
	CvDeal* pLoopDeal;
	int iLoop;
	CLLNode<TradeData>* pNode;

	for (pLoopDeal = GC.getGameINLINE().firstDeal(&iLoop); pLoopDeal != NULL; pLoopDeal = GC.getGameINLINE().nextDeal(&iLoop))
	{
		if (pLoopDeal != this)
		{
			bool bValid = true;

			if (GET_PLAYER(pLoopDeal->getFirstPlayer()).getTeam() == eFromTeam && GET_PLAYER(pLoopDeal->getSecondPlayer()).getTeam() == eToTeam)
			{
				if (pLoopDeal->getFirstTrades())
				{
					for (pNode = pLoopDeal->getFirstTrades()->head(); pNode; pNode = pLoopDeal->getFirstTrades()->next(pNode))
					{
						if (pNode->m_data.m_eItemType == eItem)
						{
							bValid = false;
						}
					}
				}
			}

			if (bValid && GET_PLAYER(pLoopDeal->getFirstPlayer()).getTeam() == eToTeam && GET_PLAYER(pLoopDeal->getSecondPlayer()).getTeam() == eFromTeam)
			{
				if (pLoopDeal->getSecondTrades() != NULL)
				{
					for (pNode = pLoopDeal->getSecondTrades()->head(); pNode; pNode = pLoopDeal->getSecondTrades()->next(pNode))
					{
						if (pNode->m_data.m_eItemType == eItem)
						{
							bValid = false;
						}
					}
				}
			}

			if (!bValid)
			{
				pLoopDeal->kill(false);
			}
		}
	}
}

bool CvDeal::isCancelable(PlayerTypes eByPlayer, CvWString* pszReason)
{
	if (isUncancelableVassalDeal(eByPlayer, pszReason))
	{
		return false;
	}

	int iTurns = turnsToCancel(eByPlayer);
	if (pszReason)
	{
		if (iTurns > 0)
		{
			*pszReason = gDLL->getText("TXT_KEY_MISC_DEAL_NO_CANCEL", iTurns);
		}
	}

	return (iTurns <= 0);
}

/*  <advc.130f> Based on isCancelable; doesn't check turnsToCancel.
	See declaration in CvDeal.h. */
bool CvDeal::isEverCancelable(PlayerTypes eByPlayer) const {

	// I don't think isUncancellableVassalDeal covers this:
	if(getFirstPlayer() != eByPlayer && getSecondPlayer() != eByPlayer)
		return false;
	return !isUncancelableVassalDeal(eByPlayer);
} // </advc.130f>

int CvDeal::turnsToCancel(PlayerTypes eByPlayer)
{
	return (getInitialGameTurn() + GC.getDefineINT("PEACE_TREATY_LENGTH") - GC.getGameINLINE().getGameTurn());
}

// static
bool CvDeal::isAnnual(TradeableItems eItem)
{
	switch (eItem)
	{
	case TRADE_RESOURCES:
	case TRADE_GOLD_PER_TURN:
	case TRADE_VASSAL:
	case TRADE_SURRENDER:
	case TRADE_OPEN_BORDERS:
	case TRADE_DEFENSIVE_PACT:
	case TRADE_PERMANENT_ALLIANCE:
		return true;
		break;
	}
	
	return false;
}

// static
bool CvDeal::isDual(TradeableItems eItem, bool bExcludePeace)
{
	switch (eItem)
	{
	case TRADE_OPEN_BORDERS:
	case TRADE_DEFENSIVE_PACT:
	case TRADE_PERMANENT_ALLIANCE:
		return true;
	case TRADE_PEACE_TREATY:
		return (!bExcludePeace);
	}

	return false;
}

// static
bool CvDeal::hasData(TradeableItems eItem)
{
	return (eItem != TRADE_MAPS &&
		eItem != TRADE_VASSAL &&
		eItem != TRADE_SURRENDER &&
		eItem != TRADE_OPEN_BORDERS &&
		eItem != TRADE_DEFENSIVE_PACT &&
		eItem != TRADE_PERMANENT_ALLIANCE &&
		eItem != TRADE_PEACE_TREATY);
}

bool CvDeal::isGold(TradeableItems eItem)
{
	return (eItem == getGoldItem() || eItem == getGoldPerTurnItem());
}

bool CvDeal::isVassal(TradeableItems eItem)
{
	return (eItem == TRADE_VASSAL || eItem == TRADE_SURRENDER);
}

bool CvDeal::isEndWar(TradeableItems eItem)
{
	if (eItem == getPeaceItem())
	{
		return true;
	}

	if (isVassal(eItem))
	{
		return true;
	}

	return false;
}

TradeableItems CvDeal::getPeaceItem()
{
	return TRADE_PEACE_TREATY;
}

TradeableItems CvDeal::getGoldItem()
{
	return TRADE_GOLD;
}

TradeableItems CvDeal::getGoldPerTurnItem()
{
	return TRADE_GOLD_PER_TURN;
}


