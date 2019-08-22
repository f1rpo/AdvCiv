#pragma once

#ifndef CV_INFO_ORGANIZATION_H
#define CV_INFO_ORGANIZATION_H

/*  advc.003x: Cut from CvInfos.h; CvReligionInfo and CvCorporationInfo. To be
	precompiled. (Changes are very rare, in fact, no changes by mods at all so far.) */

/*  advc.003x: Common base class to reduce code duplication. Just the stuff that
	was exactly the same in CvReligionInfo and CvCorporationInfo. */
class CvOrganizationInfo : public CvHotkeyInfo
{
public: // All the const functions are exposed to Python
	CvOrganizationInfo();

	int getChar() const;
	void setChar(int i);
	inline int getTechPrereq() const { return m_iTechPrereq; } // advc.130f: inline
	int getFreeUnitClass() const;
	inline int getSpreadFactor() const { return m_iSpreadFactor; } // advc.130f: inline
	int getMissionType() const;
	void setMissionType(int iNewType);

	const TCHAR* getMovieFile() const;
	void setMovieFile(const TCHAR* szVal);
	const TCHAR* getMovieSound() const;
	void setMovieSound(const TCHAR* szVal);
	const TCHAR* getSound() const;
	void setSound(const TCHAR* szVal);

	bool read(CvXMLLoadUtility* pXML);

protected:
	int m_iChar;
	int m_iTechPrereq;
	int m_iFreeUnitClass;
	int m_iSpreadFactor;
	int m_iMissionType;

	CvString m_szMovieFile;
	CvString m_szMovieSound;
	CvString m_szSound;
};

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//  class : CvReligionInfo
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
class CvReligionInfo : public CvOrganizationInfo
{
public: // All the const functions are exposed to Python
	CvReligionInfo();
	~CvReligionInfo();

	int getHolyCityChar() const;
	void setHolyCityChar(int i);
	int getNumFreeUnits() const;

	const TCHAR* getTechButton() const;
	void setTechButton(const TCHAR* szVal);
	const TCHAR* getGenericTechButton() const;
	void setGenericTechButton(const TCHAR* szVal);

	const TCHAR* getButtonDisabled() const;

	void setAdjectiveKey(const TCHAR* szVal);
	const wchar* getAdjectiveKey() const;
	std::wstring pyGetAdjectiveKey() { return getAdjectiveKey(); }

	// Array access:

	int getGlobalReligionCommerce(int i) const;
	int* getGlobalReligionCommerceArray() const;
	int getHolyCityCommerce(int i) const;
	int* getHolyCityCommerceArray() const;
	int getStateReligionCommerce(int i) const;
	int* getStateReligionCommerceArray() const;

	bool read(CvXMLLoadUtility* pXML);

protected:
	int m_iHolyCityChar;
	int m_iNumFreeUnits;

	CvString m_szTechButton;
	CvString m_szGenericTechButton;
	CvWString m_szAdjectiveKey;

	int* m_paiGlobalReligionCommerce;
	int* m_paiHolyCityCommerce;
	int* m_paiStateReligionCommerce;
};

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//  class : CvCorporationInfo
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
class CvCorporationInfo : public CvOrganizationInfo
{
public: // All the const functions are exposed to Python
	CvCorporationInfo();
	virtual ~CvCorporationInfo();

	int getHeadquarterChar() const;
	void setHeadquarterChar(int i);
	int getSpreadCost() const;
	int getMaintenance() const;

	int getBonusProduced() const;

	// Array access:

	int getPrereqBonus(int i) const;
	int getHeadquarterCommerce(int i) const;
	int* getHeadquarterCommerceArray() const;
	int getCommerceProduced(int i) const;
	int* getCommerceProducedArray() const;
	int getYieldProduced(int i) const;
	int* getYieldProducedArray() const;

	bool read(CvXMLLoadUtility* pXML);

protected:
	int m_iHeadquarterChar;
	int m_iSpreadCost;
	int m_iMaintenance;
	int m_iBonusProduced;

	int* m_paiPrereqBonuses;
	int* m_paiHeadquarterCommerce;
	int* m_paiCommerceProduced;
	int* m_paiYieldProduced;
};

#endif
