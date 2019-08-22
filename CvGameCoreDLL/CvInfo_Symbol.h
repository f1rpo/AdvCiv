#pragma once

#ifndef CV_INFO_SYMBOL_H
#define CV_INFO_SYMBOL_H

/*  advc.003x: Cut from CvInfos.h. A few nuts-and-bolts classes to be precompiled:
	CvYieldInfo, CvCommerceInfo, CvColorInfo, CvPlayerColorInfo
	advc.003f: Inlined most of the CvYieldInfo getters */
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//  class : CvYieldInfo
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
class CvYieldInfo : public CvInfoBase
{
public: // All the const functions are exposed to Python
	CvYieldInfo();
	~CvYieldInfo();

	int getChar() const;
	void setChar(int i);
	inline int getHillsChange() const { return m_iHillsChange; }
	inline int getPeakChange() const { return m_iPeakChange; }
	inline int getLakeChange() const { return m_iLakeChange; }
	inline int getCityChange() const { return m_iCityChange; }
	// advc: These two are unused in XML
	inline int getPopulationChangeOffset() const { return m_iPopulationChangeOffset; }
	inline int getPopulationChangeDivisor() const { return m_iPopulationChangeDivisor; }
	inline int getMinCity() const { return m_iMinCity; }
	int getTradeModifier() const { return m_iTradeModifier; }
	int getGoldenAgeYield() const { return m_iGoldenAgeYield; }
	int getGoldenAgeYieldThreshold() const { return m_iGoldenAgeYieldThreshold; }
	int getAIWeightPercent() const { return m_iAIWeightPercent; }
	int getColorType() const;

	const TCHAR* getSymbolPath(int i) const;

	bool read(CvXMLLoadUtility* pXML);

protected:
	int m_iChar;
	int m_iHillsChange;
	int m_iPeakChange;
	int m_iLakeChange;
	int m_iCityChange;
	int m_iPopulationChangeOffset;
	int m_iPopulationChangeDivisor;
	int m_iMinCity;
	int m_iTradeModifier;
	int m_iGoldenAgeYield;
	int m_iGoldenAgeYieldThreshold;
	int m_iAIWeightPercent;
	int m_iColorType;

	CvString* m_paszSymbolPath;
};

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//  class : CvCommerceInfo
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
class CvCommerceInfo : public CvInfoBase
{
public: // All the const functions are exposed to Python

	CvCommerceInfo();

	int getChar() const;
	void setChar(int i);
	int getInitialPercent() const;
	int getInitialHappiness() const;
	inline int getAIWeightPercent() const { return m_iAIWeightPercent; }

	bool isFlexiblePercent() const;

	bool read(CvXMLLoadUtility* pXML);

protected:
	int m_iChar;
	int m_iInitialPercent;
	int m_iInitialHappiness;
	int m_iAIWeightPercent;

	bool m_bFlexiblePercent;
};

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//  class : CvColorInfo
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
class CvColorInfo : public CvInfoBase
{
public:
	DllExport const NiColorA& getColor() const; // Exposed to Python
	bool read(CvXMLLoadUtility* pXML);

protected:
	NiColorA m_Color;
};

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//  class : CvPlayerColorInfo
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
class CvPlayerColorInfo : public CvInfoBase
{
public: // The const functions are exposed to Python
	CvPlayerColorInfo();

	DllExport int getColorTypePrimary() const;
	DllExport int getColorTypeSecondary() const;
	int getTextColorType() const;

	bool read(CvXMLLoadUtility* pXML);

protected:
	int m_iColorTypePrimary;
	int m_iColorTypeSecondary;
	int m_iTextColorType;
};

#endif
