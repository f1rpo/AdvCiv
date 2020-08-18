// plotGroup.cpp

#include "CvGameCoreDLL.h"
#include "CvPlotGroup.h"
#include "CvPlayer.h"
#include "CvCity.h"
#include "CvMap.h"

int CvPlotGroup::m_iRecalculating = 0; // advc.064d


CvPlotGroup::CvPlotGroup()
{
	reset(0, NO_PLAYER, true);
}


CvPlotGroup::~CvPlotGroup()
{
	uninit();
}


void CvPlotGroup::init(int iID, PlayerTypes eOwner, CvPlot* pPlot)
{
	reset(iID, eOwner);

	addPlot(pPlot);
}


void CvPlotGroup::uninit()
{
	m_plots.clear();
}

// FUNCTION: reset()
// Initializes data members that are serialized.
void CvPlotGroup::reset(int iID, PlayerTypes eOwner, bool bConstructorCall)
{
	uninit();

	m_iID = iID;
	m_eOwner = eOwner;

	if (!bConstructorCall)
		m_aiNumBonuses.reset();
}


void CvPlotGroup::addPlot(CvPlot* pPlot, /* advc.064d: */ bool bVerifyProduction)
{
	XYCoords xy;
	xy.iX = pPlot->getX();
	xy.iY = pPlot->getY();
	insertAtEndPlots(xy);
	pPlot->setPlotGroup(getOwner(), this, /* advc.064d: */ bVerifyProduction);
}


void CvPlotGroup::removePlot(CvPlot* pPlot, /* advc.064d: */ bool bVerifyProduction)
{
	CLLNode<XYCoords>* pPlotNode = headPlotsNode();
	while (pPlotNode != NULL)
	{
		if (GC.getMap().plotSoren(pPlotNode->m_data.iX, pPlotNode->m_data.iY) == pPlot)
		{
			pPlot->setPlotGroup(getOwner(), NULL, /* advc.064d: */ bVerifyProduction);
			pPlotNode = deletePlotsNode(pPlotNode); // can delete this CvPlotGroup
			break;
		}
		else pPlotNode = nextPlotsNode(pPlotNode);
	}
}


void CvPlotGroup::recalculatePlots(/* advc.064d: */ bool bVerifyProduction)
{
	PROFILE_FUNC(); // advc: The bulk of the time is spent by FAStar, specifically plotGroupValid (CvGameCoreUtils).

	XYCoords xy;
	PlayerTypes eOwner = getOwner();

	CLLNode<XYCoords>* pPlotNode = headPlotsNode();
	if (pPlotNode != NULL)
	{
		CvPlot const& kPlot = GC.getMap().getPlot(pPlotNode->m_data.iX, pPlotNode->m_data.iY);

		int iCount = 0;
		gDLL->getFAStarIFace()->SetData(&GC.getPlotGroupFinder(), &iCount);
		gDLL->getFAStarIFace()->GeneratePath(&GC.getPlotGroupFinder(), kPlot.getX(), kPlot.getY(),
				-1, -1, false, eOwner);
		if (iCount == getLengthPlots())
			return;
	}
	/*  <advc.064d> To deal with nested recalculatePlots calls. Mustn't
		verifyCityProduction so long as any recalculation is ongoing. */
	m_iRecalculating++;
	/*  Hopefully, it's enough to verify the production of all cities that are
		in the plot group before recalc. Any cities added during recalc get
		removed from some other plot group. However, I'm doing this only for the
		root recalc call, so ... might be inadequate. */
	std::vector<CvCity*> apOldCities; // </advc.064d>
	{
		PROFILE("CvPlotGroup::recalculatePlots update");

		CLinkList<XYCoords> oldPlotGroup;

		pPlotNode = headPlotsNode();
		while (pPlotNode != NULL)
		{
			PROFILE("CvPlotGroup::recalculatePlots update 1");

			CvPlot& kPlot = GC.getMap().getPlot(pPlotNode->m_data.iX, pPlotNode->m_data.iY);
			// <advc.064d>
			CvCity* pPlotCity = kPlot.getPlotCity();
			if (pPlotCity != NULL)
				apOldCities.push_back(pPlotCity);
			// </advc.064d>

			xy.iX = kPlot.getX();
			xy.iY = kPlot.getY();

			oldPlotGroup.insertAtEnd(xy);
			kPlot.setPlotGroup(eOwner, NULL);
			pPlotNode = deletePlotsNode(pPlotNode); // will delete this PlotGroup...
		}

		pPlotNode = oldPlotGroup.head();
		while (pPlotNode != NULL)
		{
			PROFILE("CvPlotGroup::recalculatePlots update 2");

			CvPlot& kPlot = GC.getMap().getPlot(pPlotNode->m_data.iX, pPlotNode->m_data.iY);
			kPlot.updatePlotGroup(eOwner, true);
			pPlotNode = oldPlotGroup.deleteNode(pPlotNode);
		}
	}
	// <advc.064d>
	m_iRecalculating--;
	FAssert(m_iRecalculating >= 0);
	if (m_iRecalculating == 0 && bVerifyProduction)
	{
		for (size_t i = 0; i < apOldCities.size(); i++)
			apOldCities[i]->verifyProduction();
	} // </advc.064d>
}


void CvPlotGroup::setID(int iID)
{
	m_iID = iID;
}


void CvPlotGroup::changeNumBonuses(BonusTypes eBonus, int iChange)
{
	if (iChange == 0)
		return; // advc

	//iOldNumBonuses = getNumBonuses(eBonus);
	m_aiNumBonuses.add(eBonus, iChange);

	//FAssert(m_paiNumBonuses.get(eBonus) >= 0); // XXX
	// K-Mod note, m_paiNumBonuses[eBonus] is often temporarily negative while plot groups are being updated.
	// It's an unfortunate side effect of the way the update is implemented. ... and so this assert is invalid.
	// (This isn't my fault. I haven't changed it. It has always been like this.)

	CLLNode<XYCoords>* pPlotNode = headPlotsNode();
	while (pPlotNode != NULL)
	{
		CvCity* pCity = GC.getMap().getPlot(pPlotNode->m_data.iX, pPlotNode->m_data.iY).getPlotCity();
		if (pCity != NULL)
		{
			if (pCity->getOwner() == getOwner())
				pCity->changeNumBonuses(eBonus, iChange);
		}
		pPlotNode = nextPlotsNode(pPlotNode);
	}
}

// advc.064d:
void CvPlotGroup::verifyCityProduction()
{
	PROFILE_FUNC(); // About 1 permille of the runtime
	if (m_iRecalculating > 0)
		return;
	CvMap const& kMap = GC.getMap();
	for (CLLNode<XYCoords> const* pPlotNode = headPlotsNode(); pPlotNode != NULL;
		pPlotNode = nextPlotsNode(pPlotNode))
	{
		CvCity* pCity = kMap.getPlot(pPlotNode->m_data.iX, pPlotNode->m_data.iY).getPlotCity();
		if (pCity != NULL && pCity->getOwner() == getOwner())
			pCity->verifyProduction();
	}
}


void CvPlotGroup::insertAtEndPlots(XYCoords xy)
{
	m_plots.insertAtEnd(xy);
}


CLLNode<XYCoords>* CvPlotGroup::deletePlotsNode(CLLNode<XYCoords>* pNode)
{
	CLLNode<XYCoords>* pPlotNode;
	pPlotNode = m_plots.deleteNode(pNode);
	if (getLengthPlots() == 0)
		GET_PLAYER(getOwner()).deletePlotGroup(getID());
	return pPlotNode;
}


void CvPlotGroup::read(FDataStreamBase* pStream)
{
	reset();

	uint uiFlag=0;
	pStream->Read(&uiFlag);
	pStream->Read(&m_iID);
	pStream->Read((int*)&m_eOwner);
	m_aiNumBonuses.Read(pStream);
	m_plots.Read(pStream);
}


void CvPlotGroup::write(FDataStreamBase* pStream)
{
	uint uiFlag=0;
	pStream->Write(uiFlag);
	pStream->Write(m_iID);
	pStream->Write(m_eOwner);
	m_aiNumBonuses.Write(pStream);
	m_plots.Write(pStream);
}
