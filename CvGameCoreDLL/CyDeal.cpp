//
// Python wrapper class for CvGame
//

#include "CvGameCoreDLL.h"
#include "CyDeal.h"
#include "CvDeal.h"

CyDeal::CyDeal(CvDeal* pDeal) : m_pDeal(pDeal) {}

CyDeal::~CyDeal() {}

bool CyDeal::isNone()
{
	return (m_pDeal == NULL);
}

int CyDeal::getID() const
{
	return (m_pDeal ? m_pDeal->getID() : -1);
}

int CyDeal::getInitialGameTurn() const
{
	return (m_pDeal ? m_pDeal->getInitialGameTurn() : -1);
}

int CyDeal::getFirstPlayer() const
{
	return (m_pDeal ? m_pDeal->getFirstPlayer() : -1);
}

int CyDeal::getSecondPlayer() const
{
	return (m_pDeal ? m_pDeal->getSecondPlayer() : -1);
}

int CyDeal::getLengthFirstTrades() const
{
	return (m_pDeal ? m_pDeal->getLengthFirst() : 0);
}

int CyDeal::getLengthSecondTrades() const
{
	return (m_pDeal ? m_pDeal->getLengthSecond() : 0);
}

TradeData* CyDeal::getFirstTrade(int i) const
{
	if (i >= getLengthFirstTrades() || m_pDeal == NULL)
		return NULL;
	int iCount = 0;
	FOR_EACH_TRADE_ITEM_VAR(m_pDeal->getFirstListVar())
	{
		if (iCount == i)
			return pItem;
		iCount++;
	}
	return NULL;
}

TradeData* CyDeal::getSecondTrade(int i) const
{
	if (i >= getLengthSecondTrades() || m_pDeal == NULL)
		return NULL;
	int iCount = 0;
	FOR_EACH_TRADE_ITEM_VAR(m_pDeal->getSecondListVar())
	{
		if (iCount == i)
			return pItem;
		iCount++;
	}
	return NULL;
}

void CyDeal::kill()
{
	if (m_pDeal != NULL)
		m_pDeal->kill();
}
