// plotGroup.cpp

#include "CvGameCoreDLL.h"
#include "CvPlotGroup.h"
#include "CvPlayerAI.h"
#include "CvMap.h"
#include "CvDLLFAStarIFaceBase.h"

int CvPlotGroup::m_iRecalculating = 0; // advc.064d

// Public Functions...

CvPlotGroup::CvPlotGroup()
{
	m_paiNumBonuses = NULL;

	reset(0, NO_PLAYER, true);
}


CvPlotGroup::~CvPlotGroup()
{
	uninit();
}


void CvPlotGroup::init(int iID, PlayerTypes eOwner, CvPlot* pPlot)
{
	//--------------------------------
	// Init saved data
	reset(iID, eOwner);

	//--------------------------------
	// Init non-saved data

	//--------------------------------
	// Init other game data
	addPlot(pPlot);
}


void CvPlotGroup::uninit()
{
	SAFE_DELETE_ARRAY(m_paiNumBonuses);

	m_plots.clear();
}

// FUNCTION: reset()
// Initializes data members that are serialized.
void CvPlotGroup::reset(int iID, PlayerTypes eOwner, bool bConstructorCall)
{
	int iI;

	//--------------------------------
	// Uninit class
	uninit();

	m_iID = iID;
	m_eOwner = eOwner;

	if (!bConstructorCall)
	{
		FAssertMsg((0 < GC.getNumBonusInfos()), "GC.getNumBonusInfos() is not greater than zero but an array is being allocated in CvPlotGroup::reset");
		m_paiNumBonuses = new int [GC.getNumBonusInfos()];
		for (iI = 0; iI < GC.getNumBonusInfos(); iI++)
		{
			m_paiNumBonuses[iI] = 0;
		}
	}
}


void CvPlotGroup::addPlot(CvPlot* pPlot)
{
	XYCoords xy;

	xy.iX = pPlot->getX();
	xy.iY = pPlot->getY();

	insertAtEndPlots(xy);

	pPlot->setPlotGroup(getOwner(), this);
}


void CvPlotGroup::removePlot(CvPlot* pPlot)
{
	CLLNode<XYCoords>* pPlotNode;

	pPlotNode = headPlotsNode();

	while (pPlotNode != NULL)
	{
		if (GC.getMap().plotSoren(pPlotNode->m_data.iX, pPlotNode->m_data.iY) == pPlot)
		{
			pPlot->setPlotGroup(getOwner(), NULL);

			pPlotNode = deletePlotsNode(pPlotNode); // can delete this PlotGroup...
			break;
		}
		else
		{
			pPlotNode = nextPlotsNode(pPlotNode);
		}
	}
}


void CvPlotGroup::recalculatePlots()
{
	PROFILE_FUNC();

	CLLNode<XYCoords>* pPlotNode;
	CvPlot* pPlot;
	CLinkList<XYCoords> oldPlotGroup;
	XYCoords xy;
	PlayerTypes eOwner;
	int iCount;

	eOwner = getOwner();

	pPlotNode = headPlotsNode();

	if (pPlotNode != NULL)
	{
		pPlot = GC.getMap().plotSoren(pPlotNode->m_data.iX, pPlotNode->m_data.iY);

		iCount = 0;

		gDLL->getFAStarIFace()->SetData(&GC.getPlotGroupFinder(), &iCount);
		gDLL->getFAStarIFace()->GeneratePath(&GC.getPlotGroupFinder(), pPlot->getX(), pPlot->getY(), -1, -1, false, eOwner);

		if (iCount == getLengthPlots())
		{
			return;
		}
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

		oldPlotGroup.clear();

		pPlotNode = headPlotsNode();

		while (pPlotNode != NULL)
		{
			PROFILE("CvPlotGroup::recalculatePlots update 1");

			pPlot = GC.getMap().plotSoren(pPlotNode->m_data.iX, pPlotNode->m_data.iY);
			// <advc.064d>
			CvCity* pPlotCity = pPlot->getPlotCity();
			if (pPlotCity != NULL)
				apOldCities.push_back(pPlotCity);
			// </advc.064d>
			FAssertMsg(pPlot != NULL, "Plot is not assigned a valid value");

			xy.iX = pPlot->getX();
			xy.iY = pPlot->getY();

			oldPlotGroup.insertAtEnd(xy);

			pPlot->setPlotGroup(eOwner, NULL);

			pPlotNode = deletePlotsNode(pPlotNode); // will delete this PlotGroup...
		}

		pPlotNode = oldPlotGroup.head();

		while (pPlotNode != NULL)
		{
			PROFILE("CvPlotGroup::recalculatePlots update 2");

			pPlot = GC.getMap().plotSoren(pPlotNode->m_data.iX, pPlotNode->m_data.iY);

			FAssertMsg(pPlot != NULL, "Plot is not assigned a valid value");

			pPlot->updatePlotGroup(eOwner, true);

			pPlotNode = oldPlotGroup.deleteNode(pPlotNode);
		}
	}
	// <advc.064d>
	m_iRecalculating--;
	FAssert(m_iRecalculating >= 0);
	if (m_iRecalculating == 0)
	{
		for (size_t i = 0; i < apOldCities.size(); i++)
			apOldCities[i]->verifyProduction();
	} // </advc.064d>
}


int CvPlotGroup::getID() const
{
	return m_iID;
}


void CvPlotGroup::setID(int iID)
{
	m_iID = iID;
}


int CvPlotGroup::getNumBonuses(BonusTypes eBonus) const
{
	FAssertMsg(eBonus >= 0, "eBonus is expected to be non-negative (invalid Index)");
	FAssertMsg(eBonus < GC.getNumBonusInfos(), "eBonus is expected to be within maximum bounds (invalid Index)");
	return m_paiNumBonuses[eBonus];
}


bool CvPlotGroup::hasBonus(BonusTypes eBonus)
{
	return(getNumBonuses(eBonus) > 0);
}


void CvPlotGroup::changeNumBonuses(BonusTypes eBonus, int iChange)
{
	FAssertMsg(eBonus >= 0, "eBonus is expected to be non-negative (invalid Index)");
	FAssertMsg(eBonus < GC.getNumBonusInfos(), "eBonus is expected to be within maximum bounds (invalid Index)");

	if (iChange == 0)
		return; // advc.003

	//iOldNumBonuses = getNumBonuses(eBonus);
	m_paiNumBonuses[eBonus] = (m_paiNumBonuses[eBonus] + iChange);

	//FAssertMsg(m_paiNumBonuses[eBonus] >= 0, "m_paiNumBonuses[eBonus] is expected to be non-negative (invalid Index)"); XXX
	// K-Mod note, m_paiNumBonuses[eBonus] is often temporarily negative while plot groups are being updated.
	// It's an unfortuante side effect of the way the update is implemented. ... and so this assert is invalid.
	// (This isn't my fault. I haven't changed it. It has always been like this.)

	CLLNode<XYCoords>* pPlotNode = headPlotsNode();

	while (pPlotNode != NULL)
	{
		CvCity* pCity = GC.getMap().plotSoren(pPlotNode->m_data.iX, pPlotNode->m_data.iY)->getPlotCity();
		if (pCity != NULL)
		{
			if (pCity->getOwner() == getOwner())
			{
				pCity->changeNumBonuses(eBonus, iChange);
			}
		}

		pPlotNode = nextPlotsNode(pPlotNode);
	}
}

// <advc.064d>
void CvPlotGroup::verifyCityProduction() {

	PROFILE_FUNC(); // About 1 permille of the runtime (July 2019)
	if (m_iRecalculating > 0)
		return;
	CvMap const& m = GC.getMap();
	CLLNode<XYCoords>* pPlotNode = headPlotsNode();
	while (pPlotNode != NULL) {
		CvCity* pCity = m.plotSoren(pPlotNode->m_data.iX, pPlotNode->m_data.iY)->getPlotCity();
		if (pCity != NULL && pCity->getOwner() == getOwner())
			pCity->verifyProduction();
		pPlotNode = nextPlotsNode(pPlotNode);
	}
} // </advc.064d>


void CvPlotGroup::insertAtEndPlots(XYCoords xy)
{
	m_plots.insertAtEnd(xy);
}


CLLNode<XYCoords>* CvPlotGroup::deletePlotsNode(CLLNode<XYCoords>* pNode)
{
	CLLNode<XYCoords>* pPlotNode;

	pPlotNode = m_plots.deleteNode(pNode);

	if (getLengthPlots() == 0)
	{
		GET_PLAYER(getOwner()).deletePlotGroup(getID());
	}

  return pPlotNode;
}


CLLNode<XYCoords>* CvPlotGroup::nextPlotsNode(CLLNode<XYCoords>* pNode)
{
	return m_plots.next(pNode);
}


int CvPlotGroup::getLengthPlots()
{
	return m_plots.getLength();
}


CLLNode<XYCoords>* CvPlotGroup::headPlotsNode()
{
	return m_plots.head();
}


void CvPlotGroup::read(FDataStreamBase* pStream)
{
	// Init saved data
	reset();

	uint uiFlag=0;
	pStream->Read(&uiFlag);	// flags for expansion

	pStream->Read(&m_iID);

	pStream->Read((int*)&m_eOwner);

	FAssertMsg((0 < GC.getNumBonusInfos()), "GC.getNumBonusInfos() is not greater than zero but an array is being allocated in CvPlotGroup::read");
	pStream->Read(GC.getNumBonusInfos(), m_paiNumBonuses);

	m_plots.Read(pStream);
}


void CvPlotGroup::write(FDataStreamBase* pStream)
{
	uint uiFlag=0;
	pStream->Write(uiFlag);		// flag for expansion

	pStream->Write(m_iID);

	pStream->Write(m_eOwner);

	FAssertMsg((0 < GC.getNumBonusInfos()), "GC.getNumBonusInfos() is not greater than zero but an array is being allocated in CvPlotGroup::write");
	pStream->Write(GC.getNumBonusInfos(), m_paiNumBonuses);

	m_plots.Write(pStream);
}
