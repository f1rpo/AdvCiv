#include "CvGameCoreDLL.h"
#include "AIStrengthMemoryMap.h"
#include "CvTeam.h"
#include "CvMap.h"

// advc.158: New implementation file; see comment in header.

void AIStrengthMemoryMap::init(PlotNumTypes eMapSize, TeamTypes eTeam)
{
	FAssert(eMapSize > 0);
	FAssert(eTeam != NO_TEAM);
	/*m_aiMap.clear();
	m_aiMap.resize(eMapSize, 0);*/
	m_map.clear();
	m_eTeam = eTeam;
}


void AIStrengthMemoryMap::reset()
{
	//m_aiMap.clear();
	m_map.clear();
}


void AIStrengthMemoryMap::read(FDataStreamBase* pStream, uint uiFlag, TeamTypes eTeam)
{
	m_eTeam = eTeam;
	if (uiFlag >= 4)
	{
		size_t iSize;
		pStream->Read(&iSize);
		if (iSize > 0)
		{
			/*FAssert(GC.getMap().numPlots() == (int)iSize);
			m_aiMap.resize(iSize, 0);
			pStream->Read(m_aiMap.size(), &m_aiMap[0]);*/
			for (size_t i = 0; i < iSize; i++)
			{
				PlotNumTypes ePlot;
				pStream->Read((int*)&ePlot);
				int iStrength;
				pStream->Read(&iStrength);
				m_map[ePlot] = iStrength;
			}
		}
	}
	else 
	{
		/*std::vector<int> aiDummy;
		std::vector<int>& aiMap = (CvTeam::getTeam(m_eTeam).isAlive() ?
				m_aiMap : aiDummy);
		aiMap.resize(GC.getMap().numPlots(), 0);
		if (uiFlag >= 1)
			pStream->Read(aiMap.size(), &aiMap[0]);*/
		std::vector<int> aiTmp;
		aiTmp.resize(GC.getMap().numPlots(), 0);
		if (uiFlag >= 1) // (Guaranteed b/c we can't load BtS saves)
			pStream->Read(aiTmp.size(), &aiTmp[0]);
		for (size_t i = 0; i < aiTmp.size(); i++)
		{
			if (aiTmp[i] != 0)
			{
				FAssert(aiTmp[i] > 0);
				m_map[(PlotNumTypes)i] = aiTmp[i];
			}
		}
	}
}


void AIStrengthMemoryMap::write(FDataStreamBase* pStream) const
{
	/*FAssert(m_aiMap.size() == GC.getMap().numPlots());
	// the consequences of the assert failing are really bad.
	m_aiMap.resize(GC.getMap().numPlots());
	FAssert(!m_aiMap.empty());
	pStream->Write(m_aiMap.size(), &m_aiMap[0]);*/ // K-Mod
	// Replacing the above (still using a vector):
	/*pStream->Write(m_aiMap.size());
	if (!m_aiMap.empty())
		pStream->Write(m_aiMap.size(), &m_aiMap[0]);*/
	// Using PlotStrengthMap:
	pStream->Write(m_map.size());
	for (PlotStrengthMap::const_iterator it = m_map.begin(); it != m_map.end(); ++it)
	{
		pStream->Write(it->first);
		pStream->Write(it->second);
	}
}


int AIStrengthMemoryMap::get(CvPlot const& kPlot) const
{
	return get(GC.getMap().plotNum(kPlot));
}


void AIStrengthMemoryMap::set(CvPlot const& kPlot, int iNewValue)
{
	PlotNumTypes ePlot = GC.getMap().plotNum(kPlot);
	/*FAssertBounds(0, m_aiMap.size(), ePlot);
	m_aiMap[ePlot] = iNewValue;*/
	m_map[ePlot] = iNewValue;
}


void AIStrengthMemoryMap::decay()
{
	PROFILE_FUNC();
	/*if (m_aiMap.size() != GC.getMap().numPlots())
		return;*/
	CvTeam const& kTeam = CvTeam::getTeam(m_eTeam);
	/*	Barbarian strength memory disabled for now. (The set calls are disabled
		at the call location.) */
	if (kTeam.isBarbarian())
		return;
	/*for (int i = 0; i < GC.getMap().numPlots(); i++)
	{
		if (m_aiMap[i] == 0)
			continue;
		FAssert(m_aiMap[i] > 0);
		CvPlot const& kPlot = GC.getMap().getPlotByIndex(i);*/
	PlotStrengthMap::iterator it = m_map.begin();
	while (it != m_map.end())
	{
		CvPlot const& kPlot = GC.getMap().getPlotByIndex(it->first);
		if (kPlot.isVisible(m_eTeam) &&
			!kPlot.isVisibleEnemyUnit(kTeam.getLeaderID()))
		{
			it = m_map.erase(it);
			//m_aiMap[i] = 0;
		}
		// K-Mod: reduce by 4% (arbitrary number), rounding down.
		//else m_aiMap[i] = (96 * m_aiMap[i]) / 100;
		else
		{
			it->second = (96 * it->second) / 100;
			++it;
		}
	}
}
