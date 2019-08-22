#pragma once

#ifndef CV_INFO_BUILD_H
#define CV_INFO_BUILD_H

/*  advc.003x: Cut from CvInfos.h. Just CvBuildInfo. That class is needed by
	.cpp files that deal with units and .cpp files that deal with terrain;
	don't want to bundle the unit and terrain info classes in a single big header,
	so CvBuilInfo needs to be singled out.*/

class CvBuildInfo : public CvHotkeyInfo
{
public: // All the const functions are exposed to Python // advc.130f: inlined most of them
	CvBuildInfo();
	~CvBuildInfo();

	inline int getTime() const { return m_iTime; }
	inline int getCost() const { return m_iCost; }
	inline int getTechPrereq() const { return m_iTechPrereq; }
	inline int getImprovement() const { return m_iImprovement; }
	inline int getRoute() const { return m_iRoute; }
	DllExport int getEntityEvent() const;
	DllExport int getMissionType() const;
	void setMissionType(int iNewType);

	inline bool isKill() const { return m_bKill; }

	int getFeatureTech(int i) const;
	int getFeatureTime(int i) const;
	int getFeatureProduction(int i) const;
	bool isFeatureRemove(int i) const;

	bool read(CvXMLLoadUtility* pXML);

protected:
	int m_iTime;
	int m_iCost;
	int m_iTechPrereq;
	int m_iImprovement;
	int m_iRoute;
	int m_iEntityEvent;
	int m_iMissionType;

	bool m_bKill;

	int* m_paiFeatureTech;
	int* m_paiFeatureTime;
	int* m_paiFeatureProduction;

	bool* m_pabFeatureRemove;
};

#endif
