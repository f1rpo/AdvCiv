#pragma once

// plotGroup.h

#ifndef CIV4_PLOT_GROUP_H
#define CIV4_PLOT_GROUP_H

#include "LinkedList.h"

class CvPlot;


class CvPlotGroup
{
public:

	CvPlotGroup();
	virtual ~CvPlotGroup();

	void init(int iID, PlayerTypes eOwner, CvPlot* pPlot);
	void uninit();
	void reset(int iID = 0, PlayerTypes eOwner = NO_PLAYER, bool bConstructorCall=false);

	void addPlot(CvPlot* pPlot);
	void removePlot(CvPlot* pPlot);
	void recalculatePlots();

	inline int getID() const { return m_iID; } // advc.003f
	void setID(int iID);

	//PlayerTypes getOwner() const;
	// advc.003f: The EXE doesn't call this, so no need for an external version.
	inline PlayerTypes getOwner() const { return m_eOwner; }

	int getNumBonuses(BonusTypes eBonus) const;
	bool hasBonus(BonusTypes eBonus);
	void changeNumBonuses(BonusTypes eBonus, int iChange);
	void verifyCityProduction(); // advc.064d

	void insertAtEndPlots(XYCoords xy);
	CLLNode<XYCoords>* deletePlotsNode(CLLNode<XYCoords>* pNode);
	// advc.003f: inline
	inline CLLNode<XYCoords>* nextPlotsNode(CLLNode<XYCoords>* pNode)
	{
		return m_plots.next(pNode);
	} // <advc.003s> Safer in 'for' loops
	inline CLLNode<XYCoords> const* nextPlotsNode(CLLNode<XYCoords> const* pNode)
	{
		return m_plots.next(pNode);
	} // </advc.003s>
	int getLengthPlots();
	CLLNode<XYCoords>* headPlotsNode();

	// for serialization
	void read(FDataStreamBase* pStream);
	void write(FDataStreamBase* pStream);

protected:
	static int m_iRecalculating; // advc.064d
	int m_iID;
	PlayerTypes m_eOwner;
	int* m_paiNumBonuses;
	CLinkList<XYCoords> m_plots;
};

#endif
