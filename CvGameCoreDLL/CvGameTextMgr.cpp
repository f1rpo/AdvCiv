//---------------------------------------------------------------------------------------
//
//  *****************   Civilization IV   ********************
//
//  FILE:    CvGameTextMgr.cpp
//
//  PURPOSE: Interfaces with GameText XML Files to manage the paths of art files
//	(advc: Looks like this class has expanded far beyond its original purpose)
//---------------------------------------------------------------------------------------
//  Copyright (c) 2004 Firaxis Games, Inc. All rights reserved.
//---------------------------------------------------------------------------------------

#include "CvGameCoreDLL.h"
#include "CvGamePlay.h"
#include "CvGameTextMgr.h"
#include "CvInfos.h"
#include "CvXMLLoadUtility.h"
#include "CvArtFileMgr.h"
#include "CvMap.h"
#include "RiseFall.h" // advc.700
#include "AI_Defines.h" // BBAI
#include "CvBugOptions.h"
#include "CvPopupInfo.h"
#include "CyArgsList.h"
// <advc.050>
#include "CvInitCore.h"
#include <sstream> // </advc.050>
#include "CvDLLPythonIFaceBase.h"
#include "CvDLLUtilityIFaceBase.h"
#include "CvDLLInterfaceIFaceBase.h"
#include "CvDLLSymbolIFaceBase.h"

int shortenID(int iId)
{
	return iId;
}

// For displaying Asserts and error messages
static char* szErrorMsg;


CvGameTextMgr& CvGameTextMgr::GetInstance()
{
	static CvGameTextMgr gs_GameTextMgr;
	return gs_GameTextMgr;
}


CvGameTextMgr::CvGameTextMgr()
{

}


CvGameTextMgr::~CvGameTextMgr()
{
}

//	PURPOSE: Allocate memory
void CvGameTextMgr::Initialize()
{

}

// PURPOSE: Clear memory
void CvGameTextMgr::DeInitialize()
{
	for(int i=0;i<(int)m_apbPromotion.size();i++)
	{
		delete [] m_apbPromotion[i];
	}
}


void CvGameTextMgr::Reset()
{
	/*  Access CvXMLLoadUtility to clean global text memory and reload
		the XML files */
	CvXMLLoadUtility pXML;
	pXML.LoadGlobalText();
}

// Returns the current language
int CvGameTextMgr::getCurrentLanguage()
{
	return gDLL->getCurrentLanguage();
}


void CvGameTextMgr::setYearStr(CvWString& szString, int iGameTurn, bool bSave,
		CalendarTypes eCalendar, int iStartYear, GameSpeedTypes eSpeed)
{
	int iTurnYear = getTurnYearForGame(iGameTurn, iStartYear, eCalendar, eSpeed);
	// <advc.002k>
	enum NotationTypes { AD_PREFIX = 0, AD_POSTFIX = 1, COMMON_ERA = 2 };
	NotationTypes eNotation = (NotationTypes)getBugOptionINT(
			"NJAGC__YearNotation", 0, false);
	// </advc.002k>
	if (iTurnYear < 0)
	{
		if (bSave)
		{
			szString = gDLL->getText( // <advc.002k>
					eNotation == COMMON_ERA ?
					"TXT_KEY_TIME_BCE_SAVE" : // </advc.002k>
					"TXT_KEY_TIME_BC_SAVE",
					CvWString::format(L"%04d", -iTurnYear).GetCString());
		}
		else
		{
			szString = gDLL->getText( // <advc.002k>
					eNotation == COMMON_ERA ?
					"TXT_KEY_TIME_BCE" : // </advc.002k>
					"TXT_KEY_TIME_BC", -iTurnYear);
		}
	}
	else // advc.002k: Melded the if(iTurnYear > 0)...else branches
	{
		if (bSave)
		{
			szString = gDLL->getText(
					eNotation == COMMON_ERA ? "TXT_KEY_TIME_CE_SAVE" : // advc.002k
					"TXT_KEY_TIME_AD_SAVE",
					iTurnYear > 0 ?
					CvWString::format(L"%04d", iTurnYear).GetCString() :
					L"0001");
		}
		else
		{	// <advc.002k>
			CvWString szTag;
			switch(eNotation) {
			case AD_POSTFIX: szTag = "TXT_KEY_TIME_AD"; break;
			case COMMON_ERA: szTag = "TXT_KEY_TIME_CE"; break;
			default: szTag = "TXT_KEY_AD_TIME"; break;
			} // </advc.002k>
			szString = gDLL->getText(szTag, std::max(1, iTurnYear));
		}
	}
}

void CvGameTextMgr::setDateStr(CvWString& szString, int iGameTurn, bool bSave, CalendarTypes eCalendar, int iStartYear, GameSpeedTypes eSpeed)
{
	CvWString szYearBuffer;
	CvWString szWeekBuffer;

	setYearStr(szYearBuffer, iGameTurn, bSave, eCalendar, iStartYear, eSpeed);

	switch (eCalendar)
	{
	case CALENDAR_DEFAULT:
		if (0 == (getTurnMonthForGame(iGameTurn + 1, iStartYear, eCalendar, eSpeed) - getTurnMonthForGame(iGameTurn, iStartYear, eCalendar, eSpeed)) % GC.getNumMonthInfos())
		{
			szString = szYearBuffer;
		}
		else
		{
			int iMonth = getTurnMonthForGame(iGameTurn, iStartYear, eCalendar, eSpeed);
			if (bSave)
			{
				szString = (szYearBuffer + "-" + GC.getMonthInfo((MonthTypes)(iMonth % GC.getNumMonthInfos())).getDescription());
			}
			else
			{
				szString = (GC.getMonthInfo((MonthTypes)(iMonth % GC.getNumMonthInfos())).getDescription() + CvString(", ") + szYearBuffer);
			}
		}
		break;
	case CALENDAR_YEARS:
	case CALENDAR_BI_YEARLY:
		szString = szYearBuffer;
		break;

	case CALENDAR_TURNS:
		szString = gDLL->getText("TXT_KEY_TIME_TURN", (iGameTurn + 1));
		break;

	case CALENDAR_SEASONS:
		if (bSave)
		{
			szString = (szYearBuffer + "-" + GC.getSeasonInfo((SeasonTypes)(iGameTurn % GC.getNumSeasonInfos())).getDescription());
		}
		else
		{
			szString = (GC.getSeasonInfo((SeasonTypes)(iGameTurn % GC.getNumSeasonInfos())).getDescription() + CvString(", ") + szYearBuffer);
		}
		break;

	case CALENDAR_MONTHS:
		if (bSave)
		{
			szString = (szYearBuffer + "-" + GC.getMonthInfo((MonthTypes)(iGameTurn % GC.getNumMonthInfos())).getDescription());
		}
		else
		{
			szString = (GC.getMonthInfo((MonthTypes)(iGameTurn % GC.getNumMonthInfos())).getDescription() + CvString(", ") + szYearBuffer);
		}
		break;

	case CALENDAR_WEEKS:
		szWeekBuffer = gDLL->getText("TXT_KEY_TIME_WEEK", ((iGameTurn % GC.getDefineINT("WEEKS_PER_MONTHS")) + 1));

		if (bSave)
		{
			szString = (szYearBuffer + "-" + GC.getMonthInfo((MonthTypes)((iGameTurn / GC.getDefineINT("WEEKS_PER_MONTHS")) % GC.getNumMonthInfos())).getDescription() + "-" + szWeekBuffer);
		}
		else
		{
			szString = (szWeekBuffer + ", " + GC.getMonthInfo((MonthTypes)((iGameTurn / GC.getDefineINT("WEEKS_PER_MONTHS")) % GC.getNumMonthInfos())).getDescription() + ", " + szYearBuffer);
		}
		break;

	default:
		FAssert(false);
	}
}


void CvGameTextMgr::setTimeStr(CvWString& szString, int iGameTurn, bool bSave)
{
	setDateStr(szString, iGameTurn, bSave, GC.getGame().getCalendar(), GC.getGame().getStartYear(), GC.getGame().getGameSpeedType());
}


void CvGameTextMgr::setInterfaceTime(CvWString& szString, PlayerTypes ePlayer)
{
	CvWString szTempBuffer;

	if (GET_PLAYER(ePlayer).isGoldenAge())
	{
		szString.Format(L"%c(%d) ", gDLL->getSymbolID(GOLDEN_AGE_CHAR), GET_PLAYER(ePlayer).getGoldenAgeTurns());
	}
	else
	{
		szString.clear();
	}

	setTimeStr(szTempBuffer, GC.getGame().getGameTurn(), false);
	szString += CvWString(szTempBuffer);
}


void CvGameTextMgr::setGoldStr(CvWString& szString, PlayerTypes ePlayer)
{
	if (GET_PLAYER(ePlayer).getGold() < 0)
	{
		szString.Format(L"%c: " SETCOLR L"%d" SETCOLR, GC.getCommerceInfo(COMMERCE_GOLD).getChar(), TEXT_COLOR("COLOR_NEGATIVE_TEXT"), GET_PLAYER(ePlayer).getGold());
	}
	else
	{
		szString.Format(L"%c: %d", GC.getCommerceInfo(COMMERCE_GOLD).getChar(), GET_PLAYER(ePlayer).getGold());
	}

	int iGoldRate = GET_PLAYER(ePlayer).calculateGoldRate();
	if (iGoldRate < 0)
	{
		szString += gDLL->getText("TXT_KEY_MISC_NEG_GOLD_PER_TURN", iGoldRate);
	}
	else if (iGoldRate > 0)
	{
		szString += gDLL->getText("TXT_KEY_MISC_POS_GOLD_PER_TURN", iGoldRate);
	}

	if (GET_PLAYER(ePlayer).isStrike())
	{
		szString += gDLL->getText("TXT_KEY_MISC_STRIKE");
	}
}


void CvGameTextMgr::setResearchStr(CvWString& szString, PlayerTypes ePlayer)
{
	CvPlayer const& kPlayer = GET_PLAYER(ePlayer); // advc.003
	szString = gDLL->getText("TXT_KEY_MISC_RESEARCH_STRING",
			GC.getTechInfo(kPlayer.getCurrentResearch()).getTextKeyWide());
	CvWString szTempBuffer;
	if (TEAMREF(ePlayer).getTechCount(GET_PLAYER(ePlayer).getCurrentResearch()) > 0)
	{
		szTempBuffer.Format(L" %d", TEAMREF(ePlayer).getTechCount(kPlayer.
				getCurrentResearch()) + 1);
		szString += szTempBuffer;
	} // <advc.004x>
	int iTurnsLeft = kPlayer.getResearchTurnsLeft(kPlayer.getCurrentResearch(), true);
	if(iTurnsLeft >= 0) // </advc.004x>
		szTempBuffer.Format(L" (%d)", iTurnsLeft);
	szString += szTempBuffer;
}


void CvGameTextMgr::setOOSSeeds(CvWString& szString, PlayerTypes ePlayer)
{
	if (GET_PLAYER(ePlayer).isHuman())
	{
		int iNetID = GET_PLAYER(ePlayer).getNetID();
		if (gDLL->isConnected(iNetID))
		{
			szString = gDLL->getText("TXT_KEY_PLAYER_OOS", gDLL->GetSyncOOS(iNetID), gDLL->GetOptionsOOS(iNetID));
		}
	}
}

void CvGameTextMgr::setNetStats(CvWString& szString, PlayerTypes ePlayer)  // advc.003: Some style changes
{
	if(ePlayer == GC.getGame().getActivePlayer()
			// advc.004v: Moved up
			|| !gDLL->getInterfaceIFace()->isNetStatsVisible())
		return;
	if(GET_PLAYER(ePlayer).isHuman()) {
		int iNetID = GET_PLAYER(ePlayer).getNetID();
		if(gDLL->isConnected(iNetID))
			szString = gDLL->getText("TXT_KEY_MISC_NUM_MS", gDLL->GetLastPing(iNetID));
		else szString = gDLL->getText("TXT_KEY_MISC_DISCONNECTED");
	}
	else szString = gDLL->getText("TXT_KEY_MISC_AI");
}


void CvGameTextMgr::setMinimizePopupHelp(CvWString& szString, const CvPopupInfo & info)
{
	CvCity* pCity;
	UnitTypes eTrainUnit;
	BuildingTypes eConstructBuilding;
	ProjectTypes eCreateProject;
	ReligionTypes eReligion;
	CivicTypes eCivic;

	switch (info.getButtonPopupType())
	{
	case BUTTONPOPUP_CHOOSEPRODUCTION:
		pCity = GET_PLAYER(GC.getGame().getActivePlayer()).getCity(info.getData1());
		if (pCity != NULL)
		{
			eTrainUnit = NO_UNIT;
			eConstructBuilding = NO_BUILDING;
			eCreateProject = NO_PROJECT;

			switch (info.getData2())
			{
			case (ORDER_TRAIN):
				eTrainUnit = (UnitTypes)info.getData3();
				break;
			case (ORDER_CONSTRUCT):
				eConstructBuilding = (BuildingTypes)info.getData3();
				break;
			case (ORDER_CREATE):
				eCreateProject = (ProjectTypes)info.getData3();
				break;
			default:
				break;
			}

			if (eTrainUnit != NO_UNIT)
			{
				szString += gDLL->getText("TXT_KEY_MINIMIZED_CHOOSE_PRODUCTION_UNIT", GC.getUnitInfo(eTrainUnit).getTextKeyWide(), pCity->getNameKey());
			}
			else if (eConstructBuilding != NO_BUILDING)
			{
				szString += gDLL->getText("TXT_KEY_MINIMIZED_CHOOSE_PRODUCTION_BUILDING", GC.getBuildingInfo(eConstructBuilding).getTextKeyWide(), pCity->getNameKey());
			}
			else if (eCreateProject != NO_PROJECT)
			{
				szString += gDLL->getText("TXT_KEY_MINIMIZED_CHOOSE_PRODUCTION_PROJECT", GC.getProjectInfo(eCreateProject).getTextKeyWide(), pCity->getNameKey());
			}
			else
			{
				szString += gDLL->getText("TXT_KEY_MINIMIZED_CHOOSE_PRODUCTION", pCity->getNameKey());
			}
		}
		break;

	case BUTTONPOPUP_CHANGERELIGION:
		eReligion = ((ReligionTypes)(info.getData1()));
		if (eReligion != NO_RELIGION)
		{
			szString += gDLL->getText("TXT_KEY_MINIMIZED_CHANGE_RELIGION", GC.getReligionInfo(eReligion).getTextKeyWide());
		}
		break;

	case BUTTONPOPUP_CHOOSETECH:
		if (info.getData1() > 0)
		{
			szString += gDLL->getText("TXT_KEY_MINIMIZED_CHOOSE_TECH_FREE");
		}
		else
		{
			szString += gDLL->getText("TXT_KEY_MINIMIZED_CHOOSE_TECH");
		}
		break;

	case BUTTONPOPUP_CHANGECIVIC:
		eCivic = ((CivicTypes)(info.getData2()));
		if (eCivic != NO_CIVIC)
		{
			szString += gDLL->getText("TXT_KEY_MINIMIZED_CHANGE_CIVIC", GC.getCivicInfo(eCivic).getTextKeyWide());
		}
		break;
	}
}

void CvGameTextMgr::setEspionageMissionHelp(CvWStringBuffer &szBuffer, const CvUnit* pUnit)
{
	if (pUnit->isSpy())
	{
		PlayerTypes eOwner =  pUnit->plot()->getOwner();
		if (NO_PLAYER != eOwner && GET_PLAYER(eOwner).getTeam() != pUnit->getTeam())
		{
			if (!pUnit->canEspionage(pUnit->plot()))
			{
				szBuffer.append(NEWLINE);
				szBuffer.append(gDLL->getText("TXT_KEY_UNIT_HELP_NO_ESPIONAGE"));

				if (pUnit->hasMoved() || pUnit->isMadeAttack())
				{
					szBuffer.append(gDLL->getText("TXT_KEY_UNIT_HELP_NO_ESPIONAGE_REASON_MOVED"));
				}
				else if (!pUnit->isInvisible(GET_PLAYER(eOwner).getTeam(), false))
				{
					szBuffer.append(gDLL->getText("TXT_KEY_UNIT_HELP_NO_ESPIONAGE_REASON_VISIBLE", GET_PLAYER(eOwner).getNameKey()));
				}
			}
			else if (pUnit->getFortifyTurns() > 0)
			{
				int iModifier = -(pUnit->getFortifyTurns() * GC.getDefineINT("ESPIONAGE_EACH_TURN_UNIT_COST_DECREASE"));
				if (iModifier != 0)
				{
					szBuffer.append(NEWLINE);
					szBuffer.append(gDLL->getText("TXT_KEY_ESPIONAGE_COST", iModifier));
				}
			}
		}
	}
}


void CvGameTextMgr::setUnitHelp(CvWStringBuffer &szString, const CvUnit* pUnit,
		bool bOneLine, bool bShort,
		bool bColorHostile, // advc.048
		bool bOmitOwner, // advc.061
		bool bIndicator) { // advc.007

	PROFILE_FUNC();

	CvWString szTempBuffer;
	int iI=-1;
	CvGame const& g = GC.getGame(); // advc.003
	bool bDebugMode = (g.isDebugMode() && !bIndicator); // advc.007
	bool bShift = GC.shiftKey();
	bool bAlt = GC.altKey();
	// <advc.007> Make info more compact in debug mode
	if(bDebugMode && bOneLine)
		bShort = true; // </advc.007>
	// <advc.048>
	char const* szColTag = "COLOR_UNIT_TEXT";
	if(bColorHostile) {
		szColTag = (pUnit->isEnemy(g.getActiveTeam()) ?
				"COLOR_WARNING_TEXT" : "COLOR_POSITIVE_TEXT");
	} // </advc.048>
	szTempBuffer.Format(SETCOLR L"%s" ENDCOLR, TEXT_COLOR(
			szColTag), // advc.048
			pUnit->getName().GetCString());
	szString.append(szTempBuffer);

	szString.append(L", ");

	if (pUnit->getDomainType() == DOMAIN_AIR)
	{
		if (pUnit->airBaseCombatStr() > 0)
		{
			if (pUnit->isFighting())
			{
				szTempBuffer.Format(L"?/%d%c, ", pUnit->airBaseCombatStr(), gDLL->getSymbolID(STRENGTH_CHAR));
			}
			else if (pUnit->isHurt())
			{
				szTempBuffer.Format(L"%.1f/%d%c, ", (((float)(pUnit->airBaseCombatStr() * pUnit->currHitPoints())) / ((float)(pUnit->maxHitPoints()))), pUnit->airBaseCombatStr(), gDLL->getSymbolID(STRENGTH_CHAR));
			}
			else
			{
				szTempBuffer.Format(L"%d%c, ", pUnit->airBaseCombatStr(), gDLL->getSymbolID(STRENGTH_CHAR));
			}
			szString.append(szTempBuffer);
		}
	}
	else
	{
		if (pUnit->canFight())
		{
			if (pUnit->isFighting())
			{
				szTempBuffer.Format(L"?/%d%c, ", pUnit->baseCombatStr(), gDLL->getSymbolID(STRENGTH_CHAR));
			}
			else if (pUnit->isHurt())
			{	// <advc.004>
				// as in BtS
				float fCurrStrength = pUnit->baseCombatStr() *
						pUnit->currHitPoints() / (float)pUnit->maxHitPoints();
				int iCurrStrengthTimes100 = (100 * pUnit->baseCombatStr() *
						pUnit->currHitPoints()) / pUnit->maxHitPoints();
				// Format would round up to e.g. 2.0/2 for a Warrior here
				if(pUnit->baseCombatStr() * pUnit->maxHitPoints() -
						iCurrStrengthTimes100 <= 5)
					fCurrStrength = pUnit->baseCombatStr() - 0.1f;
				// Format would show 0.0 here
				if(pUnit->baseCombatStr() * pUnit->maxHitPoints() < 5)
					fCurrStrength = 0.1f;
				// </advc.004>
				szTempBuffer.Format(L"%.1f/%d%c, ", fCurrStrength,
					pUnit->baseCombatStr(), gDLL->getSymbolID(STRENGTH_CHAR));
			}
			else
			{
				szTempBuffer.Format(L"%d%c, ", pUnit->baseCombatStr(), gDLL->getSymbolID(STRENGTH_CHAR));
			}
			szString.append(szTempBuffer);
		}
	}
	int const iDenom = GC.getMOVE_DENOMINATOR(); // advc.003
	int iCurrMoves = (pUnit->movesLeft() / iDenom) +
			(((pUnit->movesLeft() % iDenom) > 0) ? 1 : 0);
	// <advc.069>
	bool bFract = getBugOptionBOOL("MainInterface__UnitMovementPointsFraction", true);
	if(pUnit->baseMoves() == (bFract ? pUnit->movesLeft() : iCurrMoves)
			// </advc.069>
			|| pUnit->getTeam() != g.getActiveTeam())
		szTempBuffer.Format(L"%d%c", pUnit->baseMoves(), gDLL->getSymbolID(MOVES_CHAR));
	// advc.069: Display as in BtS (as integer) if non-fractional
	else if(!bFract || pUnit->movesLeft() == iCurrMoves * iDenom) {
		szTempBuffer.Format(L"%d/%d%c", iCurrMoves, pUnit->baseMoves(),
				gDLL->getSymbolID(MOVES_CHAR));
	}
	// <advc.069> Akin to BUG code in CvMainInterface.py (Unit Movement Fraction)
	else {
		float fCurrMoves = pUnit->movesLeft() / (float)iDenom;
		szTempBuffer.Format(L"%.1f/%d%c", fCurrMoves, pUnit->baseMoves(),
				gDLL->getSymbolID(MOVES_CHAR));
	} // </advc.069>
	szString.append(szTempBuffer);

	if (pUnit->airRange() > 0)
	{
		szString.append(gDLL->getText("TXT_KEY_UNIT_HELP_AIR_RANGE", pUnit->airRange()));
	}

	BuildTypes eBuild = pUnit->getBuildType();

	if (eBuild != NO_BUILD)
	{
		szString.append(L", ");
		/*  <advc.011b> Show turns-to-build as "(x+1)" when the Worker has been
			told not to finish it -- it'll build for x turns, make a pause,
			eventually build for 1 more turn. */
		szString.append(GC.getBuildInfo(eBuild).getDescription());
		int iTurns = pUnit->plot()->getBuildTurnsLeft(eBuild,
				/* advc.251: */ pUnit->getOwner(), 0, 0);
		bool bSuspend = false;
		if(iTurns > 1) {
			CLLNode<MissionData>* pNode = pUnit->getGroup()->headMissionQueueNode();
			if(pNode != NULL) {
				if(pNode->m_data.bModified && (GC.ctrlKey() ||
						getBugOptionBOOL("MiscHover__PartialBuildsAlways", false)))
					bSuspend = true;
			}
		}
		if(bSuspend) {
			szTempBuffer.Format(L"(%d+" SETCOLR L"%d" ENDCOLR L")", iTurns - 1,
					TEXT_COLOR("COLOR_HIGHLIGHT_TEXT"), 1);
			}
		else // </advc.011b>
			szTempBuffer.Format(L"(%d)", iTurns);
		szString.append(szTempBuffer);
	}

	if (pUnit->getImmobileTimer() > 0)
	{
		szString.append(L", ");
		szString.append(gDLL->getText("TXT_KEY_UNIT_HELP_IMMOBILE", pUnit->getImmobileTimer()));
	}

	/*if (!bOneLine) {
		if (pUnit->getUnitCombatType() != NO_UNITCOMBAT) {
			szTempBuffer.Format(L" (%s)", GC.getUnitCombatInfo(pUnit->getUnitCombatType()).getDescription());
			szString += szTempBuffer;
		}
	}*/

	// advc.007: Commented out.Can alway press Ctrl+Alt for this info.
	/*if (bDebugMode && !bAlt && !bShift) {
		FAssertMsg(pUnit->AI_getUnitAIType() != NO_UNITAI, "pUnit's AI type expected to != NO_UNITAI");
		szTempBuffer.Format(L" (%s)", GC.getUnitAIInfo(pUnit->AI_getUnitAIType()).getDescription());
		szString.append(szTempBuffer);
	}*/

	if ((pUnit->getTeam() == g.getActiveTeam()) || (bDebugMode
			&& !bOneLine)) // advc.007
	{
		if ((pUnit->getExperience() > 0) && !(pUnit->isFighting()))
		{
			szString.append(gDLL->getText("TXT_KEY_UNIT_HELP_LEVEL", pUnit->getExperience(), pUnit->experienceNeeded()));
		}
	}

	//if (pUnit->getOwner() != GC.getGame().getActivePlayer() && !pUnit->isAnimal() && !pUnit->getUnitInfo().isHiddenNationality())
	if(!bOmitOwner && !pUnit->isUnowned()) // advc.061: Replacing the above
	{
		CvPlayer const& kOwner = GET_PLAYER(pUnit->getOwner());
		szString.append(L", ");
		szTempBuffer.Format(SETCOLR L"%s" ENDCOLR, PLAYER_TEXT_COLOR(kOwner), kOwner.getName());
		szString.append(szTempBuffer);
	}
	bool bFirst = true; // advc.004
	for (iI = 0; iI < GC.getNumPromotionInfos(); ++iI)
	{
		if (pUnit->isHasPromotion((PromotionTypes)iI))
		{
			szTempBuffer.Format(L"<img=%S size=16 />", GC.getPromotionInfo((PromotionTypes)iI).getButton());
			// <advc.004>
			if(bFirst) {
				szString.append(' ');
				bFirst = false;
			} // </advc.004>
			szString.append(szTempBuffer);
		}
	}
	if (bAlt && //(gDLL->getChtLvl() > 0))
			bDebugMode) // advc.135c
	{
		CvSelectionGroup* eGroup = pUnit->getGroup();
		if (eGroup != NULL)
		{
			if (pUnit->isGroupHead())
				szString.append(CvWString::format(L"\nLeading "));
			else
				szString.append(L"\n");

			szTempBuffer.Format(L"Group(%d), %d units", eGroup->getID(), eGroup->getNumUnits());
			szString.append(szTempBuffer);
		}
	}

	if (!bOneLine)
	{
		setEspionageMissionHelp(szString, pUnit);

		if (pUnit->cargoSpace() > 0)
		{
			if (pUnit->getTeam() == g.getActiveTeam())
			{
				szTempBuffer = NEWLINE + gDLL->getText("TXT_KEY_UNIT_HELP_CARGO_SPACE", pUnit->getCargo(), pUnit->cargoSpace());
			}
			else
			{
				szTempBuffer = NEWLINE + gDLL->getText("TXT_KEY_UNIT_CARGO_SPACE", pUnit->cargoSpace());
			}
			szString.append(szTempBuffer);

			if (pUnit->specialCargo() != NO_SPECIALUNIT)
			{
				szString.append(gDLL->getText("TXT_KEY_UNIT_CARRIES", GC.getSpecialUnitInfo(pUnit->specialCargo()).getTextKeyWide()));
			}
		}

		if (pUnit->fortifyModifier() != 0)
		{
			szString.append(NEWLINE);
			szString.append(gDLL->getText("TXT_KEY_UNIT_HELP_FORTIFY_BONUS", pUnit->fortifyModifier()));
		}

		if (!bShort)
		{
			if (pUnit->nukeRange() >= 0)
			{
				szString.append(NEWLINE);
				szString.append(gDLL->getText("TXT_KEY_UNIT_CAN_NUKE"));
			}

			if (pUnit->alwaysInvisible())
			{
				szString.append(NEWLINE);
				szString.append(gDLL->getText("TXT_KEY_UNIT_INVISIBLE_ALL"));
			}
			else if (pUnit->getInvisibleType() != NO_INVISIBLE)
			{
				szString.append(NEWLINE);
				szString.append(gDLL->getText("TXT_KEY_UNIT_INVISIBLE_MOST"));
			}

			for (iI = 0; iI < pUnit->getNumSeeInvisibleTypes(); ++iI)
			{
				if (pUnit->getSeeInvisibleType(iI) != pUnit->getInvisibleType())
				{
					szString.append(NEWLINE);
					szString.append(gDLL->getText("TXT_KEY_UNIT_SEE_INVISIBLE", GC.getInvisibleInfo(pUnit->getSeeInvisibleType(iI)).getTextKeyWide()));
				}
			}

			if (pUnit->canMoveImpassable())
			{
				szString.append(NEWLINE);
				szString.append(gDLL->getText("TXT_KEY_UNIT_CAN_MOVE_IMPASSABLE"));
			}
		}

		if (pUnit->maxFirstStrikes() > 0)
		{
			if (pUnit->firstStrikes() == pUnit->maxFirstStrikes())
			{
				if (pUnit->firstStrikes() == 1)
				{
					szString.append(NEWLINE);
					szString.append(gDLL->getText("TXT_KEY_UNIT_ONE_FIRST_STRIKE"));
				}
				else
				{
					szString.append(NEWLINE);
					szString.append(gDLL->getText("TXT_KEY_UNIT_NUM_FIRST_STRIKES", pUnit->firstStrikes()));
				}
			}
			else
			{
				szString.append(NEWLINE);
				szString.append(gDLL->getText("TXT_KEY_UNIT_FIRST_STRIKE_CHANCES", pUnit->firstStrikes(), pUnit->maxFirstStrikes()));
			}
		}

		if (pUnit->immuneToFirstStrikes())
		{
			szString.append(NEWLINE);
			szString.append(gDLL->getText("TXT_KEY_UNIT_IMMUNE_FIRST_STRIKES"));
		}

		if (!bShort)
		{	// <advc.315> Whether a unit can attack is too important to omit
			if (pUnit->isOnlyDefensive())
			{
				szString.append(NEWLINE);
				szString.append(gDLL->getText("TXT_KEY_UNIT_ONLY_DEFENSIVE"));
			} // </advc.315>
			// <advc.315a> Same code as under setBasicUnitHelp
			if (pUnit->getUnitInfo().isOnlyAttackAnimals())
			{
				szString.append(NEWLINE);
				szString.append(gDLL->getText("TXT_KEY_UNIT_ONLY_ATTACK_ANIMALS"));
			} // </advc.315a>
			// <advc.315b>
			if (pUnit->getUnitInfo().isOnlyAttackBarbarians())
			{
				szString.append(NEWLINE);
				szString.append(gDLL->getText("TXT_KEY_UNIT_ONLY_ATTACK_BARBARIANS"));
			} // </advc.315b>
			if (pUnit->noDefensiveBonus())
			{
				szString.append(NEWLINE);
				szString.append(gDLL->getText("TXT_KEY_UNIT_NO_DEFENSE_BONUSES"));
			}

			if (pUnit->flatMovementCost())
			{
				szString.append(NEWLINE);
				szString.append(gDLL->getText("TXT_KEY_UNIT_FLAT_MOVEMENT"));
			}

			if (pUnit->ignoreTerrainCost())
			{
				szString.append(NEWLINE);
				szString.append(gDLL->getText("TXT_KEY_UNIT_IGNORE_TERRAIN"));
			}

			if (pUnit->isBlitz())
			{
				szString.append(NEWLINE);
				// <advc.164>
				int iBlitz = pUnit->getBlitzCount();
				if(iBlitz > 0) {
					if(iBlitz > 1) {
						szString.append(gDLL->getText("TXT_KEY_PROMOTION_BLITZ_LIMIT",
								iBlitz + 1));
					}
					else szString.append(gDLL->getText("TXT_KEY_PROMOTION_BLITZ_TWICE"));
				}
				else // </advc.164>
					szString.append(gDLL->getText("TXT_KEY_PROMOTION_BLITZ_TEXT"));
			}

			if (pUnit->isAmphib())
			{
				szString.append(NEWLINE);
				szString.append(gDLL->getText("TXT_KEY_PROMOTION_AMPHIB_TEXT"));
			}

			if (pUnit->isRiver())
			{
				szString.append(NEWLINE);
				szString.append(gDLL->getText("TXT_KEY_PROMOTION_RIVER_ATTACK_TEXT"));
			}

			if (pUnit->isEnemyRoute())
			{
				szString.append(NEWLINE);
				szString.append(gDLL->getText("TXT_KEY_PROMOTION_ENEMY_ROADS_TEXT"));
			}

			if (pUnit->isAlwaysHeal())
			{
				szString.append(NEWLINE);
				szString.append(gDLL->getText("TXT_KEY_PROMOTION_ALWAYS_HEAL_TEXT"));
			}

			if (pUnit->isHillsDoubleMove())
			{
				szString.append(NEWLINE);
				szString.append(gDLL->getText("TXT_KEY_PROMOTION_HILLS_MOVE_TEXT"));
			}

			for (iI = 0; iI < GC.getNumTerrainInfos(); ++iI)
			{
				if (pUnit->isTerrainDoubleMove((TerrainTypes)iI))
				{
					szString.append(NEWLINE);
					szString.append(gDLL->getText("TXT_KEY_PROMOTION_DOUBLE_MOVE_TEXT", GC.getTerrainInfo((TerrainTypes) iI).getTextKeyWide()));
				}
			}

			for (iI = 0; iI < GC.getNumFeatureInfos(); ++iI)
			{
				if (pUnit->isFeatureDoubleMove((FeatureTypes)iI))
				{
					szString.append(NEWLINE);
					szString.append(gDLL->getText("TXT_KEY_PROMOTION_DOUBLE_MOVE_TEXT", GC.getFeatureInfo((FeatureTypes) iI).getTextKeyWide()));
				}
			}

			if (pUnit->getExtraVisibilityRange() != 0)
			{
				szString.append(NEWLINE);
				szString.append(gDLL->getText("TXT_KEY_PROMOTION_VISIBILITY_TEXT", pUnit->getExtraVisibilityRange()));
			}

			if (pUnit->getExtraMoveDiscount() != 0)
			{
				szString.append(NEWLINE);
				szString.append(gDLL->getText("TXT_KEY_PROMOTION_MOVE_DISCOUNT_TEXT", -(pUnit->getExtraMoveDiscount())));
			}

			if (pUnit->getExtraEnemyHeal() != 0)
			{
				szString.append(NEWLINE);
				szString.append(gDLL->getText("TXT_KEY_PROMOTION_HEALS_EXTRA_TEXT", pUnit->getExtraEnemyHeal()) + gDLL->getText("TXT_KEY_PROMOTION_ENEMY_LANDS_TEXT"));
			}

			if (pUnit->getExtraNeutralHeal() != 0)
			{
				szString.append(NEWLINE);
				szString.append(gDLL->getText("TXT_KEY_PROMOTION_HEALS_EXTRA_TEXT", pUnit->getExtraNeutralHeal()) + gDLL->getText("TXT_KEY_PROMOTION_NEUTRAL_LANDS_TEXT"));
			}

			if (pUnit->getExtraFriendlyHeal() != 0)
			{
				szString.append(NEWLINE);
				szString.append(gDLL->getText("TXT_KEY_PROMOTION_HEALS_EXTRA_TEXT", pUnit->getExtraFriendlyHeal()) + gDLL->getText("TXT_KEY_PROMOTION_FRIENDLY_LANDS_TEXT"));
			}

			if (pUnit->getSameTileHeal() != 0)
			{
				szString.append(NEWLINE);
				szString.append(gDLL->getText("TXT_KEY_PROMOTION_HEALS_SAME_TEXT", pUnit->getSameTileHeal()) + gDLL->getText("TXT_KEY_PROMOTION_DAMAGE_TURN_TEXT"));
			}

			if (pUnit->getAdjacentTileHeal() != 0)
			{
				szString.append(NEWLINE);
				szString.append(gDLL->getText("TXT_KEY_PROMOTION_HEALS_ADJACENT_TEXT", pUnit->getAdjacentTileHeal()) + gDLL->getText("TXT_KEY_PROMOTION_DAMAGE_TURN_TEXT"));
			}
		}

		if (pUnit->currInterceptionProbability() > 0)
		{
			szString.append(NEWLINE);
			szString.append(gDLL->getText("TXT_KEY_UNIT_INTERCEPT_AIRCRAFT", pUnit->currInterceptionProbability()));
		}

		if (pUnit->evasionProbability() > 0)
		{
			szString.append(NEWLINE);
			szString.append(gDLL->getText("TXT_KEY_UNIT_EVADE_INTERCEPTION", pUnit->evasionProbability()));
		}

		if (pUnit->withdrawalProbability() > 0)
		{
			if (bShort)
			{
				szString.append(NEWLINE);
				szString.append(gDLL->getText("TXT_KEY_UNIT_WITHDRAWL_PROBABILITY_SHORT", pUnit->withdrawalProbability()));
			}
			else
			{
				szString.append(NEWLINE);
				szString.append(gDLL->getText("TXT_KEY_UNIT_WITHDRAWL_PROBABILITY", pUnit->withdrawalProbability()));
			}
		}

		if (pUnit->combatLimit() < GC.getMAX_HIT_POINTS() && pUnit->canAttack())
		{
			szString.append(NEWLINE);
			szString.append(gDLL->getText("TXT_KEY_UNIT_COMBAT_LIMIT", (100 * pUnit->combatLimit()) / GC.getMAX_HIT_POINTS()));
		}

		if (pUnit->collateralDamage() > 0)
		{
			szString.append(NEWLINE);
			if (pUnit->getExtraCollateralDamage() == 0)
			{
				szString.append(gDLL->getText("TXT_KEY_UNIT_COLLATERAL_DAMAGE", ( 100 * pUnit->getUnitInfo().getCollateralDamageLimit() / GC.getMAX_HIT_POINTS())));
			}
			else
			{
				szString.append(gDLL->getText("TXT_KEY_UNIT_COLLATERAL_DAMAGE_EXTRA", pUnit->getExtraCollateralDamage()));
			}
		}

		for (iI = 0; iI < GC.getNumUnitCombatInfos(); ++iI)
		{
			if (pUnit->getUnitInfo().getUnitCombatCollateralImmune(iI))
			{
				szString.append(NEWLINE);
				szString.append(gDLL->getText("TXT_KEY_UNIT_COLLATERAL_IMMUNE", GC.getUnitCombatInfo((UnitCombatTypes)iI).getTextKeyWide()));
			}
		}

		if (pUnit->getCollateralDamageProtection() > 0)
		{
			szString.append(NEWLINE);
			szString.append(gDLL->getText("TXT_KEY_PROMOTION_COLLATERAL_PROTECTION_TEXT", pUnit->getCollateralDamageProtection()));
		}

		if (pUnit->getExtraCombatPercent() != 0)
		{
			szString.append(NEWLINE);
			szString.append(gDLL->getText("TXT_KEY_PROMOTION_STRENGTH_TEXT", pUnit->getExtraCombatPercent()));
		}

		if (pUnit->cityAttackModifier() == pUnit->cityDefenseModifier())
		{
			if (pUnit->cityAttackModifier() != 0)
			{
				szString.append(NEWLINE);
				szString.append(gDLL->getText("TXT_KEY_UNIT_CITY_STRENGTH_MOD", pUnit->cityAttackModifier()));
			}
		}
		else
		{
			if (pUnit->cityAttackModifier() != 0)
			{
				szString.append(NEWLINE);
				szString.append(gDLL->getText("TXT_KEY_PROMOTION_CITY_ATTACK_TEXT", pUnit->cityAttackModifier()));
			}

			if (pUnit->cityDefenseModifier() != 0)
			{
				szString.append(NEWLINE);
				szString.append(gDLL->getText("TXT_KEY_PROMOTION_CITY_DEFENSE_TEXT", pUnit->cityDefenseModifier()));
			}
		}

		if (pUnit->animalCombatModifier() != 0)
		{
			szString.append(NEWLINE);
			szString.append(gDLL->getText("TXT_KEY_UNIT_ANIMAL_COMBAT_MOD", pUnit->animalCombatModifier()));
		}
		// <advc.315c>
		if (pUnit->barbarianCombatModifier() != 0)
		{
			szString.append(NEWLINE);
			szString.append(gDLL->getText("TXT_KEY_UNIT_BARBARIAN_COMBAT_MOD",
					pUnit->barbarianCombatModifier()));
		} // </advc.315c>

		if (pUnit->getDropRange() > 0)
		{
			szString.append(NEWLINE);
			szString.append(gDLL->getText("TXT_KEY_UNIT_PARADROP_RANGE", pUnit->getDropRange()));
		}

		if (pUnit->hillsAttackModifier() == pUnit->hillsDefenseModifier())
		{
			if (pUnit->hillsAttackModifier() != 0)
			{
				szString.append(NEWLINE);
				szString.append(gDLL->getText("TXT_KEY_UNIT_HILLS_STRENGTH", pUnit->hillsAttackModifier()));
			}
		}
		else
		{
			if (pUnit->hillsAttackModifier() != 0)
			{
				szString.append(NEWLINE);
				szString.append(gDLL->getText("TXT_KEY_UNIT_HILLS_ATTACK", pUnit->hillsAttackModifier()));
			}

			if (pUnit->hillsDefenseModifier() != 0)
			{
				szString.append(NEWLINE);
				szString.append(gDLL->getText("TXT_KEY_UNIT_HILLS_DEFENSE", pUnit->hillsDefenseModifier()));
			}
		}

		for (iI = 0; iI < GC.getNumTerrainInfos(); ++iI)
		{
			if (pUnit->terrainAttackModifier((TerrainTypes)iI) == pUnit->terrainDefenseModifier((TerrainTypes)iI))
			{
				if (pUnit->terrainAttackModifier((TerrainTypes)iI) != 0)
				{
					szString.append(NEWLINE);
					szString.append(gDLL->getText("TXT_KEY_UNIT_STRENGTH", pUnit->terrainAttackModifier((TerrainTypes)iI), GC.getTerrainInfo((TerrainTypes) iI).getTextKeyWide()));
				}
			}
			else
			{
				if (pUnit->terrainAttackModifier((TerrainTypes)iI) != 0)
				{
					szString.append(NEWLINE);
					szString.append(gDLL->getText("TXT_KEY_UNIT_ATTACK", pUnit->terrainAttackModifier((TerrainTypes)iI), GC.getTerrainInfo((TerrainTypes) iI).getTextKeyWide()));
				}

				if (pUnit->terrainDefenseModifier((TerrainTypes)iI) != 0)
				{
					szString.append(NEWLINE);
					szString.append(gDLL->getText("TXT_KEY_UNIT_DEFENSE", pUnit->terrainDefenseModifier((TerrainTypes)iI), GC.getTerrainInfo((TerrainTypes) iI).getTextKeyWide()));
				}
			}
		}

		for (iI = 0; iI < GC.getNumFeatureInfos(); ++iI)
		{
			if (pUnit->featureAttackModifier((FeatureTypes)iI) == pUnit->featureDefenseModifier((FeatureTypes)iI))
			{
				if (pUnit->featureAttackModifier((FeatureTypes)iI) != 0)
				{
					szString.append(NEWLINE);
					szString.append(gDLL->getText("TXT_KEY_UNIT_STRENGTH", pUnit->featureAttackModifier((FeatureTypes)iI), GC.getFeatureInfo((FeatureTypes) iI).getTextKeyWide()));
				}
			}
			else
			{
				if (pUnit->featureAttackModifier((FeatureTypes)iI) != 0)
				{
					szString.append(NEWLINE);
					szString.append(gDLL->getText("TXT_KEY_UNIT_ATTACK", pUnit->featureAttackModifier((FeatureTypes)iI), GC.getFeatureInfo((FeatureTypes) iI).getTextKeyWide()));
				}

				if (pUnit->featureDefenseModifier((FeatureTypes)iI) != 0)
				{
					szString.append(NEWLINE);
					szString.append(gDLL->getText("TXT_KEY_UNIT_DEFENSE", pUnit->featureDefenseModifier((FeatureTypes)iI), GC.getFeatureInfo((FeatureTypes) iI).getTextKeyWide()));
				}
			}
		}

		for (iI = 0; iI < GC.getNumUnitClassInfos(); ++iI)
		{
			if (pUnit->getUnitInfo().getUnitClassAttackModifier(iI) == GC.getUnitInfo(pUnit->getUnitType()).getUnitClassDefenseModifier(iI))
			{
				if (pUnit->getUnitInfo().getUnitClassAttackModifier(iI) != 0)
				{
					szString.append(NEWLINE);
					szString.append(gDLL->getText("TXT_KEY_UNIT_MOD_VS_TYPE", pUnit->getUnitInfo().getUnitClassAttackModifier(iI), GC.getUnitClassInfo((UnitClassTypes)iI).getTextKeyWide()));
				}
			}
			else
			{
				if (pUnit->getUnitInfo().getUnitClassAttackModifier(iI) != 0)
				{
					szString.append(NEWLINE);
					szString.append(gDLL->getText("TXT_KEY_UNIT_ATTACK_MOD_VS_CLASS", pUnit->getUnitInfo().getUnitClassAttackModifier(iI), GC.getUnitClassInfo((UnitClassTypes)iI).getTextKeyWide()));
				}

				if (pUnit->getUnitInfo().getUnitClassDefenseModifier(iI) != 0)
				{
					szString.append(NEWLINE);
					szString.append(gDLL->getText("TXT_KEY_UNIT_DEFENSE_MOD_VS_CLASS", pUnit->getUnitInfo().getUnitClassDefenseModifier(iI), GC.getUnitClassInfo((UnitClassTypes) iI).getTextKeyWide()));
				}
			}
		}

		for (iI = 0; iI < GC.getNumUnitCombatInfos(); ++iI)
		{
			if (pUnit->unitCombatModifier((UnitCombatTypes)iI) != 0)
			{
				szString.append(NEWLINE);
				szString.append(gDLL->getText("TXT_KEY_UNIT_MOD_VS_TYPE", pUnit->unitCombatModifier((UnitCombatTypes)iI), GC.getUnitCombatInfo((UnitCombatTypes) iI).getTextKeyWide()));
			}
		}

		for (iI = 0; iI < NUM_DOMAIN_TYPES; ++iI)
		{
			if (pUnit->domainModifier((DomainTypes)iI) != 0)
			{
				szString.append(NEWLINE);
				szString.append(gDLL->getText("TXT_KEY_UNIT_MOD_VS_TYPE", pUnit->domainModifier((DomainTypes)iI), GC.getDomainInfo((DomainTypes)iI).getTextKeyWide()));
			}
		}

		szTempBuffer.clear();
		bool bFirst = true;
		for (iI = 0; iI < GC.getNumUnitClassInfos(); ++iI)
		{
			if (pUnit->getUnitInfo().getTargetUnitClass(iI))
			{
				if (bFirst)
				{
					bFirst = false;
				}
				else
				{
					szTempBuffer += L", ";
				}

				szTempBuffer += CvWString::format(L"<link=literal>%s</link>", GC.getUnitClassInfo((UnitClassTypes)iI).getDescription());
			}
		}

		if (!bFirst)
		{
			szString.append(NEWLINE);
			szString.append(gDLL->getText("TXT_KEY_UNIT_TARGETS_UNIT_FIRST", szTempBuffer.GetCString()));
		}

		szTempBuffer.clear();
		bFirst = true;
		for (iI = 0; iI < GC.getNumUnitClassInfos(); ++iI)
		{
			if (pUnit->getUnitInfo().getDefenderUnitClass(iI))
			{
				if (bFirst)
				{
					bFirst = false;
				}
				else
				{
					szTempBuffer += L", ";
				}

				szTempBuffer += CvWString::format(L"<link=literal>%s</link>", GC.getUnitClassInfo((UnitClassTypes)iI).getDescription());
			}
		}

		if (!bFirst)
		{
			szString.append(NEWLINE);
			szString.append(gDLL->getText("TXT_KEY_UNIT_DEFENDS_UNIT_FIRST", szTempBuffer.GetCString()));
		}

		szTempBuffer.clear();
		bFirst = true;
		for (iI = 0; iI < GC.getNumUnitCombatInfos(); ++iI)
		{
			if (pUnit->getUnitInfo().getTargetUnitCombat(iI))
			{
				if (bFirst)
				{
					bFirst = false;
				}
				else
				{
					szTempBuffer += L", ";
				}

				szTempBuffer += CvWString::format(L"<link=literal>%s</link>", GC.getUnitCombatInfo((UnitCombatTypes)iI).getDescription());
			}
		}

		if (!bFirst)
		{
			szString.append(NEWLINE);
			szString.append(gDLL->getText("TXT_KEY_UNIT_TARGETS_UNIT_FIRST", szTempBuffer.GetCString()));
		}

		szTempBuffer.clear();
		bFirst = true;
		for (iI = 0; iI < GC.getNumUnitCombatInfos(); ++iI)
		{
			if (pUnit->getUnitInfo().getDefenderUnitCombat(iI))
			{
				if (bFirst)
				{
					bFirst = false;
				}
				else
				{
					szTempBuffer += L", ";
				}

				szTempBuffer += CvWString::format(L"<link=literal>%s</link>", GC.getUnitCombatInfo((UnitCombatTypes)iI).getDescription());
			}
		}

		if (!bFirst)
		{
			szString.append(NEWLINE);
			szString.append(gDLL->getText("TXT_KEY_UNIT_DEFENDS_UNIT_FIRST", szTempBuffer.GetCString()));
		}

		szTempBuffer.clear();
		bFirst = true;
		for (iI = 0; iI < GC.getNumUnitClassInfos(); ++iI)
		{
			if (pUnit->getUnitInfo().getFlankingStrikeUnitClass(iI) > 0)
			{
				if (bFirst)
				{
					bFirst = false;
				}
				else
				{
					szTempBuffer += L", ";
				}

				szTempBuffer += CvWString::format(L"<link=literal>%s</link>", GC.getUnitClassInfo((UnitClassTypes)iI).getDescription());
			}
		}

		if (!bFirst)
		{
			szString.append(NEWLINE);
			szString.append(gDLL->getText("TXT_KEY_UNIT_FLANKING_STRIKES", szTempBuffer.GetCString()));
		}

		if (pUnit->bombardRate() > 0)
		{
			if (bShort)
			{
				szString.append(NEWLINE);
				szString.append(gDLL->getText("TXT_KEY_UNIT_BOMBARD_RATE_SHORT", ((pUnit->bombardRate() * 100) / GC.getMAX_CITY_DEFENSE_DAMAGE())));
			}
			else
			{
				szString.append(NEWLINE);
				szString.append(gDLL->getText("TXT_KEY_UNIT_BOMBARD_RATE", ((pUnit->bombardRate() * 100) / GC.getMAX_CITY_DEFENSE_DAMAGE())));
			}
		}

		if (pUnit->isSpy())
		{
			szString.append(NEWLINE);
			szString.append(gDLL->getText("TXT_KEY_UNIT_IS_SPY"));
		}

		if (pUnit->getUnitInfo().isNoRevealMap())
		{
			szString.append(NEWLINE);
			szString.append(gDLL->getText("TXT_KEY_UNIT_VISIBILITY_MOVE_RANGE"));
		}

		if (!CvWString(pUnit->getUnitInfo().getHelp()).empty())
		{
			szString.append(NEWLINE);
			szString.append(pUnit->getUnitInfo().getHelp());
		}

		if (bShift && //(gDLL->getChtLvl() > 0))
				bDebugMode) // advc.135c
		{
			szTempBuffer.Format(L"\nUnitAI Type = %s.", GC.getUnitAIInfo(pUnit->AI_getUnitAIType()).getDescription());
			szString.append(szTempBuffer);
			szTempBuffer.Format(L"\nSacrifice Value = %d.", pUnit->AI_sacrificeValue(NULL));
			szString.append(szTempBuffer);
		}
	}
}

// <advc.061>
bool CvGameTextMgr::listFirstUnitBeforeSecond(CvUnit const* pFirst, CvUnit const* pSecond) {

	// Top priority: cargo - but only if owned by active team
	TeamTypes eTeam = pFirst->getTeam(); // Caller ensures that both have same owner
	// ... which also means that these ids are unique:
	int iFirstID = pFirst->getID();
	int iSecondID = pSecond->getID();
	FAssert(pFirst != pSecond && iFirstID != iSecondID);
	CvGame const& g = GC.getGame();
	if(eTeam == g.getActiveTeam() || g.isDebugMode()) {
		bool bFirstCargo = (pFirst->getCargo() > 0);
		bool bSecondCargo = (pSecond->getCargo() > 0);
		CvUnit const* pFirstTransport = (bFirstCargo ? pFirst :
				pFirst->getTransportUnit());
		CvUnit const* pSecondTransport = (bSecondCargo ? pSecond :
				pSecond->getTransportUnit());
		bool bFirstTransport = (pFirstTransport != NULL);
		bool bSecondTransport = (pSecondTransport != NULL);
		if(bFirstTransport != bSecondTransport)
			return bFirstTransport;
		if(bFirstTransport) { // I.e. they're both involved with cargo
			UnitTypes eFirstTransportType = pFirstTransport->getUnitType();
			UnitTypes eSecondTransportType = pSecondTransport->getUnitType();
			if(eFirstTransportType != eSecondTransportType) {
				return listFirstUnitTypeBeforeSecond(eFirstTransportType,
						eSecondTransportType);
			}
			int iFirstCargo = pFirstTransport->getCargo();
			int iSecondCargo = pSecondTransport->getCargo();
			if(iFirstCargo != iSecondCargo)
				return (iFirstCargo > iSecondCargo);
			// Use transport id to keep transport and cargo together
			int iFirstTransportID = pFirstTransport->getID();
			int iSecondTransportID = pSecondTransport->getID();
			if(iFirstTransportID != iSecondTransportID)
				return (iFirstTransportID < iSecondTransportID);
			// Only transport and its cargo left. Transport goes first.
			if(bFirstCargo != bSecondCargo)
				return bFirstCargo;
			/*  Now we know that pFirst and pSecond are loaded in the same transport.
				Let the non-transport code below determine their order. */
		}
	}
	UnitTypes eFirst = pFirst->getUnitType();
	UnitTypes eSecond = pSecond->getUnitType();
	if(eFirst == eSecond) {
		// Make the order stable
		return (iFirstID < iSecondID);
	}
	return listFirstUnitTypeBeforeSecond(eFirst, eSecond);
}

CvPlot const* CvGameTextMgr::m_pHelpPlot = NULL;

bool CvGameTextMgr::listFirstUnitTypeBeforeSecond(UnitTypes eFirst, UnitTypes eSecond) {

	CvUnit* pCenter = (m_pHelpPlot == NULL ? NULL : m_pHelpPlot->getCenterUnit());
	UnitTypes eCenterType = (pCenter == NULL ? NO_UNIT :
			pCenter->getUnitType());
	FAssert(eCenterType != NO_UNIT);
	if(eFirst != eSecond) {
		if(eFirst == eCenterType)
			return true;
		if(eSecond == eCenterType)
			return false;
	}
	CvUnitInfo const& u1 = GC.getUnitInfo(eFirst);
	CvUnitInfo const& u2 = GC.getUnitInfo(eSecond);
	int iFirstCombat = u1.getCombat();
	int iSecondCombat = u2.getCombat();
	if((iFirstCombat > 0) != (iSecondCombat > 0))
		return (iFirstCombat > 0);
	int iFirstDomain = u1.getDomainType();
	int iSecondDomain = u2.getDomainType();
	if(iFirstDomain != iSecondDomain) {
		DomainTypes eCenterDomain = (pCenter == NULL ? NO_DOMAIN :
				pCenter->getDomainType());
		FAssert(eCenterDomain != NO_DOMAIN);
		if(iFirstDomain == eCenterDomain)
			return true;
		if(iSecondDomain == eCenterDomain)
			return false;
		return (iFirstDomain > iSecondDomain);
	}
	if(iFirstCombat != iSecondCombat)
		return (iFirstCombat > iSecondCombat);
	return eFirst > eSecond;
}

void CvGameTextMgr::appendUnitOwnerHeading(CvWStringBuffer& szString, PlayerTypes eOwner,
		int iArmy, int iNavy, int iAir, int iTotal, bool bCollapsed) {

	int iOther = iTotal - iArmy - iNavy; // Don't display iAir separately for now
	// Don't distinguish categories when there are few units in each
	if(iArmy < 5 && iNavy < 5 && iOther < 5 && !bCollapsed) {
		iOther = iTotal;
		iArmy = 0;
		iNavy = 0;
	}
	FAssert(iOther >= 0);
	if(!szString.isEmpty()) // No newline when PlotListHelp starts with a heading
		szString.append(NEWLINE);
	CvPlayer const& kOwner = GET_PLAYER(eOwner);
	szString.append(CvWString::format(SETCOLR L"%s" ENDCOLR,
			PLAYER_TEXT_COLOR(kOwner), kOwner.getName()));
	CvWString szCounts;
	if(iArmy >= iTotal || iNavy >= iTotal || iOther >= iTotal) {
		szCounts = CvWString::format(L" (%d %s)", iTotal,
				gDLL->getText("TXT_KEY_WB_UNITS").c_str());
	}
	else {
		szCounts = L" (";
		bool bFirst = true;
		if(iArmy > 0) {
			if(!bFirst)
				szCounts += L", ";
			bFirst = false;
			szCounts += CvWString::format(L"%d %s", iArmy,
					gDLL->getText("TXT_KEY_MISC_ARMY").c_str());
		}
		if(iNavy > 0) {
			if(!bFirst)
				szCounts += L", ";
			bFirst = false;
			szCounts += CvWString::format(L"%d %s", iNavy,
					gDLL->getText("TXT_KEY_MISC_NAVY").c_str());
		}
		if(iOther > 0) {
			szCounts += L", ";
			szCounts += CvWString::format(L"%d %s", iOther,
					gDLL->getText("TXT_KEY_MISC_OTHER").c_str());
		}
		szCounts += L")";
	}
	szString.append(szCounts);
	szString.append(L":");
}

// Reuses bits and pieces from setPlotListHelp
void CvGameTextMgr::appendUnitTypeAggregated(CvWStringBuffer& szString,
		std::vector<CvUnit const*> const& ownerUnits, UnitTypes eUnit,
		CvPlot const& kPlot, bool bIndicator) { // advc.007

	CvUnit* pCenterUnit = kPlot.getCenterUnit();
	int iCount = 0;
	// These sums are computed as in BtS
	int iSumStrengthTimes100 = 0;
	int iSumMaxStrengthTimes100 = 0;
	std::vector<CvUnit const*> warlords;
	int const iPromotions = GC.getNumPromotionInfos();
	std::vector<int> promotionCounts(iPromotions, 0);
	for(size_t i = 0; i < ownerUnits.size(); i++) {
		CvUnit const* pUnit = ownerUnits[i];
		if(pUnit == NULL) {
			FAssert(pUnit != NULL);
			continue;
		}
		if(pUnit == pCenterUnit) // Already handled by caller
			continue;
		if(pUnit->getUnitType() != eUnit)
			continue;
		if(pUnit->getLeaderUnitType() != NO_UNIT) {
			warlords.push_back(pUnit);
			// These go on a separate line each
			continue;
		}
		iCount++;
		for(int j = 0; j < iPromotions; j++) {
			if(pUnit->isHasPromotion((PromotionTypes)j))
				promotionCounts[j]++;
		}
		if(pUnit->maxHitPoints() <= 0)
			continue;
		int iBase = (pUnit->getDomainType() == DOMAIN_AIR ?
				pUnit->airBaseCombatStr() : pUnit->baseCombatStr());
		if(iBase > 0) {
			iSumStrengthTimes100 += (100 * iBase * pUnit->currHitPoints()) /
					pUnit->maxHitPoints();
			iSumMaxStrengthTimes100 += 100 * iBase;
		}
	}
	if(iCount > 0) {
		szString.append(NEWLINE);
		CvUnitInfo const& u = GC.getUnitInfo(eUnit);
		szString.append(CvWString::format(SETCOLR L"%s" ENDCOLR,
				TEXT_COLOR("COLOR_UNIT_TEXT"), u.getDescription()));
		appendAverageStrength(szString, iSumMaxStrengthTimes100,
				iSumStrengthTimes100, iCount);
		// Just like in setPlotListHelp (but with a vector)
		for(int j = 0; j < iPromotions; j++) {
			if(promotionCounts[j] > 0) {
				szString.append(CvWString::format(L"%d<img=%S size=16 />",
						promotionCounts[j],
						GC.getPromotionInfo((PromotionTypes)j).getButton()));
			}
		}
	}
	for(size_t i = 0; i < warlords.size(); i++) {
		szString.append(NEWLINE);
		setUnitHelp(szString, warlords[i], true, true, false, true,
				bIndicator); // advc.007
	}
}

// Cut and pasted (and refactored) from setPlotListHelp
void CvGameTextMgr::appendAverageStrength(CvWStringBuffer& szString,
		int iSumMaxStrengthTimes100, int iSumStrengthTimes100, int iUnits) {

	if(iUnits > 1) // advc: This condition is new
		szString.append(CvWString::format(L" (%d)", iUnits));
	if(iSumMaxStrengthTimes100 <= 0)
		return;
	int iBase = (iSumMaxStrengthTimes100 / iUnits) / 100;
	int iCurrent = (iSumStrengthTimes100 / iUnits) / 100;
	int iCurrentRemainder = (iSumStrengthTimes100 / iUnits) % 100;
	if(iCurrentRemainder <= 0) {
		if(iBase == iCurrent)
			szString.append(CvWString::format(L" %d", iBase));
		else szString.append(CvWString::format(L" %d/%d", iCurrent, iBase));
	}
	else {
		szString.append(CvWString::format(L" %d.%02d/%d",
				iCurrent, iCurrentRemainder, iBase));
	}
	szString.append(CvWString::format(L"%c", gDLL->getSymbolID(STRENGTH_CHAR)));
}


void CvGameTextMgr::setPlotListHelpPerOwner(CvWStringBuffer& szString,
		CvPlot const& kPlot, bool bIndicator, bool bShort) {

	if(kPlot.getCenterUnit() == NULL)
		return;
	// <advc.002b>
	int iFontSize = 12; // default when there is no custom theme
	CvArtInfoMisc const* pTheme = ARTFILEMGR.getMiscArtInfo("DEFAULT_THEME_NAME");
	if(pTheme != NULL && pTheme->getPath() != NULL) {
		CvString szThemePath(pTheme->getPath());
		/*  Don't know how to look up the font size. Would perhaps have to
			(re-)parse the theme files (no, thanks). Instead, the DLL is told
			through XML what font size to assume.
			I do know how to check if the mod's theme has been removed, and I don't
			want to rely on players changing the XML setting after removing it. */
		if(szThemePath.find("Mods") != CvString::npos)
			iFontSize = ::range(GC.getDefineINT("FONT_SIZE_FACTOR", 13), 7, 19);
	}
	// (The code below was written for iFontSize=14, so that's fontFactor=1.)
	double fontFactor = 14.0 / iFontSize;
	// </advc.002b>
	CvGame const& g = GC.getGame();
	int iScreenHeight = g.getScreenHeight();
	int iLineLimit = (iScreenHeight == 0 ? 25 :
			::round(32 * fontFactor * g.getScreenHeight() / 1000.0 - 5));
	/*  When hovering over an indicator bubble (unit layer), only info about units
		in kPlot is shown. This means more space. Same when hovering over a flag
		(bShort). */
	if(bIndicator || bShort)
		iLineLimit += 4;
	TeamTypes eActiveTeam = g.getActiveTeam();
	PlayerTypes eActivePlayer = g.getActivePlayer();
	// Adjust to other info to be displayed
	iLineLimit += 4;
	if(kPlot.getImprovementType() != NO_IMPROVEMENT)
		iLineLimit--;
	if(kPlot.getRouteType() != NO_ROUTE)
		iLineLimit--;
	if(kPlot.getFeatureType() != NO_FEATURE)
		iLineLimit--;
	if(kPlot.defenseModifier(eActiveTeam, true) > 0)
		iLineLimit--;
	if(kPlot.isFreshWater())
		iLineLimit--;
	for(int i = 0; i < MAX_PLAYERS; i++) {
		if(kPlot.calculateCulturePercent((PlayerTypes)i) > 0)
			iLineLimit--;
	}
	iLineLimit = std::max(iLineLimit, 15);
	/*  BtS shows no owner for Privateers and Animals (but it does for Barbarians);
		I'm calling units without a visible owner "rogue" units and they get their
		own entries in the perOwner vector. I'm also treating Barbarians inside
		cities that way b/c only Barbarian units (and spies) can exist there.
		These distinctions are implemented in CvUnit::isUnowned b/c I also want
		rogues to be preferred as the center unit -- this makes sure that the
		center unit can be listed first w/o the need for a heading.
		(That said, I'm also revealing the owner of Privateers when they're stacked
		with non-Privateers, meaning that such Privateers aren't rogues and that
		rogues really can't be stacked with non-rogues, so that change to the
		center unit is arguably obsolete.) */
	int const iRogueIndex = MAX_PLAYERS;
	int const ARMY = 0;
	int const NAVY = 1;
	int const AIR = 2;
	int const OTHER = 3;
	int const ALL = 4;
	std::vector<CvUnit const*> perOwner[iRogueIndex + 1][ALL+1];
	CLLNode<IDInfo>* pNode = kPlot.headUnitNode();
	while(pNode != NULL) {
		CvUnit* pUnit = ::getUnit(pNode->m_data);
		pNode = kPlot.nextUnitNode(pNode);
		if(pUnit == NULL) {
			FAssert(pUnit != NULL);
			continue;
		}
		if(pUnit->isInvisible(eActiveTeam, true))
			continue;
		PlayerTypes eVisualOwner = pUnit->getVisualOwner(eActiveTeam);
		if(eVisualOwner == NO_PLAYER) {
			FAssert(eVisualOwner != NO_PLAYER);
			continue;
		}
		int iType = OTHER;
		if(pUnit->baseCombatStr() > 0) {
			DomainTypes eDomain = pUnit->getDomainType();
			if(eDomain == DOMAIN_LAND)
				iType = ARMY;
			else if(eDomain == DOMAIN_SEA)
				iType = NAVY;
			else if(eDomain == DOMAIN_AIR)
				iType = AIR;
		}
		if(pUnit->isUnowned()) {
			perOwner[iRogueIndex][iType].push_back(pUnit);
			perOwner[iRogueIndex][ALL].push_back(pUnit);
		}
		else {
			perOwner[eVisualOwner][iType].push_back(pUnit);
			perOwner[eVisualOwner][ALL].push_back(pUnit);
		}
	}
	int iHeadings = 0;
	uint uTotal = perOwner[iRogueIndex][ALL].size();
	for(int i = 0; i < iRogueIndex; i++) { // Rogue units don't get a heading
		int iSize = perOwner[i][ALL].size();
		if(iSize > 0) {
			//uTotal += iSize; // Not good enough; need to anticipate linewrap.
			for(int j = 0; j < iSize; j++) {
				uTotal++;
				// Ad hoc heuristic for estimating the required (horizontal) space
				int iSpaceValue = perOwner[i][ALL][j]->getName().length()
						+ (8 * perOwner[i][ALL][j]->getExperience()) / 5;
				if(perOwner[i][ALL][j]->getDamage() > 0)
					iSpaceValue += 2;
				if(iSpaceValue >= 30)
					uTotal++;
			}
			if(iSize > 1)
				iHeadings++;
		}
	}
	int iTotal = (int)uTotal;
	CvUnit const& kCenterUnit = *kPlot.getCenterUnit();
	PlayerTypes eCenterOwner = kCenterUnit.getVisualOwner();
	int iCenterOwner = eCenterOwner;
	if(kCenterUnit.isUnowned())
		iCenterOwner = iRogueIndex;
	/*  First heading, unless rogue or a small number of units of only the active player,
		or a single unit of any player as the center unit. */
	else if((iTotal >= 5 || iHeadings > 1 || eCenterOwner != eActivePlayer) &&
			perOwner[iCenterOwner][ALL].size() > 1) {
		appendUnitOwnerHeading(szString, eCenterOwner, (int)perOwner[eCenterOwner][ARMY].size(),
				(int)perOwner[eCenterOwner][NAVY].size(), (int)perOwner[eCenterOwner][AIR].size(),
				(int)perOwner[eCenterOwner][ALL].size());
	}
	// Lines required for the center unit
	int iLinesCenter = 2; // The first line may well wrap
	int iLinesCenterShort = 1;
	{
		CvWStringBuffer szTempBuffer;
		setUnitHelp(szTempBuffer, &kCenterUnit, false, false, false, true,
				bIndicator); // advc.007
		CvWString szTemp(szTempBuffer.getCString());
		iLinesCenter += (int)std::count(szTemp.begin(), szTemp.end(), L'\n');
		szTempBuffer.clear();
		setUnitHelp(szTempBuffer, &kCenterUnit, false, true, false, true,
				bIndicator); // advc.007
		szTemp = szTempBuffer.getCString();
		iLinesCenterShort += (int)std::count(szTemp.begin(), szTemp.end(), L'\n');
	}
	if(!szString.isEmpty()) // No newline at the start of PlotListHelp
		szString.append(NEWLINE);
	int iLinesUsed = 1;
	bool bOmitOwner = (iTotal > 1);
	if(iTotal + iHeadings + iLinesCenter <= iLineLimit) {
		setUnitHelp(szString, &kCenterUnit, false, false, false, bOmitOwner,
				bIndicator); // advc.007
		iLinesUsed = iLinesCenter;
	}
	else if(iTotal + iHeadings + iLinesCenterShort <= iLineLimit) {
		setUnitHelp(szString, &kCenterUnit, false, true, false, bOmitOwner,
				bIndicator); // advc.007
		iLinesUsed = iLinesCenterShort;
	}
	else {
		setUnitHelp(szString, &kCenterUnit, true, true, false, bOmitOwner,
				bIndicator); // advc.007
	}
	// Show each unit on a separate line (don't aggregate) if there is enough space
	bool bAggregate = false;
	std::set<UnitTypes> perOwnerUnitTypes[iRogueIndex + 1];
	/*  There may not even be enough space for the aggregated info; may have to
		show only a heading with unit counts for some owners. */
	bool abCollapse[iRogueIndex + 1] = { false };
	if(iTotal + iHeadings + iLinesUsed > iLineLimit) {
		bAggregate = true;
		int iTotalAggregated = 0;
		for(int i = 0; i <= iRogueIndex; i++) {
			for(size_t j = 0; j < perOwner[i][ALL].size(); j++) {
				if(perOwner[i][ALL][j] != &kCenterUnit)
					perOwnerUnitTypes[i].insert(perOwner[i][ALL][j]->getUnitType());
			}
			iTotalAggregated += perOwnerUnitTypes[i].size();
		}
		FAssert(iTotalAggregated <= iTotal);
		/*  Select the owner with the smallest number of units (but not the active
			player or rogue) and mark it as collapsed until the (estimated)
			iTotalAggregated is small enough. (implicit selection sort) */
		while(iTotalAggregated + iHeadings + iLinesUsed > iLineLimit +
			/* This may result in some overlap with the sliders, but at least
				the player can then tell that it's actually necessary to omit
				info. But if the resolution is low, the LineLimit may already
				be too generous. */
				(iScreenHeight > 900 ? 3 : 0)) {
			int iCollapseIndex = -1;
			uint iMin = MAX_INT;
			for(int i = 0; i < iRogueIndex; i++) {
				if(abCollapse[i] || // Important for loop termination
						i == eActivePlayer || i == eCenterOwner)
					continue;
				uint iUnits = perOwner[i][ALL].size();
				// Else too little to be gained by collapsing
				if(iUnits >= 4 && perOwnerUnitTypes[i].size() >= 3) {
					if(iUnits < iMin) {
						iMin = iUnits;
						iCollapseIndex = i;
					}
				}
			}
			if(iCollapseIndex < 0) {
				// Space may still be insufficient, but this can't be helped.
				break;
			}
			abCollapse[iCollapseIndex] = true;
			iTotalAggregated += 1 - (int)perOwnerUnitTypes[iCollapseIndex].size();
		}
	}
	/*  Another selection sort by the number of units per owner. Add a heading
		for each owner and list its units. Special treatment for the owner of the
		center unit (heading already added). */
	bool abDone[iRogueIndex + 1] = { false };
	bool bFirstCollapse = true;
	do {
		int iNextIndex = -1;
		if(!abDone[iCenterOwner])
			iNextIndex = iCenterOwner;
		else {
			int iMaxPriority = MIN_INT;
			for(int i = 0; i <= iRogueIndex; i++) {
				if(abDone[i])
					continue;
				int iPriority = (int)perOwner[i][ALL].size();
				if(iPriority <= 0)
					continue;
				if(abCollapse[i]) // Show collapsed entries last
					iPriority -= 10000;
				if(iPriority > iMaxPriority) {
					iMaxPriority = iPriority;
					iNextIndex = i;
				}
			}
		}
		if(iNextIndex < 0)
			break;
		abDone[iNextIndex] = true;
		// If it's just one unit, show the owner on the same line.
		if(perOwner[iNextIndex][ALL].size() == 1) {
			if(iNextIndex != iCenterOwner) { // Center unit already listed
				if(!szString.isEmpty())
					szString.append(NEWLINE);
				setUnitHelp(szString, perOwner[iNextIndex][ALL][0], true, true,
						false, false, bIndicator); // advc.007
			}
			continue;
		}
		if(iNextIndex != iCenterOwner &&
				iNextIndex <= BARBARIAN_PLAYER) { // Can be > when a Spy is in a tile with Barbarians
			appendUnitOwnerHeading(szString, (PlayerTypes)iNextIndex,
					(int)perOwner[iNextIndex][ARMY].size(), (int)perOwner[iNextIndex][NAVY].size(),
					(int)perOwner[iNextIndex][AIR].size(), (int)perOwner[iNextIndex][ALL].size(),
					abCollapse[iNextIndex]);
		}
		if(abCollapse[iNextIndex]) {
			// Still on the same line as the heading
			if(bFirstCollapse) {
				szString.append(NEWLINE);
				szString.append(gDLL->getText("TXT_KEY_MISC_NOT_ENOUGH_SPACE").c_str());
			}
			else szString.append(L"...");
			bFirstCollapse = false;
			continue;
		}
		m_pHelpPlot = &kPlot;
		if(bAggregate) {
			std::vector<UnitTypes> unitTypes(perOwnerUnitTypes[iNextIndex].begin(),
					perOwnerUnitTypes[iNextIndex].end());
			std::sort(unitTypes.begin(), unitTypes.end(), listFirstUnitTypeBeforeSecond);
			for(size_t i = 0; i < unitTypes.size(); i++) {
				appendUnitTypeAggregated(szString, perOwner[iNextIndex][ALL],
					unitTypes[i], kPlot, bIndicator);
			}
		}
		else {
			std::vector<CvUnit const*>& units = perOwner[iNextIndex][ALL];
			std::sort(units.begin(), units.end(), listFirstUnitBeforeSecond);
			for(size_t i = 0; i < units.size(); i++) {
				if(units[i] != &kCenterUnit) {
					szString.append(NEWLINE);
					setUnitHelp(szString, units[i], true, true, false, true,
							bIndicator); // advc.007
				}
			}
		}
	} while(true);
} // </advc.061>


void CvGameTextMgr::setPlotListHelp(CvWStringBuffer &szString, CvPlot* pPlot,
		bool bOneLine, bool bShort,
		bool bIndicator) // advc.061, advc.007
{
	PROFILE_FUNC();
	// <advc.003b>
	if(!pPlot->isVisible(GC.getGame().getActiveTeam(), true))
		return; // </advc.003b>

	if (//(gDLL->getChtLvl() > 0)
			GC.getGame().isDebugMode() // advc.135c
			// advc.007: Don't attach debug info to indicator bubbles
			&& !bIndicator
			&& GC.ctrlKey()) {
		/*  advc.003: Moved into subroutine. (Note: bShort and bOneLine are unused
			in BtS; advc.061 gives bShort a minor use.) */
		setPlotListHelpDebug(szString, *pPlot); // display grouping info
		return;
	}
	// <advc.061>
	if(getBugOptionBOOL("MainInterface__ListUnitsPerOwner", false)) {
		setPlotListHelpPerOwner(szString, *pPlot, bIndicator, bShort);
		return;
	} // </advc.061>

	int const iPromotionInfos = GC.getNumPromotionInfos();
	/*  advc.002b: Was 15; sometimes takes up a bit too much space with increased
		font size. */
	static const uint iMaxNumUnits = 12;
	static std::vector<CvUnit*> apUnits;
	static std::vector<int> aiUnitNumbers;
	static std::vector<int> aiUnitStrength;
	static std::vector<int> aiUnitMaxStrength;
	static std::vector<CvUnit *> plotUnits;

	GC.getGame().getPlotUnits(pPlot, plotUnits);

	int iNumVisibleUnits = 0;
	if (pPlot->isVisible(GC.getGame().getActiveTeam(), true))
	{
		CLLNode<IDInfo>* pUnitNode5 = pPlot->headUnitNode();
		while(pUnitNode5 != NULL)
		{
			CvUnit* pUnit = ::getUnit(pUnitNode5->m_data);
			pUnitNode5 = pPlot->nextUnitNode(pUnitNode5);

			if (pUnit && !pUnit->isInvisible(GC.getGame().getActiveTeam(), true))
			{
				++iNumVisibleUnits;
			}
		}
	}

	apUnits.erase(apUnits.begin(), apUnits.end());

	if (iNumVisibleUnits > iMaxNumUnits)
	{
		aiUnitNumbers.erase(aiUnitNumbers.begin(), aiUnitNumbers.end());
		aiUnitStrength.erase(aiUnitStrength.begin(), aiUnitStrength.end());
		aiUnitMaxStrength.erase(aiUnitMaxStrength.begin(), aiUnitMaxStrength.end());

		if (m_apbPromotion.size() == 0)
		{
			for (int iI = 0; iI < (GC.getNumUnitInfos() * MAX_PLAYERS); ++iI)
			{
				m_apbPromotion.push_back(new int[iPromotionInfos]);
			}
		}

		for (int iI = 0; iI < (GC.getNumUnitInfos() * MAX_PLAYERS); ++iI)
		{
			aiUnitNumbers.push_back(0);
			aiUnitStrength.push_back(0);
			aiUnitMaxStrength.push_back(0);
			for (int iJ = 0; iJ < iPromotionInfos; iJ++)
			{
				m_apbPromotion[iI][iJ] = 0;
			}
		}
	}

	int iCount = 0;
	for (int iI = iMaxNumUnits; iI < iNumVisibleUnits && iI < (int) plotUnits.size(); ++iI)
	{
		CvUnit* pLoopUnit = plotUnits[iI];

		if (pLoopUnit != NULL && pLoopUnit != pPlot->getCenterUnit())
		{
			apUnits.push_back(pLoopUnit);

			if (iNumVisibleUnits > iMaxNumUnits)
			{
				int iIndex = pLoopUnit->getUnitType() * MAX_PLAYERS + pLoopUnit->getOwner();
				if (aiUnitNumbers[iIndex] == 0)
				{
					++iCount;
				}
				++aiUnitNumbers[iIndex];

				int iBase = (DOMAIN_AIR == pLoopUnit->getDomainType() ?
						pLoopUnit->airBaseCombatStr() : pLoopUnit->baseCombatStr());
				if (iBase > 0 && pLoopUnit->maxHitPoints() > 0)
				{
					aiUnitMaxStrength[iIndex] += 100 * iBase;
					aiUnitStrength[iIndex] += (100 * iBase * pLoopUnit->currHitPoints()) / pLoopUnit->maxHitPoints();
				}

				for (int iJ = 0; iJ < iPromotionInfos; iJ++)
				{
					if (pLoopUnit->isHasPromotion((PromotionTypes)iJ))
					{
						++m_apbPromotion[iIndex][iJ];
					}
				}
			}
		}
	}


	if (iNumVisibleUnits > 0)
	{
		if (pPlot->getCenterUnit())
		{
			setUnitHelp(szString, pPlot->getCenterUnit(), iNumVisibleUnits > iMaxNumUnits, true,
					false, false, bIndicator); // advc.007
		}
		uint iNumShown = std::min<uint>(iMaxNumUnits, iNumVisibleUnits);
		for (uint iI = 0; iI < iNumShown && iI < (int) plotUnits.size(); iI++)
		{
			CvUnit* pLoopUnit = plotUnits[iI];
			if (pLoopUnit != pPlot->getCenterUnit())
			{
				szString.append(NEWLINE);
				setUnitHelp(szString, pLoopUnit, true, true,
						false, false, bIndicator); // advc.007
			}
		}
		if(iNumVisibleUnits > iMaxNumUnits)
		{
			bool bFirst = true;
			for (int iI = 0; iI < GC.getNumUnitInfos(); iI++)
			{
				for (int iJ = 0; iJ < MAX_PLAYERS; iJ++)
				{
					int iIndex = iI * MAX_PLAYERS + iJ;
					if (aiUnitNumbers[iIndex] <= 0)
						continue; // advc.003
					//if (iCount < 5 || bFirst) // advc.002b
					{
						szString.append(NEWLINE);
						bFirst = false;
					}
					//else szString.append(L", "); // advc.002b
					szString.append(CvWString::format(SETCOLR L"%s" ENDCOLR,
							TEXT_COLOR("COLOR_UNIT_TEXT"),
							GC.getUnitInfo((UnitTypes)iI).getDescription()));
					// <advc.061> Code moved into a subroutine
					appendAverageStrength(szString, aiUnitMaxStrength[iIndex],
							aiUnitStrength[iIndex], aiUnitNumbers[iIndex]);
					// </advc.061>
					for(int iK = 0; iK < iPromotionInfos; iK++)
					{
						if(m_apbPromotion[iIndex][iK] > 0) {
							szString.append(CvWString::format(L"%d<img=%S size=16 />",
									m_apbPromotion[iIndex][iK],
									GC.getPromotionInfo((PromotionTypes)iK).getButton()));
						}
					}
					if (iJ != GC.getGame().getActivePlayer() &&
							!GC.getUnitInfo((UnitTypes)iI).isAnimal() &&
							!GC.getUnitInfo((UnitTypes)iI).isHiddenNationality())
					{
						szString.append(L", ");
						CvPlayer const& kLoopPlayer = GET_PLAYER((PlayerTypes)iJ);
						szString.append(CvWString::format(SETCOLR L"%s" ENDCOLR,
								PLAYER_TEXT_COLOR(kLoopPlayer), kLoopPlayer.getName()));
					}
				}
			}
		}
	}
}

// advc.003: Cut and pasted from setPlotListHelp, and refactored.
void CvGameTextMgr::setPlotListHelpDebug(CvWStringBuffer& szString, CvPlot const& kPlot) {

	CLLNode<IDInfo>* pUnitNode = kPlot.headUnitNode();
	while(pUnitNode != NULL) {
		CvUnit* pHeadUnit = ::getUnit(pUnitNode->m_data);
		pUnitNode = kPlot.nextUnitNode(pUnitNode);
		// is this unit the head of a group, not cargo, and visible?
		if(pHeadUnit == NULL || !pHeadUnit->isGroupHead() || pHeadUnit->isCargo())
			continue;
		CvGame const& g = GC.getGame();
		TeamTypes eActiveTeam = g.getActiveTeam();
		if(pHeadUnit->isInvisible(eActiveTeam, true))
			continue;
		CvMap const& m = GC.getMap();
		CvWString szTempString;
		int const iPromotions = GC.getNumPromotionInfos();
		// head unit name and unitai
		szString.append(CvWString::format(SETCOLR L"%s" ENDCOLR, 255,190,0,255,
				pHeadUnit->getName().GetCString()));
		szString.append(CvWString::format(L" (%d)", shortenID(pHeadUnit->getID())));
		getUnitAIString(szTempString, pHeadUnit->AI_getUnitAIType());
		CvPlayer const& kHeadOwner = GET_PLAYER(pHeadUnit->getOwner());
		szString.append(CvWString::format(SETCOLR L" %s " ENDCOLR,
				PLAYER_TEXT_COLOR(kHeadOwner), szTempString.GetCString()));

		// promotion icons
		for (int iPromotionIndex = 0; iPromotionIndex < iPromotions; iPromotionIndex++)
		{
			PromotionTypes ePromotion = (PromotionTypes)iPromotionIndex;
			if (pHeadUnit->isHasPromotion(ePromotion))
			{
				szString.append(CvWString::format(L"<img=%S size=16 />",
						GC.getPromotionInfo(ePromotion).getButton()));
			}
		}

		// group
		CvSelectionGroup* pHeadGroup = pHeadUnit->getGroup();
		FAssertMsg(pHeadGroup != NULL, "unit has NULL group");
		if (pHeadGroup->getNumUnits() > 1)
		{
			// BETTER_BTS_AI_MOD, DEBUG, 07/17/09, jdog5000: START
			szString.append(CvWString::format(L"\nGroup:%d [%d units",
					shortenID(pHeadGroup->getID()), pHeadGroup->getNumUnits()));
			if (pHeadGroup->getCargo() > 0)
			{
				szString.append(CvWString::format(L" + %d cargo", pHeadGroup->getCargo()));
			}
			szString.append(CvWString::format(L"]"));

			// get average damage
			int iAverageDamage = 0;
			pUnitNode = pHeadGroup->headUnitNode();
			while (pUnitNode != NULL)
			{
				CvUnit* pLoopUnit = ::getUnit(pUnitNode->m_data);
				pUnitNode = pHeadGroup->nextUnitNode(pUnitNode);
				iAverageDamage += (pLoopUnit->getDamage() * pLoopUnit->maxHitPoints()) / 100;
			}
			iAverageDamage /= pHeadGroup->getNumUnits();
			if (iAverageDamage > 0)
			{
				szString.append(CvWString::format(L" %d%%", 100 - iAverageDamage));
			}
		}

		if (!pHeadGroup->isHuman() && pHeadGroup->isStranded())
		{
			szString.append(CvWString::format(SETCOLR L"\n***STRANDED***" ENDCOLR,
					TEXT_COLOR("COLOR_RED")));
		}

		if (!GC.altKey())
		{
			// mission ai
			MissionAITypes eMissionAI = pHeadGroup->AI_getMissionAIType();
			if (eMissionAI != NO_MISSIONAI)
			{
				getMissionAIString(szTempString, eMissionAI);
				szString.append(CvWString::format(SETCOLR L"\n%s" ENDCOLR,
						TEXT_COLOR("COLOR_HIGHLIGHT_TEXT"), szTempString.GetCString()));
			}

			// mission
			MissionTypes eMissionType = (MissionTypes) pHeadGroup->getMissionType(0);
			if (eMissionType != NO_MISSION)
			{
				getMissionTypeString(szTempString, eMissionType);
				szString.append(CvWString::format(SETCOLR L"\n%s" ENDCOLR,
						TEXT_COLOR("COLOR_HIGHLIGHT_TEXT"), szTempString.GetCString()));
			}

			// mission unit
			CvUnit* pMissionUnit = pHeadGroup->AI_getMissionAIUnit();
			if (pMissionUnit != NULL && (eMissionAI != NO_MISSIONAI || eMissionType != NO_MISSION))
			{
				// mission unit
				szString.append(L"\n to ");
				CvPlayer const& kMissionPlayer = GET_PLAYER(pMissionUnit->getOwner());
				szString.append(CvWString::format(SETCOLR L"%s" ENDCOLR,
						PLAYER_TEXT_COLOR(kMissionPlayer), pMissionUnit->getName().GetCString()));
				szString.append(CvWString::format(L"(%d) G:%d", shortenID(pMissionUnit->getID()),
						shortenID(pMissionUnit->getGroupID())));
				getUnitAIString(szTempString, pMissionUnit->AI_getUnitAIType());
				szString.append(CvWString::format(SETCOLR L" %s" ENDCOLR,
						PLAYER_TEXT_COLOR(kMissionPlayer), szTempString.GetCString()));
			}

			// mission plot
			if (eMissionAI != NO_MISSIONAI || eMissionType != NO_MISSION)
			{
				// first try the plot from the missionAI
				CvPlot* pMissionPlot = pHeadGroup->AI_getMissionAIPlot();

				// if MissionAI does not have a plot, get one from the mission itself
				if (pMissionPlot == NULL && eMissionType != NO_MISSION)
				{
					switch (eMissionType)
					{
					case MISSION_MOVE_TO:
					case MISSION_ROUTE_TO:
						pMissionPlot =  m.plot(pHeadGroup->getMissionData1(0),
								pHeadGroup->getMissionData2(0));
						break;

					case MISSION_MOVE_TO_UNIT:
						if (pMissionUnit != NULL)
						{
							pMissionPlot = pMissionUnit->plot();
						}
						break;
					}
				}

				if (pMissionPlot != NULL)
				{
					szString.append(CvWString::format(L"\n [%d,%d]",
							pMissionPlot->getX(), pMissionPlot->getY()));

					CvCity* pCity = pMissionPlot->getWorkingCity();
					if (pCity != NULL)
					{
						szString.append(L" (");

						if (!pMissionPlot->isCity())
						{
							DirectionTypes eDirection = estimateDirection(
									m.dxWrap(pMissionPlot->getX() - pCity->plot()->getX()),
									m.dyWrap(pMissionPlot->getY() - pCity->plot()->getY()));
							getDirectionTypeString(szTempString, eDirection);
							szString.append(CvWString::format(L"%s of ",
									szTempString.GetCString()));
						}
						CvPlayer const& kCityOwner = GET_PLAYER(pCity->getOwner());
						szString.append(CvWString::format(SETCOLR L"%s" ENDCOLR L")",
								PLAYER_TEXT_COLOR(kCityOwner), pCity->getName().GetCString()));
					}
					else
					{
						if (pMissionPlot != &kPlot)
						{
							DirectionTypes eDirection = estimateDirection(
									m.dxWrap(pMissionPlot->getX() - kPlot.getX()),
									m.dyWrap(pMissionPlot->getY() - kPlot.getY()));
							getDirectionTypeString(szTempString, eDirection);
							szString.append(CvWString::format(L" (%s)", szTempString.GetCString()));
						}

						PlayerTypes eMissionPlotOwner = pMissionPlot->getOwner();
						if (eMissionPlotOwner != NO_PLAYER)
						{
							CvPlayer const& kMissionPlotOwner = GET_PLAYER(eMissionPlotOwner);
							szString.append(CvWString::format(L", " SETCOLR L"%s" ENDCOLR,
									PLAYER_TEXT_COLOR(kMissionPlotOwner), kMissionPlotOwner.getName()));
						}
					}
				}
			}

			// activity
			ActivityTypes eActivityType = (ActivityTypes)pHeadGroup->getActivityType();
			if (eActivityType != NO_ACTIVITY)
			{
				getActivityTypeString(szTempString, eActivityType);
				// <advc.007> Was always a newline
				if(eMissionAI != NO_MISSIONAI || eMissionType != NO_MISSION)
					szString.append(L", ");
				else szString.append(NEWLINE); // </advc.007>
				szString.append(CvWString::format(SETCOLR L"%s" ENDCOLR,
						TEXT_COLOR("COLOR_HIGHLIGHT_TEXT"), szTempString.GetCString()));
			}
		} // BETTER_BTS_AI_MOD: END
		// BETTER_BTS_AI_MOD, DEBUG, 06/10/08, jdog5000: START
		if(!GC.altKey() && !GC.shiftKey())
		{
			// display cargo for head unit
			std::vector<CvUnit*> aCargoUnits;
			pHeadUnit->getCargoUnits(aCargoUnits);
			for (uint i = 0; i < aCargoUnits.size(); ++i)
			{
				CvUnit* pCargoUnit = aCargoUnits[i];
				if (!pCargoUnit->isInvisible(eActiveTeam, true))
				{
					// name and unitai
					szString.append(CvWString::format(SETCOLR L"\n %s" ENDCOLR,
							TEXT_COLOR("COLOR_ALT_HIGHLIGHT_TEXT"),
							pCargoUnit->getName().GetCString()));
					szString.append(CvWString::format(L"(%d)", shortenID(pCargoUnit->getID())));
					getUnitAIString(szTempString, pCargoUnit->AI_getUnitAIType());
					CvPlayer const& kCargoOwner = GET_PLAYER(pCargoUnit->getOwner());
					szString.append(CvWString::format(SETCOLR L" %s " ENDCOLR,
							PLAYER_TEXT_COLOR(kCargoOwner), szTempString.GetCString()));

					// promotion icons
					for (int iPromotionIndex = 0; iPromotionIndex < iPromotions; iPromotionIndex++)
					{
						PromotionTypes ePromotion = (PromotionTypes)iPromotionIndex;
						if (pCargoUnit->isHasPromotion(ePromotion))
						{
							szString.append(CvWString::format(L"<img=%S size=16 />",
									GC.getPromotionInfo(ePromotion).getButton()));
						}
					}
				}
			}

			// display grouped units
			CLLNode<IDInfo>* pUnitNode3 = kPlot.headUnitNode();
			while(pUnitNode3 != NULL)
			{
				CvUnit* pUnit = ::getUnit(pUnitNode3->m_data);
				pUnitNode3 = kPlot.nextUnitNode(pUnitNode3);

				// is this unit not head, in head's group and visible?
				if (pUnit && pUnit != pHeadUnit && pUnit->getGroupID() == pHeadUnit->getGroupID() &&
						!pUnit->isInvisible(eActiveTeam, true))
				{
					FAssertMsg(!pUnit->isCargo(), "unit is cargo but head unit is not cargo");
					// name and unitai
					szString.append(CvWString::format(SETCOLR L"\n-%s" ENDCOLR,
							TEXT_COLOR("COLOR_UNIT_TEXT"), pUnit->getName().GetCString()));
					szString.append(CvWString::format(L" (%d)", shortenID(pUnit->getID())));
					getUnitAIString(szTempString, pUnit->AI_getUnitAIType());
					CvPlayer const& kUnitOwner = GET_PLAYER(pUnit->getOwner());
					szString.append(CvWString::format(SETCOLR L" %s " ENDCOLR,
							PLAYER_TEXT_COLOR(kUnitOwner), szTempString.GetCString()));

					// promotion icons
					for (int iPromotionIndex = 0; iPromotionIndex < iPromotions; iPromotionIndex++)
					{
						PromotionTypes ePromotion = (PromotionTypes)iPromotionIndex;
						if (pUnit->isHasPromotion(ePromotion))
						{
							szString.append(CvWString::format(L"<img=%S size=16 />",
									GC.getPromotionInfo(ePromotion).getButton()));
						}
					}

					// display cargo for loop unit
					std::vector<CvUnit*> aLoopCargoUnits;
					pUnit->getCargoUnits(aLoopCargoUnits);
					for (uint i = 0; i < aLoopCargoUnits.size(); i++)
					{
						CvUnit* pCargoUnit = aLoopCargoUnits[i];
						if (!pCargoUnit->isInvisible(eActiveTeam, true))
						{
							// name and unitai
							szString.append(CvWString::format(SETCOLR L"\n %s" ENDCOLR,
									TEXT_COLOR("COLOR_ALT_HIGHLIGHT_TEXT"),
									pCargoUnit->getName().GetCString()));
							szString.append(CvWString::format(L"(%d)",
									shortenID(pCargoUnit->getID())));
							getUnitAIString(szTempString, pCargoUnit->AI_getUnitAIType());
							CvPlayer const& kCargoOwner = GET_PLAYER(pCargoUnit->getOwner());
							szString.append(CvWString::format(SETCOLR L" %s " ENDCOLR,
									PLAYER_TEXT_COLOR(kCargoOwner), szTempString.GetCString()));

							// promotion icons
							for (int iPromotionIndex = 0; iPromotionIndex < iPromotions; iPromotionIndex++)
							{
								PromotionTypes ePromotion = (PromotionTypes)iPromotionIndex;
								if (pCargoUnit->isHasPromotion(ePromotion))
								{
									szString.append(CvWString::format(L"<img=%S size=16 />",
											GC.getPromotionInfo(ePromotion).getButton()));
								}
							}
						}
					}
				}
			}
		}

		if(!GC.altKey() && (kPlot.getTeam() == NO_TEAM ||
				GET_TEAM(pHeadGroup->getTeam()).isAtWar(kPlot.getTeam())))
		{
			szString.append(NEWLINE);
			CvWString szTempBuffer;
			CvPlayerAI const& kOwner = GET_PLAYER(pHeadGroup->getOwner());

			//AI strategies
			if (kOwner.AI_isDoStrategy(AI_STRATEGY_DAGGER))
			{
				szTempBuffer.Format(L"Dagger, ");
				szString.append(szTempBuffer);
			}
			if (kOwner.AI_isDoStrategy(AI_STRATEGY_CRUSH))
			{
				szTempBuffer.Format(L"Crush, ");
				szString.append(szTempBuffer);
			}
			if (kOwner.AI_isDoStrategy(AI_STRATEGY_ALERT1))
			{
				szTempBuffer.Format(L"Alert1, ");
				szString.append(szTempBuffer);
			}
			if (kOwner.AI_isDoStrategy(AI_STRATEGY_ALERT2))
			{
				szTempBuffer.Format(L"Alert2, ");
				szString.append(szTempBuffer);
			}
			if (kOwner.AI_isDoStrategy(AI_STRATEGY_TURTLE))
			{
				szTempBuffer.Format(L"Turtle, ");
				szString.append(szTempBuffer);
			}
			if (kOwner.AI_isDoStrategy(AI_STRATEGY_LAST_STAND))
			{
				szTempBuffer.Format(L"Last Stand, ");
				szString.append(szTempBuffer);
			}
			if (kOwner.AI_isDoStrategy(AI_STRATEGY_FINAL_WAR))
			{
				szTempBuffer.Format(L"FinalWar, ");
				szString.append(szTempBuffer);
			}
			if (kOwner.AI_isDoStrategy(AI_STRATEGY_GET_BETTER_UNITS))
			{
				szTempBuffer.Format(L"GetBetterUnits, ");
				szString.append(szTempBuffer);
			}
			if (kOwner.AI_isDoStrategy(AI_STRATEGY_FASTMOVERS))
			{
				szTempBuffer.Format(L"FastMovers, ");
				szString.append(szTempBuffer);
			}
			if (kOwner.AI_isDoStrategy(AI_STRATEGY_LAND_BLITZ))
			{
				szTempBuffer.Format(L"LandBlitz, ");
				szString.append(szTempBuffer);
			}
			if (kOwner.AI_isDoStrategy(AI_STRATEGY_AIR_BLITZ))
			{
				szTempBuffer.Format(L"AirBlitz, ");
				szString.append(szTempBuffer);
			}
			if (kOwner.AI_isDoStrategy(AI_STRATEGY_OWABWNW))
			{
				szTempBuffer.Format(L"OWABWNW, ");
				szString.append(szTempBuffer);
			}
			if (kOwner.AI_isDoStrategy(AI_STRATEGY_PRODUCTION))
			{
				szTempBuffer.Format(L"Production, ");
				szString.append(szTempBuffer);
			}
			if (kOwner.AI_isDoStrategy(AI_STRATEGY_MISSIONARY))
			{
				szTempBuffer.Format(L"Missionary, ");
				szString.append(szTempBuffer);
			}
			if (kOwner.AI_isDoStrategy(AI_STRATEGY_BIG_ESPIONAGE))
			{
				szTempBuffer.Format(L"BigEspionage, ");
				szString.append(szTempBuffer);
			}
			if (kOwner.AI_isDoStrategy(AI_STRATEGY_ECONOMY_FOCUS)) // K-Mod
			{
				szTempBuffer.Format(L"EconomyFocus, ");
				szString.append(szTempBuffer);
			}
			if (kOwner.AI_isDoStrategy(AI_STRATEGY_ESPIONAGE_ECONOMY)) // K-Mod
			{
				szTempBuffer.Format(L"EspionageEconomy, ");
				szString.append(szTempBuffer);
			}
			CvArea const& kArea = *kPlot.area();
			//Area battle plans.
			switch(kArea.getAreaAIType(pHeadGroup->getTeam())) {
			case AREAAI_OFFENSIVE:
				szTempBuffer.Format(L"\n Area AI = OFFENSIVE"); break;
			case AREAAI_DEFENSIVE:
				szTempBuffer.Format(L"\n Area AI = DEFENSIVE"); break;
			case AREAAI_MASSING:
				szTempBuffer.Format(L"\n Area AI = MASSING"); break;
			case AREAAI_ASSAULT:
				szTempBuffer.Format(L"\n Area AI = ASSAULT"); break;
			case AREAAI_ASSAULT_MASSING:
				szTempBuffer.Format(L"\n Area AI = ASSAULT_MASSING"); break;
			case AREAAI_NEUTRAL:
				szTempBuffer.Format(L"\n Area AI = NEUTRAL"); break;
			default: szTempBuffer.Format(L"\n Unknown Area AI type"); break;
			}
			CvCity* pTargetCity = kArea.getTargetCity(pHeadGroup->getOwner());
			if(pTargetCity != NULL) {
				szString.append(CvWString::format(L"\nTarget City: %s (%d)",
						pTargetCity->getName().c_str(), pTargetCity->getOwner()));
			}
			else szString.append(CvWString::format(L"\nTarget City: None"));

			if(GC.shiftKey())
			{	// advc.003: unused
				/*int iBestTargetValue = (pTargetCity != NULL ?
						GET_PLAYER(pHeadGroup->getOwner()).
						AI_targetCityValue(pTargetCity,false,true) : 0);*/
				szString.append(CvWString::format(L"\n\nTarget City values:\n"));
				for(int iPlayer = 0; iPlayer < MAX_PLAYERS; iPlayer++)
				{
					CvPlayer const& pl = GET_PLAYER((PlayerTypes)iPlayer);
					if(GET_TEAM(pHeadGroup->getTeam()).AI_getWarPlan(pl.getTeam()) == NO_WARPLAN)
						continue;
					if(kArea.getCitiesPerPlayer((PlayerTypes)iPlayer) <= 0)
						continue;
					int iLoop = -1;
					for(CvCity* pLoopCity = pl.firstCity(&iLoop); pLoopCity != NULL;
							pLoopCity = pl.nextCity(&iLoop))
					{
						if(pLoopCity->area() == &kArea)
						{
							int iTargetValue = GET_PLAYER(pHeadGroup->getOwner()).
									AI_targetCityValue(pLoopCity,false,true);
							if((m.calculatePathDistance(&kPlot, pLoopCity->plot()) < 20)) {
								szString.append(CvWString::format(L"\n%s : %d + rand %d",
										pLoopCity->getName().c_str(), iTargetValue,
										pLoopCity->getPopulation() / 2));
							}
						}
					}
				}
			}
		} // BETTER_BTS_AI_MOD: END
		if(pUnitNode != NULL) { // advc.007: No newlines in the final iteration
			// double space non-empty groups
			if (pHeadGroup->getNumUnits() > 1 || pHeadUnit->hasCargo())
				szString.append(NEWLINE);
			szString.append(NEWLINE);
		}
	}
}


// ADVANCED COMBAT ODDS v1.1, 11/7/09, PieceOfMind: START
//Calculates the probability of a particular combat outcome
//Returns a float value (between 0 and 1)
//Written by PieceOfMind
//n_A = hits taken by attacker, n_D = hits taken by defender.
static float getCombatOddsSpecific(CvUnit* pAttacker, CvUnit* pDefender, int n_A, int n_D)
{
	int iAttackerStrength;
	int iAttackerFirepower;
	int iDefenderStrength;
	int iDefenderFirepower;
	int iDefenderOdds;
	int iAttackerOdds;
	int iStrengthFactor;
	int iDamageToAttacker;
	int iDamageToDefender;
	int iNeededRoundsAttacker;
	//int iNeededRoundsDefender;

	int AttFSnet;
	int AttFSC;
	int DefFSC;

	int iDefenderHitLimit;

	iAttackerStrength = pAttacker->currCombatStr(NULL, NULL);
	iAttackerFirepower = pAttacker->currFirepower(NULL, NULL);
	iDefenderStrength = pDefender->currCombatStr(pDefender->plot(), pAttacker);
	iDefenderFirepower = pDefender->currFirepower(pDefender->plot(), pAttacker);

	iStrengthFactor = ((iAttackerFirepower + iDefenderFirepower + 1) / 2);
	iDamageToAttacker = std::max(1,((GC.getDefineINT("COMBAT_DAMAGE") * (iDefenderFirepower + iStrengthFactor)) / (iAttackerFirepower + iStrengthFactor)));
	iDamageToDefender = std::max(1,((GC.getDefineINT("COMBAT_DAMAGE") * (iAttackerFirepower + iStrengthFactor)) / (iDefenderFirepower + iStrengthFactor)));

	iDefenderOdds = ((GC.getDefineINT("COMBAT_DIE_SIDES") * iDefenderStrength) / (iAttackerStrength + iDefenderStrength));
	iAttackerOdds = GC.getDefineINT("COMBAT_DIE_SIDES") - iDefenderOdds;
	/*  advc.001: Replacing the check below. The BUG authors must've missed this
		one when they integrated ACO into BUG. */
	if(!getBugOptionBOOL("ACO__IgnoreBarbFreeWins", false)
	//GC.getDefineINT("ACO_IgnoreBarbFreeWins")==0
			&& !GC.getGame().isOption(GAMEOPTION_SPAH)) // advc.250b
	{
		if (pDefender->isBarbarian())
		{
			//defender is barbarian
			if (!GET_PLAYER(pAttacker->getOwner()).isBarbarian() && GET_PLAYER(pAttacker->getOwner()).getWinsVsBarbs() < GC.getHandicapInfo(GET_PLAYER(pAttacker->getOwner()).getHandicapType()).getFreeWinsVsBarbs())
			{
				//attacker is not barb and attacker player has free wins left
				//I have assumed in the following code only one of the units (attacker and defender) can be a barbarian

				iDefenderOdds = std::min((10 * GC.getDefineINT("COMBAT_DIE_SIDES")) / 100, iDefenderOdds);
				iAttackerOdds = std::max((90 * GC.getDefineINT("COMBAT_DIE_SIDES")) / 100, iAttackerOdds);
			}
		}
		else if (pAttacker->isBarbarian())
		{
			//attacker is barbarian
			if (!GET_PLAYER(pDefender->getOwner()).isBarbarian() && GET_PLAYER(pDefender->getOwner()).getWinsVsBarbs() < GC.getHandicapInfo(GET_PLAYER(pDefender->getOwner()).getHandicapType()).getFreeWinsVsBarbs())
			{
				//defender is not barbarian and defender has free wins left and attacker is barbarian
				iAttackerOdds = std::min((10 * GC.getDefineINT("COMBAT_DIE_SIDES")) / 100, iAttackerOdds);
				iDefenderOdds = std::max((90 * GC.getDefineINT("COMBAT_DIE_SIDES")) / 100, iDefenderOdds);
			}
		}
	}

	iDefenderHitLimit = pDefender->maxHitPoints() - pAttacker->combatLimit();

	//iNeededRoundsAttacker = (std::max(0, pDefender->currHitPoints() - iDefenderHitLimit) + iDamageToDefender - (((pAttacker->combatLimit())==GC.getMAX_HIT_POINTS())?1:0)) / iDamageToDefender;
	iNeededRoundsAttacker = (pDefender->currHitPoints() - pDefender->maxHitPoints() + pAttacker->combatLimit() - (((pAttacker->combatLimit())==pDefender->maxHitPoints())?1:0))/iDamageToDefender + 1;

	int N_D = (std::max(0, pDefender->currHitPoints() - iDefenderHitLimit) + iDamageToDefender - (((pAttacker->combatLimit())==GC.getMAX_HIT_POINTS())?1:0)) / iDamageToDefender;

	//int N_A = (pAttacker->currHitPoints() + iDamageToAttacker - 1) / iDamageToAttacker;  //same as next line
	int N_A = (pAttacker->currHitPoints() - 1)/iDamageToAttacker + 1;

	//int iRetreatOdds = std::max((pAttacker->withdrawalProbability()),100);
	float RetreatOdds = ((float)(std::min((pAttacker->withdrawalProbability()),100)))/100.0f ;

	AttFSnet = ((pDefender->immuneToFirstStrikes()) ? 0 : pAttacker->firstStrikes()) - ((pAttacker->immuneToFirstStrikes()) ? 0 : pDefender->firstStrikes());
	AttFSC = (pDefender->immuneToFirstStrikes()) ? 0 : (pAttacker->chanceFirstStrikes());
	DefFSC = (pAttacker->immuneToFirstStrikes()) ? 0 : (pDefender->chanceFirstStrikes());

	float P_A = (float)iAttackerOdds / GC.getDefineINT("COMBAT_DIE_SIDES");
	float P_D = (float)iDefenderOdds / GC.getDefineINT("COMBAT_DIE_SIDES");
	float answer = 0.0f;
	if (n_A < N_A && n_D == iNeededRoundsAttacker)   // (1) Defender dies or is taken to combat limit
	{
		float sum1 = 0.0f;
		for (int i = (-AttFSnet-AttFSC<1?1:-AttFSnet-AttFSC); i <= DefFSC - AttFSnet; i++)
		{
			for (int j = 0; j <= i; j++)
			{
				if (n_A >= j)
				{
					sum1 += (float)getBinomialCoefficient(i,j) * pow(P_A,(float)(i-j)) * getBinomialCoefficient(iNeededRoundsAttacker-1+n_A-j,iNeededRoundsAttacker-1);

				} //if
			}//for j
		}//for i
		sum1 *= pow(P_D,(float)n_A)*pow(P_A,(float)iNeededRoundsAttacker);
		answer += sum1;

		float sum2 = 0.0f;

		for (int i = (0<AttFSnet-DefFSC?AttFSnet-DefFSC:0); i <= AttFSnet + AttFSC; i++)
		{

			for (int j = 0; j <= i; j++)
			{
				if (N_D > j)
				{
					sum2 = sum2 + getBinomialCoefficient(n_A+iNeededRoundsAttacker-j-1,n_A) * (float)getBinomialCoefficient(i,j) * pow(P_A,(float)iNeededRoundsAttacker) * pow(P_D,(float)(n_A+i-j));

				}
				else if (n_A == 0)
				{
					sum2 = sum2 + (float)getBinomialCoefficient(i,j) * pow(P_A,(float)j) * pow(P_D,(float)(i-j));
				}
				else
				{
					sum2 = sum2 + 0.0f;
				}
			}//for j

		}//for i
		answer += sum2;
	}
	else if (n_D < N_D && n_A == N_A)  // (2) Attacker dies!
	{

		float sum1 = 0.0f;
		for (int i = (-AttFSnet-AttFSC<1?1:-AttFSnet-AttFSC); i <= DefFSC - AttFSnet; i++)
		{

			for (int j = 0; j <= i; j++)
			{
				if (N_A>j)
				{
					sum1 += getBinomialCoefficient(n_D+N_A-j-1,n_D) * (float)getBinomialCoefficient(i,j) * pow(P_D,(float)(N_A)) * pow(P_A,(float)(n_D+i-j));
				}
				else
				{
					if (n_D == 0)
					{
						sum1 += (float)getBinomialCoefficient(i,j) * pow(P_D,(float)(j)) * pow(P_A,(float)(i-j));
					}//if (inside if) else sum += 0
				}//if
			}//for j
		}//for i
		answer += sum1;
		float sum2 = 0.0f;
		for (int i = (0<AttFSnet-DefFSC?AttFSnet-DefFSC:0); i <= AttFSnet + AttFSC; i++)
		{
			for (int j = 0; j <= i; j++)
			{
				if (n_D >= j)
				{
					sum2 += (float)getBinomialCoefficient(i,j) * pow(P_D,(float)(i-j)) * getBinomialCoefficient(N_A-1+n_D-j,N_A-1);
				} //if
			}//for j
		}//for i
		sum2 *= pow(P_A,(float)(n_D))*pow(P_D,(float)(N_A));
		answer += sum2;
		answer = answer * (1.0f - RetreatOdds);
	}
	else if (n_A == (N_A-1) && n_D < N_D)  // (3) Attacker retreats!
	{
		float sum1 = 0.0f;
		for (int i = (AttFSnet+AttFSC>-1?1:-AttFSnet-AttFSC); i <= DefFSC - AttFSnet; i++)
		{

			for (int j = 0; j <= i; j++)
			{
				if (N_A>j)
				{
					sum1 += getBinomialCoefficient(n_D+N_A-j-1,n_D) * (float)getBinomialCoefficient(i,j) * pow(P_D,(float)(N_A)) * pow(P_A,(float)(n_D+i-j));
				}
				else
				{
					if (n_D == 0)
					{
						sum1 += (float)getBinomialCoefficient(i,j) * pow(P_D,(float)(j)) * pow(P_A,(float)(i-j));
					}//if (inside if) else sum += 0
				}//if
			}//for j
		}//for i
		answer += sum1;

		float sum2 = 0.0f;
		for (int i = (0<AttFSnet-DefFSC?AttFSnet-DefFSC:0); i <= AttFSnet + AttFSC; i++)
		{
			for (int j = 0; j <= i; j++)
			{
				if (n_D >= j)
				{
					sum2 += (float)getBinomialCoefficient(i,j) * pow(P_D,(float)(i-j)) * getBinomialCoefficient(N_A-1+n_D-j,N_A-1);
				} //if
			}//for j
		}//for i
		sum2 *= pow(P_A,(float)(n_D))*pow(P_D,(float)(N_A));
		answer += sum2;
		answer = answer * RetreatOdds;//
	}
	else
	{
		//Unexpected value.  Process should not reach here.
		FAssertMsg(false, "unexpected value in getCombatOddsSpecific");
	}

	answer = answer / ((float)(AttFSC+DefFSC+1)); // dividing by (t+w+1) as is necessary
	return answer;
}// getCombatOddsSpecific
// ADVANCED COMBAT ODDS, 11/7/09, PieceOfMind: END

// Returns true if help was given...
// K-Mod note: this function can change the center unit on the plot. (because of a change I made)
// Also, I've made some unmarked structural changes to this function, to make it easier to read and to fix a few minor bugs.
bool CvGameTextMgr::setCombatPlotHelp(CvWStringBuffer &szString, CvPlot* pPlot)
{
	PROFILE_FUNC();
	// ADVANCED COMBAT ODDS v2.0, 3/11/09, PieceOfMind: START
	/*  Note that due to the large amount of extra content added to this function (setCombatPlotHelp), this should never be used in any function that needs to be called repeatedly (e.g. hundreds of times) quickly.
		It is fine for a human player mouse-over (which is what it is used for). */
	/* New Code */
	bool ACO_enabled = getBugOptionBOOL("ACO__Enabled", false);
	bool bShift = GC.shiftKey();
	int iView = bShift ? 2 : 1;
	if (getBugOptionBOOL("ACO__SwapViews", false))
	{
		iView = 3 - iView; //swaps 1 and 2.
	}
	CvWString szTempBuffer2;
	// ADVANCED COMBAT ODDS v2.0, 3/11/09, PieceOfMind: END

	CvWString szTempBuffer;
	CvWString szOffenseOdds;
	CvWString szDefenseOdds;
	int iModifier;
	// advc.048:
	int iLengthSelectionList = gDLL->getInterfaceIFace()->getLengthSelectionList();
	if (iLengthSelectionList == 0)
		return false;
	// advc.048:
	CvSelectionGroupAI const& kSelectionList = *static_cast<CvSelectionGroupAI*>(gDLL->getInterfaceIFace()->getSelectionList());
	bool bValid = false;
	switch (kSelectionList.getDomainType())
	{
	case DOMAIN_SEA:
		bValid = pPlot->isWater();
		break;

	case DOMAIN_AIR:
		bValid = true;
		break;

	case DOMAIN_LAND:
		bValid = !(pPlot->isWater());
		break;

	case DOMAIN_IMMOBILE:
		break;

	default:
		FAssert(false);
		break;
	}
	if (!bValid)
		return false;

	bool bMaxSurvival = GC.altKey(); // advc.048
	int iOdds;
	CvUnit* pAttacker = kSelectionList.AI_getBestGroupAttacker(pPlot, false, iOdds,
			false, false, !bMaxSurvival, bMaxSurvival); // advc.048
	if (pAttacker == NULL) {
		pAttacker = kSelectionList.AI_getBestGroupAttacker(pPlot, false, iOdds,
				true, // bypass checks for moves and war etc.
				false, !bMaxSurvival, bMaxSurvival); // advc.048
	}
	if (pAttacker == NULL)
		return false;

	CvUnit* pDefender = pPlot->getBestDefender(NO_PLAYER, pAttacker->getOwner(), pAttacker, !GC.altKey());
	if (pDefender == NULL || !pDefender->canDefend(pPlot) || !pAttacker->canAttack(*pDefender))
		return false;

	// <advc.048>
	bool bBestOddsHelp = false;
	if(!bMaxSurvival && GC.getDefineINT("GROUP_ATTACK_BEST_ODDS_HELP") > 0) {
		CvUnit* pBestOddsAttacker = kSelectionList.AI_getBestGroupAttacker(pPlot, false, iOdds,
				false, false, false, true);
		if(pBestOddsAttacker == NULL) {
			pBestOddsAttacker = kSelectionList.AI_getBestGroupAttacker(pPlot, false, iOdds,
					true, false, false, true);
		}
		if(pBestOddsAttacker != pAttacker)
			bBestOddsHelp = true;
	}
	if(!ACO_enabled && bBestOddsHelp) {
		szString.append(gDLL->getText("TXT_KEY_GROUP_ATTACK_BEST_ODDS_HELP"));
		szString.append(NEWLINE);
	} // </advc.048>

	// K-Mod. If the plot's center unit isn't one of our own units, then use this defender as the plot's center unit.
	// With this, the map will accurately show who we're up against.
	if (gDLL->getInterfaceIFace()->getSelectionPlot() != pPlot)
	{
		if (pDefender->getOwner() == GC.getGame().getActivePlayer() ||
			!pPlot->getCenterUnit() || // I don't think this is possible... but it's pretty cheap to check.
			pPlot->getCenterUnit()->getOwner() != GC.getGame().getActivePlayer())
		{
			pPlot->setCenterUnit(pDefender);
		}

	} // K-Mod end

	if (pAttacker->getDomainType() != DOMAIN_AIR)
	{
		int iCombatOdds = getCombatOdds(pAttacker, pDefender);
		bool bVictoryOddsAppended = false; // advc.048
		if (pAttacker->combatLimit() >= GC.getMAX_HIT_POINTS())
		{
			if (!ACO_enabled || getBugOptionBOOL("ACO__ForceOriginalOdds", false))
			{
				if (iCombatOdds > 999)
				{
					szTempBuffer = L"&gt; 99.9";
				}
				else if (iCombatOdds < 1)
				{
					szTempBuffer = L"&lt; 0.1";
				}
				else
				{
					szTempBuffer.Format(L"%.1f", ((float)iCombatOdds) / 10.0f);
				}

				szString.append(gDLL->getText("TXT_KEY_COMBAT_PLOT_ODDS", szTempBuffer.GetCString()));
				bVictoryOddsAppended = true; // advc.048
				if (ACO_enabled)
				{
					szString.append(NEWLINE);
				}
			}
		}


		int iWithdrawal = 0;

		if (pAttacker->combatLimit() < GC.getMAX_HIT_POINTS())
		{
			iWithdrawal += 100 * iCombatOdds;
		}

		iWithdrawal += std::min(100, pAttacker->withdrawalProbability()) * (1000 - iCombatOdds);

		if (iWithdrawal > 0 || pAttacker->combatLimit() < GC.getMAX_HIT_POINTS())
		{
			if (!ACO_enabled || getBugOptionBOOL("ACO__ForceOriginalOdds", false))
			{
				if (iWithdrawal > 99900)
				{
					szTempBuffer = L"&gt; 99.9";
				}
				else if (iWithdrawal < 100)
				{
					szTempBuffer = L"&lt; 0.1";
				}
				else
				{
					szTempBuffer.Format(L"%.1f", iWithdrawal / 1000.0f);
				}
				if(bVictoryOddsAppended) // advc.048
					szString.append(NEWLINE);
				szString.append(gDLL->getText("TXT_KEY_COMBAT_PLOT_ODDS_RETREAT", szTempBuffer.GetCString()));
				if (ACO_enabled)
				{
					szString.append(NEWLINE);
				}
			}
		}

		//szTempBuffer.Format(L"AI odds: %d%%", iOdds);
		//szString += NEWLINE + szTempBuffer;

		// ADVANCED COMBAT ODDS v2.0, 3/11/09, PieceOfMind: START
		/* New Code */
		if (ACO_enabled)
		{
			// <advc.312>
			int iMaxXPAtt = GC.getDefineINT("MAX_EXPERIENCE_PER_COMBAT");
			int iMaxXPDef = iMaxXPAtt;
			if(pAttacker->isBarbarian())
				iMaxXPDef -= 4;
			if(pDefender->isBarbarian())
				iMaxXPAtt -= 4;
			// </advc.312>
			BOOL ACO_debug = false;
			//Change this to true when you need to spot errors, particular in the expected hit points calculations
			ACO_debug = getBugOptionBOOL("ACO__Debug", false);

			/** phungus start **/
			/*bool bctrl; bctrl = GC.ctrlKey();
			if (bctrl)
			{//SWITCHAROO IS DISABLED IN V1.0.  Hopefully it will be available in the next version. At the moment is has issues when modifiers are present.
				CvUnit* swap = pAttacker;
				pAttacker = pDefender;
				pDefender = swap;
				CvPlot* pAttackerPlot = pAttacker->plot();
				CvPlot* pDefenderPlot = pDefender->plot();
			}*/
			int iAttackerExperienceModifier = 0;
			int iDefenderExperienceModifier = 0;
			for (int ePromotion = 0; ePromotion < GC.getNumPromotionInfos(); ++ePromotion)
			{
				if (pAttacker->isHasPromotion((PromotionTypes)ePromotion) && GC.getPromotionInfo((PromotionTypes)ePromotion).getExperiencePercent() != 0)
				{
					iAttackerExperienceModifier += GC.getPromotionInfo((PromotionTypes)ePromotion).getExperiencePercent();
				}
			}

			for (int ePromotion = 0; ePromotion < GC.getNumPromotionInfos(); ++ePromotion)
			{
				if (pDefender->isHasPromotion((PromotionTypes)ePromotion) && GC.getPromotionInfo((PromotionTypes)ePromotion).getExperiencePercent() != 0)
				{
					iDefenderExperienceModifier += GC.getPromotionInfo((PromotionTypes)ePromotion).getExperiencePercent();
				}
			}
			/** phungus end **/ //thanks to phungus420

			/** Many thanks to DanF5771 for some of these calculations! **/
			int iAttackerStrength  = pAttacker->currCombatStr(NULL, NULL);
			int iAttackerFirepower = pAttacker->currFirepower(NULL, NULL);
			int iDefenderStrength  = pDefender->currCombatStr(pPlot, pAttacker);
			int iDefenderFirepower = pDefender->currFirepower(pPlot, pAttacker);

			FAssert((iAttackerStrength + iDefenderStrength)*(iAttackerFirepower + iDefenderFirepower) > 0);

			int iStrengthFactor    = ((iAttackerFirepower + iDefenderFirepower + 1) / 2);
			int iDamageToAttacker  = std::max(1,((GC.getDefineINT("COMBAT_DAMAGE") * (iDefenderFirepower + iStrengthFactor)) / (iAttackerFirepower + iStrengthFactor)));
			int iDamageToDefender  = std::max(1,((GC.getDefineINT("COMBAT_DAMAGE") * (iAttackerFirepower + iStrengthFactor)) / (iDefenderFirepower + iStrengthFactor)));
			int iFlankAmount       = iDamageToAttacker;

			int iDefenderOdds = ((GC.getDefineINT("COMBAT_DIE_SIDES") * iDefenderStrength) / (iAttackerStrength + iDefenderStrength));
			int iAttackerOdds = GC.getDefineINT("COMBAT_DIE_SIDES") - iDefenderOdds;

			// Barbarian related code.
			/*  advc.001: The section below deals with FreeWins, so it should
				only be executed if !IgnoreBarbFreeWins. The condition was
				checking the opposite. */
			if (!getBugOptionBOOL("ACO__IgnoreBarbFreeWins", false)
					&& !GC.getGame().isOption(GAMEOPTION_SPAH)) // advc.250b
				//Are we not going to ignore barb free wins?  If not, skip this section...
			{
				if (pDefender->isBarbarian())
				{
					//defender is barbarian
					if (!GET_PLAYER(pAttacker->getOwner()).isBarbarian() && GET_PLAYER(pAttacker->getOwner()).getWinsVsBarbs() < GC.getHandicapInfo(GET_PLAYER(pAttacker->getOwner()).getHandicapType()).getFreeWinsVsBarbs())
					{
						//attacker is not barb and attacker player has free wins left
						//I have assumed in the following code only one of the units (attacker and defender) can be a barbarian
						iDefenderOdds = std::min((10 * GC.getDefineINT("COMBAT_DIE_SIDES")) / 100, iDefenderOdds);
						iAttackerOdds = std::max((90 * GC.getDefineINT("COMBAT_DIE_SIDES")) / 100, iAttackerOdds);
						szTempBuffer.Format(SETCOLR L"%d\n" ENDCOLR,
							TEXT_COLOR("COLOR_HIGHLIGHT_TEXT"),GC.getHandicapInfo(GET_PLAYER(pAttacker->getOwner()).getHandicapType()).getFreeWinsVsBarbs()-GET_PLAYER(pAttacker->getOwner()).getWinsVsBarbs());
						szString.append(gDLL->getText("TXT_ACO_BarbFreeWinsLeft"));
						szString.append(szTempBuffer.GetCString());
					}
				}
				else
				{
					//defender is not barbarian
					if (pAttacker->isBarbarian())
					{
						//attacker is barbarian
						if (!GET_PLAYER(pDefender->getOwner()).isBarbarian() && GET_PLAYER(pDefender->getOwner()).getWinsVsBarbs() < GC.getHandicapInfo(GET_PLAYER(pDefender->getOwner()).getHandicapType()).getFreeWinsVsBarbs())
						{
							//defender is not barbarian and defender has free wins left and attacker is barbarian
							iAttackerOdds = std::min((10 * GC.getDefineINT("COMBAT_DIE_SIDES")) / 100, iAttackerOdds);
							iDefenderOdds = std::max((90 * GC.getDefineINT("COMBAT_DIE_SIDES")) / 100, iDefenderOdds);
							szTempBuffer.Format(SETCOLR L"%d\n" ENDCOLR,
								TEXT_COLOR("COLOR_HIGHLIGHT_TEXT"),GC.getHandicapInfo(GET_PLAYER(pDefender->getOwner()).getHandicapType()).getFreeWinsVsBarbs()-GET_PLAYER(pDefender->getOwner()).getWinsVsBarbs());
							szString.append(gDLL->getText("TXT_ACO_BarbFreeWinsLeft"));
							szString.append(szTempBuffer.GetCString());
						}
					}
				}
			}


			//XP calculations
			int iExperience;
			int iWithdrawXP;//thanks to phungus420
			iWithdrawXP = GC.getDefineINT("EXPERIENCE_FROM_WITHDRAWL");//thanks to phungus420

			if (pAttacker->combatLimit() < 100)
			{
				iExperience        = GC.getDefineINT("EXPERIENCE_FROM_WITHDRAWL");
			}
			else
			{
				iExperience        = (pDefender->attackXPValue() * iDefenderStrength) / iAttackerStrength;
				iExperience        = range(iExperience, GC.getDefineINT("MIN_EXPERIENCE_PER_COMBAT"),
						iMaxXPAtt); // advc.312
			}

			int iDefExperienceKill;
			iDefExperienceKill = (pAttacker->defenseXPValue() * iAttackerStrength) / iDefenderStrength;
			iDefExperienceKill = range(iDefExperienceKill, GC.getDefineINT("MIN_EXPERIENCE_PER_COMBAT"),
					iMaxXPDef); // advc.312

			int iBonusAttackerXP = (iExperience * iAttackerExperienceModifier) / 100;
			int iBonusDefenderXP = (iDefExperienceKill * iDefenderExperienceModifier) / 100;
			int iBonusWithdrawXP = (GC.getDefineINT("EXPERIENCE_FROM_WITHDRAWL") * iAttackerExperienceModifier) / 100;


			//The following code adjusts the XP for barbarian encounters.  In standard game, barb and animal xp cap is 10,5 respectively.
			/**Thanks to phungus420 for the following block of code! **/
			if(pDefender->isBarbarian())
			{
				if (pDefender->isAnimal())
				{
					//animal
					iExperience = range(iExperience,0,GC.getDefineINT("ANIMAL_MAX_XP_VALUE")-(pAttacker->getExperience()));
					if (iExperience < 0)
					{
						iExperience = 0;
					}
					iWithdrawXP = range(iWithdrawXP,0,GC.getDefineINT("ANIMAL_MAX_XP_VALUE")-(pAttacker->getExperience()));
					if (iWithdrawXP < 0)
					{
						iWithdrawXP = 0;
					}
					iBonusAttackerXP = range(iBonusAttackerXP,0,GC.getDefineINT("ANIMAL_MAX_XP_VALUE")-(pAttacker->getExperience() + iExperience));
					if (iBonusAttackerXP < 0)
					{
						iBonusAttackerXP = 0;
					}
					iBonusWithdrawXP = range(iBonusWithdrawXP,0,GC.getDefineINT("ANIMAL_MAX_XP_VALUE")-(pAttacker->getExperience() + iWithdrawXP));
					if (iBonusWithdrawXP < 0)
					{
						iBonusWithdrawXP = 0;
					}
				}
				else
				{
					//normal barbarian
					iExperience = range(iExperience,0,GC.getDefineINT("BARBARIAN_MAX_XP_VALUE")-pAttacker->getExperience());
					if (iExperience < 0)
					{
						iExperience = 0;
					}
					iWithdrawXP = range(iWithdrawXP,0,GC.getDefineINT("BARBARIAN_MAX_XP_VALUE")-(pAttacker->getExperience()));
					if (iWithdrawXP < 0)
					{
						iWithdrawXP = 0;
					}
					iBonusAttackerXP = range(iBonusAttackerXP,0,GC.getDefineINT("BARBARIAN_MAX_XP_VALUE")-(pAttacker->getExperience() + iExperience));
					if (iBonusAttackerXP < 0)
					{
						iBonusAttackerXP = 0;
					}
					iBonusWithdrawXP = range(iBonusWithdrawXP,0,GC.getDefineINT("BARBARIAN_MAX_XP_VALUE")-(pAttacker->getExperience() + iWithdrawXP));
					if (iBonusWithdrawXP < 0)
					{
						iBonusWithdrawXP = 0;
					}
				}
			}

			int iNeededRoundsAttacker = (pDefender->currHitPoints() - pDefender->maxHitPoints() + pAttacker->combatLimit() - (((pAttacker->combatLimit())==pDefender->maxHitPoints())?1:0))/iDamageToDefender + 1;
			//The extra term introduced here was to account for the incorrect way it treated units that had combatLimits.
			//A catapult that deals 25HP per round, and has a combatLimit of 75HP must deal four successful hits before it kills the warrior -not 3.  This is proved in the way CvUnit::resolvecombat works
			// The old formula (with just a plain -1 instead of a conditional -1 or 0) was incorrectly saying three.

			// int iNeededRoundsDefender = (pAttacker->currHitPoints() + iDamageToAttacker - 1) / iDamageToAttacker;  //this is idential to the following line
			int iNeededRoundsDefender = (pAttacker->currHitPoints() - 1)/iDamageToAttacker + 1;

			//szTempBuffer.Format(L"iNeededRoundsAttacker = %d\niNeededRoundsDefender = %d",iNeededRoundsAttacker,iNeededRoundsDefender);
			//szString.append(NEWLINE);szString.append(szTempBuffer.GetCString());
			//szTempBuffer.Format(L"pDefender->currHitPoints = %d\n-pDefender->maxHitPOints = %d\n + pAttacker->combatLimit = %d\n - 1 if\npAttackercomBatlimit equals pDefender->maxHitpoints\n=(%d == %d)\nall over iDamageToDefender = %d\n+1 = ...",
			//pDefender->currHitPoints(),pDefender->maxHitPoints(),pAttacker->combatLimit(),pAttacker->combatLimit(),pDefender->maxHitPoints(),iDamageToDefender);
			//szString.append(NEWLINE);szString.append(szTempBuffer.GetCString());

			int iDefenderHitLimit = pDefender->maxHitPoints() - pAttacker->combatLimit();

			//NOW WE CALCULATE SOME INTERESTING STUFF :)

			float E_HP_Att = 0.0f;//expected damage dealt to attacker
			float E_HP_Def = 0.0f;
			float E_HP_Att_Withdraw; //Expected hitpoints for attacker if attacker withdraws (not the same as retreat)
			float E_HP_Att_Victory; //Expected hitpoints for attacker if attacker kills defender
			int E_HP_Att_Retreat = (pAttacker->currHitPoints()) - (iNeededRoundsDefender-1)*iDamageToAttacker;//this one is predetermined easily
			float E_HP_Def_Withdraw;
			float E_HP_Def_Defeat; // if attacker dies
			//Note E_HP_Def is the same for if the attacker withdraws or dies

			float AttackerUnharmed;
			float DefenderUnharmed;

			AttackerUnharmed = getCombatOddsSpecific(pAttacker,pDefender,0,iNeededRoundsAttacker);
			DefenderUnharmed = getCombatOddsSpecific(pAttacker,pDefender,iNeededRoundsDefender,0);
			DefenderUnharmed += getCombatOddsSpecific(pAttacker,pDefender,iNeededRoundsDefender-1,0);//attacker withdraws or retreats

			float prob_bottom_Att_HP; // The probability the attacker exits combat with min HP
			float prob_bottom_Def_HP; // The probability the defender exits combat with min HP

			if (ACO_debug)
			{
				szTempBuffer.Format(L"E[HP ATTACKER]");
				//szString.append(NEWLINE);
				szString.append(szTempBuffer.GetCString());
			}
			// already covers both possibility of defender not being killed AND being killed
			for (int n_A = 0; n_A < iNeededRoundsDefender; n_A++)
			{
				//prob_attack[n_A] = getCombatOddsSpecific(pAttacker,pDefender,n_A,iNeededRoundsAttacker);
				E_HP_Att += ( (pAttacker->currHitPoints()) - n_A*iDamageToAttacker) * getCombatOddsSpecific(pAttacker,pDefender,n_A,iNeededRoundsAttacker);

				if (ACO_debug)
				{
					szTempBuffer.Format(L"+%d * %.2f%%  (Def %d) (%d:%d)",
						((pAttacker->currHitPoints()) - n_A*iDamageToAttacker),100.0f*getCombatOddsSpecific(pAttacker,pDefender,n_A,iNeededRoundsAttacker),iDefenderHitLimit,n_A,iNeededRoundsAttacker);
					szString.append(NEWLINE);
					szString.append(szTempBuffer.GetCString());
				}
			}
			E_HP_Att_Victory = E_HP_Att;//NOT YET NORMALISED
			E_HP_Att_Withdraw = E_HP_Att;//NOT YET NORMALIZED
			prob_bottom_Att_HP = getCombatOddsSpecific(pAttacker,pDefender,iNeededRoundsDefender-1,iNeededRoundsAttacker);
			if((pAttacker->withdrawalProbability()) > 0)
			{
				// if withdraw odds involved
				if (ACO_debug)
				{
					szTempBuffer.Format(L"Attacker retreat odds");
					szString.append(NEWLINE);
					szString.append(szTempBuffer.GetCString());
				}
				for (int n_D = 0; n_D < iNeededRoundsAttacker; n_D++)
				{
					E_HP_Att += ( (pAttacker->currHitPoints()) - (iNeededRoundsDefender-1)*iDamageToAttacker) * getCombatOddsSpecific(pAttacker,pDefender,iNeededRoundsDefender-1,n_D);
					prob_bottom_Att_HP += getCombatOddsSpecific(pAttacker,pDefender,iNeededRoundsDefender-1,n_D);
					if (ACO_debug)
					{
						szTempBuffer.Format(L"+%d * %.2f%%  (Def %d) (%d:%d)",
							( (pAttacker->currHitPoints()) - (iNeededRoundsDefender-1)*iDamageToAttacker),100.0f*getCombatOddsSpecific(pAttacker,pDefender,iNeededRoundsDefender-1,n_D),(pDefender->currHitPoints())-n_D*iDamageToDefender,iNeededRoundsDefender-1,n_D);
						szString.append(NEWLINE);
						szString.append(szTempBuffer.GetCString());
					}
				}
			}
			// finished with the attacker HP I think.

			if (ACO_debug)
			{
				szTempBuffer.Format(L"E[HP DEFENDER]\nOdds that attacker dies or retreats");
				szString.append(NEWLINE);
				szString.append(szTempBuffer.GetCString());
			}
			for (int n_D = 0; n_D < iNeededRoundsAttacker; n_D++)
			{
				//prob_defend[n_D] = getCombatOddsSpecific(pAttacker,pDefender,iNeededRoundsDefender,n_D);//attacker dies
				//prob_defend[n_D] += getCombatOddsSpecific(pAttacker,pDefender,iNeededRoundsDefender-1,n_D);//attacker retreats
				E_HP_Def += ( (pDefender->currHitPoints()) - n_D*iDamageToDefender) * (getCombatOddsSpecific(pAttacker,pDefender,iNeededRoundsDefender,n_D)+getCombatOddsSpecific(pAttacker,pDefender,iNeededRoundsDefender-1,n_D));
				if (ACO_debug)
				{
					szTempBuffer.Format(L"+%d * %.2f%%  (Att 0 or %d) (%d:%d)",
						( (pDefender->currHitPoints()) - n_D*iDamageToDefender),100.0f*(getCombatOddsSpecific(pAttacker,pDefender,iNeededRoundsDefender,n_D)+getCombatOddsSpecific(pAttacker,pDefender,iNeededRoundsDefender-1,n_D)),(pAttacker->currHitPoints())-(iNeededRoundsDefender-1)*iDamageToAttacker,iNeededRoundsDefender,n_D);
					szString.append(NEWLINE);
					szString.append(szTempBuffer.GetCString());
				}
			}
			prob_bottom_Def_HP = getCombatOddsSpecific(pAttacker,pDefender,iNeededRoundsDefender,iNeededRoundsAttacker-1);
			//prob_bottom_Def_HP += getCombatOddsSpecific(pAttacker,pDefender,iNeededRoundsDefender-1,iNeededRoundsAttacker-1);
			E_HP_Def_Defeat = E_HP_Def;
			E_HP_Def_Withdraw = 0.0f;

			if (pAttacker->combatLimit() < pDefender->maxHitPoints())//if attacker has a combatLimit (eg. catapult)
			{
				if (pAttacker->combatLimit() == iDamageToDefender*(iNeededRoundsAttacker-1))
				{
					//Then we have an odd situation because the last successful hit by an attacker will do 0 damage, and doing either iNeededRoundsAttacker or iNeededRoundsAttacker-1 will cause the same damage
					if (ACO_debug)
					{
						szTempBuffer.Format(L"Odds that attacker withdraws at combatLimit (abnormal)");
						szString.append(NEWLINE);
						szString.append(szTempBuffer.GetCString());
					}
					for (int n_A = 0; n_A < iNeededRoundsDefender; n_A++)
					{
						//prob_defend[iNeededRoundsAttacker-1] += getCombatOddsSpecific(pAttacker,pDefender,n_A,iNeededRoundsAttacker);//this is the defender at the combatLimit
						E_HP_Def += (float)iDefenderHitLimit * getCombatOddsSpecific(pAttacker,pDefender,n_A,iNeededRoundsAttacker);
						//should be the same as
						//E_HP_Def += ( (pDefender->currHitPoints()) - (iNeededRoundsAttacker-1)*iDamageToDefender) * getCombatOddsSpecific(pAttacker,pDefender,n_A,iNeededRoundsAttacker);
						E_HP_Def_Withdraw += (float)iDefenderHitLimit * getCombatOddsSpecific(pAttacker,pDefender,n_A,iNeededRoundsAttacker);
						prob_bottom_Def_HP += getCombatOddsSpecific(pAttacker,pDefender,n_A,iNeededRoundsAttacker);
						if (ACO_debug)
						{
							szTempBuffer.Format(L"+%d * %.2f%%  (Att %d) (%d:%d)",
								iDefenderHitLimit,100.0f*getCombatOddsSpecific(pAttacker,pDefender,n_A,iNeededRoundsAttacker),100-n_A*iDamageToAttacker,n_A,iNeededRoundsAttacker);
							szString.append(NEWLINE);
							szString.append(szTempBuffer.GetCString());
						}
					}
				}
				else // normal situation
				{
					if (ACO_debug)
					{
						szTempBuffer.Format(L"Odds that attacker withdraws at combatLimit (normal)",pAttacker->combatLimit());
						szString.append(NEWLINE);
						szString.append(szTempBuffer.GetCString());
					}

					for (int n_A = 0; n_A < iNeededRoundsDefender; n_A++)
					{

						E_HP_Def += (float)iDefenderHitLimit * getCombatOddsSpecific(pAttacker,pDefender,n_A,iNeededRoundsAttacker);
						E_HP_Def_Withdraw += (float)iDefenderHitLimit * getCombatOddsSpecific(pAttacker,pDefender,n_A,iNeededRoundsAttacker);
						prob_bottom_Def_HP += getCombatOddsSpecific(pAttacker,pDefender,n_A,iNeededRoundsAttacker);
						if (ACO_debug)
						{
							szTempBuffer.Format(L"+%d * %.2f%%  (Att %d) (%d:%d)",
								iDefenderHitLimit,100.0f*getCombatOddsSpecific(pAttacker,pDefender,n_A,iNeededRoundsAttacker),GC.getMAX_HIT_POINTS()-n_A*iDamageToAttacker,n_A,iNeededRoundsAttacker);
							szString.append(NEWLINE);
							szString.append(szTempBuffer.GetCString());
						}
					}//for
				}//else
			}
			if (ACO_debug)
			{
				szString.append(NEWLINE);
			}

			float Scaling_Factor = 1.6f;//how many pixels per 1% of odds

			float AttackerKillOdds = 0.0f;
			float PullOutOdds = 0.0f;//Withdraw odds
			float RetreatOdds = 0.0f;
			float DefenderKillOdds = 0.0f;

			float CombatRatio = ((float)(pAttacker->currCombatStr(NULL, NULL))) / ((float)(pDefender->currCombatStr(pPlot, pAttacker)));
			// THE ALL-IMPORTANT COMBATRATIO


			float AttXP = (pDefender->attackXPValue())/CombatRatio;
			float DefXP = (pAttacker->defenseXPValue())*CombatRatio;// These two values are simply for the Unrounded XP display

			// General odds
			if (pAttacker->combatLimit() == pDefender->maxHitPoints()) //ie. we can kill the defender... I hope this is the most general form
			{
				//float AttackerKillOdds = 0.0f;
				for (int n_A = 0; n_A < iNeededRoundsDefender; n_A++)
				{
					AttackerKillOdds += getCombatOddsSpecific(pAttacker,pDefender,n_A,iNeededRoundsAttacker);
				}//for
			}
			else
			{
				// else we cannot kill the defender (eg. catapults attacking)
				for (int n_A = 0; n_A < iNeededRoundsDefender; n_A++)
				{
					PullOutOdds += getCombatOddsSpecific(pAttacker,pDefender,n_A,iNeededRoundsAttacker);
				}//for
			}
			if ((pAttacker->withdrawalProbability()) > 0)
			{
				for (int n_D = 0; n_D < iNeededRoundsAttacker; n_D++)
				{
					RetreatOdds += getCombatOddsSpecific(pAttacker,pDefender,iNeededRoundsDefender-1,n_D);
				}//for
			}
			for (int n_D = 0; n_D < iNeededRoundsAttacker; n_D++)
			{
				DefenderKillOdds += getCombatOddsSpecific(pAttacker,pDefender,iNeededRoundsDefender,n_D);
			}//for
			//DefenderKillOdds = 1.0f - (AttackerKillOdds + RetreatOdds + PullOutOdds);//this gives slight negative numbers sometimes, I think



			if (iView & getBugOptionINT("ACO__ShowSurvivalOdds", 0))
			{
				szTempBuffer.Format(L"%.2f%%",100.0f*(AttackerKillOdds+RetreatOdds+PullOutOdds));
				szTempBuffer2.Format(L"%.2f%%", 100.0f*(RetreatOdds+PullOutOdds+DefenderKillOdds));
				szString.append(gDLL->getText("TXT_ACO_SurvivalOdds"));
				szString.append(gDLL->getText("TXT_ACO_VS", szTempBuffer.GetCString(), szTempBuffer2.GetCString()));
				szString.append(NEWLINE);
			}

			if (pAttacker->withdrawalProbability()>=100)
			{
				// a rare situation indeed
				szString.append(gDLL->getText("TXT_ACO_SurvivalGuaranteed"));
				szString.append(NEWLINE);
			}

			//CvWString szTempBuffer2; // moved to elsewhere in the code (earlier)
			//CvWString szBuffer; // duplicate

			float prob1 = 100.0f*(AttackerKillOdds + PullOutOdds);//up to win odds
			float prob2 = prob1 + 100.0f*RetreatOdds;//up to retreat odds

			float prob = 100.0f*(AttackerKillOdds+RetreatOdds+PullOutOdds);
			int pixels_left = 199;// 1 less than 200 to account for right end bar

			int pixels = (2 * ((int)(prob1 + 0.5)))-1;  // 1% per pixel // subtracting one to account for left end bar
			int fullBlocks = pixels / 10;
			int lastBlock = pixels % 10;

			szString.append(L"<img=Art/ACO/green_bar_left_end.dds>");
			for (int i = 0; i < fullBlocks; ++i)
			{
				szString.append(L"<img=Art/ACO/green_bar_10.dds>");
				pixels_left -= 10;
			}
			if (lastBlock > 0)
			{
				szTempBuffer2.Format(L"<img=Art/ACO/green_bar_%d.dds>", lastBlock);
				szString.append(szTempBuffer2);
				pixels_left-= lastBlock;
			}


			pixels = 2 * ((int)(prob2 + 0.5)) - (pixels+1);//the number up to the next one...
			fullBlocks = pixels / 10;
			lastBlock = pixels % 10;
			for (int i = 0; i < fullBlocks; ++i)
			{
				szString.append(L"<img=Art/ACO/yellow_bar_10.dds>");
				pixels_left -= 10;
			}
			if (lastBlock > 0)
			{
				szTempBuffer2.Format(L"<img=Art/ACO/yellow_bar_%d.dds>", lastBlock);
				szString.append(szTempBuffer2);
				pixels_left-= lastBlock;
			}

			fullBlocks = pixels_left / 10;
			lastBlock = pixels_left % 10;
			for (int i = 0; i < fullBlocks; ++i)
			{
				szString.append(L"<img=Art/ACO/red_bar_10.dds>");
			}
			if (lastBlock > 0)
			{
				szTempBuffer2.Format(L"<img=Art/ACO/red_bar_%d.dds>", lastBlock);
				szString.append(szTempBuffer2);
			}

			szString.append(L"<img=Art/ACO/red_bar_right_end.dds> ");


			szString.append(NEWLINE);
			if (pAttacker->combatLimit() == pDefender->maxHitPoints())
			{
				szTempBuffer.Format(L": " SETCOLR L"%.2f%% " L"%d" ENDCOLR, TEXT_COLOR("COLOR_POSITIVE_TEXT"), 100.0f*AttackerKillOdds,iExperience);
				szString.append(gDLL->getText("TXT_ACO_Victory"));
				szString.append(szTempBuffer.GetCString());
				if (iAttackerExperienceModifier > 0)
				{
					szTempBuffer.Format(SETCOLR L"+%d" ENDCOLR,TEXT_COLOR("COLOR_HIGHLIGHT_TEXT"),iBonusAttackerXP);
					szString.append(szTempBuffer.GetCString());
				}

				szString.append(gDLL->getText("TXT_KEY_COLOR_POSITIVE"));
				szString.append(gDLL->getText("TXT_ACO_XP"));
				szString.append(gDLL->getText("TXT_KEY_COLOR_REVERT"));
				szString.append("  (");
				szString.append(gDLL->getText("TXT_KEY_COLOR_POSITIVE"));
				szTempBuffer.Format(L"%.1f",
					E_HP_Att_Victory/AttackerKillOdds);
				szString.append(szTempBuffer.GetCString());
				szString.append(gDLL->getText("TXT_ACO_HP"));
				szString.append(gDLL->getText("TXT_KEY_COLOR_REVERT"));
			}
			else
			{
				szTempBuffer.Format(L": " SETCOLR L"%.2f%% " L"%d" ENDCOLR,
					TEXT_COLOR("COLOR_POSITIVE_TEXT"),100.0f*PullOutOdds,GC.getDefineINT("EXPERIENCE_FROM_WITHDRAWL"));
				//iExperience,TEXT_COLOR("COLOR_POSITIVE_TEXT"), E_HP_Att_Victory/AttackerKillOdds);
				szString.append(gDLL->getText("TXT_ACO_Withdraw"));
				szString.append(szTempBuffer.GetCString());
				if (iAttackerExperienceModifier > 0)
				{
					szTempBuffer.Format(SETCOLR L"+%d" ENDCOLR,TEXT_COLOR("COLOR_HIGHLIGHT_TEXT"),iBonusWithdrawXP);
					szString.append(szTempBuffer.GetCString());
				}

				szString.append(gDLL->getText("TXT_KEY_COLOR_POSITIVE"));
				szString.append(gDLL->getText("TXT_ACO_XP"));
				szString.append(gDLL->getText("TXT_KEY_COLOR_REVERT"));
				szString.append("  (");
				szString.append(gDLL->getText("TXT_KEY_COLOR_POSITIVE"));
				szTempBuffer.Format(L"%.1f",E_HP_Att_Withdraw/PullOutOdds);
				szString.append(szTempBuffer.GetCString());
				szString.append(gDLL->getText("TXT_ACO_HP"));
				szString.append(gDLL->getText("TXT_KEY_COLOR_REVERT"));
				szString.append(",");
				szString.append(gDLL->getText("TXT_KEY_COLOR_NEGATIVE"));
				szTempBuffer.Format(L"%d",iDefenderHitLimit);
				szString.append(szTempBuffer.GetCString());
				szString.append(gDLL->getText("TXT_ACO_HP"));
				szString.append(gDLL->getText("TXT_KEY_COLOR_REVERT"));
			}
			szString.append(")");

			if (iDefenderOdds == 0)
			{
				szString.append(gDLL->getText("TXT_ACO_GuaranteedNoDefenderHit"));
				DefenderKillOdds = 0.0f;
			}

			if ((pAttacker->withdrawalProbability()) > 0)//if there are retreat odds
			{
				szString.append(NEWLINE);
				szTempBuffer.Format(L": " SETCOLR L"%.2f%% " ENDCOLR SETCOLR L"%d" ENDCOLR,
					TEXT_COLOR("COLOR_UNIT_TEXT"),100.0f*RetreatOdds,TEXT_COLOR("COLOR_POSITIVE_TEXT"),GC.getDefineINT("EXPERIENCE_FROM_WITHDRAWL"));
				//szString.append(gDLL->getText("TXT_ACO_Retreat"));
				szString.append(gDLL->getText("TXT_ACO_Withdraw")); // advc.048b
				szString.append(szTempBuffer.GetCString());
				if (iAttackerExperienceModifier > 0)
				{
					szTempBuffer.Format(SETCOLR L"+%d" ENDCOLR,TEXT_COLOR("COLOR_HIGHLIGHT_TEXT"),iBonusWithdrawXP);
					szString.append(szTempBuffer.GetCString());
				}
				szString.append(gDLL->getText("TXT_KEY_COLOR_POSITIVE"));
				szString.append(gDLL->getText("TXT_ACO_XP"));
				szString.append(gDLL->getText("TXT_KEY_COLOR_REVERT"));
				szString.append("  (");
				szTempBuffer.Format(SETCOLR L"%d" ENDCOLR ,
					TEXT_COLOR("COLOR_UNIT_TEXT"),E_HP_Att_Retreat);
				szString.append(szTempBuffer.GetCString());
				szString.append(gDLL->getText("TXT_ACO_HP_NEUTRAL"));
				szString.append(")");
				//szString.append(gDLL->getText("TXT_KEY_COLOR_REVERT"));
			}

			szString.append(NEWLINE);
			szTempBuffer.Format(L": " SETCOLR L"%.2f%% " L"%d" ENDCOLR,
				TEXT_COLOR("COLOR_NEGATIVE_TEXT"),100.0f*DefenderKillOdds,iDefExperienceKill);
			szString.append(gDLL->getText("TXT_ACO_Defeat"));
			szString.append(szTempBuffer.GetCString());
			if (iDefenderExperienceModifier > 0)
			{
				szTempBuffer.Format(SETCOLR L"+%d" ENDCOLR,TEXT_COLOR("COLOR_HIGHLIGHT_TEXT"),iBonusDefenderXP);
				szString.append(szTempBuffer.GetCString());
			}
			szString.append(gDLL->getText("TXT_KEY_COLOR_NEGATIVE"));
			szString.append(gDLL->getText("TXT_ACO_XP"));
			szString.append(gDLL->getText("TXT_KEY_COLOR_REVERT"));
			szString.append("  (");
			szString.append(gDLL->getText("TXT_KEY_COLOR_NEGATIVE"));
			szTempBuffer.Format(L"%.1f",
				(iDefenderOdds != 0 ? E_HP_Def_Defeat/(RetreatOdds+DefenderKillOdds):0.0));
			szString.append(szTempBuffer.GetCString());
			szString.append(gDLL->getText("TXT_ACO_HP"));
			szString.append(gDLL->getText("TXT_KEY_COLOR_REVERT"));
			szString.append(")");

			// <advc.048> XP details moved up so that it appears right after the basic XP info above
			bool bDoRange = ((iView & getBugOptionINT("ACO__ShowExperienceRange", 0))
					&& pAttacker->combatLimit() >= pDefender->maxHitPoints()); //medium and high only
			/*  Had to leave behind the part that shows the combat ratio; don't want
				that this early. */
			if(bDoRange) // </advc.048>
			{
				//we do an XP range display
				//This should hopefully now work for any max and min XP values.

				if (pAttacker->combatLimit() == pDefender->maxHitPoints())
				{
					FAssert(/* advc.312: */ iMaxXPAtt
							> GC.getDefineINT("MIN_EXPERIENCE_PER_COMBAT")); //ensuring the differences is at least 1
					int size = /* advc.312: */ iMaxXPAtt
							- GC.getDefineINT("MIN_EXPERIENCE_PER_COMBAT");
					float* CombatRatioThresholds = new float[size];

					for (int i = 0; i < size; i++) //setup the array
					{
						CombatRatioThresholds[i] = ((float)(pDefender->attackXPValue()))/((float)(
								/* <advc.312> */ iMaxXPAtt /* </advc.132> */ -i));
						//For standard game, this is the list created:
						//  {4/10, 4/9, 4/8,
						//   4/7, 4/6, 4/5,
						//   4/4, 4/3, 4/2}
					}
					for (int i = size-1; i >= 0; i--) // find which range we are in
					{
						//starting at i = 8, going through to i = 0
						if (CombatRatio>CombatRatioThresholds[i])
						{

							if (i == size - 1)//highest XP value already
							{
								szString.append(NEWLINE);
								szTempBuffer.Format(L"(%.2f:%d",
									CombatRatioThresholds[i],GC.getDefineINT("MIN_EXPERIENCE_PER_COMBAT")+1);
								szString.append(szTempBuffer.GetCString());
								szString.append(gDLL->getText("TXT_ACO_XP"));
								szTempBuffer.Format(L"), (R=" SETCOLR L"%.2f" ENDCOLR
										L":" SETCOLR L"%d" ENDCOLR, // advc.048
										TEXT_COLOR("COLOR_HIGHLIGHT_TEXT"),
										CombatRatio,
										TEXT_COLOR("COLOR_POSITIVE_TEXT"), // advc.048
										iExperience);
								szString.append(szTempBuffer.GetCString());
								szString.append(gDLL->getText("TXT_ACO_XP"));
								szString.append(")");
							}
							else // normal situation
							{
								szString.append(NEWLINE);
								szTempBuffer.Format(L"(%.2f:%d",
										CombatRatioThresholds[i],
										iMaxXPAtt // advc.312
										-i);
								szString.append(szTempBuffer.GetCString());
								szString.append(gDLL->getText("TXT_ACO_XP"));
								szTempBuffer.Format(L"), (R=" SETCOLR L"%.2f" ENDCOLR
										L":" SETCOLR L"%d" ENDCOLR, // advc.048
										TEXT_COLOR("COLOR_HIGHLIGHT_TEXT"),
										CombatRatio,
										TEXT_COLOR("COLOR_POSITIVE_TEXT"), // advc.048
										iMaxXPAtt // advc.312
										-(i+1));
								szString.append(szTempBuffer.GetCString());
								szString.append(gDLL->getText("TXT_ACO_XP"));
								szTempBuffer.Format(L"), (>%.2f:%d",
										CombatRatioThresholds[i+1],
										iMaxXPAtt // advc.312
										-(i+2));
								szString.append(szTempBuffer.GetCString());
								szString.append(gDLL->getText("TXT_ACO_XP"));
								szString.append(")");
							}
							break;

						}
						else//very rare (ratio less than or equal to 0.4)
						{
							if (i==0)//maximum XP
							{
								szString.append(NEWLINE);
								szTempBuffer.Format(L"(R=" SETCOLR L"%.2f" ENDCOLR
										// <advc.048>
										L":" SETCOLR L"%d" ENDCOLR,
										/*  Was COLOR_POSITIVE_TEXT; reserve that
											for the XP. */
										TEXT_COLOR("COLOR_HIGHLIGHT_TEXT"),
										// </advc.048>
										CombatRatio,
										TEXT_COLOR("COLOR_POSITIVE_TEXT"), // advc.048
										iMaxXPAtt); // advc.312
								szString.append(szTempBuffer.GetCString());

								szTempBuffer.Format(L"), (>%.2f:%d",
									CombatRatioThresholds[i],
									iMaxXPAtt // advc.312
									-1);
								szString.append(szTempBuffer.GetCString());
								szString.append(gDLL->getText("TXT_ACO_XP"));
								szString.append(")");
								break;
							}//if
						}// else if
					}//for
					delete[] CombatRatioThresholds; // kmodx: memory leak
					//throw away the array
				}//if
			} // else if
			//Finished Showing XP range display
			// advc.048: Moved up so that it's shown right after the XP range
			if (iView & getBugOptionINT("ACO__ShowUnroundedExperience", 2))
			{
				szTempBuffer.Format(L"%.2f", AttXP);
				szTempBuffer2.Format(L"%.2f", DefXP);
				szString.append(gDLL->getText("TXT_ACO_UnroundedXP"));
				szString.append(gDLL->getText("TXT_ACO_VS", szTempBuffer.GetCString(), szTempBuffer2.GetCString()));
			}

			float HP_percent_cutoff = 0.5f; // Probabilities lower than this (in percent) will not be shown individually for the HP detail section.
			if (!getBugOptionBOOL("ACO__MergeShortBars", true))
			{
				HP_percent_cutoff = 0.0f;
			}
			int first_combined_HP_Att = 0;
			int first_combined_HP_Def = 0;
			int last_combined_HP;
			float combined_HP_sum = 0.0f;
			BOOL bCondensed = false;



			//START ATTACKER DETAIL HP HERE
			// Individual bars for each attacker HP outcome.
			if (iView & getBugOptionINT("ACO__ShowAttackerHealthBars", 1))
			{
				for (int n_A = 0; n_A < iNeededRoundsDefender-1; n_A++)
				{
					prob = 100.0f*getCombatOddsSpecific(pAttacker,pDefender,n_A,iNeededRoundsAttacker);
					if (prob > HP_percent_cutoff || n_A==0)
					{
						if (bCondensed) // then we need to print the prev ones
						{
							pixels = (int)(Scaling_Factor*combined_HP_sum + 0.5);  // 1% per pixel
							fullBlocks = (pixels) / 10;
							lastBlock = (pixels) % 10;
							//if(pixels>=2) {szString.append(L"<img=Art/ACO/green_bar_left_end.dds>");}
							szString.append(NEWLINE);
							szString.append(L"<img=Art/ACO/green_bar_left_end.dds>");
							for (int iI = 0; iI < fullBlocks; ++iI)
							{
								szString.append(L"<img=Art/ACO/green_bar_10.dds>");
							}
							if (lastBlock > 0)
							{
								szTempBuffer2.Format(L"<img=Art/ACO/green_bar_%d.dds>", lastBlock);
								szString.append(szTempBuffer2);
							}
							szString.append(L"<img=Art/ACO/green_bar_right_end.dds>");
							szString.append(L" ");

							szString.append(gDLL->getText("TXT_KEY_COLOR_POSITIVE"));
							if (last_combined_HP!=first_combined_HP_Att)
							{
								szTempBuffer.Format(L"%d",last_combined_HP);
								szString.append(szTempBuffer.GetCString());
								szString.append(gDLL->getText("TXT_ACO_HP"));
								szString.append(gDLL->getText("-"));
							}

							szTempBuffer.Format(L"%d",first_combined_HP_Att);
							szString.append(szTempBuffer.GetCString());
							szString.append(gDLL->getText("TXT_ACO_HP"));
							szString.append(gDLL->getText("TXT_KEY_COLOR_REVERT"));
							szTempBuffer.Format(L" %.2f%%",combined_HP_sum);
							szString.append(szTempBuffer.GetCString());

							bCondensed = false;//resetting
							combined_HP_sum = 0.0f;//resetting this variable
							last_combined_HP = 0;
						}

						szString.append(NEWLINE);
						pixels = (int)(Scaling_Factor*prob + 0.5);  // 1% per pixel
						fullBlocks = (pixels) / 10;
						lastBlock = (pixels) % 10;
						szString.append(L"<img=Art/ACO/green_bar_left_end.dds>");
						for (int iI = 0; iI < fullBlocks; ++iI)
						{
							szString.append(L"<img=Art/ACO/green_bar_10.dds>");
						}
						if (lastBlock > 0)
						{
							szTempBuffer2.Format(L"<img=Art/ACO/green_bar_%d.dds>", lastBlock);
							szString.append(szTempBuffer2);
						}
						szString.append(L"<img=Art/ACO/green_bar_right_end.dds>");
						szString.append(L" ");

						szString.append(gDLL->getText("TXT_KEY_COLOR_POSITIVE"));
						szTempBuffer.Format(L"%d",
							((pAttacker->currHitPoints()) - n_A*iDamageToAttacker));
						szString.append(szTempBuffer.GetCString());
						szString.append(gDLL->getText("TXT_ACO_HP"));
						szString.append(gDLL->getText("TXT_KEY_COLOR_REVERT"));
						szTempBuffer.Format(L" %.2f%%",
							prob);
						szString.append(szTempBuffer.GetCString());
					}
					else // we add to the condensed list
					{
						bCondensed = true;
						first_combined_HP_Att = std::max(first_combined_HP_Att,((pAttacker->currHitPoints()) - n_A*iDamageToAttacker));
						last_combined_HP = ((pAttacker->currHitPoints()) - n_A*iDamageToAttacker);
						combined_HP_sum += prob;
					}
				}

				if (bCondensed) // then we need to print the prev ones
				{
					szString.append(NEWLINE);
					pixels = (int)(Scaling_Factor*combined_HP_sum + 0.5);  // 1% per pixel
					fullBlocks = (pixels) / 10;
					lastBlock = (pixels) % 10;

					szString.append(L"<img=Art/ACO/green_bar_left_end.dds>");
					for (int iI = 0; iI < fullBlocks; ++iI)
					{
						szString.append(L"<img=Art/ACO/green_bar_10.dds>");
					}
					if (lastBlock > 0)
					{
						szTempBuffer2.Format(L"<img=Art/ACO/green_bar_%d.dds>", lastBlock);
						szString.append(szTempBuffer2);
					}

					szString.append(L"<img=Art/ACO/green_bar_right_end.dds>");
					szString.append(L" ");

					szString.append(gDLL->getText("TXT_KEY_COLOR_POSITIVE"));
					if (last_combined_HP!=first_combined_HP_Att)
					{
						szTempBuffer.Format(L"%d",last_combined_HP);
						szString.append(szTempBuffer.GetCString());
						szString.append(gDLL->getText("TXT_ACO_HP"));
						szString.append(gDLL->getText("-"));
					}
					szTempBuffer.Format(L"%d",first_combined_HP_Att);
					szString.append(szTempBuffer.GetCString());
					szString.append(gDLL->getText("TXT_ACO_HP"));
					szString.append(gDLL->getText("TXT_KEY_COLOR_REVERT"));
					szTempBuffer.Format(L" %.2f%%",combined_HP_sum);
					szString.append(szTempBuffer.GetCString());

					bCondensed = false;//resetting
					combined_HP_sum = 0.0f;//resetting this variable
					last_combined_HP = 0;
				}
				/*  At the moment I am not allowing the lowest Attacker HP value to be condensed,
					as it would be confusing if it includes retreat odds.
					I may include this in the future though, but probably only if retreat odds are zero. */

				float prob_victory = 100.0f*getCombatOddsSpecific(pAttacker,pDefender,iNeededRoundsDefender-1,iNeededRoundsAttacker);
				float prob_retreat = 100.0f*RetreatOdds;

				szString.append(NEWLINE);
				int green_pixels = (int)(Scaling_Factor*prob_victory + 0.5);
				int yellow_pixels = (int)(Scaling_Factor*(prob_retreat+prob_victory) + 0.5) - green_pixels;//makes the total length of the bar more accurate - more important than the length of the pieces
				green_pixels+=1;//we put an extra 2 on every one of the bar pixel counts
				if (yellow_pixels>=1)
				{
					yellow_pixels+=1;
				}
				else
				{
					green_pixels+=1;
				}
				szString.append(L"<img=Art/ACO/green_bar_left_end.dds>");
				green_pixels--;

				green_pixels--;//subtracting off the right end
				int fullBlocks = green_pixels / 10;
				int lastBlock = green_pixels % 10;
				for (int iI = 0; iI < fullBlocks; ++iI)
				{
					szString.append(L"<img=Art/ACO/green_bar_10.dds>");
				}//for
				if (lastBlock > 0)
				{
					szTempBuffer2.Format(L"<img=Art/ACO/green_bar_%d.dds>", lastBlock);
					szString.append(szTempBuffer2);
				}//if
				if (yellow_pixels>=1)// then there will at least be a right end yellow pixel
				{
					yellow_pixels--;//subtracting off right end
					fullBlocks = yellow_pixels / 10;
					lastBlock = yellow_pixels % 10;
					for (int iI = 0; iI < fullBlocks; ++iI)
					{
						szString.append(L"<img=Art/ACO/yellow_bar_10.dds>");
					}//for
					if (lastBlock > 0)
					{
						szTempBuffer2.Format(L"<img=Art/ACO/yellow_bar_%d.dds>", lastBlock);
						szString.append(szTempBuffer2);
					}
					szString.append(L"<img=Art/ACO/yellow_bar_right_end.dds>");
					//finished
				}
				else
				{
					szString.append(L"<img=Art/ACO/green_bar_right_end.dds>");
					//finished
				}//else if

				szString.append(L" ");
				szString.append(gDLL->getText("TXT_KEY_COLOR_POSITIVE"));
				szTempBuffer.Format(L"%d",((pAttacker->currHitPoints()) - (iNeededRoundsDefender-1)*iDamageToAttacker));
				szString.append(szTempBuffer.GetCString());
				szString.append(gDLL->getText("TXT_ACO_HP"));
				szString.append(gDLL->getText("TXT_KEY_COLOR_REVERT"));
				szTempBuffer.Format(L" %.2f%%",prob_victory+prob_retreat);
				szString.append(szTempBuffer.GetCString());
			}
			//END ATTACKER DETAIL HP HERE


			//START DEFENDER DETAIL HP HERE
			first_combined_HP_Def = pDefender->currHitPoints();
			if (iView & getBugOptionINT("ACO__ShowDefenderHealthBars", 2))
			{
				float prob = 0.0f;
				for (int n_D = iNeededRoundsAttacker; n_D >= 1; n_D--)//
				{
					if (pAttacker->combatLimit() >= pDefender->maxHitPoints())// a unit with a combat limit
					{
						if (n_D == iNeededRoundsAttacker)
						{
							n_D--;//we don't need to do HP for when the unit is dead.
						}
					}

					int def_HP = std::max((pDefender->currHitPoints()) - n_D*iDamageToDefender,(pDefender->maxHitPoints()  - pAttacker->combatLimit()));

					if ((pDefender->maxHitPoints() - pAttacker->combatLimit()) == pDefender->currHitPoints() - (n_D-1)*iDamageToDefender)
					{
						// if abnormal
						if (n_D == iNeededRoundsAttacker)
						{
							n_D--;
							def_HP = (pDefender->maxHitPoints()  - pAttacker->combatLimit());
							prob += 100.0f*PullOutOdds;
							prob += 100.0f*(getCombatOddsSpecific(pAttacker,pDefender,iNeededRoundsDefender,n_D)+(getCombatOddsSpecific(pAttacker,pDefender,iNeededRoundsDefender-1,n_D)));
						}
					}
					else
					{
						//not abnormal
						if (n_D == iNeededRoundsAttacker)
						{
							prob += 100.0f*PullOutOdds;
						}
						else
						{
							prob += 100.0f*(getCombatOddsSpecific(pAttacker,pDefender,iNeededRoundsDefender,n_D)+(getCombatOddsSpecific(pAttacker,pDefender,iNeededRoundsDefender-1,n_D)));
						}
					}

					if (prob > HP_percent_cutoff || (pAttacker->combatLimit()<pDefender->maxHitPoints() && (n_D==iNeededRoundsAttacker)))
					{
						if (bCondensed) // then we need to print the prev ones
						{
							szString.append(NEWLINE);

							pixels = (int)(Scaling_Factor*combined_HP_sum + 0.5);  // 1% per pixel
							fullBlocks = (pixels) / 10;
							lastBlock = (pixels) % 10;
							szString.append(L"<img=Art/ACO/red_bar_left_end.dds>");
							for (int iI = 0; iI < fullBlocks; ++iI)
							{
								szString.append(L"<img=Art/ACO/red_bar_10.dds>");
							}
							if (lastBlock > 0)
							{
								szTempBuffer2.Format(L"<img=Art/ACO/red_bar_%d.dds>", lastBlock);
								szString.append(szTempBuffer2);
							}
							szString.append(L"<img=Art/ACO/red_bar_right_end.dds>");
							szString.append(L" ");
							szString.append(gDLL->getText("TXT_KEY_COLOR_NEGATIVE"));
							szTempBuffer.Format(L"%dHP",first_combined_HP_Def);
							szString.append(szTempBuffer.GetCString());
							szString.append(gDLL->getText("TXT_ACO_HP"));
							if (first_combined_HP_Def!=last_combined_HP)
							{
								szString.append("-");
								szTempBuffer.Format(L"%d",last_combined_HP);
								szString.append(szTempBuffer.GetCString());
								szString.append(gDLL->getText("TXT_ACO_HP"));
							}
							szString.append(gDLL->getText("TXT_KEY_COLOR_REVERT"));
							szTempBuffer.Format(L" %.2f%%",
								combined_HP_sum);
							szString.append(szTempBuffer.GetCString());

							bCondensed = false;//resetting
							combined_HP_sum = 0.0f;//resetting this variable
						}

						szString.append(NEWLINE);
						pixels = (int)(Scaling_Factor*prob + 0.5);  // 1% per pixel
						fullBlocks = (pixels) / 10;
						lastBlock = (pixels) % 10;
						//if(pixels>=2) // this is now guaranteed by the way we define number of pixels
						//{
						szString.append(L"<img=Art/ACO/red_bar_left_end.dds>");
						for (int iI = 0; iI < fullBlocks; ++iI)
						{
							szString.append(L"<img=Art/ACO/red_bar_10.dds>");
						}
						if (lastBlock > 0)
						{
							szTempBuffer2.Format(L"<img=Art/ACO/red_bar_%d.dds>", lastBlock);
							szString.append(szTempBuffer2);
						}
						szString.append(L"<img=Art/ACO/red_bar_right_end.dds>");
						//}
						szString.append(L" ");

						szTempBuffer.Format(SETCOLR L"%d" ENDCOLR,
							TEXT_COLOR("COLOR_NEGATIVE_TEXT"),def_HP);
						szString.append(szTempBuffer.GetCString());
						szString.append(gDLL->getText("TXT_KEY_COLOR_NEGATIVE"));
						szString.append(gDLL->getText("TXT_ACO_HP"));
						szString.append(gDLL->getText("TXT_KEY_COLOR_REVERT"));
						szTempBuffer.Format(L" %.2f%%",prob);
						szString.append(szTempBuffer.GetCString());
					}
					else
					{
						bCondensed = true;
						first_combined_HP_Def = (std::min(first_combined_HP_Def,def_HP));
						last_combined_HP = std::max(((pDefender->currHitPoints()) - n_D*iDamageToDefender),pDefender->maxHitPoints()-pAttacker->combatLimit());
						combined_HP_sum += prob;
					}
					prob = 0.0f;
				}//for n_D

				if (bCondensed && iNeededRoundsAttacker>1) // then we need to print the prev ones
					// the reason we need iNeededRoundsAttacker to be greater than 1 is that if it's equal to 1 then we end up with the defender detailed HP bar show up twice, because it will also get printed below
				{
					szString.append(NEWLINE);
					pixels = (int)(Scaling_Factor*combined_HP_sum + 0.5);  // 1% per pixel
					fullBlocks = (pixels) / 10;
					lastBlock = (pixels) % 10;
					//if(pixels>=2) {szString.append(L"<img=Art/ACO/green_bar_left_end.dds>");}
					szString.append(L"<img=Art/ACO/red_bar_left_end.dds>");
					for (int iI = 0; iI < fullBlocks; ++iI)
					{
						szString.append(L"<img=Art/ACO/red_bar_10.dds>");
					}
					if (lastBlock > 0)
					{
						szTempBuffer2.Format(L"<img=Art/ACO/red_bar_%d.dds>", lastBlock);
						szString.append(szTempBuffer2);
					}
					szString.append(L"<img=Art/ACO/red_bar_right_end.dds>");
					szString.append(L" ");
					szString.append(gDLL->getText("TXT_KEY_COLOR_NEGATIVE"));
					szTempBuffer.Format(L"%d",first_combined_HP_Def);
					szString.append(szTempBuffer.GetCString());
					szString.append(gDLL->getText("TXT_ACO_HP"));
					if (first_combined_HP_Def != last_combined_HP)
					{
						szTempBuffer.Format(L"-%d",last_combined_HP);
						szString.append(szTempBuffer.GetCString());
						szString.append(gDLL->getText("TXT_ACO_HP"));
					}
					szString.append(gDLL->getText("TXT_KEY_COLOR_REVERT"));
					szTempBuffer.Format(L" %.2f%%",combined_HP_sum);
					szString.append(szTempBuffer.GetCString());

					bCondensed = false;//resetting
					combined_HP_sum = 0.0f;//resetting this variable
				}

				//print the unhurt value...always

				prob = 100.0f*(getCombatOddsSpecific(pAttacker,pDefender,iNeededRoundsDefender,0)+(getCombatOddsSpecific(pAttacker,pDefender,iNeededRoundsDefender-1,0)));
				pixels = (int)(Scaling_Factor*prob + 0.5);  // 1% per pixel
				fullBlocks = (pixels) / 10;
				lastBlock = (pixels) % 10;

				szString.append(NEWLINE);
				szString.append(L"<img=Art/ACO/red_bar_left_end.dds>");
				for (int iI = 0; iI < fullBlocks; ++iI)
				{
					szString.append(L"<img=Art/ACO/red_bar_10.dds>");
				}
				if (lastBlock > 0)
				{
					szTempBuffer2.Format(L"<img=Art/ACO/red_bar_%d.dds>", lastBlock);
					szString.append(szTempBuffer2);
				}
				szString.append(L"<img=Art/ACO/red_bar_right_end.dds>");
				szString.append(L" ");
				szString.append(gDLL->getText("TXT_KEY_COLOR_NEGATIVE"));
				szTempBuffer.Format(L"%d",pDefender->currHitPoints());
				szString.append(szTempBuffer.GetCString());
				szString.append(gDLL->getText("TXT_ACO_HP"));
				szString.append(gDLL->getText("TXT_KEY_COLOR_REVERT"));
				szTempBuffer.Format(L" %.2f%%",prob);
				szString.append(szTempBuffer.GetCString());
			}
			//END DEFENDER DETAIL HP HERE

			// advc.048: Avg. health and unharmed odds move up; placed right after bar diagrams.
			if (iView & getBugOptionINT("ACO__ShowAverageHealth", 2))
			{
				szTempBuffer.Format(L"%.1f",E_HP_Att);
				szTempBuffer2.Format(L"%.1f",E_HP_Def);
				szString.append(gDLL->getText("TXT_ACO_AverageHP"));
				szString.append(gDLL->getText("TXT_ACO_VS", szTempBuffer.GetCString(), szTempBuffer2.GetCString()));
			}
			if (iView & getBugOptionINT("ACO__ShowUnharmedOdds", 0))
			{
				szTempBuffer.Format(L"%.2f%%",100.0f*AttackerUnharmed);
				szTempBuffer2.Format(L"%.2f%%",100.0f*DefenderUnharmed);
				szString.append(gDLL->getText("TXT_ACO_Unharmed"));
				szString.append(gDLL->getText("TXT_ACO_VS", szTempBuffer.GetCString(), szTempBuffer2.GetCString()));
			}

			if (iView & getBugOptionINT("ACO__ShowBasicInfo", 2))
			{	// advc.048: Opening parenthesis added
				szTempBuffer.Format(L"(" SETCOLR L"%d" ENDCOLR L", " SETCOLR L"%d " ENDCOLR,
					TEXT_COLOR("COLOR_POSITIVE_TEXT"), iDamageToDefender, TEXT_COLOR("COLOR_NEGATIVE_TEXT"), iDamageToAttacker);
				szString.append(NEWLINE);
				szString.append(szTempBuffer.GetCString());
				szString.append(gDLL->getText("TXT_ACO_HP"));
				szString.append(") "); // advc.048: Closing parenthesis added
				szString.append(gDLL->getText("TXT_ACO_MULTIPLY"));
				// advc.048: Opening parenthesis added
				szTempBuffer.Format(L" (" SETCOLR L"%d" ENDCOLR L", " SETCOLR L"%d " ENDCOLR,
					TEXT_COLOR("COLOR_POSITIVE_TEXT"),iNeededRoundsAttacker,TEXT_COLOR("COLOR_NEGATIVE_TEXT"),
					iNeededRoundsDefender);
				szString.append(szTempBuffer.GetCString());
				szString.append(gDLL->getText("TXT_ACO_HitsAt"));
				// advc.048: Closing parenthesis added
				szTempBuffer.Format(L")" SETCOLR L" %.1f%%" ENDCOLR,
					TEXT_COLOR("COLOR_POSITIVE_TEXT"),float(iAttackerOdds)*100.0f / float(GC.getDefineINT("COMBAT_DIE_SIDES")));
				szString.append(szTempBuffer.GetCString());
			}
			/*  advc.048: The else branch of this conditional contained the XP range code,
				which has moved (way) up. */
			if(!bDoRange)
			{
				if (iView & getBugOptionINT("ACO__ShowBasicInfo", 2))
				{	// advc.048: Semicolon instead of period
					szTempBuffer.Format(L"; R=" SETCOLR L"%.2f" ENDCOLR,
							TEXT_COLOR("COLOR_HIGHLIGHT_TEXT"),CombatRatio);
					szString.append(szTempBuffer.GetCString());
				}
			}
			// (advc.048: XP info, avg. health and unharmed odds moved up)
			szString.append(NEWLINE);

			if (iView & getBugOptionINT("ACO__ShowShiftInstructions", 1))
			{	// <advc.048>
				CvWString szShiftHelp(gDLL->getText("TXT_ACO_PressSHIFT"));
				szString.append(szShiftHelp);
				if(bBestOddsHelp) {
					CvWString szAltHelp(gDLL->getText("TXT_ACO_PressALT"));
					/*  The translations don't fit in one line. 60 for the color tags,
						which don't take up space. */
					if(szShiftHelp.length() + szAltHelp.length() >= 50 + 60)
						szString.append(NEWLINE);
					else { // Separator
						szTempBuffer.Format(SETCOLR L", " ENDCOLR,
								TEXT_COLOR("COLOR_HIGHLIGHT_TEXT"));
						szString.append(szTempBuffer.GetCString());
					}
					szString.append(szAltHelp);
				} // </advc.048>
				szString.append(NEWLINE);
			}

		}//if ACO_enabled
	}

	//////////

	if (ACO_enabled)
	{
		szString.append(NEWLINE);

		szTempBuffer.Format(L"%.2f",
			((pAttacker->getDomainType() == DOMAIN_AIR) ? pAttacker->airCurrCombatStrFloat(pDefender) : pAttacker->currCombatStrFloat(NULL, NULL)));

		if (pAttacker->isHurt())
		{
			szTempBuffer.append(L" (");
			szTempBuffer.append(gDLL->getText("TXT_ACO_INJURED_HP",
				pAttacker->currHitPoints(),
				pAttacker->maxHitPoints()));
			szTempBuffer.append(L")");
		}


		szTempBuffer2.Format(L"%.2f",
			pDefender->currCombatStrFloat(pPlot, pAttacker)
			);

		if (pDefender->isHurt())
		{
			szTempBuffer2.append(L" (");
			szTempBuffer2.append(gDLL->getText("TXT_ACO_INJURED_HP",
				pDefender->currHitPoints(),
				pDefender->maxHitPoints()));
			szTempBuffer2.append(L")");
		}

		szString.append(gDLL->getText("TXT_ACO_VS", szTempBuffer.GetCString(), szTempBuffer2.GetCString()));
		// advc.048: Moved attacker info above the modifier label
		if ((iView & getBugOptionINT("ACO__ShowAttackerInfo", 3)))
		{
			szString.append(NEWLINE);
			setUnitHelp(szString, pAttacker, true, true);
		}

		if (((!(pDefender->immuneToFirstStrikes())) && (pAttacker->maxFirstStrikes() > 0)) || (pAttacker->maxCombatStr(NULL,NULL)!=pAttacker->baseCombatStr()*100))
		{
			//if attacker uninjured strength is not the same as base strength (i.e. modifiers are in effect) or first strikes exist, then
			if (getBugOptionBOOL("ACO__ShowModifierLabels", false))
			{
				szString.append(gDLL->getText("TXT_ACO_AttackModifiers"));
			}
		}


		szString.append(gDLL->getText("TXT_KEY_COLOR_POSITIVE"));

		szString.append(L' ');//XXX

		if (!(pDefender->immuneToFirstStrikes()))
		{
			if (pAttacker->maxFirstStrikes() > 0)
			{
				if (pAttacker->firstStrikes() == pAttacker->maxFirstStrikes())
				{
					if (pAttacker->firstStrikes() == 1)
					{
						szString.append(NEWLINE);
						szString.append(gDLL->getText("TXT_KEY_UNIT_ONE_FIRST_STRIKE"));
					}
					else
					{
						szString.append(NEWLINE);
						szString.append(gDLL->getText("TXT_KEY_UNIT_NUM_FIRST_STRIKES", pAttacker->firstStrikes()));
					}
				}
				else
				{
					szString.append(NEWLINE);
					szString.append(gDLL->getText("TXT_KEY_UNIT_FIRST_STRIKE_CHANCES", pAttacker->firstStrikes(), pAttacker->maxFirstStrikes()));
				}
			}
		}

		iModifier = pAttacker->getExtraCombatPercent();

		if (iModifier != 0)
		{
			szString.append(NEWLINE);
			szString.append(gDLL->getText("TXT_KEY_COMBAT_PLOT_EXTRA_STRENGTH", iModifier));
		}


		szString.append(gDLL->getText("TXT_KEY_COLOR_REVERT"));

		szString.append(L' ');//XXX

		// advc.048: Moved defender info above the modifier label
		if (iView & getBugOptionINT("ACO__ShowDefenderInfo", 3))
		{
			szString.append(NEWLINE);
			setUnitHelp(szString, pDefender, true, true);
		}

		if (((!(pAttacker->immuneToFirstStrikes())) && (pDefender->maxFirstStrikes() > 0)) || (pDefender->maxCombatStr(pPlot,pAttacker)!=pDefender->baseCombatStr()*100))
		{
			//if attacker uninjured strength is not the same as base strength (i.e. modifiers are in effect) or first strikes exist, then
			if (getBugOptionBOOL("ACO__ShowModifierLabels", false))
			{
				szString.append(gDLL->getText("TXT_ACO_DefenseModifiers"));
			}
		}


		if (iView & getBugOptionINT("ACO__ShowDefenseModifiers", 3))
		{
			//if defense modifiers are enabled - recommend leaving this on unless Total defense Modifier is enabled

			szString.append(gDLL->getText("TXT_KEY_COLOR_NEGATIVE"));

			szString.append(L' ');//XXX

			if (!(pAttacker->immuneToFirstStrikes()))
			{
				if (pDefender->maxFirstStrikes() > 0)
				{
					if (pDefender->firstStrikes() == pDefender->maxFirstStrikes())
					{
						if (pDefender->firstStrikes() == 1)
						{
							szString.append(NEWLINE);
							szString.append(gDLL->getText("TXT_KEY_UNIT_ONE_FIRST_STRIKE"));
						}
						else
						{
							szString.append(NEWLINE);
							szString.append(gDLL->getText("TXT_KEY_UNIT_NUM_FIRST_STRIKES", pDefender->firstStrikes()));
						}
					}
					else
					{
						szString.append(NEWLINE);
						szString.append(gDLL->getText("TXT_KEY_UNIT_FIRST_STRIKE_CHANCES", pDefender->firstStrikes(), pDefender->maxFirstStrikes()));
					}
				}
			}
			/*  advc.003: Some 100 LoC moved into a subroutine b/c they were
				repeated in the !ACO_enabled branch. */
			appendNegativeModifiers(szString, pAttacker, pDefender, pPlot);

			szString.append(gDLL->getText("TXT_KEY_COLOR_REVERT"));

			szString.append(L' ');//XXX

			szString.append(gDLL->getText("TXT_KEY_COLOR_POSITIVE"));

			szString.append(L' ');//XXX
			// advc.003: Another batch of repeated modifiers
			appendPositiveModifiers(szString, pAttacker, pDefender, pPlot, true);
		}

		szString.append(gDLL->getText("TXT_KEY_COLOR_REVERT"));

		szString.append(L' ');//XXX

		if (iView & getBugOptionINT("ACO__ShowTotalDefenseModifier", 2))
		{
			//szString.append(L' ');//XXX
			if (pDefender->maxCombatStr(pPlot,pAttacker)>pDefender->baseCombatStr()*100) // modifier is positive
			{
				szTempBuffer.Format(SETCOLR L"%d%%" ENDCOLR,
					TEXT_COLOR("COLOR_NEGATIVE_TEXT"),(((pDefender->maxCombatStr(pPlot,pAttacker)))/pDefender->baseCombatStr())-100);
			}
			else   // modifier is negative
			{
				szTempBuffer.Format(SETCOLR L"%d%%" ENDCOLR,
					TEXT_COLOR("COLOR_POSITIVE_TEXT"),(100-((pDefender->baseCombatStr()*10000)/(pDefender->maxCombatStr(pPlot,pAttacker)))));
			}

			szString.append(gDLL->getText("TXT_ACO_TotalDefenseModifier"));
			szString.append(szTempBuffer.GetCString());
		}
	}//if
	/** What follows in the "else" block, is the original code **/
	else
	{	//ACO is not enabled
		// ADVANCED COMBAT ODDS v2.0, 3/11/09, PieceOfMind: END

		// <advc.048>
		if(iLengthSelectionList > 1) {
			szString.append(NEWLINE);
			setUnitHelp(szString, pAttacker, true, true, true);
		} // </advc.048>

		szOffenseOdds.Format(L"%.2f", ((pAttacker->getDomainType() == DOMAIN_AIR) ? pAttacker->airCurrCombatStrFloat(pDefender) : pAttacker->currCombatStrFloat(NULL, NULL)));
		szDefenseOdds.Format(L"%.2f", pDefender->currCombatStrFloat(pPlot, pAttacker));
		szString.append(NEWLINE);
		szString.append(gDLL->getText("TXT_KEY_COMBAT_PLOT_ODDS_VS", szOffenseOdds.GetCString(), szDefenseOdds.GetCString()));

		szString.append(L' ');//XXX

		szString.append(gDLL->getText("TXT_KEY_COLOR_POSITIVE"));

		szString.append(L' ');//XXX

		iModifier = pAttacker->getExtraCombatPercent();

		if (iModifier != 0)
		{
			szString.append(NEWLINE);
			szString.append(gDLL->getText("TXT_KEY_COMBAT_PLOT_EXTRA_STRENGTH", iModifier));
		}
		// advc.003: Same code as in the ACO_enabled branch; use subroutine instead.
		appendPositiveModifiers(szString, pAttacker, pDefender, pPlot, false);

		if (!(pDefender->immuneToFirstStrikes()))
		{
			if (pAttacker->maxFirstStrikes() > 0)
			{
				if (pAttacker->firstStrikes() == pAttacker->maxFirstStrikes())
				{
					if (pAttacker->firstStrikes() == 1)
					{
						szString.append(NEWLINE);
						szString.append(gDLL->getText("TXT_KEY_UNIT_ONE_FIRST_STRIKE"));
					}
					else
					{
						szString.append(NEWLINE);
						szString.append(gDLL->getText("TXT_KEY_UNIT_NUM_FIRST_STRIKES", pAttacker->firstStrikes()));
					}
				}
				else
				{
					szString.append(NEWLINE);
					szString.append(gDLL->getText("TXT_KEY_UNIT_FIRST_STRIKE_CHANCES", pAttacker->firstStrikes(), pAttacker->maxFirstStrikes()));
				}
			}
		}
		// advc.048: Commented out
		/*if (pAttacker->isHurt()) {
			szString.append(NEWLINE);
			szString.append(gDLL->getText("TXT_KEY_COMBAT_PLOT_HP", pAttacker->currHitPoints(), pAttacker->maxHitPoints()));
		}*/

		szString.append(gDLL->getText("TXT_KEY_COLOR_REVERT"));

		szString.append(L' ');//XXX

		szString.append(gDLL->getText("TXT_KEY_COLOR_NEGATIVE"));

		szString.append(L' ');//XXX
		// advc.003: Same code as in the ACO_enabled branch; use subroutine instead.
		appendNegativeModifiers(szString, pAttacker, pDefender, pPlot);

		if (!(pAttacker->immuneToFirstStrikes()))
		{
			if (pDefender->maxFirstStrikes() > 0)
			{
				if (pDefender->firstStrikes() == pDefender->maxFirstStrikes())
				{
					if (pDefender->firstStrikes() == 1)
					{
						szString.append(NEWLINE);
						szString.append(gDLL->getText("TXT_KEY_UNIT_ONE_FIRST_STRIKE"));
					}
					else
					{
						szString.append(NEWLINE);
						szString.append(gDLL->getText("TXT_KEY_UNIT_NUM_FIRST_STRIKES", pDefender->firstStrikes()));
					}
				}
				else
				{
					szString.append(NEWLINE);
					szString.append(gDLL->getText("TXT_KEY_UNIT_FIRST_STRIKE_CHANCES", pDefender->firstStrikes(), pDefender->maxFirstStrikes()));
				}
			}
		} // <advc.048>
		szString.append(gDLL->getText("TXT_KEY_COLOR_REVERT"));
		szString.append(NEWLINE);
		szString.append(gDLL->getText("TXT_KEY_MISC_VS"));
		szString.append(L' ');
		setUnitHelp(szString, pDefender, true, true, true);
		// Commented out: </advc.048>
		/*if (pDefender->isHurt()) {
			szString.append(NEWLINE);
			szString.append(gDLL->getText("TXT_KEY_COMBAT_PLOT_HP", pDefender->currHitPoints(), pDefender->maxHitPoints()));
		}*/
	}

	szString.append(gDLL->getText("TXT_KEY_COLOR_REVERT"));

	/* original code
	if ((gDLL->getChtLvl() > 0)) */
	if (GC.getGame().isDebugMode() // BBAI: Only display this info in debug mode so game can be played with cheat code entered
			&& bShift) // advc.007
	{
		szTempBuffer.Format(L"\nStack Compare Value = %d", kSelectionList.AI_compareStacks(pPlot));
		szString.append(szTempBuffer);

		if (pPlot->getPlotCity() != NULL)
		{
			szTempBuffer.Format(L"\nBombard turns = %d", kSelectionList.getBombardTurns(pPlot->getPlotCity()));
			szString.append(szTempBuffer);
		}

		const CvPlayerAI& kPlayer = GET_PLAYER(GC.getGame().getActivePlayer());
		int iOurStrengthDefense = kPlayer.AI_localDefenceStrength(pPlot, kPlayer.getTeam(), DOMAIN_LAND, 1, true, true, true);
		int iOurStrengthOffense = kPlayer.AI_localAttackStrength(pPlot, kPlayer.getTeam(), DOMAIN_LAND, 1, false, true, false);
		szTempBuffer.Format(L"\nPlot Strength(Ours)= d%d, o%d", iOurStrengthDefense, iOurStrengthOffense);
		szString.append(szTempBuffer);
		int iEnemyStrengthDefense = kPlayer.AI_localDefenceStrength(pPlot, NO_TEAM, DOMAIN_LAND, 1, true, true, true);
		int iEnemyStrengthOffense = kPlayer.AI_localAttackStrength(pPlot, NO_TEAM, DOMAIN_LAND, 1, false, true, false);
		szTempBuffer.Format(L"\nPlot Strength(Enemy)= d%d, o%d", iEnemyStrengthDefense, iEnemyStrengthOffense);
		szString.append(szTempBuffer);
	}

	szString.append(gDLL->getText("TXT_KEY_COLOR_REVERT"));

	return true;
}

// DO NOT REMOVE - needed for font testing - Moose
void createTestFontString(CvWStringBuffer& szString)
{
	int iI;
	szString.assign(L"!\"#$%&'()*+,-./0123456789:;<=>?@ABCDEFGHIJKLMNOPQRSTUVWXYZ[×]^_`abcdefghijklmnopqrstuvwxyz\n");
	szString.append(L"{}~\\ßÀÁÂÃÄÅÆÇÈÊËÌÍÎÏÐÑÒÓÔÕÖØÙÚÛÜÝÞŸßàáâãäåæçèêëìíîïðñòóôõö÷øùúûüýþÿ¿¡«»°ŠŒŽšœž™©®€£¢”‘“…’");
	for (iI=0;iI<NUM_YIELD_TYPES;++iI)
		szString.append(CvWString::format(L"%c", GC.getYieldInfo((YieldTypes) iI).getChar()));

	szString.append(L"\n");
	for (iI=0;iI<NUM_COMMERCE_TYPES;++iI)
		szString.append(CvWString::format(L"%c", GC.getCommerceInfo((CommerceTypes) iI).getChar()));
	szString.append(L"\n");
	for (iI = 0; iI < GC.getNumReligionInfos(); ++iI)
	{
		szString.append(CvWString::format(L"%c", GC.getReligionInfo((ReligionTypes) iI).getChar()));
		szString.append(CvWString::format(L"%c", GC.getReligionInfo((ReligionTypes) iI).getHolyCityChar()));
	}
	for (iI = 0; iI < GC.getNumCorporationInfos(); ++iI)
	{
		szString.append(CvWString::format(L"%c", GC.getCorporationInfo((CorporationTypes) iI).getChar()));
		szString.append(CvWString::format(L"%c", GC.getCorporationInfo((CorporationTypes) iI).getHeadquarterChar()));
	}
	szString.append(L"\n");
	for (iI = 0; iI < GC.getNumBonusInfos(); ++iI)
		szString.append(CvWString::format(L"%c", GC.getBonusInfo((BonusTypes) iI).getChar()));
	for (iI=0; iI<MAX_NUM_SYMBOLS; ++iI)
		szString.append(CvWString::format(L"%c", gDLL->getSymbolID(iI)));
}

void CvGameTextMgr::setPlotHelp(CvWStringBuffer& szString, CvPlot* pPlot)
{
	bool bShift = GC.shiftKey();
	// <advc.135c>
	if(GC.getGame().isDebugMode()) {
		setPlotHelpDebug(szString, *pPlot);
		if(bShift || GC.ctrlKey() || GC.altKey())
			return;
	} // </advc.135c>

	CvGame const& g = GC.getGame(); // advc.003
	TeamTypes eActiveTeam = g.getActiveTeam();
	PlayerTypes eActivePlayer = g.getActivePlayer();
	CvWString szTempBuffer;

	PlayerTypes eRevealedOwner = pPlot->getRevealedOwner(eActiveTeam, true);
	if (eRevealedOwner != NO_PLAYER
			// advc.099f:
			|| bShift || getBugOptionBOOL("MiscHover__CultureInUnownedTiles", false))
	{
		if (pPlot->isActiveVisible(true))
		{	/*  <advc.101> Similar to code added in
				CvDLLWidgetData::parseNationalityHelp */
			if(pPlot->isCity()) {
				CvCity const& c = *pPlot->getPlotCity();
				double prRevolt = c.revoltProbability();
				// <advc.023>
				double prDecrement = c.probabilityOccupationDecrement();
				prRevolt *= 1 - prDecrement; // </advc.023>
				if(prRevolt > 0) {
					/*  CvCity::revoltProbability rounds probabilities that are too
						small to display to 0, but that doesn't take into account
						prDecrement, so prRevolt here can still be less than
						1 permille -- though not much less, so this isn't going to
						overstate the probability much: */
					prRevolt = std::max(0.001, prRevolt);
					wchar floatBuffer[1024];
					swprintf(floatBuffer, L"%.1f", (float)(100 * prRevolt));
					szString.append(gDLL->getText("TXT_KEY_MISC_CHANCE_OF_REVOLT",
							floatBuffer));
					int iPriorRevolts = c.getNumRevolts();
					if(c.canCultureFlip(c.calculateCulturalOwner())) {
						szString.append(L" (");
						szString.append(gDLL->getText("TXT_KEY_MISC_WILL_FLIP"));
						szString.append(L")");
					} else if(iPriorRevolts > 0) {
						szString.append(L" (");
						szString.append(gDLL->getText("TXT_KEY_MISC_PRIOR", iPriorRevolts));
						szString.append(L")");
					}
					szString.append(NEWLINE);
				} // <advc.023>
				if(prDecrement > 0) {
					wchar floatBuffer[1024];
					swprintf(floatBuffer, L"%.1f", (float)(100 * prDecrement));
					szString.append(gDLL->getText("TXT_KEY_OCCUPATION_DECREASE_CHANCE",
							floatBuffer));
					szString.append(NEWLINE);
				} // </advc.023>
			} // </advc.101>
			// <advc.099g> Put the players w/ tile culture in a container first
			std::vector<std::pair<int,PlayerTypes> > aieCulturePerPlayer;
			/*  Go backwards b/c of the stable_sort call below. Removing that call
				will restore the BtS ordering (except for the special treatment
				of eActivePlayer). */
			for (int i = MAX_PLAYERS - 1; i >= 0; i--)
			{
				if (i == eRevealedOwner || i == eActivePlayer) // Prepended below
					continue; // </advc.099g>
				CvPlayer const& kPlayer = GET_PLAYER((PlayerTypes)i);
				// advc.099: Replaced "Alive" with "EverAlive"
				if (kPlayer.isEverAlive() && pPlot->getCulture(kPlayer.getID()) > 0)
				{
					int iCulture = pPlot->calculateCulturePercent(kPlayer.getID());
					/*  K-Mod, 29/sep/10, Karadoc
						Prevent display of 0% culture, to reduce the spam created by trade culture. */
					if (iCulture >= 1)
					{	// advc.099g:
						aieCulturePerPlayer.push_back(std::make_pair(iCulture, kPlayer.getID()));
					}
				}
			} // <advc.099g>
			std::stable_sort(aieCulturePerPlayer.begin(), aieCulturePerPlayer.end());
			if(eActivePlayer != eRevealedOwner) {
				int iCulture = pPlot->calculateCulturePercent(eActivePlayer);
				if(iCulture >= 1)
					aieCulturePerPlayer.push_back(std::make_pair(iCulture, eActivePlayer));
			}
			if(eRevealedOwner != NO_PLAYER) // advc.099f
			{
				aieCulturePerPlayer.push_back(std::make_pair(
						pPlot->calculateCulturePercent(eRevealedOwner), eRevealedOwner));
			}
			std::reverse(aieCulturePerPlayer.begin(), aieCulturePerPlayer.end());
			for (size_t i = 0; i < aieCulturePerPlayer.size(); i++)
			{
				CvPlayer const& kPlayer = GET_PLAYER(aieCulturePerPlayer[i].second);
				int iCulture = aieCulturePerPlayer[i].first;
				// </advc.099g>
				szTempBuffer.Format(L"%d%% " SETCOLR L"%s" ENDCOLR, iCulture,
						PLAYER_TEXT_COLOR(kPlayer), kPlayer.getCivilizationAdjective());
				szString.append(szTempBuffer);
				szString.append(NEWLINE);
				// <advc.099g> Put it all on one line? I guess better not.
				//setListHelp(szString, L"", szTempBuffer.GetCString(), L", ", i == 0);
			}
			/*if(!aieCulturePerPlayer.empty())
				szString.append(NEWLINE);*/
			// </advc.099g>
		}
		else if(eRevealedOwner != NO_PLAYER) // advc.099f
		{
			CvPlayer const& kRevealOwner = GET_PLAYER(eRevealedOwner);
			szTempBuffer.Format(SETCOLR L"%s" ENDCOLR,
					PLAYER_TEXT_COLOR(kRevealOwner), kRevealOwner.getCivilizationDescription());
			szString.append(szTempBuffer);
			szString.append(NEWLINE);
		}
	}
	CvUnit* pHeadSelectedUnit = gDLL->getInterfaceIFace()->getHeadSelectedUnit(); // advc.003
	// <advc.012>
	TeamTypes eDefTeam = (eRevealedOwner != NO_PLAYER ?
			GET_PLAYER(eRevealedOwner).getTeam() :
			NO_TEAM);
	int iDefWithFeatures = pPlot->defenseModifier(eDefTeam, true, NO_TEAM, true);
	int iDefWithoutFeatures = pPlot->defenseModifier(eDefTeam, true,
			pPlot->getTeam(), true);
	int iDelta = iDefWithFeatures - iDefWithoutFeatures;
	if(iDefWithoutFeatures != 0 || iDefWithFeatures != 0) {
		if(iDefWithoutFeatures != 0) {
			szString.append(gDLL->getText("TXT_KEY_PLOT_BONUS", iDefWithoutFeatures));
			if(iDelta != 0)
				szString.append(L", ");
		}
		if(iDelta != 0) {
			if(iDefWithoutFeatures == 0)
				szString.append(gDLL->getText("TXT_KEY_TILE_RIVALDEF"));
			szString.append(gDLL->getText("TXT_KEY_FEATURE_RIVALDEF", iDelta));
		}
		szString.append(NEWLINE);
	} // </advc.012>

	if (pPlot->getTerrainType() != NO_TERRAIN)
	{
		if (pPlot->isPeak())
		{
			szString.append(gDLL->getText("TXT_KEY_PLOT_PEAK"));
		}
		else
		{
			if (pPlot->isWater())
			{
				szTempBuffer.Format(SETCOLR, TEXT_COLOR("COLOR_WATER_TEXT"));
				szString.append(szTempBuffer);
			}

			if (pPlot->isHills())
			{
				szString.append(gDLL->getText("TXT_KEY_PLOT_HILLS"));
			}

			if (pPlot->getFeatureType() != NO_FEATURE)
			{
				szTempBuffer.Format(L"%s/", GC.getFeatureInfo(pPlot->getFeatureType()).getDescription());
				szString.append(szTempBuffer);
			}

			szString.append(GC.getTerrainInfo(pPlot->getTerrainType()).getDescription());

			if (pPlot->isWater())
			{
				szString.append(ENDCOLR);
			}
		}
	}
	/*  advc.001w: hasYield is based on getYield, which is not always consistent
		with calculateYield. */
	//if (pPlot->hasYield())
	{
		for (int iI = 0; iI < NUM_YIELD_TYPES; ++iI)
		{
			int iYield = pPlot->calculateYield(((YieldTypes)iI), true);

			if (iYield != 0)
			{
				szTempBuffer.Format(L", %d%c", iYield, GC.getYieldInfo((YieldTypes) iI).getChar());
				szString.append(szTempBuffer);
			}
		}
	}

	if (pPlot->isFreshWater())
	{
		szString.append(NEWLINE);
		szString.append(gDLL->getText("TXT_KEY_PLOT_FRESH_WATER"));
		// <advc.004b>
		if(pHeadSelectedUnit != NULL &&
			// advc.004h:
			pHeadSelectedUnit->canFound() && pHeadSelectedUnit->atPlot(pPlot)) {
			szTempBuffer = CvWString::format(L" +%d%c",
					GC.getDefineINT("FRESH_WATER_HEALTH_CHANGE"),
					gDLL->getSymbolID(HEALTHY_CHAR));
			szString.append(szTempBuffer);
		} // </advc.004b>
	}

	if (pPlot->isLake())
	{
		szString.append(NEWLINE);
		szString.append(gDLL->getText("TXT_KEY_PLOT_FRESH_WATER_LAKE"));
	}

	if (pPlot->isImpassable())
	{
		szString.append(NEWLINE);
		szString.append(gDLL->getText("TXT_KEY_PLOT_IMPASSABLE"));
	}
	// <advc.004b>
	if(pHeadSelectedUnit != NULL &&
			// advc.004h:
			pHeadSelectedUnit->canFound() && pPlot->getFeatureType() != NO_FEATURE) {
		int iHealthPercent = GC.getFeatureInfo(pPlot->getFeatureType()).
				getHealthPercent();
		if(iHealthPercent != 0) {
			szString.append(NEWLINE);
			float health = abs(iHealthPercent / 100.0f);
			int iIcon = (iHealthPercent > 0 ? gDLL->getSymbolID(HEALTHY_CHAR) :
						gDLL->getSymbolID(UNHEALTHY_CHAR));
			if(iHealthPercent % 10 == 0)
				szTempBuffer.Format(L"+%.1f%c", health, iIcon);
			else szTempBuffer.Format(L"+%.2f%c", health, iIcon);
		}
		szString.append(szTempBuffer);
	} // </advc.004b>
	// <advc.001> Moved up. BtS was using getImprovementType in the eBonus code
	ImprovementTypes ePlotImprovement = pPlot->getRevealedImprovementType(
			eActiveTeam, true); // </advc.001>
	BonusTypes eBonus = NO_BONUS;
	if (g.isDebugMode())
	{
		eBonus = pPlot->getBonusType();
	}
	else
	{
		eBonus = pPlot->getBonusType(eActiveTeam);
	}
	if (eBonus != NO_BONUS)
	{
		szTempBuffer.Format(L"%c " SETCOLR L"%s" ENDCOLR, GC.getBonusInfo(eBonus).getChar(), TEXT_COLOR("COLOR_HIGHLIGHT_TEXT"), GC.getBonusInfo(eBonus).getDescription());

		szString.append(NEWLINE);
		szString.append(szTempBuffer);

		// K-Mod. I've rearranged and edited some of the code in this section to fix some bugs.
		// (for example, obsolete bonuses shouldn't say "requires x improvement"; and neither should bonuses that are already connected with a fort.)
		if (!GET_TEAM(eActiveTeam).isBonusObsolete(eBonus))
		{
			bool bFirst = true; // advc.047
			if (GC.getBonusInfo(eBonus).getHealth() != 0)
			{	// <advc.047>
				if(bFirst) {
					szString.append(L" ");
					bFirst = false;
				}
				else // </advc.047>
					szString.append(L", ");
				szTempBuffer.Format(L"+%d%c", abs(GC.getBonusInfo(eBonus).getHealth()), ((GC.getBonusInfo(eBonus).getHealth() > 0) ? gDLL->getSymbolID(HEALTHY_CHAR): gDLL->getSymbolID(UNHEALTHY_CHAR)));
				szString.append(szTempBuffer);
			}

			if (GC.getBonusInfo(eBonus).getHappiness() != 0)
			{	// <advc.047>
				if(bFirst) {
					szString.append(L" ");
					bFirst = false;
				}
				else // </advc.047>
					szString.append(L", ");
				szTempBuffer.Format(L"+%d%c", abs(GC.getBonusInfo(eBonus).getHappiness()), ((GC.getBonusInfo(eBonus).getHappiness() > 0) ? gDLL->getSymbolID(HAPPY_CHAR): gDLL->getSymbolID(UNHAPPY_CHAR)));
				szString.append(szTempBuffer);
			}
			CvWStringBuffer szReqs; // advc.047
			if (!GET_TEAM(eActiveTeam).isHasTech((TechTypes)GC.getBonusInfo(eBonus).getTechCityTrade()))
			{
				/*szString.append(gDLL->getText("TXT_KEY_PLOT_RESEARCH",
					GC.getTechInfo((TechTypes)GC.getBonusInfo(eBonus).
					getTechCityTrade()).getTextKeyWide())); */
				// <advc.047>
				szTempBuffer.Format(SETCOLR L"%s" ENDCOLR, TEXT_COLOR("COLOR_WARNING_TEXT"),
						GC.getTechInfo((TechTypes)GC.getBonusInfo(eBonus).
						getTechCityTrade()).getText());
				szReqs.append(szTempBuffer);
			}
			bool bCity = pPlot->isCity();
			bool bConnected = GET_PLAYER(eActivePlayer).
					doesImprovementConnectBonus(ePlotImprovement, eBonus);
			// Moved up (b/c the route isn't needed for the yields)
			if(!bCity && !pPlot->isBonusNetwork(eActiveTeam) && !pPlot->isWater() &&
					bConnected) { // Mention route only if all other pieces in place
				//szString.append(gDLL->getText("TXT_KEY_PLOT_REQUIRES_ROUTE"));
				if(!szReqs.isEmpty())
					szReqs.append(L", ");
				szTempBuffer.Format(SETCOLR L"%s" ENDCOLR, TEXT_COLOR("COLOR_WARNING_TEXT"),
						gDLL->getText("TXT_KEY_ROUTE_ROAD").GetCString());
				szReqs.append(szTempBuffer);
			}
			bool bReqsDone = false;
			// </advc.047>
			if(!bCity)
			{
				if (!bConnected ||
						// <advc.047> Show what replacing Fort would yield
						(ePlotImprovement != NULL && GC.getImprovementInfo(
						ePlotImprovement).isActsAsCity())) // </advc.047>
				{
					/*  K-Mod note: unfortunately, the following code assumes that
						there is only one improvement marked "isImprovementBonusTrade"
						for each bonus. */
					/*  advc (comment): ... and it assumes that the most important
						one has the lowest id */
					for (int iI = 0; iI < GC.getNumBuildInfos(); ++iI)
					{
						ImprovementTypes eLoopImprov = (ImprovementTypes)GC.getBuildInfo(
								(BuildTypes)iI).getImprovement();
						if(eLoopImprov == NO_IMPROVEMENT ||
								/*  advc.047: Show only missing improvement;
									!bConnected doesn't ensure this when the
									city-trade tech is missing. */
								eLoopImprov == ePlotImprovement)
							continue;
						CvImprovementInfo& kImprovementInfo = GC.getImprovementInfo(
								(ImprovementTypes)GC.getBuildInfo((BuildTypes)iI).
								getImprovement());
						if(!kImprovementInfo.isImprovementBonusTrade(eBonus) ||
								!pPlot->canHaveImprovement((ImprovementTypes)
								GC.getBuildInfo((BuildTypes) iI).getImprovement(),
								eActiveTeam, true))
							continue;
						if(!bConnected && eRevealedOwner != NO_PLAYER && // advc.047
								GET_TEAM(eActiveTeam).isHasTech(
								(TechTypes)GC.getBuildInfo((BuildTypes)iI).
								getTechPrereq())) {
							/*szString.append(gDLL->getText("TXT_KEY_PLOT_REQUIRES",
									kImprovementInfo.getTextKeyWide()));*/
							// <advc.047> Add to req list instead
							if(!szReqs.isEmpty())
								szReqs.append(L", ");
							szTempBuffer.Format(SETCOLR L"%s" ENDCOLR, TEXT_COLOR(
									"COLOR_WARNING_TEXT"), kImprovementInfo.getText());
							szReqs.append(szTempBuffer);
							// </advc.047>
						}
						/*else if (GC.getBonusInfo(eBonus).getTechCityTrade() !=
								GC.getBuildInfo((BuildTypes) iI).getTechPrereq()) {
							szString.append(gDLL->getText("TXT_KEY_PLOT_RESEARCH",
									GC.getTechInfo((TechTypes) GC.getBuildInfo(
									(BuildTypes)iI).getTechPrereq()).getTextKeyWide()));
						}*/ // <advc.047>
						// ^Too complicated. Player will have to look the tech up.
						if(!szReqs.isEmpty()) {
							szString.append(L" (");
							szString.append(gDLL->getText("TXT_KEY_WITH") + L" ");
							szString.append(szReqs);
							szString.append(L")");
							bReqsDone = true;
						} // </advc.047>
						bFirst = true;
						for (int k = 0; k < NUM_YIELD_TYPES; k++)
						{
							int iYieldChange = kImprovementInfo.
									getImprovementBonusYield(eBonus, k) +
									kImprovementInfo.getYieldChange(k)
									/*  <advc.047> If there's already an improvement,
										show the difference between the yields. */
									- (ePlotImprovement == NO_IMPROVEMENT ||
										ePlotImprovement == eLoopImprov ? 0 :
									pPlot->calculateImprovementYieldChange(
									ePlotImprovement, (YieldTypes)k, eActivePlayer));
									// </advc.047>
							if (iYieldChange != 0)
							{
								if (iYieldChange > 0)
								{
									szTempBuffer.Format(L"+%d%c", iYieldChange, GC.getYieldInfo((YieldTypes)k).getChar());
								}
								else
								{
									szTempBuffer.Format(L"%d%c", iYieldChange, GC.getYieldInfo((YieldTypes)k).getChar());
								}
								setListHelp(szString, L"\n", szTempBuffer, L", ", bFirst);
								bFirst = false;
							}
						}
						if (!bFirst)
						{ // <advc.047>
						  // In red unless already shown red text about unconnected bonus
							CvWString szTag = L"TXT_KEY_BONUS_WITH_IMPROVEMENT";
							if(szReqs.isEmpty())
								szTag += L"_WARNING"; // </advc.047>
							szString.append(gDLL->getText(szTag, kImprovementInfo.getTextKeyWide()));
						}
						break;
					}
				}
				// advc.047: PLOT_REQUIRES_ROUTE code moved up
			}
			// <advc.047> Duplicate code; sorry.
			if(!bReqsDone && !szReqs.isEmpty()) {
				szString.append(L" (");
				szString.append(gDLL->getText("TXT_KEY_WITH") + L" ");
				szString.append(szReqs);
				szString.append(L")");
				bReqsDone = true;
			} // </advc.047>

			// K-Mod note: I'm not sure whether or not the help should be displayed when the bonus is obsolete.
			// The fact is, none of the bonuses have help text in K-Mod (or standard bts.)
			if (!CvWString(GC.getBonusInfo(eBonus).getHelp()).empty())
			{
				szString.append(NEWLINE);
				szString.append(GC.getBonusInfo(eBonus).getHelp());
			}
		} // end !isBonusObsolete
		// <advc.050>
		CvInitCore const& ic = GC.getInitCore();
		if(GC.altKey()) { /* [Alt] is also for combat odds. That takes
				precedence. This function isn't even called when ALT is
				pressed on a hostile unit. (Check [Ctrl] instead?) */
			CvWString szMapName = ic.getMapScriptName();
			int pos = szMapName.find(L"."); // Drop the file extension
			if(pos != CvWString::npos && pos >= 2)
				szMapName = szMapName.substr(0, pos);
			std::transform(szMapName.begin(), szMapName.end(),
					szMapName.begin(), ::toupper); // to upper case
			std::wostringstream ssKey;
			ssKey << L"TXT_KEY_" << szMapName << L"_" <<
					pPlot->getX() << L"_" << pPlot->getY() << L"_BONUS_";
			CvWString szBonusName = GC.getBonusInfo(eBonus).getDescription();
			std::transform(szBonusName.begin(), szBonusName.end(),
					szBonusName.begin(), ::toupper);
			// Tolerate plural errors
			if(szBonusName.at(szBonusName.length() - 1) == L'S')
				szBonusName = szBonusName.substr(0, szBonusName.length() - 1);
			ssKey << szBonusName;
			CvWString szKey = ssKey.str();
			CvWString szHistoryText = gDLL->getText(szKey);
			// getText returns txtKey if it can't find the text key
			if(szHistoryText.compare(szKey) == 0) {
				szKey.append(L"s");
				szHistoryText = gDLL->getText(szKey);
			}
			if(szHistoryText.compare(szKey) != 0) {
				szString.append(NEWLINE);
				szString.append(NEWLINE);
				szString.append(CvWString::format(SETCOLR,
						TEXT_COLOR("COLOR_HIGHLIGHT_TEXT")));
				szString.append(szHistoryText);
				szString.append(CvWString::format( ENDCOLR));
			}
		} // </advc.050>
	}

	if (ePlotImprovement != NO_IMPROVEMENT)
	{
		szString.append(NEWLINE);
		// <advc.005c>
		bool bNamedRuin = false;
		if(ePlotImprovement == GC.getRUINS_IMPROVEMENT()) {
			const wchar* szRuinsName = pPlot->getRuinsName();
			if(wcslen(szRuinsName) > 0) {
				szString.append(gDLL->getText("TXT_KEY_IMPROVEMENT_CITY_RUINS_NAMED",
						szRuinsName));
				bNamedRuin = true;
			}
		}
		if(!bNamedRuin) // from Volcano event // </advc.005c>
			szString.append(GC.getImprovementInfo(ePlotImprovement).getDescription());

		bool bFound = false;

		for (int iI = 0; iI < NUM_YIELD_TYPES; ++iI)
		{
			if (GC.getImprovementInfo(ePlotImprovement).getIrrigatedYieldChange(iI) != 0)
			{
				bFound = true;
				break;
			}
		}

		if (bFound)
		{
			if (pPlot->isIrrigationAvailable())
			{
				szString.append(gDLL->getText("TXT_KEY_PLOT_IRRIGATED"));
			}
			else
			{
				szString.append(gDLL->getText("TXT_KEY_PLOT_NOT_IRRIGATED"));
			}
		}

		if (GC.getImprovementInfo(ePlotImprovement).getImprovementUpgrade() != NO_IMPROVEMENT)
		{
			if ((pPlot->getUpgradeProgress() > 0) || pPlot->isBeingWorked())
			{
				int iTurns = pPlot->getUpgradeTimeLeft(ePlotImprovement, eRevealedOwner);

				szString.append(gDLL->getText("TXT_KEY_PLOT_IMP_UPGRADE", iTurns, GC.getImprovementInfo((ImprovementTypes) GC.getImprovementInfo(ePlotImprovement).getImprovementUpgrade()).getTextKeyWide()));
			}
			else
			{
				szString.append(gDLL->getText("TXT_KEY_PLOT_WORK_TO_UPGRADE", GC.getImprovementInfo((ImprovementTypes) GC.getImprovementInfo(ePlotImprovement).getImprovementUpgrade()).getTextKeyWide()));
			}
		}
	}

	if (pPlot->getRevealedRouteType(eActiveTeam, true) != NO_ROUTE)
	{
		szString.append(NEWLINE);
		szString.append(GC.getRouteInfo(pPlot->getRevealedRouteType(eActiveTeam, true)).getDescription());
	}
	// <advc.011c>
	for(int i = 0; i < GC.getNumBuildInfos(); i++) {
		BuildTypes eBuild = (BuildTypes)i;
		if(pPlot->getBuildProgress(eBuild) <= 0
				|| !pPlot->canBuild(eBuild, eActivePlayer))
			continue;
		int iTurnsLeft = pPlot->getBuildTurnsLeft(eBuild, eActivePlayer);
		if(iTurnsLeft <= 0 || iTurnsLeft == MAX_INT)
			continue; // </advc.011c>
		// <advc.011b>
		int iInitialTurnsNeeded = (int)::ceil(
				pPlot->getBuildTime(eBuild, eActivePlayer) /
				(double)GET_PLAYER(eActivePlayer).getWorkRate(eBuild));
		int iTurnsSpent = iInitialTurnsNeeded - iTurnsLeft;
		if(iTurnsSpent <= 0)
			continue;
		CvBuildInfo const& kBuild = GC.getBuildInfo(eBuild);
		CvWString szBuildDescr = kBuild.getDescription();
		/*  Nicer to use the structure (improvement or route) name if it isn't a
			build that only removes a feature. */
		ImprovementTypes eImprovement = (ImprovementTypes)kBuild.getImprovement();
		if(eImprovement != NO_IMPROVEMENT)
			szBuildDescr = GC.getImprovementInfo(eImprovement).getDescription();
		else {
			RouteTypes eRoute = (RouteTypes)kBuild.getRoute();
			if(eRoute != NO_ROUTE)
				szBuildDescr = GC.getRouteInfo(eRoute).getDescription();
		}
		szTempBuffer.Format(SETCOLR L"%s" ENDCOLR,
				TEXT_COLOR("COLOR_HIGHLIGHT_TEXT"),
				szBuildDescr.c_str());
		szBuildDescr = szTempBuffer;
		bool bDecay = (GC.getDELAY_UNTIL_BUILD_DECAY() > 0 &&
				pPlot->decayBuildProgress(true));
		if(bDecay) { // Check if Workers are getting on the task this turn
			CLLNode<IDInfo>* pUnitNode = pPlot->headUnitNode();
			while(pUnitNode != NULL) {
				CvUnit* pUnit = ::getUnit(pUnitNode->m_data);
				pUnitNode = pPlot->nextUnitNode(pUnitNode);
				if(pUnit->getTeam() == eActiveTeam &&
						pUnit->movesLeft() > 0 &&
						pUnit->getGroup()->getMissionType(0) == MISSION_BUILD) {
					bDecay = false;
					break;
				}
			}
		}
		if(GC.ctrlKey() || getBugOptionBOOL("MiscHover__PartialBuildsAlways", false)) {
			szString.append(NEWLINE);
			szString.append(szBuildDescr);
			szString.append(CvWString::format(L" (%d/%d %s%s)",
					iTurnsSpent, iInitialTurnsNeeded, gDLL->getText("TXT_KEY_REPLAY_SCREEN_TURNS").c_str(),
					bDecay ? CvWString::format(L"; " SETCOLR L"%s" ENDCOLR,
					TEXT_COLOR("COLOR_WARNING_TEXT"),
					gDLL->getText("TXT_KEY_MISC_DECAY_WARNING").c_str()).c_str() : L""));
		}
	} // </advc.011b>

	// advc.003h (BBAI code from 07/11/08 by jdog5000 moved into setPlotHelpDebug)
	if (pPlot->getBlockadedCount(eActiveTeam) > 0)
	{
		szString.append(CvWString::format(SETCOLR, TEXT_COLOR("COLOR_NEGATIVE_TEXT")));
		szString.append(NEWLINE);
		szString.append(gDLL->getText("TXT_KEY_PLOT_BLOCKADED"));
		szString.append(CvWString::format( ENDCOLR));
	}

	if (pPlot->getFeatureType() != NO_FEATURE)
	{
		int iDamage = GC.getFeatureInfo(pPlot->getFeatureType()).getTurnDamage();

		if (iDamage > 0)
		{
			szString.append(CvWString::format(SETCOLR, TEXT_COLOR("COLOR_NEGATIVE_TEXT")));
			szString.append(NEWLINE);
			szString.append(gDLL->getText("TXT_KEY_PLOT_DAMAGE", iDamage));
			szString.append(CvWString::format( ENDCOLR));
		}
		// UNOFFICIAL_PATCH, User interface (FeatureDamageFix), 06/02/10, LunarMongoose: START
		else if (iDamage < 0)
		{
			szString.append(CvWString::format(SETCOLR, TEXT_COLOR("COLOR_POSITIVE_TEXT")));
			szString.append(NEWLINE);
			szString.append(gDLL->getText("TXT_KEY_PLOT_DAMAGE", iDamage));
			szString.append(CvWString::format( ENDCOLR));
		} // UNOFFICIAL_PATCH: END
	}
}

// advc.135c:
void CvGameTextMgr::setPlotHelpDebug(CvWStringBuffer& szString, CvPlot const& kPlot) {

	//if (bCtrl && (gDLL->getChtLvl() > 0))
	// <advc.135c>
	if(GC.ctrlKey()) {
		setPlotHelpDebug_Ctrl(szString, kPlot);
		return;
	} // </advc.135c>

	bool bShift = GC.shiftKey();
	bool bAlt = GC.altKey();

	//if (bShift && !bAlt && (gDLL->getChtLvl() > 0))
	// <advc.135c>
	if(bShift && !bAlt) {
		setPlotHelpDebug_ShiftOnly(szString, kPlot);
		return;
	} // </advc.135c>

	//if (!bShift && bAlt && (gDLL->getChtLvl() > 0))
	// <advc.135c>
	if(!bShift && bAlt) {
		setPlotHelpDebug_AltOnly(szString, kPlot);
		return;
	} // </advc.135c>

	//if (bShift && bAlt && (gDLL->getChtLvl() > 0))
	// <advc.135c>
	if(bShift && bAlt) {
		setPlotHelpDebug_ShiftAltOnly(szString, kPlot);
		return;
	} // </advc.135c>
}

// advc.135c: Cut and pasted from setPlotHelp
void CvGameTextMgr::setPlotHelpDebug_Ctrl(CvWStringBuffer& szString, CvPlot const& kPlot)
{
	bool bShift = GC.shiftKey();
	if (bShift && kPlot.headUnitNode() != NULL)
		return;

	bool bAlt = GC.altKey();
	int x = kPlot.getX();
	int y = kPlot.getY();
	CvWString szTempBuffer;
	CvGame const& g = GC.getGame();
	bool bConstCache = g.isNetworkMultiPlayer(); // advc.001n

	if (kPlot.getOwner() != NO_PLAYER
			/*  advc.001n: AI_getPlotDanger calls setActivePlayerSafeRangeCache,
				which may not be a safe thing to do in multiplayer. */
			&& !bConstCache)
	{
		int iPlotDanger = GET_PLAYER(kPlot.getOwner()).AI_getPlotDanger(
				const_cast<CvPlot*>(&kPlot), /* advc.135c: */ 2);
		if (iPlotDanger > 0)
			szString.append(CvWString::format(L"\nPlot Danger = %d", iPlotDanger));
	}

	CvCity* pPlotCity = kPlot.getPlotCity();
	if (pPlotCity != NULL)
	{
		PlayerTypes ePlayer = kPlot.getOwner();
		CvPlayerAI& kPlayer = GET_PLAYER(ePlayer);

		int iCityDefenders = kPlot.plotCount(PUF_canDefendGroupHead, -1, -1, ePlayer, NO_TEAM, PUF_isCityAIType);
		int iAttackGroups = kPlot.plotCount(PUF_isUnitAIType, UNITAI_ATTACK, -1, ePlayer);
		szString.append(CvWString::format(L"\nDefenders [D+A]/N ([%d + %d] / %d)",
				iCityDefenders, iAttackGroups, pPlotCity->AI_neededDefenders(
				false, true))); // advc.001n
		szString.append(CvWString::format(L"\nFloating Defenders H/N (%d / %d)",
				kPlayer.AI_getTotalFloatingDefenders(pPlotCity->area()),
				kPlayer.AI_getTotalFloatingDefendersNeeded(pPlotCity->area())));
		szString.append(CvWString::format(L"\nAir Defenders H/N (%d / %d)",
				pPlotCity->plot()->plotCount(PUF_canAirDefend, -1, -1,
				pPlotCity->getOwner(), NO_TEAM, PUF_isDomainType, DOMAIN_AIR),
				pPlotCity->AI_neededAirDefenders(/* advc.001n: */ true)));
//		int iHostileUnits = kPlayer.AI_countNumAreaHostileUnits(pPlotCity->area());
//		if (iHostileUnits > 0)
//			szString+=CvWString::format(L"\nHostiles = %d", iHostileUnits);

		szString.append(CvWString::format(L"\nThreat C/P (%d / %d)",
				pPlotCity->AI_cityThreat(), kPlayer.AI_getTotalAreaCityThreat(pPlotCity->area())));

		bool bFirst = true;
		for (int iI = 0; iI < MAX_CIV_PLAYERS; iI++) // advc.003n: was MAX_PLAYERS
		{
			PlayerTypes eLoopPlayer = (PlayerTypes)iI;
			CvPlayerAI& kLoopPlayer = GET_PLAYER(eLoopPlayer);
			if(eLoopPlayer == ePlayer || !kLoopPlayer.isAlive())
				continue;
			// BETTER_BTS_AI_MOD, DEBUG, 06/16/08, jdog5000: START
			// original code (advc: deleted)
			if(pPlotCity->isCapital())
			{
				int iCloseness = pPlotCity->AI_playerCloseness(eLoopPlayer, DEFAULT_PLAYER_CLOSENESS,
						true); // advc.001n
				int iPlayerCloseness = kPlayer.AI_playerCloseness(eLoopPlayer, DEFAULT_PLAYER_CLOSENESS,
						true); // advc.001n
				if(GET_TEAM(kPlayer.getTeam()).isHasMet(kLoopPlayer.getTeam()) || iPlayerCloseness != 0)
				{
					if(bFirst)
					{
						bFirst = false;
						szString.append(CvWString::format(L"\n\nCloseness + War: (in %d wars)", GET_TEAM(kPlayer.getTeam()).getAtWarCount(true)));
					}
					szString.append(CvWString::format(L"\n%s(%d) : %d ", kLoopPlayer.getName(), DEFAULT_PLAYER_CLOSENESS, iCloseness));
					szString.append(CvWString::format(L" [%d, ", iPlayerCloseness));
					if(kPlayer.getTeam() != kLoopPlayer.getTeam())
					{
						szString.append(CvWString::format(L"%d]", GET_TEAM(kPlayer.
								getTeam()).AI_teamCloseness(kLoopPlayer.getTeam(), DEFAULT_PLAYER_CLOSENESS,
								false, true))); // advc.001n
					/*  advc.001n: Only relevant for the K-Mod war AI, and I'm not totally sure that
						CvTeamAI::AI_startWarVal is safe for networked games
						(despite the bConstCache param that I've added). */
						if(!getWPAI.isEnabled()) {
							if(GET_TEAM(kPlayer.getTeam()).isHasMet(kLoopPlayer.getTeam()) &&
									GET_TEAM(kPlayer.getTeam()).AI_getAttitude(kLoopPlayer.getTeam()) != ATTITUDE_FRIENDLY)
							{
								int iStartWarVal = GET_TEAM(kPlayer.getTeam()).AI_startWarVal(kLoopPlayer.getTeam(), WARPLAN_TOTAL,
										true); // advc.001n
								if(GET_TEAM(kPlayer.getTeam()).isAtWar(kLoopPlayer.getTeam()))
									szString.append(CvWString::format(L"\n   At War:   "));
								else if (GET_TEAM(kPlayer.getTeam()).AI_getWarPlan(kLoopPlayer.getTeam()) != NO_WARPLAN)
									szString.append(CvWString::format(L"\n   Plan. War:"));
								else  if(!GET_TEAM(kPlayer.getTeam()).canDeclareWar(kLoopPlayer.getTeam()))
									szString.append(CvWString::format(L"\n   Can't War:"));
								else szString.append(CvWString::format(L"\n   No War:   "));
								if(iStartWarVal > 1200)
									szString.append(CvWString::format(SETCOLR L" %d" ENDCOLR, TEXT_COLOR("COLOR_RED"), iStartWarVal));
								else if(iStartWarVal > 600)
									szString.append(CvWString::format(SETCOLR L" %d" ENDCOLR, TEXT_COLOR("COLOR_YELLOW"), iStartWarVal));
								else szString.append(CvWString::format(L" %d", iStartWarVal));
								// advc.003j: Unused
								/*szString.append(CvWString::format(L" (%d", GET_TEAM(kPlayer.getTeam()).AI_calculatePlotWarValue(kLoopPlayer.getTeam())));
								szString.append(CvWString::format(L", %d", GET_TEAM(kPlayer.getTeam()).AI_calculateBonusWarValue(kLoopPlayer.getTeam())));
								szString.append(CvWString::format(L", %d", GET_TEAM(kPlayer.getTeam()).AI_calculateCapitalProximity(kLoopPlayer.getTeam())));*/
								szString.append(CvWString::format(L", %4s", GC.getAttitudeInfo(GET_TEAM(kPlayer.getTeam()).AI_getAttitude(kLoopPlayer.getTeam())).getDescription(0)));
								szString.append(CvWString::format(L", %d%%)", 100-GET_TEAM(kPlayer.getTeam()).AI_noWarAttitudeProb(GET_TEAM(kPlayer.getTeam()).AI_getAttitude(kLoopPlayer.getTeam()))));
							}
						}
					}
					else szString.append(CvWString::format(L"-]"));
				}
			}
			else
			{
				int iCloseness = pPlotCity->AI_playerCloseness(eLoopPlayer, DEFAULT_PLAYER_CLOSENESS,
						true); // advc.001n
				if (iCloseness != 0)
				{
					if (bFirst)
					{
						bFirst = false;
						szString.append(CvWString::format(L"\n\nCloseness:"));
					}
					szString.append(CvWString::format(L"\n%s(%d) : %d ", kLoopPlayer.getName(), DEFAULT_PLAYER_CLOSENESS, iCloseness));
					szString.append(CvWString::format(L" [%d, ", kPlayer.AI_playerCloseness(eLoopPlayer, DEFAULT_PLAYER_CLOSENESS,
							true))); // advc.001n
					if (kPlayer.getTeam() != kLoopPlayer.getTeam())
					{
						szString.append(CvWString::format(L"%d]", GET_TEAM(kPlayer.getTeam()).
								AI_teamCloseness( kLoopPlayer.getTeam(), DEFAULT_PLAYER_CLOSENESS,
								false, true))); // advc.001n
					}
					else szString.append(CvWString::format(L"-]"));
				}
			}
			// BETTER_BTS_AI_MOD: END
		}

		int iWorkersHave = pPlotCity->AI_getWorkersHave();
		int iWorkersNeeded = pPlotCity->AI_getWorkersNeeded();
		szString.append(CvWString::format(L"\n\nWorkers H/N (%d , %d)", iWorkersHave, iWorkersNeeded));
		int iWorkBoatsNeeded = pPlotCity->AI_neededSeaWorkers();
		szString.append(CvWString::format(L"\n\nWorkboats Needed = %d", iWorkBoatsNeeded));
		/*  <advc.001n> AI_getNumAreaCitySites and AI_getNumAdjacentAreaCitySites
			call CvPlot::getFoundValue, which may cache its result. */
		if(!bConstCache) {
			int iAreaSiteBestValue = 0;
			int iNumAreaCitySites = kPlayer.AI_getNumAreaCitySites(kPlot.getArea(), iAreaSiteBestValue);
			int iOtherSiteBestValue = 0;
			int iNumOtherCitySites = (kPlot.waterArea() == NULL) ? 0 : kPlayer.AI_getNumAdjacentAreaCitySites(kPlot.waterArea()->getID(), kPlot.getArea(), iOtherSiteBestValue);
			szString.append(CvWString::format(L"\n\nArea Sites = %d (%d)", iNumAreaCitySites, iAreaSiteBestValue));
			szString.append(CvWString::format(L"\nOther Sites = %d (%d)", iNumOtherCitySites, iOtherSiteBestValue));
		}
	}
	else if (kPlot.getOwner() != NO_PLAYER)
	{
		CvPlayerAI const& kOwner = GET_PLAYER(kPlot.getOwner()); // advc.003
		/* original code
		for (int iI = 0; iI < GC.getNumCivicInfos(); iI++)
			szString.append(CvWString::format(L"\n %s = %d", GC.getCivicInfo((CivicTypes)iI).getDescription(), kOwner.AI_civicValue((CivicTypes)iI)));*/
		// BETTER_BTS_AI_MOD (K-Mod edited, advc: mostly disabled), Debug, 11/30/08, jdog5000
		// advc.007: Commented out
		/*if(bShift && !bAlt) {
			std::vector<int> viBonusClassRevealed(GC.getNumBonusClassInfos(), 0);
			std::vector<int> viBonusClassUnrevealed(GC.getNumBonusClassInfos(), 0);
			std::vector<int> viBonusClassHave(GC.getNumBonusClassInfos(), 0);
			for (int iI = 0; iI < GC.getNumBonusInfos(); iI++) {
				TechTypes eRevealTech = (TechTypes)GC.getBonusInfo((BonusTypes)iI).getTechReveal();
				BonusClassTypes eBonusClass = (BonusClassTypes)GC.getBonusInfo((BonusTypes)iI).getBonusClassType();
				if (eRevealTech != NO_TECH) {
					if ((GET_TEAM(kPlot.getTeam()).isHasTech(eRevealTech)))
						viBonusClassRevealed[eBonusClass]++;
					else viBonusClassUnrevealed[eBonusClass]++;
					if (kOwner.getNumAvailableBonuses((BonusTypes)iI) > 0)
						viBonusClassHave[eBonusClass]++;
					else if (kOwner.countOwnedBonuses((BonusTypes)iI) > 0)
						viBonusClassHave[eBonusClass]++;
				}
			} bool bDummy;
			for (int iI = 0; iI < GC.getNumTechInfos(); iI++) {
				int iPathLength = kOwner.findPathLength(((TechTypes)iI), false);
				if (iPathLength <= 3 && !GET_TEAM(kPlot.getTeam()).isHasTech((TechTypes)iI)) {
					szString.append(CvWString::format(L"\n%s(%d)=%8d",
							GC.getTechInfo((TechTypes)iI).getDescription(),
							iPathLength, kOwner.AI_techValue((TechTypes)iI,
							1, false, true, viBonusClassRevealed,
							viBonusClassUnrevealed, viBonusClassHave)));
					szString.append(CvWString::format(L" (bld:%d, ", kOwner.AI_techBuildingValue((TechTypes)iI, true, bDummy)));
					int iObs = kOwner.AI_obsoleteBuildingPenalty((TechTypes)iI, true);
					if (iObs != 0)
						szString.append(CvWString::format(L"obs:%d, ", -iObs));
					// <k146>
					int iPrj = kOwner.AI_techProjectValue((TechTypes)iI, 1, bDummy);
					if (iPrj != 0)
						szString.append(CvWString::format(L"prj:%d, ", iPrj));
					// </k146>
					szString.append(CvWString::format(L"unt:%d)", kOwner.AI_techUnitValue((TechTypes)iI, 1, bDummy)));
				}
			}
		}
		else*/ if(bAlt && !bShift)
		{
			if (kPlot.isHasPathToEnemyCity(kPlot.getTeam()))
			{
				szString.append(CvWString::format(L"\nCan reach an enemy city\n\n"));
			}
			else
			{
				szString.append(CvWString::format(L"\nNo reachable enemy cities\n\n"));
			}
			for (int iI = 0; iI < MAX_PLAYERS; ++iI)
			{
				if (GET_PLAYER((PlayerTypes)iI).isAlive())
				{
					if (kPlot.isHasPathToPlayerCity(kPlot.getTeam(),(PlayerTypes)iI))
					{
						szString.append(CvWString::format(SETCOLR L"Can reach %s city" ENDCOLR, TEXT_COLOR("COLOR_GREEN"), GET_PLAYER((PlayerTypes)iI).getName()));
					}
					else
					{
						szString.append(CvWString::format(SETCOLR L"Cannot reach any %s city" ENDCOLR, TEXT_COLOR("COLOR_NEGATIVE_TEXT"), GET_PLAYER((PlayerTypes)iI).getName()));
					}

					if (GET_TEAM(kPlot.getTeam()).isAtWar(TEAMID((PlayerTypes)iI)))
					{
						szString.append(CvWString::format(L" (enemy)"));
					}
					szString.append(CvWString::format(L"\n"));
					// <advc.007>
					szString.append(CvString::format("Bonus trade counter: %d\n",
							kOwner.AI_getBonusTradeCounter((PlayerTypes)iI)));
					// </advc.007>
				}
			}
		}
		else if (bShift && bAlt)
		{
			for (int iI = 0; iI < GC.getNumCivicInfos(); iI++)
			{
				szString.append(CvWString::format(L"\n %s = %d",
						GC.getCivicInfo((CivicTypes)iI).getDescription(),
						kOwner.AI_civicValue((CivicTypes)iI)));
			}
		}
		// advc.007: Commented out
		/*else if(kPlot.headUnitNode() == NULL) {
			std::vector<UnitAITypes> vecUnitAIs;
			if (kPlot.getFeatureType() != NO_FEATURE) {
				szString.append(CvWString::format(L"\nDefense unit AIs:"));
				vecUnitAIs.push_back(UNITAI_CITY_DEFENSE);
				vecUnitAIs.push_back(UNITAI_COUNTER);
				vecUnitAIs.push_back(UNITAI_CITY_COUNTER);
			}
			else {
				szString.append(CvWString::format(L"\nAttack unit AIs:"));
				vecUnitAIs.push_back(UNITAI_ATTACK);
				vecUnitAIs.push_back(UNITAI_ATTACK_CITY);
				vecUnitAIs.push_back(UNITAI_COUNTER);
			}
			CvCity* pCloseCity = GC.getMap().findCity(x, y, kPlot.getOwner(), NO_TEAM, true);
			if (pCloseCity != NULL) {
				for (uint iI = 0; iI < vecUnitAIs.size(); iI++) {
					CvWString szTempString;
					getUnitAIString(szTempString, vecUnitAIs[iI]);
					szString.append(CvWString::format(L"\n  %s  ", szTempString.GetCString()));
					for (int iJ = 0; iJ < GC.getNumUnitClassInfos(); iJ++) {
						UnitTypes eUnit = (UnitTypes)GC.getCivilizationInfo(kOwner.getCivilizationType()).getCivilizationUnits((UnitClassTypes)iJ);
						if (eUnit != NO_UNIT && pCloseCity->canTrain(eUnit)) {
							int iValue = kOwner.AI_unitValue(eUnit, vecUnitAIs[iI], kPlot.area());
							if (iValue > 0)
								szString.append(CvWString::format(L"\n %s = %d", GC.getUnitInfo(eUnit).getDescription(), iValue));
						}
					}
				}
			}
		}*/
		// BETTER_BTS_AI_MOD: END
	}
	/*  <advc.027> Shift+Ctrl on a plot w/o a unit shows a breakdown of the
		area score computed in CvPlayer::findStartingArea. */
	{ CvArea const* a = kPlot.area();
	PlayerTypes activePl = g.getActivePlayer();
	if(!a->isWater() && bShift && !bAlt && !kPlot.isUnit() &&
			activePl != NO_PLAYER) {
		int total = 0; int tmp = 0;
		tmp=a->calculateTotalBestNatureYield();total+=tmp;szTempBuffer.Format(L"\nTotalBestNatureYield: %d", tmp); szString.append(szTempBuffer);
		tmp=a->countCoastalLand() * 2;szTempBuffer.Format(L"\n(CoastalLand*2: %d)", tmp); szString.append(szTempBuffer);
		tmp=a->getNumRiverEdges();szTempBuffer.Format(L"\n(RiverEdges: %d)", tmp); szString.append(szTempBuffer);
		tmp=GET_PLAYER(activePl).coastRiverStartingAreaScore(*a);total+=tmp;szTempBuffer.Format(L"\nCoastRiverScore: %d", tmp); szString.append(szTempBuffer);
		tmp=a->getNumTiles()/2;total+=tmp;szTempBuffer.Format(L"\nTiles*0.5: %d", tmp); szString.append(szTempBuffer);
		tmp=::round(a->getNumTotalBonuses() * 1.5);total+=tmp;szTempBuffer.Format(L"\nBonuses*1.5: %d", tmp); szString.append(szTempBuffer);
		tmp=100*::round(std::min(NUM_CITY_PLOTS + 1, a->getNumTiles() + 1)/ (NUM_CITY_PLOTS + 1.0));total+=tmp;szTempBuffer.Format(L"\nTilePercent: %d", tmp); szString.append(szTempBuffer);
		szTempBuffer.Format(L"\nAreaScore: %d", total); szString.append(szTempBuffer);
	} } // </advc.027>
	// advc.007: BBAI showed this regardless of pressed buttons
	if(bShift && !bAlt) {
		// BETTER_BTS_AI_MOD, DEBUG, 07/11/08, jdog5000
		bool bFirst = true;
		for (int iK = 0; iK < MAX_TEAMS; ++iK)
		{
			TeamTypes eTeam = (TeamTypes)iK;
			if (!GET_TEAM(eTeam).isAlive())
				continue;
			if (kPlot.getBlockadedCount(eTeam) > 0)
			{
				if (bFirst)
				{
					szString.append(CvWString::format(SETCOLR, TEXT_COLOR("COLOR_NEGATIVE_TEXT")));
					szString.append(NEWLINE);
					szString.append(gDLL->getText("TXT_KEY_PLOT_BLOCKADED"));
					szString.append(CvWString::format( ENDCOLR));

					szString.append(CvWString::format(L"Teams:"));
					bFirst = false;
				}
				szString.append(CvWString::format(L" %s,", GET_TEAM(eTeam).getName().c_str()));
			}
		}
		// BETTER_BTS_AI_MOD: END
	}
	// <advc.017b> Sea explorers
	if(!bShift && !bAlt && kPlot.isWater() && kPlot.isOwned()) {
		CvPlayerAI const& kOwner = GET_PLAYER(kPlot.getOwner());
		// This corresponds to code in CvCityAI::AI_chooseProduction
		int iHave =  kOwner.AI_totalWaterAreaUnitAIs(kPlot.area(), UNITAI_EXPLORE_SEA);
		int iNeeded = kOwner.AI_neededExplorers(kPlot.area());
		szString.append(CvWString::format(L"\nSea explorers H/N (%d , %d)",
				iHave, iNeeded));
	} // </advc.017b>
}

// advc.135c: Cut and pasted from setPlotHelp
void CvGameTextMgr::setPlotHelpDebug_ShiftOnly(CvWStringBuffer& szString, CvPlot const& kPlot) {

	szString.append(GC.getTerrainInfo(kPlot.getTerrainType()).getDescription());

	FAssertMsg(0 < GC.getNumBonusInfos(), "GC.getNumBonusInfos() is negative but an array is being allocated in CvInterface::updateHelpStrings");
	// advc.007: Commented out
	/*for (int iI = 0; iI < GC.getNumBonusInfos(); ++iI) {
		if (kPlot.isPlotGroupConnectedBonus(GC.getGame().getActivePlayer(), ((BonusTypes)iI))) {
			szString.append(NEWLINE);
			szString.append(GC.getBonusInfo((BonusTypes)iI).getDescription());
			szString.append(CvWString::format(L" (%d)", GET_PLAYER(GC.getGame().getActivePlayer()).AI_bonusVal((BonusTypes)iI, 0, true)));
		}
	}*/

	CvWString szTempBuffer;
	int x = kPlot.getX();
	int y = kPlot.getY();

	if (kPlot.getPlotGroup(GC.getGame().getActivePlayer()) != NULL)
	{
		szTempBuffer.Format(L"\n(%d, %d) group: %d", x, y, kPlot.getPlotGroup(GC.getGame().getActivePlayer())->getID());
	}
	else
	{
		szTempBuffer.Format(L"\n(%d, %d) group: (-1, -1)", x, y);
	}
	szString.append(szTempBuffer);

	szTempBuffer.Format(L"\nArea: %d", kPlot.getArea());
	szString.append(szTempBuffer);

	// BETTER_BTS_AI_MOD, Debug, 07/13/09, jdog5000: START
	szTempBuffer.Format(L"\nLatitude: %d", kPlot.getLatitude());
	szString.append(szTempBuffer);
	// BETTER_BTS_AI_MOD: END
	char tempChar = 'x';
	if(kPlot.getRiverNSDirection() == CARDINALDIRECTION_NORTH)
	{
		tempChar = 'N';
	}
	else if(kPlot.getRiverNSDirection() == CARDINALDIRECTION_SOUTH)
	{
		tempChar = 'S';
	}
	szTempBuffer.Format(L"\nNSRiverFlow: %c", tempChar);
	szString.append(szTempBuffer);

	tempChar = 'x';
	if(kPlot.getRiverWEDirection() == CARDINALDIRECTION_WEST)
	{
		tempChar = 'W';
	}
	else if(kPlot.getRiverWEDirection() == CARDINALDIRECTION_EAST)
	{
		tempChar = 'E';
	}
	szTempBuffer.Format(L"\nWERiverFlow: %c", tempChar);
	szString.append(szTempBuffer);

	if(kPlot.getRouteType() != NO_ROUTE)
	{
		szTempBuffer.Format(L"\nRoute: %s", GC.getRouteInfo(kPlot.getRouteType()).getDescription());
		szString.append(szTempBuffer);
	}

	if(kPlot.getRouteSymbol() != NULL)
	{
		szTempBuffer.Format(L"\nConnection: %i", gDLL->getRouteIFace()->getConnectionMask(kPlot.getRouteSymbol()));
		szString.append(szTempBuffer);
	}

	for (int iI = 0; iI < MAX_PLAYERS; ++iI)
	{
		if (GET_PLAYER((PlayerTypes)iI).isEverAlive()) // advc.099: was isAlive
		{
			if (kPlot.getCulture((PlayerTypes)iI) > 0)
			{
				szTempBuffer.Format(L"\n%s Culture: %d", GET_PLAYER((PlayerTypes)iI).getName(), kPlot.getCulture((PlayerTypes)iI));
				szString.append(szTempBuffer);
			}
		}
	} // advc.007: Use Alt for found values
	/*if(!GC.getGame().isNetworkMultiPlayer()) { // advc.001n: Might cache FoundValue
		PlayerTypes eActivePlayer = GC.getGame().getActivePlayer();
		int iActualFoundValue = kPlot.getFoundValue(eActivePlayer);
		int iCalcFoundValue = GET_PLAYER(eActivePlayer).AI_foundValue(x, y, -1, false);
		int iStartingFoundValue = GET_PLAYER(eActivePlayer).AI_foundValue(x, y, -1, true);
		szTempBuffer.Format(L"\nFound Value: %d, (%d, %d)", iActualFoundValue, iCalcFoundValue, iStartingFoundValue);
		szString.append(szTempBuffer);
	}*/
	CvCity* pWorkingCity = kPlot.getWorkingCity();
	if (NULL != pWorkingCity)
	{
		int iPlotIndex = pWorkingCity->getCityPlotIndex(&kPlot);
		int iBuildValue = pWorkingCity->AI_getBestBuildValue(iPlotIndex);
		BuildTypes eBestBuild = pWorkingCity->AI_getBestBuild(iPlotIndex);
		// BETTER_BTS_AI_MOD, Debug, 06/25/09, jdog5000: START
		szString.append(NEWLINE);

		int iFoodMultiplier, iProductionMultiplier, iCommerceMultiplier, iDesiredFoodChange;
		pWorkingCity->AI_getYieldMultipliers( iFoodMultiplier, iProductionMultiplier, iCommerceMultiplier, iDesiredFoodChange);

		szTempBuffer.Format(L"\n%s yield multipliers: ", pWorkingCity->getName().c_str());
		szString.append(szTempBuffer);
		szTempBuffer.Format(L"\n   Food %d, Prod %d, Com %d, DesFoodChange %d", iFoodMultiplier, iProductionMultiplier, iCommerceMultiplier, iDesiredFoodChange);
		szString.append(szTempBuffer);
		szTempBuffer.Format(L"\nTarget pop: %d, (%d good tiles)", pWorkingCity->AI_getTargetPopulation(), pWorkingCity->AI_countGoodPlots());
		szString.append(szTempBuffer);

		ImprovementTypes eImprovement = kPlot.getImprovementType();

		if (NO_BUILD != eBestBuild)
		{

			if (GC.getBuildInfo(eBestBuild).getImprovement() != NO_IMPROVEMENT &&
					eImprovement != NO_IMPROVEMENT && eImprovement != GC.getBuildInfo(eBestBuild).getImprovement())
			{
				szTempBuffer.Format(SETCOLR L"\nBest Build: %s (%d) replacing %s" ENDCOLR,
						/*  advc.007: Was RED; now same color as in the else branch
							(but I'm not sure if the else branch is ever executed). */
						TEXT_COLOR("COLOR_HIGHLIGHT_TEXT"),
						GC.getBuildInfo(eBestBuild).getDescription(), iBuildValue, GC.getImprovementInfo(eImprovement).getDescription());
			}
			else
			{
				szTempBuffer.Format(SETCOLR L"\nBest Build: %s (%d)" ENDCOLR, TEXT_COLOR("COLOR_HIGHLIGHT_TEXT"), GC.getBuildInfo(eBestBuild).getDescription(), iBuildValue);
			}

			szString.append(szTempBuffer);
		}

		szTempBuffer.Format(L"\nActual Build Values: ");
		szString.append(szTempBuffer);

		for (int iI = 0; iI < GC.getNumImprovementInfos(); iI++)
		{
			if (kPlot.canHaveImprovement((ImprovementTypes)iI, pWorkingCity->getTeam()))
			{
				int iOtherBuildValue = pWorkingCity->AI_getImprovementValue(
						kPlot, (ImprovementTypes)iI, iFoodMultiplier,
						iProductionMultiplier, iCommerceMultiplier,
						iDesiredFoodChange);
				// <advc.007>
				/*  The same as above. Probably supposed to be the
					ImprovementValue of the current improvement, but that's always 0;
					I think the value already takes the current improvement into account. */
				/*int iOldValue =        pWorkingCity->AI_getImprovementValue(
						kPlot, (ImprovementTypes)iI, iFoodMultiplier,
						iProductionMultiplier, iCommerceMultiplier,
						iDesiredFoodChange);*/
				// Save space
				if(iOtherBuildValue == 0)
					continue;
				szTempBuffer.Format(L"\n   %s : %d",// (old %d)",
						GC.getImprovementInfo((ImprovementTypes)iI).getDescription(),
						iOtherBuildValue/*, iOldValue*/); // </advc.007>
				szString.append(szTempBuffer);
			}
		}

		szTempBuffer.Format(L"\nStandard Build Values: ");
		szString.append(szTempBuffer);

		for (int iI = 0; iI < GC.getNumImprovementInfos(); iI++)
		{
			int iOtherBuildValue = pWorkingCity->AI_getImprovementValue(
					kPlot, (ImprovementTypes)iI, 100, 100, 100, 0);
			if (iOtherBuildValue > 0)
			{
				szTempBuffer.Format(L"\n   %s : %d",
						GC.getImprovementInfo((ImprovementTypes)iI).getDescription(),
						iOtherBuildValue);
				szString.append(szTempBuffer);
			}
		}

		szString.append(NEWLINE);
		// BETTER_BTS_AI_MOD: END
	}

	/*{
		szTempBuffer.Format(L"\nStack Str: land=%d(%d), sea=%d(%d), air=%d(%d)",
			kPlot.AI_sumStrength(NO_PLAYER, NO_PLAYER, DOMAIN_LAND, false, false, false),
			kPlot.AI_sumStrength(NO_PLAYER, NO_PLAYER, DOMAIN_LAND, true, false, false),
			kPlot.AI_sumStrength(NO_PLAYER, NO_PLAYER, DOMAIN_SEA, false, false, false),
			kPlot.AI_sumStrength(NO_PLAYER, NO_PLAYER, DOMAIN_SEA, true, false, false),
			kPlot.AI_sumStrength(NO_PLAYER, NO_PLAYER, DOMAIN_AIR, false, false, false),
			kPlot.AI_sumStrength(NO_PLAYER, NO_PLAYER, DOMAIN_AIR, true, false, false));
		szString.append(szTempBuffer);
	}*/

	// advc.007: Commented out. Takes up too much room.
	/*if (kPlot.getPlotCity() != NULL) {
		PlayerTypes ePlayer = kPlot.getOwner();
		CvPlayerAI& kPlayer = GET_PLAYER(ePlayer);
		szString.append(CvWString::format(L"\n\nAI unit class weights ..."));
		for (int iI = 0; iI < GC.getNumUnitClassInfos(); ++iI) {
			if (kPlayer.AI_getUnitClassWeight((UnitClassTypes)iI) != 0)
				szString.append(CvWString::format(L"\n%s = %d", GC.getUnitClassInfo((UnitClassTypes)iI).getDescription(), kPlayer.AI_getUnitClassWeight((UnitClassTypes)iI)));
		}
		szString.append(CvWString::format(L"\n\nalso unit combat type weights..."));
		for (int iI = 0; iI < GC.getNumUnitCombatInfos(); ++iI) {
			if (kPlayer.AI_getUnitCombatWeight((UnitCombatTypes)iI) != 0)
				szString.append(CvWString::format(L"\n%s = % d", GC.getUnitCombatInfo((UnitCombatTypes)iI).getDescription(), kPlayer.AI_getUnitCombatWeight((UnitCombatTypes)iI)));
		}
	}*/
}

// advc.135c: Cut and pasted from setPlotHelp
void CvGameTextMgr::setPlotHelpDebug_AltOnly(CvWStringBuffer& szString, CvPlot const& kPlot) {

	CvWString szTempBuffer;

	if (kPlot.isOwned())
	{
		CvPlayerAI const& kOwner = GET_PLAYER(kPlot.getOwner()); // advc.003
		szTempBuffer.Format(L"\nThis player has %d area cities", kPlot.area()->getCitiesPerPlayer(kOwner.getID()));
		szString.append(szTempBuffer);
		for (int iI = 0; iI < GC.getNumReligionInfos(); ++iI)
		{
			int iNeededMissionaries = kOwner.AI_neededMissionaries(kPlot.area(), ((ReligionTypes)iI));
			if (iNeededMissionaries > 0)
			{
				szTempBuffer.Format(L"\nNeeded %c missionaries = %d", GC.getReligionInfo((ReligionTypes)iI).getChar(), iNeededMissionaries);
				szString.append(szTempBuffer);
			}
		}

		int iOurDefense = kOwner.AI_localDefenceStrength(&kPlot, kPlot.getTeam(), DOMAIN_LAND, 0, true, false, true);
		int iEnemyOffense = kOwner.AI_localAttackStrength(&kPlot, NO_TEAM);
		if (iEnemyOffense > 0)
		{
			szString.append(CvWString::format(SETCOLR L"\nDanger: %.2f (%d/%d)" ENDCOLR, TEXT_COLOR("COLOR_NEGATIVE_TEXT"),
				(iEnemyOffense * 1.0f) / std::max(1, iOurDefense), iEnemyOffense, iOurDefense));
		}

		CvCity* pCity = kPlot.getPlotCity();
		if (pCity != NULL)
		{
			/* szTempBuffer.Format(L"\n\nCulture Pressure Value = %d (%d)", pCity->AI_calculateCulturePressure(), pCity->culturePressureFactor());
			szString.append(szTempBuffer); */

			szTempBuffer.Format(L"\nWater World Percent = %d", pCity->AI_calculateWaterWorldPercent());
			szString.append(szTempBuffer);

			CvPlayerAI& kCityOwner = GET_PLAYER(pCity->getOwner());
			/* original bts code
			int iUnitCost = kPlayer.calculateUnitCost();
			int iTotalCosts = kPlayer.calculatePreInflatedCosts();
			int iUnitCostPercentage = (iUnitCost * 100) / std::max(1, iTotalCosts);
			szString.append(CvWString::format(L"\nUnit cost percentage: %d (%d / %d)", iUnitCostPercentage, iUnitCost, iTotalCosts)); */
			// K-Mod
			int iBuildUnitProb = pCity->AI().AI_buildUnitProb(); // advc.003 (cast)
			szString.append(CvWString::format(L"\nUnit Cost: %d (max: %d)",
					kCityOwner.AI_unitCostPerMil(), kCityOwner.AI_maxUnitCostPerMil(pCity->area(), iBuildUnitProb)));
			// K-Mod end

			szString.append(CvWString::format(L"\nUpgrade all units: %d gold", kCityOwner.AI_getGoldToUpgradeAllUnits()));
			// K-Mod
			{
				int iValue = 0;
				for (int iI = 0; iI < NUM_COMMERCE_TYPES; iI++)
				{
					iValue += kCityOwner.getCommerceRate((CommerceTypes)iI) * kCityOwner.AI_commerceWeight((CommerceTypes)iI)/100;
				}
				int iLoop=-1;
				for (CvCity* pLoopCity = kCityOwner.firstCity(&iLoop); pLoopCity != NULL; pLoopCity = kCityOwner.nextCity(&iLoop))
				{
					iValue += 2*pLoopCity->getYieldRate(YIELD_PRODUCTION);
				}
				iValue /= kCityOwner.getTotalPopulation();
				szString.append(CvWString::format(L"\nAverage citizen value: %d", iValue));

				//
				szString.append(CvWString::format(L"\nBuild unit prob: %d%%", iBuildUnitProb));
				BuildingTypes eBestBuilding = pCity->AI().AI_bestBuildingThreshold(0, 0, 0, true); // advc.003 (cast)
				int iBestBuildingValue = (eBestBuilding == NO_BUILDING) ? 0 : pCity->AI_buildingValue(eBestBuilding, 0, 0, true);

				// Note. cf. adjustments made in AI_chooseProduction
				if (GC.getNumEraInfos() > 1)
				{
					iBestBuildingValue *= 2*(GC.getNumEraInfos()-1) - kCityOwner.getCurrentEra();
					iBestBuildingValue /= GC.getNumEraInfos()-1;
				} // advc.001n: AI_getNumAreaCitySites could cache FoundValue
				if(!GC.getGame().isNetworkMultiPlayer())
				{
					int iTargetCities = GC.getWorldInfo(GC.getMap().getWorldSize()).getTargetNumCities();
					int foo=-1;
					if (kCityOwner.AI_getNumAreaCitySites(kPlot.getArea(), foo) > 0 && kCityOwner.getNumCities() < iTargetCities)
					{
						iBestBuildingValue *= kCityOwner.getNumCities() + iTargetCities;
						iBestBuildingValue /= 2*iTargetCities;
					}
				}
				//

				iBuildUnitProb *= (250 + iBestBuildingValue);
				iBuildUnitProb /= (100 + 3 * iBestBuildingValue);
				szString.append(CvWString::format(L" (%d%%)", iBuildUnitProb));
			}
			// K-Mod end

			szString.append(CvWString::format(L"\n\nRanks:"));
			szString.append(CvWString::format(L"\nPopulation:%d", pCity->findPopulationRank()));

			szString.append(CvWString::format(L"\nFood:%d(%d), ", pCity->findYieldRateRank(YIELD_FOOD), pCity->findBaseYieldRateRank(YIELD_FOOD)));
			szString.append(CvWString::format(L"Prod:%d(%d), ", pCity->findYieldRateRank(YIELD_PRODUCTION), pCity->findBaseYieldRateRank(YIELD_PRODUCTION)));
			szString.append(CvWString::format(L"Commerce:%d(%d)", pCity->findYieldRateRank(YIELD_COMMERCE), pCity->findBaseYieldRateRank(YIELD_COMMERCE)));

			szString.append(CvWString::format(L"\nGold:%d, ", pCity->findCommerceRateRank(COMMERCE_GOLD)));
			szString.append(CvWString::format(L"Research:%d, ", pCity->findCommerceRateRank(COMMERCE_RESEARCH)));
			szString.append(CvWString::format(L"Culture:%d", pCity->findCommerceRateRank(COMMERCE_CULTURE)));
		}
		szString.append(NEWLINE);

		// BETTER_BTS_AI_MOD, Debug, 06/11/08, jdog5000: START
		//AI strategies
		if (kOwner.AI_isDoStrategy(AI_STRATEGY_DAGGER))
		{
			szTempBuffer.Format(L"Dagger, ");
			szString.append(szTempBuffer);
		}
		if (kOwner.AI_isDoStrategy(AI_STRATEGY_CRUSH))
		{
			szTempBuffer.Format(L"Crush, ");
			szString.append(szTempBuffer);
		}
		if (kOwner.AI_isDoStrategy(AI_STRATEGY_ALERT1))
		{
			szTempBuffer.Format(L"Alert1, ");
			szString.append(szTempBuffer);
		}
		if (kOwner.AI_isDoStrategy(AI_STRATEGY_ALERT2))
		{
			szTempBuffer.Format(L"Alert2, ");
			szString.append(szTempBuffer);
		}
		if (kOwner.AI_isDoStrategy(AI_STRATEGY_TURTLE))
		{
			szTempBuffer.Format(L"Turtle, ");
			szString.append(szTempBuffer);
		}
		if (kOwner.AI_isDoStrategy(AI_STRATEGY_LAST_STAND))
		{
			szTempBuffer.Format(L"LastStand, ");
			szString.append(szTempBuffer);
		}
		if (kOwner.AI_isDoStrategy(AI_STRATEGY_FINAL_WAR))
		{
			szTempBuffer.Format(L"FinalWar, ");
			szString.append(szTempBuffer);
		}
		if (kOwner.AI_isDoStrategy(AI_STRATEGY_GET_BETTER_UNITS))
		{
			szTempBuffer.Format(L"GetBetterUnits, ");
			szString.append(szTempBuffer);
		}
		if (kOwner.AI_isDoStrategy(AI_STRATEGY_FASTMOVERS))
		{
			szTempBuffer.Format(L"FastMovers, ");
			szString.append(szTempBuffer);
		}
		if (kOwner.AI_isDoStrategy(AI_STRATEGY_LAND_BLITZ))
		{
			szTempBuffer.Format(L"LandBlitz, ");
			szString.append(szTempBuffer);
		}
		if (kOwner.AI_isDoStrategy(AI_STRATEGY_AIR_BLITZ))
		{
			szTempBuffer.Format(L"AirBlitz, ");
			szString.append(szTempBuffer);
		}
		if (kOwner.AI_isDoStrategy(AI_STRATEGY_OWABWNW))
		{
			szTempBuffer.Format(L"OWABWNW, ");
			szString.append(szTempBuffer);
		}
		if (kOwner.AI_isDoStrategy(AI_STRATEGY_PRODUCTION))
		{
			szTempBuffer.Format(L"Production, ");
			szString.append(szTempBuffer);
		}
		if (kOwner.AI_isDoStrategy(AI_STRATEGY_MISSIONARY))
		{
			szTempBuffer.Format(L"Missionary, ");
			szString.append(szTempBuffer);
		}
		if (kOwner.AI_isDoStrategy(AI_STRATEGY_BIG_ESPIONAGE))
		{
			szTempBuffer.Format(L"BigEspionage, ");
			szString.append(szTempBuffer);
		}
		if (kOwner.AI_isDoStrategy(AI_STRATEGY_ECONOMY_FOCUS)) // K-Mod
		{
			szTempBuffer.Format(L"EconomyFocus, ");
			szString.append(szTempBuffer);
		}
		if (kOwner.AI_isDoStrategy(AI_STRATEGY_ESPIONAGE_ECONOMY)) // K-Mod
		{
			szTempBuffer.Format(L"EspionageEconomy, ");
			szString.append(szTempBuffer);
		}

		//Area battle plans.
		AreaAITypes eAAI = kPlot.area()->getAreaAIType(kPlot.getTeam());
		switch(eAAI) { // advc.003: If-else replaced with switch
		case AREAAI_OFFENSIVE: szTempBuffer.Format(L"\n Area AI = OFFENSIVE");
			break;
		case AREAAI_DEFENSIVE: szTempBuffer.Format(L"\n Area AI = DEFENSIVE");
			break;
		case AREAAI_MASSING: szTempBuffer.Format(L"\n Area AI = MASSING");
			break;
		case AREAAI_ASSAULT: szTempBuffer.Format(L"\n Area AI = ASSAULT");
			break;
		case AREAAI_ASSAULT_MASSING: szTempBuffer.Format(L"\n Area AI = ASSAULT_MASSING");
			break;
		case AREAAI_NEUTRAL: szTempBuffer.Format(L"\n Area AI = NEUTRAL");
			break;
		// <advc.007> These two were missing
		case AREAAI_ASSAULT_ASSIST: szTempBuffer.Format(L"\n Area AI = ASSAULT_ASSIST");
			break;
		default: szTempBuffer.Format(L"\n Area AI = None"); // </advc.007>
		}

		szString.append(szTempBuffer);
		CvTeamAI const& kPlotTeam = GET_TEAM(kPlot.getTeam()); // advc.003
		szString.append(CvWString::format(L"\n\nNum Wars: %d + %d minor", kPlotTeam.getAtWarCount(true), kPlotTeam.getAtWarCount(false) - kPlotTeam.getAtWarCount(true)));
		szString.append(CvWString::format(L"\nWarplans:"));
		for (int iK = 0; iK < MAX_TEAMS; ++iK)
		{
			TeamTypes eTeam = (TeamTypes)iK;

			if (GET_TEAM(eTeam).isAlive() || GET_TEAM(eTeam).isBarbarian())
			{
				WarPlanTypes eWarPlan = kPlotTeam.AI_getWarPlan(eTeam);
				switch(eWarPlan) { // advc.003: if-else replaced with switch
				case WARPLAN_ATTACKED: szString.append(CvWString::format(L"\n%s: ATTACKED", GET_TEAM(eTeam).getName().c_str()));
					break;
				case WARPLAN_ATTACKED_RECENT: szString.append(CvWString::format(L"\n%s: ATTACKED_RECENT", GET_TEAM(eTeam).getName().c_str()));
					break;
				case WARPLAN_PREPARING_LIMITED: szString.append(CvWString::format(L"\n%s: PREP_LIM", GET_TEAM(eTeam).getName().c_str()));
					break;
				case WARPLAN_PREPARING_TOTAL: szString.append(CvWString::format(L"\n%s: PREP_TOTAL", GET_TEAM(eTeam).getName().c_str()));
					break;
				case WARPLAN_LIMITED: szString.append(CvWString::format(L"\n%s: LIMITED", GET_TEAM(eTeam).getName().c_str()));
					break;
				case WARPLAN_TOTAL: szString.append(CvWString::format(L"\n%s: TOTAL", GET_TEAM(eTeam).getName().c_str()));
					break;
				case WARPLAN_DOGPILE: szString.append(CvWString::format(L"\n%s: DOGPILE", GET_TEAM(eTeam).getName().c_str()));
					break;
				default:
				{
					if (kPlotTeam.isAtWar(eTeam))
					{
						szString.append(CvWString::format(SETCOLR L"\n%s: NO_WARPLAN!" ENDCOLR, TEXT_COLOR("COLOR_WARNING_TEXT"), GET_TEAM(eTeam).getName().c_str()));
					}
				}
				}
			}

			if (kPlotTeam.isMinorCiv() || kPlotTeam.isBarbarian())
			{
				if (kPlot.getTeam() != eTeam && !kPlotTeam.isAtWar(eTeam))
				{
					szString.append(CvWString::format(SETCOLR L"\n%s: minor/barb not at war!" ENDCOLR, TEXT_COLOR("COLOR_WARNING_TEXT"), GET_TEAM(eTeam).getName().c_str()));
				}
			}
		}

		CvCity* pTargetCity = kPlot.area()->getTargetCity(kPlot.getOwner());
		if (pTargetCity)
		{
			szString.append(CvWString::format(L"\nTarget City: %s", pTargetCity->getName().c_str()));
		}
		else
		{
			szString.append(CvWString::format(L"\nTarget City: None"));
		}
		// BETTER_BTS_AI_MOD: END
	} // end if (kPlot.isOwned())

	bool bFirst = true;
	// <advc.003>
	int x = kPlot.getX();
	int y = kPlot.getY(); // </advc.003>
	for (int iI = 0; iI < MAX_PLAYERS; ++iI)
	{
		PlayerTypes ePlayer = (PlayerTypes)iI;
		CvPlayerAI& kLoopPlayer = GET_PLAYER(ePlayer);

		if (kLoopPlayer.isAlive()
				// advc.001n: Might cache FoundValue
				&& !GC.getGame().isNetworkMultiPlayer())
		{ // <advc.007> Moved up, and skip unrevealed.
			bool bRevealed = kPlot.isRevealed(kLoopPlayer.getTeam(), false);
			if(!bRevealed)
				continue; // </advc.007>
			int iActualFoundValue = kPlot.getFoundValue(ePlayer, /* advc.052: */ true);
			//int iCalcFoundValue = kPlayer.AI_foundValue(x, y, -1, false);
			// <advc.007>
			CvPlayerAI::CvFoundSettings fset(kLoopPlayer, false);
			fset.bDebug = true;
			int iCalcFoundValue = kLoopPlayer.AI_foundValue_bulk(x, y, fset);
			int iStartingFoundValue = 0;
			// Gets in the way of debugging bStartingLoc=false </advc.007>
					//=kPlayer.AI_foundValue(x, y, -1, true);
			int iBestAreaFoundValue = kPlot.area()->getBestFoundValue(ePlayer);
			int iCitySiteBestValue;
			int iNumAreaCitySites = kLoopPlayer.AI_getNumAreaCitySites(kPlot.getArea(), iCitySiteBestValue);

			if ((iActualFoundValue > 0 || iCalcFoundValue > 0 || iStartingFoundValue > 0)
					|| ((kPlot.getOwner() == iI) && (iBestAreaFoundValue > 0)))
			{
				if (bFirst)
				{
					szString.append(CvWString::format(SETCOLR L"\nFound Values:" ENDCOLR, TEXT_COLOR("COLOR_UNIT_TEXT")));
					bFirst = false;
				}

				szString.append(NEWLINE);

				szString.append(CvWString::format(SETCOLR, TEXT_COLOR(bRevealed ? (((iActualFoundValue > 0) && (iActualFoundValue == iBestAreaFoundValue)) ? "COLOR_UNIT_TEXT" : "COLOR_ALT_HIGHLIGHT_TEXT") : "COLOR_HIGHLIGHT_TEXT")));

				//if (!bRevealed) // advc.007
				{
					szString.append(CvWString::format(L"("));
				}

				szString.append(CvWString::format(L"%s: %d", kLoopPlayer.getName(),
						iCalcFoundValue)); // advc.007: Swapped with iActual

				//if (!bRevealed) // advc.007
				{
					szString.append(CvWString::format(L")"));
				}

				szString.append(CvWString::format(ENDCOLR));

				if (iCalcFoundValue > 0 || iStartingFoundValue > 0)
				{
					szTempBuffer.Format(L" (%d,s:%d), thresh: %d",
							iActualFoundValue, // advc.007: Swapped with iCalc
							iStartingFoundValue,
							// advc.007: thresh added
							kLoopPlayer.AI_getMinFoundValue());
					szString.append(szTempBuffer);
				}

				int iDeadlockCount = kLoopPlayer.AI_countDeadlockedBonuses(&kPlot);
				if (iDeadlockCount > 0)
				{
					szTempBuffer.Format(L", " SETCOLR L"d=%d" ENDCOLR, TEXT_COLOR("COLOR_NEGATIVE_TEXT"), iDeadlockCount);
					szString.append(szTempBuffer);
				}

				if (kLoopPlayer.AI_isPlotCitySite(kPlot))
				{
					szTempBuffer.Format(L", " SETCOLR L"X" ENDCOLR, TEXT_COLOR("COLOR_UNIT_TEXT"));
					szString.append(szTempBuffer);
				}

				if ((iBestAreaFoundValue > 0) || (iNumAreaCitySites > 0))
				{
					int iBestFoundValue = kLoopPlayer.findBestFoundValue();

					szTempBuffer.Format(L"\n  Area Best = %d, Best = %d, Sites = %d", iBestAreaFoundValue, iBestFoundValue, iNumAreaCitySites);
					szString.append(szTempBuffer);
				}
			}
		}
	}
}

// advc.135c: Cut and pasted from setPlotHelp
void CvGameTextMgr::setPlotHelpDebug_ShiftAltOnly(CvWStringBuffer& szString, CvPlot const& kPlot) {

	CvWString szTempBuffer;

	CvCity*	pCity = kPlot.getWorkingCity();
	if (pCity != NULL)
	{
		// some functions we want to call are not in CvCity, worse some are protected, so made us a friend
		CvCityAI const& kCityAI = pCity->AI(); // advc.003

		CvPlayerAI const& kCityOwner = GET_PLAYER(pCity->getOwner()); // advc.003
		//bool bAvoidGrowth = kCity.AI_avoidGrowth();
		//bool bIgnoreGrowth = kCityAI.AI_ignoreGrowth();
		int iGrowthValue = kCityAI.AI_growthValuePerFood();

		// if we over the city, then do an array of all the plots
		if (kPlot.getPlotCity() != NULL)
		{
			// check avoid growth
			/* if (bAvoidGrowth || bIgnoreGrowth) {
				// red color
				szString.append(CvWString::format(SETCOLR, TEXT_COLOR("COLOR_NEGATIVE_TEXT")));
				if (bAvoidGrowth) {
					szString.append(CvWString::format(L"AvoidGrowth"));
					if (bIgnoreGrowth)
						szString.append(CvWString::format(L", "));
				}
				if (bIgnoreGrowth)
					szString.append(CvWString::format(L"IgnoreGrowth"));
				// end color
				szString.append(CvWString::format( ENDCOLR L"\n"));
			} */

			// if control key is down, ignore food
			bool bIgnoreFood = GC.ctrlKey();

			// line one is: blank, 20, 9, 10, blank
			setCityPlotYieldValueString(szString, pCity, -1, bIgnoreFood, iGrowthValue);
			setCityPlotYieldValueString(szString, pCity, 20, bIgnoreFood, iGrowthValue);
			setCityPlotYieldValueString(szString, pCity, 9, bIgnoreFood, iGrowthValue);
			setCityPlotYieldValueString(szString, pCity, 10, bIgnoreFood, iGrowthValue);
			szString.append(L"\n");

			// line two is: 19, 8, 1, 2, 11
			setCityPlotYieldValueString(szString, pCity, 19, bIgnoreFood, iGrowthValue);
			setCityPlotYieldValueString(szString, pCity, 8, bIgnoreFood, iGrowthValue);
			setCityPlotYieldValueString(szString, pCity, 1, bIgnoreFood, iGrowthValue);
			setCityPlotYieldValueString(szString, pCity, 2, bIgnoreFood, iGrowthValue);
			setCityPlotYieldValueString(szString, pCity, 11, bIgnoreFood, iGrowthValue);
			szString.append(L"\n");

			// line three is: 18, 7, 0, 3, 12
			setCityPlotYieldValueString(szString, pCity, 18, bIgnoreFood, iGrowthValue);
			setCityPlotYieldValueString(szString, pCity, 7, bIgnoreFood, iGrowthValue);
			setCityPlotYieldValueString(szString, pCity, 0, bIgnoreFood, iGrowthValue);
			setCityPlotYieldValueString(szString, pCity, 3, bIgnoreFood, iGrowthValue);
			setCityPlotYieldValueString(szString, pCity, 12, bIgnoreFood, iGrowthValue);
			szString.append(L"\n");

			// line four is: 17, 6, 5, 4, 13
			setCityPlotYieldValueString(szString, pCity, 17, bIgnoreFood, iGrowthValue);
			setCityPlotYieldValueString(szString, pCity, 6, bIgnoreFood, iGrowthValue);
			setCityPlotYieldValueString(szString, pCity, 5, bIgnoreFood, iGrowthValue);
			setCityPlotYieldValueString(szString, pCity, 4, bIgnoreFood, iGrowthValue);
			setCityPlotYieldValueString(szString, pCity, 13, bIgnoreFood, iGrowthValue);
			szString.append(L"\n");

			// line five is: blank, 16, 15, 14, blank
			setCityPlotYieldValueString(szString, pCity, -1, bIgnoreFood, iGrowthValue);
			setCityPlotYieldValueString(szString, pCity, 16, bIgnoreFood, iGrowthValue);
			setCityPlotYieldValueString(szString, pCity, 15, bIgnoreFood, iGrowthValue);
			setCityPlotYieldValueString(szString, pCity, 14, bIgnoreFood, iGrowthValue);

			// show specialist values too
			for (int iI = 0; iI < GC.getNumSpecialistInfos(); ++iI)
			{
				int iMaxThisSpecialist = pCity->getMaxSpecialistCount((SpecialistTypes) iI);
				int iSpecialistCount = pCity->getSpecialistCount((SpecialistTypes) iI);
				bool bUsingSpecialist = (iSpecialistCount > 0);
				bool bDefaultSpecialist = (iI == GC.getDefineINT("DEFAULT_SPECIALIST"));

				// can this city have any of this specialist?
				if (iMaxThisSpecialist > 0 || bDefaultSpecialist)
				{
					// start color
					if (pCity->getForceSpecialistCount((SpecialistTypes) iI) > 0)
						szString.append(CvWString::format(L"\n" SETCOLR, TEXT_COLOR("COLOR_NEGATIVE_TEXT")));
					else if (bUsingSpecialist)
						szString.append(CvWString::format(L"\n" SETCOLR, TEXT_COLOR("COLOR_ALT_HIGHLIGHT_TEXT")));
					else
						szString.append(CvWString::format(L"\n" SETCOLR, TEXT_COLOR("COLOR_HIGHLIGHT_TEXT")));

					// add name
					szString.append(GC.getSpecialistInfo((SpecialistTypes) iI).getDescription());

					// end color
					szString.append(CvWString::format(ENDCOLR));

					// add usage
					szString.append(CvWString::format(L": (%d/%d) ", iSpecialistCount, iMaxThisSpecialist));

					// add value
					int iValue = kCityAI.AI_specialistValue((SpecialistTypes)iI, bUsingSpecialist, false, iGrowthValue);
					setYieldValueString(szString, iValue, bUsingSpecialist);
				}
			}
			{
				/* int iFood = kCityOwner.AI_averageYieldMultiplier(YIELD_FOOD);
				int iHammer = kCityOwner.AI_averageYieldMultiplier(YIELD_PRODUCTION);
				int iCommerce = kCityOwner.AI_averageYieldMultiplier(YIELD_COMMERCE);

				szString.append(CvWString::format(L"\nPlayer avg:       (f%d, h%d, c%d)", iFood, iHammer, iCommerce));

				iFood = pCity->AI_yieldMultiplier(YIELD_FOOD);
				iHammer = pCity->AI_yieldMultiplier(YIELD_PRODUCTION);
				iCommerce = pCity->AI_yieldMultiplier(YIELD_COMMERCE);

				szString.append(CvWString::format(L"\nCity yield mults: (f%d, h%d, c%d)", iFood, iHammer, iCommerce));

				iFood = kCityAI.AI_specialYieldMultiplier(YIELD_FOOD);
				iHammer = kCityAI.AI_specialYieldMultiplier(YIELD_PRODUCTION);
				iCommerce = kCityAI.AI_specialYieldMultiplier(YIELD_COMMERCE);

				szString.append(CvWString::format(L"\nCity spec mults:  (f%d, h%d, c%d)", iFood, iHammer, iCommerce)); */

				// K-Mod
				szString.append(L"\n\nPlayer avg:       (");
				for (YieldTypes i = (YieldTypes)0; i < NUM_YIELD_TYPES; i = (YieldTypes)(i+1))
					szString.append(CvWString::format(L"%s%d%c", i == 0 ? L"" : L", ", kCityOwner.AI_averageYieldMultiplier(i), GC.getYieldInfo(i).getChar()));
				szString.append(L")");

				szString.append(L"\nCity yield mults: (");
				for (YieldTypes i = (YieldTypes)0; i < NUM_YIELD_TYPES; i = (YieldTypes)(i+1))
					szString.append(CvWString::format(L"%s%d%c", i == 0 ? L"" : L", ", pCity->AI_yieldMultiplier(i), GC.getYieldInfo(i).getChar()));
				szString.append(L")");

				szString.append(L"\nCity spec mults:  (");
				for (YieldTypes i = (YieldTypes)0; i < NUM_YIELD_TYPES; i = (YieldTypes)(i+1))
					szString.append(CvWString::format(L"%s%d%c", i == 0 ? L"" : L", ", kCityAI.AI_specialYieldMultiplier(i), GC.getYieldInfo(i).getChar()));
				szString.append(L")");
				szString.append(CvWString::format(L"\nCity weights: ("));
				for (CommerceTypes i = (CommerceTypes)0; i < NUM_COMMERCE_TYPES; i=(CommerceTypes)(i+1))
					szString.append(CvWString::format(L"%s%d%c", i == 0 ? L"" : L", ", kCityOwner.AI_commerceWeight(i, pCity), GC.getCommerceInfo(i).getChar()));
				szString.append(L")");
				// K-Mod end

				szString.append(CvWString::format(L"\nExchange"));
				for (int iI = 0; iI < NUM_COMMERCE_TYPES; ++iI)
				{
					int iCommerce = kCityOwner.AI_averageCommerceExchange((CommerceTypes)iI);
					szTempBuffer.Format(L", %d%c", iCommerce, GC.getCommerceInfo((CommerceTypes) iI).getChar());
					szString.append(szTempBuffer);
				}

				// BBAI
				szString.append(CvWString::format(L"\nAvg mults"));
				for (int iI = 0; iI < NUM_COMMERCE_TYPES; ++iI)
				{
					int iCommerce = kCityOwner.AI_averageCommerceMultiplier((CommerceTypes)iI);
					szTempBuffer.Format(L", %d%c", iCommerce, GC.getCommerceInfo((CommerceTypes) iI).getChar());
					szString.append(szTempBuffer);
				}
				// BBAI end
				// K-Mod
				szString.append(CvWString::format(L"\nAvg %c pressure: %d",
					GC.getCommerceInfo(COMMERCE_CULTURE).getChar(),
					kCityOwner.AI_averageCulturePressure()));
				// K-Mod end

				if (kCityOwner.AI_isFinancialTrouble())
				{
					szTempBuffer.Format(L"$$$!!!");
					szString.append(szTempBuffer);
				}
			}
		}
		else
		{
			bool bWorkingPlot = pCity->isWorkingPlot(&kPlot);

			if (bWorkingPlot)
				szTempBuffer.Format( SETCOLR L"%s is working" ENDCOLR, TEXT_COLOR("COLOR_ALT_HIGHLIGHT_TEXT"), pCity->getName().GetCString());
			else
				szTempBuffer.Format( SETCOLR L"%s not working" ENDCOLR, TEXT_COLOR("COLOR_HIGHLIGHT_TEXT"), pCity->getName().GetCString());
			szString.append(szTempBuffer);

			int iValue = kCityAI.AI_plotValue(&kPlot, bWorkingPlot, false, false, iGrowthValue);
			int iRawValue = kCityAI.AI_plotValue(&kPlot, bWorkingPlot, true, false, iGrowthValue);
			int iMagicValue = kCityAI.AI_getPlotMagicValue(kPlot, kCityAI.healthRate() == 0);

			szTempBuffer.Format(L"\nvalue = %d\nraw value = %d\nmagic value = %d", iValue, iRawValue, iMagicValue);
			szString.append(szTempBuffer);
		}
	} // <advc.007> For advc.040
	if(kPlot.isOwned()) {
		szString.append(CvString::format("\nWorkers needed in area: %d\n",
				GET_PLAYER(kPlot.getOwner()).AI_neededWorkers(kPlot.area())));
	} // </advc.007>

	// calc some bonus info
	/*if (GC.getGame().isDebugMode())
		eBonus = kPlot.getBonusType();
	else eBonus = kPlot.getBonusType(GC.getGame().getActiveTeam());*/
	BonusTypes eBonus = kPlot.getBonusType(); // advc.135c: Debug mode is guaranteed
	if (eBonus != NO_BONUS)
	{
		szString.append(CvWString::format(L"\n%s values:", GC.getBonusInfo(eBonus).getDescription()));
		for (int iPlayerIndex = 0; iPlayerIndex < MAX_PLAYERS; iPlayerIndex++)
		{
			CvPlayerAI& kLoopPlayer = GET_PLAYER((PlayerTypes) iPlayerIndex);
			if (kLoopPlayer.isAlive())
			{	/* original code
				szString.append(CvWString::format(L"\n %s: %d", kLoopPlayer.getName(), kLoopPlayer.AI_bonusVal(eBonus)));*/
				// BETTER_BTS_AI_MOD, DEBUG, 07/11/08, jdog5000: START
				BonusTypes eNonObsBonus = kPlot.getNonObsoleteBonusType(kLoopPlayer.getTeam());
				if (eNonObsBonus != NO_BONUS)
				{
					szString.append(CvWString::format(L"\n %s: %d", kLoopPlayer.getName(), kLoopPlayer.AI_bonusVal(eNonObsBonus, 0, true)));
				}
				else
				{
					szString.append(CvWString::format(L"\n %s: unknown (%d)", kLoopPlayer.getName(), kLoopPlayer.AI_bonusVal(eBonus, 0, true)));
				}
				// BETTER_BTS_AI_MOD: END
			}
		}
	}
}
// BULL - Leaderhead Relations - start  // advc: minor style changes
/*  Shows the peace/war/enemy/pact status between eThisTeam and all rivals known to the active player.
	Relations for the active player are shown first. */
void CvGameTextMgr::getAllRelationsString(CvWStringBuffer& szString, TeamTypes eThisTeam) {

	getActiveTeamRelationsString(szString, eThisTeam);
	getOtherRelationsString(szString, eThisTeam, NO_TEAM, GC.getGame().getActiveTeam());
}
// Shows the peace/war/enemy/pact status between eThisTeam and the active player.
void CvGameTextMgr::getActiveTeamRelationsString(CvWStringBuffer& szString, TeamTypes eThisTeam) {

	CvTeamAI const& kThisTeam = GET_TEAM(eThisTeam);
	TeamTypes eActiveTeam = GC.getGame().getActiveTeam();
	if(!kThisTeam.isHasMet(eActiveTeam))
		return;
	CvTeamAI const& kActiveTeam = GET_TEAM(eActiveTeam);

	if(kThisTeam.isAtWar(eActiveTeam)) {
		szString.append(NEWLINE);
		szString.append(gDLL->getText(L"TXT_KEY_AT_WAR_WITH_YOU"));
	}
	// advc: Covered by active deals
	/*else if (kThisTeam.isForcePeace(eActiveTeam)) {
		szString.append(NEWLINE);
		szString.append(gDLL->getText(L"TXT_KEY_PEACE_TREATY_WITH_YOU"));
	}*/
	if(!kThisTeam.isHuman() && kThisTeam.AI_getWorstEnemy() == eActiveTeam) {
		szString.append(NEWLINE);
		szString.append(gDLL->getText(L"TXT_KEY_WORST_ENEMY_IS_YOU"));
	}
	// advc: Covered by active deals
	/*if(kThisTeam.isDefensivePact(eActiveTeam)) {
		szString.append(NEWLINE);
		szString.append(gDLL->getText(L"TXT_KEY_DEFENSIVE_PACT_WITH_YOU"));
	}*/
}

void CvGameTextMgr::getOtherRelationsString(CvWStringBuffer& szString,
		PlayerTypes eThisPlayer, PlayerTypes eOtherPlayer) {

	if(eThisPlayer == NO_PLAYER || eOtherPlayer == NO_PLAYER)
		return;
	getOtherRelationsString(szString, TEAMID(eThisPlayer), TEAMID(eOtherPlayer), NO_TEAM);
}
/*  Shows the peace/war/enemy/pact status between eThisPlayer and all rivals known to the active player.
	If eOtherTeam is not NO_TEAM, only relations between it and eThisTeam are shown.
	If eSkipTeam is not NO_TEAM, relations involving it are not shown. */
/*  advc: This BULL function replaces CvGameTextMgr::getOtherRelationsString(
	CvWStringBuffer&, PlayerTypes, PlayerTypes). I've merged just one K-Mod
	change from the latter. */
void CvGameTextMgr::getOtherRelationsString(CvWStringBuffer& szString,
		TeamTypes eThisTeam, /* (advc: unused) */ TeamTypes eOtherTeam,
		TeamTypes eSkipTeam) {

	if(eThisTeam == NO_TEAM)
		return;
	CvTeamAI const& kThisTeam = GET_TEAM(eThisTeam);
	CvWString szWar, szPeace, szEnemy, szPact;
	bool bFirstWar = true, bFirstPeace = true, bFirstEnemy = true, bFirstPact = true;
	for(int i = 0; i < MAX_CIV_TEAMS; i++) {
		CvTeamAI const& kLoopTeam = GET_TEAM((TeamTypes)i);
		if (!kLoopTeam.isAlive() || kLoopTeam.isMinorCiv() || i == eThisTeam ||
				// K-Mod. (show "at war" even for the civ selected.)  (advc: And war-related info like DP also)
				i == eSkipTeam /*|| (eOtherTeam != NO_TEAM && i != eOtherTeam)*/)
			continue;
		if(!kLoopTeam.isHasMet(eThisTeam) ||
				!kLoopTeam.isHasMet(GC.getGame().getActiveTeam()))
			continue;
		if(::atWar(kLoopTeam.getID(), eThisTeam)) {
			setListHelp(szWar, L"", kLoopTeam.getName().GetCString(), L", ", bFirstWar);
			bFirstWar = false;
		}
		else if(kLoopTeam.isForcePeace(eThisTeam)) {
			setListHelp(szPeace, L"", kLoopTeam.getName().GetCString(), L", ", bFirstPeace);
			bFirstPeace = false;
		}
		if(!kLoopTeam.isHuman() && kLoopTeam.AI_getWorstEnemy() == eThisTeam) {
			setListHelp(szEnemy, L"", kLoopTeam.getName().GetCString(), L", ", bFirstEnemy);
			bFirstEnemy = false;
		}
		if(kLoopTeam.isDefensivePact(eThisTeam)) {
			setListHelp(szPact, L"", kLoopTeam.getName().GetCString(), L", ", bFirstPact);
			bFirstPact = false;
		}
	}
	if(!szWar.empty()) {
		szString.append(NEWLINE);
		szString.append(gDLL->getText(L"TXT_KEY_AT_WAR_WITH", szWar.GetCString()));
	}
	/*  Show only the teams whose enemy ThisTeam is - and not ThisTeam's enemy;
		too confusing to show both. */
	/*if(!kThisTeam.isHuman()) {
		TeamTypes eWorstEnemy = kThisTeam.AI_getWorstEnemy();
		if(eWorstEnemy != NO_TEAM && eWorstEnemy != eSkipTeam &&
				(eOtherTeam == NO_TEAM || eWorstEnemy == eOtherTeam) &&
				GET_TEAM(eWorstEnemy).isHasMet(GC.getGame().getActiveTeam())) {
			szString.append(NEWLINE);
			szString.append(gDLL->getText(L"TXT_KEY_WORST_ENEMY_IS",
					GET_TEAM(eWorstEnemy).getName().GetCString()));
		}
	}*/
	if(!szEnemy.empty()) {
		szString.append(NEWLINE);
		szString.append(gDLL->getText(L"TXT_KEY_WORST_ENEMY_OF", szEnemy.GetCString()));
	}
} // BULL - Leaderhead Relations - end

void CvGameTextMgr::setCityPlotYieldValueString(CvWStringBuffer &szString, CvCity* pCity, int iIndex, bool bIgnoreFood, int iGrowthValue)
{
	PROFILE_FUNC();

	CvPlot* pPlot = NULL;

	if (iIndex >= 0 && iIndex < NUM_CITY_PLOTS)
		pPlot = pCity->getCityIndexPlot(iIndex);

	if (pPlot != NULL && pPlot->getWorkingCity() == pCity)
	{
		CvCityAI const& kCityAI = pCity->AI(); // advc.003
		bool bWorkingPlot = pCity->isWorkingPlot(iIndex);

		int iValue = kCityAI.AI_plotValue(pPlot, bWorkingPlot, bIgnoreFood, false, iGrowthValue);

		setYieldValueString(szString, iValue, /*bActive*/ bWorkingPlot);
	}
	else
		setYieldValueString(szString, 0, /*bActive*/ false, /*bMakeWhitespace*/ true);
}

void CvGameTextMgr::setYieldValueString(CvWStringBuffer &szString, int iValue, bool bActive, bool bMakeWhitespace)
{
	PROFILE_FUNC();

	static bool bUseFloats = false;

	if (bActive)
		szString.append(CvWString::format(SETCOLR, TEXT_COLOR("COLOR_ALT_HIGHLIGHT_TEXT")));
	else
		szString.append(CvWString::format(SETCOLR, TEXT_COLOR("COLOR_HIGHLIGHT_TEXT")));

	if (!bMakeWhitespace)
	{
		if (bUseFloats)
		{
			float fValue = ((float) iValue) / 10000;
			szString.append(CvWString::format(L"%2.3f " ENDCOLR, fValue));
		}
		else
			szString.append(CvWString::format(L"%05d  " ENDCOLR, iValue/10));
	}
	else
		szString.append(CvWString::format(L"           " ENDCOLR));
}

void CvGameTextMgr::setCityBarHelp(CvWStringBuffer &szString, CvCity* pCity)
{
	PROFILE_FUNC();

	int iI;
	CvWString szTempBuffer;

	szString.append(pCity->getName());

	int iFoodDifference = pCity->foodDifference();
	if (iFoodDifference <= 0)
	{
		szString.append(gDLL->getText("TXT_KEY_CITY_BAR_GROWTH",
				pCity->getFood(), pCity->growthThreshold()));
	}
	else
	{
		szString.append(gDLL->getText("TXT_KEY_CITY_BAR_FOOD_GROWTH",
				iFoodDifference, pCity->getFood(), pCity->growthThreshold(),
				pCity->getFoodTurnsLeft()));
	}
	if (pCity->getProductionNeeded() != MAX_INT)
	{
		int iProductionDiffNoFood = pCity->getCurrentProductionDifference(true, true);
		int iProductionDiffJustFood = (pCity->getCurrentProductionDifference(false, true) -
				iProductionDiffNoFood);
		int iTurns = pCity->getProductionTurnsLeft(); // advc.004x
		if (iProductionDiffJustFood > 0)
		{	// <advc.004x>
			if(iTurns == MAX_INT) {
				szString.append(gDLL->getText("TXT_KEY_CITY_BAR_FOOD_HAMMER_NO_PRODUCTION",
						iProductionDiffJustFood, iProductionDiffNoFood,
						pCity->getProductionName(), pCity->getProduction(),
						pCity->getProductionNeeded()));
			}
			else { // </advc.004x>
				szString.append(gDLL->getText("TXT_KEY_CITY_BAR_FOOD_HAMMER_PRODUCTION",
						iProductionDiffJustFood, iProductionDiffNoFood,
						pCity->getProductionName(), pCity->getProduction(),
						pCity->getProductionNeeded(), iTurns));
			}
		}
		else if (iProductionDiffNoFood > 0)
		{	// <advc.004x>
			if(iTurns == MAX_INT) {
				szString.append(gDLL->getText("TXT_KEY_CITY_BAR_HAMMER_NO_PRODUCTION",
						iProductionDiffNoFood, pCity->getProductionName(),
						pCity->getProduction(), pCity->getProductionNeeded()));
			}
			else { // </advc.004x>
				szString.append(gDLL->getText("TXT_KEY_CITY_BAR_HAMMER_PRODUCTION",
						iProductionDiffNoFood, pCity->getProductionName(),
						pCity->getProduction(), pCity->getProductionNeeded(),
						iTurns));
			}
		}
		else
		{
			szString.append(gDLL->getText("TXT_KEY_CITY_BAR_PRODUCTION",
					pCity->getProductionName(), pCity->getProduction(),
					pCity->getProductionNeeded()));
		}
	}

	bool bFirst = true;

	for (iI = 0; iI < NUM_COMMERCE_TYPES; ++iI)
	{
		int iRate = pCity->getCommerceRateTimes100((CommerceTypes)iI);

		if (iRate != 0)
		{
			szTempBuffer.Format(L"%d.%02d %c", iRate/100, iRate%100, GC.getCommerceInfo((CommerceTypes)iI).getChar());
			setListHelp(szString, NEWLINE, szTempBuffer, L", ", bFirst);
			bFirst = false;
		}
	}

	int iRate = pCity->getGreatPeopleRate();

	if (iRate != 0)
	{
		szTempBuffer.Format(L"%d%c", iRate, gDLL->getSymbolID(GREAT_PEOPLE_CHAR));
		setListHelp(szString, NEWLINE, szTempBuffer, L", ", bFirst);
		bFirst = false;
	}

	if (!bFirst)
	{
		szString.append(gDLL->getText("TXT_KEY_PER_TURN"));
	}

	szString.append(NEWLINE);
	szString.append(gDLL->getText("INTERFACE_CITY_MAINTENANCE"));
	//int iMaintenance = pCity->getMaintenanceTimes100();
	int iMaintenance = pCity->getMaintenanceTimes100() * (100+GET_PLAYER(pCity->getOwner()).calculateInflationRate()) / 100; // K-Mod
	szString.append(CvWString::format(L" -%d.%02d %c", iMaintenance/100, iMaintenance%100, GC.getCommerceInfo(COMMERCE_GOLD).getChar()));

	bFirst = true;
	for (iI = 0; iI < GC.getNumBuildingInfos(); ++iI)
	{
		if (pCity->getNumRealBuilding((BuildingTypes)iI) > 0)
		{
			setListHelp(szString, NEWLINE, GC.getBuildingInfo((BuildingTypes)iI).getDescription(), L", ", bFirst);
			bFirst = false;
		}
	}

	if (pCity->getCultureLevel() != NO_CULTURELEVEL)
	{
		szString.append(gDLL->getText("TXT_KEY_CITY_BAR_CULTURE", pCity->getCulture(pCity->getOwner()), pCity->getCultureThreshold(), GC.getCultureLevelInfo(pCity->getCultureLevel()).getTextKeyWide()));
	}

	if (pCity->getGreatPeopleProgress() > 0)
	{
		szString.append(gDLL->getText("TXT_KEY_CITY_BAR_GREAT_PEOPLE", pCity->getGreatPeopleProgress(), GET_PLAYER(pCity->getOwner()).greatPeopleThreshold(false)));
	}

	int iNumUnits = pCity->plot()->countNumAirUnits(GC.getGame().getActiveTeam());
	if (pCity->getAirUnitCapacity(GC.getGame().getActiveTeam()) > 0 && iNumUnits > 0)
	{
		szString.append(NEWLINE);
		szString.append(gDLL->getText("TXT_KEY_CITY_BAR_AIR_UNIT_CAPACITY", iNumUnits, pCity->getAirUnitCapacity(GC.getGame().getActiveTeam())));
	}

	szString.append(NEWLINE);

	szString.append(gDLL->getText("TXT_KEY_CITY_BAR_SELECT", pCity->getNameKey()));
	szString.append(gDLL->getText("TXT_KEY_CITY_BAR_SELECT_CTRL"));
	szString.append(gDLL->getText("TXT_KEY_CITY_BAR_SELECT_ALT"));
}


void CvGameTextMgr::parseTraits(CvWStringBuffer &szHelpString, TraitTypes eTrait, CivilizationTypes eCivilization, bool bDawnOfMan)
{
	PROFILE_FUNC();

	CvWString szTempBuffer;
	BuildingTypes eLoopBuilding;
	UnitTypes eLoopUnit;
	int iI, iJ;
	CvWString szText;

	// Trait Name
	szText = GC.getTraitInfo(eTrait).getDescription();
	if (bDawnOfMan)
	{
		szTempBuffer.Format(L"%s", szText.GetCString());
	}
	else
	{
		szTempBuffer.Format(NEWLINE SETCOLR L"%s" ENDCOLR, TEXT_COLOR("COLOR_ALT_HIGHLIGHT_TEXT"), szText.GetCString());
	}
	szHelpString.append(szTempBuffer);

	if (!bDawnOfMan)
	{
		if (!CvWString(GC.getTraitInfo(eTrait).getHelp()).empty())
		{
			szHelpString.append(GC.getTraitInfo(eTrait).getHelp());
		}

		// iHealth
		if (GC.getTraitInfo(eTrait).getHealth() != 0)
		{
			szHelpString.append(gDLL->getText("TXT_KEY_TRAIT_HEALTH", GC.getTraitInfo(eTrait).getHealth()));
		}

		// iHappiness
		if (GC.getTraitInfo(eTrait).getHappiness() != 0)
		{
			szHelpString.append(gDLL->getText("TXT_KEY_TRAIT_HAPPINESS", GC.getTraitInfo(eTrait).getHappiness()));
		}

		// iMaxAnarchy
		if (GC.getTraitInfo(eTrait).getMaxAnarchy() != -1)
		{
			if (GC.getTraitInfo(eTrait).getMaxAnarchy() == 0)
			{
				szHelpString.append(gDLL->getText("TXT_KEY_TRAIT_NO_ANARCHY"));
			}
			else
			{
				szHelpString.append(gDLL->getText("TXT_KEY_TRAIT_MAX_ANARCHY", GC.getTraitInfo(eTrait).getMaxAnarchy()));
			}
		}

		// iUpkeepModifier
		if (GC.getTraitInfo(eTrait).getUpkeepModifier() != 0)
		{
			szHelpString.append(gDLL->getText("TXT_KEY_TRAIT_CIVIC_UPKEEP_MODIFIER", GC.getTraitInfo(eTrait).getUpkeepModifier()));
		}

		// iLevelExperienceModifier
		if (GC.getTraitInfo(eTrait).getLevelExperienceModifier() != 0)
		{
			szHelpString.append(gDLL->getText("TXT_KEY_TRAIT_CIVIC_LEVEL_MODIFIER", GC.getTraitInfo(eTrait).getLevelExperienceModifier()));
		}

		// iGreatPeopleRateModifier
		if (GC.getTraitInfo(eTrait).getGreatPeopleRateModifier() != 0)
		{
			szHelpString.append(gDLL->getText("TXT_KEY_TRAIT_GREAT_PEOPLE_MODIFIER", GC.getTraitInfo(eTrait).getGreatPeopleRateModifier()));
		}

		// iGreatGeneralRateModifier
		if (GC.getTraitInfo(eTrait).getGreatGeneralRateModifier() != 0)
		{
			szHelpString.append(gDLL->getText("TXT_KEY_TRAIT_GREAT_GENERAL_MODIFIER", GC.getTraitInfo(eTrait).getGreatGeneralRateModifier()));
		}

		if (GC.getTraitInfo(eTrait).getDomesticGreatGeneralRateModifier() != 0)
		{
			szHelpString.append(gDLL->getText("TXT_KEY_TRAIT_DOMESTIC_GREAT_GENERAL_MODIFIER", GC.getTraitInfo(eTrait).getDomesticGreatGeneralRateModifier()));
		}

		// Wonder Production Effects
		if ((GC.getTraitInfo(eTrait).getMaxGlobalBuildingProductionModifier() != 0)
			|| (GC.getTraitInfo(eTrait).getMaxTeamBuildingProductionModifier() != 0)
			|| (GC.getTraitInfo(eTrait).getMaxPlayerBuildingProductionModifier() != 0))
		{
			if ((GC.getTraitInfo(eTrait).getMaxGlobalBuildingProductionModifier() == GC.getTraitInfo(eTrait).getMaxTeamBuildingProductionModifier())
				&& 	(GC.getTraitInfo(eTrait).getMaxGlobalBuildingProductionModifier() == GC.getTraitInfo(eTrait).getMaxPlayerBuildingProductionModifier()))
			{
				szHelpString.append(gDLL->getText("TXT_KEY_TRAIT_WONDER_PRODUCTION_MODIFIER", GC.getTraitInfo(eTrait).getMaxGlobalBuildingProductionModifier()));
			}
			else
			{
				if (GC.getTraitInfo(eTrait).getMaxGlobalBuildingProductionModifier() != 0)
				{
					szHelpString.append(gDLL->getText("TXT_KEY_TRAIT_WORLD_WONDER_PRODUCTION_MODIFIER", GC.getTraitInfo(eTrait).getMaxGlobalBuildingProductionModifier()));
				}

				if (GC.getTraitInfo(eTrait).getMaxTeamBuildingProductionModifier() != 0)
				{
					szHelpString.append(gDLL->getText("TXT_KEY_TRAIT_TEAM_WONDER_PRODUCTION_MODIFIER", GC.getTraitInfo(eTrait).getMaxTeamBuildingProductionModifier()));
				}

				if (GC.getTraitInfo(eTrait).getMaxPlayerBuildingProductionModifier() != 0)
				{
					szHelpString.append(gDLL->getText("TXT_KEY_TRAIT_NATIONAL_WONDER_PRODUCTION_MODIFIER", GC.getTraitInfo(eTrait).getMaxPlayerBuildingProductionModifier()));
				}
			}
		}

		// ExtraYieldThresholds
		for (iI = 0; iI < NUM_YIELD_TYPES; ++iI)
		{	// advc.908a:
			int iExtraYieldThresh = GC.getTraitInfo(eTrait).getExtraYieldThreshold(iI);
			int iYieldChar = GC.getYieldInfo((YieldTypes) iI).getChar();
			if(iExtraYieldThresh > 0) {
				szHelpString.append(gDLL->getText("TXT_KEY_TRAIT_EXTRA_YIELD_THRESHOLDS", GC.getYieldInfo((YieldTypes) iI).getChar(),
						iExtraYieldThresh, iYieldChar,
						iExtraYieldThresh + 1, iYieldChar)); // advc.908a
			}
			// Trade Yield Modifiers
			if (GC.getTraitInfo(eTrait).getTradeYieldModifier(iI) != 0)
			{
				szHelpString.append(gDLL->getText("TXT_KEY_TRAIT_TRADE_YIELD_MODIFIERS", GC.getTraitInfo(eTrait).getTradeYieldModifier(iI), GC.getYieldInfo((YieldTypes) iI).getChar(), "YIELD"));
			}
		}

		// CommerceChanges
		for (iI = 0; iI < NUM_COMMERCE_TYPES; ++iI)
		{
			if (GC.getTraitInfo(eTrait).getCommerceChange(iI) != 0)
			{
				szHelpString.append(gDLL->getText("TXT_KEY_TRAIT_COMMERCE_CHANGES", 	GC.getTraitInfo(eTrait).getCommerceChange(iI), GC.getCommerceInfo((CommerceTypes) iI).getChar(), "COMMERCE"));
			}

			if (GC.getTraitInfo(eTrait).getCommerceModifier(iI) != 0)
			{
				szHelpString.append(gDLL->getText("TXT_KEY_TRAIT_COMMERCE_MODIFIERS", GC.getTraitInfo(eTrait).getCommerceModifier(iI), GC.getCommerceInfo((CommerceTypes) iI).getChar(), "COMMERCE"));
			}
		}

		// Free Promotions
		bool bFoundPromotion = false;
		szTempBuffer.clear();
		for (iI = 0; iI < GC.getNumPromotionInfos(); ++iI)
		{
			if (GC.getTraitInfo(eTrait).isFreePromotion(iI))
			{
				if (bFoundPromotion)
				{
					szTempBuffer += L", ";
				}

				szTempBuffer += CvWString::format(L"<link=literal>%s</link>", GC.getPromotionInfo((PromotionTypes) iI).getDescription());
				bFoundPromotion = true;
			}
		}

		if (bFoundPromotion)
		{
			szHelpString.append(gDLL->getText("TXT_KEY_TRAIT_FREE_PROMOTIONS", szTempBuffer.GetCString()));

			for (iJ = 0; iJ < GC.getNumUnitCombatInfos(); iJ++)
			{
				if (GC.getTraitInfo(eTrait).isFreePromotionUnitCombat(iJ))
				{
					szTempBuffer.Format(L"\n        %c<link=literal>%s</link>", gDLL->getSymbolID(BULLET_CHAR), GC.getUnitCombatInfo((UnitCombatTypes)iJ).getDescription());
					szHelpString.append(szTempBuffer);
				}
			}
		}

		// No Civic Maintenance
		for (iI = 0; iI < GC.getNumCivicOptionInfos(); ++iI)
		{
			if (GC.getCivicOptionInfo((CivicOptionTypes) iI).getTraitNoUpkeep(eTrait))
			{
				szHelpString.append(gDLL->getText("TXT_KEY_TRAIT_NO_UPKEEP", GC.getCivicOptionInfo((CivicOptionTypes)iI).getTextKeyWide()));
			}
		}

		// Increase Building/Unit Production Speeds
		int iLast = 0;
		for (iI = 0; iI < GC.getNumSpecialUnitInfos(); ++iI)
		{
			if (GC.getSpecialUnitInfo((SpecialUnitTypes) iI).getProductionTraits(eTrait) != 0)
			{
				if (GC.getSpecialUnitInfo((SpecialUnitTypes) iI).getProductionTraits(eTrait) == 100)
				{
					szText = gDLL->getText("TXT_KEY_TRAIT_DOUBLE_SPEED");
				}
				else
				{
					szText = gDLL->getText("TXT_KEY_TRAIT_PRODUCTION_MODIFIER", GC.getSpecialUnitInfo((SpecialUnitTypes) iI).getProductionTraits(eTrait));
				}
				setListHelp(szHelpString, szText.GetCString(), GC.getSpecialUnitInfo((SpecialUnitTypes) iI).getDescription(), L", ", (GC.getSpecialUnitInfo((SpecialUnitTypes) iI).getProductionTraits(eTrait) != iLast));
				iLast = GC.getSpecialUnitInfo((SpecialUnitTypes) iI).getProductionTraits(eTrait);
			}
		}

		// Unit Classes
		iLast = 0;
		for (iI = 0; iI < GC.getNumUnitClassInfos();++iI)
		{
			if (eCivilization == NO_CIVILIZATION)
			{
				eLoopUnit = ((UnitTypes)(GC.getUnitClassInfo((UnitClassTypes)iI).getDefaultUnitIndex()));
			}
			else
			{
				eLoopUnit = ((UnitTypes)(GC.getCivilizationInfo(eCivilization).getCivilizationUnits(iI)));
			}

			if (eLoopUnit != NO_UNIT && !isWorldUnitClass((UnitClassTypes)iI))
			{
				if (GC.getUnitInfo(eLoopUnit).getProductionTraits(eTrait) != 0)
				{
					if (GC.getUnitInfo(eLoopUnit).getProductionTraits(eTrait) == 100)
					{
						szText = gDLL->getText("TXT_KEY_TRAIT_DOUBLE_SPEED");
					}
					else
					{
						szText = gDLL->getText("TXT_KEY_TRAIT_PRODUCTION_MODIFIER", GC.getUnitInfo(eLoopUnit).getProductionTraits(eTrait));
					}
					CvWString szUnit;
					szUnit.Format(L"<link=literal>%s</link>", GC.getUnitInfo(eLoopUnit).getDescription());
					setListHelp(szHelpString, szText.GetCString(), szUnit, L", ", (GC.getUnitInfo(eLoopUnit).getProductionTraits(eTrait) != iLast));
					iLast = GC.getUnitInfo(eLoopUnit).getProductionTraits(eTrait);
				}
			}
		}

		// SpecialBuildings
		iLast = 0;
		for (iI = 0; iI < GC.getNumSpecialBuildingInfos(); ++iI)
		{
			if (GC.getSpecialBuildingInfo((SpecialBuildingTypes) iI).getProductionTraits(eTrait) != 0)
			{
				if (GC.getSpecialBuildingInfo((SpecialBuildingTypes) iI).getProductionTraits(eTrait) == 100)
				{
					szText = gDLL->getText("TXT_KEY_TRAIT_DOUBLE_SPEED");
				}
				else
				{
					szText = gDLL->getText("TXT_KEY_TRAIT_PRODUCTION_MODIFIER", GC.getSpecialBuildingInfo((SpecialBuildingTypes) iI).getProductionTraits(eTrait));
				}
				setListHelp(szHelpString, szText.GetCString(), GC.getSpecialBuildingInfo((SpecialBuildingTypes) iI).getDescription(), L", ", (GC.getSpecialBuildingInfo((SpecialBuildingTypes) iI).getProductionTraits(eTrait) != iLast));
				iLast = GC.getSpecialBuildingInfo((SpecialBuildingTypes) iI).getProductionTraits(eTrait);
			}
		}

		// Buildings
		iLast = 0;
		for (iI = 0; iI < GC.getNumBuildingClassInfos(); ++iI)
		{
			if (eCivilization == NO_CIVILIZATION)
			{
				eLoopBuilding = ((BuildingTypes)(GC.getBuildingClassInfo((BuildingClassTypes)iI).getDefaultBuildingIndex()));
			}
			else
			{
				eLoopBuilding = ((BuildingTypes)(GC.getCivilizationInfo(eCivilization).getCivilizationBuildings(iI)));
			}

			if (eLoopBuilding != NO_BUILDING && !isWorldWonderClass((BuildingClassTypes)iI))
			{
				if (GC.getBuildingInfo(eLoopBuilding).getProductionTraits(eTrait) != 0)
				{
					if (GC.getBuildingInfo(eLoopBuilding).getProductionTraits(eTrait) == 100)
					{
						szText = gDLL->getText("TXT_KEY_TRAIT_DOUBLE_SPEED");
					}
					else
					{
						szText = gDLL->getText("TXT_KEY_TRAIT_PRODUCTION_MODIFIER", GC.getBuildingInfo(eLoopBuilding).getProductionTraits(eTrait));
					}

					CvWString szBuilding;
					szBuilding.Format(L"<link=literal>%s</link>", GC.getBuildingInfo(eLoopBuilding).getDescription());
					setListHelp(szHelpString, szText.GetCString(), szBuilding, L", ", (GC.getBuildingInfo(eLoopBuilding).getProductionTraits(eTrait) != iLast));
					iLast = GC.getBuildingInfo(eLoopBuilding).getProductionTraits(eTrait);
				}
			}
		}

		// Buildings
		iLast = 0;
		for (iI = 0; iI < GC.getNumBuildingClassInfos(); ++iI)
		{
			if (eCivilization == NO_CIVILIZATION)
			{
				eLoopBuilding = ((BuildingTypes)(GC.getBuildingClassInfo((BuildingClassTypes)iI).getDefaultBuildingIndex()));
			}
			else
			{
				eLoopBuilding = ((BuildingTypes)(GC.getCivilizationInfo(eCivilization).getCivilizationBuildings(iI)));
			}

			if (eLoopBuilding != NO_BUILDING && !isWorldWonderClass((BuildingClassTypes)iI))
			{
				int iHappiness = GC.getBuildingInfo(eLoopBuilding).getHappinessTraits(eTrait);
				if (iHappiness != 0)
				{
					if (iHappiness > 0)
					{
						szText = gDLL->getText("TXT_KEY_TRAIT_BUILDING_HAPPINESS", iHappiness, gDLL->getSymbolID(HAPPY_CHAR));
					}
					else
					{
						szText = gDLL->getText("TXT_KEY_TRAIT_BUILDING_HAPPINESS", -iHappiness, gDLL->getSymbolID(UNHAPPY_CHAR));
					}

					CvWString szBuilding;
					szBuilding.Format(L"<link=literal>%s</link>", GC.getBuildingInfo(eLoopBuilding).getDescription());
					setListHelp(szHelpString, szText.GetCString(), szBuilding, L", ", (iHappiness != iLast));
					iLast = iHappiness;
				}
			}
		}

	}

//	return szHelpString;
}


//
// parseLeaderTraits - SimpleCivPicker							// LOCALIZATION READY
//
void CvGameTextMgr::parseLeaderTraits(CvWStringBuffer &szHelpString, LeaderHeadTypes eLeader, CivilizationTypes eCivilization, bool bDawnOfMan, bool bCivilopediaText)
{
	PROFILE_FUNC();

	CvWString szTempBuffer;	// Formatting
	int iI;

	//	Build help string
	if (eLeader != NO_LEADER)
	{
		if (!bDawnOfMan && !bCivilopediaText)
		{
			szTempBuffer.Format( SETCOLR L"%s" ENDCOLR , TEXT_COLOR("COLOR_HIGHLIGHT_TEXT"), GC.getLeaderHeadInfo(eLeader).getDescription());
			szHelpString.append(szTempBuffer);
		}

		FAssertMsg(GC.getNumTraitInfos() > 0, "GC.getNumTraitInfos() is less than or equal to zero but is expected to be larger than zero in CvSimpleCivPicker::setLeaderText");

		bool bFirst = true;
		for (iI = 0; iI < GC.getNumTraitInfos(); ++iI)
		{
			if (GC.getLeaderHeadInfo(eLeader).hasTrait(iI))
			{
				if (!bFirst)
				{
					if (bDawnOfMan)
					{
						szHelpString.append(L", ");
					}
				}
				else
				{
					bFirst = false;
				}
				parseTraits(szHelpString, ((TraitTypes)iI), eCivilization, bDawnOfMan);
			}
		}
	}
	else
	{
		//	Random leader
		szTempBuffer.Format( SETCOLR L"%s" ENDCOLR , TEXT_COLOR("COLOR_HIGHLIGHT_TEXT"), gDLL->getText("TXT_KEY_TRAIT_PLAYER_UNKNOWN").c_str());
		szHelpString.append(szTempBuffer);
	}

//	return szHelpString;
}

//
// parseLeaderTraits - SimpleCivPicker							// LOCALIZATION READY
//
void CvGameTextMgr::parseLeaderShortTraits(CvWStringBuffer &szHelpString, LeaderHeadTypes eLeader)
{
	PROFILE_FUNC();

	int iI;

	//	Build help string
	if (eLeader != NO_LEADER)
	{
		FAssertMsg(GC.getNumTraitInfos() > 0, "GC.getNumTraitInfos() is less than or equal to zero but is expected to be larger than zero in CvSimpleCivPicker::setLeaderText");

		bool bFirst = true;
		for (iI = 0; iI < GC.getNumTraitInfos(); ++iI)
		{
			if (GC.getLeaderHeadInfo(eLeader).hasTrait(iI))
			{
				if (!bFirst)
				{
					szHelpString.append(L"/");
				}
				szHelpString.append(gDLL->getText(GC.getTraitInfo((TraitTypes)iI).getShortDescription()));
				bFirst = false;
			}
		}
	}
	else
	{
		//	Random leader
		szHelpString.append(CvWString("???/???"));
	}

	//	return szHelpString;
}

//
// Build Civilization Info Help Text
//
void CvGameTextMgr::parseCivInfos(CvWStringBuffer &szInfoText, CivilizationTypes eCivilization, bool bDawnOfMan, bool bLinks)
{
	PROFILE_FUNC();

	CvWString szBuffer;
	CvWString szTempString;
	CvWString szText;
	UnitTypes eDefaultUnit;
	UnitTypes eUniqueUnit;
	BuildingTypes eDefaultBuilding;
	BuildingTypes eUniqueBuilding;

	if (eCivilization != NO_CIVILIZATION)
	{
		if (!bDawnOfMan)
		{
			// Civ Name
			szBuffer.Format(SETCOLR L"%s" ENDCOLR , TEXT_COLOR("COLOR_HIGHLIGHT_TEXT"), GC.getCivilizationInfo(eCivilization).getDescription());
			szInfoText.append(szBuffer);

			// Free Techs
			szBuffer.Format(NEWLINE SETCOLR L"%s" ENDCOLR , TEXT_COLOR("COLOR_ALT_HIGHLIGHT_TEXT"), gDLL->getText("TXT_KEY_FREE_TECHS").GetCString());
			szInfoText.append(szBuffer);

			bool bFound = false;
			for (int iI = 0; iI < GC.getNumTechInfos(); ++iI)
			{
				if (GC.getCivilizationInfo(eCivilization).isCivilizationFreeTechs(iI))
				{
					bFound = true;
					// Add Tech
					szText.Format((bLinks ? L"<link=literal>%s</link>" : L"%s"), GC.getTechInfo((TechTypes)iI).getDescription());
					szBuffer.Format(L"%s  %c%s", NEWLINE, gDLL->getSymbolID(BULLET_CHAR), szText.GetCString());
					szInfoText.append(szBuffer);
				}
			}

			if (!bFound)
			{
				szBuffer.Format(L"%s  %s", NEWLINE, gDLL->getText("TXT_KEY_FREE_TECHS_NO").GetCString());
				szInfoText.append(szBuffer);
			}
		}

		// Free Units
		szText = gDLL->getText("TXT_KEY_FREE_UNITS");
		if (bDawnOfMan)
		{
			szTempString.Format(L"%s: ", szText.GetCString());
		}
		else
		{
			szTempString.Format(NEWLINE SETCOLR L"%s" ENDCOLR , TEXT_COLOR("COLOR_ALT_HIGHLIGHT_TEXT"), szText.GetCString());
		}
		szInfoText.append(szTempString);

		bool bFound = false;
		for (int iI = 0; iI < GC.getNumUnitClassInfos(); ++iI)
		{
			eDefaultUnit = ((UnitTypes)(GC.getCivilizationInfo(eCivilization).getCivilizationUnits(iI)));
			eUniqueUnit = ((UnitTypes)(GC.getUnitClassInfo((UnitClassTypes) iI).getDefaultUnitIndex()));
			if ((eDefaultUnit != NO_UNIT) && (eUniqueUnit != NO_UNIT))
			{
				if (eDefaultUnit != eUniqueUnit)
				{
					// Add Unit
					if (bDawnOfMan)
					{
						if (bFound)
						{
							szInfoText.append(L", ");
						}
						szBuffer.Format((bLinks ? L"<link=literal>%s</link> - (<link=literal>%s</link>)" : L"%s - (%s)"),
							GC.getUnitInfo(eDefaultUnit).getDescription(),
							GC.getUnitInfo(eUniqueUnit).getDescription());
					}
					else
					{
						szBuffer.Format(L"\n  %c%s - (%s)", gDLL->getSymbolID(BULLET_CHAR),
							GC.getUnitInfo(eDefaultUnit).getDescription(),
							GC.getUnitInfo(eUniqueUnit).getDescription());
					}
					szInfoText.append(szBuffer);
					bFound = true;
				}
			}
		}

		if (!bFound)
		{
			szText = gDLL->getText("TXT_KEY_FREE_UNITS_NO");
			if (bDawnOfMan)
			{
				szTempString.Format(L"%s", szText.GetCString());
			}
			else
			{
				szTempString.Format(L"%s  %s", NEWLINE, szText.GetCString());
			}
			szInfoText.append(szTempString);
			bFound = true;
		}


		// Free Buildings
		szText = gDLL->getText("TXT_KEY_UNIQUE_BUILDINGS");
		if (bDawnOfMan)
		{
			if (bFound)
			{
				szInfoText.append(NEWLINE);
			}
			szTempString.Format(L"%s: ", szText.GetCString());
		}
		else
		{
			szTempString.Format(NEWLINE SETCOLR L"%s" ENDCOLR , TEXT_COLOR("COLOR_ALT_HIGHLIGHT_TEXT"), szText.GetCString());
		}
		szInfoText.append(szTempString);

		bFound = false;
		for (int iI = 0; iI < GC.getNumBuildingClassInfos(); ++iI)
		{
			eDefaultBuilding = ((BuildingTypes)(GC.getCivilizationInfo(eCivilization).getCivilizationBuildings(iI)));
			eUniqueBuilding = ((BuildingTypes)(GC.getBuildingClassInfo((BuildingClassTypes) iI).getDefaultBuildingIndex()));
			if ((eDefaultBuilding != NO_BUILDING) && (eUniqueBuilding != NO_BUILDING))
			{
				if (eDefaultBuilding != eUniqueBuilding)
				{
					// Add Building
					if (bDawnOfMan)
					{
						if (bFound)
						{
							szInfoText.append(L", ");
						}
						szBuffer.Format((bLinks ? L"<link=literal>%s</link> - (<link=literal>%s</link>)" : L"%s - (%s)"),
							GC.getBuildingInfo(eDefaultBuilding).getDescription(),
							GC.getBuildingInfo(eUniqueBuilding).getDescription());
					}
					else
					{
						szBuffer.Format(L"\n  %c%s - (%s)", gDLL->getSymbolID(BULLET_CHAR),
							GC.getBuildingInfo(eDefaultBuilding).getDescription(),
							GC.getBuildingInfo(eUniqueBuilding).getDescription());
					}
					szInfoText.append(szBuffer);
					bFound = true;
				}
			}
		}
		if (!bFound)
		{
			szText = gDLL->getText("TXT_KEY_UNIQUE_BUILDINGS_NO");
			if (bDawnOfMan)
			{
				szTempString.Format(L"%s", szText.GetCString());
			}
			else
			{
				szTempString.Format(L"%s  %s", NEWLINE, szText.GetCString());
			}
			szInfoText.append(szTempString);
		}
	}
	else
	{
		//	This is a random civ, let us know here...
		szInfoText.append(gDLL->getText("TXT_KEY_CIV_UNKNOWN"));
	}

//	return szInfoText;
}

void CvGameTextMgr::parseSpecialistHelp(CvWStringBuffer &szHelpString, SpecialistTypes eSpecialist, CvCity* pCity, bool bCivilopediaText)
{
	PROFILE_FUNC();

	CvWString szText;
	int aiYields[NUM_YIELD_TYPES];
	int aiCommerces[NUM_COMMERCE_TYPES];
	int iI;

	const CvSpecialistInfo& kInfo = GC.getSpecialistInfo(eSpecialist); // K-Mod

	if (eSpecialist != NO_SPECIALIST)
	{
		if (!bCivilopediaText)
		{
			szHelpString.append(kInfo.getDescription());
		}

		for (iI = 0; iI < NUM_YIELD_TYPES; ++iI)
		{
			if (GC.getGame().getActivePlayer() == NO_PLAYER)
			{
				aiYields[iI] = kInfo.getYieldChange(iI);
			}
			else
			{
				aiYields[iI] = GET_PLAYER((pCity != NULL) ? pCity->getOwner() : GC.getGame().getActivePlayer()).specialistYield(eSpecialist, ((YieldTypes)iI));
			}
		}

		setYieldChangeHelp(szHelpString, L"", L"", L"", aiYields);

		for (iI = 0; iI < NUM_COMMERCE_TYPES; ++iI)
		{
			if (GC.getGame().getActivePlayer() == NO_PLAYER)
			{
				aiCommerces[iI] = kInfo.getCommerceChange(iI);
			}
			else
			{
				aiCommerces[iI] = GET_PLAYER((pCity != NULL) ? pCity->getOwner() : GC.getGame().getActivePlayer()).specialistCommerce(((SpecialistTypes)eSpecialist), ((CommerceTypes)iI));
			}
		}

		setCommerceChangeHelp(szHelpString, L"", L"", L"", aiCommerces);

		if (kInfo.getExperience() > 0)
		{
			szHelpString.append(NEWLINE);
			szHelpString.append(gDLL->getText("TXT_KEY_SPECIALIST_EXPERIENCE", kInfo.getExperience()));
		}

		if (kInfo.getGreatPeopleRateChange() != 0)
		{
			szHelpString.append(NEWLINE);
			szHelpString.append(gDLL->getText("TXT_KEY_SPECIALIST_BIRTH_RATE", kInfo.getGreatPeopleRateChange()));

			// K-Mod
			if (!bCivilopediaText &&
					//gDLL->getChtLvl() > 0
					GC.getGame().isDebugMode() // advc.135c
					&& GC.ctrlKey())
			{
				szHelpString.append(NEWLINE);
				szHelpString.append(CvWString::format(L"weight: %d", GET_PLAYER((pCity != NULL) ? pCity->getOwner() : GC.getGame().getActivePlayer()).AI_getGreatPersonWeight((UnitClassTypes)kInfo.getGreatPeopleUnitClass())));
			}
			// K-Mod end
		}

// BUG - Specialist Actual Effects - start
		if (pCity && (GC.altKey() || getBugOptionBOOL("MiscHover__SpecialistActualEffects", false)) &&
				(pCity->getOwner() == GC.getGame().getActivePlayer() ||
				//gDLL->getChtLvl() > 0))
				GC.getGame().isDebugMode())) // advc.135c
		{
			bool bStarted = false;
			CvWString szStart;
			szStart.Format(L"\n"SETCOLR L"(%s", TEXT_COLOR("COLOR_LIGHT_GREY"), gDLL->getText("TXT_KEY_ACTUAL_EFFECTS").GetCString());

			// Yield
			for (iI = 0; iI < NUM_YIELD_TYPES; ++iI)
			{
				aiYields[iI] = pCity->getAdditionalYieldBySpecialist((YieldTypes)iI, eSpecialist);
			}
			bStarted = setResumableYieldChangeHelp(szHelpString, szStart, L": ", L"", aiYields, false, false, bStarted);

			// Commerce
			for (iI = 0; iI < NUM_COMMERCE_TYPES; ++iI)
			{
				aiCommerces[iI] = pCity->getAdditionalCommerceTimes100BySpecialist((CommerceTypes)iI, eSpecialist);
			}
			bStarted = setResumableCommerceTimes100ChangeHelp(szHelpString, szStart, L": ", L"", aiCommerces, false, bStarted);

			// Great People
			int iGreatPeopleRate = pCity->getAdditionalGreatPeopleRateBySpecialist(eSpecialist);
			bStarted = setResumableValueChangeHelp(szHelpString, szStart, L": ", L"", iGreatPeopleRate, gDLL->getSymbolID(GREAT_PEOPLE_CHAR), false, false, bStarted);

			if (bStarted)
				szHelpString.append(L")" ENDCOLR);
		}
// BUG - Specialist Actual Effects - end

		if (!CvWString(kInfo.getHelp()).empty() && !bCivilopediaText)
		{
			szHelpString.append(NEWLINE);
			szHelpString.append(kInfo.getHelp());
		}
	}
}

void CvGameTextMgr::parseFreeSpecialistHelp(CvWStringBuffer &szHelpString, const CvCity& kCity)
{
	PROFILE_FUNC();

	for (int iLoopSpecialist = 0; iLoopSpecialist < GC.getNumSpecialistInfos(); iLoopSpecialist++)
	{
		SpecialistTypes eSpecialist = (SpecialistTypes)iLoopSpecialist;
		int iNumSpecialists = kCity.getFreeSpecialistCount(eSpecialist);

		if (iNumSpecialists > 0)
		{
			int aiYields[NUM_YIELD_TYPES];
			int aiCommerces[NUM_COMMERCE_TYPES];

			szHelpString.append(NEWLINE);
			szHelpString.append(CvWString::format(L"%s (%d): ", GC.getSpecialistInfo(eSpecialist).getDescription(), iNumSpecialists));

			for (int iI = 0; iI < NUM_YIELD_TYPES; ++iI)
			{
				aiYields[iI] = iNumSpecialists * GET_PLAYER(kCity.getOwner()).specialistYield(eSpecialist, ((YieldTypes)iI));
			}

			CvWStringBuffer szYield;
			setYieldChangeHelp(szYield, L"", L"", L"", aiYields, false, false);
			szHelpString.append(szYield);

			for (int iI = 0; iI < NUM_COMMERCE_TYPES; ++iI)
			{
				aiCommerces[iI] = iNumSpecialists * GET_PLAYER(kCity.getOwner()).specialistCommerce(eSpecialist, ((CommerceTypes)iI));
			}

			CvWStringBuffer szCommerceString;
			setCommerceChangeHelp(szCommerceString, L"", L"", L"", aiCommerces, false, false);
			if (!szYield.isEmpty() && !szCommerceString.isEmpty())
			{
				szHelpString.append(L", ");
			}
			szHelpString.append(szCommerceString);

			if (GC.getSpecialistInfo(eSpecialist).getExperience() > 0)
			{
				if (!szYield.isEmpty() || !szCommerceString.isEmpty()) // kmodx: was !szYield.isEmpty()
				{
					szHelpString.append(L", ");
				}
				szHelpString.append(gDLL->getText("TXT_KEY_SPECIALIST_EXPERIENCE_SHORT", iNumSpecialists * GC.getSpecialistInfo(eSpecialist).getExperience()));
			}
		}
	}
}


//
// Promotion Help
//
void CvGameTextMgr::parsePromotionHelp(CvWStringBuffer &szBuffer, PromotionTypes ePromotion, const wchar* pcNewline)
{
	PROFILE_FUNC();

	CvWString szText, szText2;
	int iI;

	if (NO_PROMOTION == ePromotion)
		return;

	//if (GC.getPromotionInfo(ePromotion).isBlitz())
	// <advc.164>
	int iBlitz = GC.getPromotionInfo(ePromotion).getBlitz();
	if(iBlitz != 0) { // </advc.164>
		szBuffer.append(pcNewline);
		// <advc.164>
		if(iBlitz > 0) {
			if(iBlitz > 1)
				szBuffer.append(gDLL->getText("TXT_KEY_PROMOTION_BLITZ_LIMIT", iBlitz + 1));
			else szBuffer.append(gDLL->getText("TXT_KEY_PROMOTION_BLITZ_TWICE"));
		}
		else // </advc.164>
			szBuffer.append(gDLL->getText("TXT_KEY_PROMOTION_BLITZ_TEXT"));
	}

	if (GC.getPromotionInfo(ePromotion).isAmphib())
	{
		szBuffer.append(pcNewline);
		szBuffer.append(gDLL->getText("TXT_KEY_PROMOTION_AMPHIB_TEXT"));
	}

	if (GC.getPromotionInfo(ePromotion).isRiver())
	{
		szBuffer.append(pcNewline);
		szBuffer.append(gDLL->getText("TXT_KEY_PROMOTION_RIVER_ATTACK_TEXT"));
	}

	if (GC.getPromotionInfo(ePromotion).isEnemyRoute())
	{
		szBuffer.append(pcNewline);
		szBuffer.append(gDLL->getText("TXT_KEY_PROMOTION_ENEMY_ROADS_TEXT"));
	}

	if (GC.getPromotionInfo(ePromotion).isAlwaysHeal())
	{
		szBuffer.append(pcNewline);
		szBuffer.append(gDLL->getText("TXT_KEY_PROMOTION_ALWAYS_HEAL_TEXT"));
	}

	if (GC.getPromotionInfo(ePromotion).isHillsDoubleMove())
	{
		szBuffer.append(pcNewline);
		szBuffer.append(gDLL->getText("TXT_KEY_PROMOTION_HILLS_MOVE_TEXT"));
	}

	if (GC.getPromotionInfo(ePromotion).isImmuneToFirstStrikes())
	{
		szBuffer.append(pcNewline);
		szBuffer.append(gDLL->getText("TXT_KEY_PROMOTION_IMMUNE_FIRST_STRIKES_TEXT"));
	}

	for (iI = 0; iI < GC.getNumTerrainInfos(); ++iI)
	{
		if (GC.getPromotionInfo(ePromotion).getTerrainDoubleMove(iI))
		{
			szBuffer.append(pcNewline);
			szBuffer.append(gDLL->getText("TXT_KEY_PROMOTION_DOUBLE_MOVE_TEXT", GC.getTerrainInfo((TerrainTypes) iI).getTextKeyWide()));
		}
	}

	for (iI = 0; iI < GC.getNumFeatureInfos(); ++iI)
	{
		if (GC.getPromotionInfo(ePromotion).getFeatureDoubleMove(iI))
		{
			szBuffer.append(pcNewline);
			szBuffer.append(gDLL->getText("TXT_KEY_PROMOTION_DOUBLE_MOVE_TEXT", GC.getFeatureInfo((FeatureTypes) iI).getTextKeyWide()));
		}
	}

	if (GC.getPromotionInfo(ePromotion).getVisibilityChange() != 0)
	{
		szBuffer.append(pcNewline);
		szBuffer.append(gDLL->getText("TXT_KEY_PROMOTION_VISIBILITY_TEXT", GC.getPromotionInfo(ePromotion).getVisibilityChange()));
	}

	if (GC.getPromotionInfo(ePromotion).getMovesChange() != 0)
	{
		szBuffer.append(pcNewline);
		szBuffer.append(gDLL->getText("TXT_KEY_PROMOTION_MOVE_TEXT", GC.getPromotionInfo(ePromotion).getMovesChange()));
	}

	if (GC.getPromotionInfo(ePromotion).getMoveDiscountChange() != 0)
	{
		szBuffer.append(pcNewline);
		szBuffer.append(gDLL->getText("TXT_KEY_PROMOTION_MOVE_DISCOUNT_TEXT", -(GC.getPromotionInfo(ePromotion).getMoveDiscountChange())));
	}

	if (GC.getPromotionInfo(ePromotion).getAirRangeChange() != 0)
	{
		szBuffer.append(pcNewline);
		szBuffer.append(gDLL->getText("TXT_KEY_PROMOTION_AIR_RANGE_TEXT", GC.getPromotionInfo(ePromotion).getAirRangeChange()));
	}

	if (GC.getPromotionInfo(ePromotion).getInterceptChange() != 0)
	{
		szBuffer.append(pcNewline);
		szBuffer.append(gDLL->getText("TXT_KEY_PROMOTION_INTERCEPT_TEXT", GC.getPromotionInfo(ePromotion).getInterceptChange()));
	}

	if (GC.getPromotionInfo(ePromotion).getEvasionChange() != 0)
	{
		szBuffer.append(pcNewline);
		szBuffer.append(gDLL->getText("TXT_KEY_PROMOTION_EVASION_TEXT", GC.getPromotionInfo(ePromotion).getEvasionChange()));
	}

	if (GC.getPromotionInfo(ePromotion).getWithdrawalChange() != 0)
	{
		szBuffer.append(pcNewline);
		szBuffer.append(gDLL->getText("TXT_KEY_PROMOTION_WITHDRAWAL_TEXT", GC.getPromotionInfo(ePromotion).getWithdrawalChange()));
	}

	if (GC.getPromotionInfo(ePromotion).getCargoChange() != 0)
	{
		szBuffer.append(pcNewline);
		szBuffer.append(gDLL->getText("TXT_KEY_PROMOTION_CARGO_TEXT", GC.getPromotionInfo(ePromotion).getCargoChange()));
	}

	if (GC.getPromotionInfo(ePromotion).getCollateralDamageChange() != 0)
	{
		szBuffer.append(pcNewline);
		szBuffer.append(gDLL->getText("TXT_KEY_PROMOTION_COLLATERAL_DAMAGE_TEXT", GC.getPromotionInfo(ePromotion).getCollateralDamageChange()));
	}

	if (GC.getPromotionInfo(ePromotion).getBombardRateChange() != 0)
	{
		szBuffer.append(pcNewline);
		szBuffer.append(gDLL->getText("TXT_KEY_PROMOTION_BOMBARD_TEXT", GC.getPromotionInfo(ePromotion).getBombardRateChange()));
	}

	if (GC.getPromotionInfo(ePromotion).getFirstStrikesChange() != 0)
	{
		if (GC.getPromotionInfo(ePromotion).getFirstStrikesChange() == 1)
		{
			szBuffer.append(pcNewline);
			szBuffer.append(gDLL->getText("TXT_KEY_PROMOTION_FIRST_STRIKE_TEXT", GC.getPromotionInfo(ePromotion).getFirstStrikesChange()));
		}
		else
		{
			szBuffer.append(pcNewline);
			szBuffer.append(gDLL->getText("TXT_KEY_PROMOTION_FIRST_STRIKES_TEXT", GC.getPromotionInfo(ePromotion).getFirstStrikesChange()));
		}
	}

	if (GC.getPromotionInfo(ePromotion).getChanceFirstStrikesChange() != 0)
	{
		if (GC.getPromotionInfo(ePromotion).getChanceFirstStrikesChange() == 1)
		{
			szBuffer.append(pcNewline);
			szBuffer.append(gDLL->getText("TXT_KEY_PROMOTION_FIRST_STRIKE_CHANCE_TEXT", GC.getPromotionInfo(ePromotion).getChanceFirstStrikesChange()));
		}
		else
		{
			szBuffer.append(pcNewline);
			szBuffer.append(gDLL->getText("TXT_KEY_PROMOTION_FIRST_STRIKES_CHANCE_TEXT", GC.getPromotionInfo(ePromotion).getChanceFirstStrikesChange()));
		}
	}

	if (GC.getPromotionInfo(ePromotion).getEnemyHealChange() != 0)
	{
		szBuffer.append(pcNewline);
		szBuffer.append(gDLL->getText("TXT_KEY_PROMOTION_HEALS_EXTRA_TEXT", GC.getPromotionInfo(ePromotion).getEnemyHealChange()) + gDLL->getText("TXT_KEY_PROMOTION_ENEMY_LANDS_TEXT"));
	}

	if (GC.getPromotionInfo(ePromotion).getNeutralHealChange() != 0)
	{
		szBuffer.append(pcNewline);
		szBuffer.append(gDLL->getText("TXT_KEY_PROMOTION_HEALS_EXTRA_TEXT", GC.getPromotionInfo(ePromotion).getNeutralHealChange()) + gDLL->getText("TXT_KEY_PROMOTION_NEUTRAL_LANDS_TEXT"));
	}

	if (GC.getPromotionInfo(ePromotion).getFriendlyHealChange() != 0)
	{
		szBuffer.append(pcNewline);
		szBuffer.append(gDLL->getText("TXT_KEY_PROMOTION_HEALS_EXTRA_TEXT", GC.getPromotionInfo(ePromotion).getFriendlyHealChange()) + gDLL->getText("TXT_KEY_PROMOTION_FRIENDLY_LANDS_TEXT"));
	}

	if (GC.getPromotionInfo(ePromotion).getSameTileHealChange() != 0)
	{
		szBuffer.append(pcNewline);
		szBuffer.append(gDLL->getText("TXT_KEY_PROMOTION_HEALS_SAME_TEXT", GC.getPromotionInfo(ePromotion).getSameTileHealChange()) + gDLL->getText("TXT_KEY_PROMOTION_DAMAGE_TURN_TEXT"));
	}

	if (GC.getPromotionInfo(ePromotion).getAdjacentTileHealChange() != 0)
	{
		szBuffer.append(pcNewline);
		szBuffer.append(gDLL->getText("TXT_KEY_PROMOTION_HEALS_ADJACENT_TEXT", GC.getPromotionInfo(ePromotion).getAdjacentTileHealChange()) + gDLL->getText("TXT_KEY_PROMOTION_DAMAGE_TURN_TEXT"));
	}

	if (GC.getPromotionInfo(ePromotion).getCombatPercent() != 0)
	{
		szBuffer.append(pcNewline);
		szBuffer.append(gDLL->getText("TXT_KEY_PROMOTION_STRENGTH_TEXT", GC.getPromotionInfo(ePromotion).getCombatPercent()));
	}

	if (GC.getPromotionInfo(ePromotion).getCityAttackPercent() != 0)
	{
		szBuffer.append(pcNewline);
		szBuffer.append(gDLL->getText("TXT_KEY_PROMOTION_CITY_ATTACK_TEXT", GC.getPromotionInfo(ePromotion).getCityAttackPercent()));
	}

	if (GC.getPromotionInfo(ePromotion).getCityDefensePercent() != 0)
	{
		szBuffer.append(pcNewline);
		szBuffer.append(gDLL->getText("TXT_KEY_PROMOTION_CITY_DEFENSE_TEXT", GC.getPromotionInfo(ePromotion).getCityDefensePercent()));
	}

	if (GC.getPromotionInfo(ePromotion).getHillsAttackPercent() != 0)
	{
		szBuffer.append(pcNewline);
		szBuffer.append(gDLL->getText("TXT_KEY_UNIT_HILLS_ATTACK", GC.getPromotionInfo(ePromotion).getHillsAttackPercent()));
	}

	if (GC.getPromotionInfo(ePromotion).getHillsDefensePercent() != 0)
	{
		szBuffer.append(pcNewline);
		szBuffer.append(gDLL->getText("TXT_KEY_PROMOTION_HILLS_DEFENSE_TEXT", GC.getPromotionInfo(ePromotion).getHillsDefensePercent()));
	}

	if (GC.getPromotionInfo(ePromotion).getRevoltProtection() != 0)
	{
		szBuffer.append(pcNewline);
		szBuffer.append(gDLL->getText("TXT_KEY_PROMOTION_REVOLT_PROTECTION_TEXT", GC.getPromotionInfo(ePromotion).getRevoltProtection()));
	}

	if (GC.getPromotionInfo(ePromotion).getCollateralDamageProtection() != 0)
	{
		szBuffer.append(pcNewline);
		szBuffer.append(gDLL->getText("TXT_KEY_PROMOTION_COLLATERAL_PROTECTION_TEXT", GC.getPromotionInfo(ePromotion).getCollateralDamageProtection()));
	}

	if (GC.getPromotionInfo(ePromotion).getPillageChange() != 0)
	{
		szBuffer.append(pcNewline);
		szBuffer.append(gDLL->getText("TXT_KEY_PROMOTION_PILLAGE_CHANGE_TEXT", GC.getPromotionInfo(ePromotion).getPillageChange()));
	}

	if (GC.getPromotionInfo(ePromotion).getUpgradeDiscount() != 0)
	{
		if (100 == GC.getPromotionInfo(ePromotion).getUpgradeDiscount())
		{
			szBuffer.append(pcNewline);
			szBuffer.append(gDLL->getText("TXT_KEY_PROMOTION_UPGRADE_DISCOUNT_FREE_TEXT"));
		}
		else
		{
			szBuffer.append(pcNewline);
			szBuffer.append(gDLL->getText("TXT_KEY_PROMOTION_UPGRADE_DISCOUNT_TEXT", GC.getPromotionInfo(ePromotion).getUpgradeDiscount()));
		}
	}

	if (GC.getPromotionInfo(ePromotion).getExperiencePercent() != 0)
	{
		szBuffer.append(pcNewline);
		szBuffer.append(gDLL->getText("TXT_KEY_PROMOTION_FASTER_EXPERIENCE_TEXT", GC.getPromotionInfo(ePromotion).getExperiencePercent()));
	}

	if (GC.getPromotionInfo(ePromotion).getKamikazePercent() != 0)
	{
		szBuffer.append(pcNewline);
		szBuffer.append(gDLL->getText("TXT_KEY_PROMOTION_KAMIKAZE_TEXT", GC.getPromotionInfo(ePromotion).getKamikazePercent()));
	}

	for (iI = 0; iI < GC.getNumTerrainInfos(); ++iI)
	{
		if (GC.getPromotionInfo(ePromotion).getTerrainAttackPercent(iI) != 0)
		{
			szBuffer.append(pcNewline);
			szBuffer.append(gDLL->getText("TXT_KEY_PROMOTION_ATTACK_TEXT", GC.getPromotionInfo(ePromotion).getTerrainAttackPercent(iI), GC.getTerrainInfo((TerrainTypes) iI).getTextKeyWide()));
		}

		if (GC.getPromotionInfo(ePromotion).getTerrainDefensePercent(iI) != 0)
		{
			szBuffer.append(pcNewline);
			szBuffer.append(gDLL->getText("TXT_KEY_PROMOTION_DEFENSE_TEXT", GC.getPromotionInfo(ePromotion).getTerrainDefensePercent(iI), GC.getTerrainInfo((TerrainTypes) iI).getTextKeyWide()));
		}
	}

	for (iI = 0; iI < GC.getNumFeatureInfos(); ++iI)
	{
		if (GC.getPromotionInfo(ePromotion).getFeatureAttackPercent(iI) != 0)
		{
			szBuffer.append(pcNewline);
			szBuffer.append(gDLL->getText("TXT_KEY_PROMOTION_ATTACK_TEXT", GC.getPromotionInfo(ePromotion).getFeatureAttackPercent(iI), GC.getFeatureInfo((FeatureTypes) iI).getTextKeyWide()));
		}

		if (GC.getPromotionInfo(ePromotion).getFeatureDefensePercent(iI) != 0)
		{
			szBuffer.append(pcNewline);
			szBuffer.append(gDLL->getText("TXT_KEY_PROMOTION_DEFENSE_TEXT", GC.getPromotionInfo(ePromotion).getFeatureDefensePercent(iI), GC.getFeatureInfo((FeatureTypes) iI).getTextKeyWide()));
		}
	}

	for (iI = 0; iI < GC.getNumUnitCombatInfos(); ++iI)
	{
		if (GC.getPromotionInfo(ePromotion).getUnitCombatModifierPercent(iI) != 0)
		{
			szBuffer.append(pcNewline);
			szBuffer.append(gDLL->getText("TXT_KEY_PROMOTION_VERSUS_TEXT", GC.getPromotionInfo(ePromotion).getUnitCombatModifierPercent(iI), GC.getUnitCombatInfo((UnitCombatTypes)iI).getTextKeyWide()));
		}
	}

	for (iI = 0; iI < NUM_DOMAIN_TYPES; ++iI)
	{
		if (GC.getPromotionInfo(ePromotion).getDomainModifierPercent(iI) != 0)
		{
			szBuffer.append(pcNewline);
			szBuffer.append(gDLL->getText("TXT_KEY_PROMOTION_VERSUS_TEXT", GC.getPromotionInfo(ePromotion).getDomainModifierPercent(iI), GC.getDomainInfo((DomainTypes)iI).getTextKeyWide()));
		}
	}

	if (wcslen(GC.getPromotionInfo(ePromotion).getHelp()) > 0)
	{
		szBuffer.append(pcNewline);
		szBuffer.append(GC.getPromotionInfo(ePromotion).getHelp());
	}

	/* <advc.004e> Show promotions that promo leads to.
	Based on CvPediaPromotion.py and CvUnit::canAcquirePromotion. */
	CvUnit* pHeadSelectedUnit = gDLL->getInterfaceIFace()->getHeadSelectedUnit();
	std::vector<PromotionTypes> aeReq;
	std::vector<PromotionTypes> aeAltReq;
	for(int i = 0; i < GC.getNumPromotionInfos(); i++) {
		PromotionTypes eLoopPromo = (PromotionTypes)i;
		CvPromotionInfo const& pi = GC.getPromotionInfo(eLoopPromo);
		/* If a unit is selected, only show pi if promo will help that unit
		   to acquire pi; don't just show all promotions that promo might
		   theoretically lead to. */
		if(pHeadSelectedUnit != NULL && pHeadSelectedUnit->getOwner() != NO_PLAYER) {
			CvUnit const& kUnit = *pHeadSelectedUnit;
			if(!kUnit.isPromotionValid(eLoopPromo))
				continue;
			if(kUnit.isHasPromotion(eLoopPromo) || kUnit.canAcquirePromotion(eLoopPromo))
				continue;
			CvPlayer const& kOwner = GET_PLAYER(kUnit.getOwner());
			if(pi.getTechPrereq() != NO_TECH && !GET_TEAM(kOwner.getTeam()).isHasTech((TechTypes)
					(pi.getTechPrereq())))
				continue;
			if(pi.getStateReligionPrereq() != NO_RELIGION &&
					kOwner.getStateReligion() != pi.getStateReligionPrereq())
				continue;
		}
		if(pi.getPrereqPromotion() == ePromotion)
			aeReq.push_back(eLoopPromo);
		if(pi.getPrereqOrPromotion1() == ePromotion ||
				pi.getPrereqOrPromotion2() == ePromotion ||
				pi.getPrereqOrPromotion3() == ePromotion)
			aeAltReq.push_back(eLoopPromo);
	}
	CvWString szTemp;
	if(!aeReq.empty()) {
		szTemp.append(pcNewline);
		szTemp.append(gDLL->getText("TXT_KEY_REQUIRED_FOR"));
		szTemp.append(L" ");
		for(size_t i = 0; i < aeReq.size(); i++) {
			szTemp.append(GC.getPromotionInfo(aeReq[i]).getDescription());
			szTemp.append(L", ");
		}
		// Drop the final comma
		szTemp = szTemp.substr(0, szTemp.length() - 2);
	}
	if(!aeAltReq.empty()) {
		szTemp.append(pcNewline);
		szTemp.append(gDLL->getText("TXT_KEY_LEADS_TO"));
		szTemp.append(L" ");
		for(size_t i = 0; i < aeAltReq.size(); i++) {
			szTemp.append(GC.getPromotionInfo(aeAltReq[i]).getDescription());
			szTemp.append(L", ");
		}
		szTemp = szTemp.substr(0, szTemp.length() - 2);
	}
	szBuffer.append(szTemp);
	// </advc.004e>
}

//	Function:			parseCivicInfo()
//	Description:	Will parse the civic info help
//	Parameters:		szHelpText -- the text to put it into
//								civicInfo - what to parse
//	Returns:			nothing
void CvGameTextMgr::parseCivicInfo(CvWStringBuffer &szHelpText, CivicTypes eCivic, bool bCivilopediaText, bool bPlayerContext, bool bSkipName)
{
	PROFILE_FUNC();

	if(NO_CIVIC == eCivic)
		return;

	CvWString szFirstBuffer;
	int iI, iJ;

	szHelpText.clear();

	FAssert(GC.getGame().getActivePlayer() != NO_PLAYER || !bPlayerContext);

	CvCivicInfo const& ci = GC.getCivicInfo(eCivic); // advc.003

	if (!bSkipName)
	{
		szHelpText.append(ci.getDescription());
	}

	if (!bCivilopediaText)
	{
		if (!bPlayerContext || !(GET_PLAYER(GC.getGame().getActivePlayer()).canDoCivics(eCivic)))
		{	// <advc.912d>
			bool bValid = true;
			if(bPlayerContext && GC.getGame().isOption(GAMEOPTION_NO_SLAVERY) &&
					GET_PLAYER(GC.getGame().getActivePlayer()).isHuman()) {
				for(int i = 0; i < GC.getNumHurryInfos(); i++) {
					if(ci.isHurry(i) &&
							GC.getHurryInfo((HurryTypes)i).
							getProductionPerPopulation() > 0) {
						bValid = false;
						szHelpText.append(NEWLINE);
						szHelpText.append(gDLL->getText("TXT_KEY_CIVIC_BLOCKED_BY_OPTION",
								ci.getDescription(),
								GC.getGameOptionInfo(GAMEOPTION_NO_SLAVERY).getDescription()));
					}
				}
			} // </advc.912d>
			if (!bPlayerContext ||
					(bValid && // advc.912d: Don't show tech req if disabled by option
					!GET_TEAM(GC.getGame().getActiveTeam()).isHasTech(
					(TechTypes)(ci.getTechPrereq()))))
			{
				if (ci.getTechPrereq() != NO_TECH)
				{
					szHelpText.append(NEWLINE);
					szHelpText.append(gDLL->getText("TXT_KEY_CIVIC_REQUIRES", GC.getTechInfo((TechTypes)ci.getTechPrereq()).getTextKeyWide()));
				}
			}
		}
	}

	// Special Building Not Required...
	for (iI = 0; iI < GC.getNumSpecialBuildingInfos(); ++iI)
	{
		if (ci.isSpecialBuildingNotRequired(iI))
		{
			// XXX "Missionaries"??? - Now in XML
			szHelpText.append(NEWLINE);
			szHelpText.append(gDLL->getText("TXT_KEY_CIVIC_BUILD_MISSIONARIES", GC.getSpecialBuildingInfo((SpecialBuildingTypes)iI).getTextKeyWide()));
		}
	}

	// Valid Specialists...

	bool bFirst = true;

	for (iI = 0; iI < GC.getNumSpecialistInfos(); ++iI)
	{
		if (ci.isSpecialistValid(iI))
		{
			szFirstBuffer.Format(L"%s%s", NEWLINE, gDLL->getText("TXT_KEY_CIVIC_UNLIMTED").c_str());
			CvWString szSpecialist;
			szSpecialist.Format(L"<link=literal>%s</link>", GC.getSpecialistInfo((SpecialistTypes)iI).getDescription());
			setListHelp(szHelpText, szFirstBuffer, szSpecialist, L", ", bFirst);
			bFirst = false;
		}
	}

	//	Great People Modifier...
	if (ci.getGreatPeopleRateModifier() != 0)
	{
		szHelpText.append(NEWLINE);
		szHelpText.append(gDLL->getText("TXT_KEY_CIVIC_GREAT_PEOPLE_MOD", ci.getGreatPeopleRateModifier()));
	}

	//	Great General Modifier...
	if (ci.getGreatGeneralRateModifier() != 0)
	{
		szHelpText.append(NEWLINE);
		szHelpText.append(gDLL->getText("TXT_KEY_CIVIC_GREAT_GENERAL_MOD", ci.getGreatGeneralRateModifier()));
	}

	if (ci.getDomesticGreatGeneralRateModifier() != 0)
	{
		szHelpText.append(NEWLINE);
		szHelpText.append(gDLL->getText("TXT_KEY_DOMESTIC_GREAT_GENERAL_MODIFIER", ci.getDomesticGreatGeneralRateModifier()));
	}

	//	State Religion Great People Modifier...
	if (ci.getStateReligionGreatPeopleRateModifier() != 0)
	{
		if (bPlayerContext && (GET_PLAYER(GC.getGame().getActivePlayer()).getStateReligion() != NO_RELIGION))
		{
			szHelpText.append(NEWLINE);
			szHelpText.append(gDLL->getText("TXT_KEY_CIVIC_GREAT_PEOPLE_MOD_RELIGION", ci.getStateReligionGreatPeopleRateModifier(), GC.getReligionInfo(GET_PLAYER(GC.getGame().getActivePlayer()).getStateReligion()).getChar()));
		}
		else
		{
			szHelpText.append(NEWLINE);
			szHelpText.append(gDLL->getText("TXT_KEY_CIVIC_GREAT_PEOPLE_MOD_STATE_RELIGION", ci.getStateReligionGreatPeopleRateModifier(), gDLL->getSymbolID(RELIGION_CHAR)));
		}
	}

	//	Distance Maintenance Modifer...
	if (ci.getDistanceMaintenanceModifier() != 0)
	{
		if (ci.getDistanceMaintenanceModifier() <= -100)
		{
			szHelpText.append(NEWLINE);
			szHelpText.append(gDLL->getText("TXT_KEY_CIVIC_DISTANCE_MAINT"));
		}
		else
		{
			szHelpText.append(NEWLINE);
			szHelpText.append(gDLL->getText("TXT_KEY_CIVIC_DISTANCE_MAINT_MOD", ci.getDistanceMaintenanceModifier()));
		}
	}

	//	Num Cities Maintenance Modifer...
	if (ci.getNumCitiesMaintenanceModifier() != 0)
	{
		if (ci.getNumCitiesMaintenanceModifier() <= -100)
		{
			szHelpText.append(NEWLINE);
			szHelpText.append(gDLL->getText("TXT_KEY_CIVIC_NO_MAINT_NUM_CITIES"));
		}
		else
		{
			szHelpText.append(NEWLINE);
			szHelpText.append(gDLL->getText("TXT_KEY_CIVIC_NO_MAINT_NUM_CITIES_MOD", ci.getNumCitiesMaintenanceModifier()));
		}
	}

	//	Corporations Maintenance Modifer...
	if (ci.getCorporationMaintenanceModifier() != 0)
	{
		if (ci.getCorporationMaintenanceModifier() <= -100)
		{
			szHelpText.append(NEWLINE);
			szHelpText.append(gDLL->getText("TXT_KEY_CIVIC_NO_MAINT_CORPORATION"));
		}
		else
		{
			szHelpText.append(NEWLINE);
			szHelpText.append(gDLL->getText("TXT_KEY_CIVIC_NO_MAINT_CORPORATION_MOD", ci.getCorporationMaintenanceModifier()));
		}
	}

	//	Extra Health
	if (ci.getExtraHealth() != 0)
	{
		szHelpText.append(NEWLINE);
		szHelpText.append(gDLL->getText("TXT_KEY_CIVIC_EXTRA_HEALTH", abs(ci.getExtraHealth()), ((ci.getExtraHealth() > 0) ? gDLL->getSymbolID(HEALTHY_CHAR): gDLL->getSymbolID(UNHEALTHY_CHAR))));
	}

	//	Extra Happiness (new in K-Mod)
	if (ci.getExtraHappiness() != 0)
	{
		szHelpText.append(NEWLINE);
		szHelpText.append(gDLL->getText("TXT_KEY_CIVIC_EXTRA_HEALTH", abs(ci.getExtraHappiness()), ci.getExtraHappiness() > 0 ? gDLL->getSymbolID(HAPPY_CHAR): gDLL->getSymbolID(UNHAPPY_CHAR)));
		// note: TXT_KEY_CIVIC_EXTRA_HEALTH just says "[blah] in all cities", so it's ok for happiness as well as for health.
	}

	//	Free Experience
	if (ci.getFreeExperience() != 0)
	{
		szHelpText.append(NEWLINE);
		szHelpText.append(gDLL->getText("TXT_KEY_CIVIC_FREE_XP", ci.getFreeExperience()));
	}

	//	Worker speed modifier
	if (ci.getWorkerSpeedModifier() != 0)
	{
		szHelpText.append(NEWLINE);
		szHelpText.append(gDLL->getText("TXT_KEY_CIVIC_WORKER_SPEED", ci.getWorkerSpeedModifier()));
	}

	//	Improvement upgrade rate modifier
	if (ci.getImprovementUpgradeRateModifier() != 0)
	{
		bFirst = true;

		for (iI = 0; iI < GC.getNumImprovementInfos(); ++iI)
		{
			if (GC.getImprovementInfo((ImprovementTypes)iI).getImprovementUpgrade() != NO_IMPROVEMENT)
			{
				szFirstBuffer.Format(L"%s%s", NEWLINE, gDLL->getText("TXT_KEY_CIVIC_IMPROVEMENT_UPGRADE", ci.getImprovementUpgradeRateModifier()).c_str());
				CvWString szImprovement;
				szImprovement.Format(L"<link=literal>%s</link>", GC.getImprovementInfo((ImprovementTypes)iI).getDescription());
				setListHelp(szHelpText, szFirstBuffer, szImprovement, L", ", bFirst);
				bFirst = false;
			}
		}
	}

	//	Military unit production modifier
	if (ci.getMilitaryProductionModifier() != 0)
	{
		szHelpText.append(NEWLINE);
		szHelpText.append(gDLL->getText("TXT_KEY_CIVIC_MILITARY_PRODUCTION", ci.getMilitaryProductionModifier()));
	}

	//	Free units population percent
	if ((ci.getBaseFreeUnits() != 0) || (ci.getFreeUnitsPopulationPercent() != 0))
	{
		if (bPlayerContext)
		{
			szHelpText.append(NEWLINE);
			szHelpText.append(gDLL->getText("TXT_KEY_CIVIC_FREE_UNITS", (ci.getBaseFreeUnits() + ((GET_PLAYER(GC.getGame().getActivePlayer()).getTotalPopulation() * ci.getFreeUnitsPopulationPercent()) / 100))));
		}
		else
		{
			szHelpText.append(NEWLINE);
			szHelpText.append(gDLL->getText("TXT_KEY_CIVIC_UNIT_SUPPORT"));
		}
	}

	//	Free military units population percent
	if ((ci.getBaseFreeMilitaryUnits() != 0) || (ci.getFreeMilitaryUnitsPopulationPercent() != 0))
	{
		if (bPlayerContext)
		{
			szHelpText.append(NEWLINE);
			szHelpText.append(gDLL->getText("TXT_KEY_CIVIC_FREE_MILITARY_UNITS", (ci.getBaseFreeMilitaryUnits() + ((GET_PLAYER(GC.getGame().getActivePlayer()).getTotalPopulation() * ci.getFreeMilitaryUnitsPopulationPercent()) / 100))));
		}
		else
		{
			szHelpText.append(NEWLINE);
			szHelpText.append(gDLL->getText("TXT_KEY_CIVIC_MILITARY_UNIT_SUPPORT"));
		}
	}

	int const iHappyPerMilitaryUnit = ci.getHappyPerMilitaryUnit(); // advc.003
	if (iHappyPerMilitaryUnit != 0)
	{
		szHelpText.append(NEWLINE);
		/*  <advc.912c> If a player reverts to the BtS ability through XML,
			show the old help text (instead of "+2 happiness per 2 units"). */
		int iAbsHappyPerMilitaryUnit = abs(iHappyPerMilitaryUnit);
		if(iAbsHappyPerMilitaryUnit == 2) {
			szHelpText.append(gDLL->getText("TXT_KEY_CIVIC_UNIT_HAPPINESS",
					iAbsHappyPerMilitaryUnit / 2, ((iHappyPerMilitaryUnit > 0) ?
					gDLL->getSymbolID(HAPPY_CHAR) :
					gDLL->getSymbolID(UNHAPPY_CHAR))));
		}
		else {
			szHelpText.append(gDLL->getText("TXT_KEY_CIVIC_UNIT_HAPPINESS2",
			// </advc.912c>
				/* original bts code
				ci.getHappyPerMilitaryUnit(), ((ci.getHappyPerMilitaryUnit() > 0) ? gDLL->getSymbolID(HAPPY_CHAR) : gDLL->getSymbolID(UNHAPPY_CHAR))));*/
				// UNOFFICIAL_PATCH, Bugfix, 08/28/09, jdog5000: Use absolute value with unhappy face
				iAbsHappyPerMilitaryUnit, ((iHappyPerMilitaryUnit > 0) ? gDLL->getSymbolID(HAPPY_CHAR) : gDLL->getSymbolID(UNHAPPY_CHAR))));
		// <advc.912c>
		}
	}
	if(ci.getLuxuryModifier() != 0) {
		szHelpText.append(NEWLINE);
		szHelpText.append(gDLL->getText("TXT_KEY_CIVIC_LUXURY_MODIFIER",
				ci.getLuxuryModifier(), gDLL->getSymbolID(HAPPY_CHAR)));
	} // </advc.912c>

	//	Military units produced with food
	if (ci.isMilitaryFoodProduction())
	{
		szHelpText.append(NEWLINE);
		szHelpText.append(gDLL->getText("TXT_KEY_CIVIC_MILITARY_FOOD"));
	}

	//	Conscription
	if (getWorldSizeMaxConscript(eCivic) != 0)
	{
		szHelpText.append(NEWLINE);
		// <advc.004y>
		/*  Caller doesn't seem to set bPlayerContext and bCivilopediaText
			properly; will have to figure it out on our own: */
		bool bRange = (GC.getGame().getActivePlayer() == NO_PLAYER);
		if(bRange) {
			int iBase = ci.getMaxConscript();
			int iLow = (iBase * std::max(0, (GC.getWorldInfo((WorldSizeTypes)0).
					getMaxConscriptModifier() + 100))) / 100;
			int iHigh = (iBase * std::max(0, (GC.getWorldInfo((WorldSizeTypes)
					(GC.getNumWorldInfos() - 1)).getMaxConscriptModifier() + 100))) / 100;
			if(iHigh == iLow)
				bRange = false;
			else {
				szHelpText.append(gDLL->getText("TXT_KEY_CIVIC_CONSCRIPTION_RANGE",
						iLow, iHigh));
			}
		}
		if(!bRange) // </advc.004y>
			szHelpText.append(gDLL->getText("TXT_KEY_CIVIC_CONSCRIPTION", getWorldSizeMaxConscript(eCivic)));
	}

	//	Population Unhealthiness
/*
** K-Mod, 27/dec/10, karadoc
** replaced NoUnhealthyPopulation with UnhealthyPopulationModifier
*/
	/* original bts code
	if (ci.isNoUnhealthyPopulation())
	{
		szHelpText.append(NEWLINE);
		szHelpText.append(gDLL->getText("TXT_KEY_CIVIC_NO_POP_UNHEALTHY"));
	}
	*/
	if (ci.getUnhealthyPopulationModifier() != 0)
	{
		// If the modifier is less than -100, display the old NoUnhealth. text
		// Note: this could be techinically inaccurate if we combine this modifier with a positive modifier
		if (ci.getUnhealthyPopulationModifier() <= -100)
		{
			szHelpText.append(NEWLINE);
			szHelpText.append(gDLL->getText("TXT_KEY_CIVIC_NO_POP_UNHEALTHY"));
		}
		else
		{
			szHelpText.append(NEWLINE);
			szHelpText.append(gDLL->getText("TXT_KEY_UNHEALTHY_POP_MODIFIER", ci.getUnhealthyPopulationModifier()));
		}
	}
/*
** K-Mod end
*/

	//	Building Unhealthiness
	if (ci.isBuildingOnlyHealthy())
	{
		szHelpText.append(NEWLINE);
		szHelpText.append(gDLL->getText("TXT_KEY_CIVIC_NO_BUILDING_UNHEALTHY"));
	}

	//	Population Unhealthiness
	if (ci.getExpInBorderModifier() != 0)
	{
		szHelpText.append(NEWLINE);
		szHelpText.append(gDLL->getText("TXT_KEY_CIVIC_EXPERIENCE_IN_BORDERS", ci.getExpInBorderModifier()));
	}

	//	War Weariness
	if (ci.getWarWearinessModifier() != 0)
	{
		if (ci.getWarWearinessModifier() <= -100)
		{
			szHelpText.append(NEWLINE);
			szHelpText.append(gDLL->getText("TXT_KEY_CIVIC_NO_WAR_WEARINESS"));
		}
		else
		{
			szHelpText.append(NEWLINE);
			szHelpText.append(gDLL->getText("TXT_KEY_CIVIC_EXTRA_WAR_WEARINESS", ci.getWarWearinessModifier()));
		}
	}

	//	Free specialists
	if (ci.getFreeSpecialist() != 0)
	{
		szHelpText.append(NEWLINE);
		szHelpText.append(gDLL->getText("TXT_KEY_CIVIC_FREE_SPECIALISTS", ci.getFreeSpecialist()));
	}

	//	Trade routes
	if (ci.getTradeRoutes() != 0)
	{
		szHelpText.append(NEWLINE);
		szHelpText.append(gDLL->getText("TXT_KEY_CIVIC_TRADE_ROUTES", ci.getTradeRoutes()));
	}

	//	No Foreign Trade
	if (ci.isNoForeignTrade())
	{
		szHelpText.append(NEWLINE);
		szHelpText.append(gDLL->getText("TXT_KEY_CIVIC_NO_FOREIGN_TRADE"));
	}

	//	No Corporations
	if (ci.isNoCorporations())
	{
		szHelpText.append(NEWLINE);
		szHelpText.append(gDLL->getText("TXT_KEY_CIVIC_NO_CORPORATIONS"));
	}

	//	No Foreign Corporations
	if (ci.isNoForeignCorporations())
	{
		szHelpText.append(NEWLINE);
		szHelpText.append(gDLL->getText("TXT_KEY_CIVIC_NO_FOREIGN_CORPORATIONS"));
	}

	//	Freedom Anger
	if (ci.getCivicPercentAnger() != 0)
	{
		szHelpText.append(NEWLINE);
		szHelpText.append(gDLL->getText("TXT_KEY_CIVIC_FREEDOM_ANGER", ci.getTextKeyWide()));
	}

	if (!(ci.isStateReligion()))
	{
		bool bFound = false;

		for (iI = 0; iI < GC.getNumCivicInfos(); ++iI)
		{
			if ((GC.getCivicInfo((CivicTypes) iI).getCivicOptionType() == ci.getCivicOptionType()) && (GC.getCivicInfo((CivicTypes) iI).isStateReligion()))
			{
				bFound = true;
			}
		}

		if (bFound)
		{
			szHelpText.append(NEWLINE);
			szHelpText.append(gDLL->getText("TXT_KEY_CIVIC_NO_STATE_RELIGION"));
		}
	}

	if (ci.getStateReligionHappiness() != 0)
	{
		if (bPlayerContext && (GET_PLAYER(GC.getGame().getActivePlayer()).getStateReligion() != NO_RELIGION))
		{
			szHelpText.append(NEWLINE);
			szHelpText.append(gDLL->getText("TXT_KEY_CIVIC_STATE_RELIGION_HAPPINESS", abs(ci.getStateReligionHappiness()), ((ci.getStateReligionHappiness() > 0) ? gDLL->getSymbolID(HAPPY_CHAR) : gDLL->getSymbolID(UNHAPPY_CHAR)), GC.getReligionInfo(GET_PLAYER(GC.getGame().getActivePlayer()).getStateReligion()).getChar()));
		}
		else
		{
			szHelpText.append(NEWLINE);
			szHelpText.append(gDLL->getText("TXT_KEY_CIVIC_RELIGION_HAPPINESS", abs(ci.getStateReligionHappiness()), ((ci.getStateReligionHappiness() > 0) ? gDLL->getSymbolID(HAPPY_CHAR) : gDLL->getSymbolID(UNHAPPY_CHAR))));
		}
	}

	if (ci.getNonStateReligionHappiness() != 0)
	{
		if (!ci.isStateReligion())
		{
			szHelpText.append(NEWLINE);
			szHelpText.append(gDLL->getText("TXT_KEY_CIVIC_NON_STATE_REL_HAPPINESS_NO_STATE",
					// UNOFFICIAL_PATCH, Bugfix, 08/28/09, EmperorFool & jdog5000:
					// (params were missing)
					abs(ci.getNonStateReligionHappiness()), ((ci.getNonStateReligionHappiness() > 0) ? gDLL->getSymbolID(HAPPY_CHAR) : gDLL->getSymbolID(UNHAPPY_CHAR))));
		}
		else
		{
			szHelpText.append(NEWLINE);
			szHelpText.append(gDLL->getText("TXT_KEY_CIVIC_NON_STATE_REL_HAPPINESS_WITH_STATE",
					abs(ci.getNonStateReligionHappiness()),
					((ci.getNonStateReligionHappiness() > 0) ?
					gDLL->getSymbolID(HAPPY_CHAR) :
					gDLL->getSymbolID(UNHAPPY_CHAR))));
		}
	}

	//	State Religion Unit Production Modifier
	if (ci.getStateReligionUnitProductionModifier() != 0)
	{
		if (bPlayerContext && (GET_PLAYER(GC.getGame().getActivePlayer()).getStateReligion() != NO_RELIGION))
		{
			szHelpText.append(NEWLINE);
			szHelpText.append(gDLL->getText("TXT_KEY_CIVIC_REL_TRAIN_BONUS", GC.getReligionInfo(GET_PLAYER(GC.getGame().getActivePlayer()).getStateReligion()).getChar(), ci.getStateReligionUnitProductionModifier()));
		}
		else
		{
			szHelpText.append(NEWLINE);
			szHelpText.append(gDLL->getText("TXT_KEY_CIVIC_STATE_REL_TRAIN_BONUS", ci.getStateReligionUnitProductionModifier()));
		}
	}

	//	State Religion Building Production Modifier
	if (ci.getStateReligionBuildingProductionModifier() != 0)
	{
		if (bPlayerContext && (GET_PLAYER(GC.getGame().getActivePlayer()).getStateReligion() != NO_RELIGION))
		{
			szHelpText.append(NEWLINE);
			szHelpText.append(gDLL->getText("TXT_KEY_CIVIC_REL_BUILDING_BONUS", GC.getReligionInfo(GET_PLAYER(GC.getGame().getActivePlayer()).getStateReligion()).getChar(), ci.getStateReligionBuildingProductionModifier()));
		}
		else
		{
			szHelpText.append(NEWLINE);
			szHelpText.append(gDLL->getText("TXT_KEY_CIVIC_STATE_REL_BUILDING_BONUS", ci.getStateReligionBuildingProductionModifier()));
		}
	}

	//	State Religion Free Experience
	if (ci.getStateReligionFreeExperience() != 0)
	{
		if (bPlayerContext && (GET_PLAYER(GC.getGame().getActivePlayer()).getStateReligion() != NO_RELIGION))
		{
			szHelpText.append(NEWLINE);
			szHelpText.append(gDLL->getText("TXT_KEY_CIVIC_REL_FREE_XP", ci.getStateReligionFreeExperience(), GC.getReligionInfo(GET_PLAYER(GC.getGame().getActivePlayer()).getStateReligion()).getChar()));
		}
		else
		{
			szHelpText.append(NEWLINE);
			szHelpText.append(gDLL->getText("TXT_KEY_CIVIC_STATE_REL_FREE_XP", ci.getStateReligionFreeExperience()));
		}
	}

	if (ci.isNoNonStateReligionSpread())
	{
		szHelpText.append(NEWLINE);
		szHelpText.append(gDLL->getText("TXT_KEY_CIVIC_NO_NON_STATE_SPREAD"));
	}

	//	Yield Modifiers
	setYieldChangeHelp(szHelpText, L"", L"", gDLL->getText("TXT_KEY_CIVIC_IN_ALL_CITIES").GetCString(), ci.getYieldModifierArray(), true);

	//	Capital Yield Modifiers
	setYieldChangeHelp(szHelpText, L"", L"", gDLL->getText("TXT_KEY_CIVIC_IN_CAPITAL").GetCString(), ci.getCapitalYieldModifierArray(), true);

	//	Trade Yield Modifiers
	setYieldChangeHelp(szHelpText, L"", L"", gDLL->getText("TXT_KEY_CIVIC_FROM_TRADE_ROUTES").GetCString(), ci.getTradeYieldModifierArray(), true);

	//	Commerce Modifier
	setCommerceChangeHelp(szHelpText, L"", L"", gDLL->getText("TXT_KEY_CIVIC_IN_ALL_CITIES").GetCString(), ci.getCommerceModifierArray(), true);

	//	Capital Commerce Modifiers
	setCommerceChangeHelp(szHelpText, L"", L"", gDLL->getText("TXT_KEY_CIVIC_IN_CAPITAL").GetCString(), ci.getCapitalCommerceModifierArray(), true);

	//	Specialist Commerce
	setCommerceChangeHelp(szHelpText, L"", L"", gDLL->getText("TXT_KEY_CIVIC_PER_SPECIALIST").GetCString(), ci.getSpecialistExtraCommerceArray());

	//	Largest City Happiness
	if (ci.getLargestCityHappiness() != 0)
	{
		szHelpText.append(NEWLINE);
		szHelpText.append(gDLL->getText("TXT_KEY_CIVIC_LARGEST_CITIES_HAPPINESS",
				// UNOFFICIAL_PATCH, Bugfix, 08/28/09, jdog5000: Use absolute value with unhappy face
				abs(ci.getLargestCityHappiness()), ((ci.getLargestCityHappiness() > 0) ? gDLL->getSymbolID(HAPPY_CHAR) : gDLL->getSymbolID(UNHAPPY_CHAR)),
				GC.getWorldInfo(GC.getMap().getWorldSize()).getTargetNumCities()));
	}

	//	Improvement Yields
	for (iI = 0; iI < NUM_YIELD_TYPES; ++iI)
	{
		int iLast = 0;

		for (iJ = 0; iJ < GC.getNumImprovementInfos(); iJ++)
		{
			if (ci.getImprovementYieldChanges(iJ, iI) != 0)
			{
				szFirstBuffer.Format(L"%s%s", NEWLINE, gDLL->getText("TXT_KEY_CIVIC_IMPROVEMENT_YIELD_CHANGE", ci.getImprovementYieldChanges(iJ, iI), GC.getYieldInfo((YieldTypes)iI).getChar()).c_str());
				CvWString szImprovement;
				szImprovement.Format(L"<link=literal>%s</link>", GC.getImprovementInfo((ImprovementTypes)iJ).getDescription());
				setListHelp(szHelpText, szFirstBuffer, szImprovement, L", ", (ci.getImprovementYieldChanges(iJ, iI) != iLast));
				iLast = ci.getImprovementYieldChanges(iJ, iI);
			}
		}
	}

	//	Building Happiness
	for (iI = 0; iI < GC.getNumBuildingClassInfos(); ++iI)
	{
		if (ci.getBuildingHappinessChanges(iI) != 0)
		{
			if (bPlayerContext && NO_PLAYER != GC.getGame().getActivePlayer())
			{
				BuildingTypes eBuilding = (BuildingTypes)GC.getCivilizationInfo(GC.getGame().getActiveCivilizationType()).getCivilizationBuildings(iI);
				if (NO_BUILDING != eBuilding)
				{
					szHelpText.append(NEWLINE);
					szHelpText.append(gDLL->getText("TXT_KEY_CIVIC_BUILDING_HAPPINESS",
							// UNOFFICIAL_PATCH, Bugfix, 08/28/09, jdog5000: Use absolute value with unhappy face
							abs(ci.getBuildingHappinessChanges(iI)), ((ci.getBuildingHappinessChanges(iI) > 0) ? gDLL->getSymbolID(HAPPY_CHAR) : gDLL->getSymbolID(UNHAPPY_CHAR)),
							GC.getBuildingInfo(eBuilding).getTextKeyWide()));
				}
			}
			else
			{
				szHelpText.append(NEWLINE);
				szHelpText.append(gDLL->getText("TXT_KEY_CIVIC_BUILDING_HAPPINESS",
						// UNOFFICIAL_PATCH, Bugfix, 08/28/09, jdog5000: Use absolute value with unhappy face
						abs(ci.getBuildingHappinessChanges(iI)), ((ci.getBuildingHappinessChanges(iI) > 0) ? gDLL->getSymbolID(HAPPY_CHAR) : gDLL->getSymbolID(UNHAPPY_CHAR)),
						GC.getBuildingClassInfo((BuildingClassTypes)iI).getTextKeyWide()));
			}
		}

		if (ci.getBuildingHealthChanges(iI) != 0)
		{
			if (bPlayerContext && NO_PLAYER != GC.getGame().getActivePlayer())
			{
				BuildingTypes eBuilding = (BuildingTypes)GC.getCivilizationInfo(GC.getGame().getActiveCivilizationType()).getCivilizationBuildings(iI);
				if (NO_BUILDING != eBuilding)
				{
					szHelpText.append(NEWLINE);
					szHelpText.append(gDLL->getText("TXT_KEY_CIVIC_BUILDING_HAPPINESS",
							// UNOFFICIAL_PATCH, Bugfix, 08/28/09, jdog5000: Use absolute value with unhealthy symbol
							abs(ci.getBuildingHealthChanges(iI)), ((ci.getBuildingHealthChanges(iI) > 0) ? gDLL->getSymbolID(HEALTHY_CHAR) : gDLL->getSymbolID(UNHEALTHY_CHAR)),
							GC.getBuildingInfo(eBuilding).getTextKeyWide()));
				}
			}
			else
			{
				szHelpText.append(NEWLINE);
				szHelpText.append(gDLL->getText("TXT_KEY_CIVIC_BUILDING_HAPPINESS",
						// UNOFFICIAL_PATCH, Bugfix, 08/28/09, jdog5000: Use absolute value with unhealthy symbol
						abs(ci.getBuildingHealthChanges(iI)), ((ci.getBuildingHealthChanges(iI) > 0) ? gDLL->getSymbolID(HEALTHY_CHAR) : gDLL->getSymbolID(UNHEALTHY_CHAR)),
						GC.getBuildingClassInfo((BuildingClassTypes)iI).getTextKeyWide()));
			}
		}
	}

	//	Feature Happiness
	int iLast = 0;

	for (iI = 0; iI < GC.getNumFeatureInfos(); ++iI)
	{
		if (ci.getFeatureHappinessChanges(iI) != 0)
		{
			szFirstBuffer.Format(L"%s%s", NEWLINE, gDLL->getText("TXT_KEY_CIVIC_FEATURE_HAPPINESS",
					// UNOFFICIAL_PATCH, Bugfix, 08/28/09, jdog5000: Use absolute value with unhappy face
					abs(ci.getFeatureHappinessChanges(iI)), ((ci.getFeatureHappinessChanges(iI) > 0) ? gDLL->getSymbolID(HAPPY_CHAR) : gDLL->getSymbolID(UNHAPPY_CHAR))).c_str());
			CvWString szFeature;
			szFeature.Format(L"<link=literal>%s</link>", GC.getFeatureInfo((FeatureTypes)iI).getDescription());
			setListHelp(szHelpText, szFirstBuffer, szFeature, L", ", (ci.getFeatureHappinessChanges(iI) != iLast));
			iLast = ci.getFeatureHappinessChanges(iI);
		}
	}

	//	Hurry types
	for (iI = 0; iI < GC.getNumHurryInfos(); ++iI)
	{
		if (ci.isHurry(iI))
		{
			szHelpText.append(CvWString::format(L"%s%c%s", NEWLINE, gDLL->getSymbolID(BULLET_CHAR), GC.getHurryInfo((HurryTypes)iI).getDescription()));
		}
	}

	float fInflationFactor = bPlayerContext ? (float)(100 + GET_PLAYER(GC.getGame().getActivePlayer()).calculateInflationRate())/100 : 1.0f; // K-Mod
 	//	Gold cost per unit
	//	Gold cost per unit
	if (ci.getGoldPerUnit() != 0)
	{
		/* szHelpText.append(NEWLINE);
		szHelpText.append(gDLL->getText("TXT_KEY_CIVIC_SUPPORT_COSTS", (ci.getGoldPerUnit() > 0), GC.getCommerceInfo(COMMERCE_GOLD).getChar())); */
		// K-Mod
		szHelpText.append(CvWString::format(L"\n%c%+.2f%c %s",
			gDLL->getSymbolID(BULLET_CHAR), (float)
			ci.getGoldPerUnit()*fInflationFactor/100,
			GC.getCommerceInfo(COMMERCE_GOLD).getChar(),
			gDLL->getText("TXT_KEY_CIVIC_SUPPORT_COSTS").GetCString()));
		// K-Mod end
	}
	int iGoldPerMilitaryUnit = ci.getGoldPerMilitaryUnit(); // advc.003
	//	Gold cost per military unit
	if (iGoldPerMilitaryUnit != 0)
	{	// advc.912b:
		bool bFractional = (iGoldPerMilitaryUnit % 100 != 0);
		/* szHelpText.append(NEWLINE);
		szHelpText.append(gDLL->getText("TXT_KEY_CIVIC_MILITARY_SUPPORT_COSTS", (ci.getGoldPerMilitaryUnit() > 0), GC.getCommerceInfo(COMMERCE_GOLD).getChar())); */
		// K-Mod
		szHelpText.append(
				/*CvWString::format(L"\n%c%+.2f%c %s",
				gDLL->getSymbolID(BULLET_CHAR), (float)ci.
				getGoldPerMilitaryUnit()*fInflationFactor/100,*/
				// <advc.912b> Don't show inflation now that the total is shown
				bFractional ?
				CvWString::format(L"\n%c%+.2f%c %s",
				gDLL->getSymbolID(BULLET_CHAR), 0.01f * iGoldPerMilitaryUnit,
				// </advc.912b>
				GC.getCommerceInfo(COMMERCE_GOLD).getChar(),
				gDLL->getText("TXT_KEY_CIVIC_MILITARY_SUPPORT_COSTS").GetCString())
		// K-Mod end
				// <advc.912b>
				: CvWString::format(L"\n%c%+d%c %s", gDLL->getSymbolID(BULLET_CHAR),
				::round(0.01 * iGoldPerMilitaryUnit), // </advc.912b>
				GC.getCommerceInfo(COMMERCE_GOLD).getChar(),
				gDLL->getText("TXT_KEY_CIVIC_MILITARY_SUPPORT_COSTS").GetCString()));
		// <advc.912b>
		if(bPlayerContext) {
			int iFreeUnits, iFreeMilitaryUnits, iPaidUnits, iPaidMilitaryUnits,
					iMilitaryCost, iBaseUnitCost, iExtraCost;
			GET_PLAYER(GC.getGame().getActivePlayer()).calculateUnitCost(
					iFreeUnits, iFreeMilitaryUnits, iPaidUnits, iPaidMilitaryUnits,
					iBaseUnitCost, iMilitaryCost, iExtraCost);
			float fCurrentTotal = 0.01f * fInflationFactor * iPaidMilitaryUnits *
					iGoldPerMilitaryUnit;
			szHelpText.append(CvWString::format(L"\n(%+.1f%c %s)",
					fCurrentTotal, GC.getCommerceInfo(COMMERCE_GOLD).getChar(),
					gDLL->getText("TXT_KEY_MISC_CURRENTLY").GetCString()));
		} // </advc.912b>
	}

	if (!CvWString(ci.getHelp()).empty())
	{
		szHelpText.append(CvWString::format(L"%s%s", NEWLINE, ci.getHelp()).c_str());
	}
}


void CvGameTextMgr::setTechHelp(CvWStringBuffer &szBuffer, TechTypes eTech, bool bCivilopediaText, bool bPlayerContext, bool bStrategyText, bool bTreeInfo, TechTypes eFromTech)
{
// BULL - Trade Denial - start
	setTechTradeHelp(szBuffer, eTech, NO_PLAYER, bCivilopediaText, bPlayerContext,
			bStrategyText, bTreeInfo, eFromTech);
}


void CvGameTextMgr::setTechTradeHelp(CvWStringBuffer &szBuffer, TechTypes eTech, PlayerTypes eTradePlayer, bool bCivilopediaText, bool bPlayerContext, bool bStrategyText, bool bTreeInfo, TechTypes eFromTech)
// BULL - Trade Denial - end
{
	PROFILE_FUNC();

	CvWString szTempBuffer;
	CvWString szFirstBuffer;
	int iI;
	CvGame const& g = GC.getGame(); // advc.003

	// show debug info if cheat level > 0 and alt down
	bool bAlt = GC.altKey();
	if (bAlt && //(gDLL->getChtLvl() > 0))
			g.isDebugMode()) // advc.135c
	{
		szBuffer.clear();

		for (iI = 0; iI < MAX_PLAYERS; ++iI)
		{
			CvPlayerAI* playerI = &GET_PLAYER((PlayerTypes)iI);
			CvTeamAI* teamI = &GET_TEAM(playerI->getTeam());
			if (playerI->isAlive())
			{
				szTempBuffer.Format(L"%s: ", playerI->getName());
				szBuffer.append(szTempBuffer);

				TechTypes ePlayerTech = playerI->getCurrentResearch();
				if(ePlayerTech == NO_TECH)
					szTempBuffer.Format(L"-\n");
				else {
					szTempBuffer.Format(L"%s (%d->%dt)(%d/%d)\n",
							GC.getTechInfo(ePlayerTech).getDescription(),
							playerI->calculateResearchRate(ePlayerTech),
							playerI->getResearchTurnsLeft(ePlayerTech, true),
							teamI->getResearchProgress(ePlayerTech),
							teamI->getResearchCost(ePlayerTech));
				}
				szBuffer.append(szTempBuffer);
			}
		}

		return;
	}


	if (NO_TECH == eTech)
	{
		return;
	}

	//	Tech Name
	if (!bCivilopediaText && (!bTreeInfo || (NO_TECH == eFromTech)))
	{
		szTempBuffer.Format( SETCOLR L"%s" ENDCOLR , TEXT_COLOR("COLOR_TECH_TEXT"),
				GC.getTechInfo(eTech).getDescription());
		szBuffer.append(szTempBuffer);
	} // <advc.003>
	PlayerTypes eActivePlayer = g.getActivePlayer();
	TeamTypes eActiveTeam = (eActivePlayer == NO_PLAYER ? NO_TEAM : TEAMID(eActivePlayer));
	// </advc.003>
	FAssert(eActivePlayer != NO_PLAYER || !bPlayerContext);

	if (bTreeInfo && NO_TECH != eFromTech)
	{
		buildTechTreeString(szBuffer, eTech, bPlayerContext, eFromTech);
	}

	//	Obsolete Buildings
	for (iI = 0; iI < GC.getNumBuildingClassInfos(); ++iI)
	{
		if (!bPlayerContext || (GET_PLAYER(eActivePlayer).getBuildingClassCount((BuildingClassTypes)iI) > 0))
		{
			BuildingTypes eLoopBuilding;
			if (eActivePlayer != NO_PLAYER)
			{
				eLoopBuilding = (BuildingTypes)GC.getCivilizationInfo(g.getActiveCivilizationType()).getCivilizationBuildings(iI);
			}
			else
			{
				eLoopBuilding = (BuildingTypes)GC.getBuildingClassInfo((BuildingClassTypes)iI).getDefaultBuildingIndex();
			}

			if (eLoopBuilding != NO_BUILDING)
			{
				//	Obsolete Buildings Check...
				if (GC.getBuildingInfo(eLoopBuilding).getObsoleteTech() == eTech)
				{
					buildObsoleteString(szBuffer, eLoopBuilding, true);
				}
			}
		}
	}

	//	Obsolete Bonuses
	for (iI = 0; iI < GC.getNumBonusInfos(); ++iI)
	{
		if (GC.getBonusInfo((BonusTypes)iI).getTechObsolete() == eTech)
		{
			buildObsoleteBonusString(szBuffer, iI, true);
		}
	}

	for (iI = 0; iI < GC.getNumSpecialBuildingInfos(); ++iI)
	{
		if (GC.getSpecialBuildingInfo((SpecialBuildingTypes) iI).getObsoleteTech() == eTech)
		{
			buildObsoleteSpecialString(szBuffer, iI, true);
		}
	}

	//	Route movement change...
	buildMoveString(szBuffer, eTech, true, bPlayerContext);

	//	Creates a free unit...
	buildFreeUnitString(szBuffer, eTech, true, bPlayerContext);

	//	Increases feature production...
	buildFeatureProductionString(szBuffer, eTech, true, bPlayerContext);

	//	Increases worker build rate...
	buildWorkerRateString(szBuffer, eTech, true, bPlayerContext);

	//	Trade Routed per city change...
	buildTradeRouteString(szBuffer, eTech, true, bPlayerContext);

	//	Health increase...
	buildHealthRateString(szBuffer, eTech, true, bPlayerContext);

	//	Happiness increase...
	buildHappinessRateString(szBuffer, eTech, true, bPlayerContext);

	//	Free Techs...
	buildFreeTechString(szBuffer, eTech, true, bPlayerContext);

	//	Line of Sight Bonus across water...
	buildLOSString(szBuffer, eTech, true, bPlayerContext);

	//	Centers world map...
	buildMapCenterString(szBuffer, eTech, true, bPlayerContext);

	//	Reveals World Map...
	buildMapRevealString(szBuffer, eTech, true);

	//	Enables map trading...
	buildMapTradeString(szBuffer, eTech, true, bPlayerContext);

	//	Enables tech trading...
	buildTechTradeString(szBuffer, eTech, true, bPlayerContext);

	//	Enables gold trading...
	buildGoldTradeString(szBuffer, eTech, true, bPlayerContext);

	//	Enables open borders...
	buildOpenBordersString(szBuffer, eTech, true, bPlayerContext);

	//	Enables defensive pacts...
	buildDefensivePactString(szBuffer, eTech, true, bPlayerContext);

	//	Enables permanent alliances...
	buildPermanentAllianceString(szBuffer, eTech, true, bPlayerContext);

	//	Enables bridge building...
	buildBridgeString(szBuffer, eTech, true, bPlayerContext);

	//	Can spread irrigation...
	buildIrrigationString(szBuffer, eTech, true, bPlayerContext);

	//	Ignore irrigation...
	buildIgnoreIrrigationString(szBuffer, eTech, true, bPlayerContext);

	//	Coastal work...
	buildWaterWorkString(szBuffer, eTech, true, bPlayerContext);

	//	Enables permanent alliances...
	buildVassalStateString(szBuffer, eTech, true, bPlayerContext);

	//	Build farm, irrigation, etc...
	for (iI = 0; iI < GC.getNumBuildInfos(); ++iI)
	{
		buildImprovementString(szBuffer, eTech, iI, true, bPlayerContext);
	}

	//	Extra moves for certain domains...
	for (iI = 0; iI < NUM_DOMAIN_TYPES; ++iI)
	{
		buildDomainExtraMovesString(szBuffer, eTech, iI, true, bPlayerContext);
	}

	//	K-Mod. Extra specialist commerce
	setCommerceChangeHelp(szBuffer, L"", L"", gDLL->getText("TXT_KEY_CIVIC_PER_SPECIALIST").GetCString(), GC.getTechInfo(eTech).getSpecialistExtraCommerceArray());

	//	Adjusting culture, science, etc
	for (iI = 0; iI < NUM_COMMERCE_TYPES; ++iI)
	{
		buildAdjustString(szBuffer, eTech, iI, true, bPlayerContext);
	}

	//	Enabling trade routes on water...?
	for (iI = 0; iI < GC.getNumTerrainInfos(); ++iI)
	{
		buildTerrainTradeString(szBuffer, eTech, iI, true, bPlayerContext);
	}

	buildRiverTradeString(szBuffer, eTech, true, bPlayerContext);

	//	Special Buildings
	for (iI = 0; iI < GC.getNumSpecialBuildingInfos(); ++iI)
	{
		buildSpecialBuildingString(szBuffer, eTech, iI, true, bPlayerContext);
	}

	//	Build farm, mine, etc...
	for (iI = 0; iI < GC.getNumImprovementInfos(); ++iI)
	{
		buildYieldChangeString(szBuffer, eTech, iI, true, bPlayerContext);
	}

	bool bFirst = true;

	for (iI = 0; iI < GC.getNumBonusInfos(); ++iI)
	{
		bFirst = buildBonusRevealString(szBuffer, eTech, iI, bFirst, true, bPlayerContext);
	}

	bFirst = true;

	for (iI = 0; iI < GC.getNumCivicInfos(); ++iI)
	{
		bFirst = buildCivicRevealString(szBuffer, eTech, iI, bFirst, true, bPlayerContext);
	}

	if (!bCivilopediaText)
	{
		bFirst = true;

		for (iI = 0; iI < GC.getNumUnitClassInfos(); ++iI)
		{
			if (!bPlayerContext || !(GET_PLAYER(eActivePlayer).isProductionMaxedUnitClass((UnitClassTypes)iI)))
			{
				UnitTypes eLoopUnit;
				if (eActivePlayer != NO_PLAYER)
				{
					eLoopUnit = (UnitTypes)GC.getCivilizationInfo(g.getActiveCivilizationType()).getCivilizationUnits(iI);
				}
				else
				{
					eLoopUnit = (UnitTypes)GC.getUnitClassInfo((UnitClassTypes)iI).getDefaultUnitIndex();
				}

				if (eLoopUnit != NO_UNIT)
				{
					if (!bPlayerContext || !(GET_PLAYER(eActivePlayer).canTrain(eLoopUnit)))
					{
						if (GC.getUnitInfo(eLoopUnit).getPrereqAndTech() == eTech)
						{
							szFirstBuffer.Format(L"%s%s", NEWLINE, gDLL->getText("TXT_KEY_TECH_CAN_TRAIN").c_str());
							szTempBuffer.Format( SETCOLR L"%s" ENDCOLR , TEXT_COLOR("COLOR_UNIT_TEXT"), GC.getUnitInfo(eLoopUnit).getDescription());
							setListHelp(szBuffer, szFirstBuffer, szTempBuffer, L", ", bFirst);
							bFirst = false;
						}
						else
						{
							for (int iJ = 0; iJ < GC.getNUM_UNIT_AND_TECH_PREREQS(); iJ++)
							{
								if (GC.getUnitInfo(eLoopUnit).getPrereqAndTechs(iJ) == eTech)
								{
									szFirstBuffer.Format(L"%s%s", NEWLINE, gDLL->getText("TXT_KEY_TECH_CAN_TRAIN").c_str());
									szTempBuffer.Format( SETCOLR L"%s" ENDCOLR , TEXT_COLOR("COLOR_UNIT_TEXT"), GC.getUnitInfo(eLoopUnit).getDescription());
									setListHelp(szBuffer, szFirstBuffer, szTempBuffer, L", ", bFirst);
									bFirst = false;
									break;
								}
							}
						}
					}
				}
			}
		}

		bFirst = true;

		for (iI = 0; iI < GC.getNumBuildingClassInfos(); ++iI)
		{
			if (!bPlayerContext || !(GET_PLAYER(eActivePlayer).isProductionMaxedBuildingClass((BuildingClassTypes)iI)))
			{
				BuildingTypes eLoopBuilding;
				if (eActivePlayer != NO_PLAYER)
				{
					eLoopBuilding = (BuildingTypes)GC.getCivilizationInfo(g.getActiveCivilizationType()).getCivilizationBuildings(iI);
				}
				else
				{
					eLoopBuilding = (BuildingTypes)GC.getBuildingClassInfo((BuildingClassTypes)iI).getDefaultBuildingIndex();
				}

				if (eLoopBuilding != NO_BUILDING)
				{
					if (!bPlayerContext || !(GET_PLAYER(eActivePlayer).canConstruct(eLoopBuilding, false, true)))
					{
						if (GC.getBuildingInfo(eLoopBuilding).getPrereqAndTech() == eTech)
						{
							szFirstBuffer.Format(L"%s%s", NEWLINE, gDLL->getText("TXT_KEY_TECH_CAN_CONSTRUCT").c_str());
							szTempBuffer.Format( SETCOLR L"<link=literal>%s</link>" ENDCOLR , TEXT_COLOR("COLOR_BUILDING_TEXT"), GC.getBuildingInfo(eLoopBuilding).getDescription());
							setListHelp(szBuffer, szFirstBuffer, szTempBuffer, L", ", bFirst);
							bFirst = false;
						}
						else
						{
							for (int iJ = 0; iJ < GC.getNUM_BUILDING_AND_TECH_PREREQS(); iJ++)
							{
								if (GC.getBuildingInfo(eLoopBuilding).getPrereqAndTechs(iJ) == eTech)
								{
									szFirstBuffer.Format(L"%s%s", NEWLINE, gDLL->getText("TXT_KEY_TECH_CAN_CONSTRUCT").c_str());
									szTempBuffer.Format( SETCOLR L"<link=literal>%s</link>" ENDCOLR , TEXT_COLOR("COLOR_BUILDING_TEXT"), GC.getBuildingInfo(eLoopBuilding).getDescription());
									setListHelp(szBuffer, szFirstBuffer, szTempBuffer, L", ", bFirst);
									bFirst = false;
									break;
								}
							}
						}
					}
				}
			}
		}

		bFirst = true;

		for (iI = 0; iI < GC.getNumProjectInfos(); ++iI)
		{
			if (!bPlayerContext || !(GET_PLAYER(eActivePlayer).isProductionMaxedProject((ProjectTypes)iI)))
			{
				if (!bPlayerContext || !(GET_PLAYER(eActivePlayer).canCreate(((ProjectTypes)iI), false, true)))
				{
					if (GC.getProjectInfo((ProjectTypes)iI).getTechPrereq() == eTech)
					{
						szFirstBuffer.Format(L"%s%s", NEWLINE, gDLL->getText("TXT_KEY_TECH_CAN_CREATE").c_str());
						szTempBuffer.Format( SETCOLR L"%s" ENDCOLR , TEXT_COLOR("COLOR_PROJECT_TEXT"), GC.getProjectInfo((ProjectTypes)iI).getDescription());
						setListHelp(szBuffer, szFirstBuffer, szTempBuffer, L", ", bFirst);
						bFirst = false;
					}
				}
			}
		}
	}

	bFirst = true;
	for (iI = 0; iI < GC.getNumProcessInfos(); ++iI)
	{
		bFirst = buildProcessInfoString(szBuffer, eTech, iI, bFirst, true, bPlayerContext);
	}

	bFirst = true;
	for (iI = 0; iI < GC.getNumReligionInfos(); ++iI)
	{
		if (!bPlayerContext || !g.isReligionSlotTaken((ReligionTypes)iI))
		{
			bFirst = buildFoundReligionString(szBuffer, eTech, iI, bFirst, true, bPlayerContext);
		}
	}

	bFirst = true;
	for (iI = 0; iI < GC.getNumCorporationInfos(); ++iI)
	{
		if (!bPlayerContext || !(g.isCorporationFounded((CorporationTypes)iI)))
		{
			bFirst = buildFoundCorporationString(szBuffer, eTech, iI, bFirst, true, bPlayerContext);
		}
	}

	bFirst = true;
	for (iI = 0; iI < GC.getNumPromotionInfos(); ++iI)
	{
		bFirst = buildPromotionString(szBuffer, eTech, iI, bFirst, true, bPlayerContext);
	}

	if (bTreeInfo && NO_TECH == eFromTech)
	{
		buildSingleLineTechTreeString(szBuffer, eTech, bPlayerContext);
	}

	if (!CvWString(GC.getTechInfo(eTech).getHelp()).empty())
	{
		szBuffer.append(CvWString::format(L"%s%s", NEWLINE, GC.getTechInfo(eTech).getHelp()).c_str());
	}

	if (!bCivilopediaText)
	{
		if (eActivePlayer == NO_PLAYER)
		{
			szTempBuffer.Format(L"\n%d%c", GC.getTechInfo(eTech).getResearchCost(),
					GC.getCommerceInfo(COMMERCE_RESEARCH).getChar());
			szBuffer.append(szTempBuffer);
		}
		else if (GET_TEAM(eActiveTeam).isHasTech(eTech))
		{
			szTempBuffer.Format(L"\n%d%c", GET_TEAM(eActiveTeam).getResearchCost(eTech),
					GC.getCommerceInfo(COMMERCE_RESEARCH).getChar());
			szBuffer.append(szTempBuffer);
		}
		else
		{
			szBuffer.append(NEWLINE);
			// <advc.004x>
			bool bShowTurns = GET_PLAYER(eActivePlayer).isResearch();
			if(bShowTurns) {
				szBuffer.append(gDLL->getText("TXT_KEY_TECH_NUM_TURNS",
						GET_PLAYER(eActivePlayer).getResearchTurnsLeft(eTech,
						GC.ctrlKey() || !GC.shiftKey())));
				szBuffer.append(L' '); // advc.004x: Append this separately
			} // </advc.004x>
			szTempBuffer.Format(L"%s%d/%d %c%s",
					bShowTurns ? L"(" : L"", // advc.004x
					GET_TEAM(eActiveTeam).getResearchProgress(eTech),
					GET_TEAM(eActiveTeam).getResearchCost(eTech),
					GC.getCommerceInfo(COMMERCE_RESEARCH).getChar(),
					bShowTurns ? L")" : L""); // advc.004x
			szBuffer.append(szTempBuffer);
			// <advc.910>
			if(bShowTurns && bPlayerContext)
				setResearchModifierHelp(szBuffer, eTech); // </advc.910>
		}
	}

	if (eActivePlayer != NO_PLAYER
			/*  advc.004: Don't show this when inspecting foreign research on the
				scoreboard, nor in Civilopedia. */
			&& bPlayerContext)
	{
		if (GET_PLAYER(eActivePlayer).canResearch(eTech))
		{	// advc.004a: Commented out
			/*for (iI = 0; iI < GC.getNumUnitInfos(); ++iI) {
				CvUnitInfo& kUnit = GC.getUnitInfo((UnitTypes)iI);
				if (kUnit.getBaseDiscover() > 0 || kUnit.getDiscoverMultiplier() > 0) {
					if (::getDiscoveryTech((UnitTypes)iI, eActivePlayer) == eTech) {
						szBuffer.append(NEWLINE);
						szBuffer.append(gDLL->getText("TXT_KEY_TECH_GREAT_PERSON_DISCOVER", kUnit.getTextKeyWide()));
					}
				}
			}*/
			if (GET_PLAYER(eActivePlayer).getCurrentEra() < GC.getTechInfo(eTech).getEra())
			{
				szBuffer.append(NEWLINE);
				szBuffer.append(gDLL->getText("TXT_KEY_TECH_ERA_ADVANCE", GC.getEraInfo((EraTypes)GC.getTechInfo(eTech).getEra()).getTextKeyWide()));
			}
		}
	}
// BULL - Trade Denial - start (advc.073: refactored; getBugOptionBOOL check removed)
	if (eTradePlayer != NO_PLAYER && eActivePlayer != NO_PLAYER)
	{
		TradeData item;
		::setTradeItem(&item, TRADE_TECHNOLOGIES, eTech);
		if (GET_PLAYER(eTradePlayer).canTradeItem(eActivePlayer, item, false))
		{
			DenialTypes eDenial = GET_PLAYER(eTradePlayer).getTradeDenial(
					eActivePlayer, item);
			// <advc.073>
			if (eDenial == NO_DENIAL)
			{
				if(!GET_PLAYER(eTradePlayer).isHuman() && !GET_PLAYER(eTradePlayer).
						AI_isWillingToTalk(eActivePlayer, true))
				{
					szBuffer.append(NEWLINE);
					szBuffer.append(gDLL->getText("TXT_KEY_MISC_REFUSES_TO_TALK"));
				}
			} // </advc.073>
			else
			{
				szTempBuffer.Format(SETCOLR L"%s" ENDCOLR,
						TEXT_COLOR("COLOR_NEGATIVE_TEXT"),
						GC.getDenialInfo(eDenial).getDescription());
				szBuffer.append(NEWLINE);
				szBuffer.append(szTempBuffer);
			}
		}
	}
// BULL - Trade Denial - end

	if (bStrategyText)
	{
		if (!CvWString(GC.getTechInfo(eTech).getStrategy()).empty())
		{
			if ((eActivePlayer == NO_PLAYER) || GET_PLAYER(eActivePlayer).isOption(PLAYEROPTION_ADVISOR_HELP))
			{
				szBuffer.append(SEPARATOR);
				szBuffer.append(NEWLINE);
				szBuffer.append(gDLL->getText("TXT_KEY_SIDS_TIPS"));
				szBuffer.append(L'\"');
				szBuffer.append(GC.getTechInfo(eTech).getStrategy());
				szBuffer.append(L'\"');
			}
		}
	}
}

// <advc.910>
void CvGameTextMgr::setResearchModifierHelp(CvWStringBuffer& szBuffer,
		TechTypes eTech) {

	int iFromOtherKnown, iFromPaths, iFromTeam;
	// Not necessary here, but let's make double sure to initialize.
	iFromOtherKnown = iFromPaths = iFromTeam = 0;
	int iMod = GET_PLAYER(GC.getGame().getActivePlayer()).
			calculateResearchModifier(eTech, &iFromOtherKnown, &iFromPaths, &iFromTeam) - 100;
	int iNonZero = 0;
	if(iFromOtherKnown != 0)
		iNonZero++;
	if(iFromPaths != 0)
		iNonZero++;
	if(iFromTeam != 0)
		iNonZero++;
	if(iNonZero > 0) {
		szBuffer.append(NEWLINE);
		char const* szPos = "COLOR_POSITIVE_TEXT";
		char const* szNeg = "COLOR_NEGATIVE_TEXT";
		int iBullet = gDLL->getSymbolID(BULLET_CHAR);
		szBuffer.append(gDLL->getText("TXT_KEY_RESEARCH_MODIFIER"));
		szBuffer.append(CvWString::format(L": " SETCOLR L"%s%d%%" ENDCOLR,
				TEXT_COLOR(iMod > 0 ? szPos : szNeg), iMod > 0 ? L"+" : L"", iMod));
		if(iFromOtherKnown != 0) {
			if(iNonZero == 1)
				szBuffer.append(L" ");
			else {
				szBuffer.append(NEWLINE);
				szBuffer.append(CvWString::format(L"%c" SETCOLR L"%d%% " ENDCOLR,
					iBullet, TEXT_COLOR(iFromOtherKnown > 0 ? szPos : szNeg),
					iFromOtherKnown));
			}
			szBuffer.append(gDLL->getText("TXT_KEY_RESEARCH_MODIFIER_OTHER_KNOWN"));
		}
		if(iFromPaths != 0) {
			if(iNonZero == 1)
				szBuffer.append(L" ");
			else {
				szBuffer.append(NEWLINE);
				szBuffer.append(CvWString::format(L"%c" SETCOLR L"%d%% " ENDCOLR,
						iBullet, TEXT_COLOR(iFromPaths > 0 ? szPos : szNeg),
						iFromPaths));
			}
			szBuffer.append(gDLL->getText("TXT_KEY_RESEARCH_MODIFIER_PATHS"));
		}
		if(iFromTeam != 0) {
			if(iNonZero == 1)
				szBuffer.append(L" ");
			else {
				szBuffer.append(NEWLINE);
				szBuffer.append(CvWString::format(L"%c" SETCOLR L"%d%% " ENDCOLR,
					iBullet, TEXT_COLOR(iFromTeam > 0 ? szPos : szNeg),
					iFromTeam));
			}
			szBuffer.append(gDLL->getText("TXT_KEY_RESEARCH_MODIFIER_TEAM"));
		}
	}
} // </advc.910>


void CvGameTextMgr::setBasicUnitHelp(CvWStringBuffer &szBuffer, UnitTypes eUnit, bool bCivilopediaText)
{
	PROFILE_FUNC();

	CvWString szTempBuffer;
	bool bFirst;
	int iCount;
	int iI;

	if (NO_UNIT == eUnit)
	{
		return;
	}
	CvUnitInfo const& u = GC.getUnitInfo(eUnit); // advc.003
	if (!bCivilopediaText)
	{
		szBuffer.append(NEWLINE);

		if (u.getDomainType() == DOMAIN_AIR)
		{
			if (u.getAirCombat() > 0)
			{
				szTempBuffer.Format(L"%d%c, ", u.getAirCombat(), gDLL->getSymbolID(STRENGTH_CHAR));
				szBuffer.append(szTempBuffer);
			}
		}
		else
		{
			if (u.getCombat() > 0)
			{
				szTempBuffer.Format(L"%d%c, ", u.getCombat(), gDLL->getSymbolID(STRENGTH_CHAR));
				szBuffer.append(szTempBuffer);
			}
		}
		// <advc.905b>
		bool bAllSpeedBonusesAvailable = true;
		CvCity* pCity = gDLL->getInterfaceIFace()->getHeadSelectedCity();
		if(pCity == NULL)
			bAllSpeedBonusesAvailable = false;
		// "3 MOVES_CHAR (+1 with Coal)" when not all bonuses available
		CvWStringBuffer szSpeedBonuses;
		// "4 MOVES_CHAR (with Coal)" for city screen when all bonuses available
		CvWStringBuffer szSpeedBonusesCompact;
		int iTotalExtraMoves = 0;
		for(int i = 0; i < GC.getNUM_UNIT_PREREQ_OR_BONUSES(); i++) {
			if(u.getSpeedBonuses(i) < 0)
				continue;
			int iExtraMoves = u.getExtraMoves(i);
			if(iExtraMoves == 0)
				continue;
			if(i > 0) {
				szSpeedBonuses.append(L", ");
				szSpeedBonusesCompact.append(L",");
			}
			else {
				szSpeedBonusesCompact.append(gDLL->getText("TXT_KEY_WITH"));
				szSpeedBonusesCompact.append(L" ");
			}
			if(iExtraMoves > 0)
				szSpeedBonuses.append(L"+");
			szTempBuffer.Format(L"%d", iExtraMoves);
			iTotalExtraMoves += iExtraMoves;
			szSpeedBonuses.append(szTempBuffer);
			szSpeedBonuses.append(gDLL->getText("TXT_KEY_WITH_SPACE"));
			BonusTypes eBonus = (BonusTypes)u.getSpeedBonuses(i);
			szTempBuffer.Format(L"%c", GC.getBonusInfo(eBonus).getChar());
			szSpeedBonuses.append(szTempBuffer);
			szSpeedBonusesCompact.append(szTempBuffer);
			if(bAllSpeedBonusesAvailable && !pCity->hasBonus(eBonus))
				bAllSpeedBonusesAvailable = false;
		}
		int iMoves = u.getMoves();
		if(bAllSpeedBonusesAvailable)
			iMoves = std::max(0, iMoves + iTotalExtraMoves);
		// </advc.905b>
		szTempBuffer.Format(L"%d%c", iMoves, gDLL->getSymbolID(MOVES_CHAR));
		szBuffer.append(szTempBuffer);
		// <advc.905b>
		if(!szSpeedBonuses.isEmpty()) {
			// No space b/c there's already a bit of a space after MOVES_CHAR
			szBuffer.append(L"(");
			if(bAllSpeedBonusesAvailable)
				szBuffer.append(szSpeedBonusesCompact);
			else szBuffer.append(szSpeedBonuses);
			szBuffer.append(L")");
		} // </advc.905b>
		if (u.getAirRange() > 0)
		{
			szBuffer.append(L", ");
			szBuffer.append(gDLL->getText("TXT_KEY_UNIT_AIR_RANGE", u.getAirRange()));
		}
	}

	if (u.isGoldenAge())
	{
		szBuffer.append(NEWLINE);
		szBuffer.append(gDLL->getText("TXT_KEY_UNIT_GOLDEN_AGE"));
	}

	if (u.getLeaderExperience() > 0)
	{
		if (GC.getDefineINT("WARLORD_EXTRA_EXPERIENCE_PER_UNIT_PERCENT") == 0)
		{
			szBuffer.append(NEWLINE);
			szBuffer.append(gDLL->getText("TXT_KEY_UNIT_LEADER", u.getLeaderExperience()));
		}
		else
		{
			szBuffer.append(NEWLINE);
			szBuffer.append(gDLL->getText("TXT_KEY_UNIT_LEADER_EXPERIENCE", u.getLeaderExperience()));
		}
	}

	if (NO_PROMOTION != u.getLeaderPromotion())
	{
		szBuffer.append(CvWString::format(L"%s%c%s", NEWLINE, gDLL->getSymbolID(BULLET_CHAR), gDLL->getText("TXT_KEY_PROMOTION_WHEN_LEADING").GetCString()));
		parsePromotionHelp(szBuffer, (PromotionTypes)u.getLeaderPromotion(), L"\n   ");
	}

	if ((u.getBaseDiscover() > 0) || (u.getDiscoverMultiplier() > 0))
	{
		szBuffer.append(NEWLINE);
		szBuffer.append(gDLL->getText("TXT_KEY_UNIT_DISCOVER_TECH"));
	}

	if ((u.getBaseHurry() > 0) || (u.getHurryMultiplier() > 0))
	{
		szBuffer.append(NEWLINE);
		szBuffer.append(gDLL->getText("TXT_KEY_UNIT_HURRY_PRODUCTION"));
	}

	if ((u.getBaseTrade() > 0) || (u.getTradeMultiplier() > 0))
	{
		szBuffer.append(NEWLINE);
		szBuffer.append(gDLL->getText("TXT_KEY_UNIT_TRADE_MISSION"));
	}

	if (u.getGreatWorkCulture() > 0)
	{
		int iCulture = u.getGreatWorkCulture();
		if (NO_GAMESPEED != GC.getGame().getGameSpeedType())
		{
			iCulture *= GC.getGameSpeedInfo(GC.getGame().getGameSpeedType()).getUnitGreatWorkPercent();
			iCulture /= 100;
		}

		szBuffer.append(NEWLINE);
		szBuffer.append(gDLL->getText("TXT_KEY_UNIT_GREAT_WORK", iCulture));
	}

	if (u.getEspionagePoints() > 0)
	{
		int iEspionage = u.getEspionagePoints();
		if (NO_GAMESPEED != GC.getGame().getGameSpeedType())
		{
			iEspionage *= GC.getGameSpeedInfo(GC.getGame().getGameSpeedType()).getUnitGreatWorkPercent();
			iEspionage /= 100;
		}

		szBuffer.append(NEWLINE);
		szBuffer.append(gDLL->getText("TXT_KEY_UNIT_ESPIONAGE_MISSION", iEspionage));
	}

	bFirst = true;

	for (iI = 0; iI < GC.getNumReligionInfos(); ++iI)
	{
		if (u.getReligionSpreads(iI) > 0)
		{
			szTempBuffer.Format(L"%s%s", NEWLINE, gDLL->getText("TXT_KEY_UNIT_CAN_SPREAD").c_str());
			CvWString szReligion;
			szReligion.Format(L"<link=literal>%s</link>", GC.getReligionInfo((ReligionTypes) iI).getDescription());
			setListHelp(szBuffer, szTempBuffer, szReligion, L", ", bFirst);
			bFirst = false;
		}
	}

	bFirst = true;

	for (iI = 0; iI < GC.getNumCorporationInfos(); ++iI)
	{
		if (u.getCorporationSpreads(iI) > 0)
		{
			szTempBuffer.Format(L"%s%s", NEWLINE, gDLL->getText("TXT_KEY_UNIT_CAN_EXPAND").c_str());
			CvWString szCorporation;
			szCorporation.Format(L"<link=literal>%s</link>", GC.getCorporationInfo((CorporationTypes) iI).getDescription());
			setListHelp(szBuffer, szTempBuffer, szCorporation, L", ", bFirst);
			bFirst = false;
		}
	}

	bFirst = true;

	for (iI = 0; iI < GC.getNumSpecialistInfos(); ++iI)
	{
		if (u.getGreatPeoples(iI))
		{
			szTempBuffer.Format(L"%s%s", NEWLINE, gDLL->getText("TXT_KEY_UNIT_CAN_JOIN").c_str());
			CvWString szSpecialistLink = CvWString::format(L"<link=literal>%s</link>", GC.getSpecialistInfo((SpecialistTypes) iI).getDescription());
			setListHelp(szBuffer, szTempBuffer, szSpecialistLink.GetCString(), L", ", bFirst);
			bFirst = false;
		}
	}

	bFirst = true;

	for (iI = 0; iI < GC.getNumBuildingInfos(); ++iI)
	{
		if (u.getBuildings(iI) || u.getForceBuildings(iI))
		{
			szTempBuffer.Format(L"%s%s", NEWLINE, gDLL->getText("TXT_KEY_UNIT_CAN_CONSTRUCT").c_str());
			CvWString szBuildingLink = CvWString::format(L"<link=literal>%s</link>", GC.getBuildingInfo((BuildingTypes) iI).getDescription());
			setListHelp(szBuffer, szTempBuffer, szBuildingLink.GetCString(), L", ", bFirst);
			bFirst = false;
		}
	}

	if (u.getCargoSpace() > 0)
	{
		szBuffer.append(NEWLINE);
		szBuffer.append(gDLL->getText("TXT_KEY_UNIT_CARGO_SPACE", u.getCargoSpace()));

		if (u.getSpecialCargo() != NO_SPECIALUNIT)
		{
			szBuffer.append(gDLL->getText("TXT_KEY_UNIT_CARRIES", GC.getSpecialUnitInfo((SpecialUnitTypes) u.getSpecialCargo()).getTextKeyWide()));
		}
	}

	szTempBuffer.Format(L"%s%s ", NEWLINE, gDLL->getText("TXT_KEY_UNIT_CANNOT_ENTER").GetCString());

	bFirst = true;

	for (iI = 0; iI < GC.getNumTerrainInfos(); ++iI)
	{
		if (u.getTerrainImpassable(iI))
		{
			CvWString szTerrain;
			TechTypes eTech = (TechTypes)u.getTerrainPassableTech(iI);
			if (NO_TECH == eTech)
			{
				szTerrain.Format(L"<link=literal>%s</link>", GC.getTerrainInfo((TerrainTypes)iI).getDescription());
			}
			else
			{
				szTerrain = gDLL->getText("TXT_KEY_TERRAIN_UNTIL_TECH", GC.getTerrainInfo((TerrainTypes)iI).getTextKeyWide(), GC.getTechInfo(eTech).getTextKeyWide());
			}
			setListHelp(szBuffer, szTempBuffer, szTerrain, L", ", bFirst);
			bFirst = false;
		}
	}

	for (iI = 0; iI < GC.getNumFeatureInfos(); ++iI)
	{
		if (u.getFeatureImpassable(iI))
		{
			CvWString szFeature;
			TechTypes eTech = (TechTypes)u.getTerrainPassableTech(iI);
			if (NO_TECH == eTech)
			{
				szFeature.Format(L"<link=literal>%s</link>", GC.getFeatureInfo((FeatureTypes)iI).getDescription());
			}
			else
			{
				szFeature = gDLL->getText("TXT_KEY_TERRAIN_UNTIL_TECH", GC.getFeatureInfo((FeatureTypes)iI).getTextKeyWide(), GC.getTechInfo(eTech).getTextKeyWide());
			}
			setListHelp(szBuffer, szTempBuffer, szFeature, L", ", bFirst);
			bFirst = false;
		}
	}

	if (u.isInvisible())
	{
		szBuffer.append(NEWLINE);
		szBuffer.append(gDLL->getText("TXT_KEY_UNIT_INVISIBLE_ALL"));
	}
	else if (u.getInvisibleType() != NO_INVISIBLE)
	{
		szBuffer.append(NEWLINE);
		szBuffer.append(gDLL->getText("TXT_KEY_UNIT_INVISIBLE_MOST"));
	}

	for (iI = 0; iI < u.getNumSeeInvisibleTypes(); ++iI)
	{
		if (bCivilopediaText || (u.getSeeInvisibleType(iI) != u.getInvisibleType()))
		{
			szBuffer.append(NEWLINE);
			szBuffer.append(gDLL->getText("TXT_KEY_UNIT_SEE_INVISIBLE", GC.getInvisibleInfo((InvisibleTypes) u.getSeeInvisibleType(iI)).getTextKeyWide()));
		}
	}

	if (u.isCanMoveImpassable())
	{
		szBuffer.append(NEWLINE);
		szBuffer.append(gDLL->getText("TXT_KEY_UNIT_CAN_MOVE_IMPASSABLE"));
	}


	if (u.isNoBadGoodies())
	{
		szBuffer.append(NEWLINE);
		szBuffer.append(gDLL->getText("TXT_KEY_UNIT_NO_BAD_GOODIES"));
	}

	if (u.isHiddenNationality())
	{
		szBuffer.append(NEWLINE);
		szBuffer.append(gDLL->getText("TXT_KEY_UNIT_HIDDEN_NATIONALITY"));
	}

	if (u.isAlwaysHostile())
	{	/*  advc.033: Hidden nationality sounds like it implies "attack without
			declaring war". Only show the latter when the former isn't true. */
		if(!u.isHiddenNationality()) {
			szBuffer.append(NEWLINE);
			szBuffer.append(gDLL->getText("TXT_KEY_UNIT_ALWAYS_HOSTILE"));
		}
		// <advc.033> Not technically a unit ability, but should be regarded as one.
		if(u.isPillage() && u.getDomainType() == DOMAIN_SEA) {
			szBuffer.append(NEWLINE);
			szBuffer.append(gDLL->getText("TXT_KEY_UNIT_CAN_PLUNDER"));
		} // </advc.033>
	}

	if (u.isOnlyDefensive())
	{
		szBuffer.append(NEWLINE);
		szBuffer.append(gDLL->getText("TXT_KEY_UNIT_ONLY_DEFENSIVE"));
	}
	// <advc.315a>
	if (u.isOnlyAttackAnimals())
	{
		szBuffer.append(NEWLINE);
		szBuffer.append(gDLL->getText("TXT_KEY_UNIT_ONLY_ATTACK_ANIMALS"));
	} // </advc.315a>
	// <advc.315b>
	if (u.isOnlyAttackBarbarians())
	{
		szBuffer.append(NEWLINE);
		szBuffer.append(gDLL->getText("TXT_KEY_UNIT_ONLY_ATTACK_BARBARIANS"));
	} // </advc.315b>

	if (u.isNoCapture())
	{
		szBuffer.append(NEWLINE);
		szBuffer.append(gDLL->getText("TXT_KEY_UNIT_CANNOT_CAPTURE"));
	}

	if (u.isRivalTerritory())
	{
		szBuffer.append(NEWLINE);
		szBuffer.append(gDLL->getText("TXT_KEY_UNIT_EXPLORE_RIVAL"));
	}

	if (u.isFound())
	{
		szBuffer.append(NEWLINE);
		szBuffer.append(gDLL->getText("TXT_KEY_UNIT_FOUND_CITY"));
	}

	iCount = 0;

	for (iI = 0; iI < GC.getNumBuildInfos(); ++iI)
	{
		if (u.getBuilds(iI))
		{
			iCount++;
		}
	}

	if (iCount > ((GC.getNumBuildInfos() * 3) / 4))
	{
		szBuffer.append(NEWLINE);
		szBuffer.append(gDLL->getText("TXT_KEY_UNIT_IMPROVE_PLOTS"));
	}
	else
	{
		bFirst = true;

		for (iI = 0; iI < GC.getNumBuildInfos(); ++iI)
		{
			if (u.getBuilds(iI))
			{	// <advc.004w>
				TechTypes eTech = (TechTypes)GC.getBuildInfo((BuildTypes)iI).getTechPrereq();
				PlayerTypes ePlayer = GC.getGame().getActivePlayer();
				if(eTech != NO_TECH && ePlayer != NO_PLAYER && GC.getTechInfo(eTech).
						getEra() - GET_PLAYER(ePlayer).getCurrentEra() > 1)
					continue; // </advc.004w>
				// K-Mod, 4/jan/11, karadoc: removed the space on the next line. (ie. from "%s%s ", to "%s%s")
				szTempBuffer.Format(L"%s%s", NEWLINE, gDLL->getText("TXT_KEY_UNIT_CAN").c_str());
				setListHelp(szBuffer, szTempBuffer, GC.getBuildInfo((BuildTypes) iI).getDescription(), L", ", bFirst);
				bFirst = false;
			}
		}
	}

	if (u.getNukeRange() != -1)
	{
		szBuffer.append(NEWLINE);
		szBuffer.append(gDLL->getText("TXT_KEY_UNIT_CAN_NUKE"));
	}

	if (u.isCounterSpy())
	{
		szBuffer.append(NEWLINE);
		szBuffer.append(gDLL->getText("TXT_KEY_UNIT_EXPOSE_SPIES"));
	}

	if ((u.getFirstStrikes() + u.getChanceFirstStrikes()) > 0)
	{
		if (u.getChanceFirstStrikes() == 0)
		{
			if (u.getFirstStrikes() == 1)
			{
				szBuffer.append(NEWLINE);
				szBuffer.append(gDLL->getText("TXT_KEY_UNIT_ONE_FIRST_STRIKE"));
			}
			else
			{
				szBuffer.append(NEWLINE);
				szBuffer.append(gDLL->getText("TXT_KEY_UNIT_NUM_FIRST_STRIKES", u.getFirstStrikes()));
			}
		}
		else
		{
			szBuffer.append(NEWLINE);
			szBuffer.append(gDLL->getText("TXT_KEY_UNIT_FIRST_STRIKE_CHANCES", u.getFirstStrikes(), u.getFirstStrikes() + u.getChanceFirstStrikes()));
		}
	}

	if (u.isFirstStrikeImmune())
	{
		szBuffer.append(NEWLINE);
		szBuffer.append(gDLL->getText("TXT_KEY_UNIT_IMMUNE_FIRST_STRIKES"));
	}

	if (u.isNoDefensiveBonus())
	{
		szBuffer.append(NEWLINE);
		szBuffer.append(gDLL->getText("TXT_KEY_UNIT_NO_DEFENSE_BONUSES"));
	}

	if (u.isFlatMovementCost())
	{
		szBuffer.append(NEWLINE);
		szBuffer.append(gDLL->getText("TXT_KEY_UNIT_FLAT_MOVEMENT"));
	}

	if (u.isIgnoreTerrainCost())
	{
		szBuffer.append(NEWLINE);
		szBuffer.append(gDLL->getText("TXT_KEY_UNIT_IGNORE_TERRAIN"));
	}

	if (u.getInterceptionProbability() > 0)
	{
		szBuffer.append(NEWLINE);
		szBuffer.append(gDLL->getText("TXT_KEY_UNIT_INTERCEPT_AIRCRAFT", u.getInterceptionProbability()));
	}

	if (u.getEvasionProbability() > 0)
	{
		szBuffer.append(NEWLINE);
		szBuffer.append(gDLL->getText("TXT_KEY_UNIT_EVADE_INTERCEPTION", u.getEvasionProbability()));
	}

	if (u.getWithdrawalProbability() > 0)
	{
		szBuffer.append(NEWLINE);
		szBuffer.append(gDLL->getText("TXT_KEY_UNIT_WITHDRAWL_PROBABILITY", u.getWithdrawalProbability()));
	}

	if (u.getCombatLimit() < GC.getMAX_HIT_POINTS() && u.getCombat() > 0 && !u.isOnlyDefensive())
	{
		szBuffer.append(NEWLINE);
		szBuffer.append(gDLL->getText("TXT_KEY_UNIT_COMBAT_LIMIT", (100 * u.getCombatLimit()) / GC.getMAX_HIT_POINTS()));
	}

	if (u.getCollateralDamage() > 0)
	{
		szBuffer.append(NEWLINE);
		szBuffer.append(gDLL->getText("TXT_KEY_UNIT_COLLATERAL_DAMAGE", ( 100 * u.getCollateralDamageLimit() / GC.getMAX_HIT_POINTS())));
	}

	for (iI = 0; iI < GC.getNumUnitCombatInfos(); ++iI)
	{
		if (u.getUnitCombatCollateralImmune(iI))
		{
			szBuffer.append(NEWLINE);
			szBuffer.append(gDLL->getText("TXT_KEY_UNIT_COLLATERAL_IMMUNE", GC.getUnitCombatInfo((UnitCombatTypes)iI).getTextKeyWide()));
		}
	}

	if (u.getCityAttackModifier() == u.getCityDefenseModifier())
	{
		if (u.getCityAttackModifier() != 0)
		{
			szBuffer.append(NEWLINE);
			szBuffer.append(gDLL->getText("TXT_KEY_UNIT_CITY_STRENGTH_MOD", u.getCityAttackModifier()));
		}
	}
	else
	{
		if (u.getCityAttackModifier() != 0)
		{
			szBuffer.append(NEWLINE);
			szBuffer.append(gDLL->getText("TXT_KEY_UNIT_CITY_ATTACK_MOD", u.getCityAttackModifier()));
		}

		if (u.getCityDefenseModifier() != 0)
		{
			szBuffer.append(NEWLINE);
			szBuffer.append(gDLL->getText("TXT_KEY_UNIT_CITY_DEFENSE_MOD", u.getCityDefenseModifier()));
		}
	}

	if (u.getAnimalCombatModifier() != 0)
	{
		szBuffer.append(NEWLINE);
		szBuffer.append(gDLL->getText("TXT_KEY_UNIT_ANIMAL_COMBAT_MOD", u.getAnimalCombatModifier()));
	}
	// <advc.315c>
	if (u.getBarbarianCombatModifier() != 0)
	{
		szBuffer.append(NEWLINE);
		szBuffer.append(gDLL->getText("TXT_KEY_UNIT_BARBARIAN_COMBAT_MOD",
				u.getBarbarianCombatModifier()));
	} // </advc.315c>

	if (u.getDropRange() > 0)
	{
		szBuffer.append(NEWLINE);
		szBuffer.append(gDLL->getText("TXT_KEY_UNIT_PARADROP_RANGE", u.getDropRange()));
	}

	if (u.getHillsDefenseModifier() == u.getHillsAttackModifier())
	{
		if (u.getHillsAttackModifier() != 0)
		{
			szBuffer.append(NEWLINE);
			szBuffer.append(gDLL->getText("TXT_KEY_UNIT_HILLS_STRENGTH", u.getHillsAttackModifier()));
		}
	}
	else
	{
		if (u.getHillsAttackModifier() != 0)
		{
			szBuffer.append(NEWLINE);
			szBuffer.append(gDLL->getText("TXT_KEY_UNIT_HILLS_ATTACK", u.getHillsAttackModifier()));
		}

		if (u.getHillsDefenseModifier() != 0)
		{
			szBuffer.append(NEWLINE);
			szBuffer.append(gDLL->getText("TXT_KEY_UNIT_HILLS_DEFENSE", u.getHillsDefenseModifier()));
		}
	}

	for (iI = 0; iI < GC.getNumTerrainInfos(); ++iI)
	{
		if (u.getTerrainDefenseModifier(iI) != 0)
		{
			szBuffer.append(NEWLINE);
			szBuffer.append(gDLL->getText("TXT_KEY_UNIT_DEFENSE", u.getTerrainDefenseModifier(iI), GC.getTerrainInfo((TerrainTypes) iI).getTextKeyWide()));
		}
		if (u.getTerrainAttackModifier(iI) != 0)
		{
			szBuffer.append(NEWLINE);
			szBuffer.append(gDLL->getText("TXT_KEY_UNIT_ATTACK", u.getTerrainAttackModifier(iI), GC.getTerrainInfo((TerrainTypes) iI).getTextKeyWide()));
		}
	}

	for (iI = 0; iI < GC.getNumFeatureInfos(); ++iI)
	{
		if (u.getFeatureDefenseModifier(iI) != 0)
		{
			szBuffer.append(NEWLINE);
			szBuffer.append(gDLL->getText("TXT_KEY_UNIT_DEFENSE", u.getFeatureDefenseModifier(iI), GC.getFeatureInfo((FeatureTypes) iI).getTextKeyWide()));
		}

		if (u.getFeatureAttackModifier(iI) != 0)
		{
			szBuffer.append(NEWLINE);
			szBuffer.append(gDLL->getText("TXT_KEY_UNIT_ATTACK", u.getFeatureAttackModifier(iI), GC.getFeatureInfo((FeatureTypes) iI).getTextKeyWide()));
		}
	}

	for (iI = 0; iI < GC.getNumUnitClassInfos(); ++iI)
	{
		if (u.getUnitClassAttackModifier(iI) == u.getUnitClassDefenseModifier(iI))
		{
			if (u.getUnitClassAttackModifier(iI) != 0)
			{
				szBuffer.append(NEWLINE);
				szBuffer.append(gDLL->getText("TXT_KEY_UNIT_MOD_VS_TYPE", u.getUnitClassAttackModifier(iI), GC.getUnitClassInfo((UnitClassTypes)iI).getTextKeyWide()));
			}
		}
		else
		{
			if (u.getUnitClassAttackModifier(iI) != 0)
			{
				szBuffer.append(NEWLINE);
				szBuffer.append(gDLL->getText("TXT_KEY_UNIT_ATTACK_MOD_VS_CLASS", u.getUnitClassAttackModifier(iI), GC.getUnitClassInfo((UnitClassTypes)iI).getTextKeyWide()));
			}

			if (u.getUnitClassDefenseModifier(iI) != 0)
			{
				szBuffer.append(NEWLINE);
				szBuffer.append(gDLL->getText("TXT_KEY_UNIT_DEFENSE_MOD_VS_CLASS", u.getUnitClassDefenseModifier(iI), GC.getUnitClassInfo((UnitClassTypes) iI).getTextKeyWide()));
			}
		}
	}

	for (iI = 0; iI < GC.getNumUnitCombatInfos(); ++iI)
	{
		if (u.getUnitCombatModifier(iI) != 0)
		{
			szBuffer.append(NEWLINE);
			szBuffer.append(gDLL->getText("TXT_KEY_UNIT_MOD_VS_TYPE", u.getUnitCombatModifier(iI), GC.getUnitCombatInfo((UnitCombatTypes) iI).getTextKeyWide()));
		}
	}

	for (iI = 0; iI < NUM_DOMAIN_TYPES; ++iI)
	{
		if (u.getDomainModifier(iI) != 0)
		{
			szBuffer.append(NEWLINE);
			szBuffer.append(gDLL->getText("TXT_KEY_UNIT_MOD_VS_TYPE_NO_LINK", u.getDomainModifier(iI), GC.getDomainInfo((DomainTypes)iI).getTextKeyWide()));
		}
	}

	szTempBuffer.clear();
	bFirst = true;
	for (iI = 0; iI < GC.getNumUnitClassInfos(); ++iI)
	{
		if (u.getTargetUnitClass(iI))
		{
			if (bFirst)
			{
				bFirst = false;
			}
			else
			{
				szTempBuffer += L", ";
			}

			szTempBuffer += CvWString::format(L"<link=literal>%s</link>", GC.getUnitClassInfo((UnitClassTypes)iI).getDescription());
		}
	}

	if (!bFirst)
	{
		szBuffer.append(NEWLINE);
		szBuffer.append(gDLL->getText("TXT_KEY_UNIT_TARGETS_UNIT_FIRST", szTempBuffer.GetCString()));
	}

	szTempBuffer.clear();
	bFirst = true;
	for (iI = 0; iI < GC.getNumUnitClassInfos(); ++iI)
	{
		if (u.getDefenderUnitClass(iI))
		{
			if (bFirst)
			{
				bFirst = false;
			}
			else
			{
				szTempBuffer += L", ";
			}

			szTempBuffer += CvWString::format(L"<link=literal>%s</link>", GC.getUnitClassInfo((UnitClassTypes)iI).getDescription());
		}
	}

	if (!bFirst)
	{
		szBuffer.append(NEWLINE);
		szBuffer.append(gDLL->getText("TXT_KEY_UNIT_DEFENDS_UNIT_FIRST", szTempBuffer.GetCString()));
	}

	szTempBuffer.clear();
	bFirst = true;
	for (iI = 0; iI < GC.getNumUnitCombatInfos(); ++iI)
	{
		if (u.getTargetUnitCombat(iI))
		{
			if (bFirst)
			{
				bFirst = false;
			}
			else
			{
				szTempBuffer += L", ";
			}

			szTempBuffer += CvWString::format(L"<link=literal>%s</link>", GC.getUnitCombatInfo((UnitCombatTypes)iI).getDescription());
		}
	}

	if (!bFirst)
	{
		szBuffer.append(NEWLINE);
		szBuffer.append(gDLL->getText("TXT_KEY_UNIT_TARGETS_UNIT_FIRST", szTempBuffer.GetCString()));
	}

	szTempBuffer.clear();
	bFirst = true;
	for (iI = 0; iI < GC.getNumUnitCombatInfos(); ++iI)
	{
		if (u.getDefenderUnitCombat(iI))
		{
			if (bFirst)
			{
				bFirst = false;
			}
			else
			{
				szTempBuffer += L", ";
			}

			szTempBuffer += CvWString::format(L"<link=literal>%s</link>", GC.getUnitCombatInfo((UnitCombatTypes)iI).getDescription());
		}
	}

	if (!bFirst)
	{
		szBuffer.append(NEWLINE);
		szBuffer.append(gDLL->getText("TXT_KEY_UNIT_DEFENDS_UNIT_FIRST", szTempBuffer.GetCString()));
	}

	szTempBuffer.clear();
	bFirst = true;
	for (iI = 0; iI < GC.getNumUnitClassInfos(); ++iI)
	{
		if (u.getFlankingStrikeUnitClass(iI) > 0)
		{
			if (bFirst)
			{
				bFirst = false;
			}
			else
			{
				szTempBuffer += L", ";
			}

			szTempBuffer += CvWString::format(L"<link=literal>%s</link>", GC.getUnitClassInfo((UnitClassTypes)iI).getDescription());
		}
	}

	if (!bFirst)
	{
		szBuffer.append(NEWLINE);
		szBuffer.append(gDLL->getText("TXT_KEY_UNIT_FLANKING_STRIKES", szTempBuffer.GetCString()));
	}

	if (u.getBombRate() > 0)
	{
		szBuffer.append(NEWLINE);
		szBuffer.append(gDLL->getText("TXT_KEY_UNIT_BOMB_RATE", ((u.getBombRate() * 100) / GC.getMAX_CITY_DEFENSE_DAMAGE())));
	}

	if (u.getBombardRate() > 0)
	{
		szBuffer.append(NEWLINE);
		szBuffer.append(gDLL->getText("TXT_KEY_UNIT_BOMBARD_RATE", ((u.getBombardRate() * 100) / GC.getMAX_CITY_DEFENSE_DAMAGE())));
	}
	// <advc.905b>
	if(bCivilopediaText) {
		for(int i = 0; i < GC.getNUM_UNIT_PREREQ_OR_BONUSES(); i++) {
			if(u.getSpeedBonuses(i) < 0)
				continue;
			int iExtraMoves = u.getExtraMoves(i);
			if(iExtraMoves == 0)
				continue;
			szBuffer.append(NEWLINE);
			szTempBuffer.Format(L"%c", gDLL->getSymbolID(BULLET_CHAR));
			szBuffer.append(szTempBuffer);
			if(iExtraMoves > 0)
				szBuffer.append(L"+");
			szTempBuffer.Format(L"%d%c", iExtraMoves, gDLL->getSymbolID(MOVES_CHAR));
			szBuffer.append(szTempBuffer);
			szBuffer.append(gDLL->getText("TXT_KEY_WITH") + L" ");
			szTempBuffer.Format(L"<link=literal>%s</link>", GC.getBonusInfo((BonusTypes)
					u.getSpeedBonuses(i)).getDescription());
			szBuffer.append(szTempBuffer);
		}
	} // </advc.905b>
	bFirst = true;

	for (iI = 0; iI < GC.getNumPromotionInfos(); ++iI)
	{
		if (u.getFreePromotions(iI))
		{
			szTempBuffer.Format(L"%s%s", NEWLINE, gDLL->getText("TXT_KEY_UNIT_STARTS_WITH").c_str());
			setListHelp(szBuffer, szTempBuffer, CvWString::format(L"<link=literal>%s</link>", GC.getPromotionInfo((PromotionTypes) iI).getDescription()), L", ", bFirst);
			bFirst = false;
		}
	}

	if (u.getExtraCost() != 0)
	{
		szBuffer.append(NEWLINE);
		szBuffer.append(gDLL->getText("TXT_KEY_UNIT_EXTRA_COST", u.getExtraCost()));
	}
	// (advc.004w: ProductionTraits moved into setProductionSpeedHelp)
	if (!CvWString(u.getHelp()).empty())
	{
		szBuffer.append(NEWLINE);
		szBuffer.append(u.getHelp());
	}
}


void CvGameTextMgr::setUnitHelp(CvWStringBuffer &szBuffer, UnitTypes eUnit, bool bCivilopediaText, bool bStrategyText, bool bTechChooserText, CvCity* pCity)
{
	PROFILE_FUNC();

	CvWString szTempBuffer;
	int iI;

	if (NO_UNIT == eUnit)
	{
		return;
	}
	CvUnitInfo const& u = GC.getUnitInfo(eUnit);
	PlayerTypes ePlayer = NO_PLAYER;
	if (pCity != NULL)
	{
		ePlayer = pCity->getOwner();
	}
	else
	{
		ePlayer = GC.getGame().getActivePlayer();
	}

	if (!bCivilopediaText)
	{
		szTempBuffer.Format(SETCOLR L"%s" ENDCOLR , TEXT_COLOR("COLOR_UNIT_TEXT"), u.getDescription());
		szBuffer.append(szTempBuffer);

		if (u.getUnitCombatType() != NO_UNITCOMBAT)
		{
			szTempBuffer.Format(L" (%s)", GC.getUnitCombatInfo((UnitCombatTypes) u.getUnitCombatType()).getDescription());
			szBuffer.append(szTempBuffer);
		}
	}
	// test for unique unit
	UnitClassTypes eUnitClass = (UnitClassTypes)u.getUnitClassType();
	CvUnitClassInfo const& kUnitClass = GC.getUnitClassInfo(eUnitClass); // advc.003
	UnitTypes eDefaultUnit = (UnitTypes)kUnitClass.getDefaultUnitIndex();

	if (NO_UNIT != eDefaultUnit && eDefaultUnit != eUnit)
	{
		for (iI  = 0; iI < GC.getNumCivilizationInfos(); ++iI)
		{
			UnitTypes eUniqueUnit = (UnitTypes)GC.getCivilizationInfo((CivilizationTypes)iI).getCivilizationUnits((int)eUnitClass);
			if (eUniqueUnit == eUnit)
			{
				szBuffer.append(NEWLINE);
				szBuffer.append(gDLL->getText("TXT_KEY_UNIQUE_UNIT", GC.getCivilizationInfo((CivilizationTypes)iI).getTextKeyWide()));
			}
		}

		szBuffer.append(NEWLINE);
		szBuffer.append(gDLL->getText("TXT_KEY_REPLACES_UNIT", GC.getUnitInfo(eDefaultUnit).getTextKeyWide()));
	}

	if (isWorldUnitClass(eUnitClass))
	{
		if (pCity == NULL)
		{
			szBuffer.append(NEWLINE);
			szBuffer.append(gDLL->getText("TXT_KEY_UNIT_WORLD_UNIT_ALLOWED", kUnitClass.getMaxGlobalInstances()));
		}
		else
		{
			//szBuffer.append(gDLL->getText("TXT_KEY_UNIT_WORLD_UNIT_LEFT", (kUnitClass.getMaxGlobalInstances() - (ePlayer != NO_PLAYER ? GC.getGame().getUnitClassCreatedCount(eUnitClass) + GET_TEAM(GET_PLAYER(ePlayer).getTeam()).getUnitClassMaking(eUnitClass) : 0))));
			// K-Mod
			int iRemaining = std::max(0, kUnitClass.getMaxGlobalInstances() - (ePlayer != NO_PLAYER ? GC.getGame().getUnitClassCreatedCount(eUnitClass) + GET_TEAM(GET_PLAYER(ePlayer).getTeam()).getUnitClassMaking(eUnitClass) : 0));
			szBuffer.append(NEWLINE);

			if (iRemaining <= 0)
				szBuffer.append(CvWString::format(SETCOLR, TEXT_COLOR("COLOR_WARNING_TEXT")));

			szBuffer.append(gDLL->getText("TXT_KEY_UNIT_WORLD_UNIT_LEFT", iRemaining));

			if (iRemaining <= 0)
				szBuffer.append(ENDCOLR);
			// K-Mod end
		}
	}

	if (isTeamUnitClass(eUnitClass))
	{
		if (pCity == NULL)
		{
			szBuffer.append(NEWLINE);
			szBuffer.append(gDLL->getText("TXT_KEY_UNIT_TEAM_UNIT_ALLOWED", kUnitClass.getMaxTeamInstances()));
		}
		else
		{
			//szBuffer.append(gDLL->getText("TXT_KEY_UNIT_TEAM_UNIT_LEFT", (kUnitClass.getMaxTeamInstances() - (ePlayer != NO_PLAYER ? GET_TEAM(GET_PLAYER(ePlayer).getTeam()).getUnitClassCountPlusMaking(eUnitClass) : 0))));
			// K-Mod
			int iRemaining = std::max(0, kUnitClass.getMaxTeamInstances() - (ePlayer != NO_PLAYER ? GET_TEAM(GET_PLAYER(ePlayer).getTeam()).getUnitClassCountPlusMaking(eUnitClass) : 0));
			szBuffer.append(NEWLINE);

			if (iRemaining <= 0)
				szBuffer.append(CvWString::format(SETCOLR, TEXT_COLOR("COLOR_WARNING_TEXT")));

			szBuffer.append(gDLL->getText("TXT_KEY_UNIT_TEAM_UNIT_LEFT", iRemaining));

			if (iRemaining <= 0)
				szBuffer.append(ENDCOLR);
			// K-Mod end
		}
	}

	if (isNationalUnitClass(eUnitClass))
	{
		if (pCity == NULL)
		{
			szBuffer.append(NEWLINE);
			szBuffer.append(gDLL->getText("TXT_KEY_UNIT_NATIONAL_UNIT_ALLOWED", kUnitClass.getMaxPlayerInstances()));
		}
		else
		{
			//szBuffer.append(gDLL->getText("TXT_KEY_UNIT_NATIONAL_UNIT_LEFT", (kUnitClass.getMaxPlayerInstances() - (ePlayer != NO_PLAYER ? GET_PLAYER(ePlayer).getUnitClassCountPlusMaking(eUnitClass) : 0))));
			// K-Mod
			int iRemaining = std::max(0, kUnitClass.getMaxPlayerInstances() - (ePlayer != NO_PLAYER ? GET_PLAYER(ePlayer).getUnitClassCountPlusMaking(eUnitClass) : 0));
			szBuffer.append(NEWLINE);

			if (iRemaining <= 0)
				szBuffer.append(CvWString::format(SETCOLR, TEXT_COLOR("COLOR_WARNING_TEXT")));

			szBuffer.append(gDLL->getText("TXT_KEY_UNIT_NATIONAL_UNIT_LEFT", iRemaining));

			if (iRemaining <= 0)
				szBuffer.append(ENDCOLR);
			// K-Mod end
		}
	}

	setBasicUnitHelp(szBuffer, eUnit, bCivilopediaText);
	// advc.004g: Swapped so that BasicUnitHelp is printed before InstanceCostModifier
	if (kUnitClass.getInstanceCostModifier() != 0)
	{
		szBuffer.append(NEWLINE);
		szBuffer.append(gDLL->getText("TXT_KEY_UNIT_INSTANCE_COST_MOD",
				kUnitClass.getInstanceCostModifier(),
				// advc.004g: Now also requires the unit class name
				kUnitClass.getDescription()));
	}

	if ((pCity == NULL) || !(pCity->canTrain(eUnit)))
	{
		if (pCity != NULL)
		{
			if (GC.getGame().isNoNukes())
			{
				if (u.getNukeRange() != -1)
				{
					szBuffer.append(NEWLINE);
					szBuffer.append(gDLL->getText("TXT_KEY_UNIT_NO_NUKES"));
				}
			}
		}

		if (u.getHolyCity() != NO_RELIGION)
		{
			if ((pCity == NULL) || !(pCity->isHolyCity((ReligionTypes)(u.getHolyCity()))))
			{
				szBuffer.append(NEWLINE);
				szBuffer.append(gDLL->getText("TXT_KEY_UNIT_REQUIRES_HOLY_CITY", GC.getReligionInfo((ReligionTypes)(u.getHolyCity())).getChar()));
			}
		}

		bool bFirst = true;

		if (u.getSpecialUnitType() != NO_SPECIALUNIT)
		{
			if ((pCity == NULL) || !(GC.getGame().isSpecialUnitValid((SpecialUnitTypes)(u.getSpecialUnitType()))))
			{
				for (iI = 0; iI < GC.getNumProjectInfos(); ++iI)
				{
					if (GC.getProjectInfo((ProjectTypes)iI).getEveryoneSpecialUnit() == u.getSpecialUnitType())
					{
						szTempBuffer.Format(L"%s%s", NEWLINE, gDLL->getText("TXT_KEY_REQUIRES").c_str());
						CvWString szProject;
						szProject.Format(L"<link=literal>%s</link>", GC.getProjectInfo((ProjectTypes)iI).getDescription());
						setListHelp(szBuffer, szTempBuffer, szProject, gDLL->getText("TXT_KEY_OR").c_str(), bFirst);
						bFirst = false;
					}
				}
			}
		}

		if (!bFirst)
		{
			szBuffer.append(ENDCOLR);
		}

		bFirst = true;

		if (u.getNukeRange() != -1)
		{
			if (NULL == pCity || !GC.getGame().isNukesValid())
			{
				for (iI = 0; iI < GC.getNumProjectInfos(); ++iI)
				{
					if (GC.getProjectInfo((ProjectTypes)iI).isAllowsNukes())
					{
						szTempBuffer.Format(L"%s%s", NEWLINE, gDLL->getText("TXT_KEY_REQUIRES").c_str());
						CvWString szProject;
						szProject.Format(L"<link=literal>%s</link>", GC.getProjectInfo((ProjectTypes)iI).getDescription());
						setListHelp(szBuffer, szTempBuffer, szProject, gDLL->getText("TXT_KEY_OR").c_str(), bFirst);
						bFirst = false;
					}
				}

				for (iI = 0; iI < GC.getNumBuildingInfos(); ++iI)
				{
					if (GC.getBuildingInfo((BuildingTypes)iI).isAllowsNukes())
					{
						szTempBuffer.Format(L"%s%s", NEWLINE, gDLL->getText("TXT_KEY_REQUIRES").c_str());
						CvWString szBuilding;
						szBuilding.Format(L"<link=literal>%s</link>", GC.getBuildingInfo((BuildingTypes)iI).getDescription());
						setListHelp(szBuffer, szTempBuffer, szBuilding, gDLL->getText("TXT_KEY_OR").c_str(), bFirst);
						bFirst = false;
					}
				}
			}
		}

		if (!bFirst)
		{
			szBuffer.append(ENDCOLR);
		}

		if (!bCivilopediaText)
		{
			if (u.getPrereqBuilding() != NO_BUILDING)
			{
				if ((pCity == NULL) || (pCity->getNumBuilding((BuildingTypes)(u.getPrereqBuilding())) <= 0))
				{
					// K-Mod. Check if the player has an exemption from the building requirement.
					SpecialBuildingTypes eSpecialBuilding = (SpecialBuildingTypes)GC.getBuildingInfo((BuildingTypes)(u.getPrereqBuilding())).getSpecialBuildingType();

					if (pCity == NULL || eSpecialBuilding == NO_SPECIALBUILDING || !GET_PLAYER(pCity->getOwner()).isSpecialBuildingNotRequired(eSpecialBuilding))
					{
					// K-Mod end
						szBuffer.append(NEWLINE);
						szBuffer.append(gDLL->getText("TXT_KEY_UNIT_REQUIRES_STRING", GC.getBuildingInfo((BuildingTypes)(u.getPrereqBuilding())).getTextKeyWide()));
					}
				}
			}

			if (!bTechChooserText)
			{
				if (u.getPrereqAndTech() != NO_TECH)
				{
					if (GC.getGame().getActivePlayer() == NO_PLAYER || !(GET_TEAM(GET_PLAYER(ePlayer).getTeam()).isHasTech((TechTypes)(u.getPrereqAndTech()))))
					{
						szBuffer.append(NEWLINE);
						szBuffer.append(gDLL->getText("TXT_KEY_UNIT_REQUIRES_STRING", GC.getTechInfo((TechTypes)(u.getPrereqAndTech())).getTextKeyWide()));
					}
				}
			}

			bFirst = true;

			for (iI = 0; iI < GC.getNUM_UNIT_AND_TECH_PREREQS(); ++iI)
			{
				if (u.getPrereqAndTechs(iI) != NO_TECH)
				{
					if (bTechChooserText || GC.getGame().getActivePlayer() == NO_PLAYER || !(GET_TEAM(GET_PLAYER(ePlayer).getTeam()).isHasTech((TechTypes)(u.getPrereqAndTechs(iI)))))
					{
						szTempBuffer.Format(L"%s%s", NEWLINE, gDLL->getText("TXT_KEY_REQUIRES").c_str());
						setListHelp(szBuffer, szTempBuffer, GC.getTechInfo(((TechTypes)(u.getPrereqAndTechs(iI)))).getDescription(), gDLL->getText("TXT_KEY_AND").c_str(), bFirst);
						bFirst = false;
					}
				}
			}

			if (!bFirst)
			{
				szBuffer.append(ENDCOLR);
			}

			if (u.getPrereqAndBonus() != NO_BONUS)
			{
				if ((pCity == NULL) || !(pCity->hasBonus((BonusTypes)u.getPrereqAndBonus())))
				{
					szBuffer.append(NEWLINE);
					szBuffer.append(gDLL->getText("TXT_KEY_UNIT_REQUIRES_STRING", GC.getBonusInfo((BonusTypes)(u.getPrereqAndBonus())).getTextKeyWide()));
				}
			}

			bFirst = true;

			for (iI = 0; iI < GC.getNUM_UNIT_PREREQ_OR_BONUSES(); ++iI)
			{
				if (u.getPrereqOrBonuses(iI) != NO_BONUS)
				{
					if ((pCity == NULL) || !(pCity->hasBonus((BonusTypes)u.getPrereqOrBonuses(iI))))
					{
						szTempBuffer.Format(L"%s%s", NEWLINE, gDLL->getText("TXT_KEY_REQUIRES").c_str());
						setListHelp(szBuffer, szTempBuffer, GC.getBonusInfo((BonusTypes) u.getPrereqOrBonuses(iI)).getDescription(), gDLL->getText("TXT_KEY_OR").c_str(), bFirst);
						bFirst = false;
					}
				}
			}

			if (!bFirst)
			{
				szBuffer.append(ENDCOLR);
			}
		}
	} /* <advc.004> Show this right before the cost in Civilopedia text and
		 otherwise (i.e. in hover text) after the cost */
	if(bCivilopediaText)
		setProductionSpeedHelp(szBuffer, ORDER_TRAIN, &u, pCity, true);
	// </advc.004w>
	// advc.004y: Commented out
	//if (!bCivilopediaText && GC.getGame().getActivePlayer() != NO_PLAYER)
	if (pCity == NULL)
	{
		if(u.getProductionCost() > 0) { // advc.004y
			szTempBuffer.Format(L"%s%d%c", NEWLINE,
					// <advc.004y>
					(GC.getGame().getActivePlayer() == NO_PLAYER ?
					u.getProductionCost() : // </advc.004y>
					GET_PLAYER(ePlayer).getProductionNeeded(eUnit)),
					GC.getYieldInfo(YIELD_PRODUCTION).getChar());
			szBuffer.append(szTempBuffer);
		}
	}
	else
	{
		szBuffer.append(NEWLINE);
		int iTurns = pCity->getProductionTurnsLeft(eUnit,
				(GC.ctrlKey() || !GC.shiftKey()) ? 0 : pCity->getOrderQueueLength());
		if(iTurns < MAX_INT) { // advc.004x
			szBuffer.append(gDLL->getText("TXT_KEY_UNIT_TURNS", iTurns));
				// advc.003: TXT_KEY_UNIT_TURNS takes only one argument
				//pCity->getProductionNeeded(eUnit), GC.getYieldInfo(YIELD_PRODUCTION).getChar()));
			szBuffer.append(L" - "); // advc.004x: Moved up
		}
		int iProduction = pCity->getUnitProduction(eUnit);
		if (iProduction > 0)
		{
			szTempBuffer.Format(L"%d/%d%c", iProduction,
					pCity->getProductionNeeded(eUnit),
					GC.getYieldInfo(YIELD_PRODUCTION).getChar());
			szBuffer.append(szTempBuffer);
		}
		else
		{
			szTempBuffer.Format(L"%d%c", pCity->getProductionNeeded(eUnit),
					GC.getYieldInfo(YIELD_PRODUCTION).getChar());
			szBuffer.append(szTempBuffer);
		}
	}
	// <advc.004w> BonusProductionModifier moved into subroutine
	if(!bCivilopediaText)
		setProductionSpeedHelp(szBuffer, ORDER_TRAIN, &u, pCity, false);
	// </advc.004w>
	// <advc.001b>
	if(pCity != NULL && pCity->plot() != NULL &&
			u.getAirUnitCap() > 0 && pCity->plot()->
			airUnitSpaceAvailable(TEAMID(pCity->getOwner())) < 1) {
		szTempBuffer.Format(L"%s%s", NEWLINE, gDLL->getText(
				"TXT_KEY_NOT_ENOUGH_SPACE").c_str());
		szBuffer.append(gDLL->getText("TXT_KEY_COLOR_NEGATIVE"));
		szBuffer.append(szTempBuffer);
		szBuffer.append(ENDCOLR);
	} // </advc.001b>

	if (bStrategyText)
	{
		if (!CvWString(u.getStrategy()).empty())
		{
			if ((ePlayer == NO_PLAYER) || GET_PLAYER(ePlayer).isOption(PLAYEROPTION_ADVISOR_HELP))
			{
				szBuffer.append(SEPARATOR);
				szBuffer.append(NEWLINE);
				szBuffer.append(gDLL->getText("TXT_KEY_SIDS_TIPS"));
				szBuffer.append(L'\"');
				szBuffer.append(u.getStrategy());
				szBuffer.append(L'\"');
			}
		}
	}

	if (bCivilopediaText)
	{
		if (eDefaultUnit == eUnit)
		{
			for (iI = 0; iI < GC.getNumUnitInfos(); ++iI)
			{
				if (iI != eUnit)
				{
					if (eUnitClass == GC.getUnitInfo((UnitTypes)iI).getUnitClassType())
					{
						szBuffer.append(NEWLINE);
						szBuffer.append(gDLL->getText("TXT_KEY_REPLACED_BY_UNIT", GC.getUnitInfo((UnitTypes)iI).getTextKeyWide()));
					}
				}
			}
		}
	}

	if (pCity != NULL)
	{
		if (//(gDLL->getChtLvl() > 0)
				GC.getGame().isDebugMode() // advc.135c
				&& GC.ctrlKey())
		{
			szBuffer.append(NEWLINE);
			for (int iUnitAI = 0; iUnitAI < NUM_UNITAI_TYPES; iUnitAI++)
			{
				int iTempValue = GET_PLAYER(pCity->getOwner()).AI_unitValue(eUnit, (UnitAITypes)iUnitAI, pCity->area());
				if (iTempValue != 0)
				{
					CvWString szTempString;
					getUnitAIString(szTempString, (UnitAITypes)iUnitAI);
					szBuffer.append(CvWString::format(L"(%s : %d) ", szTempString.GetCString(), iTempValue));
				}
			}
		}
	}
}

// BUG - Building Actual Effects - start
/*
 * Calls new function below without displaying actual effects.
 */
void CvGameTextMgr::setBuildingHelp(CvWStringBuffer &szBuffer, BuildingTypes eBuilding, bool bCivilopediaText, bool bStrategyText, bool bTechChooserText, CvCity* pCity)
{
	setBuildingHelpActual(szBuffer, eBuilding, bCivilopediaText, bStrategyText, bTechChooserText, pCity, false);
}

/*
 * Adds the actual effects of adding a building to the city.
 */
void CvGameTextMgr::setBuildingNetEffectsHelp(CvWStringBuffer &szBuffer, BuildingTypes eBuilding, CvCity* pCity)
{
	if (NULL != pCity)
	{
		bool bStarted = false;
		CvWString szStart;
		szStart.Format(L"\n"SETCOLR L"(%s", TEXT_COLOR("COLOR_LIGHT_GREY"), gDLL->getText("TXT_KEY_ACTUAL_EFFECTS").GetCString());

		// Defense
		int iDefense = pCity->getAdditionalDefenseByBuilding(eBuilding);
		bStarted = setResumableValueChangeHelp(szBuffer, szStart, L": ", L"", iDefense, gDLL->getSymbolID(DEFENSE_CHAR), true, false, bStarted);

		// Happiness
		int iGood = 0;
		int iBad = 0;
		pCity->getAdditionalHappinessByBuilding(eBuilding, iGood, iBad);
		//int iAngryPop = pCity->getAdditionalAngryPopuplation(iGood, iBad);
		bStarted = setResumableGoodBadChangeHelp(szBuffer, szStart, L": ", L"", iGood, gDLL->getSymbolID(HAPPY_CHAR), iBad, gDLL->getSymbolID(UNHAPPY_CHAR), false, false, bStarted);
		//bStarted = setResumableValueChangeHelp(szBuffer, szStart, L": ", L"", iAngryPop, gDLL->getSymbolID(ANGRY_POP_CHAR), false, bNewLine, bStarted);

		// Health
		iGood = 0;
		iBad = 0;
		pCity->getAdditionalHealthByBuilding(eBuilding, iGood, iBad);
		//int iSpoiledFood = pCity->getAdditionalSpoiledFood(iGood, iBad);
		//int iStarvation = pCity->getAdditionalStarvation(iSpoiledFood);
		bStarted = setResumableGoodBadChangeHelp(szBuffer, szStart, L": ", L"", iGood, gDLL->getSymbolID(HEALTHY_CHAR), iBad, gDLL->getSymbolID(UNHEALTHY_CHAR), false, false, bStarted);
		//bStarted = setResumableValueChangeHelp(szBuffer, szStart, L": ", L"", iSpoiledFood, gDLL->getSymbolID(EATEN_FOOD_CHAR), false, false, bStarted);
		//bStarted = setResumableValueChangeHelp(szBuffer, szStart, L": ", L"", iStarvation, gDLL->getSymbolID(BAD_FOOD_CHAR), false, false, bStarted);

		// Yield
		int aiYields[NUM_YIELD_TYPES];
		for (int iI = 0; iI < NUM_YIELD_TYPES; ++iI)
		{
			aiYields[iI] = pCity->getAdditionalYieldByBuilding((YieldTypes)iI, eBuilding);
		}
		bStarted = setResumableYieldChangeHelp(szBuffer, szStart, L": ", L"", aiYields, false, false, bStarted);

		// Commerce
		int aiCommerces[NUM_COMMERCE_TYPES];
		for (int iI = 0; iI < NUM_COMMERCE_TYPES; ++iI)
		{
			aiCommerces[iI] = pCity->getAdditionalCommerceTimes100ByBuilding((CommerceTypes)iI, eBuilding);
		}
		// Maintenance - add to gold
		aiCommerces[COMMERCE_GOLD] += pCity->getSavedMaintenanceTimes100ByBuilding(eBuilding);
		bStarted = setResumableCommerceTimes100ChangeHelp(szBuffer, szStart, L": ", L"", aiCommerces, false, bStarted);

		// Great People
		int iGreatPeopleRate = pCity->getAdditionalGreatPeopleRateByBuilding(eBuilding);
		bStarted = setResumableValueChangeHelp(szBuffer, szStart, L": ", L"", iGreatPeopleRate, gDLL->getSymbolID(GREAT_PEOPLE_CHAR), false, false, bStarted);

		if (bStarted)
			szBuffer.append(L")" ENDCOLR);
	}
}

/*
 * Adds option to display actual effects.
 */
//void CvGameTextMgr::setBuildingHelp(CvWStringBuffer &szBuffer, BuildingTypes eBuilding, bool bCivilopediaText, bool bStrategyText, bool bTechChooserText, CvCity* pCity)
void CvGameTextMgr::setBuildingHelpActual(CvWStringBuffer &szBuffer, BuildingTypes eBuilding, bool bCivilopediaText, bool bStrategyText, bool bTechChooserText, CvCity* pCity, bool bActual)
// BUG - Building Actual Effects - end
{
	PROFILE_FUNC();

	if(NO_BUILDING == eBuilding)
		return; // advc.003

	CvWString szFirstBuffer;
	CvWString szTempBuffer;

	// <advc.003>
	CvGame const& g = GC.getGame();
	CvBuildingInfo& kBuilding = GC.getBuildingInfo(eBuilding);
	PlayerTypes ePlayer = NO_PLAYER;
	if(pCity != NULL)
		ePlayer = pCity->getOwner();
	else ePlayer = g.getActivePlayer();
	BuildingClassTypes bct = (BuildingClassTypes)kBuilding.getBuildingClassType();
	if(bct == NO_BUILDINGCLASS)
		return;
	CvBuildingClassInfo const& bci = GC.getBuildingClassInfo(bct);
	/*  ePlayer is NO_PLAYER if Civilopedia accessed from main menu.
		(bCivilopediaText is true when help text for a Civilpedia article is being
		composed; false for Civilopedia hover text and all non-Civilopedia texts.) */
	CvPlayer const* pPlayer = (ePlayer == NO_PLAYER ? NULL : &GET_PLAYER(ePlayer));
	//if(!bCivilopediaText && ePlayer != NO_PLAYER) {
	if(!bCivilopediaText) { // </advc.003>
		szTempBuffer.Format( SETCOLR L"<link=literal>%s</link>" ENDCOLR ,
				TEXT_COLOR("COLOR_BUILDING_TEXT"), kBuilding.getDescription());
		szBuffer.append(szTempBuffer);
	}

	// <advc.004w>
	bool bInBuildingList = false;
	if(!bCivilopediaText && pCity != NULL &&
			pCity->getNumBuilding(eBuilding) >= GC.getCITY_MAX_NUM_BUILDINGS())
		bInBuildingList = true;
	bool bObsolete = true;
	if(pPlayer != NULL)
		bObsolete = TEAMREF(ePlayer).isObsoleteBuilding(eBuilding);
	CvWString szObsoleteWithTag = bObsolete ? L"TXT_KEY_BUILDING_OBSOLETE_WITH" :
			L"TXT_KEY_BUILDING_NOT_OBSOLETE";
	// </advc.004w>

	/* original bts code
	if (NULL != pCity)
		iHappiness = pCity->getBuildingHappiness(eBuilding);
	else iHappiness = kBuilding.getHappiness();*/
	/*  K-Mod, 30/dec/10, karadoc
		changed so that conditional happiness is not double-reported.
		(such as happiness from state-religion buildings, or culture slider) */
	int iHappiness = kBuilding.getHappiness();
	if (pCity)
	{
		// special modifiers (eg. events). These modifiers don't get their own line of text, so they need to be included here.
		iHappiness += pCity->getBuildingHappyChange(bct);
		iHappiness += pPlayer->getExtraBuildingHappiness(eBuilding);
		// 'Extra building happiness' includes happiness from several sources, including events, civics, traits, and boosts from other buildings.
		// My aim here is to only include in the total what isn't already in the list of bonuses below. As far as I know the only thing that would
		// be double-reported is the civic happiness. So I'll subtract that.
		for (int i = 0; i < GC.getNumCivicInfos(); i++)
		{
			if (pPlayer->isCivic((CivicTypes)i))
			{
				iHappiness -= GC.getCivicInfo((CivicTypes)i).getBuildingHappinessChanges(bct);
			}
		}
	} // K-Mod end

	if (iHappiness != 0)
	{
		szTempBuffer.Format(L", +%d%c", abs(iHappiness), ((iHappiness > 0) ? gDLL->getSymbolID(HAPPY_CHAR) : gDLL->getSymbolID(UNHAPPY_CHAR)));
		szBuffer.append(szTempBuffer);
	}

	/* original bts code
	if (NULL != pCity)
		iHealth = pCity->getBuildingGoodHealth(eBuilding);
	else {
		iHealth = kBuilding.getHealth();
		if (ePlayer != NO_PLAYER) {
			if (eBuilding == GC.getCivilizationInfo(GET_PLAYER(ePlayer).getCivilizationType()).getCivilizationBuildings(kBuilding.getBuildingClassType()))
				iHealth += GET_PLAYER(ePlayer).getExtraBuildingHealth(eBuilding);
		}
	}
	if (iHealth != 0) {
		szTempBuffer.Format(L", +%d%c", abs(iHealth), ((iHealth > 0) ? gDLL->getSymbolID(HEALTHY_CHAR): gDLL->getSymbolID(UNHEALTHY_CHAR)));
		szBuffer.append(szTempBuffer);
	}
	iHealth = 0;
	if (NULL != pCity)
		iHealth = pCity->getBuildingBadHealth(eBuilding);*/
	/*  K-Mod, 30/dec/10, karadoc
		changed so that conditional healthiness is not counted.
		(such as healthiness from public transport with environmentalism) */
	int iHealth = kBuilding.getHealth();
	if (pCity)
	{
		// special modifiers (eg. events). These modifiers don't get their own line of text, so they need to be included here.
		iHealth += pCity->getBuildingHealthChange(bct);
		iHealth += pPlayer->getExtraBuildingHealth(eBuilding);
		// We need to subtract any civic bonuses from 'extra building health', so as not to double-report. (see comments for the happiness section.)
		for (int i = 0; i < GC.getNumCivicInfos(); i++)
		{
			if (pPlayer->isCivic((CivicTypes)i))
			{
				iHealth -= GC.getCivicInfo((CivicTypes)i).getBuildingHealthChanges(bct);
			}
		}
	} // K-Mod end

	if (iHealth != 0)
	{
		szTempBuffer.Format(L", +%d%c", abs(iHealth), ((iHealth > 0) ? gDLL->getSymbolID(HEALTHY_CHAR): gDLL->getSymbolID(UNHEALTHY_CHAR)));
		szBuffer.append(szTempBuffer);
	}

	int aiYields[NUM_YIELD_TYPES];
	for (int iI = 0; iI < NUM_YIELD_TYPES; ++iI)
	{
		aiYields[iI] = kBuilding.getYieldChange(iI);

		if (NULL != pCity)
		{
			aiYields[iI] += pCity->getBuildingYieldChange(bct, (YieldTypes)iI);
		}
	}
	setYieldChangeHelp(szBuffer, L", ", L"", L"", aiYields, false, false);

	int aiCommerces[NUM_COMMERCE_TYPES];
	for (int iI = 0; iI < NUM_COMMERCE_TYPES; ++iI)
	{
		if ((NULL != pCity) && (pCity->getNumBuilding(eBuilding) > 0))
		{
			aiCommerces[iI] = pCity->getBuildingCommerceByBuilding((CommerceTypes)iI, eBuilding);
		}
		else
		{
			aiCommerces[iI] = kBuilding.getCommerceChange(iI);
			aiCommerces[iI] += kBuilding.getObsoleteSafeCommerceChange(iI);
			/*  K-Mod, 30/dec/10, karadoc
				added relgious building bonus info */
			if (ePlayer != NO_PLAYER &&
					kBuilding.getReligionType() != NO_RELIGION &&
					kBuilding.getReligionType() == pPlayer->getStateReligion())
				aiCommerces[iI] += pPlayer->getStateReligionBuildingCommerce((CommerceTypes)iI);
			// K-Mod end
		}
	}
	setCommerceChangeHelp(szBuffer, L", ", L"", L"", aiCommerces, false, false);
	setYieldChangeHelp(szBuffer, L", ", L"", L"",
			kBuilding.getYieldModifierArray(), true, bCivilopediaText);
	setCommerceChangeHelp(szBuffer, L", ", L"", L"",
			kBuilding.getCommerceModifierArray(), true, bCivilopediaText);

	if (kBuilding.getGreatPeopleRateChange() != 0)
	{
		szTempBuffer.Format(
				// advc.004w: Doesn't fit in one line
				(bInBuildingList ? L"\n%s%d%c" :
				L", %s%d%c"), ((kBuilding.getGreatPeopleRateChange() > 0) ? "+" : ""), kBuilding.getGreatPeopleRateChange(), gDLL->getSymbolID(GREAT_PEOPLE_CHAR));
		szBuffer.append(szTempBuffer);

		if (kBuilding.getGreatPeopleUnitClass() != NO_UNITCLASS)
		{
			UnitTypes eGreatPeopleUnit = NO_UNIT; // advc.003
			if (ePlayer != NO_PLAYER)
			{
				eGreatPeopleUnit = ((UnitTypes)(GC.getCivilizationInfo(pPlayer->getCivilizationType()).getCivilizationUnits(kBuilding.getGreatPeopleUnitClass())));
			}
			else
			{
				eGreatPeopleUnit = (UnitTypes)GC.getUnitClassInfo((UnitClassTypes)kBuilding.getGreatPeopleUnitClass()).getDefaultUnitIndex();
			}

			if (eGreatPeopleUnit != NO_UNIT)
			{
				szTempBuffer.Format(L" (%s)", GC.getUnitInfo(eGreatPeopleUnit).getDescription());
				szBuffer.append(szTempBuffer);
			}
		}
	}

	// test for unique building
	BuildingTypes eDefaultBuilding = (BuildingTypes)GC.getBuildingClassInfo(bct).getDefaultBuildingIndex();

	if (NO_BUILDING != eDefaultBuilding && eDefaultBuilding != eBuilding
			&& !bInBuildingList) // advc.004w
	{
		for (int iI  = 0; iI < GC.getNumCivilizationInfos(); ++iI)
		{
			BuildingTypes eUniqueBuilding = (BuildingTypes)GC.getCivilizationInfo((CivilizationTypes)iI).getCivilizationBuildings(bct);
			if (eUniqueBuilding == eBuilding)
			{
				szBuffer.append(NEWLINE);
				szBuffer.append(gDLL->getText("TXT_KEY_UNIQUE_BUILDING", GC.getCivilizationInfo((CivilizationTypes)iI).getTextKeyWide()));
			}
		}

		szBuffer.append(NEWLINE);
		szBuffer.append(gDLL->getText("TXT_KEY_REPLACES_UNIT", GC.getBuildingInfo(eDefaultBuilding).getTextKeyWide()));
	}

	if (bCivilopediaText
			|| ePlayer == NO_PLAYER) // advc.003: Civilopedia from main menu
	{	// advc.003: Already done higher up
		/*setYieldChangeHelp(szBuffer, L"", L"", L"", kBuilding.getYieldModifierArray(), true, bCivilopediaText);
		setCommerceChangeHelp(szBuffer, L"", L"", L"", kBuilding.getCommerceModifierArray(), true, bCivilopediaText);*/
	}
	else
	{	// advc.003: style changes in this branch
		// <advc.004w> Either already constructed in pCity, or queued.
		bool bConstruct = false;
		if(pCity != NULL) {
			if(bInBuildingList)
				bConstruct = true;
			else {
				for(CLLNode<OrderData>* pNode = pCity->headOrderQueueNode();
						pNode != NULL; pNode = pCity->nextOrderQueueNode(pNode)) {
					if(pNode->m_data.eOrderType != ORDER_CONSTRUCT)
						continue;
					if(pNode->m_data.iData1 == eBuilding) {
						bConstruct = true;
						break;
					}
				}
			}
		}
		if(isWorldWonderClass(bct)) {
			szBuffer.append(NEWLINE); // Newline in any case
			int iMaxGlobal = bci.getMaxGlobalInstances();
			if(pCity == NULL) {
				if(iMaxGlobal == 1)
					szBuffer.append(gDLL->getText("TXT_KEY_BUILDING_WORLD_WONDER1"));
				else szBuffer.append(gDLL->getText("TXT_KEY_BUILDING_WORLD_WONDER_ALLOWED",
						iMaxGlobal)); // </advc.004w>
			}
			else { // <advc.004w>
				iMaxGlobal -= (g.getBuildingClassCreatedCount(bct) +
						TEAMREF(ePlayer).getBuildingClassMaking(bct));
				if(iMaxGlobal == 1 || (iMaxGlobal == 0 && bConstruct))
					szBuffer.append(gDLL->getText("TXT_KEY_BUILDING_WORLD_WONDER1"));
				else {
					if(!bConstruct)
						szBuffer.append(gDLL->getText("TXT_KEY_COLOR_NEGATIVE"));
					szBuffer.append(gDLL->getText(
							"TXT_KEY_BUILDING_WORLD_WONDER_LEFT", iMaxGlobal));
					if(!bConstruct)
						szBuffer.append(gDLL->getText("TXT_KEY_COLOR_REVERT"));
				} // </advc.004w>
			}
		}
		if(isTeamWonderClass(bct)) { // <advc.004w>
			szBuffer.append(NEWLINE); // Newline in any case
			int iMaxTeam = bci.getMaxTeamInstances();
			if(pCity == NULL) {
				if(iMaxTeam == 1)
					szBuffer.append(gDLL->getText("TXT_KEY_BUILDING_TEAM_WONDER1"));
				else {
					if(!bConstruct)
						szBuffer.append(gDLL->getText("TXT_KEY_COLOR_NEGATIVE"));
					szBuffer.append(gDLL->getText(
							"TXT_KEY_BUILDING_TEAM_WONDER_ALLOWED", iMaxTeam));
					if(!bConstruct)
						szBuffer.append(gDLL->getText("TXT_KEY_COLOR_REVERT"));
				} // </advc.004w>
			}
			else { // <advc.004w>
				iMaxTeam -= TEAMREF(ePlayer).getBuildingClassCountPlusMaking(bct);
				if(iMaxTeam == 1)
					szBuffer.append(gDLL->getText("TXT_KEY_BUILDING_TEAM_WONDER1"));
				else {
					if(!bConstruct)
						szBuffer.append(gDLL->getText("TXT_KEY_COLOR_NEGATIVE"));
					szBuffer.append(gDLL->getText(
							"TXT_KEY_BUILDING_TEAM_WONDER_LEFT", iMaxTeam));
					if(!bConstruct)
						szBuffer.append(gDLL->getText("TXT_KEY_COLOR_REVERT"));
				} // </advc.004w>
			}
		}
		if(isNationalWonderClass(bct) &&
				// <advc.004w> Palace isn't really a National Wonder
				!kBuilding.isCapital() &&
				// That case is handled below; no need for "0 left" in addition.
				(pCity == NULL || !pCity->isNationalWondersMaxed() || bConstruct)) {
			szBuffer.append(NEWLINE); // Newline in any case
			int iMaxPlayer = bci.getMaxPlayerInstances();
			if(pCity == NULL) {
				if(iMaxPlayer == 1)
					szBuffer.append(gDLL->getText("TXT_KEY_BUILDING_NATIONAL_WONDER1"));
				else szBuffer.append(gDLL->getText("TXT_KEY_BUILDING_NATIONAL_WONDER_ALLOWED",
						iMaxPlayer)); // </advc.004w>
			}
			else { // <advc.004w>
				iMaxPlayer -= pPlayer->getBuildingClassCountPlusMaking(bct);
				/*  This would be the same behavior as for great wonders, but I
					want to show "0 left" only if the wonder is being built
					elsewhere; otherwise, show how many more national wonders
					the city can build. */
				/*if(iMaxPlayer == 1)
					szBuffer.append(gDLL->getText("TXT_KEY_BUILDING_NATIONAL_WONDER1"));
				else szBuffer.append(gDLL->getText("TXT_KEY_BUILDING_NATIONAL_WONDER_LEFT",
						iMaxPlayer)); */
				if(iMaxPlayer <= 0 && !bConstruct) {
					szBuffer.append(gDLL->getText("TXT_KEY_COLOR_NEGATIVE"));
					szBuffer.append(gDLL->getText(
							"TXT_KEY_BUILDING_NATIONAL_WONDER_LEFT", iMaxPlayer));
					szBuffer.append(gDLL->getText("TXT_KEY_COLOR_REVERT"));
				}
				else {
					iMaxPlayer = pCity->getNumNationalWondersLeft();
					if(iMaxPlayer < 0)
						szBuffer.append(gDLL->getText("TXT_KEY_BUILDING_NATIONAL_WONDER1"));
					else szBuffer.append(gDLL->getText(
							"TXT_KEY_BUILDING_NATIONAL_WONDER_LEFT", iMaxPlayer));
				} // </advc.004w>
			}
		}
	}

	if (kBuilding.getGlobalReligionCommerce() != NO_RELIGION)
	{
		szFirstBuffer = gDLL->getText("TXT_KEY_BUILDING_PER_CITY_WITH", GC.getReligionInfo((ReligionTypes) kBuilding.getGlobalReligionCommerce()).getChar());
		setCommerceChangeHelp(szBuffer, L"", L"", szFirstBuffer, GC.getReligionInfo((ReligionTypes) kBuilding.getGlobalReligionCommerce()).getGlobalReligionCommerceArray());
	}
	// <advc.179>
	if(!bCivilopediaText && ePlayer != NO_PLAYER)
		buildBuildingReligionYieldString(szBuffer, kBuilding); // </advc.179>
	if (NO_CORPORATION != kBuilding.getFoundsCorporation()
			&& !bInBuildingList) // advc.004w
	{
		szBuffer.append(NEWLINE);
		szBuffer.append(gDLL->getText("TXT_KEY_FOUNDS_CORPORATION", GC.getCorporationInfo((CorporationTypes)kBuilding.getFoundsCorporation()).getTextKeyWide()));
	}

	if (kBuilding.getGlobalCorporationCommerce() != NO_CORPORATION)
	{
		szFirstBuffer = gDLL->getText("TXT_KEY_BUILDING_PER_CITY_WITH", GC.getCorporationInfo((CorporationTypes) kBuilding.getGlobalCorporationCommerce()).getChar());
		setCommerceChangeHelp(szBuffer, L"", L"", szFirstBuffer, GC.getCorporationInfo((CorporationTypes) kBuilding.getGlobalCorporationCommerce()).getHeadquarterCommerceArray());
	}

	if (kBuilding.getNoBonus() != NO_BONUS)
	{
		CvBonusInfo& kBonus = GC.getBonusInfo((BonusTypes) kBuilding.getNoBonus());
		szBuffer.append(NEWLINE);
		szBuffer.append(gDLL->getText("TXT_KEY_BUILDING_DISABLES", kBonus.getTextKeyWide(), kBonus.getChar()));
	}

	if (kBuilding.getFreeBonus() != NO_BONUS)
	{
		szBuffer.append(NEWLINE);
		CvBonusInfo const& kFreeBonus = GC.getBonusInfo((BonusTypes)kBuilding.getFreeBonus());
		// <advc.004y>
		bool bRange = (ePlayer == NO_PLAYER);
		if(bRange) {
			int iLow = GC.getWorldInfo((WorldSizeTypes)
					0).getNumFreeBuildingBonuses();
			int iHigh = GC.getWorldInfo((WorldSizeTypes)
					(GC.getNumWorldInfos() - 1)).getNumFreeBuildingBonuses();
			if(iHigh == iLow)
				bRange = false;
			else {
				szBuffer.append(gDLL->getText("TXT_KEY_BUILDING_PROVIDES_RANGE",
						iLow, iHigh, kFreeBonus.getTextKeyWide(), kFreeBonus.getChar()));
			}
		}
		if(!bRange) { // </advc.004y>
			szBuffer.append(gDLL->getText("TXT_KEY_BUILDING_PROVIDES",
					g.getNumFreeBonuses(eBuilding), kFreeBonus.getTextKeyWide(),
					kFreeBonus.getChar()));
		}
		if (kFreeBonus.getHealth() != 0)
		{
			szTempBuffer.Format(L", +%d%c", abs(kFreeBonus.getHealth()),
					(kFreeBonus.getHealth() > 0 ? gDLL->getSymbolID(HEALTHY_CHAR) :
					gDLL->getSymbolID(UNHEALTHY_CHAR)));
			szBuffer.append(szTempBuffer);
		}

		if (kFreeBonus.getHappiness() != 0)
		{
			szTempBuffer.Format(L", +%d%c", abs(kFreeBonus.getHappiness()),
					(kFreeBonus.getHappiness() > 0 ? gDLL->getSymbolID(HAPPY_CHAR) :
					gDLL->getSymbolID(UNHAPPY_CHAR)));
			szBuffer.append(szTempBuffer);
		}
	}

	if (kBuilding.getFreeBuildingClass() != NO_BUILDINGCLASS)
	{
		BuildingTypes eFreeBuilding;
		if (ePlayer != NO_PLAYER)
		{
			eFreeBuilding = ((BuildingTypes)(GC.getCivilizationInfo(pPlayer->getCivilizationType()).getCivilizationBuildings(kBuilding.getFreeBuildingClass())));
		}
		else
		{
			eFreeBuilding = (BuildingTypes)GC.getBuildingClassInfo((BuildingClassTypes)kBuilding.getFreeBuildingClass()).getDefaultBuildingIndex();
		}

		if (NO_BUILDING != eFreeBuilding)
		{
			szBuffer.append(NEWLINE);
			szBuffer.append(gDLL->getText("TXT_KEY_BUILDING_FREE_IN_CITY", GC.getBuildingInfo(eFreeBuilding).getTextKeyWide()));
		}
	}

	if (kBuilding.getFreePromotion() != NO_PROMOTION)
	{
		szBuffer.append(NEWLINE);
		szBuffer.append(gDLL->getText("TXT_KEY_BUILDING_FREE_PROMOTION", GC.getPromotionInfo((PromotionTypes)(kBuilding.getFreePromotion())).getTextKeyWide()));
	}

	if (kBuilding.getCivicOption() != NO_CIVICOPTION)
	{
		szBuffer.append(NEWLINE);
		szBuffer.append(gDLL->getText("TXT_KEY_BUILDING_ENABLES_CIVICS", GC.getCivicOptionInfo((CivicOptionTypes)(kBuilding.getCivicOption())).getTextKeyWide()));
	}

	if (kBuilding.isPower())
	{
		szBuffer.append(NEWLINE);
		szBuffer.append(gDLL->getText("TXT_KEY_BUILDING_PROVIDES_POWER"));

		/* original bts code
		if (kBuilding.isDirtyPower() && (GC.getDefineINT("DIRTY_POWER_HEALTH_CHANGE") != 0))
		{
			szTempBuffer.Format(L" (+%d%c)", abs(GC.getDefineINT("DIRTY_POWER_HEALTH_CHANGE")), ((GC.getDefineINT("DIRTY_POWER_HEALTH_CHANGE") > 0) ? gDLL->getSymbolID(HEALTHY_CHAR): gDLL->getSymbolID(UNHEALTHY_CHAR)));
			szBuffer.append(szTempBuffer);
		} */
		// K-Mod. Also include base health change from power.
		int iPowerHealth = GC.getDefineINT("POWER_HEALTH_CHANGE") + (kBuilding.isDirtyPower() ? GC.getDefineINT("DIRTY_POWER_HEALTH_CHANGE") : 0);
		if (iPowerHealth)
		{
			szTempBuffer.Format(L" (+%d%c)", abs(iPowerHealth), iPowerHealth > 0 ? gDLL->getSymbolID(HEALTHY_CHAR): gDLL->getSymbolID(UNHEALTHY_CHAR));
			szBuffer.append(szTempBuffer);
		}
		// K-Mod end
	}

	if (kBuilding.isAreaCleanPower())
	{
		szBuffer.append(NEWLINE);
		szBuffer.append(gDLL->getText("TXT_KEY_BUILDING_PROVIDES_AREA_CLEAN_POWER"));
	}

	if (kBuilding.isAreaBorderObstacle())
	{
		szBuffer.append(NEWLINE);
		szBuffer.append(gDLL->getText("TXT_KEY_BUILDING_BORDER_OBSTACLE"));
	}

	// (advc.179: Triggered Election, Eligibility text moved down)

	if (kBuilding.isCapital())
	{
		szBuffer.append(NEWLINE);
		szBuffer.append(gDLL->getText("TXT_KEY_BUILDING_CAPITAL"));
	}

	if (kBuilding.isGovernmentCenter())
	{
		szBuffer.append(NEWLINE);
		szBuffer.append(gDLL->getText("TXT_KEY_BUILDING_REDUCES_MAINTENANCE"));
	}

	if (kBuilding.isGoldenAge() /* advc.004w: */ && !bInBuildingList)
	{
		szBuffer.append(NEWLINE);
		szBuffer.append(gDLL->getText("TXT_KEY_BUILDING_GOLDEN_AGE"));
	}

	if (kBuilding.isAllowsNukes())
	{
		szBuffer.append(NEWLINE);
		szBuffer.append(gDLL->getText("TXT_KEY_PROJECT_ENABLES_NUKES"));
	}

	if (kBuilding.isMapCentering() /* advc.004w: */ && !bInBuildingList)
	{
		szBuffer.append(NEWLINE);
		szBuffer.append(gDLL->getText("TXT_KEY_BUILDING_CENTERS_MAP"));
	}

	if (kBuilding.isNoUnhappiness())
	{
		szBuffer.append(NEWLINE);
		szBuffer.append(gDLL->getText("TXT_KEY_BUILDING_NO_UNHAPPY"));
	}
	/* original bts code
	if (kBuilding.isNoUnhealthyPopulation()) {
		szBuffer.append(NEWLINE);
		szBuffer.append(gDLL->getText("TXT_KEY_BUILDING_NO_UNHEALTHY_POP"));
	}*/
	/*  K-Mod, 27/dec/10, karadoc
		replaced NoUnhealthyPopulation with UnhealthyPopulationModifier */
	if (kBuilding.getUnhealthyPopulationModifier() != 0)
	{
		// If the modifier is less than -100, display the old NoUnhealth. text
		// Note: this could be techinically inaccurate if we combine this modifier with a positive modifier
		if (kBuilding.getUnhealthyPopulationModifier() <= -100)
		{
			szBuffer.append(NEWLINE);
			szBuffer.append(gDLL->getText("TXT_KEY_CIVIC_NO_POP_UNHEALTHY"));
		}
		else
		{
			szBuffer.append(NEWLINE);
			szBuffer.append(gDLL->getText("TXT_KEY_UNHEALTHY_POP_MODIFIER", kBuilding.getUnhealthyPopulationModifier()));
		}
	} // K-Mod end

	if (kBuilding.isBuildingOnlyHealthy())
	{
		szBuffer.append(NEWLINE);
		szBuffer.append(gDLL->getText("TXT_KEY_BUILDING_NO_UNHEALTHY_BUILDINGS"));
	}

	if (kBuilding.getGreatPeopleRateModifier() != 0)
	{
		szBuffer.append(NEWLINE);
		szBuffer.append(gDLL->getText("TXT_KEY_BUILDING_BIRTH_RATE_MOD", kBuilding.getGreatPeopleRateModifier()));
	}

	if (kBuilding.getGreatGeneralRateModifier() != 0)
	{
		szBuffer.append(NEWLINE);
		szBuffer.append(gDLL->getText("TXT_KEY_BUILDING_GENERAL_RATE_MOD", kBuilding.getGreatGeneralRateModifier()));
	}

	if (kBuilding.getDomesticGreatGeneralRateModifier() != 0)
	{
		szBuffer.append(NEWLINE);
		szBuffer.append(gDLL->getText("TXT_KEY_DOMESTIC_GREAT_GENERAL_MODIFIER", kBuilding.getDomesticGreatGeneralRateModifier()));
	}

	if (kBuilding.getGlobalGreatPeopleRateModifier() != 0)
	{
		szBuffer.append(NEWLINE);
		szBuffer.append(gDLL->getText("TXT_KEY_BUILDING_BIRTH_RATE_MOD_ALL_CITIES", kBuilding.getGlobalGreatPeopleRateModifier()));
	}

	if (kBuilding.getAnarchyModifier() != 0)
	{
		if (-100 == kBuilding.getAnarchyModifier())
		{
			szBuffer.append(NEWLINE);
			szBuffer.append(gDLL->getText("TXT_KEY_BUILDING_NO_ANARCHY"));
		}
		else
		{
			szBuffer.append(NEWLINE);
			szBuffer.append(gDLL->getText("TXT_KEY_BUILDING_ANARCHY_MOD", kBuilding.getAnarchyModifier()));

			szBuffer.append(NEWLINE);
			szBuffer.append(gDLL->getText("TXT_KEY_BUILDING_ANARCHY_TIMER_MOD", kBuilding.getAnarchyModifier()));
		}
	}

	if (kBuilding.getGoldenAgeModifier() != 0)
	{
		szBuffer.append(NEWLINE);
		szBuffer.append(gDLL->getText("TXT_KEY_BUILDING_GOLDENAGE_MOD", kBuilding.getGoldenAgeModifier()));
	}

	if (kBuilding.getGlobalHurryModifier() != 0)
	{
		szBuffer.append(NEWLINE);
		szBuffer.append(gDLL->getText("TXT_KEY_BUILDING_HURRY_MOD", kBuilding.getGlobalHurryModifier()));
	}

	if (kBuilding.getFreeExperience() != 0)
	{
		szBuffer.append(NEWLINE);
		szBuffer.append(gDLL->getText("TXT_KEY_BUILDING_FREE_XP_UNITS", kBuilding.getFreeExperience()));
	}

	if (kBuilding.getGlobalFreeExperience() != 0)
	{
		szBuffer.append(NEWLINE);
		szBuffer.append(gDLL->getText("TXT_KEY_BUILDING_FREE_XP_ALL_CITIES", kBuilding.getGlobalFreeExperience()));
	}

	if (kBuilding.getFoodKept() > 0)
	{
		szBuffer.append(NEWLINE);
		szBuffer.append(gDLL->getText("TXT_KEY_BUILDING_STORES_FOOD", kBuilding.getFoodKept()));
	}

	if (kBuilding.getAirlift() > 0)
	{
		szBuffer.append(NEWLINE);
		szBuffer.append(gDLL->getText("TXT_KEY_BUILDING_AIRLIFT", kBuilding.getAirlift()));
	}

	if (kBuilding.getAirModifier() != 0)
	{
		szBuffer.append(NEWLINE);
		szBuffer.append(gDLL->getText("TXT_KEY_BUILDING_AIR_DAMAGE_MOD", kBuilding.getAirModifier()));
	}

	if (kBuilding.getAirUnitCapacity() != 0)
	{
		szBuffer.append(NEWLINE);
		szBuffer.append(gDLL->getText("TXT_KEY_BUILDING_AIR_UNIT_CAPACITY", kBuilding.getAirUnitCapacity()));
	}

	if (kBuilding.getNukeModifier() != 0)
	{
		szBuffer.append(NEWLINE);
		szBuffer.append(gDLL->getText("TXT_KEY_BUILDING_NUKE_DAMAGE_MOD", kBuilding.getNukeModifier()));
	}

	if (kBuilding.getNukeExplosionRand() != 0)
	{
		szBuffer.append(NEWLINE);
		szBuffer.append(gDLL->getText("TXT_KEY_BUILDING_NUKE_EXPLOSION_CHANCE"));
	}

	if (kBuilding.getFreeSpecialist() != 0)
	{
		szBuffer.append(NEWLINE);
		szBuffer.append(gDLL->getText("TXT_KEY_BUILDING_FREE_SPECIALISTS", kBuilding.getFreeSpecialist()));
	}

	if (kBuilding.getAreaFreeSpecialist() != 0)
	{
		szBuffer.append(NEWLINE);
		szBuffer.append(gDLL->getText("TXT_KEY_BUILDING_FREE_SPECIALISTS_CONT", kBuilding.getAreaFreeSpecialist()));
	}

	if (kBuilding.getGlobalFreeSpecialist() != 0)
	{
		szBuffer.append(NEWLINE);
		szBuffer.append(gDLL->getText("TXT_KEY_BUILDING_FREE_SPECIALISTS_ALL_CITIES", kBuilding.getGlobalFreeSpecialist()));
	}

	if (kBuilding.getMaintenanceModifier() != 0)
	{
		szBuffer.append(NEWLINE);
		szBuffer.append(gDLL->getText("TXT_KEY_BUILDING_MAINT_MOD", kBuilding.getMaintenanceModifier()));
	}

	if (kBuilding.getHurryAngerModifier() != 0)
	{
		szBuffer.append(NEWLINE);
		szBuffer.append(gDLL->getText("TXT_KEY_BUILDING_HURRY_ANGER_MOD", kBuilding.getHurryAngerModifier()));
		// <advc.912d>
		if(ePlayer != NO_PLAYER && g.isOption(GAMEOPTION_NO_SLAVERY) &&
				kBuilding.getHurryAngerModifier() < 0 &&
				GET_PLAYER(ePlayer).isHuman()) {
			szBuffer.append(NEWLINE);
			szTempBuffer.Format(L"%c", gDLL->getSymbolID(BULLET_CHAR));
			szBuffer.append(szTempBuffer);
			szBuffer.append(gDLL->getText("TXT_KEY_HURRY_POPULATION"));
		} // </advc.912d>
	}

	if (kBuilding.getWarWearinessModifier() != 0)
	{
		szBuffer.append(NEWLINE);
		szBuffer.append(gDLL->getText("TXT_KEY_BUILDING_WAR_WEAR_MOD", kBuilding.getWarWearinessModifier()));
	}

	if (kBuilding.getGlobalWarWearinessModifier() != 0)
	{
		szBuffer.append(NEWLINE);
		szBuffer.append(gDLL->getText("TXT_KEY_BUILDING_WAR_WEAR_MOD_ALL_CITIES", kBuilding.getGlobalWarWearinessModifier()));
	}

	if (kBuilding.getEnemyWarWearinessModifier() != 0)
	{
		szBuffer.append(NEWLINE);
		szBuffer.append(gDLL->getText("TXT_KEY_BUILDING_ENEMY_WAR_WEAR", kBuilding.getEnemyWarWearinessModifier()));
	}

	if (kBuilding.getHealRateChange() != 0)
	{
		szBuffer.append(NEWLINE);
		szBuffer.append(gDLL->getText("TXT_KEY_BUILDING_HEAL_MOD", kBuilding.getHealRateChange()));
	}

	if (kBuilding.getAreaHealth() != 0)
	{
		szBuffer.append(NEWLINE);
		szBuffer.append(gDLL->getText("TXT_KEY_BUILDING_HEALTH_CHANGE_CONT", abs(kBuilding.getAreaHealth()), ((kBuilding.getAreaHealth() > 0) ? gDLL->getSymbolID(HEALTHY_CHAR): gDLL->getSymbolID(UNHEALTHY_CHAR))));
	}

	if (kBuilding.getGlobalHealth() != 0)
	{
		szBuffer.append(NEWLINE);
		szBuffer.append(gDLL->getText("TXT_KEY_BUILDING_HEALTH_CHANGE_ALL_CITIES", abs(kBuilding.getGlobalHealth()), ((kBuilding.getGlobalHealth() > 0) ? gDLL->getSymbolID(HEALTHY_CHAR): gDLL->getSymbolID(UNHEALTHY_CHAR))));
	}

	if (kBuilding.getAreaHappiness() != 0)
	{
		szBuffer.append(NEWLINE);
		szBuffer.append(gDLL->getText("TXT_KEY_BUILDING_HAPPY_CHANGE_CONT",
				// UNOFFICIAL_PATCH, Bugfix, 08/28/09, jdog5000: Use absolute value with unhappy face
				abs(kBuilding.getAreaHappiness()), ((kBuilding.getAreaHappiness() > 0) ? gDLL->getSymbolID(HAPPY_CHAR) : gDLL->getSymbolID(UNHAPPY_CHAR))));
	}

	if (kBuilding.getGlobalHappiness() != 0)
	{
		szBuffer.append(NEWLINE);
		szBuffer.append(gDLL->getText("TXT_KEY_BUILDING_HAPPY_CHANGE_ALL_CITIES",
				// UNOFFICIAL_PATCH, Bugfix, 08/28/09, jdog5000: Use absolute value with unhappy face
				abs(kBuilding.getGlobalHappiness()), ((kBuilding.getGlobalHappiness() > 0) ? gDLL->getSymbolID(HAPPY_CHAR) : gDLL->getSymbolID(UNHAPPY_CHAR))));
	}

	if (kBuilding.getStateReligionHappiness() != 0)
	{
		if (kBuilding.getReligionType() != NO_RELIGION)
		{
			szBuffer.append(NEWLINE);
			szBuffer.append(gDLL->getText("TXT_KEY_BUILDING_RELIGION_HAPPINESS",
					// UNOFFICIAL_PATCH, Bugfix, 08/28/09, jdog5000: Use absolute value with unhappy face
					abs(kBuilding.getStateReligionHappiness()), ((kBuilding.getStateReligionHappiness() > 0) ? gDLL->getSymbolID(HAPPY_CHAR) : gDLL->getSymbolID(UNHAPPY_CHAR)), GC.getReligionInfo((ReligionTypes)(kBuilding.getReligionType())).getChar()));
		}
	}

	if (kBuilding.getWorkerSpeedModifier() != 0)
	{
		szBuffer.append(NEWLINE);
		szBuffer.append(gDLL->getText("TXT_KEY_BUILDING_WORKER_MOD", kBuilding.getWorkerSpeedModifier()));
	}

	if (kBuilding.getMilitaryProductionModifier() != 0)
	{
		szBuffer.append(NEWLINE);
		szBuffer.append(gDLL->getText("TXT_KEY_BUILDING_MILITARY_MOD", kBuilding.getMilitaryProductionModifier()));
	}

	if (kBuilding.getSpaceProductionModifier() != 0)
	{
		szBuffer.append(NEWLINE);
		szBuffer.append(gDLL->getText("TXT_KEY_BUILDING_SPACESHIP_MOD", kBuilding.getSpaceProductionModifier()));
	}

	if (kBuilding.getGlobalSpaceProductionModifier() != 0)
	{
		szBuffer.append(NEWLINE);
		szBuffer.append(gDLL->getText("TXT_KEY_BUILDING_SPACESHIP_MOD_ALL_CITIES", kBuilding.getGlobalSpaceProductionModifier()));
	}

	if (kBuilding.getTradeRoutes() != 0)
	{
		szBuffer.append(NEWLINE);
		szBuffer.append(gDLL->getText("TXT_KEY_BUILDING_TRADE_ROUTES", kBuilding.getTradeRoutes()));
	}

	if (kBuilding.getCoastalTradeRoutes() != 0)
	{
		szBuffer.append(NEWLINE);
		szBuffer.append(gDLL->getText("TXT_KEY_BUILDING_COASTAL_TRADE_ROUTES", kBuilding.getCoastalTradeRoutes()));
	}

	if (kBuilding.getAreaTradeRoutes() != 0) // advc.310: Renamed; was iGlobalTradeRoutes.
	{
		szBuffer.append(NEWLINE);
		szBuffer.append(gDLL->getText("TXT_KEY_BUILDING_TRADE_ROUTES_ALL_CITIES",
				kBuilding.getAreaTradeRoutes())); // advc.310
	}

	if (kBuilding.getTradeRouteModifier() != 0)
	{
		szBuffer.append(NEWLINE);
		szBuffer.append(gDLL->getText("TXT_KEY_BUILDING_TRADE_ROUTE_MOD", kBuilding.getTradeRouteModifier()));
	}

	if (kBuilding.getForeignTradeRouteModifier() != 0)
	{
		szBuffer.append(NEWLINE);
		szBuffer.append(gDLL->getText("TXT_KEY_BUILDING_FOREIGN_TRADE_ROUTE_MOD", kBuilding.getForeignTradeRouteModifier()));
	}

	if (kBuilding.getGlobalPopulationChange() != 0)
	{
		szBuffer.append(NEWLINE);
		szBuffer.append(gDLL->getText("TXT_KEY_BUILDING_GLOBAL_POP", kBuilding.getGlobalPopulationChange()));
	}

	if (kBuilding.getFreeTechs() != 0)
	{
		if (kBuilding.getFreeTechs() == 1)
		{
			szBuffer.append(NEWLINE);
			szBuffer.append(gDLL->getText("TXT_KEY_BUILDING_FREE_TECH"));
		}
		else
		{
			szBuffer.append(NEWLINE);
			szBuffer.append(gDLL->getText("TXT_KEY_BUILDING_FREE_TECHS", kBuilding.getFreeTechs()));
		}
	}

	if (kBuilding.getDefenseModifier() != 0)
	{
		szBuffer.append(NEWLINE);
		szBuffer.append(gDLL->getText("TXT_KEY_BUILDING_DEFENSE_MOD", kBuilding.getDefenseModifier()));
	}

	if (kBuilding.getBombardDefenseModifier() != 0)
	{
		szBuffer.append(NEWLINE);
		szBuffer.append(gDLL->getText("TXT_KEY_BUILDING_BOMBARD_DEFENSE_MOD", -kBuilding.getBombardDefenseModifier()));
	}

	if (kBuilding.getAllCityDefenseModifier() != 0)
	{
		szBuffer.append(NEWLINE);
		szBuffer.append(gDLL->getText("TXT_KEY_BUILDING_DEFENSE_MOD_ALL_CITIES", kBuilding.getAllCityDefenseModifier()));
	}

	if (kBuilding.getEspionageDefenseModifier() != 0)
	{
		szBuffer.append(NEWLINE);
		szBuffer.append(gDLL->getText("TXT_KEY_BUILDING_ESPIONAGE_DEFENSE_MOD", kBuilding.getEspionageDefenseModifier()));
		// UNOFFICIAL_PATCH, Bugfix, 12/07/09, Afforess & jdog5000: guard added
		if (kBuilding.getEspionageDefenseModifier() > 0)
		{
			szBuffer.append(NEWLINE);
			szBuffer.append(gDLL->getText("TXT_KEY_UNIT_EXPOSE_SPIES"));
		}
	}

	setYieldChangeHelp(szBuffer, gDLL->getText("TXT_KEY_BUILDING_WATER_PLOTS").c_str(), L": ", L"", kBuilding.getSeaPlotYieldChangeArray());

	setYieldChangeHelp(szBuffer, gDLL->getText("TXT_KEY_BUILDING_RIVER_PLOTS").c_str(), L": ", L"", kBuilding.getRiverPlotYieldChangeArray());

	setYieldChangeHelp(szBuffer, gDLL->getText("TXT_KEY_BUILDING_WATER_PLOTS_ALL_CITIES").c_str(), L": ", L"", kBuilding.getGlobalSeaPlotYieldChangeArray());

	setYieldChangeHelp(szBuffer, L"", L"", gDLL->getText("TXT_KEY_BUILDING_WITH_POWER").c_str(), kBuilding.getPowerYieldModifierArray(), true);

	setYieldChangeHelp(szBuffer, L"", L"", gDLL->getText("TXT_KEY_BUILDING_ALL_CITIES_THIS_CONTINENT").c_str(), kBuilding.getAreaYieldModifierArray(), true);

	setYieldChangeHelp(szBuffer, L"", L"", gDLL->getText("TXT_KEY_BUILDING_ALL_CITIES").c_str(), kBuilding.getGlobalYieldModifierArray(), true);

	setCommerceChangeHelp(szBuffer, L"", L"", gDLL->getText("TXT_KEY_BUILDING_ALL_CITIES").c_str(), kBuilding.getGlobalCommerceModifierArray(), true);

	setCommerceChangeHelp(szBuffer, L"", L"", gDLL->getText("TXT_KEY_BUILDING_PER_SPECIALIST_ALL_CITIES").c_str(), kBuilding.getSpecialistExtraCommerceArray());

	/* original bts code
	if (ePlayer != NO_PLAYER && GET_PLAYER(ePlayer).getStateReligion() != NO_RELIGION)
		szTempBuffer = gDLL->getText("TXT_KEY_BUILDING_FROM_ALL_REL_BUILDINGS", GC.getReligionInfo(GET_PLAYER(ePlayer).getStateReligion()).getChar());
	else
		szTempBuffer = gDLL->getText("TXT_KEY_BUILDING_STATE_REL_BUILDINGS");*/
	/*  K-Mod, 30/dec/10, karadoc
		Changed to always say state religion, rather than the particular religion that happens to be the current state religion. */
	szTempBuffer = gDLL->getText("TXT_KEY_BUILDING_STATE_REL_BUILDINGS");

	setCommerceChangeHelp(szBuffer, L"", L"", szTempBuffer, kBuilding.getStateReligionCommerceArray());

	for (iI = 0; iI < NUM_COMMERCE_TYPES; ++iI)
	{
		if (kBuilding.getCommerceHappiness(iI) != 0)
		{
			szBuffer.append(NEWLINE);
			szBuffer.append(gDLL->getText("TXT_KEY_BUILDING_PER_LEVEL",
					// UNOFFICIAL_PATCH, Bugfix, 08/28/09, jdog5000: Use absolute value with unhappy face
					((kBuilding.getCommerceHappiness(iI) > 0) ? gDLL->getSymbolID(HAPPY_CHAR) : gDLL->getSymbolID(UNHAPPY_CHAR)),
					abs(100 / kBuilding.getCommerceHappiness(iI)), GC.getCommerceInfo((CommerceTypes)iI).getChar()));
		}

		if (kBuilding.isCommerceFlexible(iI) != 0)
		{
			szBuffer.append(NEWLINE);
			szBuffer.append(gDLL->getText("TXT_KEY_BUILDING_ADJUST_COMM_RATE", GC.getCommerceInfo((CommerceTypes) iI).getChar()));
		}
	}
	// UNOFFICIAL_PATCH, Efficiency, 06/27/10, Afforess & jdog5000: guard added
	if (kBuilding.isAnySpecialistYieldChange())
	{
		for (iI = 0; iI < GC.getNumSpecialistInfos(); ++iI)
		{
			szFirstBuffer = gDLL->getText("TXT_KEY_BUILDING_FROM_IN_ALL_CITIES", GC.getSpecialistInfo((SpecialistTypes) iI).getTextKeyWide());
			setYieldChangeHelp(szBuffer, L"", L"", szFirstBuffer, kBuilding.getSpecialistYieldChangeArray(iI));
		}
	}
	// UNOFFICIAL_PATCH, Efficiency, 06/27/10, Afforess & jdog5000: guard added
	if (kBuilding.isAnyBonusYieldModifier())
	{
		for (iI = 0; iI < GC.getNumBonusInfos(); ++iI)
		{
			szFirstBuffer = gDLL->getText("TXT_KEY_BUILDING_WITH_BONUS", GC.getBonusInfo((BonusTypes) iI).getTextKeyWide());
			setYieldChangeHelp(szBuffer, L"", L"", szFirstBuffer, kBuilding.getBonusYieldModifierArray(iI), true);
		}
	}

	for (iI = 0; iI < GC.getNumReligionInfos(); ++iI)
	{
		if (kBuilding.getReligionChange(iI) > 0)
		{
			szTempBuffer.Format(L"%s%s", NEWLINE, gDLL->getText("TXT_KEY_BUILDING_SPREADS_RELIGION", GC.getReligionInfo((ReligionTypes) iI).getChar()).c_str());
			szBuffer.append(szTempBuffer);
		}
	}

	for (iI = 0; iI < GC.getNumSpecialistInfos(); ++iI)
	{
		if (kBuilding.getSpecialistCount(iI) > 0)
		{
			if (kBuilding.getSpecialistCount(iI) == 1)
			{
				szBuffer.append(NEWLINE);
				szBuffer.append(gDLL->getText("TXT_KEY_BUILDING_TURN_CITIZEN_INTO", GC.getSpecialistInfo((SpecialistTypes) iI).getTextKeyWide()));
			}
			else
			{
				szBuffer.append(NEWLINE);
				szBuffer.append(gDLL->getText("TXT_KEY_BUILDING_TURN_CITIZENS_INTO", kBuilding.getSpecialistCount(iI), GC.getSpecialistInfo((SpecialistTypes) iI).getTextKeyWide()));
			}
		}

		if (kBuilding.getFreeSpecialistCount(iI) > 0)
		{
			szBuffer.append(NEWLINE);
			szBuffer.append(gDLL->getText("TXT_KEY_BUILDING_FREE_SPECIALIST", kBuilding.getFreeSpecialistCount(iI), GC.getSpecialistInfo((SpecialistTypes) iI).getTextKeyWide()));
		}
	}

	int iLast = 0;

	for (iI = 0; iI < GC.getNumImprovementInfos(); ++iI)
	{
		if (kBuilding.getImprovementFreeSpecialist(iI) > 0)
		{
			szFirstBuffer.Format(L"%s%s", NEWLINE, gDLL->getText("TXT_KEY_BUILDING_IMPROVEMENT_FREE_SPECIALISTS", kBuilding.getImprovementFreeSpecialist(iI)).GetCString());
			szTempBuffer.Format(L"<link=literal>%s</link>", GC.getImprovementInfo((ImprovementTypes)iI).getDescription());
			setListHelp(szBuffer, szFirstBuffer, szTempBuffer, L", ", (kBuilding.getImprovementFreeSpecialist(iI) != iLast));
			iLast = kBuilding.getImprovementFreeSpecialist(iI);
		}
	}

	iLast = 0;

	for (iI = 0; iI < GC.getNumBonusInfos(); ++iI)
	{
		if (kBuilding.getBonusHealthChanges(iI) != 0)
		{
			szFirstBuffer.Format(L"%s%s", NEWLINE, gDLL->getText("TXT_KEY_BUILDING_HEALTH_HAPPINESS_CHANGE", abs(kBuilding.getBonusHealthChanges(iI)), ((kBuilding.getBonusHealthChanges(iI) > 0) ? gDLL->getSymbolID(HEALTHY_CHAR): gDLL->getSymbolID(UNHEALTHY_CHAR))).c_str());
			szTempBuffer.Format(L"<link=literal>%s</link>", GC.getBonusInfo((BonusTypes)iI).getDescription());
			setListHelp(szBuffer, szFirstBuffer, szTempBuffer, L", ", (kBuilding.getBonusHealthChanges(iI) != iLast));
			iLast = kBuilding.getBonusHealthChanges(iI);
		}
	}

	iLast = 0;

	for (iI = 0; iI < GC.getNumCivicInfos(); ++iI)
	{
		int iChange = GC.getCivicInfo((CivicTypes)iI).getBuildingHealthChanges(bct);
		if (iChange != 0)
		{
			szFirstBuffer.Format(L"%s%s", NEWLINE, gDLL->getText("TXT_KEY_BUILDING_CIVIC_HEALTH_HAPPINESS_CHANGE", abs(iChange), ((iChange > 0) ? gDLL->getSymbolID(HEALTHY_CHAR): gDLL->getSymbolID(UNHEALTHY_CHAR))).c_str());
			szTempBuffer.Format(L"<link=literal>%s</link>", GC.getCivicInfo((CivicTypes)iI).getDescription());
			setListHelp(szBuffer, szFirstBuffer, szTempBuffer, L", ", (iChange != iLast));
			iLast = iChange;
		}
	}

	iLast = 0;

	for (iI = 0; iI < GC.getNumBonusInfos(); ++iI)
	{
		if (kBuilding.getBonusHappinessChanges(iI) != 0)
		{
			szFirstBuffer.Format(L"%s%s", NEWLINE, gDLL->getText("TXT_KEY_BUILDING_HEALTH_HAPPINESS_CHANGE", abs(kBuilding.getBonusHappinessChanges(iI)), ((kBuilding.getBonusHappinessChanges(iI) > 0) ? gDLL->getSymbolID(HAPPY_CHAR) : gDLL->getSymbolID(UNHAPPY_CHAR))).c_str());
			szTempBuffer.Format(L"<link=literal>%s</link>", GC.getBonusInfo((BonusTypes)iI).getDescription());
			setListHelp(szBuffer, szFirstBuffer, szTempBuffer, L", ", (kBuilding.getBonusHappinessChanges(iI) != iLast));
			iLast = kBuilding.getBonusHappinessChanges(iI);
		}
	}
	/*  <advc.004w> Moved this block up so that the (guaranteed) XP from Barracks
		appears above the highly situational happiness from civics. */
	for (iI = 0; iI < GC.getNumUnitCombatInfos(); ++iI)
	{
		if (kBuilding.getUnitCombatFreeExperience(iI) != 0)
		{
			szBuffer.append(NEWLINE);
			szBuffer.append(gDLL->getText("TXT_KEY_BUILDING_FREE_XP", GC.getUnitCombatInfo((UnitCombatTypes)iI).getTextKeyWide(), kBuilding.getUnitCombatFreeExperience(iI)));
		}
	}
	for (iI = 0; iI < NUM_DOMAIN_TYPES; ++iI)
	{
		if (kBuilding.getDomainFreeExperience(iI) != 0)
		{
			szBuffer.append(NEWLINE);
			szBuffer.append(gDLL->getText("TXT_KEY_BUILDING_FREE_XP", GC.getDomainInfo((DomainTypes)iI).getTextKeyWide(), kBuilding.getDomainFreeExperience(iI)));
		}
	} // </advc.004w>

	iLast = 0;
	for (iI = 0; iI < GC.getNumCivicInfos(); ++iI)
	{
		int iChange = GC.getCivicInfo((CivicTypes)iI).getBuildingHappinessChanges(bct);
		if (iChange != 0)
		{	// <advc.004w>
			bool bParentheses = false;
			if(pCity != NULL) {
				CivicTypes ct = (CivicTypes)iI;
				// Don't mention bonuses that won't happen in this or the next era
				TechTypes tech = (TechTypes)GC.getCivicInfo(ct).getTechPrereq();
				if(tech != NO_TECH && GC.getTechInfo(tech).getEra() -
						pPlayer->getCurrentEra() > 1)
					continue;
				if(!pPlayer->isCivic(ct))
					bParentheses = true;
			} // </advc.004w>
			szFirstBuffer.Format(L"%s%s", NEWLINE, gDLL->getText( // <advc.004w>
					(bParentheses ?
					  "TXT_KEY_BUILDING_CIVIC_HEALTH_HAPPINESS_CHANGE_PAR"
					: "TXT_KEY_BUILDING_CIVIC_HEALTH_HAPPINESS_CHANGE"),
					// </advc.004w>
					abs(iChange), ((iChange > 0) ? gDLL->getSymbolID(HAPPY_CHAR): gDLL->getSymbolID(UNHAPPY_CHAR))).c_str());
			// <advc.004w>
			if(bParentheses)
				szTempBuffer.Format(L"<link=literal>%s)</link>", GC.getCivicInfo((CivicTypes)iI).getDescription());
			else // </advc.004w>
				szTempBuffer.Format(L"<link=literal>%s</link>", GC.getCivicInfo((CivicTypes)iI).getDescription());
			setListHelp(szBuffer, szFirstBuffer, szTempBuffer, L", ", (iChange != iLast));
			iLast = iChange;
		}
	}

	for (iI = 0; iI < NUM_DOMAIN_TYPES; ++iI)
	{
		if (kBuilding.getDomainProductionModifier(iI) != 0)
		{
			szBuffer.append(NEWLINE);
			szBuffer.append(gDLL->getText("TXT_KEY_BUILDING_BUILDS_FASTER_DOMAIN", GC.getDomainInfo((DomainTypes)iI).getTextKeyWide(), kBuilding.getDomainProductionModifier(iI)));
		}
	}

	bool bFirst = true;

	for (iI = 0; iI < GC.getNumUnitInfos(); ++iI)
	{
		if (GC.getUnitInfo((UnitTypes)iI).getPrereqBuilding() == eBuilding)
		{
			szFirstBuffer.Format(L"%s%s", NEWLINE, gDLL->getText("TXT_KEY_BUILDING_REQUIRED_TO_TRAIN").c_str());
			szTempBuffer.Format( SETCOLR L"<link=literal>%s</link>" ENDCOLR , TEXT_COLOR("COLOR_UNIT_TEXT"), GC.getUnitInfo((UnitTypes)iI).getDescription());
			setListHelp(szBuffer, szFirstBuffer, szTempBuffer, L", ", bFirst);
			bFirst = false;
		}
	}

	bFirst = true;
	/*  advc.004w: If in city screen, the player is apparently not constructing
		the building through a Great Person. */
	if(!gDLL->getInterfaceIFace()->isCityScreenUp())
	{
		for (iI = 0; iI < GC.getNumUnitInfos(); ++iI)
		{
			if (GC.getUnitInfo((UnitTypes)iI).getBuildings(eBuilding) || GC.getUnitInfo((UnitTypes)iI).getForceBuildings(eBuilding))
			{
				szFirstBuffer.Format(L"%s%s", NEWLINE, gDLL->getText("TXT_KEY_UNIT_REQUIRED_TO_BUILD").c_str());
				szTempBuffer.Format( SETCOLR L"<link=literal>%s</link>" ENDCOLR , TEXT_COLOR("COLOR_UNIT_TEXT"), GC.getUnitInfo((UnitTypes)iI).getDescription());
				setListHelp(szBuffer, szFirstBuffer, szTempBuffer, L", ", bFirst);
				bFirst = false;
			}
		}
	}

	iLast = 0;
	for (iI = 0; iI < GC.getNumBuildingClassInfos(); ++iI)
	{
		if (kBuilding.getBuildingHappinessChanges(iI) != 0)
		{
			szTempBuffer.Format(L"%s%s", NEWLINE, gDLL->getText("TXT_KEY_BUILDING_HAPPINESS_CHANGE",
					// UNOFFICIAL_PATCH, Bugfix, 08/28/09, jdog5000: Use absolute value with unhappy face
					abs(kBuilding.getBuildingHappinessChanges(iI)),
					((kBuilding.getBuildingHappinessChanges(iI) > 0) ? gDLL->getSymbolID(HAPPY_CHAR) : gDLL->getSymbolID(UNHAPPY_CHAR))).c_str());
			CvWString szBuilding;
			if (NO_PLAYER != ePlayer)
			{
				BuildingTypes ePlayerBuilding = ((BuildingTypes)(GC.getCivilizationInfo(pPlayer->getCivilizationType()).getCivilizationBuildings(iI)));
				if (NO_BUILDING != ePlayerBuilding)
				{
					szBuilding.Format(L"<link=literal>%s</link>", GC.getBuildingClassInfo((BuildingClassTypes)iI).getDescription());
				}
			}
			else
			{
				szBuilding.Format(L"<link=literal>%s</link>", GC.getBuildingClassInfo((BuildingClassTypes)iI).getDescription());
			}
			setListHelp(szBuffer, szTempBuffer, szBuilding, L", ", (kBuilding.getBuildingHappinessChanges(iI) != iLast));
			iLast = kBuilding.getBuildingHappinessChanges(iI);
		}
	}

	if (kBuilding.getPowerBonus() != NO_BONUS)
	{
		szBuffer.append(NEWLINE);
		szBuffer.append(gDLL->getText("TXT_KEY_BUILDING_PROVIDES_POWER_WITH", GC.getBonusInfo((BonusTypes)kBuilding.getPowerBonus()).getTextKeyWide()));

		/* original bts code
		if (kBuilding.isDirtyPower() && (GC.getDefineINT("DIRTY_POWER_HEALTH_CHANGE") != 0))
		{
			szTempBuffer.Format(L" (+%d%c)", abs(GC.getDefineINT("DIRTY_POWER_HEALTH_CHANGE")), ((GC.getDefineINT("DIRTY_POWER_HEALTH_CHANGE") > 0) ? gDLL->getSymbolID(HEALTHY_CHAR): gDLL->getSymbolID(UNHEALTHY_CHAR)));
			szBuffer.append(szTempBuffer);
		} */
		// K-Mod. Also include base health change from power.
		int iPowerHealth = GC.getDefineINT("POWER_HEALTH_CHANGE") + (kBuilding.isDirtyPower() ? GC.getDefineINT("DIRTY_POWER_HEALTH_CHANGE") : 0);
		if (iPowerHealth)
		{
			szTempBuffer.Format(L" (+%d%c)", abs(iPowerHealth), iPowerHealth > 0 ? gDLL->getSymbolID(HEALTHY_CHAR): gDLL->getSymbolID(UNHEALTHY_CHAR));
			szBuffer.append(szTempBuffer);
		}
		// K-Mod end
	}

	bFirst = true;

	for (iI = 0; iI < GC.getNumBuildingClassInfos(); ++iI)
	{
		BuildingTypes eLoopBuilding = NO_BUILDING; // advc.003
		if (ePlayer != NO_PLAYER)
		{
			eLoopBuilding = ((BuildingTypes)(GC.getCivilizationInfo(pPlayer->getCivilizationType()).getCivilizationBuildings(iI)));
		}
		else
		{
			eLoopBuilding = (BuildingTypes)GC.getBuildingClassInfo((BuildingClassTypes)iI).getDefaultBuildingIndex();
		}
		if(eLoopBuilding == NO_BUILDING)
			continue; // advc.003
		if (GC.getBuildingInfo(eLoopBuilding).isBuildingClassNeededInCity(bct)
				&& !bInBuildingList) // advc.004w
		{
			if ((pCity == NULL) || pCity->canConstruct(eLoopBuilding, false, true))
			{
				szFirstBuffer.Format(L"%s%s", NEWLINE, gDLL->getText("TXT_KEY_BUILDING_REQUIRED_TO_BUILD").c_str());
				szTempBuffer.Format(SETCOLR L"<link=literal>%s</link>" ENDCOLR, TEXT_COLOR("COLOR_BUILDING_TEXT"), GC.getBuildingInfo(eLoopBuilding).getDescription());
				setListHelp(szBuffer, szFirstBuffer, szTempBuffer, L", ", bFirst);
				bFirst = false;
			}
		}
	}

	if (bCivilopediaText)
	{
		// (advc.004w: ProductionTraits moved into setProductionSpeedHelp)
		for (int i = 0; i < GC.getNumTraitInfos(); ++i)
		{
			if (kBuilding.getHappinessTraits((TraitTypes)i) != 0)
			{
				szBuffer.append(NEWLINE);
				szBuffer.append(gDLL->getText("TXT_KEY_BUILDING_HAPPINESS_TRAIT", kBuilding.getHappinessTraits((TraitTypes)i), GC.getTraitInfo((TraitTypes)i).getTextKeyWide()));
			}
		}
	}

	if (bCivilopediaText)
	{
		if (kBuilding.getGreatPeopleUnitClass() != NO_UNITCLASS)
		{
			UnitTypes eGreatPeopleUnit = NO_UNIT; // advc.003
			if (ePlayer != NO_PLAYER)
			{
				eGreatPeopleUnit = ((UnitTypes)(GC.getCivilizationInfo(pPlayer->getCivilizationType()).getCivilizationUnits(kBuilding.getGreatPeopleUnitClass())));
			}
			else
			{
				eGreatPeopleUnit = (UnitTypes)GC.getUnitClassInfo((UnitClassTypes)kBuilding.getGreatPeopleUnitClass()).getDefaultUnitIndex();
			}

			if (eGreatPeopleUnit!= NO_UNIT)
			{
				szBuffer.append(NEWLINE);
				szBuffer.append(gDLL->getText("TXT_KEY_BUILDING_LIKELY_TO_GENERATE", GC.getUnitInfo(eGreatPeopleUnit).getTextKeyWide()));
			}
		}
		// advc.004w: Commented out
		/*if (kBuilding.getFreeStartEra() != NO_ERA) {
			szBuffer.append(NEWLINE);
			szBuffer.append(gDLL->getText("TXT_KEY_BUILDING_FREE_START_ERA", GC.getEraInfo((EraTypes)kBuilding.getFreeStartEra()).getTextKeyWide()));
		}*/
	}

	if (!CvWString(kBuilding.getHelp()).empty())
	{
		szBuffer.append(NEWLINE);
		szBuffer.append(kBuilding.getHelp());
	}
	if(!bInBuildingList) // advc.004w
		buildBuildingRequiresString(szBuffer, eBuilding, bCivilopediaText, bTechChooserText, pCity);

	if (pCity != NULL /* advc.004w: */ && !bInBuildingList)
	{
		if (!(GC.getBuildingClassInfo(bct).isNoLimit()))
		{
			if (isWorldWonderClass(bct))
			{
				if (pCity->isWorldWondersMaxed())
				{
					szBuffer.append(NEWLINE);
					szBuffer.append(gDLL->getText("TXT_KEY_BUILDING_WORLD_WONDERS_PER_CITY", GC.getDefineINT("MAX_WORLD_WONDERS_PER_CITY")));
				}
			}
			else if (isTeamWonderClass(bct))
			{
				if (pCity->isTeamWondersMaxed())
				{
					szBuffer.append(NEWLINE);
					szBuffer.append(gDLL->getText("TXT_KEY_BUILDING_TEAM_WONDERS_PER_CITY", GC.getDefineINT("MAX_TEAM_WONDERS_PER_CITY")));
				}
			}
			else if (isNationalWonderClass(bct))
			{
				if (pCity->isNationalWondersMaxed())
				{
					int iMaxNumWonders = (g.isOption(GAMEOPTION_ONE_CITY_CHALLENGE) && GET_PLAYER(pCity->getOwner()).isHuman()) ? GC.getDefineINT("MAX_NATIONAL_WONDERS_PER_CITY_FOR_OCC") : GC.getDefineINT("MAX_NATIONAL_WONDERS_PER_CITY");
					szBuffer.append(NEWLINE);
					szBuffer.append(gDLL->getText("TXT_KEY_BUILDING_NATIONAL_WONDERS_PER_CITY", iMaxNumWonders));
				}
			}
			else
			{
				if (pCity->isBuildingsMaxed())
				{
					szBuffer.append(NEWLINE);
					szBuffer.append(gDLL->getText("TXT_KEY_BUILDING_NUM_PER_CITY", GC.getDefineINT("MAX_BUILDINGS_PER_CITY")));
				}
			}
		}
	} /* <advc.004> Show this right before the cost in Civilopedia text and
		 otherwise (i.e. in hover text) after the cost */
	if(bCivilopediaText)
		setProductionSpeedHelp(szBuffer, ORDER_CONSTRUCT, &kBuilding, pCity, true);
	// </advc.004w>
	/*if ((pCity == NULL) || pCity->getNumRealBuilding(eBuilding) < GC.getCITY_MAX_NUM_BUILDINGS()){
		if (!bCivilopediaText) { */
	/*  advc.004w: Replacing the above. inBuildingList is based on getNumBuilding,
		which, I think, makes more sense than getNumRealBuilding. */
	if(!bInBuildingList)
	{
		if (pCity == NULL)
		{
			if (kBuilding.getProductionCost() > 0)
			{
				szTempBuffer.Format(L"\n%d%c",
						(ePlayer == NO_PLAYER ? kBuilding.getProductionCost() : // advc.003
						pPlayer->getProductionNeeded(eBuilding)), GC.getYieldInfo(YIELD_PRODUCTION).getChar());
				szBuffer.append(szTempBuffer);
			}
		}
		else
		{
			// BUG - Building Actual Effects (edited and moved by K-Mod) - start
			if (bActual && (GC.altKey() || getBugOptionBOOL("MiscHover__BuildingActualEffects", false)) &&
					(pCity->getOwner() == g.getActivePlayer() ||
					//gDLL->getChtLvl() > 0))
					GC.getGame().isDebugMode())) // advc.135c
				setBuildingNetEffectsHelp(szBuffer, eBuilding, pCity);
			// BUG - Building Actual Effects - end

			szBuffer.append(NEWLINE);
			int iTurns = pCity->getProductionTurnsLeft(eBuilding,
					(GC.ctrlKey() || !GC.shiftKey()) ? 0 : pCity->getOrderQueueLength());
			if(iTurns < MAX_INT) { // advc.004x
				szBuffer.append(gDLL->getText("TXT_KEY_BUILDING_NUM_TURNS", iTurns));
				szBuffer.append(L" - "); // advc.004x: Moved up
			}

			int iProduction = pCity->getBuildingProduction(eBuilding);
			int iProductionNeeded = pCity->getProductionNeeded(eBuilding);
			if (iProduction > 0)
			{
				szTempBuffer.Format(L"%d/%d%c", iProduction, iProductionNeeded, GC.getYieldInfo(YIELD_PRODUCTION).getChar());
				szBuffer.append(szTempBuffer);
			}
			else
			{
				szTempBuffer.Format(L"%d%c", iProductionNeeded, GC.getYieldInfo(YIELD_PRODUCTION).getChar());
				szBuffer.append(szTempBuffer);
			}
		}
		// <advc.004w> BonusProductionModifier moved into subroutine
		if(!bCivilopediaText)
			setProductionSpeedHelp(szBuffer, ORDER_CONSTRUCT, &kBuilding, pCity, false);
		// </advc.004w>
		if (kBuilding.getObsoleteTech() != NO_TECH)
		{
			szBuffer.append(NEWLINE);
			szBuffer.append(gDLL->getText(/* advc.004w: */ szObsoleteWithTag,
					GC.getTechInfo((TechTypes) kBuilding.getObsoleteTech()).
					getTextKeyWide()));

			if (kBuilding.getDefenseModifier() != 0 || kBuilding.getBombardDefenseModifier() != 0)
			{
				szBuffer.append(gDLL->getText("TXT_KEY_BUILDING_OBSOLETE_EXCEPT"));
			}
		}

		if (kBuilding.getSpecialBuildingType() != NO_SPECIALBUILDING)
		{
			if (GC.getSpecialBuildingInfo((SpecialBuildingTypes)
					kBuilding.getSpecialBuildingType()).getObsoleteTech() != NO_TECH)
			{
				szBuffer.append(NEWLINE);
				szBuffer.append(gDLL->getText(/* advc.004w: */ szObsoleteWithTag,
						GC.getTechInfo((TechTypes) GC.getSpecialBuildingInfo(
						(SpecialBuildingTypes) kBuilding.getSpecialBuildingType()).
						getObsoleteTech()).getTextKeyWide()));
			}
		}
	} // <advc.004w>
	else if(bObsolete) {
		// Copy-pasted from above
		if(kBuilding.getObsoleteTech() != NO_TECH) {
			szBuffer.append(NEWLINE);
			szBuffer.append(gDLL->getText(szObsoleteWithTag, GC.getTechInfo((TechTypes)
					kBuilding.getObsoleteTech()).getTextKeyWide()));
			if(kBuilding.getDefenseModifier() != 0 || kBuilding.getBombardDefenseModifier() != 0)
				szBuffer.append(gDLL->getText("TXT_KEY_BUILDING_OBSOLETE_EXCEPT"));
		}
		if(kBuilding.getSpecialBuildingType() != NO_SPECIALBUILDING) {
			if(GC.getSpecialBuildingInfo((SpecialBuildingTypes)kBuilding.
					getSpecialBuildingType()).getObsoleteTech() != NO_TECH) {
				szBuffer.append(NEWLINE);
				szBuffer.append(gDLL->getText(szObsoleteWithTag,
						GC.getTechInfo((TechTypes)GC.getSpecialBuildingInfo(
						(SpecialBuildingTypes)kBuilding.getSpecialBuildingType()).
						getObsoleteTech()).getTextKeyWide()));
			}
		}
	} // </advc.004w>
	// K-Mod. Moved from inside that }, above.
	if (pCity &&
			//gDLL->getChtLvl() > 0
			GC.getGame().isDebugMode() // advc.135c
			&& GC.ctrlKey())
	{
		int iBuildingValue = pCity->AI_buildingValue(eBuilding, 0, 0, true);
		szBuffer.append(CvWString::format(L"\nAI Building Value = %d", iBuildingValue));
	} // K-Mod end

	if (bStrategyText)
	{
		if (!CvWString(kBuilding.getStrategy()).empty())
		{
			if (pPlayer->isOption(PLAYEROPTION_ADVISOR_HELP))
			{
				szBuffer.append(SEPARATOR);
				szBuffer.append(NEWLINE);
				szBuffer.append(gDLL->getText("TXT_KEY_SIDS_TIPS"));
				szBuffer.append(L'\"');
				szBuffer.append(kBuilding.getStrategy());
				szBuffer.append(L'\"');
			}
		}
	}

	if (bCivilopediaText)
	{
		if (eDefaultBuilding == eBuilding)
		{
			for (iI = 0; iI < GC.getNumBuildingInfos(); ++iI)
			{
				if (iI != eBuilding)
				{
					if (bct == GC.getBuildingInfo((BuildingTypes)iI).getBuildingClassType())
					{
						szBuffer.append(NEWLINE);
						szBuffer.append(gDLL->getText("TXT_KEY_REPLACED_BY_BUILDING", GC.getBuildingInfo((BuildingTypes)iI).getTextKeyWide()));
					}
				}
			}
		}
	}

}

void CvGameTextMgr::buildBuildingRequiresString(CvWStringBuffer& szBuffer, BuildingTypes eBuilding, bool bCivilopediaText, bool bTechChooserText, const CvCity* pCity)
{
	// K-mod note. I've made a couple of style adjustments throughout this function to make it easier for me to read & maintain.

	CvBuildingInfo& kBuilding = GC.getBuildingInfo(eBuilding);

	PlayerTypes ePlayer = pCity ? pCity->getOwner() : GC.getGame().getActivePlayer();

	if (NULL == pCity || !pCity->canConstruct(eBuilding))
	{
		if (kBuilding.getHolyCity() != NO_RELIGION)
		{
			if (pCity == NULL || !pCity->isHolyCity((ReligionTypes)(kBuilding.getHolyCity())))
			{
				szBuffer.append(NEWLINE);
				szBuffer.append(gDLL->getText("TXT_KEY_ACTION_ONLY_HOLY_CONSTRUCT", GC.getReligionInfo((ReligionTypes) kBuilding.getHolyCity()).getChar()));
			}
		}

		bool bFirst = true;

		if (kBuilding.getSpecialBuildingType() != NO_SPECIALBUILDING)
		{
			if ((pCity == NULL) || !(GC.getGame().isSpecialBuildingValid((SpecialBuildingTypes)(kBuilding.getSpecialBuildingType()))))
			{
				for (int iI = 0; iI < GC.getNumProjectInfos(); ++iI)
				{
					if (GC.getProjectInfo((ProjectTypes)iI).getEveryoneSpecialBuilding() == kBuilding.getSpecialBuildingType())
					{
						CvWString szTempBuffer;
						szTempBuffer.Format(L"%s%s", NEWLINE, gDLL->getText("TXT_KEY_REQUIRES").c_str());
						CvWString szProject;
						szProject.Format(L"<link=literal>%s</link>", GC.getProjectInfo((ProjectTypes)iI).getDescription());
						setListHelp(szBuffer, szTempBuffer, szProject, gDLL->getText("TXT_KEY_OR").c_str(), bFirst);
						bFirst = false;
					}
				}
			}
		}

		if (!bFirst)
		{
			szBuffer.append(ENDCOLR);
		}

		if (kBuilding.getSpecialBuildingType() != NO_SPECIALBUILDING)
		{
			if ((pCity == NULL) || !(GC.getGame().isSpecialBuildingValid((SpecialBuildingTypes)(kBuilding.getSpecialBuildingType()))))
			{
				TechTypes eTech = (TechTypes)GC.getSpecialBuildingInfo((SpecialBuildingTypes)kBuilding.getSpecialBuildingType()).getTechPrereqAnyone();
				if (NO_TECH != eTech)
				{
					szBuffer.append(NEWLINE);
					szBuffer.append(gDLL->getText("TXT_KEY_REQUIRES_TECH_ANYONE", GC.getTechInfo(eTech).getTextKeyWide()));
				}
			}
		}

		for (int iI = 0; iI < GC.getNumBuildingClassInfos(); ++iI)
		{
			// K-Mod note: I've rearranged the conditions in the following block. Originally it was something like this:
			//if (ePlayer == NO_PLAYER && kBuilding.getPrereqNumOfBuildingClass((BuildingClassTypes)iI) > 0)
			//else if (ePlayer != NO_PLAYER && GET_PLAYER(ePlayer).getBuildingClassPrereqBuilding(eBuilding, ((BuildingClassTypes)iI)) > 0)
			//

			// K-Mod. Check that we can actually build this class of building. (Previously this was checked in every single block below.)
			BuildingTypes eLoopBuilding = (BuildingTypes)(ePlayer == NO_PLAYER
				? GC.getBuildingClassInfo((BuildingClassTypes)iI).getDefaultBuildingIndex()
				: GC.getCivilizationInfo(GET_PLAYER(ePlayer).getCivilizationType()).getCivilizationBuildings(iI));
			if (eLoopBuilding == NO_BUILDING)
				continue;
			// K-Mod end

			/*  UNOFFICIAL_PATCH, Bugfix, 06/10/10, EmperorFool:
				show "Requires Hospital" if "Requires Hospital (x/5)" requirement has been met */
			bool bShowedPrereq = false;

			if (kBuilding.getPrereqNumOfBuildingClass((BuildingClassTypes)iI) > 0)
			{
				if (ePlayer == NO_PLAYER)
				{	// <advc.004y>
					int iLow = kBuilding.getPrereqNumOfBuildingClass((BuildingClassTypes)iI);
					WorldSizeTypes eWorld = (WorldSizeTypes)(GC.getNumWorldInfos() - 1);
					int iHigh = iLow * std::max(0, (GC.getWorldInfo(eWorld).
							getBuildingClassPrereqModifier() + 100));
					iHigh = (int)::ceil(iHigh / 100.0); // advc.140
					// </advc.004y>
					CvWString szTempBuffer;
					szTempBuffer.Format(L"%s%s", NEWLINE, gDLL->getText(
							// <advc.004y>
							(iLow == iHigh ?
							"TXT_KEY_BUILDING_REQ_BUILDINGS_NOCITY" :
							"TXT_KEY_BUILDING_REQ_BUILDINGS_NOCITY_RANGE"),
							// </advc.004y>
							GC.getBuildingInfo(eLoopBuilding).getTextKeyWide(),
							iLow, iHigh // advc.004y
						).c_str());

					szBuffer.append(szTempBuffer);
					bShowedPrereq = true; // UNOFFICIAL_PATCH, Bugfix, 06/10/10, EmperorFool
				}
				else
				{
					// K-Mod. In the city screen, include in the prereqs calculation the number of eBuilding under construction.
					// But in the civilopedia, don't even include the number we have already built!
					const CvPlayer& kPlayer = GET_PLAYER(ePlayer);
					int iNeeded = kPlayer.getBuildingClassPrereqBuilding(eBuilding, (BuildingClassTypes)iI, bCivilopediaText ? -kPlayer.getBuildingClassCount((BuildingClassTypes)kBuilding.getBuildingClassType()) : kPlayer.getBuildingClassMaking((BuildingClassTypes)kBuilding.getBuildingClassType()));
					FAssert(iNeeded > 0);
					//if ((pCity == NULL) || (GET_PLAYER(ePlayer).getBuildingClassCount((BuildingClassTypes)iI) < GET_PLAYER(ePlayer).getBuildingClassPrereqBuilding(eBuilding, ((BuildingClassTypes)iI))))
					// K-Mod end

					CvWString szTempBuffer;
					if (pCity != NULL)
					{
						//szTempBuffer.Format(L"%s%s", NEWLINE, gDLL->getText("TXT_KEY_BUILDING_REQUIRES_NUM_SPECIAL_BUILDINGS", GC.getBuildingInfo(eLoopBuilding).getTextKeyWide(), GET_PLAYER(ePlayer).getBuildingClassCount((BuildingClassTypes)iI), GET_PLAYER(ePlayer).getBuildingClassPrereqBuilding(eBuilding, ((BuildingClassTypes)iI))).c_str());
						szTempBuffer.Format(L"%s%s", NEWLINE, gDLL->getText("TXT_KEY_BUILDING_REQUIRES_NUM_SPECIAL_BUILDINGS", GC.getBuildingInfo(eLoopBuilding).getTextKeyWide(), kPlayer.getBuildingClassCount((BuildingClassTypes)iI), iNeeded).c_str()); // K-Mod
					}
					else
					{
						//szTempBuffer.Format(L"%s%s", NEWLINE, gDLL->getText("TXT_KEY_BUILDING_REQUIRES_NUM_SPECIAL_BUILDINGS_NO_CITY", GC.getBuildingInfo(eLoopBuilding).getTextKeyWide(), kPlayer.getBuildingClassPrereqBuilding(eBuilding, (BuildingClassTypes)iI)).c_str());
						szTempBuffer.Format(L"%s%s", NEWLINE, gDLL->getText("TXT_KEY_BUILDING_REQUIRES_NUM_SPECIAL_BUILDINGS_NO_CITY", GC.getBuildingInfo(eLoopBuilding).getTextKeyWide(), iNeeded).c_str()); // K-Mod
					}

					szBuffer.append(szTempBuffer);
					bShowedPrereq = true; // UNOFFICIAL_PATCH, Bugfix, 06/10/10, EmperorFool
				}
			}
			// UNOFFICIAL_PATCH, Bugfix (bShowedPrereq), 06/10/10, EmperorFool: START
			if (!bShowedPrereq && kBuilding.isBuildingClassNeededInCity(iI))
			{
				if ((pCity == NULL) || (pCity->getNumBuilding(eLoopBuilding) <= 0))
				{
					szBuffer.append(NEWLINE);
					szBuffer.append(gDLL->getText("TXT_KEY_BUILDING_REQUIRES_STRING", GC.getBuildingInfo(eLoopBuilding).getTextKeyWide()));
				}
			} // UNOFFICIAL_PATCH: END
		}

		if (kBuilding.isStateReligion())
		{
			if (NULL == pCity || NO_PLAYER == ePlayer || NO_RELIGION == GET_PLAYER(ePlayer).getStateReligion() || !pCity->isHasReligion(GET_PLAYER(ePlayer).getStateReligion()))
			{
				szBuffer.append(NEWLINE);
				szBuffer.append(gDLL->getText("TXT_KEY_REQUIRES_STATE_RELIGION"));
			}
		}
		// <advc.179>
		if(bCivilopediaText || ePlayer == NO_PLAYER)
			buildBuildingReligionYieldString(szBuffer, kBuilding);
		bool bEligibilityDone = false;
		/*  Moved Triggered Election and Eligibility down so that the
			VoteSource stuff appears in one place. */
		// </advc.179>
		for (int iI = 0; iI < GC.getNumVoteSourceInfos(); ++iI)
		{
			if (kBuilding.getVoteSourceType() == (VoteSourceTypes)iI)
			{
				szBuffer.append(NEWLINE);
				szBuffer.append(gDLL->getText("TXT_KEY_BUILDING_DIPLO_VOTE", GC.getVoteSourceInfo((VoteSourceTypes)iI).getTextKeyWide()));
				// <advc.179> Put eligiblity in the same line
				if(kBuilding.isForceTeamVoteEligible()) {
					szBuffer.append(L" " + gDLL->getText(
							"TXT_KEY_BUILDING_ELECTION_ELIGIBILITY_SHORT"));
					bEligibilityDone = true;
				} // </advc.179>
			}
		}
		if (kBuilding.isForceTeamVoteEligible()
				&& !bEligibilityDone) // advc.179
		{
			szBuffer.append(NEWLINE);
			szBuffer.append(gDLL->getText("TXT_KEY_BUILDING_ELECTION_ELIGIBILITY"));
		}

		if (kBuilding.getNumCitiesPrereq() > 0)
		{
			if (NO_PLAYER == ePlayer || GET_PLAYER(ePlayer).getNumCities() < kBuilding.getNumCitiesPrereq())
			{
				szBuffer.append(NEWLINE);
				szBuffer.append(gDLL->getText("TXT_KEY_BUILDING_REQUIRES_NUM_CITIES", kBuilding.getNumCitiesPrereq()));
			}
		}

		if (kBuilding.getUnitLevelPrereq() > 0)
		{
			if (NO_PLAYER == ePlayer || GET_PLAYER(ePlayer).getHighestUnitLevel() < kBuilding.getUnitLevelPrereq())
			{
				szBuffer.append(NEWLINE);
				szBuffer.append(gDLL->getText("TXT_KEY_BUILDING_REQUIRES_UNIT_LEVEL", kBuilding.getUnitLevelPrereq()));
			}
		}

		if (kBuilding.getMinLatitude() > 0)
		{
			if (NULL == pCity || pCity->plot()->getLatitude() < kBuilding.getMinLatitude())
			{
				szBuffer.append(NEWLINE);
				szBuffer.append(gDLL->getText("TXT_KEY_MIN_LATITUDE", kBuilding.getMinLatitude()));
			}
		}

		if (kBuilding.getMaxLatitude() < 90)
		{
			if (NULL == pCity || pCity->plot()->getLatitude() > kBuilding.getMaxLatitude())
			{
				szBuffer.append(NEWLINE);
				szBuffer.append(gDLL->getText("TXT_KEY_MAX_LATITUDE", kBuilding.getMaxLatitude()));
			}
		}

		if (pCity != NULL)
		{
			if (GC.getGame().isNoNukes())
			{
				if (kBuilding.isAllowsNukes())
				{
					for (int iI = 0; iI < GC.getNumUnitInfos(); ++iI)
					{
						if (GC.getUnitInfo((UnitTypes)iI).getNukeRange() != -1)
						{
							szBuffer.append(NEWLINE);
							szBuffer.append(gDLL->getText("TXT_KEY_PROJECT_NO_NUKES"));
							break;
						}
					}
				}
			}
		}

		if (bCivilopediaText)
		{	// advc.008a: Moved this block up a bit
			if (kBuilding.getNumTeamsPrereq() > 0)
			{
				szBuffer.append(NEWLINE);
				szBuffer.append(gDLL->getText("TXT_KEY_BUILDING_REQUIRES_NUM_TEAMS", kBuilding.getNumTeamsPrereq()));
			}
			if (kBuilding.getVictoryPrereq() != NO_VICTORY)
			{
				// <advc.008a>
				if(ePlayer == NO_PLAYER ||
						!GC.getGame().isVictoryValid((VictoryTypes)
						(kBuilding.getVictoryPrereq()))) {
					if(ePlayer != NO_PLAYER)
						szBuffer.append(gDLL->getText("TXT_KEY_COLOR_NEGATIVE"));
					// </advc.008a>
					szBuffer.append(NEWLINE);
					szBuffer.append(gDLL->getText("TXT_KEY_BUILDING_REQUIRES_VICTORY", GC.getVictoryInfo((VictoryTypes)(kBuilding.getVictoryPrereq())).getTextKeyWide()));
					// <advc.008a>
					if(ePlayer != NO_PLAYER)
						szBuffer.append(gDLL->getText("TXT_KEY_COLOR_REVERT"));
				} // </advc.008a>
			}

			if (kBuilding.getMaxStartEra() != NO_ERA)
			{	// <advc.008a>
				TechTypes eTechReq = (TechTypes)kBuilding.getPrereqAndTech();
				bool bUnavailable = (ePlayer != NO_PLAYER &&
						GC.getGame().getStartEra() >
						kBuilding.getMaxStartEra());
				if(bUnavailable || (ePlayer == NO_PLAYER && (eTechReq == NO_TECH ||
						GC.getTechInfo(eTechReq).getEra() !=
						kBuilding.getMaxStartEra() - 1))) { // </advc.008a>
					szBuffer.append(NEWLINE);
					// <advc.008a>
					if(bUnavailable)
						szBuffer.append(gDLL->getText("TXT_KEY_COLOR_NEGATIVE"));
					// </advc.008a>
					szBuffer.append(gDLL->getText("TXT_KEY_BUILDING_MAX_START_ERA",
							GC.getEraInfo((EraTypes)kBuilding.getMaxStartEra()).
							getTextKeyWide()));
					// <advc.008a>
					if(bUnavailable)
						szBuffer.append(gDLL->getText("TXT_KEY_COLOR_REVERT"));
					// </advc.008a>
				}
			}
			// advc.008a: REQUIRES_NUM_TEAMS moved up
		}
		else
		{
			if (!bTechChooserText)
			{
				if (kBuilding.getPrereqAndTech() != NO_TECH)
				{
					if (ePlayer == NO_PLAYER || !(GET_TEAM(GET_PLAYER(ePlayer).getTeam()).isHasTech((TechTypes)(kBuilding.getPrereqAndTech()))))
					{
						szBuffer.append(NEWLINE);
						szBuffer.append(gDLL->getText("TXT_KEY_BUILDING_REQUIRES_STRING", GC.getTechInfo((TechTypes)(kBuilding.getPrereqAndTech())).getTextKeyWide()));
					}
				}
			}

			bFirst = true;

			for (int iI = 0; iI < GC.getNUM_BUILDING_AND_TECH_PREREQS(); ++iI)
			{
				if (kBuilding.getPrereqAndTechs(iI) != NO_TECH)
				{
					if (bTechChooserText || ePlayer == NO_PLAYER || !(GET_TEAM(GET_PLAYER(ePlayer).getTeam()).isHasTech((TechTypes)(kBuilding.getPrereqAndTechs(iI)))))
					{
						CvWString szTempBuffer;
						szTempBuffer.Format(L"%s%s", NEWLINE, gDLL->getText("TXT_KEY_REQUIRES").c_str());
						setListHelp(szBuffer, szTempBuffer, GC.getTechInfo(((TechTypes)(kBuilding.getPrereqAndTechs(iI)))).getDescription(), gDLL->getText("TXT_KEY_AND").c_str(), bFirst);
						bFirst = false;
					}
				}
			}
			// <advc.004w> Based on code copied from above
			if(kBuilding.getSpecialBuildingType() != NO_SPECIALBUILDING) {
				SpecialBuildingTypes eSpecial = (SpecialBuildingTypes)kBuilding.getSpecialBuildingType();
				if(pCity == NULL || !GC.getGame().isSpecialBuildingValid(eSpecial)) {
					TechTypes eTechReq = (TechTypes)GC.getSpecialBuildingInfo(eSpecial).
							getTechPrereq();
					if(eTechReq != NO_TECH) {
						szBuffer.append(NEWLINE);
						szBuffer.append(gDLL->getText("TXT_KEY_REQUIRES"));
						szBuffer.append(GC.getTechInfo(eTechReq).getDescription());
						szBuffer.append(gDLL->getText("TXT_KEY_COLOR_REVERT"));
					}
				}
			} // </advc.004w>

			if (!bFirst)
			{
				szBuffer.append(ENDCOLR);
			}

			if (kBuilding.getPrereqAndBonus() != NO_BONUS)
			{
				if ((pCity == NULL) || !(pCity->hasBonus((BonusTypes)kBuilding.getPrereqAndBonus())))
				{
					szBuffer.append(NEWLINE);
					szBuffer.append(gDLL->getText("TXT_KEY_UNIT_REQUIRES_STRING", GC.getBonusInfo((BonusTypes)kBuilding.getPrereqAndBonus()).getTextKeyWide()));
				}
			}

			CvWStringBuffer szBonusList;
			bFirst = true;

			for (int iI = 0; iI < GC.getNUM_BUILDING_PREREQ_OR_BONUSES(); ++iI)
			{
				if (kBuilding.getPrereqOrBonuses(iI) != NO_BONUS)
				{
					if ((pCity == NULL) || !(pCity->hasBonus((BonusTypes)kBuilding.getPrereqOrBonuses(iI))))
					{
						CvWString szTempBuffer;
						szTempBuffer.Format(L"%s%s", NEWLINE, gDLL->getText("TXT_KEY_REQUIRES").c_str());
						setListHelp(szBonusList, szTempBuffer, GC.getBonusInfo((BonusTypes)kBuilding.getPrereqOrBonuses(iI)).getDescription(), gDLL->getText("TXT_KEY_OR").c_str(), bFirst);
						bFirst = false;
					}
					else if (NULL != pCity)
					{
						bFirst = true;
						break;
					}
				}
			}

			if (!bFirst)
			{
				szBonusList.append(ENDCOLR);
				szBuffer.append(szBonusList);
			}

			if (NO_CORPORATION != kBuilding.getFoundsCorporation())
			{
				bFirst = true;
				szBonusList.clear();
				for (int iI = 0; iI < GC.getNUM_CORPORATION_PREREQ_BONUSES(); ++iI)
				{
					BonusTypes eBonus = (BonusTypes)GC.getCorporationInfo((CorporationTypes)kBuilding.getFoundsCorporation()).getPrereqBonus(iI);
					if (NO_BONUS != eBonus)
					{
						if ((pCity == NULL) || !(pCity->hasBonus(eBonus)))
						{
							CvWString szTempBuffer, szFirstBuffer;
							szFirstBuffer.Format(L"%s%s", NEWLINE, gDLL->getText("TXT_KEY_REQUIRES").c_str());
							szTempBuffer.Format(L"<link=literal>%s</link>", GC.getBonusInfo(eBonus).getDescription());
							setListHelp(szBonusList, szFirstBuffer, szTempBuffer, gDLL->getText("TXT_KEY_OR"), bFirst);
							bFirst = false;
						}
						else if (NULL != pCity)
						{
							bFirst = true;
							break;
						}
					}
				}

				if (!bFirst)
				{
					szBonusList.append(ENDCOLR);
					szBuffer.append(szBonusList);
				}
			}
		}
	}
}

// <advc.179>
void CvGameTextMgr::buildBuildingReligionYieldString(CvWStringBuffer& szBuffer,
		CvBuildingInfo const& kBuilding) {

	for(int i = 0; i < GC.getNumVoteSourceInfos(); i++) {
		VoteSourceTypes eVS = (VoteSourceTypes)i;
		if(eVS != kBuilding.getVoteSourceType())
			continue;
		CvVoteSourceInfo& kVS = GC.getVoteSourceInfo(eVS);
		bool bYield = false;
		int aiYields[NUM_YIELD_TYPES];
		for(int j = 0; j < NUM_YIELD_TYPES; j++) {
			aiYields[j] = kVS.getReligionYield(j);
			if(kVS.getReligionYield(j) != 0)
				bYield = true;
		}
		if(bYield) {
			setResumableYieldChangeHelp(szBuffer, L"", L"", L"", aiYields);
			szBuffer.append(L" " + gDLL->getText(
					"TXT_KEY_BUILDING_RELIGION_YIELD",
					kBuilding.getDescription()));
		}
	}
}
// </advc.179>

void CvGameTextMgr::setProjectHelp(CvWStringBuffer &szBuffer, ProjectTypes eProject, bool bCivilopediaText, CvCity* pCity)
{
	PROFILE_FUNC();

	CvWString szTempBuffer;
	CvWString szFirstBuffer;
	int iI;

	if (NO_PROJECT == eProject)
	{
		return;
	}

	CvProjectInfo& kProject = GC.getProjectInfo(eProject);
	PlayerTypes ePlayer = NO_PLAYER;
	if (pCity != NULL)
	{
		ePlayer = pCity->getOwner();
	}
	else
	{
		ePlayer = GC.getGame().getActivePlayer();
	}
	if(!bCivilopediaText) { // advc.003
		szTempBuffer.Format( SETCOLR L"<link=literal>%s</link>" ENDCOLR , TEXT_COLOR("COLOR_PROJECT_TEXT"), kProject.getDescription());
		szBuffer.append(szTempBuffer);
	}
	if (!bCivilopediaText
			&& ePlayer != NO_PLAYER) // advc.004w: Civilopedia from main menu
	{
		int iMaking = TEAMREF(ePlayer).getProjectMaking(eProject); // advc.004w
		if (isWorldProject(eProject))
		{	// <advc.004w>
			int iMaxGlobal = kProject.getMaxGlobalInstances();
			szBuffer.append(NEWLINE); // newline in any case </advc.004w>
			if (pCity == NULL)
			{	// <advc.004w>
				if(iMaxGlobal == 1)
					szBuffer.append(gDLL->getText("TXT_KEY_PROJECT_WORLD1"));
				else szBuffer.append(gDLL->getText("TXT_KEY_PROJECT_WORLD_NUM_ALLOWED",
						iMaxGlobal)); // </advc.004w>
			}
			else
			{	// <advc.004w>
				if(iMaxGlobal == 1)
					szBuffer.append(gDLL->getText("TXT_KEY_PROJECT_WORLD1"));
				else szBuffer.append(gDLL->getText("TXT_KEY_PROJECT_WORLD_NUM_LEFT",
						iMaxGlobal - GC.getGame().getProjectCreatedCount(eProject) -
						iMaking)); // </advc.004w>
			}
		}

		if (isTeamProject(eProject))
		{	// <advc.004w>
			int iMaxTeam = kProject.getMaxTeamInstances();
			szBuffer.append(NEWLINE); // newline in any case </advc.004w>
			if (pCity == NULL)
			{	// <advc.004w>
				if(iMaxTeam == 1)
					szBuffer.append(gDLL->getText("TXT_KEY_PROJECT_TEAM1"));
				else szBuffer.append(gDLL->getText("TXT_KEY_PROJECT_TEAM_NUM_ALLOWED",
						iMaxTeam)); // </advc.004w>
			}
			else
			{	// <advc.004w>
				if(iMaxTeam == 1)
					szBuffer.append(gDLL->getText("TXT_KEY_PROJECT_TEAM1"));
				else szBuffer.append(gDLL->getText("TXT_KEY_PROJECT_TEAM_NUM_LEFT",
						iMaxTeam - TEAMREF(ePlayer).getProjectCount(eProject) -
						iMaking)); // </advc.004w>
			}
		}
	}

	if (kProject.getNukeInterception() != 0)
	{
		szBuffer.append(NEWLINE);
		szBuffer.append(gDLL->getText("TXT_KEY_PROJECT_CHANCE_INTERCEPT_NUKES", kProject.getNukeInterception()));
	}

	if (kProject.getTechShare() != 0)
	{
		szBuffer.append(NEWLINE);
		szBuffer.append(gDLL->getText("TXT_KEY_PROJECT_TECH_SHARE", kProject.getTechShare()));
	}

	if (kProject.isAllowsNukes())
	{
		szBuffer.append(NEWLINE);
		szBuffer.append(gDLL->getText("TXT_KEY_PROJECT_ENABLES_NUKES"));
	}

	if (kProject.getEveryoneSpecialUnit() != NO_SPECIALUNIT)
	{
		szBuffer.append(NEWLINE);
		szBuffer.append(gDLL->getText("TXT_KEY_PROJECT_ENABLES_SPECIAL", GC.getSpecialUnitInfo((SpecialUnitTypes)(kProject.getEveryoneSpecialUnit())).getTextKeyWide()));
	}

	if (kProject.getEveryoneSpecialBuilding() != NO_SPECIALBUILDING)
	{
		szBuffer.append(NEWLINE);
		szBuffer.append(gDLL->getText("TXT_KEY_PROJECT_ENABLES_SPECIAL", GC.getSpecialBuildingInfo((SpecialBuildingTypes)(kProject.getEveryoneSpecialBuilding())).getTextKeyWide()));
	}

	for (iI = 0; iI < GC.getNumVictoryInfos(); ++iI)
	{
		if (kProject.getVictoryThreshold(iI) > 0)
		{
			if (kProject.getVictoryThreshold(iI) == kProject.getVictoryMinThreshold(iI))
			{
				szTempBuffer.Format(L"%d", kProject.getVictoryThreshold(iI));
			}
			else
			{
				szTempBuffer.Format(L"%d-%d", kProject.getVictoryMinThreshold(iI), kProject.getVictoryThreshold(iI));
			}

			szBuffer.append(NEWLINE);
			szBuffer.append(gDLL->getText("TXT_KEY_PROJECT_REQUIRED_FOR_VICTORY", szTempBuffer.GetCString(), GC.getVictoryInfo((VictoryTypes)iI).getTextKeyWide()));
		}
	}

	bool bFirst = true;

	for (iI = 0; iI < GC.getNumProjectInfos(); ++iI)
	{
		if (GC.getProjectInfo((ProjectTypes)iI).getAnyoneProjectPrereq() == eProject)
		{
			szFirstBuffer.Format(L"%s%s", NEWLINE, gDLL->getText("TXT_KEY_PROJECT_REQUIRED_TO_CREATE_ANYONE").c_str());
			szTempBuffer.Format(SETCOLR L"<link=literal>%s</link>" ENDCOLR, TEXT_COLOR("COLOR_PROJECT_TEXT"), GC.getProjectInfo((ProjectTypes)iI).getDescription());
			setListHelp(szBuffer, szFirstBuffer, szTempBuffer, L", ", bFirst);
			bFirst = false;
		}
	}

	bFirst = true;

	for (iI = 0; iI < GC.getNumProjectInfos(); ++iI)
	{
		if (GC.getProjectInfo((ProjectTypes)iI).getProjectsNeeded(eProject) > 0)
		{
			if ((pCity == NULL) || pCity->canCreate(((ProjectTypes)iI), false, true))
			{
				szFirstBuffer.Format(L"%s%s", NEWLINE, gDLL->getText("TXT_KEY_PROJECT_REQUIRED_TO_CREATE").c_str());
				szTempBuffer.Format(SETCOLR L"<link=literal>%s</link>" ENDCOLR, TEXT_COLOR("COLOR_PROJECT_TEXT"), GC.getProjectInfo((ProjectTypes)iI).getDescription());
				setListHelp(szBuffer, szFirstBuffer, szTempBuffer, L", ", bFirst);
				bFirst = false;
			}
		}
	}

	if ((pCity == NULL) || !(pCity->canCreate(eProject)))
	{
		if (pCity != NULL)
		{
			if (GC.getGame().isNoNukes())
			{
				if (kProject.isAllowsNukes())
				{
					for (iI = 0; iI < GC.getNumUnitInfos(); ++iI)
					{
						if (GC.getUnitInfo((UnitTypes)iI).getNukeRange() != -1)
						{
							szBuffer.append(NEWLINE);
							szBuffer.append(gDLL->getText("TXT_KEY_PROJECT_NO_NUKES"));
							break;
						}
					}
				}
			}
		}

		if (kProject.getAnyoneProjectPrereq() != NO_PROJECT)
		{
			if ((pCity == NULL) || (GC.getGame().getProjectCreatedCount((ProjectTypes)(kProject.getAnyoneProjectPrereq())) == 0))
			{
				szBuffer.append(NEWLINE);
				szBuffer.append(gDLL->getText("TXT_KEY_PROJECT_REQUIRES_ANYONE", GC.getProjectInfo((ProjectTypes)kProject.getAnyoneProjectPrereq()).getTextKeyWide()));
			}
		}

		for (iI = 0; iI < GC.getNumProjectInfos(); ++iI)
		{
			if (kProject.getProjectsNeeded(iI) > 0)
			{
				if ((pCity == NULL) || (GET_TEAM(GET_PLAYER(ePlayer).getTeam()).getProjectCount((ProjectTypes)iI) < kProject.getProjectsNeeded(iI)))
				{
					if (pCity != NULL)
					{
						szBuffer.append(NEWLINE);
						szBuffer.append(gDLL->getText("TXT_KEY_PROJECT_REQUIRES", GC.getProjectInfo((ProjectTypes)iI).getTextKeyWide(), GET_TEAM(GET_PLAYER(ePlayer).getTeam()).getProjectCount((ProjectTypes)iI), kProject.getProjectsNeeded(iI)));
					}
					else
					{
						szBuffer.append(NEWLINE);
						szBuffer.append(gDLL->getText("TXT_KEY_PROJECT_REQUIRES_NO_CITY", GC.getProjectInfo((ProjectTypes)iI).getTextKeyWide(), kProject.getProjectsNeeded(iI)));
					}
				}
			}
		}

		if (bCivilopediaText)
		{
			if (kProject.getVictoryPrereq() != NO_VICTORY)
			{	// <advc.008a>
				if(ePlayer == NO_PLAYER ||
						!GC.getGame().isVictoryValid((VictoryTypes)
						(kProject.getVictoryPrereq()))) {
					if(ePlayer != NO_PLAYER)
						szBuffer.append(gDLL->getText("TXT_KEY_COLOR_NEGATIVE"));
					// </advc.008a>
					szBuffer.append(NEWLINE);
					szBuffer.append(gDLL->getText("TXT_KEY_PROJECT_REQUIRES_STRING_VICTORY", GC.getVictoryInfo((VictoryTypes)(kProject.getVictoryPrereq())).getTextKeyWide()));
					// <advc.008a>
					if(ePlayer != NO_PLAYER)
						szBuffer.append(gDLL->getText("TXT_KEY_COLOR_REVERT"));
					// </advc.008a>
				}
			}
		}
	}

	//if (!bCivilopediaText) { // advc.003
	if (pCity == NULL)
	{
		int iCost=-1; // advc.251
		if (ePlayer != NO_PLAYER)
			iCost = GET_PLAYER(ePlayer).getProductionNeeded(eProject);
		else
		{
			// <advc.251>
			int const iBaseCost = kProject.getProductionCost();
			iCost = iBaseCost;
			iCost *= GC.getDefineINT("PROJECT_PRODUCTION_PERCENT");
			iCost /= 100;
			// To match CvPlayer::getProductionNeeded
			iCost = ::roundToMultiple(iCost, iBaseCost > 500 ? 50 : 5);
			// </advc.251>
		}
		szTempBuffer.Format(L"\n%d%c", iCost, GC.getYieldInfo(YIELD_PRODUCTION).getChar());
		szBuffer.append(szTempBuffer);
	}
	else
	{
		szBuffer.append(NEWLINE);
		int iTurns = pCity->getProductionTurnsLeft(eProject,
				(GC.ctrlKey() || !GC.shiftKey()) ? 0 : pCity->getOrderQueueLength());
		if(iTurns < MAX_INT) { // advc.004x
			szBuffer.append(gDLL->getText("TXT_KEY_PROJECT_NUM_TURNS", iTurns));
			szBuffer.append(L" - "); // advc.004x: Moved up
		}

		int iProduction = pCity->getProjectProduction(eProject);
		if (iProduction > 0)
		{
			szTempBuffer.Format(L"%d/%d%c", iProduction, pCity->getProductionNeeded(eProject), GC.getYieldInfo(YIELD_PRODUCTION).getChar());
		}
		else
		{
			szTempBuffer.Format(L"%d%c", pCity->getProductionNeeded(eProject), GC.getYieldInfo(YIELD_PRODUCTION).getChar());
		}
		szBuffer.append(szTempBuffer);
	}

	for (iI = 0; iI < GC.getNumBonusInfos(); ++iI)
	{
		if (kProject.getBonusProductionModifier(iI) != 0)
		{
			if (pCity != NULL)
			{
				if (pCity->hasBonus((BonusTypes)iI))
				{
					szBuffer.append(gDLL->getText("TXT_KEY_COLOR_POSITIVE"));
				}
				else
				{
					szBuffer.append(gDLL->getText("TXT_KEY_COLOR_NEGATIVE"));
				}
			}
			if (!bCivilopediaText)
			{
				szBuffer.append(L" (");
			}
			else
			{
				szTempBuffer.Format(L"%s%c", NEWLINE, gDLL->getSymbolID(BULLET_CHAR));
				szBuffer.append(szTempBuffer);
			}
			if (kProject.getBonusProductionModifier(iI) == 100)
			{
				szBuffer.append(gDLL->getText(
						//"TXT_KEY_PROJECT_DOUBLE_SPEED_WITH",
						"TXT_KEY_DOUBLE_PRODUCTION_WITH", // advc.004w
						GC.getBonusInfo((BonusTypes)iI).getTextKeyWide()));
			}
			else
			{
				szBuffer.append(gDLL->getText(
						//TXT_KEY_PROJECT_BUILDS_FASTER_WITH",
						"TXT_KEY_FASTER_PRODUCTION_WITH", // advc.004w
						kProject.getBonusProductionModifier(iI),
						GC.getBonusInfo((BonusTypes)iI).getTextKeyWide()));
			}
			if (!bCivilopediaText)
			{
				szBuffer.append(L')');
			}
			if (pCity != NULL)
			{
				szBuffer.append(gDLL->getText("TXT_KEY_COLOR_REVERT"));
			}
		}
	}

	// K-Mod
	if (pCity &&
			//gDLL->getChtLvl() > 0
			GC.getGame().isDebugMode() // advc.135c
			&& GC.ctrlKey())
	{
		int iValue = pCity->AI_projectValue(eProject);
		szBuffer.append(CvWString::format(L"\nProject Value (base) = %d", iValue));

		ProjectTypes eBestProject = pCity->AI().AI_bestProject(&iValue, // advc.003 (cast replaced)
				true); // advc.001n
		if (eBestProject == eProject)
		{
			szBuffer.append(CvWString::format(SETCOLR L"\n(Best project value (scaled) = %d)" ENDCOLR, TEXT_COLOR("COLOR_LIGHT_GREY"), iValue));
		}
	} // K-Mod end
}


void CvGameTextMgr::setProcessHelp(CvWStringBuffer &szBuffer, ProcessTypes eProcess)
{

	szBuffer.append(GC.getProcessInfo(eProcess).getDescription());
	for (int iI = 0; iI < NUM_COMMERCE_TYPES; ++iI)
	{
		if (GC.getProcessInfo(eProcess).getProductionToCommerceModifier(iI) != 0)
		{
			szBuffer.append(NEWLINE);
			szBuffer.append(gDLL->getText("TXT_KEY_PROCESS_CONVERTS", GC.getProcessInfo(eProcess).getProductionToCommerceModifier(iI), GC.getYieldInfo(YIELD_PRODUCTION).getChar(), GC.getCommerceInfo((CommerceTypes) iI).getChar()));
		}
	}
}

void CvGameTextMgr::setBadHealthHelp(CvWStringBuffer &szBuffer, CvCity& city)
{
	if(city.badHealth() <= 0)
		return;

	int iHealth = -(city.getFreshWaterBadHealth());
	if (iHealth > 0)
	{
		szBuffer.append(gDLL->getText("TXT_KEY_MISC_FROM_FRESH_WATER", iHealth));
		szBuffer.append(NEWLINE);
	}

	iHealth = -(city.getFeatureBadHealth());
	if (iHealth > 0)
	{
		FeatureTypes eFeature = NO_FEATURE;

		for (int iI = 0; iI < NUM_CITY_PLOTS; ++iI)
		{
			CvPlot* pLoopPlot = plotCity(city.getX(), city.getY(), iI);

			if (pLoopPlot != NULL)
			{
				if (pLoopPlot->getFeatureType() != NO_FEATURE)
				{
					if (GC.getFeatureInfo(pLoopPlot->getFeatureType()).getHealthPercent() < 0)
					{
						if (eFeature == NO_FEATURE)
						{
							eFeature = pLoopPlot->getFeatureType();
						}
						else if (eFeature != pLoopPlot->getFeatureType())
						{
							eFeature = NO_FEATURE;
							break;
						}
					}
				}
			}
		}

		szBuffer.append(gDLL->getText("TXT_KEY_MISC_FEAT_HEALTH", iHealth, ((eFeature == NO_FEATURE) ? L"TXT_KEY_MISC_FEATURES" : GC.getFeatureInfo(eFeature).getTextKeyWide())));
		szBuffer.append(NEWLINE);
	}

	iHealth = city.getEspionageHealthCounter();
	if (iHealth > 0)
	{
		szBuffer.append(gDLL->getText("TXT_KEY_MISC_HEALTH_FROM_ESPIONAGE", iHealth));
		szBuffer.append(NEWLINE);
	}

	iHealth = -(city.getPowerBadHealth());
	if (iHealth > 0)
	{
		szBuffer.append(gDLL->getText("TXT_KEY_MISC_HEALTH_FROM_POWER", iHealth));
		szBuffer.append(NEWLINE);
	}

	iHealth = -(city.getBonusBadHealth());
	if (iHealth > 0)
	{
		szBuffer.append(gDLL->getText("TXT_KEY_MISC_HEALTH_FROM_BONUSES", iHealth));
		szBuffer.append(NEWLINE);
	}

	iHealth = -(city.totalBadBuildingHealth());
	if (iHealth > 0)
	{
		szBuffer.append(gDLL->getText("TXT_KEY_MISC_HEALTH_FROM_BUILDINGS", iHealth));
		szBuffer.append(NEWLINE);
	}

	iHealth = -(GET_PLAYER(city.getOwner()).getExtraHealth());
	if (iHealth > 0)
	{
		szBuffer.append(gDLL->getText(
				//"TXT_KEY_MISC_HEALTH_FROM_CIV",
				"TXT_KEY_FROM_TRAIT", // advc.004
				iHealth));
		szBuffer.append(NEWLINE);
	}

	iHealth = -city.getExtraHealth();
	if (iHealth > 0)
	{
		szBuffer.append(gDLL->getText("TXT_KEY_MISC_UNHEALTH_EXTRA", iHealth));
		szBuffer.append(NEWLINE);
	}

	iHealth = -(GC.getHandicapInfo(city.getHandicapType()).getHealthBonus());
	if (iHealth > 0)
	{
		szBuffer.append(gDLL->getText("TXT_KEY_MISC_HEALTH_FROM_HANDICAP", iHealth));
		szBuffer.append(NEWLINE);
	}

	iHealth = city.unhealthyPopulation();
	if (iHealth > 0)
	{
		szBuffer.append(gDLL->getText("TXT_KEY_MISC_HEALTH_FROM_POP", iHealth));
/*
** K-Mod, 29/dec/10, karadoc
** added modifier text
*/
		if (city.getUnhealthyPopulationModifier() != 0)
		{
			wchar szTempBuffer[1024];

			swprintf(szTempBuffer, 1024, L" (%+d%%)", city.getUnhealthyPopulationModifier());
			szBuffer.append(szTempBuffer);
		}
/*
** K-Mod end
*/
		szBuffer.append(NEWLINE);
	}

	szBuffer.append(L"-----------------------\n");

	szBuffer.append(gDLL->getText("TXT_KEY_MISC_TOTAL_UNHEALTHY", city.badHealth()));
}

void CvGameTextMgr::setGoodHealthHelp(CvWStringBuffer &szBuffer, CvCity& city)
{
	if(city.goodHealth() <= 0)
		return;

	int iHealth = city.getFreshWaterGoodHealth();
	if (iHealth > 0)
	{
		szBuffer.append(gDLL->getText("TXT_KEY_MISC_HEALTH_FROM_FRESH_WATER", iHealth));
		szBuffer.append(NEWLINE);
	}

	iHealth = city.getFeatureGoodHealth();
	if (iHealth > 0)
	{
		FeatureTypes eFeature = NO_FEATURE;

		for (int iI = 0; iI < NUM_CITY_PLOTS; ++iI)
		{
			CvPlot* pLoopPlot = plotCity(city.getX(), city.getY(), iI);

			if (pLoopPlot != NULL)
			{
				if (pLoopPlot->getFeatureType() != NO_FEATURE)
				{
					if (GC.getFeatureInfo(pLoopPlot->getFeatureType()).getHealthPercent() > 0)
					{
						if (eFeature == NO_FEATURE)
						{
							eFeature = pLoopPlot->getFeatureType();
						}
						else if (eFeature != pLoopPlot->getFeatureType())
						{
							eFeature = NO_FEATURE;
							break;
						}
					}
				}
			}
		}

		szBuffer.append(gDLL->getText("TXT_KEY_MISC_FEAT_GOOD_HEALTH", iHealth, ((eFeature == NO_FEATURE) ? L"TXT_KEY_MISC_FEATURES" : GC.getFeatureInfo(eFeature).getTextKeyWide())));
		szBuffer.append(NEWLINE);
	}

	iHealth = city.getPowerGoodHealth();
	if (iHealth > 0)
	{
		szBuffer.append(gDLL->getText("TXT_KEY_MISC_GOOD_HEALTH_FROM_POWER", iHealth));
		szBuffer.append(NEWLINE);
	}

	iHealth = city.getBonusGoodHealth();
	if (iHealth > 0)
	{
		szBuffer.append(gDLL->getText("TXT_KEY_MISC_GOOD_HEALTH_FROM_BONUSES", iHealth));
		szBuffer.append(NEWLINE);
	}

	iHealth = city.totalGoodBuildingHealth();
	if (iHealth > 0)
	{
		szBuffer.append(gDLL->getText("TXT_KEY_MISC_GOOD_HEALTH_FROM_BUILDINGS", iHealth));
		szBuffer.append(NEWLINE);
	}

	iHealth = GET_PLAYER(city.getOwner()).getExtraHealth();
	if (iHealth > 0)
	{
		szBuffer.append(gDLL->getText("TXT_KEY_MISC_GOOD_HEALTH_FROM_CIV", iHealth));
		szBuffer.append(NEWLINE);
	}

	iHealth = city.getExtraHealth();
	if (iHealth > 0)
	{
		szBuffer.append(gDLL->getText("TXT_KEY_MISC_HEALTH_EXTRA", iHealth));
		szBuffer.append(NEWLINE);
	}

	iHealth = GC.getHandicapInfo(city.getHandicapType()).getHealthBonus();
	if (iHealth > 0)
	{
		szBuffer.append(gDLL->getText("TXT_KEY_MISC_GOOD_HEALTH_FROM_HANDICAP", iHealth));
		szBuffer.append(NEWLINE);
	}

	szBuffer.append(L"-----------------------\n");

	szBuffer.append(gDLL->getText("TXT_KEY_MISC_TOTAL_HEALTHY", city.goodHealth()));
}

// BUG - Building Additional Health - start
bool CvGameTextMgr::setBuildingAdditionalHealthHelp(CvWStringBuffer &szBuffer, const CvCity& city, const CvWString& szStart, bool bStarted)
{
	CvWString szLabel;
	CvCivilizationInfo& kCivilization = GC.getCivilizationInfo(GET_PLAYER(city.getOwner()).getCivilizationType());

	for (int iI = 0; iI < GC.getNumBuildingClassInfos(); iI++)
	{
		BuildingTypes eBuilding = (BuildingTypes)kCivilization.getCivilizationBuildings((BuildingClassTypes)iI);

		if (eBuilding != NO_BUILDING && city.canConstruct(eBuilding, false, true, false))
		{
			int iGood = 0, iBad = 0;
			//int iChange = // advc.003: unused
				city.getAdditionalHealthByBuilding(eBuilding, iGood, iBad);

			if (iGood != 0 || iBad != 0)
			{
				int iSpoiledFood = city.getAdditionalSpoiledFood(iGood, iBad);
				int iStarvation = city.getAdditionalStarvation(iSpoiledFood);

				if (!bStarted)
				{
					szBuffer.append(szStart);
					bStarted = true;
				}

				szLabel.Format(SETCOLR L"%s" ENDCOLR, TEXT_COLOR("COLOR_BUILDING_TEXT"), GC.getBuildingInfo(eBuilding).getDescription());
				bool bStartedLine = setResumableGoodBadChangeHelp(szBuffer, szLabel, L": ", L"", iGood, gDLL->getSymbolID(HEALTHY_CHAR), iBad, gDLL->getSymbolID(UNHEALTHY_CHAR), false, true);
				setResumableValueChangeHelp(szBuffer, szLabel, L": ", L"", iSpoiledFood, gDLL->getSymbolID(EATEN_FOOD_CHAR), false, true, bStartedLine);
				setResumableValueChangeHelp(szBuffer, szLabel, L": ", L"", iStarvation, gDLL->getSymbolID(BAD_FOOD_CHAR), false, true, bStartedLine);
			}
		}
	}

	return bStarted;
}
// BUG - Building Additional Health - end

void CvGameTextMgr::setAngerHelp(CvWStringBuffer &szBuffer, CvCity& city)
{
	int iOldAngerPercent;
	int iNewAngerPercent;
	int iOldAnger;
	int iNewAnger;
	int iAnger;
	int iI;

	if (city.isOccupation())
	{
		szBuffer.append(gDLL->getText("TXT_KEY_ANGER_RESISTANCE"));
	}
	else if (GET_PLAYER(city.getOwner()).isAnarchy())
	{
		szBuffer.append(gDLL->getText("TXT_KEY_ANGER_ANARCHY"));
	}
	else if (city.unhappyLevel() > 0)
	{
		iOldAngerPercent = 0;
		iNewAngerPercent = 0;

		iOldAnger = 0;
		iNewAnger = 0;

/*
** K-Mod, 5/jan/11, karadoc
** all anger perecent bits were like this:
iNewAnger += (((iNewAngerPercent * city.getPopulation()) / GC.getPERCENT_ANGER_DIVISOR()) - ((iOldAngerPercent * city.getPopulation()) / GC.getDefineINT("PERCENT_ANGER_DIVISOR")));
** I've changed it to use GC.getPERCENT_ANGER_DIVISOR() for both parts.
*/
		// XXX decomp these???
		iNewAngerPercent += city.getOvercrowdingPercentAnger();
		iNewAnger += (((iNewAngerPercent * city.getPopulation()) / GC.getPERCENT_ANGER_DIVISOR()) - ((iOldAngerPercent * city.getPopulation()) / GC.getPERCENT_ANGER_DIVISOR()));
		iAnger = ((iNewAnger - iOldAnger) + std::min(0, iOldAnger));
		if (iAnger > 0)
		{
			szBuffer.append(gDLL->getText("TXT_KEY_ANGER_OVERCROWDING", iAnger));
			szBuffer.append(NEWLINE);
		}
		iOldAngerPercent = iNewAngerPercent;
		iOldAnger = iNewAnger;

		iNewAngerPercent += city.getNoMilitaryPercentAnger();
		iNewAnger += (((iNewAngerPercent * city.getPopulation()) / GC.getPERCENT_ANGER_DIVISOR()) - ((iOldAngerPercent * city.getPopulation()) / GC.getPERCENT_ANGER_DIVISOR()));
		iAnger = ((iNewAnger - iOldAnger) + std::min(0, iOldAnger));
		if (iAnger > 0)
		{
			szBuffer.append(gDLL->getText("TXT_KEY_ANGER_MILITARY_PROTECTION", iAnger));
			szBuffer.append(NEWLINE);
		}
		iOldAngerPercent = iNewAngerPercent;
		iOldAnger = iNewAnger;

		iNewAngerPercent += city.getCulturePercentAnger();
		iNewAnger += (((iNewAngerPercent * city.getPopulation()) / GC.getPERCENT_ANGER_DIVISOR()) - ((iOldAngerPercent * city.getPopulation()) / GC.getPERCENT_ANGER_DIVISOR()));
		iAnger = ((iNewAnger - iOldAnger) + std::min(0, iOldAnger));
		if (iAnger > 0)
		{
			szBuffer.append(gDLL->getText("TXT_KEY_ANGER_OCCUPIED", iAnger));
			szBuffer.append(NEWLINE);
		}
		iOldAngerPercent = iNewAngerPercent;
		iOldAnger = iNewAnger;

		iNewAngerPercent += city.getReligionPercentAnger();
		iNewAnger += (((iNewAngerPercent * city.getPopulation()) / GC.getPERCENT_ANGER_DIVISOR()) - ((iOldAngerPercent * city.getPopulation()) / GC.getPERCENT_ANGER_DIVISOR()));
		iAnger = ((iNewAnger - iOldAnger) + std::min(0, iOldAnger));
		if (iAnger > 0)
		{
			szBuffer.append(gDLL->getText("TXT_KEY_ANGER_RELIGION_FIGHT", iAnger));
			szBuffer.append(NEWLINE);
		}
		iOldAngerPercent = iNewAngerPercent;
		iOldAnger = iNewAnger;

		iNewAngerPercent += city.getHurryPercentAnger();
		iNewAnger += (((iNewAngerPercent * city.getPopulation()) / GC.getPERCENT_ANGER_DIVISOR()) - ((iOldAngerPercent * city.getPopulation()) / GC.getPERCENT_ANGER_DIVISOR()));
		iAnger = ((iNewAnger - iOldAnger) + std::min(0, iOldAnger));
		if (iAnger > 0)
		{
			szBuffer.append(gDLL->getText("TXT_KEY_ANGER_OPPRESSION", iAnger));
			szBuffer.append(NEWLINE);
		}
		iOldAngerPercent = iNewAngerPercent;
		iOldAnger = iNewAnger;

		iNewAngerPercent += city.getConscriptPercentAnger();
		iNewAnger += (((iNewAngerPercent * city.getPopulation()) / GC.getPERCENT_ANGER_DIVISOR()) - ((iOldAngerPercent * city.getPopulation()) / GC.getPERCENT_ANGER_DIVISOR()));
		iAnger = ((iNewAnger - iOldAnger) + std::min(0, iOldAnger));
		if (iAnger > 0)
		{
			szBuffer.append(gDLL->getText("TXT_KEY_ANGER_DRAFT", iAnger));
			szBuffer.append(NEWLINE);
		}
		iOldAngerPercent = iNewAngerPercent;
		iOldAnger = iNewAnger;

		iNewAngerPercent += city.getDefyResolutionPercentAnger();
		iNewAnger += (((iNewAngerPercent * city.getPopulation()) / GC.getPERCENT_ANGER_DIVISOR()) - ((iOldAngerPercent * city.getPopulation()) / GC.getPERCENT_ANGER_DIVISOR()));
		iAnger = ((iNewAnger - iOldAnger) + std::min(0, iOldAnger));
		if (iAnger > 0)
		{
			szBuffer.append(gDLL->getText("TXT_KEY_ANGER_DEFY_RESOLUTION", iAnger));
			szBuffer.append(NEWLINE);
		}
		iOldAngerPercent = iNewAngerPercent;
		iOldAnger = iNewAnger;

		iNewAngerPercent += city.getWarWearinessPercentAnger();
		iNewAnger += (((iNewAngerPercent * city.getPopulation()) / GC.getPERCENT_ANGER_DIVISOR()) - ((iOldAngerPercent * city.getPopulation()) / GC.getPERCENT_ANGER_DIVISOR()));
		iAnger = ((iNewAnger - iOldAnger) + std::min(0, iOldAnger));
		if (iAnger > 0)
		{
			szBuffer.append(gDLL->getText("TXT_KEY_ANGER_WAR_WEAR", iAnger));
			szBuffer.append(NEWLINE);
		}
		iOldAngerPercent = iNewAngerPercent;
		iOldAnger = iNewAnger;
/*
** K-Mod, 30/dec/10, karadoc
** Global warming unhappiness
*/
		// when I say 'percent' I mean 1/100. Unfortunately, people who made the rest of the game meant something else...
		// so I have to multiply my GwPercentAnger by 10 to make it fit in.
		iNewAngerPercent += std::max(0, GET_PLAYER(city.getOwner()).getGwPercentAnger()*10);
		iNewAnger += (((iNewAngerPercent * city.getPopulation()) / GC.getPERCENT_ANGER_DIVISOR()) - ((iOldAngerPercent * city.getPopulation()) / GC.getPERCENT_ANGER_DIVISOR()));
		iAnger = ((iNewAnger - iOldAnger) + std::min(0, iOldAnger));
		if (iAnger > 0)
		{
			szBuffer.append(gDLL->getText("TXT_KEY_ANGER_GLOBAL_WARMING", iAnger));
			szBuffer.append(NEWLINE);
		}
		iOldAngerPercent = iNewAngerPercent;
		iOldAnger = iNewAnger;
/*
** K-Mod end
*/

		iNewAnger += std::max(0, city.getVassalUnhappiness());
		iAnger = ((iNewAnger - iOldAnger) + std::min(0, iOldAnger));
		if (iAnger > 0)
		{
			szBuffer.append(gDLL->getText("TXT_KEY_UNHAPPY_VASSAL", iAnger));
			szBuffer.append(NEWLINE);
		}
		iOldAnger = iNewAnger;

		iNewAnger += std::max(0, city.getEspionageHappinessCounter());
		iAnger = ((iNewAnger - iOldAnger) + std::min(0, iOldAnger));
		if (iAnger > 0)
		{
			szBuffer.append(gDLL->getText("TXT_KEY_ANGER_ESPIONAGE", iAnger));
			szBuffer.append(NEWLINE);
		}
		iOldAnger = iNewAnger;

		for (iI = 0; iI < GC.getNumCivicInfos(); ++iI)
		{
			iNewAngerPercent += GET_PLAYER(city.getOwner()).getCivicPercentAnger((CivicTypes)iI);
			iNewAnger += (((iNewAngerPercent * city.getPopulation()) / GC.getPERCENT_ANGER_DIVISOR()) - ((iOldAngerPercent * city.getPopulation()) / GC.getPERCENT_ANGER_DIVISOR()));
			iAnger = ((iNewAnger - iOldAnger) + std::min(0, iOldAnger));
			if (iAnger > 0)
			{
				szBuffer.append(gDLL->getText("TXT_KEY_ANGER_DEMAND_CIVIC", iAnger, GC.getCivicInfo((CivicTypes) iI).getTextKeyWide()));
				szBuffer.append(NEWLINE);
			}
			iOldAngerPercent = iNewAngerPercent;
			iOldAnger = iNewAnger;
		}

		iNewAnger -= std::min(0, city.getLargestCityHappiness());
		iAnger = ((iNewAnger - iOldAnger) + std::min(0, iOldAnger));
		if (iAnger > 0)
		{
			szBuffer.append(gDLL->getText("TXT_KEY_ANGER_BIG_CITY", iAnger));
			szBuffer.append(NEWLINE);
		}
		iOldAnger = iNewAnger;

		iNewAnger -= std::min(0, city.getMilitaryHappiness());
		iAnger = ((iNewAnger - iOldAnger) + std::min(0, iOldAnger));
		if (iAnger > 0)
		{
			szBuffer.append(gDLL->getText("TXT_KEY_ANGER_MILITARY_PRESENCE", iAnger));
			szBuffer.append(NEWLINE);
		}
		iOldAnger = iNewAnger;

		iNewAnger -= std::min(0, city.getCurrentStateReligionHappiness());
		iAnger = ((iNewAnger - iOldAnger) + std::min(0, iOldAnger));
		if (iAnger > 0)
		{
			szBuffer.append(gDLL->getText("TXT_KEY_ANGER_STATE_RELIGION", iAnger));
			szBuffer.append(NEWLINE);
		}
		iOldAnger = iNewAnger;

		iNewAnger -= std::min(0, (city.getBuildingBadHappiness() + city.getExtraBuildingBadHappiness()));
		iAnger = ((iNewAnger - iOldAnger) + std::min(0, iOldAnger));
		if (iAnger > 0)
		{
			szBuffer.append(gDLL->getText("TXT_KEY_ANGER_BUILDINGS", iAnger));
			szBuffer.append(NEWLINE);
		}
		iOldAnger = iNewAnger;

		iNewAnger -= std::min(0, city.getFeatureBadHappiness());
		iAnger = ((iNewAnger - iOldAnger) + std::min(0, iOldAnger));
		if (iAnger > 0)
		{
			szBuffer.append(gDLL->getText("TXT_KEY_ANGER_FEATURES", iAnger));
			szBuffer.append(NEWLINE);
		}
		iOldAnger = iNewAnger;

		iNewAnger -= std::min(0, city.getBonusBadHappiness());
		iAnger = ((iNewAnger - iOldAnger) + std::min(0, iOldAnger));
		if (iAnger > 0)
		{
			szBuffer.append(gDLL->getText("TXT_KEY_ANGER_BONUS", iAnger));
			szBuffer.append(NEWLINE);
		}
		iOldAnger = iNewAnger;

		iNewAnger -= std::min(0, city.getReligionBadHappiness());
		iAnger = ((iNewAnger - iOldAnger) + std::min(0, iOldAnger));
		if (iAnger > 0)
		{
			szBuffer.append(gDLL->getText("TXT_KEY_ANGER_RELIGIOUS_FREEDOM", iAnger));
			szBuffer.append(NEWLINE);
		}
		iOldAnger = iNewAnger;

		iNewAnger -= std::min(0, city.getCommerceHappiness());
		iAnger = ((iNewAnger - iOldAnger) + std::min(0, iOldAnger));
		if (iAnger > 0)
		{
			szBuffer.append(gDLL->getText("TXT_KEY_ANGER_BAD_ENTERTAINMENT", iAnger));
			szBuffer.append(NEWLINE);
		}
		iOldAnger = iNewAnger;

		iNewAnger -= std::min(0, city.area()->getBuildingHappiness(city.getOwner()));
		iAnger = ((iNewAnger - iOldAnger) + std::min(0, iOldAnger));
		if (iAnger > 0)
		{
			szBuffer.append(gDLL->getText("TXT_KEY_ANGER_BUILDINGS", iAnger));
			szBuffer.append(NEWLINE);
		}
		iOldAnger = iNewAnger;

		iNewAnger -= std::min(0, GET_PLAYER(city.getOwner()).getBuildingHappiness());
		iAnger = ((iNewAnger - iOldAnger) + std::min(0, iOldAnger));
		if (iAnger > 0)
		{
			szBuffer.append(gDLL->getText("TXT_KEY_ANGER_BUILDINGS", iAnger));
			szBuffer.append(NEWLINE);
		}
		iOldAnger = iNewAnger;

		iNewAnger -= std::min(0, (city.getExtraHappiness() + GET_PLAYER(city.getOwner()).getExtraHappiness()));
		iAnger = ((iNewAnger - iOldAnger) + std::min(0, iOldAnger));
		if (iAnger > 0)
		{
			szBuffer.append(gDLL->getText("TXT_KEY_ANGER_ARGH", iAnger));
			szBuffer.append(NEWLINE);
		}
		iOldAnger = iNewAnger;

		iNewAnger -= std::min(0, GC.getHandicapInfo(city.getHandicapType()).getHappyBonus());
		iAnger = ((iNewAnger - iOldAnger) + std::min(0, iOldAnger));
		if (iAnger > 0)
		{
			szBuffer.append(gDLL->getText("TXT_KEY_ANGER_HANDICAP", iAnger));
			szBuffer.append(NEWLINE);
		}
		iOldAnger = iNewAnger;

		szBuffer.append(L"-----------------------\n");

		szBuffer.append(gDLL->getText("TXT_KEY_ANGER_TOTAL_UNHAPPY", iOldAnger));

		FAssert(iOldAnger == city.unhappyLevel());
	}
}


void CvGameTextMgr::setHappyHelp(CvWStringBuffer &szBuffer, CvCity& city)
{
	if(city.isOccupation() || GET_PLAYER(city.getOwner()).isAnarchy())
		return;
	if (city.happyLevel() <= 0)
		return;

	int iTotalHappy = 0;
	int iHappy = city.getLargestCityHappiness();
	if (iHappy > 0)
	{
		iTotalHappy += iHappy;
		szBuffer.append(gDLL->getText("TXT_KEY_HAPPY_BIG_CITY", iHappy));
		szBuffer.append(NEWLINE);
	}

	iHappy = city.getMilitaryHappiness();
	if (iHappy > 0)
	{
		iTotalHappy += iHappy;
		szBuffer.append(gDLL->getText("TXT_KEY_HAPPY_MILITARY_PRESENCE", iHappy));
		szBuffer.append(NEWLINE);
	}

	iHappy = city.getVassalHappiness();
	if (iHappy > 0)
	{
		iTotalHappy += iHappy;
		szBuffer.append(gDLL->getText("TXT_KEY_HAPPY_VASSAL", iHappy));
		szBuffer.append(NEWLINE);
	}

	iHappy = city.getCurrentStateReligionHappiness();
	if (iHappy > 0)
	{
		iTotalHappy += iHappy;
		szBuffer.append(gDLL->getText("TXT_KEY_HAPPY_STATE_RELIGION", iHappy));
		szBuffer.append(NEWLINE);
	}

	iHappy = (city.getBuildingGoodHappiness() + city.getExtraBuildingGoodHappiness());
	if (iHappy > 0)
	{
		iTotalHappy += iHappy;
		szBuffer.append(gDLL->getText("TXT_KEY_HAPPY_BUILDINGS", iHappy));
		szBuffer.append(NEWLINE);
	}

	iHappy = city.getFeatureGoodHappiness();
	if (iHappy > 0)
	{
		iTotalHappy += iHappy;
		szBuffer.append(gDLL->getText("TXT_KEY_HAPPY_FEATURES", iHappy));
		szBuffer.append(NEWLINE);
	}

	iHappy = city.getBonusGoodHappiness();
	if (iHappy > 0)
	{
		iTotalHappy += iHappy;
		szBuffer.append(gDLL->getText("TXT_KEY_HAPPY_BONUS", iHappy));
		szBuffer.append(NEWLINE);
	}

	iHappy = city.getReligionGoodHappiness();
	if (iHappy > 0)
	{
		iTotalHappy += iHappy;
		szBuffer.append(gDLL->getText("TXT_KEY_HAPPY_RELIGIOUS_FREEDOM", iHappy));
		szBuffer.append(NEWLINE);
	}

	iHappy = city.getCommerceHappiness();
	if (iHappy > 0)
	{
		iTotalHappy += iHappy;
		szBuffer.append(gDLL->getText("TXT_KEY_HAPPY_ENTERTAINMENT", iHappy));
		szBuffer.append(NEWLINE);
	}

	iHappy = city.area()->getBuildingHappiness(city.getOwner());
	if (iHappy > 0)
	{
		iTotalHappy += iHappy;
		szBuffer.append(gDLL->getText(
				"TXT_KEY_HAPPY_BUILDINGS_AREA", // advc.004g
				iHappy));
		szBuffer.append(NEWLINE);
	}

	iHappy = GET_PLAYER(city.getOwner()).getBuildingHappiness();
	if (iHappy > 0)
	{
		iTotalHappy += iHappy;
		szBuffer.append(gDLL->getText("TXT_KEY_HAPPY_BUILDINGS", iHappy));
		szBuffer.append(NEWLINE);
	}

	iHappy = (city.getExtraHappiness() + GET_PLAYER(city.getOwner()).getExtraHappiness());
	if (iHappy > 0)
	{
		iTotalHappy += iHappy;
		szBuffer.append(gDLL->getText("TXT_KEY_HAPPY_YEAH", iHappy));
		szBuffer.append(NEWLINE);
	}

	if (city.getHappinessTimer() > 0)
	{
		iHappy = GC.getDefineINT("TEMP_HAPPY");
		iTotalHappy += iHappy;
		szBuffer.append(gDLL->getText("TXT_KEY_HAPPY_TEMP", iHappy, city.getHappinessTimer()));
		szBuffer.append(NEWLINE);
	}

	iHappy = GC.getHandicapInfo(city.getHandicapType()).getHappyBonus();
	if (iHappy > 0)
	{
		iTotalHappy += iHappy;
		szBuffer.append(gDLL->getText("TXT_KEY_HAPPY_HANDICAP", iHappy));
		szBuffer.append(NEWLINE);
	}

	szBuffer.append(L"-----------------------\n");

	szBuffer.append(gDLL->getText("TXT_KEY_HAPPY_TOTAL_HAPPY", iTotalHappy));

	FAssert(iTotalHappy == city.happyLevel());
}

// BUG - Building Additional Happiness - start
bool CvGameTextMgr::setBuildingAdditionalHappinessHelp(CvWStringBuffer &szBuffer, const CvCity& city, const CvWString& szStart, bool bStarted)
{
	CvWString szLabel;
	CvCivilizationInfo& kCivilization = GC.getCivilizationInfo(GET_PLAYER(city.getOwner()).getCivilizationType());

	for (int iI = 0; iI < GC.getNumBuildingClassInfos(); iI++)
	{
		BuildingTypes eBuilding = (BuildingTypes)kCivilization.getCivilizationBuildings((BuildingClassTypes)iI);

		if (eBuilding != NO_BUILDING && city.canConstruct(eBuilding, false, true, false))
		{
			int iGood = 0, iBad = 0;
			//int iChange = // advc.003: unused
				city.getAdditionalHappinessByBuilding(eBuilding, iGood, iBad);

			if (iGood != 0 || iBad != 0)
			{
				int iAngryPop = city.getAdditionalAngryPopuplation(iGood, iBad);

				if (!bStarted)
				{
					szBuffer.append(szStart);
					bStarted = true;
				}

				szLabel.Format(SETCOLR L"%s" ENDCOLR, TEXT_COLOR("COLOR_BUILDING_TEXT"), GC.getBuildingInfo(eBuilding).getDescription());
				bool bStartedLine = setResumableGoodBadChangeHelp(szBuffer, szLabel, L": ", L"", iGood, gDLL->getSymbolID(HAPPY_CHAR), iBad, gDLL->getSymbolID(UNHAPPY_CHAR), false, true);
				setResumableValueChangeHelp(szBuffer, szLabel, L": ", L"", iAngryPop, gDLL->getSymbolID(ANGRY_POP_CHAR), false, true, bStartedLine);
			}
		}
	}

	return bStarted;
}
// BUG - Building Additional Happiness - end

/* replaced by BUG
void CvGameTextMgr::setYieldChangeHelp(CvWStringBuffer &szBuffer, const CvWString& szStart, const CvWString& szSpace, const CvWString& szEnd, const int* piYieldChange, bool bPercent, bool bNewLine)
{
	PROFILE_FUNC();

	CvWString szTempBuffer;
	bool bStarted;
	int iI;

	bStarted = false;

	for (iI = 0; iI < NUM_YIELD_TYPES; ++iI)
	{
		if (piYieldChange[iI] != 0)
		{
			if (!bStarted)
			{
				if (bNewLine)
				{
					szTempBuffer.Format(L"\n%c", gDLL->getSymbolID(BULLET_CHAR));
				}
				szTempBuffer += CvWString::format(L"%s%s%s%d%s%c",
					szStart.GetCString(),
					szSpace.GetCString(),
					piYieldChange[iI] > 0 ? L"+" : L"",
					piYieldChange[iI],
					bPercent ? L"%" : L"",
					GC.getYieldInfo((YieldTypes)iI).getChar());
			}
			else
			{
				szTempBuffer.Format(L", %s%d%s%c",
					piYieldChange[iI] > 0 ? L"+" : L"",
					piYieldChange[iI],
					bPercent ? L"%" : L"",
					GC.getYieldInfo((YieldTypes)iI).getChar());
			}
			szBuffer.append(szTempBuffer);

			bStarted = true;
		}
	}

	if (bStarted)
	{
		szBuffer.append(szEnd);
	}
}

void CvGameTextMgr::setCommerceChangeHelp(CvWStringBuffer &szBuffer, const CvWString& szStart, const CvWString& szSpace, const CvWString& szEnd, const int* piCommerceChange, bool bPercent, bool bNewLine)
{
	CvWString szTempBuffer;
	bool bStarted;
	int iI;

	bStarted = false;

	for (iI = 0; iI < NUM_COMMERCE_TYPES; ++iI)
	{
		if (piCommerceChange[iI] != 0)
		{
			if (!bStarted)
			{
				if (bNewLine)
				{
					szTempBuffer.Format(L"\n%c", gDLL->getSymbolID(BULLET_CHAR));
				}
				szTempBuffer += CvWString::format(L"%s%s%s%d%s%c", szStart.GetCString(), szSpace.GetCString(), ((piCommerceChange[iI] > 0) ? L"+" : L""), piCommerceChange[iI], ((bPercent) ? L"%" : L""), GC.getCommerceInfo((CommerceTypes) iI).getChar());
			}
			else
			{
				szTempBuffer.Format(L", %s%d%s%c", ((piCommerceChange[iI] > 0) ? L"+" : L""), piCommerceChange[iI], ((bPercent) ? L"%" : L""), GC.getCommerceInfo((CommerceTypes) iI).getChar());
			}
			szBuffer.append(szTempBuffer);

			bStarted = true;
		}
	}

	if (bStarted)
	{
		szBuffer.append(szEnd);
	}
} */

// BUG - Resumable Value Change Help - start
void CvGameTextMgr::setYieldChangeHelp(CvWStringBuffer &szBuffer, const CvWString& szStart, const CvWString& szSpace, const CvWString& szEnd, const int* piYieldChange, bool bPercent, bool bNewLine)
{
	setResumableYieldChangeHelp(szBuffer, szStart, szSpace, szEnd, piYieldChange, bPercent, bNewLine);
}

/*
 * Adds the ability to pass in and get back the value of bStarted so that
 * it can be used with other setResumable<xx>ChangeHelp() calls on a single line.
 */
bool CvGameTextMgr::setResumableYieldChangeHelp(CvWStringBuffer &szBuffer,
		const CvWString& szStart, const CvWString& szSpace, const CvWString& szEnd,
		const int* piYieldChange, bool bPercent, bool bNewLine, bool bStarted)
{
	CvWString szTempBuffer;

	for (int iI = 0; iI < NUM_YIELD_TYPES; ++iI)
	{
		if (piYieldChange[iI] != 0)
		{
			if (!bStarted)
			{
				if (bNewLine)
				{
					szTempBuffer.Format(L"\n%c", gDLL->getSymbolID(BULLET_CHAR));
				}
				szTempBuffer += CvWString::format(L"%s%s%s%d%s%c",
					szStart.GetCString(),
					szSpace.GetCString(),
					piYieldChange[iI] > 0 ? L"+" : L"",
					piYieldChange[iI],
					bPercent ? L"%" : L"",
					GC.getYieldInfo((YieldTypes)iI).getChar());
			}
			else
			{
				szTempBuffer.Format(L", %s%d%s%c",
					piYieldChange[iI] > 0 ? L"+" : L"",
					piYieldChange[iI],
					bPercent ? L"%" : L"",
					GC.getYieldInfo((YieldTypes)iI).getChar());
			}
			szBuffer.append(szTempBuffer);

			bStarted = true;
		}
	}

	if (bStarted)
	{
		szBuffer.append(szEnd);
	}

// added
	return bStarted;
}

void CvGameTextMgr::setCommerceChangeHelp(CvWStringBuffer &szBuffer, const CvWString& szStart, const CvWString& szSpace, const CvWString& szEnd, const int* piCommerceChange, bool bPercent, bool bNewLine)
{
	setResumableCommerceChangeHelp(szBuffer, szStart, szSpace, szEnd, piCommerceChange, bPercent, bNewLine);
}

/*
 * Adds the ability to pass in and get back the value of bStarted so that
 * it can be used with other setResumable<xx>ChangeHelp() calls on a single line.
 */
bool CvGameTextMgr::setResumableCommerceChangeHelp(CvWStringBuffer &szBuffer, const CvWString& szStart, const CvWString& szSpace, const CvWString& szEnd, const int* piCommerceChange, bool bPercent, bool bNewLine, bool bStarted)
{
	CvWString szTempBuffer;
//	bool bStarted;
	int iI;

//	bStarted = false;

	for (iI = 0; iI < NUM_COMMERCE_TYPES; ++iI)
	{
		if (piCommerceChange[iI] != 0)
		{
			if (!bStarted)
			{
				if (bNewLine)
				{
					szTempBuffer.Format(L"\n%c", gDLL->getSymbolID(BULLET_CHAR));
				}
				szTempBuffer += CvWString::format(L"%s%s%s%d%s%c", szStart.GetCString(), szSpace.GetCString(), ((piCommerceChange[iI] > 0) ? L"+" : L""), piCommerceChange[iI], ((bPercent) ? L"%" : L""), GC.getCommerceInfo((CommerceTypes) iI).getChar());
			}
			else
			{
				szTempBuffer.Format(L", %s%d%s%c", ((piCommerceChange[iI] > 0) ? L"+" : L""), piCommerceChange[iI], ((bPercent) ? L"%" : L""), GC.getCommerceInfo((CommerceTypes) iI).getChar());
			}
			szBuffer.append(szTempBuffer);

			bStarted = true;
		}
	}

	if (bStarted)
	{
		szBuffer.append(szEnd);
	}

// added
	return bStarted;
}

/*
 * Displays float values by dividing each value by 100.
 */
void CvGameTextMgr::setCommerceTimes100ChangeHelp(CvWStringBuffer &szBuffer, const CvWString& szStart, const CvWString& szSpace, const CvWString& szEnd, const int* piCommerceChange, bool bNewLine, bool bStarted)
{
	setResumableCommerceTimes100ChangeHelp(szBuffer, szStart, szSpace, szEnd, piCommerceChange, bNewLine);
}

/*
 * Adds the ability to pass in and get back the value of bStarted so that
 * it can be used with other setResumable<xx>ChangeHelp() calls on a single line.
 */
bool CvGameTextMgr::setResumableCommerceTimes100ChangeHelp(CvWStringBuffer &szBuffer, const CvWString& szStart, const CvWString& szSpace, const CvWString& szEnd, const int* piCommerceChange, bool bNewLine, bool bStarted)
{
	CvWString szTempBuffer;

	for (int iI = 0; iI < NUM_COMMERCE_TYPES; ++iI)
	{
		int iChange = piCommerceChange[iI];
		if (iChange != 0)
		{
			if (!bStarted)
			{
				if (bNewLine)
				{
					szTempBuffer.Format(L"\n%c", gDLL->getSymbolID(BULLET_CHAR));
				}
				szTempBuffer += CvWString::format(L"%s%s", szStart.GetCString(), szSpace.GetCString());
				bStarted = true;
			}
			else
			{
				szTempBuffer.Format(L", ");
			}
			szBuffer.append(szTempBuffer);

			if (iChange % 100 == 0)
			{
				szTempBuffer.Format(L"%+d%c", iChange / 100, GC.getCommerceInfo((CommerceTypes) iI).getChar());
			}
			else
			{
				if (iChange >= 0)
				{
					szBuffer.append(L"+");
				}
				else
				{
					iChange = - iChange;
					szBuffer.append(L"-");
				}
				szTempBuffer.Format(L"%d.%02d%c", iChange / 100, iChange % 100, GC.getCommerceInfo((CommerceTypes) iI).getChar());
			}
			szBuffer.append(szTempBuffer);
		}
	}

	if (bStarted)
	{
		szBuffer.append(szEnd);
	}

	return bStarted;
}

/*
 * Adds the ability to pass in and get back the value of bStarted so that
 * it can be used with other setResumable<xx>ChangeHelp() calls on a single line.
 */
bool CvGameTextMgr::setResumableGoodBadChangeHelp(CvWStringBuffer &szBuffer, const CvWString& szStart, const CvWString& szSpace, const CvWString& szEnd, int iGood, int iGoodSymbol, int iBad, int iBadSymbol, bool bPercent, bool bNewLine, bool bStarted)
{
	bStarted = setResumableValueChangeHelp(szBuffer, szStart, szSpace, szEnd, iGood, iGoodSymbol, bPercent, bNewLine, bStarted);
	bStarted = setResumableValueChangeHelp(szBuffer, szStart, szSpace, szEnd, iBad, iBadSymbol, bPercent, bNewLine, bStarted);

	return bStarted;
}

/*
 * Adds the ability to pass in and get back the value of bStarted so that
 * it can be used with other setResumable<xx>ChangeHelp() calls on a single line.
 */
bool CvGameTextMgr::setResumableValueChangeHelp(CvWStringBuffer &szBuffer, const CvWString& szStart, const CvWString& szSpace, const CvWString& szEnd, int iValue, int iSymbol, bool bPercent, bool bNewLine, bool bStarted)
{
	CvWString szTempBuffer;

	if (iValue != 0)
	{
		if (!bStarted)
		{
			if (bNewLine)
			{
				szTempBuffer.Format(L"\n%c", gDLL->getSymbolID(BULLET_CHAR));
			}
			szTempBuffer += CvWString::format(L"%s%s", szStart.GetCString(), szSpace.GetCString());
		}
		else
		{
			szTempBuffer = L", ";
		}
		szBuffer.append(szTempBuffer);

		szTempBuffer.Format(L"%+d%s%c", iValue, bPercent ? L"%" : L"", iSymbol);
		szBuffer.append(szTempBuffer);

		bStarted = true;
	}

	return bStarted;
}

/*
 * Adds the ability to pass in and get back the value of bStarted so that
 * it can be used with other setResumable<xx>ChangeHelp() calls on a single line.
 */
bool CvGameTextMgr::setResumableValueTimes100ChangeHelp(CvWStringBuffer &szBuffer, const CvWString& szStart, const CvWString& szSpace, const CvWString& szEnd, int iValue, int iSymbol, bool bNewLine, bool bStarted)
{
	CvWString szTempBuffer;

	if (iValue != 0)
	{
		if (!bStarted)
		{
			if (bNewLine)
			{
				szTempBuffer.Format(L"\n%c", gDLL->getSymbolID(BULLET_CHAR));
			}
			szTempBuffer += CvWString::format(L"%s%s", szStart.GetCString(), szSpace.GetCString());
		}
		else
		{
			szTempBuffer = L", ";
		}
		szBuffer.append(szTempBuffer);

		if (iValue % 100 == 0)
		{
			szTempBuffer.Format(L"%+d%c", iValue / 100, iSymbol);
		}
		else
		{
			if (iValue >= 0)
			{
				szBuffer.append(L"+");
			}
			else
			{
				iValue = - iValue;
				szBuffer.append(L"-");
			}
			szTempBuffer.Format(L"%d.%02d%c", iValue / 100, iValue % 100, iSymbol);
		}
		szBuffer.append(szTempBuffer);

		bStarted = true;
	}

	return bStarted;
}
// BUG - Resumable Value Change Help - end

void CvGameTextMgr::setBonusHelp(CvWStringBuffer &szBuffer, BonusTypes eBonus, bool bCivilopediaText)
{
// BULL - Trade Denial - start  (advc.073: bImport param added)
	setBonusTradeHelp(szBuffer, eBonus, bCivilopediaText, NO_PLAYER, false);
}

// This function has been effectly rewritten for K-Mod. (there were a lot of things to change.)
void CvGameTextMgr::setBonusTradeHelp(CvWStringBuffer &szBuffer, BonusTypes eBonus,
		bool bCivilopediaText, PlayerTypes eTradePlayer, bool bImport)
// BULL - Trade Denial - end
{	// <advc.003>
	if(NO_BONUS == eBonus)
		return;
	CvGame const& g = GC.getGame();
	PlayerTypes eActivePlayer = g.getActivePlayer();
	CvPlayerAI const* pActivePlayer = (eActivePlayer == NO_PLAYER ? NULL:
			&GET_PLAYER(eActivePlayer)); // </advc.003>
	// gDLL->isMPDiplomacy() does sth. else, apparently.
	bool bDiplo = (g.isGameMultiPlayer() ? gDLL->isMPDiplomacyScreenUp() :
			gDLL->isDiplomacy());
	CvCity* pCity = (gDLL->getInterfaceIFace()->isCityScreenUp() ?
			/*  A city can also be selected without the city screen being up;
				don't want that here. */
			gDLL->getInterfaceIFace()->getHeadSelectedCity() : NULL);

	int iHappiness = GC.getBonusInfo(eBonus).getHappiness();
	int iHealth = GC.getBonusInfo(eBonus).getHealth();

	if (bCivilopediaText)
	{
		// K-Mod. for the civilopedia text, display the basic bonuses as individual bullet points.
		// (they are displayed beside the name of the bonus when outside of the civilopedia.)
		if (iHappiness != 0)
		{
			szBuffer.append(CvWString::format(L"\n%c+%d%c", gDLL->getSymbolID(BULLET_CHAR), abs(iHappiness), iHappiness > 0 ? gDLL->getSymbolID(HAPPY_CHAR) : gDLL->getSymbolID(UNHAPPY_CHAR)));
		}
		if (iHealth != 0)
		{
			szBuffer.append(CvWString::format(L"\n%c+%d%c", gDLL->getSymbolID(BULLET_CHAR), abs(iHealth), iHealth > 0 ? gDLL->getSymbolID(HEALTHY_CHAR) : gDLL->getSymbolID(UNHEALTHY_CHAR)));
		}
	}
	else
	{
		szBuffer.append(CvWString::format( SETCOLR L"%s" ENDCOLR , TEXT_COLOR("COLOR_HIGHLIGHT_TEXT"), GC.getBonusInfo(eBonus).getDescription()));
		// advc.004w: Don't omit the basic effect in main menu Civilopedia hovers
		//if (NO_PLAYER != eActivePlayer)

		// K-Mod. Bonuses now display "(Obsolete)" instead of "(player has 0)" when the bonus is obsolete.
		if (NO_PLAYER != eActivePlayer && // advc.004w
				TEAMREF(eActivePlayer).isBonusObsolete(eBonus))
			szBuffer.append(gDLL->getText("TXT_KEY_BONUS_OBSOLETE"));
		else
		{
			// display the basic bonuses next to the name of the bonus
			if (iHappiness != 0)
			{
				szBuffer.append(CvWString::format(L", +%d%c", abs(iHappiness),
						iHappiness > 0 ? gDLL->getSymbolID(HAPPY_CHAR) :
						gDLL->getSymbolID(UNHAPPY_CHAR)));
			}
			if (iHealth != 0)
			{
				szBuffer.append(CvWString::format(L", +%d%c", abs(iHealth),
						iHealth > 0 ? gDLL->getSymbolID(HEALTHY_CHAR) :
						gDLL->getSymbolID(UNHEALTHY_CHAR)));
			}
			if(NO_PLAYER != eActivePlayer) { // advc.004w: Moved from above
				// <advc.004w>
				int iAvailable = pActivePlayer->getNumAvailableBonuses(eBonus);
				if(bDiplo || // Trade screen, domestic resources box or import column
						(eTradePlayer != NO_PLAYER &&
						(eTradePlayer == eActivePlayer || bImport))) {
						// </advc.004w>
					szBuffer.append(gDLL->getText("TXT_KEY_BONUS_AVAILABLE_PLAYER",
							iAvailable, pActivePlayer->getNameKey()));
				} // <advc.004w> Show their amount in the export columns
				else if(eTradePlayer != NO_PLAYER &&
						eTradePlayer != eActivePlayer && !bImport) {
					szBuffer.append(gDLL->getText("TXT_KEY_BONUS_AVAILABLE_PLAYER",
							GET_PLAYER(eTradePlayer).getNumAvailableBonuses(eBonus),
							GET_PLAYER(eTradePlayer).getNameKey()));
				} // Don't show amount on city screen if it's 1
				else if(iAvailable != 1 || pCity == NULL) {
					// When not trading, this should be clear enough:
					szBuffer.append(gDLL->getText("TXT_KEY_BONUS_AVAILABLE_US",
							iAvailable));
				} // </advc.004w>
				for (int iCorp = 0; iCorp < GC.getNumCorporationInfos(); ++iCorp)
				{
					bool bCorpBonus = false;
					if (pActivePlayer->getHasCorporationCount((CorporationTypes)iCorp) > 0)
					{
						for (int i = 0; !bCorpBonus && i < GC.getNUM_CORPORATION_PREREQ_BONUSES(); ++i)
						{
							bCorpBonus = eBonus == GC.getCorporationInfo((CorporationTypes)iCorp).getPrereqBonus(i);
						}
					}

					if (bCorpBonus)
						szBuffer.append(GC.getCorporationInfo((CorporationTypes)iCorp).getChar());
				}
			}
		}
		// <advc.004w>
		if(eActivePlayer == NO_PLAYER || (!bDiplo &&
				!gDLL->getInterfaceIFace()->isCityScreenUp() &&
				eTradePlayer == NO_PLAYER)) { // </advc.004w>
			setYieldChangeHelp(szBuffer, L"", L"", gDLL->getText("TXT_KEY_BONUS_ON_PLOT"),
					GC.getBonusInfo(eBonus).getYieldChangeArray());
		}
		// <advc.004w> Only show reveal tech if we don't have it yet
		TechTypes eRevealTech = (TechTypes)GC.getBonusInfo(eBonus).getTechReveal();
		if (eRevealTech != NO_TECH && (eActivePlayer == NO_PLAYER ||
				!TEAMREF(eActivePlayer).isHasTech(eRevealTech))) // </advc.004w>
		{
			szBuffer.append(NEWLINE);
			szBuffer.append(gDLL->getText("TXT_KEY_BONUS_REVEALED_BY", GC.getTechInfo((TechTypes)GC.getBonusInfo(eBonus).getTechReveal()).getTextKeyWide()));
		}
	}

	if(bCivilopediaText || eActivePlayer == NO_PLAYER ||
			// K-Mod. Only display the perks of the bonus if it is not already obsolete
			!TEAMREF(eActivePlayer).isBonusObsolete(eBonus)) {
		// advc.004w: Effects from buildings, projects and units in a subroutine
		setBonusExtraHelp(szBuffer, eBonus, bCivilopediaText, eTradePlayer, bDiplo, pCity);
	}
// BULL - Trade Denial - start  (advc.073: refactored; getBugOptionBOOL check removed)
	if (eTradePlayer != NO_PLAYER && eActivePlayer != NO_PLAYER)
	{
		bool bHuman = GET_PLAYER(eTradePlayer).isHuman(); // advc.073
		TradeData item;
		::setTradeItem(&item, TRADE_RESOURCES, eBonus);
		// <advc.073>
		int iGoldChar = GC.getCommerceInfo(COMMERCE_GOLD).getChar(); // advc.036
		if(eTradePlayer == eActivePlayer) {
			FAssert(!bImport);
			/*  This means we're hovering over a surplus resource of the
				active player and all takers need to be listed. */
			std::vector<PlayerTypes> aTakers;
			for(int i = 0; i < MAX_CIV_PLAYERS; i++) {
				if(i == eActivePlayer)
					continue;
				CvPlayerAI const& kTaker = GET_PLAYER((PlayerTypes)i);
				if(!kTaker.isAlive() || kTaker.isHuman() ||
						!TEAMREF(eActivePlayer).isHasMet(kTaker.getTeam()))
					continue;
				if(pActivePlayer->canTradeItem(kTaker.getID(), item, true) &&
						kTaker.AI_isWillingToTalk(eActivePlayer, true))
					aTakers.push_back(kTaker.getID());
			}
			if(aTakers.empty())
				return;
			szBuffer.append(NEWLINE);
			szBuffer.append(gDLL->getText("TXT_KEY_MISC_WILL_IMPORT"
					// Better not to repeat the resource name
					/*,GC.getBonusInfo(eBonus).getTextKeyWide()*/));
			szBuffer.append(/*L": "*/L":\n");
			/*  Should perhaps sort by the gold they're willing to give, but that's
				tedious to implement, and using player id also has merit as the
				portraits on the Resources tab are listed by id. */
			::setTradeItem(&item, TRADE_GOLD_PER_TURN, 1);
			for(size_t i = 0; i < aTakers.size(); i++) {
				CvPlayerAI const& kTaker = GET_PLAYER(aTakers[i]);
				szBuffer.append(kTaker.getName());
				if(kTaker.canTradeItem(eActivePlayer, item, false)) {
					int iGold = kTaker.AI_goldForBonus(eBonus, eActivePlayer);
					if(iGold > 0)
						szBuffer.append(CvWString::format(L" (%d%c)", iGold, iGoldChar));
				}
				if(i < aTakers.size() - 1)
					szBuffer.append(L", ");
			}
			return;
		}
		if (!bImport && // </advc.073>
				GET_PLAYER(eTradePlayer).canTradeItem(eActivePlayer, item, false))
		{
			DenialTypes eDenial = GET_PLAYER(eTradePlayer).getTradeDenial(
					eActivePlayer, item);
			if (eDenial != NO_DENIAL
					&& eDenial != DENIAL_JOKING) // advc.073
			{
				CvWString szTempBuffer;
				szTempBuffer.Format(SETCOLR L"%s" ENDCOLR,
						TEXT_COLOR("COLOR_NEGATIVE_TEXT"),
						GC.getDenialInfo(eDenial).getDescription());
				szBuffer.append(NEWLINE);
				szBuffer.append(szTempBuffer);
				// <advc.036>
				return;
			}
			else if(eDenial == NO_DENIAL && !bHuman) {
				if(GET_PLAYER(eTradePlayer).AI_isWillingToTalk(eActivePlayer, true)) {
					::setTradeItem(&item, TRADE_GOLD_PER_TURN, 1);
					if(pActivePlayer->canTradeItem(eTradePlayer, item, false)) {
						int iGold = pActivePlayer->AI_goldForBonus(eBonus, eTradePlayer);
						if(iGold > 0) {
							szBuffer.append(NEWLINE);
							szBuffer.append(CvWString::format(L"%s %d%c",
									gDLL->getText("TXT_KEY_MISC_WILL_ASK").GetCString(),
									iGold, iGoldChar));
							return;
						}
					}
				}
				else {
					szBuffer.append(NEWLINE);
					szBuffer.append(gDLL->getText("TXT_KEY_MISC_REFUSES_TO_TALK"));
				}
			}
		}
		if(bImport && !bHuman) {
			bool bWillTalk = GET_PLAYER(eTradePlayer).AI_isWillingToTalk(eActivePlayer, true);
			if(bWillTalk && pActivePlayer->canTradeItem(eTradePlayer, item, true)) {
				::setTradeItem(&item, TRADE_GOLD_PER_TURN, 1);
				if(GET_PLAYER(eTradePlayer).canTradeItem(eActivePlayer, item, false)) {
					int iGold = GET_PLAYER(eTradePlayer).AI_goldForBonus(eBonus, eActivePlayer);
					if(iGold > 0) {
						szBuffer.append(NEWLINE);
						szBuffer.append(CvWString::format(L"%s %d%c",
								gDLL->getText("TXT_KEY_MISC_WILL_PAY").GetCString(),
								iGold, iGoldChar));
					}
				}
			}
			else if(!bWillTalk) {
				szBuffer.append(NEWLINE);
				szBuffer.append(gDLL->getText("TXT_KEY_MISC_REFUSES_TO_TALK"));
			}
		} // </advc.036>
	}
// BULL - Trade Denial - end
}

// advc.004w: Some code cut from setBonusHelp, but mostly new code.
void CvGameTextMgr::setBonusExtraHelp(CvWStringBuffer &szBuffer, BonusTypes eBonus,
		bool bCivilopediaText, PlayerTypes eTradePlayer, bool bDiplo, CvCity* pCity) {

	PROFILE_FUNC();
	CvGame const& g = GC.getGame();
	CivilizationTypes eCivilization = g.getActiveCivilizationType();
	PlayerTypes eActivePlayer = g.getActivePlayer();
	// NULL when in main menu Civilopedia
	CvPlayerAI const* pActivePlayer = NULL;
	int iCurrentEra = 0;
	if(eActivePlayer != NO_PLAYER) {
		pActivePlayer = &GET_PLAYER(eActivePlayer);
		iCurrentEra = pActivePlayer->getCurrentEra();
	}

	for(int i = 0; i < GC.getNumBuildingClassInfos(); i++) {
		BuildingClassTypes eBuildingClass = (BuildingClassTypes)i;
		BuildingTypes eLoopBuilding;
		if(eCivilization == NO_CIVILIZATION) {
			eLoopBuilding = (BuildingTypes)(GC.getBuildingClassInfo(eBuildingClass).
						getDefaultBuildingIndex());
		}
		else {
			eLoopBuilding = (BuildingTypes)(GC.getCivilizationInfo(eCivilization).
					getCivilizationBuildings(i));
		}
		if(eLoopBuilding == NO_BUILDING)
			continue;
		// <advc.004w> Don't list impossibilities
		bool bCanEverConstruct = true;
		if(eActivePlayer != NO_PLAYER && !g.canConstruct(eLoopBuilding, true, true))
			bCanEverConstruct = false;
		// Don't list remote possibilities
		CvBuildingInfo& kBuilding = GC.getBuildingInfo(eLoopBuilding);
		int iTechEraDelta = 0;
		if(bCanEverConstruct && !bCivilopediaText && eActivePlayer != NO_PLAYER) {
			ReligionTypes eReligion = (ReligionTypes)kBuilding.getPrereqReligion();
			if(eReligion != NO_RELIGION &&
					!pActivePlayer->canDoReligion(eReligion))
				bCanEverConstruct = false;
			if(bCanEverConstruct) {
				TechTypes eTech = (TechTypes)kBuilding.getPrereqAndTech();
				if(eTech == NO_TECH) {
					SpecialBuildingTypes eSpecial = (SpecialBuildingTypes)kBuilding.getSpecialBuildingType();
					if(eSpecial != NO_SPECIALBUILDING)
						eTech = (TechTypes)GC.getSpecialBuildingInfo(eSpecial).getTechPrereq();
				}
				if(eTech != NO_TECH)
					iTechEraDelta = GC.getTechInfo(eTech).getEra() - iCurrentEra;
				if(iTechEraDelta >= 2 || (iTechEraDelta >= 1 &&
						// Special treatment for the very early game
						iCurrentEra <= 0 && pActivePlayer->getNumCities() <= 2))
					bCanEverConstruct = false;
			}
			if(bCanEverConstruct && TEAMREF(eActivePlayer).isObsoleteBuilding(eLoopBuilding))
				bCanEverConstruct = false;
			if(bCanEverConstruct) {
				VictoryTypes eVict = (VictoryTypes)kBuilding.getVictoryPrereq();
				if(eVict != NO_VICTORY && !g.isVictoryValid(eVict))
					bCanEverConstruct = false;
			}
			if(bCanEverConstruct && (TEAMREF(eActivePlayer).isBuildingClassMaxedOut(eBuildingClass) ||
					pActivePlayer->isBuildingClassMaxedOut(eBuildingClass)))
				bCanEverConstruct = false;
			if(bCanEverConstruct) {
				// CvCity::canConstruct would be too strict, even with the bIgnore flags.
				if(pCity != NULL && !pCity->isValidBuildingLocation(eLoopBuilding))
					bCanEverConstruct = false;
			}
		}
		// Don't skip e.g. active wonders
		if(!bCanEverConstruct && pActivePlayer->getBuildingClassCount(eBuildingClass) <= 0)
			continue;
		// </advc.004w>
		if(kBuilding.getBonusHappinessChanges(eBonus) != 0) {
			szBuffer.append(CvWString::format(L"\n%s",
					gDLL->getText("TXT_KEY_BUILDING_CIVIC_HEALTH_HAPPINESS_CHANGE",
					abs(kBuilding.getBonusHappinessChanges(eBonus)),
					kBuilding.getBonusHappinessChanges(eBonus) > 0 ?
					gDLL->getSymbolID(HAPPY_CHAR) :
					gDLL->getSymbolID(UNHAPPY_CHAR)).c_str()));
			//szBuffer.append(szBuildingDescr); // advc.004w: Handled below
		}
		if(kBuilding.getBonusHealthChanges(eBonus) != 0) {
			szBuffer.append(CvWString::format(L"\n%s",
					gDLL->getText("TXT_KEY_BUILDING_CIVIC_HEALTH_HAPPINESS_CHANGE",
					abs(kBuilding.getBonusHealthChanges(eBonus)),
					kBuilding.getBonusHealthChanges(eBonus) > 0 ?
					gDLL->getSymbolID(HEALTHY_CHAR) :
					gDLL->getSymbolID(UNHEALTHY_CHAR)).c_str()));
			//szBuffer.append(szBuildingDescr); // advc.004w: Handled below
		} // <advc.004w>
		if(kBuilding.getBonusHealthChanges(eBonus) != 0 ||
				kBuilding.getBonusHappinessChanges(eBonus) != 0) {
			CvWString szDescr(kBuilding.getDescription());
			if(pCity != NULL) {
				::applyColorToString(szDescr,
						pCity->getNumActiveBuilding(eLoopBuilding) > 0 ?
						"COLOR_POSITIVE_TEXT" : "COLOR_NEGATIVE_TEXT");
			}
			szBuffer.append(szDescr);
		}
		/*  Not much room in the "Effects" box of Civilopedia. Put only health
			and happiness effects there. */
		if(bCivilopediaText)
			continue;
		bool bWonder = (::isWorldWonderClass(eBuildingClass) || ::isNationalWonderClass(eBuildingClass));
		// Similar to setBuildingHelpActual, but that's from the building's pov.
		if(eActivePlayer != NO_PLAYER && ((pCity == NULL &&
				// Show yield modifiers of wonders only if we already have them
				(!bWonder || pActivePlayer->getBuildingClassCount(eBuildingClass) > 0))
				|| (pCity != NULL && pCity->getNumActiveBuilding(eLoopBuilding) > 0))) {
			for(int y = 0; y < NUM_YIELD_TYPES; y++) {
				if(kBuilding.getBonusYieldModifier(eBonus, y) != 0) {
					CvWString szStart(kBuilding.getDescription());
					szStart.append(L": ");
					setYieldChangeHelp(szBuffer, szStart, L"", L"",
							kBuilding.getBonusYieldModifierArray(eBonus), true);
					break;
				}
			}
		}
		// Only construction effects remain
		if(!bCanEverConstruct)
			continue;
		// Stricter check than the one for happiness/health effects above
		if(pCity != NULL && !pCity->canConstruct(eLoopBuilding, false, true, true))
			continue;

		// Highlight building name when it's under construction
		bool bConstructing = false;
		if(pCity != NULL) {
			CLLNode<OrderData>* pOrderNode = pCity->headOrderQueueNode();
			if(pOrderNode != NULL && pOrderNode->m_data.eOrderType == ORDER_CONSTRUCT &&
					pOrderNode->m_data.iData1 == eLoopBuilding)
				bConstructing = true;
		}
		bool bHighlight = (bConstructing ||
				(eActivePlayer != NO_PLAYER && pCity == NULL &&
				/*  I.e. not on the main interface b/c it's too difficult to
					keep those texts updated */
				(eTradePlayer != NO_PLAYER || bDiplo) &&
				pActivePlayer->getBuildingClassMaking(eBuildingClass) > 0));

		// These are unused, but I'd like to use them someday.
		bool bBonusReq = (kBuilding.getPrereqAndBonus() == eBonus);
		if(!bBonusReq) {
			for(int j = 0; j < GC.getNUM_BUILDING_PREREQ_OR_BONUSES(); j++) {
				if(kBuilding.getPrereqOrBonuses(j) == eBonus) {
					bBonusReq = true;
					break;
				}
			}
		}
		if(bBonusReq) {
			CvWString szDescr(kBuilding.getDescription());
			if(bHighlight)
				::applyColorToString(szDescr, "COLOR_HIGHLIGHT_TEXT");
			szBuffer.append(NEWLINE);
			szBuffer.append(gDLL->getText("TXT_KEY_TECH_CAN_CONSTRUCT"));
			szBuffer.append(szDescr);
		}
		if(eActivePlayer == NO_PLAYER)
			continue;
		// Don't show wonder production bonuses for future eras
		if(!bWonder || iTechEraDelta <= 0) {
			int iProductionMod = kBuilding.getBonusProductionModifier(eBonus);
			if(iProductionMod != 0) {
				szBuffer.append(NEWLINE);
				CvWString szDescr(kBuilding.getDescription());
				if(bHighlight)
					::applyColorToString(szDescr, "COLOR_HIGHLIGHT_TEXT");
				szBuffer.append(CvWString::format(L"%c%s%d%% %s",
						gDLL->getSymbolID(BULLET_CHAR),
						iProductionMod > 0 ? "+" : "", iProductionMod,
						gDLL->getText("TXT_KEY_BONUS_SPEED_FOR", szDescr.GetCString()).GetCString()));
			}
		}
	}
	if(bCivilopediaText)
		return;
	if(eActivePlayer != NO_PLAYER) {
		// Based on the loop above
		for(int i = 0; i < GC.getNumProjectInfos(); i++) {
			ProjectTypes eLoopProject = (ProjectTypes)i;
			CvProjectInfo& kProject = GC.getProjectInfo(eLoopProject);
			int iProductionMod = kProject.getBonusProductionModifier(eBonus);
			if(iProductionMod == 0)
				continue;
			if(pCity == NULL) {
				TechTypes eTech = (TechTypes)kProject.getTechPrereq();
				if(eTech != NO_TECH && GC.getTechInfo(eTech).getEra() > iCurrentEra
						+ (::isWorldProject(eLoopProject) ? 0 : 1))
					continue;
				VictoryTypes eVict = (VictoryTypes)kProject.getVictoryPrereq();
				if(eVict != NO_VICTORY && !g.isVictoryValid(eVict))
					continue;
				if(g.isProjectMaxedOut(eLoopProject) && TEAMREF(eActivePlayer).
						getProjectCount(eLoopProject) <= 0)
					continue;
				if(kProject.isSpaceship() && (pActivePlayer->getCapitalCity() == NULL ||
						!pActivePlayer->getCapitalCity()->canCreate(eLoopProject, false, true)))
					continue;
			}
			else if(!pCity->canCreate(eLoopProject, false, true))
				continue;
			// Copy-paste from the building loop
			szBuffer.append(NEWLINE);
			szBuffer.append(CvWString::format(L"%c%s%d%% %s",
					gDLL->getSymbolID(BULLET_CHAR),
					iProductionMod > 0 ? "+" : "",
					iProductionMod, gDLL->getText("TXT_KEY_BONUS_SPEED_FOR",
					kProject.getDescription()).GetCString()));
		}
		// To weed out obsolete units
		CvCity* pTrainCity = pCity;
		if(pTrainCity == NULL)
			pTrainCity = pActivePlayer->getCapitalCity();
		std::vector<CvUnitInfo*> aEnables;
		for(int i = 0; i < GC.getNumUnitClassInfos(); i++) {
			UnitClassTypes eUnitClass = (UnitClassTypes)i;
			UnitTypes eLoopUnit = (UnitTypes)(GC.getCivilizationInfo(
					eCivilization).getCivilizationUnits(i));
			if(eLoopUnit == NO_UNIT)
				continue;
			CvUnitInfo& kUnit = GC.getUnitInfo(eLoopUnit);
			// <advc.905b>
			int iSpeed = 0;
			for(int j = 0; j < GC.getNUM_UNIT_PREREQ_OR_BONUSES(); j++) {
				if(kUnit.getSpeedBonuses(j) != eBonus)
					continue;
				iSpeed = kUnit.getExtraMoves(j);
			} // </advc.905b>
			bool bBonusReq = (kUnit.getPrereqAndBonus() == eBonus);
			if(!bBonusReq) {
				for(int j = 0; j < GC.getNUM_UNIT_PREREQ_OR_BONUSES(); j++) {
					if(kUnit.getPrereqOrBonuses(j) == eBonus) {
						bBonusReq = true;
						break;
					}
				}
			}
			if(!bBonusReq && iSpeed == 0)
				continue;
			if(pTrainCity == NULL) {
				if(!pActivePlayer->canTrain(eLoopUnit, false, true, true))
					continue;
			}
			else if(!pTrainCity->canTrain(eLoopUnit, false, true, true, false, false))
				continue;
			if(bBonusReq)
				aEnables.push_back(&kUnit);
			if(iSpeed != 0) {
				szBuffer.append(NEWLINE);
				szBuffer.append(CvWString::format(L"%c%s%d%c: ",
						gDLL->getSymbolID(BULLET_CHAR), iSpeed > 0 ? "+" : "",
						iSpeed, gDLL->getSymbolID(MOVES_CHAR)));
				szBuffer.append(kUnit.getDescription());
			}
		}
		/*  The capital can be NULL and it's not necessarily going to be coastal.
			Address these cases through an era threshold (b/c checking upgrade paths
			without a CvCity object is too complicated). */
		int iEraThresh = std::max(0, iCurrentEra - 3);
		int iCount = aEnables.size();
		while(iCount > 5 && iEraThresh < iCurrentEra) {
			iCount = aEnables.size();
			for(size_t i = 0; i < aEnables.size(); i++) {
				TechTypes eTech = (TechTypes)aEnables[i]->getPrereqAndTech();
				if(eTech != NO_TECH && GC.getTechInfo(eTech).getEra() <= iEraThresh)
					iCount--;
			}
			iEraThresh++;
		}
		if(iCount <= 0) // Show too many units rather than none
			iEraThresh = 0;
		for(size_t i = 0; i < aEnables.size(); i++) {
			TechTypes eTech = (TechTypes)aEnables[i]->getPrereqAndTech();
			bool bSea = (DOMAIN_SEA == (DomainTypes)aEnables[i]->getDomainType());
			bool bUseEraThresh = (eTech != NO_TECH && (pTrainCity == NULL ||
					(bSea != pTrainCity->isCoastal())));
			if(!bUseEraThresh || GC.getTechInfo(eTech).getEra() >= iEraThresh) {
				szBuffer.append(NEWLINE);
				szBuffer.append(gDLL->getText("TXT_KEY_TECH_CAN_TRAIN"));
				szBuffer.append(aEnables[i]->getDescription());
			}
		}
	} // </advc.004w>
	if (!CvWString(GC.getBonusInfo(eBonus).getHelp()).empty()) {
		szBuffer.append(NEWLINE);
		szBuffer.append(GC.getBonusInfo(eBonus).getHelp());
	}
}


void CvGameTextMgr::setReligionHelp(CvWStringBuffer &szBuffer, ReligionTypes eReligion, bool bCivilopedia)
{
	UnitTypes eFreeUnit;

	if (NO_RELIGION == eReligion)
	{
		return;
	}
	CvReligionInfo& religion = GC.getReligionInfo(eReligion);

	if (!bCivilopedia)
	{
		szBuffer.append(CvWString::format(SETCOLR L"%s" ENDCOLR , TEXT_COLOR("COLOR_HIGHLIGHT_TEXT"), religion.getDescription()));
	}

	setCommerceChangeHelp(szBuffer, gDLL->getText("TXT_KEY_RELIGION_HOLY_CITY").c_str(), L": ", L"", religion.getHolyCityCommerceArray());
	setCommerceChangeHelp(szBuffer, gDLL->getText("TXT_KEY_RELIGION_ALL_CITIES").c_str(), L": ", L"", religion.getStateReligionCommerceArray());

	if (!bCivilopedia)
	{
		if (religion.getTechPrereq() != NO_TECH)
		{
			szBuffer.append(NEWLINE);
			szBuffer.append(gDLL->getText("TXT_KEY_RELIGION_FOUNDED_FIRST", GC.getTechInfo((TechTypes)religion.getTechPrereq()).getTextKeyWide()));
		}
	}

	if (religion.getFreeUnitClass() != NO_UNITCLASS)
	{
		if (GC.getGame().getActivePlayer() != NO_PLAYER)
		{
			eFreeUnit = ((UnitTypes)(GC.getCivilizationInfo(GET_PLAYER(GC.getGame().getActivePlayer()).getCivilizationType()).getCivilizationUnits(religion.getFreeUnitClass())));
		}
		else
		{
			eFreeUnit = (UnitTypes)GC.getUnitClassInfo((UnitClassTypes)religion.getFreeUnitClass()).getDefaultUnitIndex();
		}

		if (eFreeUnit != NO_UNIT)
		{
			if (religion.getNumFreeUnits() > 1)
			{
				szBuffer.append(NEWLINE);
				szBuffer.append(gDLL->getText("TXT_KEY_RELIGION_FOUNDER_RECEIVES_NUM", GC.getUnitInfo(eFreeUnit).getTextKeyWide(), religion.getNumFreeUnits()));
			}
			else if (religion.getNumFreeUnits() > 0)
			{
				szBuffer.append(NEWLINE);
				szBuffer.append(gDLL->getText("TXT_KEY_RELIGION_FOUNDER_RECEIVES", GC.getUnitInfo(eFreeUnit).getTextKeyWide()));
			}
		}
	}
}

void CvGameTextMgr::setReligionHelpCity(CvWStringBuffer &szBuffer, ReligionTypes eReligion, CvCity *pCity, bool bCityScreen, bool bForceReligion, bool bForceState, bool bNoStateReligion)
{
	if(pCity == NULL)
		return;

	CvWString szTempBuffer;

	ReligionTypes eStateReligion = (bNoStateReligion ? NO_RELIGION : GET_PLAYER(pCity->getOwner()).getStateReligion());

	if (bCityScreen)
	{
		szBuffer.append(CvWString::format(SETCOLR L"%s" ENDCOLR , TEXT_COLOR("COLOR_HIGHLIGHT_TEXT"), GC.getReligionInfo(eReligion).getDescription()));
		szBuffer.append(NEWLINE);

		if (!(GC.getGame().isReligionFounded(eReligion)) && !GC.getGame().isOption(GAMEOPTION_PICK_RELIGION))
		{
			if (GC.getReligionInfo(eReligion).getTechPrereq() != NO_TECH)
			{
				szBuffer.append(gDLL->getText("TXT_KEY_RELIGION_FOUNDED_FIRST", GC.getTechInfo((TechTypes)(GC.getReligionInfo(eReligion).getTechPrereq())).getTextKeyWide()));
			}
		}
	}

	// K-Mod
	if (GC.getGame().isReligionFounded(eReligion) && pCity &&
			//gDLL->getChtLvl() > 0
			GC.getGame().isDebugMode() // advc.135c
			&& GC.ctrlKey())
	{
		szBuffer.append(CvWString::format(L"grip: %d", pCity->getReligionGrip(eReligion)));
		szBuffer.append(NEWLINE);
	}
	// K-Mod end

	if (!bForceReligion)
	{
		if (!(pCity->isHasReligion(eReligion)))
		{
			return;
		}
	}
	bool bHandled = false;
	if (eStateReligion == eReligion || eStateReligion == NO_RELIGION || bForceState)
	{
		for (int i = 0; i < NUM_COMMERCE_TYPES; i++)
		{
			int iCommerce = GC.getReligionInfo(eReligion).getStateReligionCommerce((CommerceTypes)i);

			if (pCity->isHolyCity(eReligion))
			{
				iCommerce += GC.getReligionInfo(eReligion).getHolyCityCommerce((CommerceTypes)i);
			}

			if (iCommerce != 0)
			{
				if (bHandled)
				{
					szBuffer.append(L", ");
				}

				szTempBuffer.Format(L"%s%d%c", iCommerce > 0 ? "+" : "", iCommerce, GC.getCommerceInfo((CommerceTypes)i).getChar());
				szBuffer.append(szTempBuffer);
				bHandled = true;
			}
		}
	}

	if (eStateReligion == eReligion || bForceState)
	{
		int iHappiness = (pCity->getStateReligionHappiness(eReligion) + GET_PLAYER(pCity->getOwner()).getStateReligionHappiness());

		if (iHappiness != 0)
		{
			if (bHandled)
				szBuffer.append(L", ");
			szTempBuffer.Format(L"%d%c",
					// UNOFFICIAL_PATCH, Bugfix, 08/28/09, jdog5000: Use absolute value with unhappy face
					abs(iHappiness), ((iHappiness > 0) ? gDLL->getSymbolID(HAPPY_CHAR) : gDLL->getSymbolID(UNHAPPY_CHAR)));
			szBuffer.append(szTempBuffer);
			bHandled = true;
		}

		int iProductionModifier = GET_PLAYER(pCity->getOwner()).getStateReligionBuildingProductionModifier();
		if (iProductionModifier != 0)
		{
			if (bHandled)
			{
				szBuffer.append(L", ");
			}

			szBuffer.append(gDLL->getText("TXT_KEY_RELIGION_BUILDING_PROD_MOD", iProductionModifier));
			bHandled = true;
		}

		iProductionModifier = GET_PLAYER(pCity->getOwner()).getStateReligionUnitProductionModifier();
		if (iProductionModifier != 0)
		{
			if (bHandled)
			{
				szBuffer.append(L", ");
			}

			szBuffer.append(gDLL->getText("TXT_KEY_RELIGION_UNIT_PROD_MOD", iProductionModifier));
			bHandled = true;
		}

		int iFreeExperience = GET_PLAYER(pCity->getOwner()).getStateReligionFreeExperience();
		if (iFreeExperience != 0)
		{
			if (bHandled)
			{
				szBuffer.append(L", ");
			}

			szBuffer.append(gDLL->getText("TXT_KEY_RELIGION_FREE_XP", iFreeExperience));
			bHandled = true;
		}

		int iGreatPeopleRateModifier = GET_PLAYER(pCity->getOwner()).getStateReligionGreatPeopleRateModifier();
		if (iGreatPeopleRateModifier != 0)
		{
			if (bHandled)
			{
				szBuffer.append(L", ");
			}

			szBuffer.append(gDLL->getText("TXT_KEY_RELIGION_BIRTH_RATE_MOD", iGreatPeopleRateModifier));
			bHandled = true;
		}
	}
}

void CvGameTextMgr::setCorporationHelp(CvWStringBuffer &szBuffer, CorporationTypes eCorporation, bool bCivilopedia)
{
	UnitTypes eFreeUnit;
	CvWString szTempBuffer;

	if (NO_CORPORATION == eCorporation)
	{
		return;
	}
	CvCorporationInfo& kCorporation = GC.getCorporationInfo(eCorporation);

	if (!bCivilopedia)
	{
		szBuffer.append(CvWString::format(SETCOLR L"%s" ENDCOLR , TEXT_COLOR("COLOR_HIGHLIGHT_TEXT"), kCorporation.getDescription()));
	}

	szTempBuffer.clear();
	for (int iI = 0; iI < NUM_YIELD_TYPES; ++iI)
	{
		int iYieldProduced = GC.getCorporationInfo(eCorporation).getYieldProduced((YieldTypes)iI);
		if (NO_PLAYER != GC.getGame().getActivePlayer())
		{
			iYieldProduced *= GC.getWorldInfo(GC.getMap().getWorldSize()).getCorporationMaintenancePercent();
			iYieldProduced /= 100;
		}

		if (iYieldProduced != 0)
		{
			if (!szTempBuffer.empty())
			{
				szTempBuffer += L", ";
			}

			if (iYieldProduced % 100 == 0)
			{
				szTempBuffer += CvWString::format(L"%s%d%c",
					iYieldProduced > 0 ? L"+" : L"",
					iYieldProduced / 100,
					GC.getYieldInfo((YieldTypes)iI).getChar());
			}
			else
			{
				szTempBuffer += CvWString::format(L"%s%.2f%c",
					iYieldProduced > 0 ? L"+" : L"",
					0.01f * abs(iYieldProduced),
					GC.getYieldInfo((YieldTypes)iI).getChar());
			}
		}
	}

	if (!szTempBuffer.empty())
	{
		szBuffer.append(NEWLINE);
		szBuffer.append(gDLL->getText("TXT_KEY_CORPORATION_ALL_CITIES", szTempBuffer.GetCString()));
	}

	szTempBuffer.clear();
	for (int iI = 0; iI < NUM_COMMERCE_TYPES; ++iI)
	{
		int iCommerceProduced = GC.getCorporationInfo(eCorporation).getCommerceProduced((CommerceTypes)iI);
		if (NO_PLAYER != GC.getGame().getActivePlayer())
		{
			iCommerceProduced *= GC.getWorldInfo(GC.getMap().getWorldSize()).getCorporationMaintenancePercent();
			iCommerceProduced /= 100;
		}
		if (iCommerceProduced != 0)
		{
			if (!szTempBuffer.empty())
			{
				szTempBuffer += L", ";
			}

			if (iCommerceProduced % 100 == 0)
			{
				szTempBuffer += CvWString::format(L"%s%d%c",
					iCommerceProduced > 0 ? L"+" : L"",
					iCommerceProduced / 100,
					GC.getCommerceInfo((CommerceTypes)iI).getChar());
			}
			else
			{
				szTempBuffer += CvWString::format(L"%s%.2f%c",
					iCommerceProduced > 0 ? L"+" : L"",
					0.01f * abs(iCommerceProduced),
					GC.getCommerceInfo((CommerceTypes)iI).getChar());
			}

		}
	}

	if (!szTempBuffer.empty())
	{
		szBuffer.append(NEWLINE);
		szBuffer.append(gDLL->getText("TXT_KEY_CORPORATION_ALL_CITIES", szTempBuffer.GetCString()));
	}

	if (!bCivilopedia)
	{
		if (kCorporation.getTechPrereq() != NO_TECH)
		{
			szBuffer.append(NEWLINE);
			szBuffer.append(gDLL->getText("TXT_KEY_CORPORATION_FOUNDED_FIRST", GC.getTechInfo((TechTypes)kCorporation.getTechPrereq()).getTextKeyWide()));
		}
	}

	szBuffer.append(NEWLINE);
	szBuffer.append(gDLL->getText("TXT_KEY_CORPORATION_BONUS_REQUIRED"));
	bool bFirst = true;
	for (int i = 0; i < GC.getNUM_CORPORATION_PREREQ_BONUSES(); ++i)
	{
		if (NO_BONUS != kCorporation.getPrereqBonus(i))
		{
			if (bFirst)
			{
				bFirst = false;
			}
			else
			{
				szBuffer.append(L", ");
			}

			szBuffer.append(CvWString::format(L"%c", GC.getBonusInfo((BonusTypes)kCorporation.getPrereqBonus(i)).getChar()));
		}
	}

	if (kCorporation.getBonusProduced() != NO_BONUS)
	{
		szBuffer.append(NEWLINE);
		szBuffer.append(gDLL->getText("TXT_KEY_CORPORATION_BONUS_PRODUCED", GC.getBonusInfo((BonusTypes)kCorporation.getBonusProduced()).getChar()));
	}

	if (kCorporation.getFreeUnitClass() != NO_UNITCLASS)
	{
		if (GC.getGame().getActivePlayer() != NO_PLAYER)
		{
			eFreeUnit = ((UnitTypes)(GC.getCivilizationInfo(GET_PLAYER(GC.getGame().getActivePlayer()).getCivilizationType()).getCivilizationUnits(kCorporation.getFreeUnitClass())));
		}
		else
		{
			eFreeUnit = (UnitTypes)GC.getUnitClassInfo((UnitClassTypes)kCorporation.getFreeUnitClass()).getDefaultUnitIndex();
		}

		if (eFreeUnit != NO_UNIT)
		{
			szBuffer.append(NEWLINE);
			szBuffer.append(gDLL->getText("TXT_KEY_RELIGION_FOUNDER_RECEIVES", GC.getUnitInfo(eFreeUnit).getTextKeyWide()));
		}
	}

	std::vector<CorporationTypes> aCompetingCorps;
	bFirst = true;
	for (int iCorporation = 0; iCorporation < GC.getNumCorporationInfos(); ++iCorporation)
	{
		if (iCorporation != eCorporation)
		{
			bool bCompeting = false;

			CvCorporationInfo& kLoopCorporation = GC.getCorporationInfo((CorporationTypes)iCorporation);
			for (int i = 0; i < GC.getNUM_CORPORATION_PREREQ_BONUSES(); ++i)
			{
				if (kCorporation.getPrereqBonus(i) != NO_BONUS)
				{
					for (int j = 0; j < GC.getNUM_CORPORATION_PREREQ_BONUSES(); ++j)
					{
						if (kLoopCorporation.getPrereqBonus(j) == kCorporation.getPrereqBonus(i))
						{
							bCompeting = true;
							break;
						}
					}
				}

				if (bCompeting)
				{
					break;
				}
			}

			if (bCompeting)
			{
				CvWString szTemp = CvWString::format(L"<link=literal>%s</link>", kLoopCorporation.getDescription());
				setListHelp(szBuffer, gDLL->getText("TXT_KEY_CORPORATION_COMPETES").c_str(), szTemp.GetCString(), L", ", bFirst);
				bFirst = false;
			}
		}
	}
}

void CvGameTextMgr::setCorporationHelpCity(CvWStringBuffer &szBuffer, CorporationTypes eCorporation, CvCity *pCity, bool bCityScreen, bool bForceCorporation)
{
	if (pCity == NULL)
	{
		return;
	}

	CvCorporationInfo& kCorporation = GC.getCorporationInfo(eCorporation);

	if (bCityScreen)
	{
		szBuffer.append(CvWString::format(SETCOLR L"%s" ENDCOLR , TEXT_COLOR("COLOR_HIGHLIGHT_TEXT"), kCorporation.getDescription()));
		szBuffer.append(NEWLINE);

		if (!(GC.getGame().isCorporationFounded(eCorporation)))
		{
			if (GC.getCorporationInfo(eCorporation).getTechPrereq() != NO_TECH)
			{
				szBuffer.append(gDLL->getText("TXT_KEY_CORPORATION_FOUNDED_FIRST", GC.getTechInfo((TechTypes)(kCorporation.getTechPrereq())).getTextKeyWide()));
			}
		}
	}

	if (!bForceCorporation)
	{
		if (!(pCity->isHasCorporation(eCorporation)))
		{
			return;
		}
	}

	int iNumResources = 0;
	for (int i = 0; i < GC.getNUM_CORPORATION_PREREQ_BONUSES(); ++i)
	{
		BonusTypes eBonus = (BonusTypes)kCorporation.getPrereqBonus(i);
		if (NO_BONUS != eBonus)
		{
			iNumResources += pCity->getNumBonuses(eBonus);
		}
	}

	bool bActive = (pCity->isActiveCorporation(eCorporation) || (bForceCorporation && iNumResources > 0));

	bool bHandled = false;
	for (int i = 0; i < NUM_YIELD_TYPES; ++i)
	{
		int iYield = 0;

		if (bActive)
		{
			iYield += (kCorporation.getYieldProduced(i) * iNumResources * GC.getWorldInfo(GC.getMap().getWorldSize()).getCorporationMaintenancePercent()) / 100;
		}

		if (iYield != 0)
		{
			if (bHandled)
			{
				szBuffer.append(L", ");
			}

			CvWString szTempBuffer;
			szTempBuffer.Format(L"%s%d%c", iYield > 0 ? "+" : "", (iYield + 99) / 100, GC.getYieldInfo((YieldTypes)i).getChar());
			szBuffer.append(szTempBuffer);
			bHandled = true;
		}
	}

	bHandled = false;
	for (int i = 0; i < NUM_COMMERCE_TYPES; ++i)
	{
		int iCommerce = 0;

		if (bActive)
		{
			iCommerce += (kCorporation.getCommerceProduced(i) * iNumResources * GC.getWorldInfo(GC.getMap().getWorldSize()).getCorporationMaintenancePercent()) / 100;
		}

		if (iCommerce != 0)
		{
			if (bHandled)
			{
				szBuffer.append(L", ");
			}

			CvWString szTempBuffer;
			szTempBuffer.Format(L"%s%d%c", iCommerce > 0 ? "+" : "", (iCommerce + 99) / 100, GC.getCommerceInfo((CommerceTypes)i).getChar());
			szBuffer.append(szTempBuffer);
			bHandled = true;
		}
	}

	int iMaintenance = 0;

	if (bActive)
	{
		iMaintenance += pCity->calculateCorporationMaintenanceTimes100(eCorporation);
		iMaintenance *= 100 + pCity->getMaintenanceModifier();
		iMaintenance /= 100;
		iMaintenance = (100+GET_PLAYER(pCity->getOwner()).calculateInflationRate())*iMaintenance / 100; // K-Mod
	}

	if (iMaintenance != 0)
	{
		if (bHandled)
		{
			szBuffer.append(L", ");
		}

		CvWString szTempBuffer;
		szTempBuffer.Format(L"%d%c", -iMaintenance / 100, GC.getCommerceInfo(COMMERCE_GOLD).getChar());
		szBuffer.append(szTempBuffer);
		bHandled = true;
	}

	if (bCityScreen)
	{
		szBuffer.append(NEWLINE);
		szBuffer.append(gDLL->getText("TXT_KEY_CORPORATION_BONUS_REQUIRED"));
		bool bFirst = true;
		for (int i = 0; i < GC.getNUM_CORPORATION_PREREQ_BONUSES(); ++i)
		{
			if (NO_BONUS != kCorporation.getPrereqBonus(i))
			{
				if (bFirst)
				{
					bFirst = false;
				}
				else
				{
					szBuffer.append(L", ");
				}

				szBuffer.append(CvWString::format(L"%c", GC.getBonusInfo((BonusTypes)kCorporation.getPrereqBonus(i)).getChar()));
			}
		}

		if (bActive)
		{
			if (kCorporation.getBonusProduced() != NO_BONUS)
			{
				szBuffer.append(NEWLINE);
				szBuffer.append(gDLL->getText("TXT_KEY_CORPORATION_BONUS_PRODUCED", GC.getBonusInfo((BonusTypes)kCorporation.getBonusProduced()).getChar()));
			}
		}
	}
	else
	{
		if (kCorporation.getBonusProduced() != NO_BONUS)
		{
			if (bActive)
			{
				if (bHandled)
				{
					szBuffer.append(L", ");
				}

				szBuffer.append(CvWString::format(L"%c", GC.getBonusInfo((BonusTypes)kCorporation.getBonusProduced()).getChar()));
			}
		}
	}
}

void CvGameTextMgr::buildObsoleteString(CvWStringBuffer &szBuffer, int iItem, bool bList, bool bPlayerContext)
{
	CvWString szTempBuffer;

	if (bList)
	{
		szBuffer.append(NEWLINE);
	}
	szBuffer.append(gDLL->getText("TXT_KEY_TECH_OBSOLETES", GC.getBuildingInfo((BuildingTypes) iItem).getTextKeyWide()));
}

void CvGameTextMgr::buildObsoleteBonusString(CvWStringBuffer &szBuffer, int iItem, bool bList, bool bPlayerContext)
{
	CvWString szTempBuffer;

	if (bList)
	{
		szBuffer.append(NEWLINE);
	}
	szBuffer.append(gDLL->getText("TXT_KEY_TECH_OBSOLETES", GC.getBonusInfo((BonusTypes) iItem).getTextKeyWide()));
}

void CvGameTextMgr::buildObsoleteSpecialString(CvWStringBuffer &szBuffer, int iItem, bool bList, bool bPlayerContext)
{
	CvWString szTempBuffer;

	if (bList)
	{
		szBuffer.append(NEWLINE);
	}
	szBuffer.append(gDLL->getText("TXT_KEY_TECH_OBSOLETES_NO_LINK", GC.getSpecialBuildingInfo((SpecialBuildingTypes) iItem).getTextKeyWide()));
}

void CvGameTextMgr::buildMoveString(CvWStringBuffer &szBuffer, TechTypes eTech, bool bList, bool bPlayerContext)
{
	for (int iI = 0; iI < GC.getNumRouteInfos(); ++iI)
	{
		int iMoveDiff = ((GC.getMOVE_DENOMINATOR() / std::max(1, (GC.getRouteInfo((RouteTypes) iI).getMovementCost() + ((bPlayerContext) ? GET_TEAM(GC.getGame().getActiveTeam()).getRouteChange((RouteTypes)iI) : 0)))) - (GC.getMOVE_DENOMINATOR() / std::max(1, (GC.getRouteInfo((RouteTypes) iI).getMovementCost() + ((bPlayerContext) ? GET_TEAM(GC.getGame().getActiveTeam()).getRouteChange((RouteTypes)iI) : 0) + GC.getRouteInfo((RouteTypes) iI).getTechMovementChange(eTech)))));

		if (iMoveDiff != 0)
		{
			if (bList)
			{
				szBuffer.append(NEWLINE);
			}
			szBuffer.append(gDLL->getText("TXT_KEY_UNIT_MOVEMENT", -(iMoveDiff), GC.getRouteInfo((RouteTypes) iI).getTextKeyWide()));
			bList = true;
		}
	}
}

void CvGameTextMgr::buildFreeUnitString(CvWStringBuffer &szBuffer, TechTypes eTech, bool bList, bool bPlayerContext)
{
	UnitTypes eFreeUnit = NO_UNIT;
	if (GC.getGame().getActivePlayer() != NO_PLAYER)
	{
		eFreeUnit = GET_PLAYER(GC.getGame().getActivePlayer()).getTechFreeUnit(eTech);
	}
	else
	{
		if (GC.getTechInfo(eTech).getFirstFreeUnitClass() != NO_UNITCLASS)
		{
			eFreeUnit = (UnitTypes)GC.getUnitClassInfo((UnitClassTypes)GC.getTechInfo(eTech).getFirstFreeUnitClass()).getDefaultUnitIndex();
		}
	}

	if (eFreeUnit != NO_UNIT)
	{
		if (!bPlayerContext || (GC.getGame().countKnownTechNumTeams(eTech) == 0))
		{
			if (bList)
			{
				szBuffer.append(NEWLINE);
			}
			szBuffer.append(gDLL->getText("TXT_KEY_TECH_FIRST_RECEIVES", GC.getUnitInfo(eFreeUnit).getTextKeyWide()));
		}
	}
}

void CvGameTextMgr::buildFeatureProductionString(CvWStringBuffer &szBuffer, TechTypes eTech, bool bList, bool bPlayerContext)
{
	if (GC.getTechInfo(eTech).getFeatureProductionModifier() != 0)
	{
		if (bList)
		{
			szBuffer.append(NEWLINE);
		}
		szBuffer.append(gDLL->getText("TXT_KEY_FEATURE_PRODUCTION_MODIFIER", GC.getTechInfo(eTech).getFeatureProductionModifier()));
	}
}

void CvGameTextMgr::buildWorkerRateString(CvWStringBuffer &szBuffer, TechTypes eTech, bool bList, bool bPlayerContext)
{
	if (GC.getTechInfo(eTech).getWorkerSpeedModifier() != 0)
	{
		if (bList)
		{
			szBuffer.append(NEWLINE);
		}
		szBuffer.append(gDLL->getText("TXT_KEY_UNIT_WORKERS_FASTER", GC.getTechInfo(eTech).getWorkerSpeedModifier()));
	}
}

void CvGameTextMgr::buildTradeRouteString(CvWStringBuffer &szBuffer, TechTypes eTech, bool bList, bool bPlayerContext)
{
	if (GC.getTechInfo(eTech).getTradeRoutes() != 0)
	{
		if (bList)
		{
			szBuffer.append(NEWLINE);
		}
		szBuffer.append(gDLL->getText("TXT_KEY_MISC_TRADE_ROUTES", GC.getTechInfo(eTech).getTradeRoutes()));
	}
}

void CvGameTextMgr::buildHealthRateString(CvWStringBuffer &szBuffer, TechTypes eTech, bool bList, bool bPlayerContext)
{
	if (GC.getTechInfo(eTech).getHealth() != 0)
	{
		if (bList)
		{
			szBuffer.append(NEWLINE);
		}
		szBuffer.append(gDLL->getText("TXT_KEY_MISC_HEALTH_ALL_CITIES", abs(GC.getTechInfo(eTech).getHealth()), ((GC.getTechInfo(eTech).getHealth() > 0) ? gDLL->getSymbolID(HEALTHY_CHAR): gDLL->getSymbolID(UNHEALTHY_CHAR))));
	}
}

void CvGameTextMgr::buildHappinessRateString(CvWStringBuffer &szBuffer, TechTypes eTech, bool bList, bool bPlayerContext)
{
	if (GC.getTechInfo(eTech).getHappiness() != 0)
	{
		if (bList)
		{
			szBuffer.append(NEWLINE);
		}
		szBuffer.append(gDLL->getText("TXT_KEY_MISC_HAPPINESS_ALL_CITIES", abs(GC.getTechInfo(eTech).getHappiness()), ((GC.getTechInfo(eTech).getHappiness() > 0) ? gDLL->getSymbolID(HAPPY_CHAR): gDLL->getSymbolID(UNHAPPY_CHAR))));
	}
}

void CvGameTextMgr::buildFreeTechString(CvWStringBuffer &szBuffer, TechTypes eTech, bool bList, bool bPlayerContext)
{
	if (GC.getTechInfo(eTech).getFirstFreeTechs() > 0)
	{
		if (!bPlayerContext || (GC.getGame().countKnownTechNumTeams(eTech) == 0))
		{
			if (bList)
			{
				szBuffer.append(NEWLINE);
			}

			if (GC.getTechInfo(eTech).getFirstFreeTechs() == 1)
			{
				szBuffer.append(gDLL->getText("TXT_KEY_TECH_FIRST_FREE_TECH"));
			}
			else
			{
				szBuffer.append(gDLL->getText("TXT_KEY_TECH_FIRST_FREE_TECHS", GC.getTechInfo(eTech).getFirstFreeTechs()));
			}
		}
	}
}

void CvGameTextMgr::buildLOSString(CvWStringBuffer &szBuffer, TechTypes eTech, bool bList, bool bPlayerContext)
{
	if (GC.getTechInfo(eTech).isExtraWaterSeeFrom() && (!bPlayerContext || !(GET_TEAM(GC.getGame().getActiveTeam()).isExtraWaterSeeFrom())))
	{
		if (bList)
		{
			szBuffer.append(NEWLINE);
		}
		szBuffer.append(gDLL->getText("TXT_KEY_UNIT_EXTRA_SIGHT"));
	}
}

void CvGameTextMgr::buildMapCenterString(CvWStringBuffer &szBuffer, TechTypes eTech, bool bList, bool bPlayerContext)
{
	if (GC.getTechInfo(eTech).isMapCentering() && (!bPlayerContext || !(GET_TEAM(GC.getGame().getActiveTeam()).isMapCentering())))
	{
		if (bList)
		{
			szBuffer.append(NEWLINE);
		}
		szBuffer.append(gDLL->getText("TXT_KEY_MISC_CENTERS_MAP"));
	}
}

void CvGameTextMgr::buildMapRevealString(CvWStringBuffer &szBuffer, TechTypes eTech, bool bList)
{
	if (GC.getTechInfo(eTech).isMapVisible())
	{
		if (bList)
		{
			szBuffer.append(NEWLINE);
		}
		szBuffer.append(gDLL->getText("TXT_KEY_MISC_REVEALS_MAP"));
	}
}

void CvGameTextMgr::buildMapTradeString(CvWStringBuffer &szBuffer, TechTypes eTech, bool bList, bool bPlayerContext)
{
	if (GC.getTechInfo(eTech).isMapTrading() && (!bPlayerContext || !(GET_TEAM(GC.getGame().getActiveTeam()).isMapTrading())))
	{
		if (bList)
		{
			szBuffer.append(NEWLINE);
		}
		szBuffer.append(gDLL->getText("TXT_KEY_MISC_ENABLES_MAP_TRADING"));
	}
}

void CvGameTextMgr::buildTechTradeString(CvWStringBuffer &szBuffer, TechTypes eTech, bool bList, bool bPlayerContext)
{
	if (GC.getTechInfo(eTech).isTechTrading() && (!bPlayerContext || !(GET_TEAM(GC.getGame().getActiveTeam()).isTechTrading())))
	{
		if (bList)
		{
			szBuffer.append(NEWLINE);
		}
		szBuffer.append(gDLL->getText("TXT_KEY_MISC_ENABLES_TECH_TRADING"));
	}
}

void CvGameTextMgr::buildGoldTradeString(CvWStringBuffer &szBuffer, TechTypes eTech, bool bList, bool bPlayerContext)
{
	if (GC.getTechInfo(eTech).isGoldTrading() && (!bPlayerContext || !(GET_TEAM(GC.getGame().getActiveTeam()).isGoldTrading())))
	{
		if (bList)
		{
			szBuffer.append(NEWLINE);
		}
		szBuffer.append(gDLL->getText("TXT_KEY_MISC_ENABLES_GOLD_TRADING"));
	}
}

void CvGameTextMgr::buildOpenBordersString(CvWStringBuffer &szBuffer, TechTypes eTech, bool bList, bool bPlayerContext)
{
	if (GC.getTechInfo(eTech).isOpenBordersTrading() && (!bPlayerContext || !(GET_TEAM(GC.getGame().getActiveTeam()).isOpenBordersTrading())))
	{
		if (bList)
		{
			szBuffer.append(NEWLINE);
		}
		szBuffer.append(gDLL->getText("TXT_KEY_MISC_ENABLES_OPEN_BORDERS"));
	}
}

void CvGameTextMgr::buildDefensivePactString(CvWStringBuffer &szBuffer, TechTypes eTech, bool bList, bool bPlayerContext)
{
	if (GC.getTechInfo(eTech).isDefensivePactTrading() && (!bPlayerContext || !(GET_TEAM(GC.getGame().getActiveTeam()).isDefensivePactTrading())))
	{
		if (bList)
		{
			szBuffer.append(NEWLINE);
		}
		szBuffer.append(gDLL->getText("TXT_KEY_MISC_ENABLES_DEFENSIVE_PACTS"));
	}
}

void CvGameTextMgr::buildPermanentAllianceString(CvWStringBuffer &szBuffer, TechTypes eTech, bool bList, bool bPlayerContext)
{
	if (GC.getTechInfo(eTech).isPermanentAllianceTrading() && (!bPlayerContext || (!(GET_TEAM(GC.getGame().getActiveTeam()).isPermanentAllianceTrading()) && GC.getGame().isOption(GAMEOPTION_PERMANENT_ALLIANCES))))
	{
		if (bList)
		{
			szBuffer.append(NEWLINE);
		}
		szBuffer.append(gDLL->getText("TXT_KEY_MISC_ENABLES_PERM_ALLIANCES"));
	}
}

void CvGameTextMgr::buildVassalStateString(CvWStringBuffer &szBuffer, TechTypes eTech, bool bList, bool bPlayerContext)
{
	if (GC.getTechInfo(eTech).isVassalStateTrading() && (!bPlayerContext || (!(GET_TEAM(GC.getGame().getActiveTeam()).isVassalStateTrading()) && GC.getGame().isOption(GAMEOPTION_NO_VASSAL_STATES))))
	{
		if (bList)
		{
			szBuffer.append(NEWLINE);
		}
		szBuffer.append(gDLL->getText("TXT_KEY_MISC_ENABLES_VASSAL_STATES"));
	}
}

void CvGameTextMgr::buildBridgeString(CvWStringBuffer &szBuffer, TechTypes eTech, bool bList, bool bPlayerContext)
{
	if (GC.getTechInfo(eTech).isBridgeBuilding() && (!bPlayerContext || !(GET_TEAM(GC.getGame().getActiveTeam()).isBridgeBuilding())))
	{
		if (bList)
		{
			szBuffer.append(NEWLINE);
		}
		szBuffer.append(gDLL->getText("TXT_KEY_MISC_ENABLES_BRIDGE_BUILDING"));
	}
}

void CvGameTextMgr::buildIrrigationString(CvWStringBuffer &szBuffer, TechTypes eTech, bool bList, bool bPlayerContext)
{
	if (GC.getTechInfo(eTech).isIrrigation() && (!bPlayerContext || !(GET_TEAM(GC.getGame().getActiveTeam()).isIrrigation())))
	{
		if (bList)
		{
			szBuffer.append(NEWLINE);
		}
		szBuffer.append(gDLL->getText("TXT_KEY_MISC_SPREAD_IRRIGATION"));
	}
}

void CvGameTextMgr::buildIgnoreIrrigationString(CvWStringBuffer &szBuffer, TechTypes eTech, bool bList, bool bPlayerContext)
{
	if (GC.getTechInfo(eTech).isIgnoreIrrigation() && (!bPlayerContext || !(GET_TEAM(GC.getGame().getActiveTeam()).isIgnoreIrrigation())))
	{
		if (bList)
		{
			szBuffer.append(NEWLINE);
		}
		szBuffer.append(gDLL->getText("TXT_KEY_MISC_IRRIGATION_ANYWHERE"));
	}
}

void CvGameTextMgr::buildWaterWorkString(CvWStringBuffer &szBuffer, TechTypes eTech, bool bList, bool bPlayerContext)
{
	if (GC.getTechInfo(eTech).isWaterWork() && (!bPlayerContext || !(GET_TEAM(GC.getGame().getActiveTeam()).isWaterWork())))
	{
		if (bList)
		{
			szBuffer.append(NEWLINE);
		}
		szBuffer.append(gDLL->getText("TXT_KEY_MISC_WATER_WORK"));
	}
}

void CvGameTextMgr::buildImprovementString(CvWStringBuffer &szBuffer, TechTypes eTech, int iImprovement, bool bList, bool bPlayerContext)
{
	bool bTechFound = false;

	if (GC.getBuildInfo((BuildTypes) iImprovement).getTechPrereq() == NO_TECH)
	{
		for (int iJ = 0; iJ < GC.getNumFeatureInfos(); iJ++)
		{
			if (GC.getBuildInfo((BuildTypes) iImprovement).getFeatureTech(iJ) == eTech)
			{
				bTechFound = true;
			}
		}
	}
	else
	{
		if (GC.getBuildInfo((BuildTypes) iImprovement).getTechPrereq() == eTech)
		{
			bTechFound = true;
		}
	}

	if (bTechFound)
	{
		if (bList)
		{
			szBuffer.append(NEWLINE);
		}

		szBuffer.append(gDLL->getText("TXT_KEY_MISC_CAN_BUILD_IMPROVEMENT", GC.getBuildInfo((BuildTypes) iImprovement).getTextKeyWide()));
	}
}

void CvGameTextMgr::buildDomainExtraMovesString(CvWStringBuffer &szBuffer, TechTypes eTech, int iDomainType, bool bList, bool bPlayerContext)
{
	if (GC.getTechInfo(eTech).getDomainExtraMoves(iDomainType) != 0)
	{
		if (bList)
		{
			szBuffer.append(NEWLINE);
		}

		szBuffer.append(gDLL->getText("TXT_KEY_MISC_EXTRA_MOVES", GC.getTechInfo(eTech).getDomainExtraMoves(iDomainType), GC.getDomainInfo((DomainTypes)iDomainType).getTextKeyWide()));
	}
}

void CvGameTextMgr::buildAdjustString(CvWStringBuffer &szBuffer, TechTypes eTech, int iCommerceType, bool bList, bool bPlayerContext)
{
	if (GC.getTechInfo(eTech).isCommerceFlexible(iCommerceType) && (!bPlayerContext || !(GET_TEAM(GC.getGame().getActiveTeam()).isCommerceFlexible((CommerceTypes)iCommerceType))))
	{
		if (bList)
		{
			szBuffer.append(NEWLINE);
		}
		szBuffer.append(gDLL->getText("TXT_KEY_MISC_ADJUST_COMMERCE_RATE",
				GC.getCommerceInfo((CommerceTypes) iCommerceType).getChar()));
		// <advc.120c>
		if(!bList && iCommerceType == COMMERCE_ESPIONAGE &&
				getBugOptionBOOL("MainInterface__Hide_EspSlider", true))
			szBuffer.append(gDLL->getText("TXT_KEY_MISC_ADJUST_ON_ESPIONGAE_SCREEN"));
		// </advc.120c>
	}
}

void CvGameTextMgr::buildTerrainTradeString(CvWStringBuffer &szBuffer, TechTypes eTech, int iTerrainType, bool bList, bool bPlayerContext)
{
	if (GC.getTechInfo(eTech).isTerrainTrade(iTerrainType) && (!bPlayerContext || !(GET_TEAM(GC.getGame().getActiveTeam()).isTerrainTrade((TerrainTypes)iTerrainType))))
	{
		if (bList)
		{
			szBuffer.append(NEWLINE);
		}
		szBuffer.append(gDLL->getText("TXT_KEY_MISC_ENABLES_ON_TERRAIN", gDLL->getSymbolID(TRADE_CHAR), GC.getTerrainInfo((TerrainTypes) iTerrainType).getTextKeyWide()));
	}
}

void CvGameTextMgr::buildRiverTradeString(CvWStringBuffer &szBuffer, TechTypes eTech, bool bList, bool bPlayerContext)
{
	if (GC.getTechInfo(eTech).isRiverTrade() && (!bPlayerContext || !(GET_TEAM(GC.getGame().getActiveTeam()).isRiverTrade())))
	{
		if (bList)
		{
			szBuffer.append(NEWLINE);
		}
		szBuffer.append(gDLL->getText("TXT_KEY_MISC_ENABLES_ON_TERRAIN", gDLL->getSymbolID(TRADE_CHAR), gDLL->getText("TXT_KEY_MISC_RIVERS").GetCString()));
	}
}

void CvGameTextMgr::buildSpecialBuildingString(CvWStringBuffer &szBuffer, TechTypes eTech, int iBuildingType, bool bList, bool bPlayerContext)
{
	if (GC.getSpecialBuildingInfo((SpecialBuildingTypes)iBuildingType).getTechPrereq() == eTech)
	{
		if (bList)
		{
			szBuffer.append(NEWLINE);
		}
		szBuffer.append(gDLL->getText("TXT_KEY_MISC_CAN_CONSTRUCT_BUILDING", GC.getSpecialBuildingInfo((SpecialBuildingTypes) iBuildingType).getTextKeyWide()));
	}

	if (GC.getSpecialBuildingInfo((SpecialBuildingTypes)iBuildingType).getTechPrereqAnyone() == eTech)
	{
		if (bList)
		{
			szBuffer.append(NEWLINE);
		}
		szBuffer.append(gDLL->getText("TXT_KEY_MISC_CAN_CONSTRUCT_BUILDING_ANYONE", GC.getSpecialBuildingInfo((SpecialBuildingTypes) iBuildingType).getTextKeyWide()));
	}
}

void CvGameTextMgr::buildYieldChangeString(CvWStringBuffer &szBuffer,
		TechTypes eTech, int iYieldType, bool bList, bool bPlayerContext)
{
	CvWString szTempBuffer;
	CvImprovementInfo const& kImprov = GC.getImprovementInfo((ImprovementTypes)iYieldType);
	if (bList)
	{
		szTempBuffer.Format(L"<link=literal>%s</link>", kImprov.getDescription());
	}
	else
	{
		szTempBuffer.Format(L"%c<link=literal>%s</link>",
				gDLL->getSymbolID(BULLET_CHAR), kImprov.getDescription());
	}

	setYieldChangeHelp(szBuffer, szTempBuffer, L": ", L"",
			kImprov.getTechYieldChangesArray(eTech), false, bList);
}

bool CvGameTextMgr::buildBonusRevealString(CvWStringBuffer &szBuffer, TechTypes eTech, int iBonusType, bool bFirst, bool bList, bool bPlayerContext)
{
	CvWString szTempBuffer;

	if (GC.getBonusInfo((BonusTypes) iBonusType).getTechReveal() == eTech)
	{
		if (bList && bFirst)
		{
			szBuffer.append(NEWLINE);
		}
		szTempBuffer.Format( SETCOLR L"<link=literal>%s</link>" ENDCOLR , TEXT_COLOR("COLOR_HIGHLIGHT_TEXT"), GC.getBonusInfo((BonusTypes) iBonusType).getDescription());
		setListHelp(szBuffer, gDLL->getText("TXT_KEY_MISC_REVEALS").c_str(), szTempBuffer, L", ", bFirst);
		bFirst = false;
	}
	return bFirst;
}

bool CvGameTextMgr::buildCivicRevealString(CvWStringBuffer &szBuffer, TechTypes eTech, int iCivicType, bool bFirst, bool bList, bool bPlayerContext)
{
	CvWString szTempBuffer;

	if (GC.getCivicInfo((CivicTypes) iCivicType).getTechPrereq() == eTech)
	{
		if (bList && bFirst)
		{
			szBuffer.append(NEWLINE);
		}
		szTempBuffer.Format( SETCOLR L"<link=literal>%s</link>" ENDCOLR , TEXT_COLOR("COLOR_HIGHLIGHT_TEXT"), GC.getCivicInfo((CivicTypes) iCivicType).getDescription());
		setListHelp(szBuffer, gDLL->getText("TXT_KEY_MISC_ENABLES").c_str(), szTempBuffer, L", ", bFirst);
		bFirst = false;
	}
	return bFirst;
}

bool CvGameTextMgr::buildProcessInfoString(CvWStringBuffer &szBuffer, TechTypes eTech, int iProcessType, bool bFirst, bool bList, bool bPlayerContext)
{
	CvWString szTempBuffer;

	if (GC.getProcessInfo((ProcessTypes) iProcessType).getTechPrereq() == eTech)
	{
		if (bList && bFirst)
		{
			szBuffer.append(NEWLINE);
		}
		szTempBuffer.Format( SETCOLR L"<link=literal>%s</link>" ENDCOLR , TEXT_COLOR("COLOR_HIGHLIGHT_TEXT"), GC.getProcessInfo((ProcessTypes) iProcessType).getDescription());
		setListHelp(szBuffer, gDLL->getText("TXT_KEY_MISC_CAN_BUILD").c_str(), szTempBuffer, L", ", bFirst);
		bFirst = false;
	}
	return bFirst;
}

bool CvGameTextMgr::buildFoundReligionString(CvWStringBuffer &szBuffer, TechTypes eTech, int iReligionType, bool bFirst, bool bList, bool bPlayerContext)
{
	CvWString szTempBuffer;

	if (GC.getReligionInfo((ReligionTypes) iReligionType).getTechPrereq() == eTech)
	{
		if (!bPlayerContext || (GC.getGame().countKnownTechNumTeams(eTech) == 0))
		{
			if (bList && bFirst)
			{
				szBuffer.append(NEWLINE);
			}

			if (GC.getGame().isOption(GAMEOPTION_PICK_RELIGION))
			{
				szTempBuffer = gDLL->getText("TXT_KEY_RELIGION_UNKNOWN");
			}
			else
			{
				szTempBuffer.Format( SETCOLR L"<link=literal>%s</link>" ENDCOLR , TEXT_COLOR("COLOR_HIGHLIGHT_TEXT"), GC.getReligionInfo((ReligionTypes) iReligionType).getDescription());
			}
			setListHelp(szBuffer, gDLL->getText("TXT_KEY_MISC_FIRST_DISCOVER_FOUNDS").c_str(), szTempBuffer, L", ", bFirst);
			bFirst = false;
		}
	}
	return bFirst;
}

bool CvGameTextMgr::buildFoundCorporationString(CvWStringBuffer &szBuffer, TechTypes eTech, int iCorporationType, bool bFirst, bool bList, bool bPlayerContext)
{
	CvWString szTempBuffer;

	if (GC.getCorporationInfo((CorporationTypes) iCorporationType).getTechPrereq() == eTech)
	{
		if (!bPlayerContext || (GC.getGame().countKnownTechNumTeams(eTech) == 0))
		{
			if (bList && bFirst)
			{
				szBuffer.append(NEWLINE);
			}
			szTempBuffer.Format( SETCOLR L"<link=literal>%s</link>" ENDCOLR , TEXT_COLOR("COLOR_HIGHLIGHT_TEXT"), GC.getCorporationInfo((CorporationTypes) iCorporationType).getDescription());
			setListHelp(szBuffer, gDLL->getText("TXT_KEY_MISC_FIRST_DISCOVER_INCORPORATES").c_str(), szTempBuffer, L", ", bFirst);
			bFirst = false;
		}
	}
	return bFirst;
}

bool CvGameTextMgr::buildPromotionString(CvWStringBuffer &szBuffer, TechTypes eTech, int iPromotionType, bool bFirst, bool bList, bool bPlayerContext)
{
	CvWString szTempBuffer;

	if (GC.getPromotionInfo((PromotionTypes) iPromotionType).getTechPrereq() == eTech)
	{
		if (bList && bFirst)
		{
			szBuffer.append(NEWLINE);
		}
		szTempBuffer.Format( SETCOLR L"<link=literal>%s</link>" ENDCOLR , TEXT_COLOR("COLOR_HIGHLIGHT_TEXT"), GC.getPromotionInfo((PromotionTypes) iPromotionType).getDescription());
		setListHelp(szBuffer, gDLL->getText("TXT_KEY_MISC_ENABLES").c_str(), szTempBuffer, L", ", bFirst);
		bFirst = false;
	}
	return bFirst;
}

// <advc.910> Not much BtS code left unchanged here
void CvGameTextMgr::buildSingleLineTechTreeString(CvWStringBuffer &szBuffer,
		TechTypes eTech, bool bPlayerContext) {

	if(eTech == NO_TECH) { // This used to be a BtS comment; the other comments in this function are new.
		FAssertMsg(eTech != NO_TECH, "you need to specify a tech of origin for this method to do anything");
		return;
	}
	/*  Separate containers for techs that eTech
		- will immediately enable,
		- those that have an additional requirement which is yet unmet and
		- (not listed by BtS) those that are merely sped up (OR req. already met).
		Put the tech cost in the first component b/c I want to sort by that. */
	std::vector<std::pair<int,TechTypes> > aieImmediate, aieLater, aieSpeedsUp;
	for(int iI = 0; iI < GC.getNumTechInfos(); iI++) {
		TechTypes eLeadsTo = (TechTypes)iI;
		bool bCanAlreadyResearch = false;
		bool bAlreadyHas = false;
		if(bPlayerContext) {
			PlayerTypes eActivePlayer = GC.getGame().getActivePlayer();
			bAlreadyHas = TEAMREF(eActivePlayer).isHasTech(eLeadsTo);
			bCanAlreadyResearch = (!bAlreadyHas && GET_PLAYER(eActivePlayer).canResearch(eLeadsTo));
		}
		if (bAlreadyHas)
			continue;
		CvTechInfo const& kLeadsTo = GC.getTechInfo(eLeadsTo);
		bool bTechFound = false;
		bool bAlreadyMetOR = false;
		bool bMeetsOR = false;
		bool bAnyOR = false;
		for(int iJ = 0; iJ < GC.getNUM_OR_TECH_PREREQS(); iJ++) {
			TechTypes eOrTech = (TechTypes)kLeadsTo.getPrereqOrTechs(iJ);
			if(eOrTech == NO_TECH)
				continue;
			bAnyOR = true;
			if(eOrTech == eTech) {
				bTechFound = true;
				bMeetsOR = true;
			}
			else if(bPlayerContext) {
				if(eOrTech != NO_TECH && GET_TEAM(GC.getGame().getActiveTeam()).isHasTech(eOrTech))
					bAlreadyMetOR = true;
			}
		}
		if(!bTechFound && !bCanAlreadyResearch) {
			for(int iJ = 0; iJ < GC.getNUM_AND_TECH_PREREQS(); iJ++) {
				if(kLeadsTo.getPrereqAndTechs(iJ) == eTech) {
					bTechFound = true;
					FAssertMsg(!bMeetsOR, "The same tech shouldn't be both AND and OR req.");
					break;
				}
			}
		}
		if(!bTechFound)
			continue;
		bool bImmediate = (bAlreadyMetOR || bMeetsOR || !bAnyOR || !bPlayerContext);
		if(bImmediate && bMeetsOR && bPlayerContext) {
			CvTeam const& kActiveTeam = GET_TEAM(GC.getGame().getActiveTeam());
			for(int iJ = 0; iJ < GC.getNUM_AND_TECH_PREREQS(); iJ++) {
				if(!kActiveTeam.isHasTech((TechTypes)kLeadsTo.getPrereqAndTechs(iJ))) {
					bImmediate = false;
					break;
				}
			}
		}
		std::pair<int,TechTypes> eiLeadsTo = std::make_pair(kLeadsTo.getResearchCost(), eLeadsTo);
		if(bMeetsOR && bAlreadyMetOR)
			aieSpeedsUp.push_back(eiLeadsTo);
		else if(bImmediate)
			aieImmediate.push_back(eiLeadsTo);
		else aieLater.push_back(eiLeadsTo);

	}
	std::sort(aieImmediate.begin(), aieImmediate.end());
	std::sort(aieLater.begin(), aieLater.end());
	std::sort(aieSpeedsUp.begin(), aieSpeedsUp.end());
	CvWString szTempBuffer;
	bool bFirst = true;
	for(size_t i = 0; i < aieImmediate.size(); i++) {
		TechTypes eLeadsTo = aieImmediate[i].second;
		// Mostly as in BtS
		szTempBuffer.Format(SETCOLR L"<link=literal>%s</link>" ENDCOLR, TEXT_COLOR("COLOR_TECH_TEXT"),
				GC.getTechInfo(eLeadsTo).getDescription());
		setListHelp(szBuffer, gDLL->getText("TXT_KEY_MISC_LEADS_TO").c_str(), szTempBuffer, L", ", bFirst);
		bFirst = false;
	}
	bFirst = true;
	for(size_t i = 0; i < aieLater.size(); i++) {
		TechTypes eLeadsTo = aieLater[i].second;
		// No color
		szTempBuffer.Format(L"<link=literal>%s</link>", GC.getTechInfo(eLeadsTo).getDescription());
		setListHelp(szBuffer, gDLL->getText(aieImmediate.empty() ?
				"TXT_KEY_MISC_LATER_LEADS_TO" :
				"TXT_KEY_MISC_AND_LATER").c_str(), szTempBuffer, L", ", bFirst);
		bFirst = false;
	}
	FAssert(GC.getTECH_COST_FIRST_KNOWN_PREREQ_MODIFIER() == 0); // Not accounted for above
	int iSpeedUpPercent = GC.getTECH_COST_KNOWN_PREREQ_MODIFIER();
	if(iSpeedUpPercent <= 0) {
		FAssert(iSpeedUpPercent > 0);
		return;
	}
	bFirst = true;
	for(size_t i = 0; i < aieSpeedsUp.size(); i++) {
		TechTypes eLeadsTo = aieSpeedsUp[i].second;
		szTempBuffer.Format(SETCOLR L"<link=literal>%s</link>" ENDCOLR, TEXT_COLOR("COLOR_TECH_TEXT"),
				GC.getTechInfo(eLeadsTo).getDescription());
		setListHelp(szBuffer, gDLL->getText("TXT_KEY_MISC_SPEEDS_UP").c_str(), szTempBuffer, L", ", bFirst);
		bFirst = false;
	}
	if(!aieSpeedsUp.empty())
		szBuffer.append(gDLL->getText("TXT_KEY_MISC_SPEED_UP_BY_PERCENT", iSpeedUpPercent));
} // </advc.910>

// Information about other prerequisite technologies to eTech besides eFromTech
void CvGameTextMgr::buildTechTreeString(CvWStringBuffer &szBuffer, TechTypes eTech, bool bPlayerContext, TechTypes eFromTech)
{
	CvWString szTempBuffer;	// Formatting

	if (NO_TECH == eTech || NO_TECH == eFromTech)
	{
		return;
	}

	szTempBuffer.Format(SETCOLR L"%s" ENDCOLR, TEXT_COLOR("COLOR_TECH_TEXT"), GC.getTechInfo(eTech).getDescription());
	szBuffer.append(szTempBuffer);

	// Loop through OR prerequisites to make list
	CvWString szOtherOrTechs;
	int nOtherOrTechs = 0;
	bool bOrTechFound = false;
	for (int iJ = 0; iJ < GC.getNUM_OR_TECH_PREREQS(); iJ++)
	{
		TechTypes eTestTech = (TechTypes)GC.getTechInfo(eTech).getPrereqOrTechs(iJ);
		if (eTestTech >= 0)
		{
			bool bTechAlreadyResearched = false;
			if (bPlayerContext)
			{
				bTechAlreadyResearched = GET_TEAM(GC.getGame().getActiveTeam()).isHasTech(eTestTech);
			}
			if (!bTechAlreadyResearched)
			{
				if (eTestTech == eFromTech)
				{
					bOrTechFound = true;
				}
				else
				{
					szTempBuffer.Format( SETCOLR L"%s" ENDCOLR , TEXT_COLOR("COLOR_TECH_TEXT"), GC.getTechInfo(eTestTech).getDescription());
					setListHelp(szOtherOrTechs, L"", szTempBuffer, gDLL->getText("TXT_KEY_OR").c_str(), nOtherOrTechs == 0);
					nOtherOrTechs++;
				}
			}
		}
	}

	// Loop through AND prerequisites to make list
	CvWString szOtherAndTechs;
	int nOtherAndTechs = 0;
	bool bAndTechFound = false;
	for (int iJ = 0; iJ < GC.getNUM_AND_TECH_PREREQS(); iJ++)
	{
		TechTypes eTestTech = (TechTypes)GC.getTechInfo(eTech).getPrereqAndTechs(iJ);
		if (eTestTech >= 0)
		{
			bool bTechAlreadyResearched = false;
			if (bPlayerContext)
			{
				bTechAlreadyResearched = GET_TEAM(GC.getGame().getActiveTeam()).isHasTech(eTestTech);
			}
			if (!bTechAlreadyResearched)
			{
				if (eTestTech == eFromTech)
				{
					bAndTechFound = true;
				}
				else
				{
					szTempBuffer.Format( SETCOLR L"%s" ENDCOLR , TEXT_COLOR("COLOR_TECH_TEXT"), GC.getTechInfo(eTestTech).getDescription());
					setListHelp(szOtherAndTechs, L"", szTempBuffer, L", ", nOtherAndTechs == 0);
					nOtherAndTechs++;
				}
			}
		}
	}

	if (bOrTechFound || bAndTechFound)
	{
		if (nOtherAndTechs > 0 || nOtherOrTechs > 0)
		{
			szBuffer.append(L' ');

			if (nOtherAndTechs > 0)
			{
				szBuffer.append(gDLL->getText("TXT_KEY_WITH_SPACE"));
				szBuffer.append(szOtherAndTechs);
			}

			if (nOtherOrTechs > 0)
			{
				if (bAndTechFound)
				{
					if (nOtherAndTechs > 0)
					{
						szBuffer.append(gDLL->getText("TXT_KEY_AND_SPACE"));
					}
					else
					{
						szBuffer.append(gDLL->getText("TXT_KEY_WITH_SPACE"));
					}
					szBuffer.append(szOtherOrTechs);
				}
				else if (bOrTechFound)
				{
					szBuffer.append(NEWLINE);
					szBuffer.append(gDLL->getText("TXT_KEY_MISC_ALTERNATIVELY_DERIVED", GC.getTechInfo(eTech).getTextKeyWide(), szOtherOrTechs.GetCString()));
				}
			}
		}
	}
}

// <advc.034>
void CvGameTextMgr::buildDisengageString(CvWString& szString, PlayerTypes ePlayer,
		PlayerTypes eOther) {

	int iTurns = 0;
	CvGame const& g = GC.getGame(); int foo=-1;
	for(CvDeal* d = g.firstDeal(&foo); d != NULL; d = g.nextDeal(&foo)) {
		if(d->isDisengage() && d->isBetween(ePlayer, eOther)) {
			iTurns = d->turnsToCancel();
			break;
		}
	}
	FAssert(iTurns >= 0);
	// This is "%s1 (%d2 [NUM2:Turn:Turns])", just what I need.
	szString.append(gDLL->getText("INTERFACE_CITY_PRODUCTION",
			gDLL->getText("TXT_KEY_MISC_OPEN_BORDERS").GetCString(), iTurns));
} // </advc.034>


void CvGameTextMgr::setPromotionHelp(CvWStringBuffer &szBuffer, PromotionTypes ePromotion, bool bCivilopediaText)
{
	if (!bCivilopediaText)
	{
		CvWString szTempBuffer;

		if (NO_PROMOTION == ePromotion)
		{
			return;
		}
		CvPromotionInfo& promo = GC.getPromotionInfo(ePromotion);

		szTempBuffer.Format( SETCOLR L"%s" ENDCOLR , TEXT_COLOR("COLOR_HIGHLIGHT_TEXT"), promo.getDescription());
		szBuffer.append(szTempBuffer);
	}

	parsePromotionHelp(szBuffer, ePromotion);
}

void CvGameTextMgr::setUnitCombatHelp(CvWStringBuffer &szBuffer, UnitCombatTypes eUnitCombat)
{
	szBuffer.append(GC.getUnitCombatInfo(eUnitCombat).getDescription());
}

void CvGameTextMgr::setImprovementHelp(CvWStringBuffer &szBuffer, ImprovementTypes eImprovement, bool bCivilopediaText)
{
	if(NO_IMPROVEMENT == eImprovement)
		return;

	CvWString szTempBuffer;
	CvWString szFirstBuffer;

	CvImprovementInfo& info = GC.getImprovementInfo(eImprovement);
	if (!bCivilopediaText)
	{
		szTempBuffer.Format( SETCOLR L"%s" ENDCOLR, TEXT_COLOR("COLOR_HIGHLIGHT_TEXT"), info.getDescription());
		szBuffer.append(szTempBuffer);

		setYieldChangeHelp(szBuffer, L", ", L"", L"", info.getYieldChangeArray(), false, false);

		setYieldChangeHelp(szBuffer, L"", L"", gDLL->getText("TXT_KEY_MISC_WITH_IRRIGATION").c_str(), info.getIrrigatedYieldChangeArray());
		setYieldChangeHelp(szBuffer, L"", L"", gDLL->getText("TXT_KEY_MISC_ON_HILLS").c_str(), info.getHillsYieldChangeArray());
		setYieldChangeHelp(szBuffer, L"", L"", gDLL->getText("TXT_KEY_MISC_ALONG_RIVER").c_str(), info.getRiverSideYieldChangeArray());

		for (int iTech = 0; iTech < GC.getNumTechInfos(); iTech++)
		{
			for (int iYield = 0; iYield < NUM_YIELD_TYPES; iYield++)
			{
				if (info.getTechYieldChanges(iTech, iYield) != 0)
				{
					szBuffer.append(NEWLINE);
					szBuffer.append(gDLL->getText("TXT_KEY_IMPROVEMENT_WITH_TECH", info.getTechYieldChanges(iTech, iYield), GC.getYieldInfo((YieldTypes)iYield).getChar(), GC.getTechInfo((TechTypes)iTech).getTextKeyWide()));
				}
			}
		}

		//	Civics
		for (int iYield = 0; iYield < NUM_YIELD_TYPES; iYield++)
		{
			for (int iCivic = 0; iCivic < GC.getNumCivicInfos(); iCivic++)
			{
				int iChange = GC.getCivicInfo((CivicTypes)iCivic).getImprovementYieldChanges(eImprovement, iYield);
				if (iChange != 0)
				{
					szTempBuffer.Format( SETCOLR L"%s" ENDCOLR , TEXT_COLOR("COLOR_HIGHLIGHT_TEXT"), GC.getCivicInfo((CivicTypes)iCivic).getDescription());
					szBuffer.append(NEWLINE);
					szBuffer.append(gDLL->getText("TXT_KEY_CIVIC_IMPROVEMENT_YIELD_CHANGE", iChange, GC.getYieldInfo((YieldTypes)iYield).getChar()));
					szBuffer.append(szTempBuffer);
				}
			}
		}
	}

	if (info.isRequiresRiverSide())
	{
		szBuffer.append(NEWLINE);
		szBuffer.append(gDLL->getText("TXT_KEY_IMPROVEMENT_REQUIRES_RIVER"));
	}
	if (info.isCarriesIrrigation())
	{
		szBuffer.append(NEWLINE);
		szBuffer.append(gDLL->getText("TXT_KEY_IMPROVEMENT_CARRIES_IRRIGATION"));
	}
	if (bCivilopediaText)
	{
		if (info.isNoFreshWater())
		{
			szBuffer.append(NEWLINE);
			szBuffer.append(gDLL->getText("TXT_KEY_IMPROVEMENT_NO_BUILD_FRESH_WATER"));
		}
		if (info.isWater())
		{
			szBuffer.append(NEWLINE);
			szBuffer.append(gDLL->getText("TXT_KEY_IMPROVEMENT_BUILD_ONLY_WATER"));
		}
		if (info.isRequiresFlatlands())
		{
			szBuffer.append(NEWLINE);
			szBuffer.append(gDLL->getText("TXT_KEY_IMPROVEMENT_ONLY_BUILD_FLATLANDS"));
		}
	}

	if (info.getImprovementUpgrade() != NO_IMPROVEMENT)
	{
		int iTurns = GC.getGame().getImprovementUpgradeTime(eImprovement);

		szBuffer.append(NEWLINE);
		szBuffer.append(gDLL->getText("TXT_KEY_IMPROVEMENT_EVOLVES", GC.getImprovementInfo((ImprovementTypes) info.getImprovementUpgrade()).getTextKeyWide(), iTurns));
	}

	int iLast = -1;
	for (int iBonus = 0; iBonus < GC.getNumBonusInfos(); iBonus++)
	{
		int iRand = info.getImprovementBonusDiscoverRand(iBonus);
		if (iRand > 0)
		{
			szFirstBuffer.Format(L"%s%s", NEWLINE, gDLL->getText("TXT_KEY_IMPROVEMENT_CHANCE_DISCOVER").c_str());
			szTempBuffer.Format(L"%c", GC.getBonusInfo((BonusTypes) iBonus).getChar());
			setListHelp(szBuffer, szFirstBuffer, szTempBuffer, L", ", iRand != iLast);
			iLast = iRand;
		}
	}

	if (info.getDefenseModifier() != 0)
	{
		szBuffer.append(NEWLINE);
		szBuffer.append(gDLL->getText("TXT_KEY_IMPROVEMENT_DEFENSE_MODIFIER", info.getDefenseModifier()));
	}

	if (info.getHappiness() != 0)
	{
		szBuffer.append(NEWLINE);
		szBuffer.append(gDLL->getText("TXT_KEY_MISC_ICON_CHANGE_NEARBY_CITIES",
				// UNOFFICIAL_PATCH, Bugfix, 08/28/09, jdog5000: Use absolute value with unhappy face
				abs(info.getHappiness()), (info.getHappiness() > 0 ? gDLL->getSymbolID(HAPPY_CHAR) : gDLL->getSymbolID(UNHAPPY_CHAR))));
	}

	if (info.isActsAsCity())
	{
		szBuffer.append(NEWLINE);
		szBuffer.append(gDLL->getText("TXT_KEY_IMPROVEMENT_DEFENSE_MODIFIER_EXTRA"));
	}

	if (info.getFeatureGrowthProbability() > 0)
	{
		szBuffer.append(NEWLINE);
		szBuffer.append(gDLL->getText("TXT_KEY_IMPROVEMENT_MORE_GROWTH"));
	}
	else if (info.getFeatureGrowthProbability() < 0)
	{
		szBuffer.append(NEWLINE);
		szBuffer.append(gDLL->getText("TXT_KEY_IMPROVEMENT_LESS_GROWTH"));
	}

	if (bCivilopediaText)
	{
		if (info.getPillageGold() > 0)
		{
			szBuffer.append(NEWLINE);
			szBuffer.append(gDLL->getText("TXT_KEY_IMPROVEMENT_PILLAGE_YIELDS", info.getPillageGold()));
		}
	}
}


void CvGameTextMgr::getDealString(CvWStringBuffer& szBuffer, CvDeal const& deal, PlayerTypes ePlayerPerspective,
		bool bCancel) // advc.004w
{
	PlayerTypes ePlayer1 = deal.getFirstPlayer();
	PlayerTypes ePlayer2 = deal.getSecondPlayer();

	const CLinkList<TradeData>* pListPlayer1 = deal.getFirstTrades();
	const CLinkList<TradeData>* pListPlayer2 = deal.getSecondTrades();

	getDealString(szBuffer, ePlayer1, ePlayer2, pListPlayer1, pListPlayer2, ePlayerPerspective,
			(bCancel ? -1 : deal.turnsToCancel())); // advc.004w
}

void CvGameTextMgr::getDealString(CvWStringBuffer& szBuffer, PlayerTypes ePlayer1, PlayerTypes ePlayer2, const CLinkList<TradeData>* pListPlayer1, const CLinkList<TradeData>* pListPlayer2, PlayerTypes ePlayerPerspective,
		int iTurnsToCancel) // advc.004w
{
	if (NO_PLAYER == ePlayer1 || NO_PLAYER == ePlayer2)
	{
		FAssertMsg(false, "Deal needs two parties");
		return;
	}

	CvWStringBuffer szDealOne;
	if (NULL != pListPlayer1 && pListPlayer1->getLength() > 0)
	{
		CLLNode<TradeData>* pTradeNode;
		bool bFirst = true;
		for (pTradeNode = pListPlayer1->head(); pTradeNode; pTradeNode = pListPlayer1->next(pTradeNode))
		{
			CvWStringBuffer szTrade;
			getTradeString(szTrade, pTradeNode->m_data, ePlayer1, ePlayer2,
					iTurnsToCancel); // advc.004w
			setListHelp(szDealOne, L"", szTrade.getCString(), L", ", bFirst);
			bFirst = false;
		}
	}

	CvWStringBuffer szDealTwo;
	if (NULL != pListPlayer2 && pListPlayer2->getLength() > 0)
	{
		CLLNode<TradeData>* pTradeNode;
		bool bFirst = true;
		for (pTradeNode = pListPlayer2->head(); pTradeNode; pTradeNode = pListPlayer2->next(pTradeNode))
		{
			CvWStringBuffer szTrade;
			getTradeString(szTrade, pTradeNode->m_data, ePlayer2, ePlayer1,
					iTurnsToCancel); // advc.004w
			setListHelp(szDealTwo, L"", szTrade.getCString(), L", ", bFirst);
			bFirst = false;
		}
	} // <advc.004w>
	bool bAllDual = false;
	if(pListPlayer1 != NULL && pListPlayer2 != NULL) {
		for(int pass = 0; pass < 2; pass++) {
			CLinkList<TradeData> const& list = *(pass == 0 ? pListPlayer1 : pListPlayer2);
			for(CLLNode<TradeData>* node = list.head(); node != NULL;
					node = list.next(node)) {
				TradeableItems item = node->m_data.m_eItemType;
				if(!CvDeal::isDual(item)) {
					bAllDual = false;
					break;
				}
				else bAllDual = true;
			}
		}
	}
	if(bAllDual) {
		szBuffer.append(gDLL->getText("TXT_KEY_MISC_DEAL_DUAL",
				szDealOne.getCString(), (ePlayerPerspective == ePlayer1 ?
				GET_PLAYER(ePlayer2).getNameKey() :
				GET_PLAYER(ePlayer1).getNameKey())));
	} else // </advc.004w>
		if (!szDealOne.isEmpty())
	{
		if (!szDealTwo.isEmpty())
		{
			if (ePlayerPerspective == ePlayer1)
			{
				szBuffer.append(gDLL->getText("TXT_KEY_MISC_OUR_DEAL", szDealOne.getCString(), GET_PLAYER(ePlayer2).getNameKey(), szDealTwo.getCString()));
			}
			else if (ePlayerPerspective == ePlayer2)
			{
				szBuffer.append(gDLL->getText("TXT_KEY_MISC_OUR_DEAL", szDealTwo.getCString(), GET_PLAYER(ePlayer1).getNameKey(), szDealOne.getCString()));
			}
			else
			{
				szBuffer.append(gDLL->getText("TXT_KEY_MISC_DEAL", GET_PLAYER(ePlayer1).getNameKey(), szDealOne.getCString(), GET_PLAYER(ePlayer2).getNameKey(), szDealTwo.getCString()));
			}
		}
		else
		{
			if (ePlayerPerspective == ePlayer1)
			{
				szBuffer.append(gDLL->getText("TXT_KEY_MISC_DEAL_ONESIDED_OURS", szDealOne.getCString(), GET_PLAYER(ePlayer2).getNameKey()));
			}
			else if (ePlayerPerspective == ePlayer2)
			{
				szBuffer.append(gDLL->getText("TXT_KEY_MISC_DEAL_ONESIDED_THEIRS", szDealOne.getCString(), GET_PLAYER(ePlayer1).getNameKey()));
			}
			else
			{
				szBuffer.append(gDLL->getText("TXT_KEY_MISC_DEAL_ONESIDED", GET_PLAYER(ePlayer1).getNameKey(), szDealOne.getCString(), GET_PLAYER(ePlayer2).getNameKey()));
			}
		}
	}
	else if (!szDealTwo.isEmpty())
	{
		if (ePlayerPerspective == ePlayer1)
		{
			szBuffer.append(gDLL->getText("TXT_KEY_MISC_DEAL_ONESIDED_THEIRS", szDealTwo.getCString(), GET_PLAYER(ePlayer2).getNameKey()));
		}
		else if (ePlayerPerspective == ePlayer2)
		{
			szBuffer.append(gDLL->getText("TXT_KEY_MISC_DEAL_ONESIDED_OURS", szDealTwo.getCString(), GET_PLAYER(ePlayer1).getNameKey()));
		}
		else
		{
			szBuffer.append(gDLL->getText("TXT_KEY_MISC_DEAL_ONESIDED", GET_PLAYER(ePlayer2).getNameKey(), szDealTwo.getCString(), GET_PLAYER(ePlayer1).getNameKey()));
		}
	}
}

void CvGameTextMgr::getWarplanString(CvWStringBuffer& szString, WarPlanTypes eWarPlan)
{
	switch (eWarPlan)
	{
		case WARPLAN_ATTACKED_RECENT: szString.assign(L"new defensive war"); break;
		case WARPLAN_ATTACKED: szString.assign(L"defensive war"); break;
		case WARPLAN_PREPARING_LIMITED: szString.assign(L"preparing limited war"); break;
		case WARPLAN_PREPARING_TOTAL: szString.assign(L"preparing total war"); break;
		case WARPLAN_LIMITED: szString.assign(L"limited war"); break;
		case WARPLAN_TOTAL: szString.assign(L"total war"); break;
		case WARPLAN_DOGPILE: szString.assign(L"dogpile war"); break;
		case NO_WARPLAN: szString.assign(L"no warplan"); break;
		default:  szString.assign(L"unknown warplan"); break;
	}
}

void CvGameTextMgr::getAttitudeString(CvWStringBuffer& szBuffer, PlayerTypes ePlayer,
		PlayerTypes eTargetPlayer, /* advc.sha: */ bool bConstCache)
{
	CvPlayerAI& kPlayer = GET_PLAYER(ePlayer);
	if (kPlayer.isHuman())
		return; // K-Mod

	CvGame const& g = GC.getGame();
	CvWString szTempBuffer;
	CvWStringBuffer szBreakdown; // advc.sha
	// advc.sha:
	bool bSHowHiddenAttitude = (GC.getDefineINT("SHOW_HIDDEN_ATTITUDE") > 0 || g.isDebugMode());
	// <advc.004q>
	bool bObscurePersonality = (g.isOption(GAMEOPTION_RANDOM_PERSONALITIES) && !g.isDebugMode());

	// advc.004q: ATTITUDE_TOWARDS moved to the end of this function
	// (K-Mod note: vassal information has been moved from here to a new function)

	// Attitude breakdown ...
	int iTotal = 0; // advc.sha
	for (int iPass = 0; iPass < 2; iPass++)
	{
		int iAttitudeChange = kPlayer.AI_getCloseBordersAttitude(eTargetPlayer);
		if ((iPass == 0) ? (iAttitudeChange > 0) : (iAttitudeChange < 0))
		{
			szTempBuffer.Format(SETCOLR L"%s" ENDCOLR, TEXT_COLOR((iAttitudeChange > 0) ? "COLOR_POSITIVE_TEXT" : "COLOR_NEGATIVE_TEXT"), gDLL->getText("TXT_KEY_MISC_ATTITUDE_LAND_TARGET", iAttitudeChange).GetCString());
			szBreakdown.append(NEWLINE);
			szBreakdown.append(szTempBuffer);
			iTotal += iAttitudeChange;
		}
		// <advc.sha>
		iAttitudeChange = kPlayer.AI_getWarAttitude(eTargetPlayer,
				kPlayer.AI_getAttitudeVal(eTargetPlayer) - iAttitudeChange);
		// </advc.sha>
		if ((iPass == 0) ? (iAttitudeChange > 0) : (iAttitudeChange < 0))
		{
			szTempBuffer.Format(SETCOLR L"%s" ENDCOLR, TEXT_COLOR((iAttitudeChange > 0) ? "COLOR_POSITIVE_TEXT" : "COLOR_NEGATIVE_TEXT"), gDLL->getText("TXT_KEY_MISC_ATTITUDE_WAR", iAttitudeChange).GetCString());
			szBreakdown.append(NEWLINE);
			szBreakdown.append(szTempBuffer);
			iTotal += iAttitudeChange;
		}

		iAttitudeChange = kPlayer.AI_getPeaceAttitude(eTargetPlayer);
		if ((iPass == 0) ? (iAttitudeChange > 0) : (iAttitudeChange < 0))
		{
			szTempBuffer.Format(SETCOLR L"%s" ENDCOLR, TEXT_COLOR((iAttitudeChange > 0) ? "COLOR_POSITIVE_TEXT" : "COLOR_NEGATIVE_TEXT"), gDLL->getText("TXT_KEY_MISC_ATTITUDE_PEACE", iAttitudeChange).GetCString());
			szBreakdown.append(NEWLINE);
			szBreakdown.append(szTempBuffer);
			iTotal += iAttitudeChange;
		}

		iAttitudeChange = kPlayer.AI_getSameReligionAttitude(eTargetPlayer);
		if ((iPass == 0) ? (iAttitudeChange > 0) : (iAttitudeChange < 0))
		{
			szTempBuffer.Format(SETCOLR L"%s" ENDCOLR, TEXT_COLOR((iAttitudeChange > 0) ? "COLOR_POSITIVE_TEXT" : "COLOR_NEGATIVE_TEXT"), gDLL->getText("TXT_KEY_MISC_ATTITUDE_SAME_RELIGION", iAttitudeChange).GetCString());
			szBreakdown.append(NEWLINE);
			szBreakdown.append(szTempBuffer);
			iTotal += iAttitudeChange;
		}

		iAttitudeChange = kPlayer.AI_getDifferentReligionAttitude(eTargetPlayer);
		if ((iPass == 0) ? (iAttitudeChange > 0) : (iAttitudeChange < 0))
		{
			szTempBuffer.Format(SETCOLR L"%s" ENDCOLR, TEXT_COLOR((iAttitudeChange > 0) ? "COLOR_POSITIVE_TEXT" : "COLOR_NEGATIVE_TEXT"), gDLL->getText("TXT_KEY_MISC_ATTITUDE_DIFFERENT_RELIGION", iAttitudeChange).GetCString());
			szBreakdown.append(NEWLINE);
			szBreakdown.append(szTempBuffer);
			iTotal += iAttitudeChange;
		}

		iAttitudeChange = kPlayer.AI_getBonusTradeAttitude(eTargetPlayer);
		if ((iPass == 0) ? (iAttitudeChange > 0) : (iAttitudeChange < 0))
		{
			szTempBuffer.Format(SETCOLR L"%s" ENDCOLR, TEXT_COLOR((iAttitudeChange > 0) ? "COLOR_POSITIVE_TEXT" : "COLOR_NEGATIVE_TEXT"), gDLL->getText("TXT_KEY_MISC_ATTITUDE_BONUS_TRADE", iAttitudeChange).GetCString());
			szBreakdown.append(NEWLINE);
			szBreakdown.append(szTempBuffer);
			iTotal += iAttitudeChange;
		}

		iAttitudeChange = kPlayer.AI_getOpenBordersAttitude(eTargetPlayer);
		if ((iPass == 0) ? (iAttitudeChange > 0) : (iAttitudeChange < 0))
		{
			szTempBuffer.Format(SETCOLR L"%s" ENDCOLR, TEXT_COLOR((iAttitudeChange > 0) ? "COLOR_POSITIVE_TEXT" : "COLOR_NEGATIVE_TEXT"), gDLL->getText("TXT_KEY_MISC_ATTITUDE_OPEN_BORDERS", iAttitudeChange).GetCString());
			szBreakdown.append(NEWLINE);
			szBreakdown.append(szTempBuffer);
			iTotal += iAttitudeChange;
		}

		iAttitudeChange = kPlayer.AI_getDefensivePactAttitude(eTargetPlayer);
		if ((iPass == 0) ? (iAttitudeChange > 0) : (iAttitudeChange < 0))
		{
			szTempBuffer.Format(SETCOLR L"%s" ENDCOLR, TEXT_COLOR((iAttitudeChange > 0) ? "COLOR_POSITIVE_TEXT" : "COLOR_NEGATIVE_TEXT"), gDLL->getText("TXT_KEY_MISC_ATTITUDE_DEFENSIVE_PACT", iAttitudeChange).GetCString());
			szBreakdown.append(NEWLINE);
			szBreakdown.append(szTempBuffer);
			iTotal += iAttitudeChange;
		}

		iAttitudeChange = kPlayer.AI_getRivalDefensivePactAttitude(eTargetPlayer);
		if ((iPass == 0) ? (iAttitudeChange > 0) : (iAttitudeChange < 0))
		{
			szTempBuffer.Format(SETCOLR L"%s" ENDCOLR, TEXT_COLOR((iAttitudeChange > 0) ? "COLOR_POSITIVE_TEXT" : "COLOR_NEGATIVE_TEXT"), gDLL->getText("TXT_KEY_MISC_ATTITUDE_RIVAL_DEFENSIVE_PACT", iAttitudeChange).GetCString());
			szBreakdown.append(NEWLINE);
			szBreakdown.append(szTempBuffer);
			iTotal += iAttitudeChange;
		}

		iAttitudeChange = kPlayer.AI_getRivalVassalAttitude(eTargetPlayer);
		if ((iPass == 0) ? (iAttitudeChange > 0) : (iAttitudeChange < 0))
		{
			szTempBuffer.Format(SETCOLR L"%s" ENDCOLR, TEXT_COLOR((iAttitudeChange > 0) ? "COLOR_POSITIVE_TEXT" : "COLOR_NEGATIVE_TEXT"), gDLL->getText("TXT_KEY_MISC_ATTITUDE_RIVAL_VASSAL", iAttitudeChange).GetCString());
			szBreakdown.append(NEWLINE);
			szBreakdown.append(szTempBuffer);
			iTotal += iAttitudeChange;
		}

		// <advc.130w>
		iAttitudeChange = kPlayer.AI_getExpansionistAttitude(eTargetPlayer);
		if((iPass == 0) ? (iAttitudeChange > 0) : (iAttitudeChange < 0))
		{
			szTempBuffer.Format(SETCOLR L"%s" ENDCOLR, TEXT_COLOR((iAttitudeChange > 0) ?
					"COLOR_POSITIVE_TEXT" : "COLOR_NEGATIVE_TEXT"),
					gDLL->getText("TXT_KEY_MISC_ATTITUDE_EXPANSIONIST",
					iAttitudeChange).GetCString());
			szBreakdown.append(NEWLINE);
			szBreakdown.append(szTempBuffer);
			iTotal += iAttitudeChange;
		} // </advc.130w>

		iAttitudeChange = kPlayer.AI_getShareWarAttitude(eTargetPlayer);
		if ((iPass == 0) ? (iAttitudeChange > 0) : (iAttitudeChange < 0))
		{
			szTempBuffer.Format(SETCOLR L"%s" ENDCOLR, TEXT_COLOR((iAttitudeChange > 0) ? "COLOR_POSITIVE_TEXT" : "COLOR_NEGATIVE_TEXT"), gDLL->getText("TXT_KEY_MISC_ATTITUDE_SHARE_WAR", iAttitudeChange).GetCString());
			szBreakdown.append(NEWLINE);
			szBreakdown.append(szTempBuffer);
			iTotal += iAttitudeChange;
		}

		iAttitudeChange = kPlayer.AI_getFavoriteCivicAttitude(eTargetPlayer);
		if ((iPass == 0) ? (iAttitudeChange > 0) : (iAttitudeChange < 0))
		{
			szTempBuffer.Format(SETCOLR L"%s" ENDCOLR, TEXT_COLOR((iAttitudeChange > 0) ? "COLOR_POSITIVE_TEXT" : "COLOR_NEGATIVE_TEXT"), gDLL->getText("TXT_KEY_MISC_ATTITUDE_FAVORITE_CIVIC", iAttitudeChange).GetCString());
			szBreakdown.append(NEWLINE);
			szBreakdown.append(szTempBuffer);
			iTotal += iAttitudeChange;
		}

		iAttitudeChange = kPlayer.AI_getTradeAttitude(eTargetPlayer);
		if ((iPass == 0) ? (iAttitudeChange > 0) : (iAttitudeChange < 0))
		{
			szTempBuffer.Format(SETCOLR L"%s" ENDCOLR, TEXT_COLOR((iAttitudeChange > 0) ? "COLOR_POSITIVE_TEXT" : "COLOR_NEGATIVE_TEXT"), gDLL->getText("TXT_KEY_MISC_ATTITUDE_TRADE", iAttitudeChange).GetCString());
			szBreakdown.append(NEWLINE);
			szBreakdown.append(szTempBuffer);
			iTotal += iAttitudeChange;
		}

		iAttitudeChange = kPlayer.AI_getRivalTradeAttitude(eTargetPlayer);
		if ((iPass == 0) ? (iAttitudeChange > 0) : (iAttitudeChange < 0))
		{
			szTempBuffer.Format(SETCOLR L"%s" ENDCOLR, TEXT_COLOR((iAttitudeChange > 0) ? "COLOR_POSITIVE_TEXT" : "COLOR_NEGATIVE_TEXT"), gDLL->getText("TXT_KEY_MISC_ATTITUDE_RIVAL_TRADE", iAttitudeChange).GetCString());
			szBreakdown.append(NEWLINE);
			szBreakdown.append(szTempBuffer);
			iTotal += iAttitudeChange;
		}
		// advc.130r: Commented out
		/*iAttitudeChange = GET_PLAYER(ePlayer).AI_getColonyAttitude(eTargetPlayer);
		if ((iPass == 0) ? (iAttitudeChange > 0) : (iAttitudeChange < 0)) {
			szTempBuffer.Format(SETCOLR L"%s" ENDCOLR, TEXT_COLOR((iAttitudeChange > 0) ? "COLOR_POSITIVE_TEXT" : "COLOR_NEGATIVE_TEXT"), gDLL->getText("TXT_KEY_MISC_ATTITUDE_FREEDOM", iAttitudeChange).GetCString());
			szBreakdown.append(NEWLINE);
			szBreakdown.append(szTempBuffer);
			iTotal += iAttitudeChange;
		}*/

		iAttitudeChange = GET_PLAYER(ePlayer).AI_getAttitudeExtra(eTargetPlayer);
		if ((iPass == 0) ? (iAttitudeChange > 0) : (iAttitudeChange < 0))
		{
			szTempBuffer.Format(SETCOLR L"%s" ENDCOLR, TEXT_COLOR((iAttitudeChange > 0) ? "COLOR_POSITIVE_TEXT" : "COLOR_NEGATIVE_TEXT"), gDLL->getText(((iAttitudeChange > 0) ? "TXT_KEY_MISC_ATTITUDE_EXTRA_GOOD" : "TXT_KEY_MISC_ATTITUDE_EXTRA_BAD"), iAttitudeChange).GetCString());
			szBreakdown.append(NEWLINE);
			szBreakdown.append(szTempBuffer);
			iTotal += iAttitudeChange;
		} // <advc.sha>
		if(bSHowHiddenAttitude) // BEGIN: Show Hidden Attitude Mod 01/22/2010
		{
			iAttitudeChange = kPlayer.AI_getTeamSizeAttitude(eTargetPlayer);
			if((iPass == 0) ? (iAttitudeChange > 0) : (iAttitudeChange < 0))
			{
				szTempBuffer.Format(SETCOLR L"%s" ENDCOLR,
						TEXT_COLOR((iAttitudeChange > 0) ? "COLOR_POSITIVE_TEXT" :
						"COLOR_NEGATIVE_TEXT"),
						gDLL->getText("TXT_KEY_MISC_ATTITUDE_TEAM_SIZE",
						iAttitudeChange).GetCString());
				szBreakdown.append(NEWLINE);
				szBreakdown.append(szTempBuffer);
				iTotal += iAttitudeChange;
			}
			if (!bObscurePersonality) // advc: Moved this check from CvPlayerAI
			{
				iAttitudeChange = kPlayer.AI_getRankDifferenceAttitude(eTargetPlayer);
				CvWString szRankText = gDLL->getText(
						g.getPlayerRank(ePlayer) < g.getPlayerRank(eTargetPlayer) ?
						"TXT_KEY_MISC_ATTITUDE_BETTER_RANK" :
						"TXT_KEY_MISC_ATTITUDE_WORSE_RANK", iAttitudeChange);
				if(iAttitudeChange > 0 && iPass == 0)
				{
					szTempBuffer.Format(SETCOLR L"%s" ENDCOLR,
							TEXT_COLOR("COLOR_POSITIVE_TEXT"), szRankText.GetCString());
					szBreakdown.append(NEWLINE);
					szBreakdown.append(szTempBuffer);
					iTotal += iAttitudeChange;
				}
				if(iAttitudeChange < 0 && iPass != 0)
				{
					szTempBuffer.Format(SETCOLR L"%s" ENDCOLR,
							TEXT_COLOR("COLOR_NEGATIVE_TEXT"), szRankText.GetCString());
					szBreakdown.append(NEWLINE);
					szBreakdown.append(szTempBuffer);
					iTotal += iAttitudeChange;
				}
				iAttitudeChange = kPlayer.AI_getFirstImpressionAttitude(eTargetPlayer);
				if((iPass == 0) ? (iAttitudeChange > 0) : (iAttitudeChange < 0))
				{
					szTempBuffer.Format(SETCOLR L"%s" ENDCOLR,
							TEXT_COLOR((iAttitudeChange > 0) ? "COLOR_POSITIVE_TEXT" :
							"COLOR_NEGATIVE_TEXT"),
							gDLL->getText("TXT_KEY_MISC_ATTITUDE_FIRST_IMPRESSION",
							iAttitudeChange).GetCString());
					szBreakdown.append(NEWLINE);
					szBreakdown.append(szTempBuffer);
					iTotal += iAttitudeChange;
				}
			}
			// Lost-war attitude: disabled
			/*iAttitudeChange = kPlayer.AI_getLostWarAttitude(eTargetPlayer);
			if((iPass == 0) ? (iAttitudeChange > 0) : (iAttitudeChange < 0)) {
				szTempBuffer.Format(SETCOLR L"%s" ENDCOLR, TEXT_COLOR((iAttitudeChange > 0) ? "COLOR_POSITIVE_TEXT" : "COLOR_NEGATIVE_TEXT"), gDLL->getText("TXT_KEY_MISC_ATTITUDE_LOST_WAR", iAttitudeChange).GetCString());
				szBreakdown.append(NEWLINE);
				szBreakdown.append(szTempBuffer);
				iTotal += iAttitudeChange;
			}*/ // END: Show Hidden Attitude Mod
		} // </advc.sha>
		// <advc.004q> Kludge. Really need default values for personality stats.
		static MemoryTypes aeObscureMemoryTypes[] =
		{
			MEMORY_DENIED_STOP_TRADING, MEMORY_DENIED_JOIN_WAR,
			MEMORY_ACCEPTED_CIVIC, MEMORY_DENIED_CIVIC,
			MEMORY_ACCEPTED_RELIGION, MEMORY_DENIED_RELIGION,
			MEMORY_REJECTED_DEMAND, MEMORY_ACCEPT_DEMAND,
			MEMORY_REFUSED_HELP, MEMORY_GIVE_HELP,
			MEMORY_SPY_CAUGHT, MEMORY_ACCEPTED_STOP_TRADING,
			MEMORY_NUKED_FRIEND, MEMORY_DECLARED_WAR_ON_FRIEND,
		};
		static int const iNumObscureMemoryTypes = sizeof(aeObscureMemoryTypes) / sizeof(MemoryTypes);
		// </advc.004q>
		for (int iI = 0; iI < NUM_MEMORY_TYPES; ++iI)
		{
			iAttitudeChange = kPlayer.AI_getMemoryAttitude(eTargetPlayer, (MemoryTypes)iI);
			if ((iPass == 0) ? (iAttitudeChange > 0) : (iAttitudeChange < 0))
			{
				/*  <advc.004q> Cap attitude change at 1 for memory types that
					can give away a leader's hidden personality. */
				int iAbsAttitudeChange = std::abs(iAttitudeChange);
				if (iAbsAttitudeChange > 1 && bObscurePersonality &&
					kPlayer.AI_getMemoryCount(eTargetPlayer, (MemoryTypes)iI) <= 2)
				{
					for (int j = 0; j < iNumObscureMemoryTypes; j++)
					{
						if (iI == aeObscureMemoryTypes[j])
						{
							iAbsAttitudeChange = 1;
							break;
						}
					}
					iAttitudeChange = (iAttitudeChange > 0 ? iAbsAttitudeChange : -iAbsAttitudeChange);
				} // </advc.004q>
				szTempBuffer.Format(SETCOLR L"%s" ENDCOLR, TEXT_COLOR(
						(iAttitudeChange > 0) ? "COLOR_POSITIVE_TEXT" : "COLOR_NEGATIVE_TEXT"),
						gDLL->getText("TXT_KEY_MISC_ATTITUDE_MEMORY", iAttitudeChange,
						GC.getMemoryInfo((MemoryTypes)iI).getDescription()).GetCString());
				szBreakdown.append(NEWLINE);
				szBreakdown.append(szTempBuffer);
				iTotal += iAttitudeChange;
			}
		}
	}
	// <advc.004q>
	int iTotalCached = kPlayer.AI_getAttitudeVal(eTargetPlayer, false);
	if (!bConstCache && bSHowHiddenAttitude && !bObscurePersonality &&
		iTotal != iTotalCached && !g.isNetworkMultiPlayer())
	{
		FAssertMsg(iTotal == iTotalCached, "Attitude cache out of date "
				"(OK after loading a save created during AI Auto play or prior to v0.95)");
		kPlayer.AI_updateAttitudeCache(eTargetPlayer, true);
		// Try again, this time without recursion. szBuffer hasn't been changed yet.
		getAttitudeString(szBuffer, ePlayer, eTargetPlayer, true);
		return;
	}
	// Attitude string - append this to szBuffer before appending the breakdown
	szBuffer.append(NEWLINE);
	//szBuffer.append(gDLL->getText("TXT_KEY_ATTITUDE_TOWARDS", GC.getAttitudeInfo(kPlayer.AI_getAttitude(eTargetPlayer)).getTextKeyWide(), GET_PLAYER(eTargetPlayer).getNameKey()));
	// Replacing the BtS line above
	wchar const* szAttitude = GC.getAttitudeInfo(
			kPlayer.AI_getAttitude(eTargetPlayer)).getTextKeyWide();
	wchar const* szTargetName = GET_PLAYER(eTargetPlayer).getNameKey();
	AttitudeTypes eAttitudeCached = kPlayer.AI_getAttitudeFromValue(iTotal);
	if(eAttitudeCached == ATTITUDE_CAUTIOUS)
	{
		if(iTotal == 0)
		{
			szBuffer.append(gDLL->getText("TXT_KEY_ZERO_ATTITUDE_TOWARDS",
					szAttitude, szTargetName));
		}
		else if(iTotal > 0)
		{
			szBuffer.append(gDLL->getText("TXT_KEY_POSITIVE_NEUTRAL_ATTITUDE_TOWARDS",
				szAttitude, iTotal, szTargetName));
		}
		else szBuffer.append(gDLL->getText("TXT_KEY_NEGATIVE_NEUTRAL_ATTITUDE_TOWARDS",
				szAttitude, iTotal, szTargetName));
	}
	else if(eAttitudeCached > ATTITUDE_CAUTIOUS)
	{
		szBuffer.append(gDLL->getText("TXT_KEY_GOOD_ATTITUDE_TOWARDS",
				szAttitude, iTotal, szTargetName));
	}
	else szBuffer.append(gDLL->getText("TXT_KEY_BAD_ATTITUDE_TOWARDS",
			szAttitude, iTotal, szTargetName));

	szBuffer.append(szBreakdown);
	// </advc.004q>
	/*if (NO_PLAYER != eTargetPlayer) {
		int iWarWeariness = GET_PLAYER(eTargetPlayer).getModifiedWarWearinessPercentAnger(GET_TEAM(GET_PLAYER(eTargetPlayer).getTeam()).getWarWeariness(eTeam) * std::max(0, 100 + kTeam.getEnemyWarWearinessModifier()));
		if (iWarWeariness / 10000 > 0) {
			szBuffer.append(NEWLINE);
			szBuffer.append(gDLL->getText("TXT_KEY_WAR_WEAR_HELP", iWarWeariness / 10000));
		}
	}*/ // K-Mod, I've moved this to a new function
}

// K-Mod
void CvGameTextMgr::getVassalInfoString(CvWStringBuffer& szBuffer, PlayerTypes ePlayer)
{
	FAssert(ePlayer != NO_PLAYER);

	const CvTeam& kTeam = GET_TEAM(GET_PLAYER(ePlayer).getTeam());
	//CvTeam& kTargetTeam = GET_TEAM(GET_PLAYER(eTargetPlayer).getTeam());

	for (TeamTypes i = (TeamTypes)0; i < MAX_TEAMS; i=(TeamTypes)(i+1))
	{
		const CvTeam& kLoopTeam = GET_TEAM(i);
		if (kLoopTeam.isAlive())
		{
			//if (kTargetTeam.isHasMet(i))

			if (kTeam.isVassal(i))
			{
				szBuffer.append(NEWLINE);
				szBuffer.append(CvWString::format(SETCOLR, TEXT_COLOR("COLOR_LIGHT_GREY")));
				// <advc.130v>
				if(kTeam.isCapitulated())
					szBuffer.append(gDLL->getText("TXT_KEY_ATTITUDE_VASSAL_CAP_OF",
							kLoopTeam.getName().GetCString()));
				else // </advc.130v>
				szBuffer.append(gDLL->getText("TXT_KEY_ATTITUDE_VASSAL_OF", kLoopTeam.getName().GetCString()));
				setVassalRevoltHelp(szBuffer, i, kTeam.getID());
				szBuffer.append(ENDCOLR);
			}
			else if (kLoopTeam.isVassal(kTeam.getID()))
			{
				szBuffer.append(NEWLINE);
				szBuffer.append(CvWString::format(SETCOLR, TEXT_COLOR("COLOR_LIGHT_GREY")));
				szBuffer.append(gDLL->getText("TXT_KEY_ATTITUDE_MASTER_OF", kLoopTeam.getName().GetCString()));
				szBuffer.append(ENDCOLR);
			}
		}
	}
}

void CvGameTextMgr::getWarWearinessString(CvWStringBuffer& szBuffer, PlayerTypes ePlayer, PlayerTypes eTargetPlayer) const
{
	FAssert(ePlayer != NO_PLAYER);
	// Show ePlayer's war weariness towards eTargetPlayer.
	// (note: this is the reverse of what was shown in the original code.)
	// War weariness should be shown in it natural units - it's a percentage of population
	const CvPlayer& kPlayer = GET_PLAYER(ePlayer);

	int iWarWeariness = 0;
	if (eTargetPlayer == NO_PLAYER || eTargetPlayer == ePlayer)
	{
		// If eTargetPlayer == NO_PLAYER, show ePlayer's total war weariness?
		// There are a couple of problems with displaying the total war weariness: information leak, out-of-date information...
		// lets do it only for the active player.
		if (GC.getGame().getActivePlayer() == ePlayer)
			iWarWeariness = kPlayer.getWarWearinessPercentAnger();
	}
	else
	{
		const CvPlayer& kTargetPlayer = GET_PLAYER(eTargetPlayer);
		if (atWar(kPlayer.getTeam(), kTargetPlayer.getTeam()) && (GC.getGame().isDebugMode() || GET_PLAYER(GC.getGame().getActivePlayer()).canSeeDemographics(ePlayer)))
		{
			iWarWeariness = kPlayer.getModifiedWarWearinessPercentAnger(GET_TEAM(kPlayer.getTeam()).getWarWeariness(kTargetPlayer.getTeam(), true)/100);
		}
	}

	iWarWeariness *= 100;
	iWarWeariness /= GC.getPERCENT_ANGER_DIVISOR();

	if (iWarWeariness != 0)
		szBuffer.append(CvWString::format(L"\n%s: %d%%", gDLL->getText("TXT_KEY_WAR_WEAR_HELP").GetCString(), iWarWeariness));
}
// K-Mod end

void CvGameTextMgr::getEspionageString(CvWStringBuffer& szBuffer, PlayerTypes ePlayer, PlayerTypes eTargetPlayer)
{
	FAssertMsg(false, "obsolete function. (getEspionageString)"); // K-Mod
	if (!GC.getGame().isOption(GAMEOPTION_NO_ESPIONAGE))
	{
		CvPlayer& kPlayer = GET_PLAYER(ePlayer);
		TeamTypes eTeam = (TeamTypes) kPlayer.getTeam();
		CvTeam& kTeam = GET_TEAM(eTeam);
		CvPlayer& kTargetPlayer = GET_PLAYER(eTargetPlayer);

		szBuffer.append(gDLL->getText("TXT_KEY_ESPIONAGE_AGAINST_PLAYER", kTargetPlayer.getNameKey(), kTeam.getEspionagePointsAgainstTeam(kTargetPlayer.getTeam()), GET_TEAM(kTargetPlayer.getTeam()).getEspionagePointsAgainstTeam(kPlayer.getTeam())));
	}
}


void CvGameTextMgr::getTradeString(CvWStringBuffer& szBuffer, const TradeData& tradeData, PlayerTypes ePlayer1, PlayerTypes ePlayer2,
		int iTurnsToCancel) // advc.004w
{
	switch (tradeData.m_eItemType)
	{
	case TRADE_GOLD:
		szBuffer.append(gDLL->getText("TXT_KEY_MISC_GOLD", tradeData.m_iData));
		break;
	case TRADE_GOLD_PER_TURN:
		szBuffer.append(gDLL->getText("TXT_KEY_MISC_GOLD_PER_TURN", tradeData.m_iData));
		break;
	case TRADE_MAPS:
		szBuffer.append(gDLL->getText("TXT_KEY_MISC_WORLD_MAP"));
		break;
	case TRADE_SURRENDER:
		szBuffer.append(gDLL->getText("TXT_KEY_MISC_CAPITULATE"));
		break;
	case TRADE_VASSAL:
		szBuffer.append(gDLL->getText("TXT_KEY_MISC_VASSAL"));
		break;
	case TRADE_OPEN_BORDERS:
		szBuffer.append(gDLL->getText("TXT_KEY_MISC_OPEN_BORDERS"));
		break;
	case TRADE_DEFENSIVE_PACT:
		szBuffer.append(gDLL->getText("TXT_KEY_MISC_DEFENSIVE_PACT"));
		break;
	case TRADE_PERMANENT_ALLIANCE:
		szBuffer.append(gDLL->getText("TXT_KEY_MISC_PERMANENT_ALLIANCE"));
		break;
	case TRADE_PEACE_TREATY:
		szBuffer.append(gDLL->getText("TXT_KEY_MISC_PEACE_TREATY",
				// <advc.004w>
				(iTurnsToCancel < 0 ? GC.getPEACE_TREATY_LENGTH() :
				iTurnsToCancel))); // </advc.004w>
		break;
	case TRADE_TECHNOLOGIES:
		szBuffer.assign(CvWString::format(L"%s", GC.getTechInfo((TechTypes)tradeData.m_iData).getDescription()));
		break;
	case TRADE_RESOURCES:
		szBuffer.assign(CvWString::format(L"%s", GC.getBonusInfo((BonusTypes)tradeData.m_iData).getDescription()));
		break;
	case TRADE_CITIES:
		szBuffer.assign(CvWString::format(L"%s", GET_PLAYER(ePlayer1).getCity(tradeData.m_iData)->getName().GetCString()));
		break;
	case TRADE_PEACE:
	case TRADE_WAR:
	case TRADE_EMBARGO:
		szBuffer.assign(CvWString::format(L"%s", GET_TEAM((TeamTypes)tradeData.m_iData).getName().GetCString()));
		break;
	case TRADE_CIVIC:
		szBuffer.assign(CvWString::format(L"%s", GC.getCivicInfo((CivicTypes)tradeData.m_iData).getDescription()));
		break;
	case TRADE_RELIGION:
		szBuffer.assign(CvWString::format(L"%s", GC.getReligionInfo((ReligionTypes)tradeData.m_iData).getDescription()));
		break; // <advc.034>
	case TRADE_DISENGAGE: {
		CvWString szString;
		buildDisengageString(szString, ePlayer1, ePlayer2);
		szBuffer.append(szString); }
		break; // </advc.034>
	default:
		FAssert(false);
		break;
	}
}

void CvGameTextMgr::setFeatureHelp(CvWStringBuffer &szBuffer, FeatureTypes eFeature, bool bCivilopediaText)
{
	if (NO_FEATURE == eFeature)
	{
		return;
	}
	CvFeatureInfo& feature = GC.getFeatureInfo(eFeature);

	int aiYields[NUM_YIELD_TYPES];
	if (!bCivilopediaText)
	{
		szBuffer.append(feature.getDescription());

		for (int iI = 0; iI < NUM_YIELD_TYPES; ++iI)
		{
			aiYields[iI] = feature.getYieldChange(iI);
		}
		setYieldChangeHelp(szBuffer, L"", L"", L"", aiYields);
	}
	for (int iI = 0; iI < NUM_YIELD_TYPES; ++iI)
	{
		aiYields[iI] = feature.getRiverYieldChange(iI);
	}
	setYieldChangeHelp(szBuffer, L"", L"", gDLL->getText("TXT_KEY_TERRAIN_NEXT_TO_RIVER"), aiYields);

	for (int iI = 0; iI < NUM_YIELD_TYPES; ++iI)
	{
		aiYields[iI] = feature.getHillsYieldChange(iI);
	}
	setYieldChangeHelp(szBuffer, L"", L"", gDLL->getText("TXT_KEY_TERRAIN_ON_HILLS"), aiYields);

	if (feature.getMovementCost() != 1)
	{
		szBuffer.append(gDLL->getText("TXT_KEY_TERRAIN_MOVEMENT_COST", feature.getMovementCost()));
	}

	CvWString szHealth;
	szHealth.Format(L"%.2f", 0.01f * abs(feature.getHealthPercent()));
	if (feature.getHealthPercent() > 0)
	{
		szBuffer.append(gDLL->getText("TXT_KEY_FEATURE_GOOD_HEALTH", szHealth.GetCString()));
	}
	else if (feature.getHealthPercent() < 0)
	{
		szBuffer.append(gDLL->getText("TXT_KEY_FEATURE_BAD_HEALTH", szHealth.GetCString()));
	}

	if (feature.getDefenseModifier() != 0)
	{
		szBuffer.append(gDLL->getText("TXT_KEY_TERRAIN_DEFENSE_MODIFIER", feature.getDefenseModifier()));
	}
	// UNOFFICIAL_PATCH, Bugfix (FeatureDamageFix), 06/02/10, LunarMongoose: START
	if (feature.getTurnDamage() > 0)
	{
		szBuffer.append(gDLL->getText("TXT_KEY_TERRAIN_TURN_DAMAGE", feature.getTurnDamage()));
	}
	else if (feature.getTurnDamage() < 0)
	{
		szBuffer.append(gDLL->getText("TXT_KEY_TERRAIN_TURN_HEALING", -feature.getTurnDamage()));
	} // UNOFFICIAL_PATCH: END

	if (feature.isAddsFreshWater())
	{
		szBuffer.append(gDLL->getText("TXT_KEY_FEATURE_ADDS_FRESH_WATER"));
	}

	if (feature.isImpassable())
	{
		szBuffer.append(gDLL->getText("TXT_KEY_TERRAIN_IMPASSABLE"));
	}

	if (feature.isNoCity())
	{
		szBuffer.append(gDLL->getText("TXT_KEY_TERRAIN_NO_CITIES"));
	}

	if (feature.isNoImprovement())
	{
		szBuffer.append(gDLL->getText("TXT_KEY_FEATURE_NO_IMPROVEMENT"));
	}

}


void CvGameTextMgr::setTerrainHelp(CvWStringBuffer &szBuffer, TerrainTypes eTerrain, bool bCivilopediaText)
{
	if (NO_TERRAIN == eTerrain)
	{
		return;
	}
	CvTerrainInfo& terrain = GC.getTerrainInfo(eTerrain);

	int aiYields[NUM_YIELD_TYPES];
	if (!bCivilopediaText)
	{
		szBuffer.append(terrain.getDescription());

		for (int iI = 0; iI < NUM_YIELD_TYPES; ++iI)
		{
			aiYields[iI] = terrain.getYield(iI);
		}
		setYieldChangeHelp(szBuffer, L"", L"", L"", aiYields);
	}
	for (int iI = 0; iI < NUM_YIELD_TYPES; ++iI)
	{
		aiYields[iI] = terrain.getRiverYieldChange(iI);
	}
	setYieldChangeHelp(szBuffer, L"", L"", gDLL->getText("TXT_KEY_TERRAIN_NEXT_TO_RIVER"), aiYields);

	for (int iI = 0; iI < NUM_YIELD_TYPES; ++iI)
	{
		aiYields[iI] = terrain.getHillsYieldChange(iI);
	}
	setYieldChangeHelp(szBuffer, L"", L"", gDLL->getText("TXT_KEY_TERRAIN_ON_HILLS"), aiYields);

	if (terrain.getMovementCost() != 1)
	{
		szBuffer.append(gDLL->getText("TXT_KEY_TERRAIN_MOVEMENT_COST", terrain.getMovementCost()));
	}

	if (terrain.getBuildModifier() != 0)
	{
		szBuffer.append(gDLL->getText("TXT_KEY_TERRAIN_BUILD_MODIFIER", terrain.getBuildModifier()));
	}

	if (terrain.getDefenseModifier() != 0)
	{
		szBuffer.append(gDLL->getText("TXT_KEY_TERRAIN_DEFENSE_MODIFIER", terrain.getDefenseModifier()));
	}

	if (terrain.isImpassable())
	{
		szBuffer.append(gDLL->getText("TXT_KEY_TERRAIN_IMPASSABLE"));
	}
	if (!terrain.isFound())
	{
		szBuffer.append(gDLL->getText("TXT_KEY_TERRAIN_NO_CITIES"));
		bool bFirst = true;
		if (terrain.isFoundCoast())
		{
			szBuffer.append(gDLL->getText("TXT_KEY_TERRAIN_COASTAL_CITIES"));
			bFirst = false;
		}
		if (!bFirst)
		{
			szBuffer.append(gDLL->getText("TXT_KEY_OR"));
		}
		if (terrain.isFoundFreshWater())
		{
			szBuffer.append(gDLL->getText("TXT_KEY_TERRAIN_FRESH_WATER_CITIES"));
			bFirst = false;
		}
	}
}

void CvGameTextMgr::buildFinanceInflationString(CvWStringBuffer& szBuffer, PlayerTypes ePlayer)
{
	if (NO_PLAYER == ePlayer)
		return;
	CvPlayer& kPlayer = GET_PLAYER(ePlayer);

	int iInflationRate = kPlayer.calculateInflationRate();
	//if (iInflationRate != 0)
	{
		int iPreInflation = kPlayer.calculatePreInflatedCosts();
		CvWString szTmp; // advc.086: Get rid of newline set in XML
		szTmp.append(NEWLINE);
		szTmp.append(gDLL->getText("TXT_KEY_FINANCE_ADVISOR_INFLATION",
				iPreInflation, iInflationRate, iInflationRate, iPreInflation,
				iPreInflation * iInflationRate / 100));
		// <advc.086>
		if(szBuffer.isEmpty())
			szBuffer.assign(szTmp.substr(2, szTmp.length()));
		else szBuffer.append(szTmp); // </advc.086>
	}
}

void CvGameTextMgr::buildFinanceUnitCostString(CvWStringBuffer& szBuffer, PlayerTypes ePlayer)
{
	if (NO_PLAYER == ePlayer)
		return;
	CvPlayer& player = GET_PLAYER(ePlayer);

	int iFreeUnits = 0;
	int iFreeMilitaryUnits = 0;
	int iUnits = player.getNumUnits();
	int iMilitaryUnits = player.getNumMilitaryUnits();
	int iPaidUnits = iUnits;
	int iPaidMilitaryUnits = iMilitaryUnits;
	int iMilitaryCost = 0;
	int iUnitCost = 0; // was called "base unit cost"
	int iExtraCost = 0;
	int iCost = player.calculateUnitCost(iFreeUnits, iFreeMilitaryUnits, iPaidUnits, iPaidMilitaryUnits, iUnitCost, iMilitaryCost, iExtraCost);
	int iHandicap = iCost-iUnitCost-iMilitaryCost-iExtraCost;

	// K-Mod include inflation
	int const inflFactor = 100+player.calculateInflationRate();
	iCost = ROUND_DIVIDE(iCost*inflFactor, 100);
	iUnitCost = ROUND_DIVIDE(iUnitCost*inflFactor, 100);
	iMilitaryCost = ROUND_DIVIDE(iMilitaryCost*inflFactor, 100);
	iHandicap = ROUND_DIVIDE(iHandicap*inflFactor, 100);
	// K-Mod end
	CvWString szTmp; // advc.086
	szTmp.append(NEWLINE);
	szTmp.append(gDLL->getText("TXT_KEY_FINANCE_ADVISOR_UNIT_COST",//iPaidUnits, iFreeUnits, iBaseUnitCost
			iUnits, iFreeUnits, iUnitCost)); // K-Mod
	// <advc.086>
	if(szBuffer.isEmpty())
		szBuffer.assign(szTmp.substr(2, szTmp.length()));
	else szBuffer.append(szTmp); // </advc.086>
	//if (iPaidMilitaryUnits != 0)
	if (iMilitaryCost != 0) // K-Mod
	{
		szBuffer.append(gDLL->getText("TXT_KEY_FINANCE_ADVISOR_UNIT_COST_2",//iPaidMilitaryUnits, iFreeMilitaryUnits, iMilitaryCost
				iMilitaryUnits, iFreeMilitaryUnits, iMilitaryCost)); // K-Mod
	}
	if (iExtraCost != 0)
		szBuffer.append(gDLL->getText("TXT_KEY_FINANCE_ADVISOR_UNIT_COST_3", iExtraCost));
	if (iHandicap != 0)
	{
		FAssertMsg(false, "not all unit costs were accounted for"); // K-Mod (handicap modifier are now rolled into the other costs)
		szBuffer.append(gDLL->getText("TXT_KEY_FINANCE_ADVISOR_HANDICAP_COST", iHandicap));
		szBuffer.append(CvWString::format(L" (%+d%%)", GC.getHandicapInfo(player.getHandicapType()).getUnitCostPercent()-100)); // K-Mod
	}
	szBuffer.append(gDLL->getText("TXT_KEY_FINANCE_ADVISOR_UNIT_COST_4", iCost));
}

void CvGameTextMgr::buildFinanceAwaySupplyString(CvWStringBuffer& szBuffer, PlayerTypes ePlayer)
{
	if (NO_PLAYER == ePlayer)
		return;
	CvPlayer& player = GET_PLAYER(ePlayer);

	int iPaidUnits = 0;
	int iBaseCost = 0;
	int iCost = player.calculateUnitSupply(iPaidUnits, iBaseCost);
	int iHandicap = iCost - iBaseCost;

	// K-Mod include inflation
	int const inflFactor = 100+player.calculateInflationRate();
	iCost = ROUND_DIVIDE(iCost*inflFactor, 100);
	iBaseCost = ROUND_DIVIDE(iBaseCost*inflFactor, 100);
	iHandicap = ROUND_DIVIDE(iHandicap*inflFactor, 100);
	// K-Mod end

	CvWString szHandicap;
	if (iHandicap != 0)
	{
		FAssertMsg(false, "not all supply costs were accounted for"); // K-Mod (handicap modifier are now rolled into the other costs)
		szHandicap = gDLL->getText("TXT_KEY_FINANCE_ADVISOR_HANDICAP_COST", iHandicap);
	}
	CvWString szTmp; // advc.086
	szTmp.append(NEWLINE);
	szTmp.append(gDLL->getText("TXT_KEY_FINANCE_ADVISOR_SUPPLY_COST",
			//iPaidUnits, GC.getDefineINT("INITIAL_FREE_OUTSIDE_UNITS"),
			// K-Mod:
			player.getNumOutsideUnits(), player.getNumOutsideUnits() - iPaidUnits,
			iBaseCost, szHandicap.GetCString(), iCost));
	// <advc.086>
	if(szBuffer.isEmpty())
		szBuffer.assign(szTmp.substr(2, szTmp.length()));
	else szBuffer.append(szTmp); // </advc.086>
}

void CvGameTextMgr::buildFinanceCityMaintString(CvWStringBuffer& szBuffer, PlayerTypes ePlayer)
{
	if (NO_PLAYER == ePlayer)
		return;
	int iLoop;
	int iDistanceMaint = 0;
	int iColonyMaint = 0;
	int iCorporationMaint = 0;

	CvPlayer& player = GET_PLAYER(ePlayer);
	for (CvCity* pLoopCity = player.firstCity(&iLoop); pLoopCity != NULL; pLoopCity = player.nextCity(&iLoop))
	{
/**
*** K-Mod, 06/sep/10, Karadoc
*** Bug fix
**/

		/* Old BTS code
		iDistanceMaint += (pLoopCity->calculateDistanceMaintenanceTimes100() * std::max(0, (pLoopCity->getMaintenanceModifier() + 100))) / 100;
		iColonyMaint += (pLoopCity->calculateColonyMaintenanceTimes100() * std::max(0, (pLoopCity->getMaintenanceModifier() + 100))) / 100;
		iCorporationMaint += (pLoopCity->calculateCorporationMaintenanceTimes100() * std::max(0, (pLoopCity->getMaintenanceModifier() + 100))) / 100;
		*/
		if (!pLoopCity->isDisorder() && !pLoopCity->isWeLoveTheKingDay() && (pLoopCity->getPopulation() > 0))
		{
			iDistanceMaint += (pLoopCity->calculateDistanceMaintenanceTimes100() * std::max(0, (pLoopCity->getMaintenanceModifier() + 100))+50) / 100;
			iColonyMaint += (pLoopCity->calculateColonyMaintenanceTimes100() * std::max(0, (pLoopCity->getMaintenanceModifier() + 100))+50) / 100;
			iCorporationMaint += (pLoopCity->calculateCorporationMaintenanceTimes100() * std::max(0, (pLoopCity->getMaintenanceModifier() + 100))+50) / 100;
		}
/**
*** K-Mod end
**/
	}
	// K-Mod. Changed to include the effects of inflation.
	int iInflationFactor = player.calculateInflationRate()+100;

	iDistanceMaint = ROUND_DIVIDE(iDistanceMaint * iInflationFactor, 10000);
	iColonyMaint = ROUND_DIVIDE(iColonyMaint * iInflationFactor, 10000);
	iCorporationMaint = ROUND_DIVIDE(iCorporationMaint * iInflationFactor, 10000); // Note: currently, calculateCorporationMaintenanceTimes100 includes the inverse of this factor.

	int iNumCityMaint = ROUND_DIVIDE(player.getTotalMaintenance() * iInflationFactor, 100) - iDistanceMaint - iColonyMaint - iCorporationMaint;
	CvWString szTmp; // advc.086
	szTmp.append(NEWLINE);
	szTmp.append(gDLL->getText("TXT_KEY_FINANCE_ADVISOR_CITY_MAINT_COST",
			iDistanceMaint, iNumCityMaint, iColonyMaint, iCorporationMaint,
			player.getTotalMaintenance()*iInflationFactor/100));
	// <advc.086>
	if(szBuffer.isEmpty())
		szBuffer.assign(szTmp.substr(2, szTmp.length()));
	else szBuffer.append(szTmp); // </advc.086>

}

void CvGameTextMgr::buildFinanceCivicUpkeepString(CvWStringBuffer& szBuffer, PlayerTypes ePlayer)
{
	if (NO_PLAYER == ePlayer)
		return;
	CvPlayer& player = GET_PLAYER(ePlayer);
	CvWString szCivicOptionCosts;

	int inflFactor = 100+player.calculateInflationRate();

	for (int iI = 0; iI < GC.getNumCivicOptionInfos(); ++iI)
	{
		CivicTypes eCivic = player.getCivics((CivicOptionTypes)iI);
		if (NO_CIVIC != eCivic)
		{
			CvWString szTemp;
			//szTemp.Format(L"%d%c: %s", player.getSingleCivicUpkeep(eCivic), GC.getCommerceInfo(COMMERCE_GOLD).getChar(),  GC.getCivicInfo(eCivic).getDescription());
			szTemp.Format(L"%d%c: %s", ROUND_DIVIDE(player.getSingleCivicUpkeep(eCivic)*inflFactor,100), GC.getCommerceInfo(COMMERCE_GOLD).getChar(), GC.getCivicInfo(eCivic).getDescription()); // K-Mod
			szCivicOptionCosts += NEWLINE + szTemp;
		}
	}
	CvWString szTmp; // advc.086
	szTmp.append(NEWLINE);
	szTmp.append(gDLL->getText("TXT_KEY_FINANCE_ADVISOR_CIVIC_UPKEEP_COST",
			szCivicOptionCosts.GetCString(),
			//player.getCivicUpkeep()));
			ROUND_DIVIDE(player.getCivicUpkeep()*inflFactor,100))); // K-Mod
	// <advc.086>
	if(szBuffer.isEmpty())
		szBuffer.assign(szTmp.substr(2, szTmp.length()));
	else szBuffer.append(szTmp); // </advc.086>
}

void CvGameTextMgr::buildFinanceForeignIncomeString(CvWStringBuffer& szBuffer, PlayerTypes ePlayer)
{
	if (NO_PLAYER == ePlayer)
		return;
	CvPlayer& player = GET_PLAYER(ePlayer);

	CvWString szPlayerIncome;
	for (int iI = 0; iI < MAX_PLAYERS; ++iI)
	{
		CvPlayer& otherPlayer = GET_PLAYER((PlayerTypes)iI);
		if (otherPlayer.isAlive() && player.getGoldPerTurnByPlayer((PlayerTypes)iI) != 0)
		{
			CvWString szTemp;
			szTemp.Format(L"%d%c: %s", player.getGoldPerTurnByPlayer((PlayerTypes)iI), GC.getCommerceInfo(COMMERCE_GOLD).getChar(), otherPlayer.getCivilizationShortDescription());
			szPlayerIncome += NEWLINE + szTemp;
		}
	}
	if (!szPlayerIncome.empty())
	{
		CvWString szTmp; // advc.086
		szTmp.append(NEWLINE);
		szTmp.append(gDLL->getText("TXT_KEY_FINANCE_ADVISOR_FOREIGN_INCOME", szPlayerIncome.GetCString(), player.getGoldPerTurn()));
		// <advc.086>
		if(szBuffer.isEmpty())
			szBuffer.assign(szTmp.substr(2, szTmp.length()));
		else szBuffer.append(szTmp); // </advc.086>
	}
}

// BUG - Building Additional Yield - start
bool CvGameTextMgr::setBuildingAdditionalYieldHelp(CvWStringBuffer &szBuffer, const CvCity& city, YieldTypes eIndex, const CvWString& szStart, bool bStarted)
{
	CvYieldInfo& info = GC.getYieldInfo(eIndex);
	CvWString szLabel;
	CvCivilizationInfo& kCivilization = GC.getCivilizationInfo(GET_PLAYER(city.getOwner()).getCivilizationType());

	for (int iI = 0; iI < GC.getNumBuildingClassInfos(); iI++)
	{
		BuildingTypes eBuilding = (BuildingTypes)kCivilization.getCivilizationBuildings((BuildingClassTypes)iI);

		if (eBuilding != NO_BUILDING && city.canConstruct(eBuilding, false, true, false))
		{
			int iChange = city.getAdditionalYieldByBuilding(eIndex, eBuilding);

			if (iChange != 0)
			{
				if (!bStarted)
				{
					szBuffer.append(szStart);
					bStarted = true;
				}

				szLabel.Format(SETCOLR L"%s" ENDCOLR, TEXT_COLOR("COLOR_BUILDING_TEXT"), GC.getBuildingInfo(eBuilding).getDescription());
				setResumableValueChangeHelp(szBuffer, szLabel, L": ", L"", iChange, info.getChar(), false, true);
			}
		}
	}

	return bStarted;
}
// BUG - Building Additional Yield - end

// BUG - Building Additional Commerce - start
bool CvGameTextMgr::setBuildingAdditionalCommerceHelp(CvWStringBuffer &szBuffer, const CvCity& city, CommerceTypes eIndex, const CvWString& szStart, bool bStarted)
{
	CvCommerceInfo& info = GC.getCommerceInfo(eIndex);
	CvWString szLabel;
	CvCivilizationInfo& kCivilization = GC.getCivilizationInfo(GET_PLAYER(city.getOwner()).getCivilizationType());

	for (int iI = 0; iI < GC.getNumBuildingClassInfos(); iI++)
	{
		BuildingTypes eBuilding = (BuildingTypes)kCivilization.getCivilizationBuildings((BuildingClassTypes)iI);

		if (eBuilding != NO_BUILDING && city.canConstruct(eBuilding, false, true, false))
		{
			int iChange = city.getAdditionalCommerceTimes100ByBuilding(eIndex, eBuilding);

			if (iChange != 0)
			{
				if (!bStarted)
				{
					szBuffer.append(szStart);
					bStarted = true;
				}

				szLabel.Format(SETCOLR L"%s" ENDCOLR, TEXT_COLOR("COLOR_BUILDING_TEXT"), GC.getBuildingInfo(eBuilding).getDescription());
				setResumableValueTimes100ChangeHelp(szBuffer, szLabel, L": ", L"", iChange, info.getChar(), true);
			}
		}
	}

	return bStarted;
}
// BUG - Building Additional Commerce - end

// BUG - Building Saved Maintenance - start
bool CvGameTextMgr::setBuildingSavedMaintenanceHelp(CvWStringBuffer &szBuffer, const CvCity& city, const CvWString& szStart, bool bStarted)
{
	CvCommerceInfo& info = GC.getCommerceInfo(COMMERCE_GOLD);
	CvWString szLabel;
	CvCivilizationInfo& kCivilization = GC.getCivilizationInfo(GET_PLAYER(city.getOwner()).getCivilizationType());

	for (int iI = 0; iI < GC.getNumBuildingClassInfos(); iI++)
	{
		BuildingTypes eBuilding = (BuildingTypes)kCivilization.getCivilizationBuildings((BuildingClassTypes)iI);

		if (eBuilding != NO_BUILDING && city.canConstruct(eBuilding, false, true, false))
		{
			int iChange = city.getSavedMaintenanceTimes100ByBuilding(eBuilding);

			if (iChange != 0)
			{
				if (!bStarted)
				{
					szBuffer.append(szStart);
					bStarted = true;
				}

				szLabel.Format(SETCOLR L"%s" ENDCOLR, TEXT_COLOR("COLOR_BUILDING_TEXT"), GC.getBuildingInfo(eBuilding).getDescription());
				setResumableValueTimes100ChangeHelp(szBuffer, szLabel, L": ", L"", iChange, info.getChar(), true);
			}
		}
	}

	return bStarted;
}
// BUG - Building Saved Maintenance - end

void CvGameTextMgr::setProductionHelp(CvWStringBuffer &szBuffer, CvCity const& city)
{
	FAssertMsg(NO_PLAYER != city.getOwner(), "City must have an owner");

	bool bProcess = city.isProductionProcess();
	// advc.064b: To be displayed at the end; iPastOverflow also moved.
	int iFromChopsUnused = city.getFeatureProduction();
	// <advc.064b>
	int iFromChops = 0;
	city.getCurrentProductionDifference(false, !bProcess, false, false, false, &iFromChops);
	if(!city.isProduction())
		iFromChops = iFromChopsUnused;
	iFromChopsUnused -= iFromChops; // </advc.064b>
	if (iFromChops != 0)
	{
		szBuffer.append(gDLL->getText("TXT_KEY_MISC_HELP_PROD_CHOPS", iFromChops));
		szBuffer.append(NEWLINE);
	}

	//if (city.getCurrentProductionDifference(false, true) == 0)
// BUG - Building Additional Production - start
	bool bBuildingAdditionalYield = (getBugOptionBOOL("MiscHover__BuildingAdditionalProduction", false)
			|| GC.altKey()); // advc.063
	if (city.getCurrentProductionDifference(false, true) == 0 && !bBuildingAdditionalYield)
// BUG - Building Additional Production - end
	{
		return;
	}

	setYieldHelp(szBuffer, city, YIELD_PRODUCTION);

	/*  advc.064b: Moved down b/c the generic bonuses in setYieldHelp no longer
		apply to iPastOverflow */
	int iPastOverflow = city.getOverflowProduction();
	if (iPastOverflow != 0 && !bProcess) // advc.064b: Display it later if bProcess
	{
		szBuffer.append(gDLL->getText("TXT_KEY_MISC_HELP_PROD_OVERFLOW", iPastOverflow));
		szBuffer.append(NEWLINE);
	}
	// advc.064b: Don't add iOverflow yet
	int iBaseProduction = city.getBaseYieldRate(YIELD_PRODUCTION) + iFromChops;
	/*  advc (note): This is the sum of all modifiers that apply to iBaseProduction
		(not to food production) */
	int iBaseModifier = city.getBaseYieldRateModifier(YIELD_PRODUCTION);

	UnitTypes eUnit = city.getProductionUnit();
	if (NO_UNIT != eUnit)
	{
		CvUnitInfo& unit = GC.getUnitInfo(eUnit);

		// Domain
		int iDomainMod = city.getDomainProductionModifier((DomainTypes)unit.getDomainType());
		if (iDomainMod != 0)
		{
			szBuffer.append(gDLL->getText("TXT_KEY_MISC_HELP_PROD_DOMAIN", iDomainMod, GC.getDomainInfo((DomainTypes)unit.getDomainType()).getTextKeyWide()));
			szBuffer.append(NEWLINE);
			iBaseModifier += iDomainMod;
		}

		// Military
		if (unit.isMilitaryProduction())
		{
			int iMilitaryMod = city.getMilitaryProductionModifier() + GET_PLAYER(city.getOwner()).getMilitaryProductionModifier();
			if (iMilitaryMod != 0)
			{
				szBuffer.append(gDLL->getText("TXT_KEY_MISC_HELP_PROD_MILITARY", iMilitaryMod));
				szBuffer.append(NEWLINE);
				iBaseModifier += iMilitaryMod;
			}
		}

		// Bonus
		for (int i = 0; i < GC.getNumBonusInfos(); i++)
		{
			if (city.hasBonus((BonusTypes)i))
			{
				int iBonusMod = unit.getBonusProductionModifier(i);
				if (iBonusMod != 0)
				{
					szBuffer.append(gDLL->getText("TXT_KEY_MISC_HELP_PROD_MOD_BONUS", iBonusMod, unit.getTextKeyWide(), GC.getBonusInfo((BonusTypes)i).getTextKeyWide()));
					szBuffer.append(NEWLINE);
					iBaseModifier += iBonusMod;
				}
			}
		}

		// Trait
		for (int i = 0; i < GC.getNumTraitInfos(); i++)
		{
			if (city.hasTrait((TraitTypes)i))
			{
				int iTraitMod = unit.getProductionTraits(i);

				if (unit.getSpecialUnitType() != NO_SPECIALUNIT)
				{
					iTraitMod += GC.getSpecialUnitInfo((SpecialUnitTypes) unit.getSpecialUnitType()).getProductionTraits(i);
				}
				if (iTraitMod != 0)
				{
					szBuffer.append(gDLL->getText("TXT_KEY_MISC_HELP_PROD_TRAIT", iTraitMod, unit.getTextKeyWide(), GC.getTraitInfo((TraitTypes)i).getTextKeyWide()));
					szBuffer.append(NEWLINE);
					iBaseModifier += iTraitMod;
				}
			}
		}

		// Religion
		if (NO_PLAYER != city.getOwner() && NO_RELIGION != GET_PLAYER(city.getOwner()).getStateReligion())
		{
			if (city.isHasReligion(GET_PLAYER(city.getOwner()).getStateReligion()))
			{
				int iReligionMod = GET_PLAYER(city.getOwner()).getStateReligionUnitProductionModifier();
				if (iReligionMod != 0)
				{
					szBuffer.append(gDLL->getText("TXT_KEY_MISC_HELP_PROD_RELIGION", iReligionMod, GC.getReligionInfo(GET_PLAYER(city.getOwner()).getStateReligion()).getTextKeyWide()));
					szBuffer.append(NEWLINE);
					iBaseModifier += iReligionMod;
				}
			}
		}
	}

	BuildingTypes eBuilding = city.getProductionBuilding();
	if (NO_BUILDING != eBuilding)
	{
		CvBuildingInfo& building = GC.getBuildingInfo(eBuilding);

		// Bonus
		for (int i = 0; i < GC.getNumBonusInfos(); i++)
		{
			if (city.hasBonus((BonusTypes)i))
			{
				int iBonusMod = building.getBonusProductionModifier(i);
				if (iBonusMod != 0)
				{
					szBuffer.append(gDLL->getText("TXT_KEY_MISC_HELP_PROD_MOD_BONUS", iBonusMod, building.getTextKeyWide(), GC.getBonusInfo((BonusTypes)i).getTextKeyWide()));
					szBuffer.append(NEWLINE);
					iBaseModifier += iBonusMod;
				}
			}
		}

		// Trait
		for (int i = 0; i < GC.getNumTraitInfos(); i++)
		{
			if (city.hasTrait((TraitTypes)i))
			{
				int iTraitMod = building.getProductionTraits(i);

				if (building.getSpecialBuildingType() != NO_SPECIALBUILDING)
				{
					iTraitMod += GC.getSpecialBuildingInfo((SpecialBuildingTypes) building.getSpecialBuildingType()).getProductionTraits(i);
				}
				if (iTraitMod != 0)
				{
					szBuffer.append(gDLL->getText("TXT_KEY_MISC_HELP_PROD_TRAIT", iTraitMod, building.getTextKeyWide(), GC.getTraitInfo((TraitTypes)i).getTextKeyWide()));
					szBuffer.append(NEWLINE);
					iBaseModifier += iTraitMod;
				}
			}
		}

		// Wonder
		if (isWorldWonderClass((BuildingClassTypes)(GC.getBuildingInfo(eBuilding).getBuildingClassType())) && NO_PLAYER != city.getOwner())
		{
			int iWonderMod = GET_PLAYER(city.getOwner()).getMaxGlobalBuildingProductionModifier();
			if (iWonderMod != 0)
			{
				szBuffer.append(gDLL->getText("TXT_KEY_MISC_HELP_PROD_WONDER", iWonderMod));
				szBuffer.append(NEWLINE);
				iBaseModifier += iWonderMod;
			}
		}

		// Team Wonder
		if (isTeamWonderClass((BuildingClassTypes)(GC.getBuildingInfo(eBuilding).getBuildingClassType())) && NO_PLAYER != city.getOwner())
		{
			int iWonderMod = GET_PLAYER(city.getOwner()).getMaxTeamBuildingProductionModifier();
			if (iWonderMod != 0)
			{
				szBuffer.append(gDLL->getText("TXT_KEY_MISC_HELP_PROD_TEAM_WONDER", iWonderMod));
				szBuffer.append(NEWLINE);
				iBaseModifier += iWonderMod;
			}
		}

		// National Wonder
		if (isNationalWonderClass((BuildingClassTypes)(GC.getBuildingInfo(eBuilding).getBuildingClassType())) && NO_PLAYER != city.getOwner())
		{
			int iWonderMod = GET_PLAYER(city.getOwner()).getMaxPlayerBuildingProductionModifier();
			if (iWonderMod != 0)
			{
				szBuffer.append(gDLL->getText("TXT_KEY_MISC_HELP_PROD_NATIONAL_WONDER", iWonderMod));
				szBuffer.append(NEWLINE);
				iBaseModifier += iWonderMod;
			}
		}

		// Religion
		if (NO_PLAYER != city.getOwner() && NO_RELIGION != GET_PLAYER(city.getOwner()).getStateReligion())
		{
			if (city.isHasReligion(GET_PLAYER(city.getOwner()).getStateReligion()))
			{
				int iReligionMod = GET_PLAYER(city.getOwner()).getStateReligionBuildingProductionModifier();
				if (iReligionMod != 0)
				{
					szBuffer.append(gDLL->getText("TXT_KEY_MISC_HELP_PROD_RELIGION", iReligionMod, GC.getReligionInfo(GET_PLAYER(city.getOwner()).getStateReligion()).getTextKeyWide()));
					szBuffer.append(NEWLINE);
					iBaseModifier += iReligionMod;
				}
			}
		}
	}

	ProjectTypes eProject = city.getProductionProject();
	if (NO_PROJECT != eProject)
	{
		CvProjectInfo& project = GC.getProjectInfo(eProject);

		// Spaceship
		if (project.isSpaceship())
		{
			int iSpaceshipMod = city.getSpaceProductionModifier();
			if (NO_PLAYER != city.getOwner())
			{
				iSpaceshipMod += GET_PLAYER(city.getOwner()).getSpaceProductionModifier();
			}
			if (iSpaceshipMod != 0)
			{
				szBuffer.append(gDLL->getText("TXT_KEY_MISC_HELP_PROD_SPACESHIP", iSpaceshipMod));
				szBuffer.append(NEWLINE);
				iBaseModifier += iSpaceshipMod;
			}
		}

		// Bonus
		for (int i = 0; i < GC.getNumBonusInfos(); i++)
		{
			if (city.hasBonus((BonusTypes)i))
			{
				int iBonusMod = project.getBonusProductionModifier(i);
				if (iBonusMod != 0)
				{
					szBuffer.append(gDLL->getText("TXT_KEY_MISC_HELP_PROD_MOD_BONUS", iBonusMod, project.getTextKeyWide(), GC.getBonusInfo((BonusTypes)i).getTextKeyWide()));
					szBuffer.append(NEWLINE);
					iBaseModifier += iBonusMod;
				}
			}
		}
	}

	int iFoodProduction = (city.isFoodProduction() ? std::max(0, (city.getYieldRate(YIELD_FOOD) - city.foodConsumption(true))) : 0);
	if (iFoodProduction > 0)
	{
		szBuffer.append(gDLL->getText("TXT_KEY_MISC_HELP_PROD_FOOD", iFoodProduction, iFoodProduction));
		szBuffer.append(NEWLINE);
	} // advc.064b: To match the change in CvCity::getProductionDifference
	int iOverflowModifier = iBaseModifier - city.getBaseYieldRateModifier(YIELD_PRODUCTION);
	int iModProduction = iFoodProduction + (iBaseModifier * iBaseProduction
			// advc.064b:
			+ (bProcess ? 0 : (iPastOverflow * (100 + iOverflowModifier)))) / 100;
	FAssertMsg(iModProduction == city.getCurrentProductionDifference(false, !bProcess)
			// advc.064b: Display and rules don't quite align when no production chosen
			+ (city.isProduction() ? 0 : city.getFeatureProduction()),
			"Modified Production does not match actual value");

	szBuffer.append(gDLL->getText("TXT_KEY_MISC_HELP_PROD_FINAL_YIELD", iModProduction));
	// <advc.064b>
	OrderTypes eOrderType = city.getOrderData(0).eOrderType;
	int iNewOverflow = 0;
	if(eOrderType != NO_ORDER && !bProcess) {
		iNewOverflow = iModProduction + city.getProduction() - city.getProductionNeeded();
		int iProductionGold = 0;
		int iLostProduction = 0;
		FAssertMsg((eUnit != NO_UNIT && iOverflowModifier == city.getProductionModifier(eUnit)) ||
				(eBuilding != NO_BUILDING && iOverflowModifier == city.getProductionModifier(eBuilding)) ||
				(eProject != NO_PROJECT && iOverflowModifier == city.getProductionModifier(eProject)),
				"Displayed modifier inconsistent with the one computed by CvCity");
		iNewOverflow = city.computeOverflow(iNewOverflow, iOverflowModifier,
				eOrderType, &iProductionGold, &iLostProduction);
		if(iLostProduction > 0) {
			szBuffer.append(NEWLINE);
			szBuffer.append(gDLL->getText("TXT_KEY_MISC_HELP_PROD_GOLD",
					iLostProduction, iProductionGold));
		}
	}
	else if(eOrderType != NO_ORDER)
		iNewOverflow = iPastOverflow;
	if(iNewOverflow > 0) {
		szBuffer.append(NEWLINE);
		szBuffer.append(gDLL->getText("TXT_KEY_MISC_HELP_PROD_NEW_OVERFLOW",
				iNewOverflow));
	}
	if(iFromChopsUnused != 0) {
		szBuffer.append(NEWLINE);
		szBuffer.append(gDLL->getText("TXT_KEY_MISC_HELP_PROD_CHOPS_UNUSED",
				iFromChopsUnused));
	} // </advc.064b>
	// BUG - Building Additional Production - start
	if (bBuildingAdditionalYield && city.getOwner() == GC.getGame().getActivePlayer())
		setBuildingAdditionalYieldHelp(szBuffer, city, YIELD_PRODUCTION, DOUBLE_SEPARATOR);
	// BUG - Building Additional Production - end
}


void CvGameTextMgr::parsePlayerTraits(CvWStringBuffer &szBuffer, PlayerTypes ePlayer)
{
	bool bFirst = true;

	for (int iTrait = 0; iTrait < GC.getNumTraitInfos(); ++iTrait)
	{
		if (GET_PLAYER(ePlayer).hasTrait((TraitTypes)iTrait))
		{
			if (bFirst)
			{
				szBuffer.append(L" (");
				bFirst = false;
			}
			else
			{
				szBuffer.append(L", ");
			}
			szBuffer.append(GC.getTraitInfo((TraitTypes)iTrait).getDescription());
		}
	}

	if (!bFirst)
	{
		szBuffer.append(L")");
	}
}

// K-Mod. I've rewritten most of this function.
void CvGameTextMgr::parseLeaderHeadHelp(CvWStringBuffer &szBuffer, PlayerTypes eThisPlayer, PlayerTypes eOtherPlayer)
{
	if (eThisPlayer == NO_PLAYER)
		return;

	const CvPlayerAI& kPlayer = GET_PLAYER(eThisPlayer);

	szBuffer.append(CvWString::format(SETCOLR L"%s" ENDCOLR, TEXT_COLOR("COLOR_HIGHLIGHT_TEXT"), kPlayer.getName()));

	parsePlayerTraits(szBuffer, eThisPlayer);

	// Some debug info: found-site traits, and AI flavours
	if (//gDLL->getChtLvl() > 0
			GC.getGame().isDebugMode() // advc.135c
			&& GC.altKey())
	{
		szBuffer.append(CvWString::format(SETCOLR SEPARATOR NEWLINE, TEXT_COLOR("COLOR_LIGHT_GREY")));
		szBuffer.append(CvWString::format(L"id=%d\n", eThisPlayer)); // advc.007
		CvPlayerAI::CvFoundSettings kFoundSet(kPlayer, false);

		bool bFirst = true;

#define trait_info(x) do { \
	if (kFoundSet.b##x) \
	{ \
		szBuffer.append(CvWString::format(L"%s"L#x, bFirst? L"" : L", ")); \
		bFirst = false; \
	} \
} while (0)

		trait_info(Ambitious);
		trait_info(Defensive);
		trait_info(EasyCulture);
		trait_info(Expansive);
		trait_info(Financial);
		trait_info(Seafaring);

#undef trait_info

#define flavour_info(x) do { \
	if (kPlayer.AI_getFlavorValue(FLAVOR_##x)) \
	{ \
		szBuffer.append(CvWString::format(L"%s"L#x L"=%d", bFirst? L"" : L", ", kPlayer.AI_getFlavorValue(FLAVOR_##x))); \
		bFirst = false; \
	} \
} while (0)

		flavour_info(MILITARY);
		flavour_info(RELIGION);
		flavour_info(PRODUCTION);
		flavour_info(GOLD);
		flavour_info(SCIENCE);
		flavour_info(CULTURE);
		flavour_info(GROWTH);

#undef flavour_info

		szBuffer.append(SEPARATOR ENDCOLR);
	}

	//szBuffer.append(L"\n");

	if (eOtherPlayer == NO_PLAYER)
	{
		getWarWearinessString(szBuffer, eThisPlayer, NO_PLAYER); // total war weariness
		return; // advc.003
	}
	CvTeam& kThisTeam = GET_TEAM(kPlayer.getTeam());
	TeamTypes eOtherTeam = GET_PLAYER(eOtherPlayer).getTeam();
	if (!kThisTeam.isHasMet(eOtherTeam))
		return;
	getVassalInfoString(szBuffer, eThisPlayer); // K-Mod
	// disabled by K-Mod. (The player should not be told exactly how many espionage points everyone has.)
	//getEspionageString(szBuffer, eThisPlayer, eOtherPlayer);
	if (eOtherPlayer != eThisPlayer)
	{
		getAttitudeString(szBuffer, eThisPlayer, eOtherPlayer);
		getActiveDealsString(szBuffer, eThisPlayer, eOtherPlayer);
	}
	getWarWearinessString(szBuffer, eThisPlayer, eOtherPlayer);
	// K-Mod. Allow the "other relations string" to display even if eOtherPlayer == eThisPlayer. It's useful info.
	getOtherRelationsString(szBuffer, eThisPlayer, eOtherPlayer);
}

// <advc.152>
void CvGameTextMgr::parseWarTradesHelp(CvWStringBuffer& szBuffer,
		PlayerTypes eThisPlayer, PlayerTypes eOtherPlayer) {

	/*  Same checks as in getAttitudeText (AttitudeUtil.py), which displays
		the fist icon. */
	PlayerTypes eActivePlayer = GC.getGame().getActivePlayer();
	if(TEAMID(eOtherPlayer) == TEAMID(eActivePlayer) ||
			eOtherPlayer == NO_PLAYER || TEAMID(eThisPlayer) == TEAMID(eActivePlayer) ||
			TEAMID(eThisPlayer) == TEAMID(eOtherPlayer) || eThisPlayer == NO_PLAYER ||
			TEAMREF(eThisPlayer).isAtWar(TEAMID(eOtherPlayer)) ||
			TEAMREF(eOtherPlayer).isHuman())
		return;
	if(TEAMREF(eThisPlayer).AI_declareWarTrade(TEAMID(eOtherPlayer),
			TEAMID(eActivePlayer)) == NO_DENIAL) {
		szBuffer.append(NEWLINE);
		szBuffer.append(gDLL->getText("TXT_KEY_WILLING_START_WAR",
				GET_PLAYER(eOtherPlayer).getName()));
	}
} // </advc.152>


void CvGameTextMgr::parseLeaderLineHelp(CvWStringBuffer &szBuffer, PlayerTypes eThisPlayer, PlayerTypes eOtherPlayer)
{
	// advc.003: This is apparently dead code
	FAssertMsg(false, "Just checking if this function is ever even called");
	if (NO_PLAYER == eThisPlayer || NO_PLAYER == eOtherPlayer)
		return;

	CvTeam& thisTeam = GET_TEAM(GET_PLAYER(eThisPlayer).getTeam());
	CvTeam& otherTeam = GET_TEAM(GET_PLAYER(eOtherPlayer).getTeam());

	if (thisTeam.getID() == otherTeam.getID())
	{
		szBuffer.append(gDLL->getText("TXT_KEY_MISC_PERMANENT_ALLIANCE"));
		szBuffer.append(NEWLINE);
	}
	else if (thisTeam.isAtWar(otherTeam.getID()))
	{
		szBuffer.append(gDLL->getText("TXT_KEY_CONCEPT_WAR"));
		szBuffer.append(NEWLINE);
	}
	else
	{
		if (thisTeam.isDefensivePact(otherTeam.getID()))
		{
			szBuffer.append(gDLL->getText("TXT_KEY_MISC_DEFENSIVE_PACT"));
			szBuffer.append(NEWLINE);
		}
		if (thisTeam.isOpenBorders(otherTeam.getID()))
		{
			szBuffer.append(gDLL->getText("TXT_KEY_MISC_OPEN_BORDERS"));
			szBuffer.append(NEWLINE);
		} // <advc.034>
		else if(thisTeam.isDisengage(otherTeam.getID())) {
			CvWString szString;
			buildDisengageString(szString, eThisPlayer, eOtherPlayer);
			szBuffer.append(szString);
			szBuffer.append(NEWLINE);
		} // </advc.034>
		if (thisTeam.isVassal(otherTeam.getID()))
		{	// <advc.130v> Make clearer when a vassal is capitulated
			if(thisTeam.isCapitulated())
				szBuffer.append(gDLL->getText("TXT_KEY_ATTITUDE_VASSAL_CAP_OF",
						GET_TEAM(thisTeam.getMasterTeam()).getName().GetCString()));
			else szBuffer.append(gDLL->getText("TXT_KEY_ATTITUDE_VASSAL_OF",
					GET_TEAM(thisTeam.getMasterTeam()).getName().GetCString()));
			//szBuffer.append(gDLL->getText("TXT_KEY_MISC_VASSAL")); // </advc.130v>
			szBuffer.append(NEWLINE);
		}
	}
}


void CvGameTextMgr::getActiveDealsString(CvWStringBuffer &szBuffer,
		PlayerTypes eThisPlayer, PlayerTypes eOtherPlayer,
		bool bExludeDual) // advc.087
{
	int iIndex;
	CvDeal* pDeal = GC.getGame().firstDeal(&iIndex);
	while (NULL != pDeal)
	{
		if ((pDeal->getFirstPlayer() == eThisPlayer && pDeal->getSecondPlayer() == eOtherPlayer)
			|| (pDeal->getFirstPlayer() == eOtherPlayer && pDeal->getSecondPlayer() == eThisPlayer))
		{	// <advc.087>
			if(bExludeDual) {
				bool bDual = true;
				CLLNode<TradeData>* pNode;
				for(pNode = pDeal->headFirstTradesNode(); pNode != NULL;
						pNode = pDeal->nextFirstTradesNode(pNode)) {
					if(!CvDeal::isDual(pNode->m_data.m_eItemType)) {
						bDual = false;
						break;
					}
				}
				for(pNode = pDeal->headSecondTradesNode(); pNode != NULL;
						pNode = pDeal->nextSecondTradesNode(pNode)) {
					if(!CvDeal::isDual(pNode->m_data.m_eItemType)) {
						bDual = false;
						break;
					}
				}
				if(bDual) {
					pDeal = GC.getGame().nextDeal(&iIndex);
					continue;
				}
			} // </advc.087>
			szBuffer.append(NEWLINE);
			szBuffer.append(CvWString::format(L"%c", gDLL->getSymbolID(BULLET_CHAR)));
			getDealString(szBuffer, *pDeal, eThisPlayer);
		}
		pDeal = GC.getGame().nextDeal(&iIndex);
	}
}

/*  advc.004: Merged into BULL's CvGameTextMgr::getOtherRelationsString that operates
	on teams. Tbd.: Delete this once the BULL function is sufficiently tested. */
/*void CvGameTextMgr::getOtherRelationsString(CvWStringBuffer& szString,
		PlayerTypes eThisPlayer, PlayerTypes eOtherPlayer) {
	if(eThisPlayer == NO_PLAYER || eOtherPlayer == NO_PLAYER)
		return;
	CvPlayer const& kThisPlayer = GET_PLAYER(eThisPlayer);
	CvPlayer const& kOtherPlayer = GET_PLAYER(eOtherPlayer);
	// BETTER_BTS_AI, 11/08/08, jdog5000, START:
	// original code (advc: deleted)
	// Put all war, worst enemy strings on one line
	CvWStringBuffer szWarWithString;
	CvWStringBuffer szWorstEnemyString;
	bool bFirst = true;
	bool bFirst2 = true;
	for(int i = 0; i < MAX_CIV_TEAMS; i++) {
		CvTeamAI const& kLoopTeam = GET_TEAM((TeamTypes)i);
		if(!kLoopTeam.isAlive() || kLoopTeam.isMinorCiv() || i == kThisPlayer.getTeam())
				// K-Mod. (show "at war" even for the civ selected.)
				//|| i != kOtherPlayer.getTeam())
			continue;
		if(!kLoopTeam.isHasMet(kOtherPlayer.getTeam()) ||
				!kLoopTeam.isHasMet(GC.getGame().getActiveTeam()))
			continue;
		// UNOFFICIAL_PATCH, 09/28/09, EmperorFool & jdog5000
		if(!kLoopTeam.isHasMet(kThisPlayer.getTeam()))
			continue; // UNOFFICIAL_PATCH: END
		if(::atWar(kLoopTeam.getID(), kThisPlayer.getTeam())) {
			setListHelp(szWarWithString, L"", kLoopTeam.getName().GetCString(), L", ", bFirst);
			bFirst = false;
		}
		if(!kLoopTeam.isHuman() && kLoopTeam.AI_getWorstEnemy() == kThisPlayer.getTeam()) {
			setListHelp(szWorstEnemyString, L"", kLoopTeam.getName().GetCString(), L", ", bFirst2);
			bFirst2 = false;
		}
	}
	if(!szWorstEnemyString.isEmpty()) {
		CvWString szTempBuffer;
		szTempBuffer.assign(gDLL->getText(L"TXT_KEY_WORST_ENEMY_OF", szWorstEnemyString.getCString()));
		szString.append(NEWLINE);
		szString.append(szTempBuffer);
	}
	if(!szWarWithString.isEmpty()) {
		CvWString szTempBuffer;
		szTempBuffer.assign(gDLL->getText(L"TXT_KEY_AT_WAR_WITH", szWarWithString.getCString()));
		szString.append(NEWLINE);
		szString.append(szTempBuffer);
	} // BETTER_BTS_AI: END
}*/

void CvGameTextMgr::buildHintsList(CvWStringBuffer& szBuffer)
{
	/*  <avdc.008d> Hints are no longer guaranteed to be unique, and this is
		bad when listing them in Civilopedia. Cleaner solution would be to keep
		them unique in XML, but add a frequency tag. However, I don't think the
		code that shows hints while loading is in the SDK, so this doesn't work,
		and the only way I see to affect the frequency is to define some hints
		multiple times in XML. */
	/*  Hopefully this uses a decent hash function, but, since it's only called
		when opening the Hints section of Civilopedia, it doesn't really matter. */
	std::set<CvWString> uniqueHints;
	for(int i = 0; i < GC.getNumHints(); i++)
		uniqueHints.insert(CvWString(GC.getHints(i).getText()));
	for(std::set<CvWString>::const_iterator it = uniqueHints.begin(); // </advc.008d>
			it != uniqueHints.end(); it++)
	{
		szBuffer.append(CvWString::format(L"%c%s", gDLL->getSymbolID(BULLET_CHAR),
				it->c_str())); //GC.getHints(i).getText())); // advc.008d
		szBuffer.append(NEWLINE);
		szBuffer.append(NEWLINE);
	}
}

void CvGameTextMgr::setCommerceHelp(CvWStringBuffer &szBuffer, CvCity const& kCity,
		CommerceTypes eCommerce) {  // advc.003: style changes
	// BUG - Building Additional Commerce - start
	bool bBuildingAdditionalCommerce = (getBugOptionBOOL("MiscHover__BuildingAdditionalCommerce", false)
			|| GC.altKey()); // advc.063
	if(NO_COMMERCE == eCommerce ||
			(kCity.getCommerceRateTimes100(eCommerce) == 0 &&
			!bBuildingAdditionalCommerce)
	// BUG - Building Additional Commerce - end
			|| kCity.isDisorder()) // advc.001
		return;
	CvCommerceInfo& kCommerce = GC.getCommerceInfo(eCommerce);
	int const iCommerceChar = kCommerce.getChar();

	if(kCity.getOwner() == NO_PLAYER)
		return;
	CvPlayer& kOwner = GET_PLAYER(kCity.getOwner());
	// <advc.004p>
	int iBaseCommerceRate = 0;
	CvWString szRate = "";
	if(kOwner.getCommercePercent(eCommerce) > 0) { // </advc.004p>
		setYieldHelp(szBuffer, kCity, YIELD_COMMERCE);
		// Slider
		iBaseCommerceRate = kCity.getCommerceFromPercent(eCommerce,
				kCity.getYieldRate(YIELD_COMMERCE) * 100);
		szRate = CvWString::format(L"%d.%02d",
				iBaseCommerceRate / 100, iBaseCommerceRate % 100);
		szBuffer.append(gDLL->getText("TXT_KEY_MISC_HELP_SLIDER_PERCENT_FLOAT",
				kOwner.getCommercePercent(eCommerce), kCity.getYieldRate(YIELD_COMMERCE),
				szRate.GetCString(), iCommerceChar));
		szBuffer.append(NEWLINE);
	} // advc.004p

	bool bNeedSubtotal = false; // BUG - Base Commerce

	int iSpecialistCommerce = kCity.getSpecialistCommerce(eCommerce) +
			(kCity.getSpecialistPopulation() + kCity.getNumGreatPeople()) *
			kOwner.getSpecialistExtraCommerce(eCommerce);
	if(iSpecialistCommerce != 0) {
		szBuffer.append(gDLL->getText("TXT_KEY_MISC_HELP_SPECIALIST_COMMERCE",
				iSpecialistCommerce, iCommerceChar, L"TXT_KEY_CONCEPT_SPECIALISTS"));
		szBuffer.append(NEWLINE);
		iBaseCommerceRate += 100 * iSpecialistCommerce;
		bNeedSubtotal = true; // BUG - Base Commerce
	}

	int iReligionCommerce = kCity.getReligionCommerce(eCommerce);
	if(iReligionCommerce != 0) {
		szBuffer.append(gDLL->getText("TXT_KEY_MISC_HELP_RELIGION_COMMERCE",
				iReligionCommerce, iCommerceChar));
		szBuffer.append(NEWLINE);
		iBaseCommerceRate += 100 * iReligionCommerce;
		bNeedSubtotal = true; // BUG - Base Commerce
	}

	int iCorporationCommerce = kCity.getCorporationCommerce(eCommerce);
	if(iCorporationCommerce != 0) {
		szBuffer.append(gDLL->getText("TXT_KEY_MISC_HELP_CORPORATION_COMMERCE",
				iCorporationCommerce, iCommerceChar));
		szBuffer.append(NEWLINE);
		iBaseCommerceRate += 100 * iCorporationCommerce;
		bNeedSubtotal = true; // BUG - Base Commerce
	}

	int iBuildingCommerce = kCity.getBuildingCommerce(eCommerce);
	if(iBuildingCommerce != 0) {
		szBuffer.append(gDLL->getText("TXT_KEY_MISC_HELP_BUILDING_COMMERCE",
				iBuildingCommerce, iCommerceChar));
		szBuffer.append(NEWLINE);
		iBaseCommerceRate += 100 * iBuildingCommerce;
		bNeedSubtotal = true; // BUG - Base Commerce
	}

	int iFreeCityCommerce = kOwner.getFreeCityCommerce(eCommerce);
	if(iFreeCityCommerce != 0) {
		szBuffer.append(gDLL->getText("TXT_KEY_MISC_HELP_FREE_CITY_COMMERCE",
				iFreeCityCommerce, iCommerceChar));
		szBuffer.append(NEWLINE);
		iBaseCommerceRate += 100 * iFreeCityCommerce;
		bNeedSubtotal = true; // BUG - Base Commerce
	}

	// BUG - Base Commerce - start
	if (bNeedSubtotal && kCity.getCommerceRateModifier(eCommerce) != 0
			// advc.065: No longer optional
			/*&& getBugOptionBOOL("MiscHover__BaseCommerce", false)*/) {
		CvWString szYield = CvWString::format(L"%d.%02d",
				iBaseCommerceRate / 100, iBaseCommerceRate % 100);
		szBuffer.append(gDLL->getText("TXT_KEY_MISC_HELP_COMMERCE_SUBTOTAL_YIELD_FLOAT",
				kCommerce.getTextKeyWide(), szYield.GetCString(), iCommerceChar));
		szBuffer.append(NEWLINE);
	} // BUG - Base Commerce - end

	FAssertMsg(kCity.getBaseCommerceRateTimes100(eCommerce) == iBaseCommerceRate,
			"Base Commerce rate does not agree with actual value");

	int iModifier = 100;

	// Buildings
	int iBuildingMod = 0;
	for(int i = 0; i < GC.getNumBuildingInfos(); i++) {
		BuildingTypes eBuilding = (BuildingTypes)i;
		CvBuildingInfo& kBuilding = GC.getBuildingInfo(eBuilding);
		if(kCity.getNumBuilding(eBuilding) > 0 &&
				!GET_TEAM(kCity.getTeam()).isObsoleteBuilding(eBuilding)) {
			for(int j = 0; j < kCity.getNumBuilding(eBuilding); j++)
				iBuildingMod += kBuilding.getCommerceModifier(eCommerce);
		}
		for(int j = 0; j < MAX_PLAYERS; j++) {
			CvPlayer const& kMember = GET_PLAYER((PlayerTypes)j);
			if (!kMember.isAlive() || kMember.getTeam() != kOwner.getTeam())
				continue;
			int iLoop;
			for(CvCity* pLoopCity = kMember.firstCity(&iLoop); pLoopCity != NULL;
					pLoopCity = kMember.nextCity(&iLoop)) {
				if(pLoopCity->getNumBuilding(eBuilding) > 0 &&
						!GET_TEAM(pLoopCity->getTeam()).isObsoleteBuilding(eBuilding)) {
					for(int k = 0; k < pLoopCity->getNumBuilding(eBuilding); k++)
						iBuildingMod += kBuilding.getGlobalCommerceModifier(eCommerce);
				}
			}
		}
	}
	if(iBuildingMod != 0) {
		szBuffer.append(gDLL->getText("TXT_KEY_MISC_HELP_YIELD_BUILDINGS",
				iBuildingMod, iCommerceChar));
		szBuffer.append(NEWLINE);
		iModifier += iBuildingMod;
	}

	// Trait
	for(int i = 0; i < GC.getNumTraitInfos(); i++) {
		if(!kCity.hasTrait((TraitTypes)i))
			continue;
		CvTraitInfo& kTrait = GC.getTraitInfo((TraitTypes)i);
		int iTraitMod = kTrait.getCommerceModifier(eCommerce);
		if(iTraitMod != 0) {
			szBuffer.append(gDLL->getText("TXT_KEY_MISC_HELP_COMMERCE_TRAIT", iTraitMod,
					iCommerceChar, kTrait.getTextKeyWide()));
			szBuffer.append(NEWLINE);
			iModifier += iTraitMod;
		}
	}

	// Capital
	int iCapitalMod = kCity.isCapital() ? kOwner.getCapitalCommerceRateModifier(eCommerce) : 0;
	if(iCapitalMod != 0) {
		szBuffer.append(gDLL->getText("TXT_KEY_MISC_HELP_YIELD_CAPITAL",
				iCapitalMod, iCommerceChar));
		szBuffer.append(NEWLINE);
		iModifier += iCapitalMod;
	}

	// Civics
	int iCivicMod = 0;
	for(int i = 0; i < GC.getNumCivicOptionInfos(); i++) {
		CivicOptionTypes eCivicOption = (CivicOptionTypes)i;
		if(kOwner.getCivics(eCivicOption) != NO_CIVIC)
			iCivicMod += GC.getCivicInfo(kOwner.getCivics(eCivicOption)).getCommerceModifier(eCommerce);
	}
	if(iCivicMod != 0) {
		szBuffer.append(gDLL->getText("TXT_KEY_MISC_HELP_YIELD_CIVICS", iCivicMod, iCommerceChar));
		szBuffer.append(NEWLINE);
		iModifier += iCivicMod;
	}

	int iModYield = (iModifier * iBaseCommerceRate) / 100;

	int iProductionToCommerce = kCity.getProductionToCommerceModifier(eCommerce) *
			kCity.getYieldRate(YIELD_PRODUCTION);
	if(iProductionToCommerce != 0) {
		if(iProductionToCommerce % 100 == 0) {
			szBuffer.append(gDLL->getText("TXT_KEY_MISC_HELP_PRODUCTION_TO_COMMERCE",
					iProductionToCommerce / 100, iCommerceChar));
			szBuffer.append(NEWLINE);
		}
		else {
			szRate = CvWString::format(L"+%d.%02d",
					iProductionToCommerce / 100, iProductionToCommerce % 100);
			szBuffer.append(gDLL->getText("TXT_KEY_MISC_HELP_PRODUCTION_TO_COMMERCE_FLOAT",
					szRate.GetCString(), iCommerceChar));
			szBuffer.append(NEWLINE);
		}
		iModYield += iProductionToCommerce;
	}

	if(eCommerce == COMMERCE_CULTURE && GC.getGame().isOption(GAMEOPTION_NO_ESPIONAGE)) {
		int iEspionageToCommerce = kCity.getCommerceRateTimes100(COMMERCE_CULTURE) - iModYield;
		if(iEspionageToCommerce != 0) {
			if(iEspionageToCommerce % 100 == 0) {
				szBuffer.append(gDLL->getText("TXT_KEY_MISC_HELP_COMMERCE_TO_COMMERCE",
						iEspionageToCommerce / 100, iCommerceChar,
						GC.getCommerceInfo(COMMERCE_ESPIONAGE).getChar()));
				szBuffer.append(NEWLINE);
			}
			else {
				szRate = CvWString::format(L"+%d.%02d",
						iEspionageToCommerce / 100, iEspionageToCommerce % 100);
				szBuffer.append(gDLL->getText("TXT_KEY_MISC_HELP_COMMERCE_TO_COMMERCE_FLOAT",
						szRate.GetCString(), iCommerceChar,
						GC.getCommerceInfo(COMMERCE_ESPIONAGE).getChar()));
				szBuffer.append(NEWLINE);
			}
			iModYield += iEspionageToCommerce;
		}
	}

	FAssertMsg(iModYield == kCity.getCommerceRateTimes100(eCommerce),
			"Commerce yield does not match actual value");

	CvWString szYield = CvWString::format(L"%d.%02d", iModYield / 100, iModYield % 100);
	szBuffer.append(gDLL->getText("TXT_KEY_MISC_HELP_COMMERCE_FINAL_YIELD_FLOAT",
			kCommerce.getTextKeyWide(), szYield.GetCString(), iCommerceChar));

	// BUG - Building Additional Commerce - start
	if(bBuildingAdditionalCommerce && kCity.getOwner() == GC.getGame().getActivePlayer())
		setBuildingAdditionalCommerceHelp(szBuffer, kCity, eCommerce, DOUBLE_SEPARATOR);
	// BUG - Building Additional Commerce - end
}

void CvGameTextMgr::setYieldHelp(CvWStringBuffer &szBuffer, CvCity const& kCity,
		YieldTypes eYield) {  // advc.003: style changes

	if(eYield == NO_YIELD)
		return;
	CvYieldInfo& kYield = GC.getYieldInfo(eYield);
	int const iYieldChar = kYield.getChar();

	if(kCity.getOwner() == NO_PLAYER)
		return;
	CvPlayer& kOwner = GET_PLAYER(kCity.getOwner());

	int iBaseYieldRate = kCity.getBaseYieldRate(eYield);
	szBuffer.append(gDLL->getText("TXT_KEY_MISC_HELP_BASE_YIELD",
			kYield.getTextKeyWide(), iBaseYieldRate, iYieldChar));
	szBuffer.append(NEWLINE);

	int iBaseModifier = 100;

	// Buildings
	int iBuildingMod = 0;
	for(int i = 0; i < GC.getNumBuildingInfos(); i++) {
		BuildingTypes eBuilding = (BuildingTypes)i;
		CvBuildingInfo& kBuilding = GC.getBuildingInfo(eBuilding);
		if(kCity.getNumBuilding(eBuilding) > 0 &&
				!GET_TEAM(kCity.getTeam()).isObsoleteBuilding((BuildingTypes)i)) {
			for(int j = 0; j < kCity.getNumBuilding((BuildingTypes)i); j++)
				iBuildingMod += kBuilding.getYieldModifier(eYield);
		}
		for(int j = 0; j < MAX_PLAYERS; j++) {
			CvPlayer const& kMember = GET_PLAYER((PlayerTypes)j);
			if(!kMember.isAlive() || kMember.getTeam() != kOwner.getTeam())
				continue;
			int iLoop;
			for(CvCity* pLoopCity = kMember.firstCity(&iLoop); pLoopCity != NULL;
					pLoopCity = kMember.nextCity(&iLoop)) {
				if(pLoopCity->getNumBuilding(eBuilding) > 0 &&
						!GET_TEAM(pLoopCity->getTeam()).isObsoleteBuilding(eBuilding)) {
					for(int k = 0; k < pLoopCity->getNumBuilding(eBuilding); k++)
						iBuildingMod += kBuilding.getGlobalYieldModifier(eYield);
				}
			}
		}
	}
	iBuildingMod += kCity.area()->getYieldRateModifier(kOwner.getID(), eYield);
	if(iBuildingMod != 0) {
		szBuffer.append(gDLL->getText("TXT_KEY_MISC_HELP_YIELD_BUILDINGS", iBuildingMod, iYieldChar));
		szBuffer.append(NEWLINE);
		iBaseModifier += iBuildingMod;
	}

	// Power
	if(kCity.isPower()) {
		int iPowerMod = kCity.getPowerYieldRateModifier(eYield);
		if(iPowerMod != 0) {
			szBuffer.append(gDLL->getText("TXT_KEY_MISC_HELP_YIELD_POWER", iPowerMod, iYieldChar));
			szBuffer.append(NEWLINE);
			iBaseModifier += iPowerMod;
		}
	}

	// Resources
	int iBonusMod = kCity.getBonusYieldRateModifier(eYield);
	if(iBonusMod != 0) {
		szBuffer.append(gDLL->getText("TXT_KEY_MISC_HELP_YIELD_BONUS", iBonusMod, iYieldChar));
		szBuffer.append(NEWLINE);
		iBaseModifier += iBonusMod;
	}

	// Capital
	if(kCity.isCapital()) {
		int iCapitalMod = kOwner.getCapitalYieldRateModifier(eYield);
		if(iCapitalMod != 0) {
			szBuffer.append(gDLL->getText("TXT_KEY_MISC_HELP_YIELD_CAPITAL", iCapitalMod, iYieldChar));
			szBuffer.append(NEWLINE);
			iBaseModifier += iCapitalMod;
		}
	}

	// Civics
	int iCivicMod = 0;
	for(int i = 0; i < GC.getNumCivicOptionInfos(); i++) {
		if(kOwner.getCivics((CivicOptionTypes)i) != NO_CIVIC)
			iCivicMod += GC.getCivicInfo(kOwner.getCivics((CivicOptionTypes)i)).getYieldModifier(eYield);
	}
	if(iCivicMod != 0) {
		szBuffer.append(gDLL->getText("TXT_KEY_MISC_HELP_YIELD_CIVICS", iCivicMod, iYieldChar));
		szBuffer.append(NEWLINE);
		iBaseModifier += iCivicMod;
	}

	FAssertMsg((iBaseModifier * iBaseYieldRate) / 100 == kCity.getYieldRate(eYield),
			"Yield Modifier in setProductionHelp does not agree with actual value");
}

void CvGameTextMgr::setConvertHelp(CvWStringBuffer& szBuffer, PlayerTypes ePlayer, ReligionTypes eReligion)
{
	CvWString szReligion = L"TXT_KEY_MISC_NO_STATE_RELIGION";

	if (eReligion != NO_RELIGION)
	{
		szReligion = GC.getReligionInfo(eReligion).getTextKeyWide();
	}

	szBuffer.assign(gDLL->getText("TXT_KEY_MISC_CANNOT_CONVERT_TO", szReligion.GetCString()));

	if (GET_PLAYER(ePlayer).isAnarchy())
	{
		szBuffer.append(gDLL->getText("TXT_KEY_MISC_WHILE_IN_ANARCHY"));
	}
	else if (GET_PLAYER(ePlayer).getStateReligion() == eReligion)
	{
		szBuffer.append(L". ");
		szBuffer.append(gDLL->getText("TXT_KEY_MISC_ALREADY_STATE_REL"));
		szBuffer.append(L"."); // advc.004g
	}
	else if (GET_PLAYER(ePlayer).getConversionTimer() > 0)
	{
		szBuffer.append(gDLL->getText("TXT_KEY_MISC_ANOTHER_REVOLUTION_RECENTLY"));
		szBuffer.append(L". ");
		szBuffer.append(gDLL->getText("TXT_KEY_MISC_WAIT_MORE_TURNS", GET_PLAYER(ePlayer).getConversionTimer()));
		szBuffer.append(L"."); // advc.004g
	}
}

void CvGameTextMgr::setRevolutionHelp(CvWStringBuffer& szBuffer, PlayerTypes ePlayer)
{
	szBuffer.assign(gDLL->getText("TXT_KEY_MISC_CANNOT_CHANGE_CIVICS"));

	if (GET_PLAYER(ePlayer).isAnarchy())
	{
		szBuffer.append(gDLL->getText("TXT_KEY_MISC_WHILE_IN_ANARCHY"));
	}
	else if (GET_PLAYER(ePlayer).getRevolutionTimer() > 0)
	{
		szBuffer.append(gDLL->getText("TXT_KEY_MISC_ANOTHER_REVOLUTION_RECENTLY"));
		szBuffer.append(L" : ");
		szBuffer.append(gDLL->getText("TXT_KEY_MISC_WAIT_MORE_TURNS", GET_PLAYER(ePlayer).getRevolutionTimer()));
	}
}

void CvGameTextMgr::setVassalRevoltHelp(CvWStringBuffer& szBuffer, TeamTypes eMaster, TeamTypes eVassal)
{
	if (NO_TEAM == eMaster || NO_TEAM == eVassal)
	{
		return;
	}

	if (!GET_TEAM(eVassal).isCapitulated())
	{
		return;
	}

	if (GET_TEAM(eMaster).isParent(eVassal))
	{
		return;
	}

	CvTeam& kMaster = GET_TEAM(eMaster);
	CvTeam& kVassal = GET_TEAM(eVassal);

	int iMasterLand = kMaster.getTotalLand(false);
	// advc.112: Lower bound added
	int iVassalLand = std::max(10, kVassal.getTotalLand(false));
	if (iMasterLand > 0 && GC.getDefineINT("FREE_VASSAL_LAND_PERCENT") >= 0)
	{
		szBuffer.append(gDLL->getText("TXT_KEY_MISC_VASSAL_LAND_STATS", (iVassalLand * 100) / iMasterLand, GC.getDefineINT("FREE_VASSAL_LAND_PERCENT")));
	}

	int iMasterPop = kMaster.getTotalPopulation(false);
	int iVassalPop = kVassal.getTotalPopulation(false);
	if (iMasterPop > 0 && GC.getDefineINT("FREE_VASSAL_POPULATION_PERCENT") >= 0)
	{
		szBuffer.append(gDLL->getText("TXT_KEY_MISC_VASSAL_POPULATION_STATS", (iVassalPop * 100) / iMasterPop, GC.getDefineINT("FREE_VASSAL_POPULATION_PERCENT")));
	}

	if (GC.getDefineINT("VASSAL_REVOLT_OWN_LOSSES_FACTOR") > 0 && kVassal.getVassalPower() > 0)
	{
		szBuffer.append(gDLL->getText("TXT_KEY_MISC_VASSAL_AREA_LOSS", (iVassalLand * 100) / kVassal.getVassalPower(), GC.getDefineINT("VASSAL_REVOLT_OWN_LOSSES_FACTOR")));
	}

	if (GC.getDefineINT("VASSAL_REVOLT_MASTER_LOSSES_FACTOR") > 0 && kVassal.getMasterPower() > 0)
	{
		szBuffer.append(gDLL->getText("TXT_KEY_MISC_MASTER_AREA_LOSS", (iMasterLand * 100) / kVassal.getMasterPower(), GC.getDefineINT("VASSAL_REVOLT_MASTER_LOSSES_FACTOR")));
	}
}


void CvGameTextMgr::parseGreatPeopleHelp(CvWStringBuffer &szBuffer,
		CvCity const& kCity)  // advc.003: Some style changes
{
	if(NO_PLAYER == kCity.getOwner())
		return;
	CvPlayer const& kOwner = GET_PLAYER(kCity.getOwner());
	szBuffer.assign(gDLL->getText("TXT_KEY_MISC_GREAT_PERSON", kCity.getGreatPeopleProgress(),
			kOwner.greatPeopleThreshold(false)));
	int iTurnsLeft = kCity.GPTurnsLeft(); // advc.001c:
	if (kCity.getGreatPeopleRate() > 0)
	{
		int iGPPLeft = kOwner.greatPeopleThreshold(false) - kCity.getGreatPeopleProgress();
		if (iGPPLeft > 0)
		{	// advc.001c: Moved into CvCity::GPTurnsLeft
			/*iTurnsLeft = iGPPLeft / city.getGreatPeopleRate();
			if (iTurnsLeft * city.getGreatPeopleRate() <  iGPPLeft)
				iTurnsLeft++;*/
			szBuffer.append(NEWLINE);
			szBuffer.append(gDLL->getText("INTERFACE_CITY_TURNS",
					std::max(1, iTurnsLeft)));
		}
	}

	int iTotalGreatPeopleUnitProgress = 0;
	for (int iI = 0; iI < GC.getNumUnitInfos(); iI++) {
		UnitTypes eGPType = (UnitTypes)iI; // advc.003
		iTotalGreatPeopleUnitProgress += kCity.getGreatPeopleUnitProgress(eGPType);
		// advc.001c:
		iTotalGreatPeopleUnitProgress += kCity.getGreatPeopleUnitRate(eGPType);
	}

	if (iTotalGreatPeopleUnitProgress > 0)
	{
		szBuffer.append(SEPARATOR);
		szBuffer.append(NEWLINE);
		szBuffer.append(gDLL->getText("TXT_KEY_MISC_PROB"));

		std::vector<std::pair<UnitTypes,int> > aUnitProgress;
		// advc.001c: BtS code moved into CvCity::GPProjection
		kCity.GPProjection(aUnitProgress);
		for(int iI = 0; iI < (int)aUnitProgress.size(); iI++) {
			szBuffer.append(CvWString::format(L"%s%s - %d%%", NEWLINE,
					GC.getUnitInfo(aUnitProgress[iI].first).getDescription(),
					aUnitProgress[iI].second));
		}
	}

	//if (city.getGreatPeopleRate() == 0)
// BUG - Building Additional Great People - start
	bool bBuildingAdditionalGreatPeople = (getBugOptionBOOL("MiscHover__BuildingAdditionalGreatPeople", false)
			|| GC.altKey()); // advc.063
	if (kCity.getGreatPeopleRate() == 0 && !bBuildingAdditionalGreatPeople)
// BUG - Building Additional Great People - end
		return;

	szBuffer.append(SEPARATOR);
	szBuffer.append(NEWLINE);
	szBuffer.append(gDLL->getText("TXT_KEY_MISC_HELP_GREATPEOPLE_BASE_RATE", kCity.getBaseGreatPeopleRate()));
	szBuffer.append(NEWLINE);

	int iModifier = 100;

	// Buildings
	int iBuildingMod = 0;
	for (int i = 0; i < GC.getNumBuildingInfos(); i++)
	{
		CvBuildingInfo& infoBuilding = GC.getBuildingInfo((BuildingTypes)i);
		if (kCity.getNumBuilding((BuildingTypes)i) > 0 && !GET_TEAM(kCity.getTeam()).isObsoleteBuilding((BuildingTypes)i))
		{
			for (int iLoop = 0; iLoop < kCity.getNumBuilding((BuildingTypes)i); iLoop++)
			{
				iBuildingMod += infoBuilding.getGreatPeopleRateModifier();
			}
		}
		for (int j = 0; j < MAX_PLAYERS; j++)
		{
			if (GET_PLAYER((PlayerTypes)j).isAlive())
			{
				if (GET_PLAYER((PlayerTypes)j).getTeam() == kOwner.getTeam())
				{
					int iLoop;
					for (CvCity* pLoopCity = GET_PLAYER((PlayerTypes)j).firstCity(&iLoop); pLoopCity != NULL; pLoopCity = GET_PLAYER((PlayerTypes)j).nextCity(&iLoop))
					{
						if (pLoopCity->getNumBuilding((BuildingTypes)i) > 0 && !GET_TEAM(pLoopCity->getTeam()).isObsoleteBuilding((BuildingTypes)i))
						{
							for (int k = 0; k < pLoopCity->getNumBuilding((BuildingTypes)i); k++)
							{
								iBuildingMod += infoBuilding.getGlobalGreatPeopleRateModifier();
							}
						}
					}
				}
			}
		}
	}
	if (iBuildingMod != 0)
	{
		szBuffer.append(gDLL->getText("TXT_KEY_MISC_HELP_GREATPEOPLE_BUILDINGS", iBuildingMod));
		szBuffer.append(NEWLINE);
		iModifier += iBuildingMod;
	}

	// Civics
	int iCivicMod = 0;
	for (int i = 0; i < GC.getNumCivicOptionInfos(); i++)
	{
		if (NO_CIVIC != kOwner.getCivics((CivicOptionTypes)i))
		{
			iCivicMod += GC.getCivicInfo(kOwner.getCivics((CivicOptionTypes)i)).getGreatPeopleRateModifier();
			if (kOwner.getStateReligion() != NO_RELIGION && kCity.isHasReligion(kOwner.getStateReligion()))
			{
				iCivicMod += GC.getCivicInfo(kOwner.getCivics((CivicOptionTypes)i)).getStateReligionGreatPeopleRateModifier();
			}
		}
	}
	if (iCivicMod != 0)
	{
		szBuffer.append(gDLL->getText("TXT_KEY_MISC_HELP_GREATPEOPLE_CIVICS", iCivicMod));
		szBuffer.append(NEWLINE);
		iModifier += iCivicMod;
	}

	// Trait
	for (int i = 0; i < GC.getNumTraitInfos(); i++)
	{
		if (kCity.hasTrait((TraitTypes)i))
		{
			CvTraitInfo& trait = GC.getTraitInfo((TraitTypes)i);
			int iTraitMod = trait.getGreatPeopleRateModifier();
			if (iTraitMod != 0)
			{
				szBuffer.append(gDLL->getText("TXT_KEY_MISC_HELP_GREATPEOPLE_TRAIT", iTraitMod, trait.getTextKeyWide()));
				szBuffer.append(NEWLINE);
				iModifier += iTraitMod;
			}
		}
	}

	if (kOwner.isGoldenAge())
	{
		int iGoldenAgeMod = GC.getDefineINT("GOLDEN_AGE_GREAT_PEOPLE_MODIFIER");

		if (iGoldenAgeMod != 0)
		{
			szBuffer.append(gDLL->getText("TXT_KEY_MISC_HELP_GREATPEOPLE_GOLDEN_AGE", iGoldenAgeMod));
			szBuffer.append(NEWLINE);
			iModifier += iGoldenAgeMod;
		}
	}

	int iModGreatPeople = (iModifier * kCity.getBaseGreatPeopleRate()) / 100;

	FAssertMsg(iModGreatPeople == kCity.getGreatPeopleRate(), "Great person rate does not match actual value");

	szBuffer.append(gDLL->getText("TXT_KEY_MISC_HELP_GREATPEOPLE_FINAL", iModGreatPeople));
	//szBuffer.append(NEWLINE);
// BUG - Building Additional Great People - start
	if (bBuildingAdditionalGreatPeople && kCity.getOwner() == GC.getGame().getActivePlayer())
	{
		setBuildingAdditionalGreatPeopleHelp(szBuffer, kCity, DOUBLE_SEPARATOR);
	}
// BUG - Building Additional Great People - end
}

// BUG - Building Additional Great People - start
bool CvGameTextMgr::setBuildingAdditionalGreatPeopleHelp(CvWStringBuffer &szBuffer, const CvCity& city, const CvWString& szStart, bool bStarted)
{
	CvWString szLabel;
	CvCivilizationInfo& kCivilization = GC.getCivilizationInfo(GET_PLAYER(city.getOwner()).getCivilizationType());

	for (int iI = 0; iI < GC.getNumBuildingClassInfos(); iI++)
	{
		BuildingTypes eBuilding = (BuildingTypes)kCivilization.getCivilizationBuildings((BuildingClassTypes)iI);

		if (eBuilding != NO_BUILDING && city.canConstruct(eBuilding, false, true, false))
		{
			int iChange = city.getAdditionalGreatPeopleRateByBuilding(eBuilding);

			if (iChange != 0)
			{
				if (!bStarted)
				{
					szBuffer.append(szStart);
					bStarted = true;
				}

				szLabel.Format(SETCOLR L"%s" ENDCOLR, TEXT_COLOR("COLOR_BUILDING_TEXT"), GC.getBuildingInfo(eBuilding).getDescription());
				setResumableValueChangeHelp(szBuffer, szLabel, L": ", L"", iChange, gDLL->getSymbolID(GREAT_PEOPLE_CHAR), false, true);
			}
		}
	}

	return bStarted;
}
// BUG - Building Additional Great People - end

void CvGameTextMgr::parseGreatGeneralHelp(CvWStringBuffer &szBuffer, CvPlayer& kPlayer)
{
	szBuffer.assign(gDLL->getText("TXT_KEY_MISC_GREAT_GENERAL", kPlayer.getCombatExperience(), kPlayer.greatPeopleThreshold(true)));
}


//------------------------------------------------------------------------------------------------

void CvGameTextMgr::buildCityBillboardIconString( CvWStringBuffer& szBuffer, CvCity* pCity)
{
	szBuffer.clear();

	// government center icon
	if (pCity->isGovernmentCenter() && !(pCity->isCapital()))
	{
		szBuffer.append(CvWString::format(L"%c", gDLL->getSymbolID(SILVER_STAR_CHAR)));
	}

	// happiness, healthiness, superlative icons
	if (pCity->canBeSelected())
	{
		if (pCity->angryPopulation() > 0)
		{
			szBuffer.append(CvWString::format(L"%c", gDLL->getSymbolID(UNHAPPY_CHAR)));
		}

		if (pCity->healthRate() < 0)
		{
			szBuffer.append(CvWString::format(L"%c", gDLL->getSymbolID(UNHEALTHY_CHAR)));
		}
		// advc.076: Disabled
		/*if (gDLL->getGraphicOption(GRAPHICOPTION_CITY_DETAIL)) {
			if (GET_PLAYER(pCity->getOwner()).getNumCities() > 2) {
				if (pCity->findYieldRateRank(YIELD_PRODUCTION) == 1)
					szBuffer.append(CvWString::format(L"%c", GC.getYieldInfo(YIELD_PRODUCTION).getChar()));
				if (pCity->findCommerceRateRank(COMMERCE_GOLD) == 1)
					szBuffer.append(CvWString::format(L"%c", GC.getCommerceInfo(COMMERCE_GOLD).getChar()));
				if (pCity->findCommerceRateRank(COMMERCE_RESEARCH) == 1)
					szBuffer.append(CvWString::format(L"%c", GC.getCommerceInfo(COMMERCE_RESEARCH).getChar()));
			}
		}*/

		if (pCity->isConnectedToCapital()
				// advc.002f:
				&& getBugOptionBOOL("MainInterface__CityNetworkIcon", false))
		{
			if (GET_PLAYER(pCity->getOwner()).countNumCitiesConnectedToCapital() > 1)
				szBuffer.append(CvWString::format(L"%c", gDLL->getSymbolID(TRADE_CHAR)));
		}
		// <advc.002f>
		// BUG - Airport Icon - start
		if (getBugOptionBOOL("MainInterface__AirportIcon", true))
		{
			int eAirportClass = GC.getInfoTypeForString("BUILDINGCLASS_AIRPORT"
					// Mod-mods that don't have an airport should set bHideAssert:
					/*,true*/);
			if (eAirportClass != -1)
			{
				int eAirport = GC.getCivilizationInfo(pCity->getCivilizationType()).getCivilizationBuildings(eAirportClass);
				if (eAirport != -1 && pCity->getNumBuilding((BuildingTypes)eAirport) > 0)
				{
					szBuffer.append(CvWString::format(L"%c", gDLL->getSymbolID(AIRPORT_CHAR)));
				}
			}
		} // BUG - Airport Icon - end
		// </advc.002f>
	}

	// religion icons
	for (int iI = 0; iI < GC.getNumReligionInfos(); ++iI)
	{
		if (pCity->isHasReligion((ReligionTypes)iI))
		{
			if (pCity->isHolyCity((ReligionTypes)iI))
			{
				szBuffer.append(CvWString::format(L"%c", GC.getReligionInfo((ReligionTypes) iI).getHolyCityChar()));
			}
			else
			{
				szBuffer.append(CvWString::format(L"%c", GC.getReligionInfo((ReligionTypes) iI).getChar()));
			}
		}
	}

	// corporation icons
	for (int iI = 0; iI < GC.getNumCorporationInfos(); ++iI)
	{
		if (pCity->isHeadquarters((CorporationTypes)iI))
		{
			if (pCity->isHasCorporation((CorporationTypes)iI))
			{
				szBuffer.append(CvWString::format(L"%c", GC.getCorporationInfo((CorporationTypes) iI).getHeadquarterChar()));
			}
		}
		else
		{
			if (pCity->isActiveCorporation((CorporationTypes)iI))
			{
				szBuffer.append(CvWString::format(L"%c", GC.getCorporationInfo((CorporationTypes) iI).getChar()));
			}
		}
	}

	if (pCity->getTeam() == GC.getGame().getActiveTeam())
	{
		if (pCity->isPower())
		{
			szBuffer.append(CvWString::format(L"%c", gDLL->getSymbolID(POWER_CHAR)));
		}
	}

	// XXX out this in bottom bar???
	if (pCity->isOccupation())
	{
		szBuffer.append(CvWString::format(L" (%c:%d)", gDLL->getSymbolID(OCCUPATION_CHAR), pCity->getOccupationTimer()));
	}

	// defense icon and text
	//if (pCity->getTeam() != GC.getGame().getActiveTeam())
	{
		if (pCity->isVisible(GC.getGame().getActiveTeam(), true))
		{
			int iDefenseModifier = pCity->getDefenseModifier(GC.getGame().selectionListIgnoreBuildingDefense());

			if (iDefenseModifier != 0)
			{
				//szBuffer.append(CvWString::format(L" %c:%s%d%%", gDLL->getSymbolID(DEFENSE_CHAR), ((iDefenseModifier > 0) ? "+" : ""), iDefenseModifier));
				// <advc.002f> Replacing the above
				szBuffer.append(CvWString::format(L"   " SETCOLR L"%s%d%%" ENDCOLR L"%c",
						// I've tried some other colors, but they're no easier to read.
						TEXT_COLOR("COLOR_WHITE"),
						((iDefenseModifier > 0) ? "+" : ""),
						iDefenseModifier,
						gDLL->getSymbolID(DEFENSE_CHAR))); // </advc.002f>
			}
		}
	}
}

void CvGameTextMgr::buildCityBillboardCityNameString( CvWStringBuffer& szBuffer, CvCity* pCity)
{
	szBuffer.assign(pCity->getName());

	if (pCity->canBeSelected())
	{
		if (gDLL->getGraphicOption(GRAPHICOPTION_CITY_DETAIL))
		{
			if (pCity->foodDifference() > 0)
			{
				int iTurns = pCity->getFoodTurnsLeft();

				if ((iTurns > 1) || !(pCity->AI_isEmphasizeAvoidGrowth()))
				{
					if (iTurns < MAX_INT)
					{
						szBuffer.append(CvWString::format(L" (%d)", iTurns));
					}
				}
			}
		}
	}
}

void CvGameTextMgr::buildCityBillboardProductionString(CvWStringBuffer& szBuffer, CvCity* pCity)  // advc.003: style changes
{
	if (pCity->getOrderQueueLength() <= 0) {
		szBuffer.clear();
		return;
	}

	szBuffer.assign(pCity->getProductionName());
	if (gDLL->getGraphicOption(GRAPHICOPTION_CITY_DETAIL)) {
		int iTurns = pCity->getProductionTurnsLeft();
		if (iTurns < MAX_INT)
			szBuffer.append(CvWString::format(L" (%d)", iTurns));
	}
}


void CvGameTextMgr::buildCityBillboardCitySizeString( CvWStringBuffer& szBuffer, CvCity* pCity, const NiColorA& kColor)
{
#define CAPARAMS(c) (int)((c).r * 255.0f), (int)((c).g * 255.0f), (int)((c).b * 255.0f), (int)((c).a * 255.0f)
	szBuffer.assign(CvWString::format(SETCOLR L"%d" ENDCOLR, CAPARAMS(kColor), pCity->getPopulation()));
#undef CAPARAMS
}

void CvGameTextMgr::getCityBillboardFoodbarColors(CvCity* pCity, std::vector<NiColorA>& aColors)
{
	aColors.resize(NUM_INFOBAR_TYPES);
	aColors[INFOBAR_STORED] = GC.getColorInfo((ColorTypes)(GC.getYieldInfo(YIELD_FOOD).getColorType())).getColor();
	aColors[INFOBAR_RATE] = aColors[INFOBAR_STORED];
	aColors[INFOBAR_RATE].a = 0.5f;
	aColors[INFOBAR_RATE_EXTRA] = GC.getColorInfo((ColorTypes)GC.getInfoTypeForString("COLOR_NEGATIVE_RATE")).getColor();
	aColors[INFOBAR_EMPTY] = GC.getColorInfo((ColorTypes)GC.getInfoTypeForString("COLOR_EMPTY")).getColor();
}

void CvGameTextMgr::getCityBillboardProductionbarColors(CvCity* pCity, std::vector<NiColorA>& aColors)
{
	aColors.resize(NUM_INFOBAR_TYPES);
	aColors[INFOBAR_STORED] = GC.getColorInfo((ColorTypes)(GC.getYieldInfo(YIELD_PRODUCTION).getColorType())).getColor();
	aColors[INFOBAR_RATE] = aColors[INFOBAR_STORED];
	aColors[INFOBAR_RATE].a = 0.5f;
	aColors[INFOBAR_RATE_EXTRA] = GC.getColorInfo((ColorTypes)(GC.getYieldInfo(YIELD_FOOD).getColorType())).getColor();
	aColors[INFOBAR_RATE_EXTRA].a = 0.5f;
	aColors[INFOBAR_EMPTY] = GC.getColorInfo((ColorTypes)GC.getInfoTypeForString("COLOR_EMPTY")).getColor();
}


void CvGameTextMgr::setScoreHelp(CvWStringBuffer &szString, PlayerTypes ePlayer)
{
	if (NO_PLAYER != ePlayer)
	{
		CvPlayer& player = GET_PLAYER(ePlayer);

		int iPop = player.getPopScore();
		int iMaxPop = GC.getGame().getMaxPopulation();
		int iPopScore = 0;
		if (iMaxPop > 0)
		{
			iPopScore = (GC.getDefineINT("SCORE_POPULATION_FACTOR") * iPop) / iMaxPop;
		}
		int iLand = player.getLandScore();
		int iMaxLand = GC.getGame().getMaxLand();
		int iLandScore = 0;
		if (iMaxLand > 0)
		{
			iLandScore = (GC.getDefineINT("SCORE_LAND_FACTOR") * iLand) / iMaxLand;
		}
		int iTech = player.getTechScore();
		int iMaxTech = GC.getGame().getMaxTech();
		int iTechScore = 0;
		if (iMaxTech > 0) // BETTER_BTS_AI_MOD, Bugfix, 02/24/10, jdog5000
			iTechScore = (GC.getDefineINT("SCORE_TECH_FACTOR") * iTech) / iMaxTech;
		int iWonders = player.getWondersScore();
		int iMaxWonders = GC.getGame().getMaxWonders();
		int iWondersScore = 0;
		if (iMaxWonders > 0) // BETTER_BTS_AI_MOD, Bugfix, 02/24/10, jdog5000
			iWondersScore = (GC.getDefineINT("SCORE_WONDER_FACTOR") * iWonders) / iMaxWonders;
		int iTotalScore = iPopScore + iLandScore + iTechScore + iWondersScore;
		int iVictoryScore = player.calculateScore(true, true);
		// <advc.250c> Show leader name while in Advanced Start
		if(GC.getGame().isInAdvancedStart()) {
			szString.append(GC.getLeaderHeadInfo(GET_PLAYER(ePlayer).
					getLeaderType()).getText());
			szString.append(L"\n");
		} // </advc.250c>
		if (iTotalScore == player.calculateScore())
		{	// <advc.703> Advertise the Score Tab
			if(GC.getGame().isOption(GAMEOPTION_RISE_FALL))
				szString.append(gDLL->getText("TXT_KEY_RF_CIV_SCORE_BREAKDOWN", iPopScore, iPop, iMaxPop, iLandScore, iLand, iMaxLand, iTechScore, iTech, iMaxTech, iWondersScore, iWonders, iMaxWonders, iTotalScore));
			else // </advc.703>
			szString.append(gDLL->getText("TXT_KEY_SCORE_BREAKDOWN", iPopScore, iPop, iMaxPop, iLandScore, iLand, iMaxLand, iTechScore, iTech, iMaxTech, iWondersScore, iWonders, iMaxWonders, iTotalScore, iVictoryScore));
		}
	}
}

void CvGameTextMgr::setEventHelp(CvWStringBuffer& szBuffer, EventTypes eEvent, int iEventTriggeredId, PlayerTypes ePlayer)
{
	if (NO_EVENT == eEvent || NO_PLAYER == ePlayer)
	{
		return;
	}

	CvEventInfo& kEvent = GC.getEventInfo(eEvent);
	CvPlayer& kActivePlayer = GET_PLAYER(ePlayer);
	EventTriggeredData* pTriggeredData = kActivePlayer.getEventTriggered(iEventTriggeredId);

	if (NULL == pTriggeredData)
	{
		return;
	}

	CvCity* pCity = kActivePlayer.getCity(pTriggeredData->m_iCityId);
	CvCity* pOtherPlayerCity = NULL;
	CvPlot* pPlot = GC.getMap().plot(pTriggeredData->m_iPlotX, pTriggeredData->m_iPlotY);
	CvUnit* pUnit = kActivePlayer.getUnit(pTriggeredData->m_iUnitId);

	if (NO_PLAYER != pTriggeredData->m_eOtherPlayer)
	{
		pOtherPlayerCity = GET_PLAYER(pTriggeredData->m_eOtherPlayer).getCity(pTriggeredData->m_iOtherPlayerCityId);
	}

	CvWString szCity = gDLL->getText("TXT_KEY_EVENT_THE_CITY");
	if (NULL != pCity && kEvent.isCityEffect())
	{
		szCity = pCity->getNameKey();
	}
	else if (NULL != pOtherPlayerCity && kEvent.isOtherPlayerCityEffect())
	{
		szCity = pOtherPlayerCity->getNameKey();
	}

	CvWString szUnit = gDLL->getText("TXT_KEY_EVENT_THE_UNIT");
	if (NULL != pUnit)
	{
		szUnit = pUnit->getNameKey();
	}

	CvWString szReligion = gDLL->getText("TXT_KEY_EVENT_THE_RELIGION");
	if (NO_RELIGION != pTriggeredData->m_eReligion)
	{
		szReligion = GC.getReligionInfo(pTriggeredData->m_eReligion).getTextKeyWide();
	}

	eventGoldHelp(szBuffer, eEvent, ePlayer, pTriggeredData->m_eOtherPlayer);

	eventTechHelp(szBuffer, eEvent, kActivePlayer.getBestEventTech(eEvent, pTriggeredData->m_eOtherPlayer), ePlayer, pTriggeredData->m_eOtherPlayer);

	if (NO_PLAYER != pTriggeredData->m_eOtherPlayer && NO_BONUS != kEvent.getBonusGift())
	{
		szBuffer.append(NEWLINE);
		szBuffer.append(gDLL->getText("TXT_KEY_EVENT_GIFT_BONUS_TO_PLAYER", GC.getBonusInfo((BonusTypes)kEvent.getBonusGift()).getTextKeyWide(), GET_PLAYER(pTriggeredData->m_eOtherPlayer).getNameKey()));
	}

	if (kEvent.getHappy() != 0)
	{
		if (NO_PLAYER != pTriggeredData->m_eOtherPlayer)
		{
			if (kEvent.getHappy() > 0)
			{
				szBuffer.append(NEWLINE);
				szBuffer.append(gDLL->getText("TXT_KEY_EVENT_HAPPY_FROM_PLAYER", kEvent.getHappy(), kEvent.getHappy(), GET_PLAYER(pTriggeredData->m_eOtherPlayer).getNameKey()));
			}
			else
			{
				szBuffer.append(NEWLINE);
				szBuffer.append(gDLL->getText("TXT_KEY_EVENT_HAPPY_TO_PLAYER", -kEvent.getHappy(), -kEvent.getHappy(), GET_PLAYER(pTriggeredData->m_eOtherPlayer).getNameKey()));
			}
		}
		else
		{
			if (kEvent.getHappy() > 0)
			{
				if (kEvent.isCityEffect() || kEvent.isOtherPlayerCityEffect())
				{
					szBuffer.append(NEWLINE);
					szBuffer.append(gDLL->getText("TXT_KEY_EVENT_HAPPY_CITY", kEvent.getHappy(), szCity.GetCString()));
				}
				else
				{
					szBuffer.append(NEWLINE);
					szBuffer.append(gDLL->getText("TXT_KEY_EVENT_HAPPY", kEvent.getHappy()));
				}
			}
			else
			{
				if (kEvent.isCityEffect() || kEvent.isOtherPlayerCityEffect())
				{
					szBuffer.append(NEWLINE);
					szBuffer.append(gDLL->getText("TXT_KEY_EVENT_UNHAPPY_CITY", -kEvent.getHappy(), szCity.GetCString()));
				}
				else
				{
					szBuffer.append(NEWLINE);
					szBuffer.append(gDLL->getText("TXT_KEY_EVENT_UNHAPPY", -kEvent.getHappy()));
				}
			}
		}
	}

	if (kEvent.getHealth() != 0)
	{
		if (NO_PLAYER != pTriggeredData->m_eOtherPlayer)
		{
			if (kEvent.getHealth() > 0)
			{
				szBuffer.append(NEWLINE);
				szBuffer.append(gDLL->getText("TXT_KEY_EVENT_HEALTH_FROM_PLAYER", kEvent.getHealth(), kEvent.getHealth(), GET_PLAYER(pTriggeredData->m_eOtherPlayer).getNameKey()));
			}
			else
			{
				szBuffer.append(NEWLINE);
				szBuffer.append(gDLL->getText("TXT_KEY_EVENT_HEALTH_TO_PLAYER", -kEvent.getHealth(), -kEvent.getHealth(), GET_PLAYER(pTriggeredData->m_eOtherPlayer).getNameKey()));
			}
		}
		else
		{
			if (kEvent.getHealth() > 0)
			{
				if (kEvent.isCityEffect() || kEvent.isOtherPlayerCityEffect())
				{
					szBuffer.append(NEWLINE);
					szBuffer.append(gDLL->getText("TXT_KEY_EVENT_HEALTH_CITY", kEvent.getHealth(), szCity.GetCString()));
				}
				else
				{
					szBuffer.append(NEWLINE);
					szBuffer.append(gDLL->getText("TXT_KEY_EVENT_HEALTH", kEvent.getHealth()));
				}
			}
			else
			{
				if (kEvent.isCityEffect() || kEvent.isOtherPlayerCityEffect())
				{
					szBuffer.append(NEWLINE);
					szBuffer.append(gDLL->getText("TXT_KEY_EVENT_UNHEALTH", -kEvent.getHealth(), szCity.GetCString()));
				}
				else
				{
					szBuffer.append(NEWLINE);
					szBuffer.append(gDLL->getText("TXT_KEY_EVENT_UNHEALTH_CITY", -kEvent.getHealth()));
				}
			}
		}
	}

	if (kEvent.getHurryAnger() != 0)
	{
		if (kEvent.isCityEffect() || kEvent.isOtherPlayerCityEffect())
		{
			szBuffer.append(NEWLINE);
			szBuffer.append(gDLL->getText("TXT_KEY_EVENT_HURRY_ANGER_CITY", kEvent.getHurryAnger(), szCity.GetCString()));
		}
		else
		{
			szBuffer.append(NEWLINE);
			szBuffer.append(gDLL->getText("TXT_KEY_EVENT_HURRY_ANGER", kEvent.getHurryAnger()));
		}
	}

	if (kEvent.getHappyTurns() > 0)
	{
		if (kEvent.isCityEffect() || kEvent.isOtherPlayerCityEffect())
		{
			szBuffer.append(NEWLINE);
			szBuffer.append(gDLL->getText("TXT_KEY_EVENT_TEMP_HAPPY_CITY", GC.getDefineINT("TEMP_HAPPY"), kEvent.getHappyTurns(), szCity.GetCString()));
		}
		else
		{
			szBuffer.append(NEWLINE);
			szBuffer.append(gDLL->getText("TXT_KEY_EVENT_TEMP_HAPPY", GC.getDefineINT("TEMP_HAPPY"), kEvent.getHappyTurns()));
		}
	}

	if (kEvent.getFood() != 0)
	{
		if (kEvent.isCityEffect() || kEvent.isOtherPlayerCityEffect())
		{
			szBuffer.append(NEWLINE);
			szBuffer.append(gDLL->getText("TXT_KEY_EVENT_FOOD_CITY", kEvent.getFood(), szCity.GetCString()));
		}
		else
		{
			szBuffer.append(NEWLINE);
			szBuffer.append(gDLL->getText("TXT_KEY_EVENT_FOOD", kEvent.getFood()));
		}
	}

	if (kEvent.getFoodPercent() != 0)
	{
		if (kEvent.isCityEffect() || kEvent.isOtherPlayerCityEffect())
		{
			szBuffer.append(NEWLINE);
			szBuffer.append(gDLL->getText("TXT_KEY_EVENT_FOOD_PERCENT_CITY", kEvent.getFoodPercent(), szCity.GetCString()));
		}
		else
		{
			szBuffer.append(NEWLINE);
			szBuffer.append(gDLL->getText("TXT_KEY_EVENT_FOOD_PERCENT", kEvent.getFoodPercent()));
		}
	}

	if (kEvent.getRevoltTurns() > 0)
	{
		if (kEvent.isCityEffect() || kEvent.isOtherPlayerCityEffect())
		{
			szBuffer.append(NEWLINE);
			szBuffer.append(gDLL->getText("TXT_KEY_EVENT_REVOLT_TURNS", kEvent.getRevoltTurns(), szCity.GetCString()));
		}
	}

	if (kEvent.getSpaceProductionModifier() != 0)
	{
		if (kEvent.isCityEffect() || kEvent.isOtherPlayerCityEffect())
		{
			szBuffer.append(NEWLINE);
			szBuffer.append(gDLL->getText("TXT_KEY_EVENT_SPACE_PRODUCTION_CITY", kEvent.getSpaceProductionModifier(), szCity.GetCString()));
		}
		else
		{
			szBuffer.append(NEWLINE);
			szBuffer.append(gDLL->getText("TXT_KEY_BUILDING_SPACESHIP_MOD_ALL_CITIES", kEvent.getSpaceProductionModifier()));
		}
	}

	if (kEvent.getMaxPillage() > 0)
	{
		if (kEvent.isCityEffect() || kEvent.isOtherPlayerCityEffect())
		{
			if (kEvent.getMaxPillage() == kEvent.getMinPillage())
			{
				szBuffer.append(NEWLINE);
				szBuffer.append(gDLL->getText("TXT_KEY_EVENT_PILLAGE_CITY", kEvent.getMinPillage(), szCity.GetCString()));
			}
			else
			{
				szBuffer.append(NEWLINE);
				szBuffer.append(gDLL->getText("TXT_KEY_EVENT_PILLAGE_RANGE_CITY", kEvent.getMinPillage(), kEvent.getMaxPillage(), szCity.GetCString()));
			}
		}
		else
		{
			if (kEvent.getMaxPillage() == kEvent.getMinPillage())
			{
				szBuffer.append(NEWLINE);
				szBuffer.append(gDLL->getText("TXT_KEY_EVENT_PILLAGE", kEvent.getMinPillage()));
			}
			else
			{
				szBuffer.append(NEWLINE);
				szBuffer.append(gDLL->getText("TXT_KEY_EVENT_PILLAGE_RANGE", kEvent.getMinPillage(), kEvent.getMaxPillage()));
			}
		}
	}

	for (int i = 0; i < GC.getNumSpecialistInfos(); ++i)
	{
		if (kEvent.getFreeSpecialistCount(i) > 0)
		{
			if (kEvent.isCityEffect() || kEvent.isOtherPlayerCityEffect())
			{
				szBuffer.append(NEWLINE);
				szBuffer.append(gDLL->getText("TXT_KEY_EVENT_FREE_SPECIALIST", kEvent.getFreeSpecialistCount(i), GC.getSpecialistInfo((SpecialistTypes)i).getTextKeyWide(), szCity.GetCString()));
			}
		}
	}

	if (kEvent.getPopulationChange() != 0)
	{
		if (kEvent.isCityEffect() || kEvent.isOtherPlayerCityEffect())
		{
			szBuffer.append(NEWLINE);
			szBuffer.append(gDLL->getText("TXT_KEY_EVENT_POPULATION_CHANGE_CITY", kEvent.getPopulationChange(), szCity.GetCString()));
		}
		else
		{
			szBuffer.append(NEWLINE);
			szBuffer.append(gDLL->getText("TXT_KEY_EVENT_POPULATION_CHANGE", kEvent.getPopulationChange()));
		}
	}

	if (kEvent.getCulture() != 0)
	{
		if (kEvent.isCityEffect() || kEvent.isOtherPlayerCityEffect())
		{
			szBuffer.append(NEWLINE);
			szBuffer.append(gDLL->getText("TXT_KEY_EVENT_CULTURE_CITY", kEvent.getCulture(), szCity.GetCString()));
		}
		else
		{
			szBuffer.append(NEWLINE);
			szBuffer.append(gDLL->getText("TXT_KEY_EVENT_CULTURE", kEvent.getCulture()));
		}
	}

	if (kEvent.getUnitClass() != NO_UNITCLASS)
	{
		CivilizationTypes eCiv = kActivePlayer.getCivilizationType();
		if (NO_CIVILIZATION != eCiv)
		{
			UnitTypes eUnit = (UnitTypes)GC.getCivilizationInfo(eCiv).getCivilizationUnits(kEvent.getUnitClass());
			if (eUnit != NO_UNIT)
			{
				szBuffer.append(NEWLINE);
				szBuffer.append(gDLL->getText("TXT_KEY_EVENT_BONUS_UNIT", kEvent.getNumUnits(), GC.getUnitInfo(eUnit).getTextKeyWide()));
			}
		}
	}

	if (kEvent.getBuildingClass() != NO_BUILDINGCLASS)
	{
		CivilizationTypes eCiv = kActivePlayer.getCivilizationType();
		if (NO_CIVILIZATION != eCiv)
		{
			BuildingTypes eBuilding = (BuildingTypes)GC.getCivilizationInfo(eCiv).getCivilizationBuildings(kEvent.getBuildingClass());
			if (eBuilding != NO_BUILDING)
			{
				if (kEvent.getBuildingChange() > 0)
				{
					szBuffer.append(NEWLINE);
					szBuffer.append(gDLL->getText("TXT_KEY_EVENT_BONUS_BUILDING", GC.getBuildingInfo(eBuilding).getTextKeyWide()));
				}
				else if (kEvent.getBuildingChange() < 0)
				{
					szBuffer.append(NEWLINE);
					szBuffer.append(gDLL->getText("TXT_KEY_EVENT_REMOVE_BUILDING", GC.getBuildingInfo(eBuilding).getTextKeyWide()));
				}
			}
		}
	}

	if (kEvent.getNumBuildingYieldChanges() > 0)
	{
		CvWStringBuffer szYield;
		for (int iBuildingClass = 0; iBuildingClass < GC.getNumBuildingClassInfos(); ++iBuildingClass)
		{
			CivilizationTypes eCiv = kActivePlayer.getCivilizationType();
			if (NO_CIVILIZATION != eCiv)
			{
				BuildingTypes eBuilding = (BuildingTypes)GC.getCivilizationInfo(eCiv).getCivilizationBuildings(iBuildingClass);
				if (eBuilding != NO_BUILDING)
				{
					int aiYields[NUM_YIELD_TYPES];
					for (int iYield = 0; iYield < NUM_YIELD_TYPES; ++iYield)
					{
						aiYields[iYield] = kEvent.getBuildingYieldChange(iBuildingClass, iYield);
					}

					szYield.clear();
					setYieldChangeHelp(szYield, L"", L"", L"", aiYields, false, false);
					if (!szYield.isEmpty())
					{
						szBuffer.append(NEWLINE);
						szBuffer.append(gDLL->getText("TXT_KEY_EVENT_YIELD_CHANGE_BUILDING", GC.getBuildingInfo(eBuilding).getTextKeyWide(), szYield.getCString()));
					}
				}
			}
		}
	}

	if (kEvent.getNumBuildingCommerceChanges() > 0)
	{
		CvWStringBuffer szCommerce;
		for (int iBuildingClass = 0; iBuildingClass < GC.getNumBuildingClassInfos(); ++iBuildingClass)
		{
			CivilizationTypes eCiv = kActivePlayer.getCivilizationType();
			if (NO_CIVILIZATION != eCiv)
			{
				BuildingTypes eBuilding = (BuildingTypes)GC.getCivilizationInfo(eCiv).getCivilizationBuildings(iBuildingClass);
				if (eBuilding != NO_BUILDING)
				{
					int aiCommerces[NUM_COMMERCE_TYPES];
					for (int iCommerce = 0; iCommerce < NUM_COMMERCE_TYPES; ++iCommerce)
					{
						aiCommerces[iCommerce] = kEvent.getBuildingCommerceChange(iBuildingClass, iCommerce);
					}

					szCommerce.clear();
					setCommerceChangeHelp(szCommerce, L"", L"", L"", aiCommerces, false, false);
					if (!szCommerce.isEmpty())
					{
						szBuffer.append(NEWLINE);
						szBuffer.append(gDLL->getText("TXT_KEY_EVENT_YIELD_CHANGE_BUILDING", GC.getBuildingInfo(eBuilding).getTextKeyWide(), szCommerce.getCString()));
					}
				}
			}
		}
	}

	if (kEvent.getNumBuildingHappyChanges() > 0)
	{
		for (int iBuildingClass = 0; iBuildingClass < GC.getNumBuildingClassInfos(); ++iBuildingClass)
		{
			CivilizationTypes eCiv = kActivePlayer.getCivilizationType();
			if (NO_CIVILIZATION != eCiv)
			{
				BuildingTypes eBuilding = (BuildingTypes)GC.getCivilizationInfo(eCiv).getCivilizationBuildings(iBuildingClass);
				if (eBuilding != NO_BUILDING)
				{
					int iHappy = kEvent.getBuildingHappyChange(iBuildingClass);
					if (iHappy > 0)
					{
						szBuffer.append(NEWLINE);
						szBuffer.append(gDLL->getText("TXT_KEY_EVENT_HAPPY_BUILDING", GC.getBuildingInfo(eBuilding).getTextKeyWide(), iHappy, gDLL->getSymbolID(HAPPY_CHAR)));
					}
					else if (iHappy < 0)
					{
						szBuffer.append(NEWLINE);
						szBuffer.append(gDLL->getText("TXT_KEY_EVENT_HAPPY_BUILDING", GC.getBuildingInfo(eBuilding).getTextKeyWide(), -iHappy, gDLL->getSymbolID(UNHAPPY_CHAR)));
					}
				}
			}
		}
	}

	if (kEvent.getNumBuildingHealthChanges() > 0)
	{
		for (int iBuildingClass = 0; iBuildingClass < GC.getNumBuildingClassInfos(); ++iBuildingClass)
		{
			CivilizationTypes eCiv = kActivePlayer.getCivilizationType();
			if (NO_CIVILIZATION != eCiv)
			{
				BuildingTypes eBuilding = (BuildingTypes)GC.getCivilizationInfo(eCiv).getCivilizationBuildings(iBuildingClass);
				if (eBuilding != NO_BUILDING)
				{
					int iHealth = kEvent.getBuildingHealthChange(iBuildingClass);
					if (iHealth > 0)
					{
						szBuffer.append(NEWLINE);
						szBuffer.append(gDLL->getText("TXT_KEY_EVENT_HAPPY_BUILDING", GC.getBuildingInfo(eBuilding).getTextKeyWide(), iHealth, gDLL->getSymbolID(HEALTHY_CHAR)));
					}
					else if (iHealth < 0)
					{
						szBuffer.append(NEWLINE);
						szBuffer.append(gDLL->getText("TXT_KEY_EVENT_HAPPY_BUILDING", GC.getBuildingInfo(eBuilding).getTextKeyWide(), -iHealth, gDLL->getSymbolID(UNHEALTHY_CHAR)));
					}
				}
			}
		}
	}

	if (kEvent.getFeatureChange() > 0)
	{
		if (kEvent.getFeature() != NO_FEATURE)
		{
			szBuffer.append(NEWLINE);
			szBuffer.append(gDLL->getText("TXT_KEY_EVENT_FEATURE_GROWTH", GC.getFeatureInfo((FeatureTypes)kEvent.getFeature()).getTextKeyWide()));
		}
	}
	else if (kEvent.getFeatureChange() < 0)
	{
		if (NULL != pPlot && NO_FEATURE != pPlot->getFeatureType())
		{
			szBuffer.append(NEWLINE);
			szBuffer.append(gDLL->getText("TXT_KEY_EVENT_FEATURE_REMOVE", GC.getFeatureInfo(pPlot->getFeatureType()).getTextKeyWide()));
		}
	}

	if (kEvent.getImprovementChange() > 0)
	{
		if (kEvent.getImprovement() != NO_IMPROVEMENT)
		{
			szBuffer.append(NEWLINE);
			szBuffer.append(gDLL->getText("TXT_KEY_EVENT_IMPROVEMENT_GROWTH", GC.getImprovementInfo((ImprovementTypes)kEvent.getImprovement()).getTextKeyWide()));
		}
	}
	else if (kEvent.getImprovementChange() < 0)
	{
		if (NULL != pPlot && NO_IMPROVEMENT != pPlot->getImprovementType())
		{
			szBuffer.append(NEWLINE);
			szBuffer.append(gDLL->getText("TXT_KEY_EVENT_IMPROVEMENT_REMOVE", GC.getImprovementInfo(pPlot->getImprovementType()).getTextKeyWide()));
		}
	}

	if (kEvent.getBonusChange() > 0)
	{
		if (kEvent.getBonus() != NO_BONUS)
		{
			szBuffer.append(NEWLINE);
			szBuffer.append(gDLL->getText("TXT_KEY_EVENT_BONUS_GROWTH", GC.getBonusInfo((BonusTypes)kEvent.getBonus()).getTextKeyWide()));
		}
	}
	else if (kEvent.getBonusChange() < 0)
	{
		if (NULL != pPlot && NO_BONUS != pPlot->getBonusType())
		{
			szBuffer.append(NEWLINE);
			szBuffer.append(gDLL->getText("TXT_KEY_EVENT_BONUS_REMOVE", GC.getBonusInfo(pPlot->getBonusType()).getTextKeyWide()));
		}
	}

	if (kEvent.getRouteChange() > 0)
	{
		if (kEvent.getRoute() != NO_ROUTE)
		{
			szBuffer.append(NEWLINE);
			szBuffer.append(gDLL->getText("TXT_KEY_EVENT_ROUTE_GROWTH", GC.getRouteInfo((RouteTypes)kEvent.getRoute()).getTextKeyWide()));
		}
	}
	else if (kEvent.getRouteChange() < 0)
	{
		if (NULL != pPlot && NO_ROUTE != pPlot->getRouteType())
		{
			szBuffer.append(NEWLINE);
			szBuffer.append(gDLL->getText("TXT_KEY_EVENT_ROUTE_REMOVE", GC.getRouteInfo(pPlot->getRouteType()).getTextKeyWide()));
		}
	}

	int aiYields[NUM_YIELD_TYPES];
	for (int i = 0; i < NUM_YIELD_TYPES; ++i)
	{
		aiYields[i] = kEvent.getPlotExtraYield(i);
	}

	CvWStringBuffer szYield;
	setYieldChangeHelp(szYield, L"", L"", L"", aiYields, false, false);
	if (!szYield.isEmpty())
	{
		szBuffer.append(NEWLINE);
		szBuffer.append(gDLL->getText("TXT_KEY_EVENT_YIELD_CHANGE_PLOT", szYield.getCString()));
	}

	if (NO_BONUS != kEvent.getBonusRevealed())
	{
		szBuffer.append(NEWLINE);
		szBuffer.append(gDLL->getText("TXT_KEY_EVENT_BONUS_REVEALED", GC.getBonusInfo((BonusTypes)kEvent.getBonusRevealed()).getTextKeyWide()));
	}

	if (kEvent.getUnitExperience() != 0)
	{
		szBuffer.append(NEWLINE);
		szBuffer.append(gDLL->getText("TXT_KEY_EVENT_UNIT_EXPERIENCE", kEvent.getUnitExperience(), szUnit.GetCString()));
	}

	if (kEvent.isDisbandUnit() != 0)
	{
		szBuffer.append(NEWLINE);
		szBuffer.append(gDLL->getText("TXT_KEY_EVENT_UNIT_DISBAND", szUnit.GetCString()));
	}

	if (NO_PROMOTION != kEvent.getUnitPromotion())
	{
		szBuffer.append(NEWLINE);
		szBuffer.append(gDLL->getText("TXT_KEY_EVENT_UNIT_PROMOTION", szUnit.GetCString(), GC.getPromotionInfo((PromotionTypes)kEvent.getUnitPromotion()).getTextKeyWide()));
	}

	for (int i = 0; i < GC.getNumUnitCombatInfos(); ++i)
	{
		if (NO_PROMOTION != kEvent.getUnitCombatPromotion(i))
		{
			szBuffer.append(NEWLINE);
			szBuffer.append(gDLL->getText("TXT_KEY_EVENT_UNIT_COMBAT_PROMOTION", GC.getUnitCombatInfo((UnitCombatTypes)i).getTextKeyWide(), GC.getPromotionInfo((PromotionTypes)kEvent.getUnitCombatPromotion(i)).getTextKeyWide()));
		}
	}

	for (int i = 0; i < GC.getNumUnitClassInfos(); ++i)
	{
		if (NO_PROMOTION != kEvent.getUnitClassPromotion(i))
		{
			UnitTypes ePromotedUnit = ((UnitTypes)(GC.getCivilizationInfo(kActivePlayer.getCivilizationType()).getCivilizationUnits(i)));
			if (NO_UNIT != ePromotedUnit)
			{
				szBuffer.append(NEWLINE);
				szBuffer.append(gDLL->getText("TXT_KEY_EVENT_UNIT_CLASS_PROMOTION", GC.getUnitInfo(ePromotedUnit).getTextKeyWide(), GC.getPromotionInfo((PromotionTypes)kEvent.getUnitClassPromotion(i)).getTextKeyWide()));
			}
		}
	}

	if (kEvent.getConvertOwnCities() > 0)
	{
		szBuffer.append(NEWLINE);
		szBuffer.append(gDLL->getText("TXT_KEY_EVENT_CONVERT_OWN_CITIES", kEvent.getConvertOwnCities(), szReligion.GetCString()));
	}

	if (kEvent.getConvertOtherCities() > 0)
	{
		szBuffer.append(NEWLINE);
		szBuffer.append(gDLL->getText("TXT_KEY_EVENT_CONVERT_OTHER_CITIES", kEvent.getConvertOtherCities(), szReligion.GetCString()));
	}

	if (NO_PLAYER != pTriggeredData->m_eOtherPlayer)
	{
		if (kEvent.getAttitudeModifier() > 0)
		{
			szBuffer.append(NEWLINE);
			szBuffer.append(gDLL->getText("TXT_KEY_EVENT_ATTITUDE_GOOD", kEvent.getAttitudeModifier(), GET_PLAYER(pTriggeredData->m_eOtherPlayer).getNameKey()));
		}
		else if (kEvent.getAttitudeModifier() < 0)
		{
			szBuffer.append(NEWLINE);
			szBuffer.append(gDLL->getText("TXT_KEY_EVENT_ATTITUDE_BAD", kEvent.getAttitudeModifier(), GET_PLAYER(pTriggeredData->m_eOtherPlayer).getNameKey()));
		}
	}

	if (NO_PLAYER != pTriggeredData->m_eOtherPlayer)
	{
		TeamTypes eWorstEnemy = GET_TEAM(GET_PLAYER(pTriggeredData->m_eOtherPlayer).getTeam()).AI_getWorstEnemy();
		if (NO_TEAM != eWorstEnemy)
		{
			if (kEvent.getTheirEnemyAttitudeModifier() > 0)
			{
				szBuffer.append(NEWLINE);
				szBuffer.append(gDLL->getText("TXT_KEY_EVENT_ATTITUDE_GOOD", kEvent.getTheirEnemyAttitudeModifier(), GET_TEAM(eWorstEnemy).getName().GetCString()));
			}
			else if (kEvent.getTheirEnemyAttitudeModifier() < 0)
			{
				szBuffer.append(NEWLINE);
				szBuffer.append(gDLL->getText("TXT_KEY_EVENT_ATTITUDE_BAD", kEvent.getTheirEnemyAttitudeModifier(), GET_TEAM(eWorstEnemy).getName().GetCString()));
			}
		}
	}

	if (NO_PLAYER != pTriggeredData->m_eOtherPlayer)
	{
		if (kEvent.getEspionagePoints() > 0)
		{
			szBuffer.append(NEWLINE);
			szBuffer.append(gDLL->getText("TXT_KEY_EVENT_ESPIONAGE_POINTS", kEvent.getEspionagePoints(), GET_PLAYER(pTriggeredData->m_eOtherPlayer).getNameKey()));
		}
		else if (kEvent.getEspionagePoints() < 0)
		{
			szBuffer.append(NEWLINE);
			szBuffer.append(gDLL->getText("TXT_KEY_EVENT_ESPIONAGE_COST", -kEvent.getEspionagePoints()));
		}
	}

	if (kEvent.isGoldenAge())
	{
		szBuffer.append(NEWLINE);
		szBuffer.append(gDLL->getText("TXT_KEY_EVENT_GOLDEN_AGE"));
	}

	if (kEvent.getFreeUnitSupport() != 0)
	{
		szBuffer.append(NEWLINE);
		szBuffer.append(gDLL->getText("TXT_KEY_EVENT_FREE_UNIT_SUPPORT", kEvent.getFreeUnitSupport()));
	}

	if (kEvent.getInflationModifier() != 0)
	{
		szBuffer.append(NEWLINE);
		szBuffer.append(gDLL->getText("TXT_KEY_EVENT_INFLATION_MODIFIER", kEvent.getInflationModifier()));
	}

	if (kEvent.isDeclareWar())
	{
		szBuffer.append(NEWLINE);
		szBuffer.append(gDLL->getText("TXT_KEY_EVENT_DECLARE_WAR", GET_PLAYER(pTriggeredData->m_eOtherPlayer).getCivilizationAdjectiveKey()));
	}

	if (kEvent.getUnitImmobileTurns() > 0)
	{
		szBuffer.append(NEWLINE);
		szBuffer.append(gDLL->getText("TXT_KEY_EVENT_IMMOBILE_UNIT", kEvent.getUnitImmobileTurns(), szUnit.GetCString()));
	}

	if (!CvWString(kEvent.getPythonHelp()).empty())
	{
		CvWString szHelp;
		CyArgsList argsList;
		argsList.add(eEvent);
		argsList.add(gDLL->getPythonIFace()->makePythonObject(pTriggeredData));

		gDLL->getPythonIFace()->callFunction(PYRandomEventModule, kEvent.getPythonHelp(), argsList.makeFunctionArgs(), &szHelp);

		szBuffer.append(NEWLINE);
		szBuffer.append(szHelp);
	}

	CvWStringBuffer szTemp;
	for (int i = 0; i < GC.getNumEventInfos(); ++i)
	{
		if (kEvent.getAdditionalEventTime(i) == 0)
		{
			if (kEvent.getAdditionalEventChance(i) > 0)
			{
				if (GET_PLAYER(GC.getGame().getActivePlayer()).canDoEvent((EventTypes)i, *pTriggeredData))
				{
					szTemp.clear();
					setEventHelp(szTemp, (EventTypes)i, iEventTriggeredId, ePlayer);

					if (!szTemp.isEmpty())
					{
						szBuffer.append(NEWLINE);
						szBuffer.append(gDLL->getText("TXT_KEY_EVENT_ADDITIONAL_CHANCE", kEvent.getAdditionalEventChance(i), L""));
						szBuffer.append(NEWLINE);
						szBuffer.append(szTemp);
					}
				}
			}
		}
		else
		{
			szTemp.clear();
			setEventHelp(szTemp, (EventTypes)i, iEventTriggeredId, ePlayer);

			if (!szTemp.isEmpty())
			{
				CvWString szDelay = gDLL->getText("TXT_KEY_EVENT_DELAY_TURNS", (GC.getGameSpeedInfo(GC.getGame().getGameSpeedType()).getGrowthPercent() * kEvent.getAdditionalEventTime((EventTypes)i)) / 100);

				if (kEvent.getAdditionalEventChance(i) > 0)
				{
					szBuffer.append(NEWLINE);
					szBuffer.append(gDLL->getText("TXT_KEY_EVENT_ADDITIONAL_CHANCE", kEvent.getAdditionalEventChance(i), szDelay.GetCString()));
				}
				else
				{
					szBuffer.append(NEWLINE);
					szBuffer.append(gDLL->getText("TXT_KEY_EVENT_DELAY", szDelay.GetCString()));
				}

				szBuffer.append(NEWLINE);
				szBuffer.append(szTemp);
			}
		}
	}

	if (NO_TECH != kEvent.getPrereqTech())
	{
		if (!GET_TEAM(kActivePlayer.getTeam()).isHasTech((TechTypes)kEvent.getPrereqTech()))
		{
			szBuffer.append(NEWLINE);
			szBuffer.append(gDLL->getText("TXT_KEY_BUILDING_REQUIRES_STRING", GC.getTechInfo((TechTypes)(kEvent.getPrereqTech())).getTextKeyWide()));
		}
	}

	bool done = false;
	while(!done)
	{
		done = true;
		if(!szBuffer.isEmpty())
		{
			const wchar* wideChar = szBuffer.getCString();
			if(wideChar[0] == L'\n')
			{
				CvWString tempString(&wideChar[1]);
				szBuffer.clear();
				szBuffer.append(tempString);
				done = false;
			}
		}
	}
}

void CvGameTextMgr::eventTechHelp(CvWStringBuffer& szBuffer, EventTypes eEvent, TechTypes eTech, PlayerTypes eActivePlayer, PlayerTypes eOtherPlayer)
{
	CvEventInfo& kEvent = GC.getEventInfo(eEvent);

	if (eTech != NO_TECH)
	{
		if (100 == kEvent.getTechPercent())
		{
			if (NO_PLAYER != eOtherPlayer)
			{
				szBuffer.append(NEWLINE);
				szBuffer.append(gDLL->getText("TXT_KEY_EVENT_TECH_GAINED_FROM_PLAYER", GC.getTechInfo(eTech).getTextKeyWide(), GET_PLAYER(eOtherPlayer).getNameKey()));
			}
			else
			{
				szBuffer.append(NEWLINE);
				szBuffer.append(gDLL->getText("TXT_KEY_EVENT_TECH_GAINED", GC.getTechInfo(eTech).getTextKeyWide()));
			}
		}
		else if (kEvent.getTechPercent() != 0)
		{
			CvTeam& kTeam = GET_TEAM(GET_PLAYER(eActivePlayer).getTeam());
			int iBeakers = (kTeam.getResearchCost(eTech) * kEvent.getTechPercent()) / 100;
			if (kEvent.getTechPercent() > 0)
			{
				iBeakers = std::min(kTeam.getResearchLeft(eTech), iBeakers);
			}
			else if (kEvent.getTechPercent() < 0)
			{
				iBeakers = std::max(kTeam.getResearchLeft(eTech) - kTeam.getResearchCost(eTech), iBeakers);
			}

			if (NO_PLAYER != eOtherPlayer)
			{
				szBuffer.append(NEWLINE);
				szBuffer.append(gDLL->getText("TXT_KEY_EVENT_TECH_GAINED_FROM_PLAYER_PERCENT", iBeakers, GC.getTechInfo(eTech).getTextKeyWide(), GET_PLAYER(eOtherPlayer).getNameKey()));
			}
			else
			{
				szBuffer.append(NEWLINE);
				szBuffer.append(gDLL->getText("TXT_KEY_EVENT_TECH_GAINED_PERCENT", iBeakers, GC.getTechInfo(eTech).getTextKeyWide()));
			}
		}
	}
}

void CvGameTextMgr::eventGoldHelp(CvWStringBuffer& szBuffer, EventTypes eEvent, PlayerTypes ePlayer, PlayerTypes eOtherPlayer)
{
	CvEventInfo& kEvent = GC.getEventInfo(eEvent);
	CvPlayer& kPlayer = GET_PLAYER(ePlayer);

	int iGold1 = kPlayer.getEventCost(eEvent, eOtherPlayer, false);
	int iGold2 = kPlayer.getEventCost(eEvent, eOtherPlayer, true);

	if (iGold1 != 0 || iGold2 != 0)
	{
		if (iGold1 == iGold2)
		{
			if (NO_PLAYER != eOtherPlayer && kEvent.isGoldToPlayer())
			{
				if (iGold1 > 0)
				{
					szBuffer.append(NEWLINE);
					szBuffer.append(gDLL->getText("TXT_KEY_EVENT_GOLD_FROM_PLAYER", iGold1, GET_PLAYER(eOtherPlayer).getNameKey()));
				}
				else
				{
					szBuffer.append(NEWLINE);
					szBuffer.append(gDLL->getText("TXT_KEY_EVENT_GOLD_TO_PLAYER", -iGold1, GET_PLAYER(eOtherPlayer).getNameKey()));
				}
			}
			else
			{
				if (iGold1 > 0)
				{
					szBuffer.append(NEWLINE);
					szBuffer.append(gDLL->getText("TXT_KEY_EVENT_GOLD_GAINED", iGold1));
				}
				else
				{
					szBuffer.append(NEWLINE);
					szBuffer.append(gDLL->getText("TXT_KEY_EVENT_GOLD_LOST", -iGold1));
				}
			}
		}
		else
		{
			if (NO_PLAYER != eOtherPlayer && kEvent.isGoldToPlayer())
			{
				if (iGold1 > 0)
				{
					szBuffer.append(NEWLINE);
					szBuffer.append(gDLL->getText("TXT_KEY_EVENT_GOLD_RANGE_FROM_PLAYER", iGold1, iGold2, GET_PLAYER(eOtherPlayer).getNameKey()));
				}
				else
				{
					szBuffer.append(NEWLINE);
					szBuffer.append(gDLL->getText("TXT_KEY_EVENT_GOLD_RANGE_TO_PLAYER", -iGold1, -iGold2, GET_PLAYER(eOtherPlayer).getNameKey()));
				}
			}
			else
			{
				if (iGold1 > 0)
				{
					szBuffer.append(NEWLINE);
					szBuffer.append(gDLL->getText("TXT_KEY_EVENT_GOLD_RANGE_GAINED", iGold1, iGold2));
				}
				else
				{
					szBuffer.append(NEWLINE);
					szBuffer.append(gDLL->getText("TXT_KEY_EVENT_GOLD_RANGE_LOST", -iGold1, iGold2));
				}
			}
		}
	}
}

void CvGameTextMgr::setTradeRouteHelp(CvWStringBuffer &szBuffer, int iRoute, CvCity* pCity)
{
	if (NULL != pCity)
	{
		CvCity* pOtherCity = pCity->getTradeCity(iRoute);

		if (NULL != pOtherCity)
		{
			szBuffer.append(pOtherCity->getName());

			int iProfit = pCity->getBaseTradeProfit(pOtherCity);

			szBuffer.append(NEWLINE);
			CvWString szBaseProfit;
			szBaseProfit.Format(L"%d.%02d", iProfit/100, iProfit%100);
			szBuffer.append(gDLL->getText("TXT_KEY_TRADE_ROUTE_HELP_BASE", szBaseProfit.GetCString()));

			int iModifier = 100;
			int iNewMod;

			for (int iBuilding = 0; iBuilding < GC.getNumBuildingInfos(); ++iBuilding)
			{
				if (pCity->getNumActiveBuilding((BuildingTypes)iBuilding) > 0)
				{
					iNewMod = pCity->getNumActiveBuilding((BuildingTypes)iBuilding) * GC.getBuildingInfo((BuildingTypes)iBuilding).getTradeRouteModifier();
					if (iNewMod != 0)
					{
						szBuffer.append(NEWLINE);
						szBuffer.append(gDLL->getText("TXT_KEY_TRADE_ROUTE_MOD_BUILDING", GC.getBuildingInfo((BuildingTypes)iBuilding).getTextKeyWide(), iNewMod));
						iModifier += iNewMod;
					}
				}
			}

			iNewMod = pCity->getPopulationTradeModifier();
			if (iNewMod != 0)
			{
				szBuffer.append(NEWLINE);
				szBuffer.append(gDLL->getText("TXT_KEY_TRADE_ROUTE_MOD_POPULATION", iNewMod));
				iModifier += iNewMod;
			}

			if (pCity->isConnectedToCapital())
			{
				iNewMod = GC.getDefineINT("CAPITAL_TRADE_MODIFIER");
				if (iNewMod != 0)
				{
					szBuffer.append(NEWLINE);
					szBuffer.append(gDLL->getText("TXT_KEY_TRADE_ROUTE_MOD_CAPITAL", iNewMod));
					iModifier += iNewMod;
				}
			}

			if (NULL != pOtherCity)
			{
				if (pCity->area() != pOtherCity->area())
				{
					iNewMod = GC.getDefineINT("OVERSEAS_TRADE_MODIFIER");
					if (iNewMod != 0)
					{
						szBuffer.append(NEWLINE);
						szBuffer.append(gDLL->getText("TXT_KEY_TRADE_ROUTE_MOD_OVERSEAS", iNewMod));
						iModifier += iNewMod;
					}
				}

				if (pCity->getTeam() != pOtherCity->getTeam())
				{
					iNewMod = pCity->getForeignTradeRouteModifier();
					if (iNewMod != 0)
					{
						szBuffer.append(NEWLINE);
						szBuffer.append(gDLL->getText("TXT_KEY_TRADE_ROUTE_MOD_FOREIGN", iNewMod));
						iModifier += iNewMod;
					}

					iNewMod = pCity->getPeaceTradeModifier(pOtherCity->getTeam());
					if (iNewMod != 0)
					{
						szBuffer.append(NEWLINE);
						szBuffer.append(gDLL->getText("TXT_KEY_TRADE_ROUTE_MOD_PEACE", iNewMod));
						iModifier += iNewMod;
					}
				}
			}

			FAssert(pCity->totalTradeModifier(pOtherCity) == iModifier);

			iProfit *= iModifier;
			iProfit /= 10000;

			FAssert(iProfit == pCity->calculateTradeProfit(pOtherCity));

			szBuffer.append(SEPARATOR);

			szBuffer.append(NEWLINE);
			szBuffer.append(gDLL->getText("TXT_KEY_TRADE_ROUTE_TOTAL", iProfit));
		}
	}
}

void CvGameTextMgr::setEspionageCostHelp(CvWStringBuffer &szBuffer, EspionageMissionTypes eMission, PlayerTypes eTargetPlayer, const CvPlot* pPlot, int iExtraData, const CvUnit* pSpyUnit)
{
	CvPlayer& kPlayer = GET_PLAYER(GC.getGame().getActivePlayer());
	CvEspionageMissionInfo& kMission = GC.getEspionageMissionInfo(eMission);

	//szBuffer.assign(kMission.getDescription());

	int iMissionCost = kPlayer.getEspionageMissionBaseCost(eMission, eTargetPlayer, pPlot, iExtraData, pSpyUnit);
	//iMissionCost *= GET_TEAM(kPlayer.getTeam()).getNumMembers(); // K-Mod
	// dlph.33/advc:
	iMissionCost = kPlayer.adjustMissionCostToTeamSize(iMissionCost, eTargetPlayer);

	if (kMission.isDestroyImprovement())
	{
		if (NULL != pPlot && NO_IMPROVEMENT != pPlot->getImprovementType())
		{
			szBuffer.append(gDLL->getText("TXT_KEY_ESPIONAGE_HELP_DESTROY_IMPROVEMENT", GC.getImprovementInfo(pPlot->getImprovementType()).getTextKeyWide()));
			szBuffer.append(NEWLINE);
		}
	}

	if (kMission.getDestroyBuildingCostFactor() > 0)
	{
		BuildingTypes eTargetBuilding = (BuildingTypes)iExtraData;

		if (NULL != pPlot)
		{
			CvCity* pCity = pPlot->getPlotCity();

			if (NULL != pCity)
			{
				szBuffer.append(gDLL->getText("TXT_KEY_ESPIONAGE_HELP_DESTROY_IMPROVEMENT", GC.getBuildingInfo(eTargetBuilding).getTextKeyWide()));
				szBuffer.append(NEWLINE);
			}
		}
	}

	if (kMission.getDestroyProjectCostFactor() > 0)
	{
		ProjectTypes eTargetProject = (ProjectTypes)iExtraData;

		if (NULL != pPlot)
		{
			CvCity* pCity = pPlot->getPlotCity();

			if (NULL != pCity)
			{
				szBuffer.append(gDLL->getText("TXT_KEY_ESPIONAGE_HELP_DESTROY_IMPROVEMENT", GC.getProjectInfo(eTargetProject).getTextKeyWide()));
				szBuffer.append(NEWLINE);
			}
		}
	}

	if (kMission.getDestroyProductionCostFactor() > 0)
	{
		if (NULL != pPlot)
		{
			CvCity* pCity = pPlot->getPlotCity();

			if (NULL != pCity)
			{
				szBuffer.append(gDLL->getText("TXT_KEY_ESPIONAGE_HELP_DESTROY_PRODUCTION", pCity->getProduction()));
				szBuffer.append(NEWLINE);
			}
		}
	}

	if (kMission.getDestroyUnitCostFactor() > 0)
	{
		if (NO_PLAYER != eTargetPlayer)
		{
			int iTargetUnitID = iExtraData;

			CvUnit* pUnit = GET_PLAYER(eTargetPlayer).getUnit(iTargetUnitID);

			if (NULL != pUnit)
			{
				szBuffer.append(gDLL->getText("TXT_KEY_ESPIONAGE_HELP_DESTROY_UNIT", pUnit->getNameKey()));
				szBuffer.append(NEWLINE);
			}
		}
	}

	if (kMission.getBuyUnitCostFactor() > 0)
	{
		if (NO_PLAYER != eTargetPlayer)
		{
			int iTargetUnitID = iExtraData;

			CvUnit* pUnit = GET_PLAYER(eTargetPlayer).getUnit(iTargetUnitID);

			if (NULL != pUnit)
			{
				szBuffer.append(gDLL->getText("TXT_KEY_ESPIONAGE_HELP_BRIBE", pUnit->getNameKey()));
				szBuffer.append(NEWLINE);
			}
		}
	}

	if (kMission.getBuyCityCostFactor() > 0)
	{
		if (NULL != pPlot)
		{
			CvCity* pCity = pPlot->getPlotCity();

			if (NULL != pCity)
			{
				szBuffer.append(gDLL->getText("TXT_KEY_ESPIONAGE_HELP_BRIBE", pCity->getNameKey()));
				szBuffer.append(NEWLINE);
			}
		}
	}

	if (kMission.getCityInsertCultureCostFactor() > 0)
	{
		if (NULL != pPlot)
		{
			CvCity* pCity = pPlot->getPlotCity();

			if (NULL != pCity && pPlot->getCulture(kPlayer.getID()) > 0)
			{
				int iCultureAmount = kMission.getCityInsertCultureAmountFactor() *  pCity->countTotalCultureTimes100();
				iCultureAmount /= 10000;
				iCultureAmount = std::max(1, iCultureAmount);

				szBuffer.append(gDLL->getText("TXT_KEY_ESPIONAGE_HELP_INSERT_CULTURE",
						pCity->getNameKey(), iCultureAmount,
						kMission.getCityInsertCultureAmountFactor()));
				szBuffer.append(NEWLINE);
			}
		}
	}

	if (kMission.getCityPoisonWaterCounter() > 0)
	{
		if (NULL != pPlot)
		{
			CvCity* pCity = pPlot->getPlotCity();

			if (NULL != pCity)
			{
				szBuffer.append(gDLL->getText("TXT_KEY_ESPIONAGE_HELP_POISON",
						kMission.getCityPoisonWaterCounter(),
						gDLL->getSymbolID(UNHEALTHY_CHAR), pCity->getNameKey(),
						kMission.getCityPoisonWaterCounter()));
				szBuffer.append(NEWLINE);
			}
		}
	}

	if (kMission.getCityUnhappinessCounter() > 0)
	{
		if (NULL != pPlot)
		{
			CvCity* pCity = pPlot->getPlotCity();

			if (NULL != pCity)
			{
				szBuffer.append(gDLL->getText("TXT_KEY_ESPIONAGE_HELP_POISON",
						kMission.getCityUnhappinessCounter(),
						gDLL->getSymbolID(UNHAPPY_CHAR), pCity->getNameKey(),
						kMission.getCityUnhappinessCounter()));
				szBuffer.append(NEWLINE);
			}
		}
	}

	if (kMission.getCityRevoltCounter() > 0)
	{
		if (NULL != pPlot)
		{
			CvCity* pCity = pPlot->getPlotCity();

			if (NULL != pCity)
			{
				szBuffer.append(gDLL->getText("TXT_KEY_ESPIONAGE_HELP_REVOLT",
						pCity->getNameKey(), kMission.getCityRevoltCounter()));
				szBuffer.append(NEWLINE);
			}
		}
	}

	if (kMission.getStealTreasuryTypes() > 0)
	{
		if (NO_PLAYER != eTargetPlayer)
		{
			//int iNumTotalGold = (GET_PLAYER(eTargetPlayer).getGold() * kMission.getStealTreasuryTypes()) / 100;
			int iNumTotalGold
				= 0; // kmodx: Missing initialization

			if (NULL != pPlot)
			{
				CvCity* pCity = pPlot->getPlotCity();

				if (NULL != pCity)
				{
					/* iNumTotalGold *= pCity->getPopulation();
					iNumTotalGold /= std::max(1, GET_PLAYER(eTargetPlayer).getTotalPopulation());*/
					// K-Mod
					iNumTotalGold = kPlayer.getEspionageGoldQuantity(eMission, eTargetPlayer, pCity);
				}
			}

			szBuffer.append(gDLL->getText("TXT_KEY_ESPIONAGE_HELP_STEAL_TREASURY",
					iNumTotalGold, GET_PLAYER(eTargetPlayer).getCivilizationAdjectiveKey()));
			szBuffer.append(NEWLINE);
		}
	}

	if (kMission.getBuyTechCostFactor() > 0)
	{
		szBuffer.append(gDLL->getText("TXT_KEY_ESPIONAGE_HELP_STEAL_TECH",
				GC.getTechInfo((TechTypes)iExtraData).getTextKeyWide()));
		szBuffer.append(NEWLINE);
	}

	if (kMission.getSwitchCivicCostFactor() > 0)
	{
		if (NO_PLAYER != eTargetPlayer)
		{
			szBuffer.append(gDLL->getText("TXT_KEY_ESPIONAGE_HELP_SWITCH_CIVIC", GET_PLAYER(eTargetPlayer).getNameKey(), GC.getCivicInfo((CivicTypes)iExtraData).getTextKeyWide()));
			szBuffer.append(NEWLINE);
		}
	}

	if (kMission.getSwitchReligionCostFactor() > 0)
	{
		if (NO_PLAYER != eTargetPlayer)
		{
			szBuffer.append(gDLL->getText("TXT_KEY_ESPIONAGE_HELP_SWITCH_CIVIC", GET_PLAYER(eTargetPlayer).getNameKey(), GC.getReligionInfo((ReligionTypes)iExtraData).getTextKeyWide()));
			szBuffer.append(NEWLINE);
		}
	}

	if (kMission.getPlayerAnarchyCounter() > 0)
	{
		if (NO_PLAYER != eTargetPlayer)
		{
			int iTurns = (kMission.getPlayerAnarchyCounter() * GC.getGameSpeedInfo(GC.getGame().getGameSpeedType()).getAnarchyPercent()) / 100;
			szBuffer.append(gDLL->getText("TXT_KEY_ESPIONAGE_HELP_ANARCHY", GET_PLAYER(eTargetPlayer).getNameKey(), iTurns));
			szBuffer.append(NEWLINE);
		}
	}

	if (kMission.getCounterespionageNumTurns() > 0 && kMission.getCounterespionageMod() > 0)
	{
		if (NO_PLAYER != eTargetPlayer)
		{
			int iTurns = (kMission.getCounterespionageNumTurns() * GC.getGameSpeedInfo(GC.getGame().getGameSpeedType()).getResearchPercent()) / 100;

			szBuffer.append(gDLL->getText("TXT_KEY_ESPIONAGE_HELP_COUNTERESPIONAGE", kMission.getCounterespionageMod(), GET_PLAYER(eTargetPlayer).getCivilizationAdjectiveKey(), iTurns));
			szBuffer.append(NEWLINE);
		}
	}
	// <advc.103>
	if(kMission.isInvestigateCity() && pPlot != NULL) {
		CvCity* pCity = pPlot->getPlotCity();
		if(pCity != NULL) {
			szBuffer.append(gDLL->getText("TXT_KEY_ESPIONAGE_HELP_INVESTIGATE",
					pCity->getNameKey()));
			szBuffer.append(NEWLINE);
		}
	}
	if(!kMission.isReturnToCapital() || kPlayer.getCapitalCity() == NULL) {
		szBuffer.append(NEWLINE);
		szBuffer.append(gDLL->getText("TXT_KEY_ESPIONAGE_HELP_NO_RETURN"));
		szBuffer.append(NEWLINE);
	}
	// </advc.103>
	szBuffer.append(NEWLINE);
	szBuffer.append(gDLL->getText("TXT_KEY_ESPIONAGE_BASE_COST", iMissionCost));

	if (kPlayer.getEspionageMissionCost(eMission, eTargetPlayer, pPlot, iExtraData, pSpyUnit) > 0)
	{
		int iModifier = 100;
		int iTempModifier = 0;
		CvCity* pCity = NULL;
		if (NULL != pPlot)
		{
			pCity = pPlot->getPlotCity();
		}

		if (pCity != NULL && kMission.isTargetsCity())
		{
			// City Population
			iTempModifier = (GC.getDefineINT("ESPIONAGE_CITY_POP_EACH_MOD") * (pCity->getPopulation() - 1));
			if (iTempModifier != 0)
			{
				szBuffer.append(NEWLINE);
				szBuffer.append(gDLL->getText("TXT_KEY_ESPIONAGE_POPULATION_MOD", iTempModifier));
				iModifier *= 100 + iTempModifier;
				iModifier /= 100;
			}

			// Trade Route
			if (pCity->isTradeRoute(kPlayer.getID()))
			{
				iTempModifier = GC.getDefineINT("ESPIONAGE_CITY_TRADE_ROUTE_MOD");
				if (iTempModifier != 0)
				{
					szBuffer.append(NEWLINE);
					szBuffer.append(gDLL->getText("TXT_KEY_ESPIONAGE_TRADE_ROUTE_MOD", iTempModifier));
					iModifier *= 100 + iTempModifier;
					iModifier /= 100;
				}
			}

			ReligionTypes eReligion = kPlayer.getStateReligion();
			if (NO_RELIGION != eReligion)
			{
				iTempModifier = 0;

				if (pCity->isHasReligion(eReligion))
				{
					if (GET_PLAYER(eTargetPlayer).getStateReligion() != eReligion)
					{
						iTempModifier += GC.getDefineINT("ESPIONAGE_CITY_RELIGION_STATE_MOD");
					}

					if (kPlayer.hasHolyCity(eReligion))
					{
						iTempModifier += GC.getDefineINT("ESPIONAGE_CITY_HOLY_CITY_MOD");
					}
				}

				if (iTempModifier != 0)
				{
					szBuffer.append(NEWLINE);
					szBuffer.append(gDLL->getText("TXT_KEY_ESPIONAGE_RELIGION_MOD", iTempModifier));
					iModifier *= 100 + iTempModifier;
					iModifier /= 100;
				}
			}

			// City's culture affects cost
			/* original bts code
			iTempModifier = - (pCity->getCultureTimes100(kPlayer.getID()) * GC.getDefineINT("ESPIONAGE_CULTURE_MULTIPLIER_MOD")) / std::max(1, pCity->getCultureTimes100(eTargetPlayer) + pCity->getCultureTimes100(kPlayer.getID()));
			if (iTempModifier != 0)
			{
				szBuffer.append(NEWLINE);
				szBuffer.append(gDLL->getText("TXT_KEY_ESPIONAGE_CULTURE_MOD", iTempModifier));
				iModifier *= 100 + iTempModifier;
				iModifier /= 100;
			} */ // (moved and changed by K-Mod)

			iTempModifier = pCity->getEspionageDefenseModifier();
			if (iTempModifier != 0)
			{
				szBuffer.append(NEWLINE);
				szBuffer.append(gDLL->getText("TXT_KEY_ESPIONAGE_DEFENSE_MOD", iTempModifier));
				iModifier *= 100 + iTempModifier;
				iModifier /= 100;
			}
		}

		if (pPlot != NULL)
		{
			// K-Mod. Culture Mod. (Based on plot culture rather than city culture.)
			if (kMission.isSelectPlot() || kMission.isTargetsCity())
			{
				iTempModifier = - (pPlot->getCulture(kPlayer.getID()) * GC.getDefineINT("ESPIONAGE_CULTURE_MULTIPLIER_MOD")) / std::max(1, pPlot->getCulture(eTargetPlayer) + pPlot->getCulture(kPlayer.getID()));
				if (iTempModifier != 0)
				{
					szBuffer.append(NEWLINE);
					szBuffer.append(gDLL->getText("TXT_KEY_ESPIONAGE_CULTURE_MOD", iTempModifier));
					iModifier *= 100 + iTempModifier;
					iModifier /= 100;
				}
			}
			// K-Mod end

			// Distance mod
			int iDistance = GC.getMap().maxPlotDistance();

			CvCity* pOurCapital = kPlayer.getCapitalCity();
			if (NULL != pOurCapital)
			{
				if (kMission.isSelectPlot() || kMission.isTargetsCity())
				{
					iDistance = plotDistance(pOurCapital->getX(), pOurCapital->getY(), pPlot->getX(), pPlot->getY());
				}
				else
				{
					CvCity* pTheirCapital = GET_PLAYER(eTargetPlayer).getCapitalCity();
					if (NULL != pTheirCapital)
					{
						iDistance = plotDistance(pOurCapital->getX(), pOurCapital->getY(), pTheirCapital->getX(), pTheirCapital->getY());
					}
				}
			}

			iTempModifier = (iDistance + GC.getMap().maxPlotDistance()) * GC.getDefineINT("ESPIONAGE_DISTANCE_MULTIPLIER_MOD") / GC.getMap().maxPlotDistance() - 100;
			if (iTempModifier != 0)
			{
				szBuffer.append(NEWLINE);
				szBuffer.append(gDLL->getText("TXT_KEY_ESPIONAGE_DISTANCE_MOD", iTempModifier));
				iModifier *= 100 + iTempModifier;
				iModifier /= 100;
			}
		}

		// Spy presence mission cost alteration
		if (NULL != pSpyUnit)
		{
			iTempModifier = -(pSpyUnit->getFortifyTurns() * GC.getDefineINT("ESPIONAGE_EACH_TURN_UNIT_COST_DECREASE"));
			if (iTempModifier != 0)
			{
				szBuffer.append(NEWLINE);
				szBuffer.append(gDLL->getText("TXT_KEY_ESPIONAGE_SPY_STATIONARY_MOD", iTempModifier));
				iModifier *= 100 + iTempModifier;
				iModifier /= 100;
			}
		}

		// My points VS. Your points to mod cost
		iTempModifier = ::getEspionageModifier(kPlayer.getTeam(), GET_PLAYER(eTargetPlayer).getTeam()) - 100;
		if (iTempModifier != 0)
		{
			szBuffer.append(SEPARATOR);
			szBuffer.append(NEWLINE);
			szBuffer.append(gDLL->getText("TXT_KEY_ESPIONAGE_EP_RATIO_MOD", iTempModifier));
			iModifier *= 100 + iTempModifier;
			iModifier /= 100;
		}

		// Counterespionage Mission Mod
		CvTeam& kTargetTeam = GET_TEAM(GET_PLAYER(eTargetPlayer).getTeam());
		if (kTargetTeam.getCounterespionageModAgainstTeam(kPlayer.getTeam()) > 0)
		{
			//iTempModifier = kTargetTeam.getCounterespionageModAgainstTeam(kPlayer.getTeam()) - 100;
			// K-Mod
			iTempModifier = std::max(-100, kTargetTeam.getCounterespionageModAgainstTeam(kPlayer.getTeam()));
			if (iTempModifier != 0)
			{
				szBuffer.append(SEPARATOR);
				szBuffer.append(NEWLINE);
				szBuffer.append(gDLL->getText("TXT_KEY_ESPIONAGE_COUNTERESPIONAGE_MOD", iTempModifier));
				iModifier *= 100 + iTempModifier;
				iModifier /= 100;
			}
		}

		FAssert(iModifier == kPlayer.getEspionageMissionCostModifier(eMission, eTargetPlayer, pPlot, iExtraData, pSpyUnit));

		iMissionCost *= iModifier;
		iMissionCost /= 100;

		FAssert(iMissionCost == kPlayer.getEspionageMissionCost(eMission, eTargetPlayer, pPlot, iExtraData, pSpyUnit));

		szBuffer.append(SEPARATOR);

		szBuffer.append(NEWLINE);
		szBuffer.append(gDLL->getText("TXT_KEY_ESPIONAGE_COST_TOTAL", iMissionCost));


		if (NULL != pSpyUnit)
		{
			int iInterceptChance = (pSpyUnit->getSpyInterceptPercent(GET_PLAYER(eTargetPlayer).getTeam(), true) * (100 + kMission.getDifficultyMod())) / 100;

			szBuffer.append(NEWLINE);
			szBuffer.append(NEWLINE);
			szBuffer.append(gDLL->getText("TXT_KEY_ESPIONAGE_CHANCE_OF_SUCCESS", std::min(100, std::max(0, 100 - iInterceptChance))));
		}
	}
}

void CvGameTextMgr::getTradeScreenTitleIcon(CvString& szButton, CvWidgetDataStruct& widgetData, PlayerTypes ePlayer)
{
	szButton.clear();

	ReligionTypes eReligion = GET_PLAYER(ePlayer).getStateReligion();
	if (eReligion != NO_RELIGION)
	{
		szButton = GC.getReligionInfo(eReligion).getButton();
		widgetData.m_eWidgetType = WIDGET_HELP_RELIGION;
		widgetData.m_iData1 = eReligion;
		widgetData.m_iData2 = -1;
		widgetData.m_bOption = false;
	}
}

void CvGameTextMgr::getTradeScreenIcons(std::vector< std::pair<CvString, CvWidgetDataStruct> >& aIconInfos, PlayerTypes ePlayer)
{
	aIconInfos.clear();
	for (int i = 0; i < GC.getNumCivicOptionInfos(); i++)
	{
		CivicTypes eCivic = GET_PLAYER(ePlayer).getCivics((CivicOptionTypes)i);
		CvWidgetDataStruct widgetData;
		widgetData.m_eWidgetType = WIDGET_PEDIA_JUMP_TO_CIVIC;
		widgetData.m_iData1 = eCivic;
		widgetData.m_iData2 = -1;
		widgetData.m_bOption = false;
		aIconInfos.push_back(std::make_pair(GC.getCivicInfo(eCivic).getButton(), widgetData));
	}

}

void CvGameTextMgr::getTradeScreenHeader(CvWString& szHeader, PlayerTypes ePlayer, PlayerTypes eOtherPlayer, bool bAttitude)
{
	CvPlayer& kPlayer = GET_PLAYER(ePlayer);
	szHeader.Format(L"%s - %s", CvWString(kPlayer.getName()).GetCString(), CvWString(kPlayer.getCivilizationDescription()).GetCString());
	if (bAttitude)
	{
		szHeader += CvWString::format(L" (%s)", GC.getAttitudeInfo(kPlayer.AI_getAttitude(eOtherPlayer)).getDescription());
	}
}
// BULL - Finance Advisor - start
void CvGameTextMgr::buildFinanceSpecialistGoldString(CvWStringBuffer& szBuffer, PlayerTypes ePlayer) {

	if(ePlayer == NO_PLAYER)
		return;
	CvPlayer const& kPlayer = GET_PLAYER(ePlayer);
	int* iCounts = new int[GC.getNumSpecialistInfos()](); int foo;
	for(CvCity* pCity = kPlayer.firstCity(&foo); pCity != NULL; pCity = kPlayer.nextCity(&foo)) {
		if(!pCity->isDisorder()) {
			for(int i = 0; i < GC.getNumSpecialistInfos(); i++) {
				SpecialistTypes eSpecialist = (SpecialistTypes)i;
				iCounts[i] += pCity->getSpecialistCount(eSpecialist) +
						pCity->getFreeSpecialistCount(eSpecialist);
			}
		}
	}
	//bool bFirst = true; // advc.086
	int iTotal = 0;
	for(int i = 0; i < GC.getNumSpecialistInfos(); i++) {
		SpecialistTypes eSpecialist = (SpecialistTypes)i;
		int iGold = iCounts[i] * kPlayer.specialistCommerce(eSpecialist, COMMERCE_GOLD);
		if(iGold != 0) {
			/*if(bFirst) {
				szBuffer.append(NEWLINE);
				bFirst = false;
			}*/
			szBuffer.append(gDLL->getText("TXT_KEY_BUG_FINANCIAL_ADVISOR_SPECIALIST_GOLD",
					iGold, iCounts[i], GC.getSpecialistInfo(eSpecialist).getDescription()));
			szBuffer.append(NEWLINE); // advc.086
			iTotal += iGold;
		}
	}
	szBuffer.append(gDLL->getText("TXT_KEY_BUG_FINANCIAL_ADVISOR_SPECIALIST_TOTAL_GOLD", iTotal));
	SAFE_DELETE_ARRAY(iCounts);
}

void CvGameTextMgr::buildDomesticTradeString(CvWStringBuffer& szBuffer, PlayerTypes ePlayer) {

	//buildTradeString(szBuffer, ePlayer, NO_PLAYER, true, false, false);
	// <advc.086> Replacing the above
	if(ePlayer == NO_PLAYER)
		return;
	int foo1 = 0, foo2 = 0;
	int iDomesticRoutes = 0, iDomesticYield = 0;
	GET_PLAYER(ePlayer).calculateTradeTotals(YIELD_COMMERCE, iDomesticYield, iDomesticRoutes,
			foo1, foo2, NO_PLAYER);
	if(iDomesticRoutes > 1) {
		szBuffer.assign(gDLL->getText("TXT_KEY_BUG_FINANCIAL_ADVISOR_DOMESTIC_TRADE",
				iDomesticYield, iDomesticRoutes));
	} // </advc.086>
}

void CvGameTextMgr::buildForeignTradeString(CvWStringBuffer& szBuffer, PlayerTypes ePlayer) {

	//buildTradeString(szBuffer, ePlayer, NO_PLAYER, false, true, false);
	// <advc.086> Replacing the above
	if(ePlayer == NO_PLAYER)
		return;
	int foo1 = 0, foo2 = 0;
	int iForeignRoutes = 0, iForeignYield = 0;
	GET_PLAYER(ePlayer).calculateTradeTotals(YIELD_COMMERCE, foo1, foo2, iForeignYield,
			iForeignRoutes, NO_PLAYER);
	if(iForeignRoutes > 1) {
		szBuffer.assign(gDLL->getText("TXT_KEY_BUG_FINANCIAL_ADVISOR_FOREIGN_TRADE",
				iForeignYield, iForeignRoutes));
	} // </advc.086>
} // BULL - Finance Advisor - end
// BULL - Trade Hover - start  // advc: style changes, _MOD_FRACTRADE removed
void CvGameTextMgr::buildTradeString(CvWStringBuffer& szBuffer, PlayerTypes ePlayer,
		PlayerTypes eWithPlayer, bool bDomestic, bool bForeign, bool bHeading) {

	if(ePlayer == NO_PLAYER)
		return;
	CvPlayer const& kPlayer = GET_PLAYER(ePlayer);
	if(bHeading) {
		if(ePlayer == eWithPlayer)
			szBuffer.append(gDLL->getText("TXT_KEY_BUG_DOMESTIC_TRADE_HEADING"));
		else if(eWithPlayer != NO_PLAYER) {
			CvPlayer const& kWithPlayer = GET_PLAYER(eWithPlayer);
			if(kPlayer.canHaveTradeRoutesWith(eWithPlayer)) {
				szBuffer.append(gDLL->getText("TXT_KEY_BUG_FOREIGN_TRADE_HEADING",
						// advc: 2nd param takes up too much room
						kWithPlayer.getNameKey()/*, kWithPlayer.getCivilizationShortDescription()*/));
			}
			else {
				szBuffer.append(gDLL->getText("TXT_KEY_BUG_CANNOT_TRADE_HEADING",
						kWithPlayer.getNameKey()/*, kWithPlayer.getCivilizationShortDescription()*/));
			}
		}
		else szBuffer.append(gDLL->getText("TXT_KEY_BUG_TRADE_HEADING"));
		//szBuffer.append(NEWLINE); // advc.087: removed
	}

	if(eWithPlayer != NO_PLAYER) {
		bDomestic = (ePlayer == eWithPlayer);
		bForeign = (ePlayer != eWithPlayer);
		if(bForeign && !kPlayer.canHaveTradeRoutesWith(eWithPlayer)) {
			CvPlayer const& kWithPlayer = GET_PLAYER(eWithPlayer);
			bool bCanTrade = true;
			if(!kWithPlayer.isAlive()) {
				szBuffer.append(gDLL->getText("TXT_KEY_BUG_CANNOT_TRADE_DEAD"));
				return;
			}
			if(!kPlayer.canTradeNetworkWith(eWithPlayer)) {
				szBuffer.append(gDLL->getText("TXT_KEY_BUG_CANNOT_TRADE_NETWORK_NOT_CONNECTED"));
				bCanTrade = false;
			}
			if(!GET_TEAM(kPlayer.getTeam()).isFreeTrade(kWithPlayer.getTeam())) {
				szBuffer.append(gDLL->getText("TXT_KEY_BUG_CANNOT_TRADE_CLOSED_BORDERS"));
				bCanTrade = false;
			}
			if(kPlayer.isNoForeignTrade()) {
				szBuffer.append(gDLL->getText("TXT_KEY_BUG_CANNOT_TRADE_FOREIGN_YOU"));
				bCanTrade = false;
			}
			if(kWithPlayer.isNoForeignTrade()) {
				szBuffer.append(gDLL->getText("TXT_KEY_BUG_CANNOT_TRADE_FOREIGN_THEM"));
				bCanTrade = false;
			}
			// <advc.124>
			bool bAnyCityKnown = false; int foo;
			for(CvCity* pLoopCity = kWithPlayer.firstCity(&foo); pLoopCity != NULL;
					pLoopCity = kWithPlayer.nextCity(&foo)) {
				if(pLoopCity->isRevealed(kPlayer.getTeam(), false)) {
					bAnyCityKnown = true;
					break;
				}
			}
			if(!bAnyCityKnown) {
				szBuffer.append(gDLL->getText("TXT_KEY_BUG_CANNOT_TRADE_NO_KNOWN_CITIES"));
				bCanTrade = false;
			} // </advc.124>
			if (!bCanTrade)
				return;
		}
	}
	int iDomesticYield = 0;
	int iDomesticRoutes = 0;
	int iForeignYield = 0;
	int iForeignRoutes = 0;
	kPlayer.calculateTradeTotals(YIELD_COMMERCE, iDomesticYield, iDomesticRoutes,
			iForeignYield, iForeignRoutes, eWithPlayer);
	int iTotalYield = 0;
	int iTotalRoutes = 0;
	if(bDomestic) {
		iTotalYield += iDomesticYield;
		iTotalRoutes += iDomesticRoutes;
	}
	if(bForeign) {
		iTotalYield += iForeignYield;
		iTotalRoutes += iForeignRoutes;
	}
	CvWString szYield;
	szYield.Format(L"%d", iTotalYield);
	/*szBuffer.append(gDLL->getText("TXT_KEY_BUG_TOTAL_TRADE_YIELD", szYield.GetCString()));
	szBuffer.append(gDLL->getText("TXT_KEY_BUG_TOTAL_TRADE_ROUTES", iTotalRoutes));
	if(iTotalRoutes > 0) {
		int iAverage = 100 * iTotalYield / iTotalRoutes;
		CvWString szAverage;
		szAverage.Format(L"%d.%02d", iAverage / 100, iAverage % 100);
		szBuffer.append(gDLL->getText("TXT_KEY_BUG_AVERAGE_TRADE_YIELD", szAverage.GetCString()));
	}*/
	// <advc.086> Shorter:
	szBuffer.append(NEWLINE);
	szBuffer.append(gDLL->getText("TXT_KEY_BUG_COMMERCE_FROM_ROUTES", iTotalYield, iTotalRoutes));
	// </advc.086>
} // BULL - Trade Hover - end
// BULL - Leaderhead Relations - start  // advc: style changes
// Displays the relations between two leaders only. This is used by the F4:GLANCE and F5:SIT-REP tabs.
void CvGameTextMgr::parseLeaderHeadRelationsHelp(CvWStringBuffer &szBuffer,
		PlayerTypes eThisPlayer, PlayerTypes eOtherPlayer) {

	if(eThisPlayer == NO_PLAYER)
		return;
	if(eOtherPlayer == NO_PLAYER) {
		parseLeaderHeadHelp(szBuffer, eThisPlayer, NO_PLAYER);
		return;
	}
	szBuffer.append(CvWString::format(L"%s", GET_PLAYER(eThisPlayer).getName()));
	parsePlayerTraits(szBuffer, eThisPlayer);
	//szBuffer.append(NEWLINE); // advc: removed
	PlayerTypes eActivePlayer = GC.getGame().getActivePlayer();
	TeamTypes eThisTeam = GET_PLAYER(eThisPlayer).getTeam();
	CvTeam const& kThisTeam = GET_TEAM(eThisTeam);
	if(eThisPlayer != eOtherPlayer && kThisTeam.isHasMet(TEAMID(eOtherPlayer))) {
		// advc: K-Mod keeps this secret
		//getEspionageString(szBuffer, eThisPlayer, eOtherPlayer);
		getAttitudeString(szBuffer, eThisPlayer, eOtherPlayer);
		getActiveDealsString(szBuffer, eThisPlayer, eOtherPlayer);
		if(eOtherPlayer == eActivePlayer)
			getActiveTeamRelationsString(szBuffer, eThisTeam);
		else getOtherRelationsString(szBuffer, eThisPlayer, eOtherPlayer);
	}
	else getAllRelationsString(szBuffer, eThisTeam);
} // BULL - Leaderhead Relations - end
// BUG - Food Rate Hover - start
/*
	+14 from Worked Tiles
	+2 from Specialists
	+5 from Corporations
	+1 from Buildings
	----------------------- |
	Base Food Produced: 22  |-- only if there are modifiers
	+25% from Buildings     |
	-----------------------
	Total Food Produced: 27
	=======================
	+16 for Population
	+2 for Health
	-----------------------
	Total Food Consumed: 18
	=======================
	Net Food: +9            or
	Net Food for Settler: 9
	=======================
	* Lighthouse: +3
	* Supermarket: +1
*/
void CvGameTextMgr::setFoodHelp(CvWStringBuffer &szBuffer, CvCity const& kCity) {

	CvYieldInfo const& kFood = GC.getYieldInfo(YIELD_FOOD);
	bool bNeedSubtotal = false;
	int iBaseRate = 0;
	bool bSimple = true; // advc.087

	// Worked Tiles
	int iTileFood = 0;
	for(int i = 0; i < NUM_CITY_PLOTS; i++) {
		CvPlot* pPlot = kCity.getCityIndexPlot(i);
		if(pPlot != NULL && kCity.isWorkingPlot(i))
			iTileFood += pPlot->getYield(YIELD_FOOD);
	}
	if(iTileFood != 0) {
		szBuffer.append(gDLL->getText("TXT_KEY_MISC_HELP_WORKED_TILES_YIELD",
				iTileFood, kFood.getChar()));
		iBaseRate += iTileFood;
	}
	// Trade
	int iTradeFood = kCity.getTradeYield(YIELD_FOOD);
	if(iTradeFood != 0) {
		bSimple = false; // advc.087
		szBuffer.append(NEWLINE);
		szBuffer.append(gDLL->getText("TXT_KEY_MISC_HELP_SPECIALIST_COMMERCE",
				iTradeFood, kFood.getChar(), L"TXT_KEY_HEADING_TRADEROUTE_LIST"));
		iBaseRate += iTradeFood;
		bNeedSubtotal = true;
	}
	// Specialists
	int iSpecialistFood = 0;
	for(int i = 0; i < GC.getNumSpecialistInfos(); i++) {
		iSpecialistFood += GET_PLAYER(kCity.getOwner()).specialistYield(
				(SpecialistTypes)i, YIELD_FOOD) *
				(kCity.getSpecialistCount((SpecialistTypes)i) +
				kCity.getFreeSpecialistCount((SpecialistTypes)i));
	}
	if(iSpecialistFood != 0) {
		bSimple = false; // advc.087
		szBuffer.append(NEWLINE);
		szBuffer.append(gDLL->getText("TXT_KEY_MISC_HELP_SPECIALIST_COMMERCE",
				iSpecialistFood, kFood.getChar(), L"TXT_KEY_CONCEPT_SPECIALISTS"));
		iBaseRate += iSpecialistFood;
		bNeedSubtotal = true;
	}
	// Corporations
	int iCorporationFood = kCity.getCorporationYield(YIELD_FOOD);
	if(iCorporationFood != 0) {
		bSimple = false; // advc.087
		szBuffer.append(NEWLINE);
		szBuffer.append(gDLL->getText("TXT_KEY_MISC_HELP_CORPORATION_COMMERCE",
				iCorporationFood, kFood.getChar()));
		iBaseRate += iCorporationFood;
		bNeedSubtotal = true;
	}
	// Buildings
	int iBuildingFood = 0;
	for(int i = 0; i < GC.getNumBuildingInfos(); i++) {
		int iCount = kCity.getNumActiveBuilding((BuildingTypes)i);
		if(iCount > 0) {
			CvBuildingInfo& kBuilding = GC.getBuildingInfo((BuildingTypes)i);
			iBuildingFood += iCount * (kBuilding.getYieldChange(YIELD_FOOD) +
					kCity.getBuildingYieldChange((BuildingClassTypes)kBuilding.
					getBuildingClassType(), YIELD_FOOD));
		}
	}
	if(iBuildingFood != 0) {
		bSimple = false; // advc.087
		szBuffer.append(NEWLINE);
		szBuffer.append(gDLL->getText("TXT_KEY_MISC_HELP_BUILDING_COMMERCE",
				iBuildingFood, kFood.getChar()));
		iBaseRate += iBuildingFood;
		bNeedSubtotal = true;
	}
	// Base and modifiers (only if there are modifiers since total is always shown)
	if(kCity.getBaseYieldRateModifier(YIELD_FOOD) != 100) {
		bSimple = false; // advc.087
		szBuffer.append(SEPARATOR);
		szBuffer.append(NEWLINE);
		setYieldHelp(szBuffer, kCity, YIELD_FOOD); // shows Base Food and lists all modifiers
	}
	else szBuffer.append(NEWLINE);
	// Total Produced
	int iBaseModifier = kCity.getBaseYieldRateModifier(YIELD_FOOD);
	int iRate = iBaseRate;
	if(iBaseModifier != 100) {
		bSimple = false; // advc.087
		iRate *= iBaseModifier;
		iRate /= 100;
	}
	szBuffer.append(gDLL->getText("TXT_KEY_MISC_HELP_TOTAL_FOOD_PRODUCED", iRate));
	// ==========================
	szBuffer.append(DOUBLE_SEPARATOR);

	int iFoodConsumed = 0; // Eaten
	int iEatenFood = kCity.getPopulation() * GC.getFOOD_CONSUMPTION_PER_POPULATION();
	if(iEatenFood != 0) {
		szBuffer.append(NEWLINE);
		szBuffer.append(gDLL->getText("TXT_KEY_MISC_HELP_EATEN_FOOD", iEatenFood));
		iFoodConsumed += iEatenFood;
	}
	// Health
	int iSpoiledFood = - kCity.healthRate();
	if (iSpoiledFood != 0) {
		bSimple = false; // advc.087
		szBuffer.append(NEWLINE);
		szBuffer.append(gDLL->getText("TXT_KEY_MISC_HELP_SPOILED_FOOD", iSpoiledFood));
		iFoodConsumed += iSpoiledFood;
	}
	// Total Consumed
	szBuffer.append(NEWLINE);
	szBuffer.append(gDLL->getText("TXT_KEY_MISC_HELP_TOTAL_FOOD_CONSUMED", iFoodConsumed));
	// ==========================
	szBuffer.append(DOUBLE_SEPARATOR NEWLINE);
	iRate -= iFoodConsumed;
	// Production
	if(kCity.isFoodProduction() && iRate > 0) {
		//bSimple = false; // advc.087
		szBuffer.append(gDLL->getText("TXT_KEY_MISC_HELP_NET_FOOD_PRODUCTION",
				iRate, kCity.getProductionNameKey()));
	}
	else {
		// cannot starve a size 1 city with no food in
		if(iRate < 0 && kCity.getPopulation() == 1 && kCity.getFood() == 0) {
			bSimple = false; // advc.087
			iRate = 0;
		} // <advc.087>
		if(bSimple) { // Overwrite all the text appended so far
			szBuffer.assign(gDLL->getText("TXT_KEY_MISC_HELP_WORKED_TILES_EATEN",
					iTileFood, iEatenFood));
			szBuffer.append(SEPARATOR);
			szBuffer.append(NEWLINE);
		} // </advc.087>
		// Net Food
		if(iRate > 0)
			szBuffer.append(gDLL->getText("TXT_KEY_MISC_HELP_NET_FOOD_GROW", iRate));
		else if (iRate < 0)
			szBuffer.append(gDLL->getText("TXT_KEY_MISC_HELP_NET_FOOD_SHRINK", iRate));
		else szBuffer.append(gDLL->getText("TXT_KEY_MISC_HELP_NET_FOOD_STAGNATE"));
	}
	// BULL - Building Additional Food - start
	if(kCity.getOwner() == GC.getGame().getActivePlayer() &&
			(getBugOptionBOOL("MiscHover__BuildingAdditionalFood", false) ||
			GC.altKey())) {
		// ==========================
		setBuildingAdditionalYieldHelp(szBuffer, kCity, YIELD_FOOD, DOUBLE_SEPARATOR);
	} // BULL - Building Additional Food - end
} // BULL - Food Rate Hover - end

void CvGameTextMgr::getGlobeLayerName(GlobeLayerTypes eType, int iOption, CvWString& strName)
{
	switch (eType)
	{
	case GLOBE_LAYER_STRATEGY:
		switch(iOption)
		{
		case 0:
			strName = gDLL->getText("TXT_KEY_GLOBELAYER_STRATEGY_VIEW");
			break;
		case 1:
			strName = gDLL->getText("TXT_KEY_GLOBELAYER_STRATEGY_NEW_LINE");
			break;
		case 2:
			strName = gDLL->getText("TXT_KEY_GLOBELAYER_STRATEGY_NEW_SIGN");
			break;
		case 3:
			strName = gDLL->getText("TXT_KEY_GLOBELAYER_STRATEGY_DELETE");
			break;
		case 4:
			strName = gDLL->getText("TXT_KEY_GLOBELAYER_STRATEGY_DELETE_LINES");
			break;
		}
		break;
	case GLOBE_LAYER_UNIT:
		switch(iOption)
		{
		case SHOW_ALL_MILITARY:
			strName = gDLL->getText("TXT_KEY_GLOBELAYER_UNITS_ALLMILITARY");
			break;
		case SHOW_TEAM_MILITARY:
			strName = gDLL->getText("TXT_KEY_GLOBELAYER_UNITS_TEAMMILITARY");
			break;
		case SHOW_ENEMIES_IN_TERRITORY:
			strName = gDLL->getText("TXT_KEY_GLOBELAYER_UNITS_ENEMY_TERRITORY_MILITARY");
			break;
		case SHOW_ENEMIES:
			strName = gDLL->getText("TXT_KEY_GLOBELAYER_UNITS_ENEMYMILITARY");
			break;
		case SHOW_PLAYER_DOMESTICS:
			strName = gDLL->getText("TXT_KEY_GLOBELAYER_UNITS_DOMESTICS");
			break;
		}
		break;
	case GLOBE_LAYER_RESOURCE:
		switch(iOption)
		{
		case SHOW_ALL_RESOURCES:
			strName = gDLL->getText("TXT_KEY_GLOBELAYER_RESOURCES_EVERYTHING");
			break;
		case SHOW_STRATEGIC_RESOURCES:
			strName = gDLL->getText("TXT_KEY_GLOBELAYER_RESOURCES_GENERAL");
			break;
		case SHOW_HAPPY_RESOURCES:
			strName = gDLL->getText("TXT_KEY_GLOBELAYER_RESOURCES_LUXURIES");
			break;
		case SHOW_HEALTH_RESOURCES:
			strName = gDLL->getText("TXT_KEY_GLOBELAYER_RESOURCES_FOOD");
			break;
		}
		break;
	case GLOBE_LAYER_RELIGION:
		strName = GC.getReligionInfo((ReligionTypes) iOption).getDescription();
		break;
	case GLOBE_LAYER_CULTURE:
	case GLOBE_LAYER_TRADE:
		// these have no sub-options
		strName.clear();
		break;
	}
}

void CvGameTextMgr::getPlotHelp(CvPlot* pMouseOverPlot, CvCity* pCity, CvPlot* pFlagPlot, bool bAlt, CvWStringBuffer& strHelp)
{
	if (gDLL->getInterfaceIFace()->isCityScreenUp())
	{
		if (pMouseOverPlot != NULL)
		{
			CvCity* pHeadSelectedCity = gDLL->getInterfaceIFace()->getHeadSelectedCity();
			if (pHeadSelectedCity != NULL)
			{
				if (pMouseOverPlot->getWorkingCity() == pHeadSelectedCity)
				{
					if (pMouseOverPlot->isRevealed(GC.getGame().getActiveTeam(), true))
					{
						setPlotHelp(strHelp, pMouseOverPlot);
					}
				}
			}
		}
	}
	else
	{
		if (pCity != NULL)
		{
			setCityBarHelp(strHelp, pCity);
		}
		else if (pFlagPlot != NULL)
		{
			setPlotListHelp(strHelp, pFlagPlot, false, true);
		}

		if (strHelp.isEmpty())
		{
			if (pMouseOverPlot != NULL)
			{
				//if ((pMouseOverPlot == gDLL->getInterfaceIFace()->getGotoPlot()) || bAlt)
				if (pMouseOverPlot == gDLL->getInterfaceIFace()->getGotoPlot() ||
						(bAlt && //gDLL->getChtLvl() == 0)) // K-Mod. (alt does something else in cheat mode)
						!GC.getGame().isDebugMode())) // advc.135c
				{
					if (pMouseOverPlot->isActiveVisible(true))
					{
						setCombatPlotHelp(strHelp, pMouseOverPlot);
					}
				}
			}
		}

		if (strHelp.isEmpty())
		{
			if (pMouseOverPlot != NULL)
			{
				if (pMouseOverPlot->isRevealed(GC.getGame().getActiveTeam(), true))
				{
					if (pMouseOverPlot->isActiveVisible(true))
					{
						setPlotListHelp(strHelp, pMouseOverPlot, true, false);

						if (!strHelp.isEmpty())
						{
							strHelp.append(L"\n");
						}
					}

					setPlotHelp(strHelp, pMouseOverPlot);
				}
			}
		}

		InterfaceModeTypes eInterfaceMode = gDLL->getInterfaceIFace()->getInterfaceMode();
		if (eInterfaceMode != INTERFACEMODE_SELECTION)
		{
			CvWString szTempBuffer;
			szTempBuffer.Format(SETCOLR L"%s" ENDCOLR NEWLINE, TEXT_COLOR("COLOR_HIGHLIGHT_TEXT"), GC.getInterfaceModeInfo(eInterfaceMode).getDescription());

			switch (eInterfaceMode)
			{
			case INTERFACEMODE_REBASE:
				getRebasePlotHelp(pMouseOverPlot, szTempBuffer);
				break;

			case INTERFACEMODE_NUKE:
				getNukePlotHelp(pMouseOverPlot, szTempBuffer);
				break;

			default:
				break;
			}

			szTempBuffer += strHelp.getCString();
			strHelp.assign(szTempBuffer);
		}
	}
}

void CvGameTextMgr::getRebasePlotHelp(CvPlot* pPlot, CvWString& strHelp)
{
	if (NULL != pPlot)
	{
		CvUnit* pHeadSelectedUnit = gDLL->getInterfaceIFace()->getHeadSelectedUnit();
		if (NULL != pHeadSelectedUnit)
		{
			if (pPlot->isFriendlyCity(*pHeadSelectedUnit, true))
			{
				CvCity* pCity = pPlot->getPlotCity();
				if (NULL != pCity)
				{
					int iNumUnits = pCity->plot()->countNumAirUnits(GC.getGame().getActiveTeam());
					bool bFull = (iNumUnits >= pCity->getAirUnitCapacity(GC.getGame().getActiveTeam()));

					if (bFull)
					{
						strHelp += CvWString::format(SETCOLR, TEXT_COLOR("COLOR_WARNING_TEXT"));
					}

					strHelp +=  NEWLINE + gDLL->getText("TXT_KEY_CITY_BAR_AIR_UNIT_CAPACITY", iNumUnits, pCity->getAirUnitCapacity(GC.getGame().getActiveTeam()));

					if (bFull)
					{
						strHelp += ENDCOLR;
					}

					strHelp += NEWLINE;
				}
			}
		}
	}
}

void CvGameTextMgr::getNukePlotHelp(CvPlot* pPlot, CvWString& strHelp)
{
	if(pPlot == NULL)
		return;

	CvUnit* pHeadSelectedUnit = gDLL->getInterfaceIFace()->getHeadSelectedUnit();
	if(pHeadSelectedUnit == NULL)
		return;

	for(int iI = 0; iI < MAX_TEAMS; iI++) {
		TeamTypes eVictimTeam = (TeamTypes)iI;
		if(pHeadSelectedUnit->isNukeVictim(pPlot, eVictimTeam) &&
				!pHeadSelectedUnit->isEnemy(eVictimTeam)) {
			// advc.130q: No newline
			strHelp += gDLL->getText("TXT_KEY_CANT_NUKE_FRIENDS");
			break;
		}
	}
	strHelp += NEWLINE; // advc.130q: Newline added
}

void CvGameTextMgr::getInterfaceCenterText(CvWString& strText)
{
	strText.clear();
	if (!gDLL->getInterfaceIFace()->isCityScreenUp())
	{
		if (GC.getGame().getWinner() != NO_TEAM)
		{
			strText = gDLL->getText("TXT_KEY_MISC_WINS_VICTORY", GET_TEAM(GC.getGame().getWinner()).getName().GetCString(), GC.getVictoryInfo(GC.getGame().getVictory()).getTextKeyWide());
		}
		else if (!(GET_PLAYER(GC.getGame().getActivePlayer()).isAlive()))
		{
			strText = gDLL->getText("TXT_KEY_MISC_DEFEAT");
		}
	}
}

void CvGameTextMgr::getTurnTimerText(CvWString& strText)
{
	strText.clear();
	CvGame const& g = GC.getGame(); // advc.003
	if (gDLL->getInterfaceIFace()->getShowInterface() == INTERFACE_SHOW || gDLL->getInterfaceIFace()->getShowInterface() == INTERFACE_ADVANCED_START)
	{
		if (g.isMPOption(MPOPTION_TURN_TIMER))
		{
			// Get number of turn slices remaining until end-of-turn
			int iTurnSlicesRemaining = g.getTurnSlicesRemaining();

			if (iTurnSlicesRemaining > 0)
			{
				// Get number of seconds remaining
				int iTurnSecondsRemaining = ((int)floorf((float)(iTurnSlicesRemaining-1) * ((float)gDLL->getMillisecsPerTurn()/1000.0f)) + 1);
				int iTurnMinutesRemaining = (int)(iTurnSecondsRemaining/60);
				iTurnSecondsRemaining = (iTurnSecondsRemaining%60);
				int iTurnHoursRemaining = (int)(iTurnMinutesRemaining/60);
				iTurnMinutesRemaining = (iTurnMinutesRemaining%60);

				// Display time remaining
				CvWString szTempBuffer;
				szTempBuffer.Format(L"%d:%02d:%02d", iTurnHoursRemaining, iTurnMinutesRemaining, iTurnSecondsRemaining);
				strText += szTempBuffer;
			}
			else
			{
				// Flash zeroes
				if (iTurnSlicesRemaining % 2 == 0)
				{
					// Display 0
					strText+=L"0:00";
				}
			}
		}
		if (g.getGameState() == GAMESTATE_ON)
		{	// <advc.700> Top priority for Auto Play timer
			if(g.isOption(GAMEOPTION_RISE_FALL)) {
				int autoPlayCountdown = g.getRiseFall().getAutoPlayCountdown();
				if(autoPlayCountdown > 0) {
					strText = gDLL->getText("TXT_KEY_RF_INTERLUDE_COUNTDOWN",
							autoPlayCountdown);
					return;
				}
			} // </advc.700>
			int iMinVictoryTurns = MAX_INT;
			for (int i = 0; i < GC.getNumVictoryInfos(); ++i)
			{
				TeamTypes eActiveTeam = g.getActiveTeam();
				if (NO_TEAM != eActiveTeam)
				{
					int iCountdown = GET_TEAM(eActiveTeam).getVictoryCountdown((VictoryTypes)i);
					if (iCountdown > 0 && iCountdown < iMinVictoryTurns)
					{
						iMinVictoryTurns = iCountdown;
					}
				}
			}

			if (g.isOption(GAMEOPTION_ADVANCED_START) && !g.isOption(GAMEOPTION_ALWAYS_WAR) && g.getElapsedGameTurns() <= GC.getPEACE_TREATY_LENGTH()
					/*  advc.250b: No need to (constantly) remind human of
						"universal" peace when the AI civs have big headstarts. */
					&& !g.isOption(GAMEOPTION_SPAH))
			{
				if(!strText.empty())
					strText += L" -- ";
				strText += gDLL->getText("TXT_KEY_MISC_ADVANCED_START_PEACE_REMAINING", GC.getPEACE_TREATY_LENGTH() - g.getElapsedGameTurns());
			}
			else if (iMinVictoryTurns < MAX_INT)
			{
				if (!strText.empty())
					strText += L" -- ";
				strText += gDLL->getText("TXT_KEY_MISC_TURNS_LEFT_TO_VICTORY", iMinVictoryTurns);
			} /* <advc.003> Merged these two conditions so that more else-if
				clauses can be added below */
			else if (g.getMaxTurns() > 0 && (g.getElapsedGameTurns() >= g.getMaxTurns() -
					30 // advc.004: was 100
					&& g.getElapsedGameTurns() < g.getMaxTurns())) { // </advc.003>
				if(!strText.empty())
					strText += L" -- ";
				strText += gDLL->getText("TXT_KEY_MISC_TURNS_LEFT", g.getMaxTurns() - g.getElapsedGameTurns());
			} // <advc.700> The other countdowns take precedence
			else if(g.isOption(GAMEOPTION_RISE_FALL)) {
				std::pair<int,int> rfCountdown = g.getRiseFall().
						getChapterCountdown();
				/*  Only show it toward the end of a chapter. A permanent countdown
					would have to be placed elsewhere b/c the font is too large and
					its size appears to be set outside the SDK. */
				if(rfCountdown.second >= 0 && rfCountdown.second - rfCountdown.first < 3)
					strText = gDLL->getText("TXT_KEY_RF_CHAPTER_COUNTDOWN",
							rfCountdown.first, rfCountdown.second);
			} // </advc.700>
		}
	}
}


void CvGameTextMgr::getFontSymbols(std::vector< std::vector<wchar> >& aacSymbols, std::vector<int>& aiMaxNumRows)
{
	aacSymbols.push_back(std::vector<wchar>());
	aiMaxNumRows.push_back(1);
	for (int iI = 0; iI < NUM_YIELD_TYPES; iI++)
	{
		aacSymbols[aacSymbols.size() - 1].push_back((wchar) GC.getYieldInfo((YieldTypes) iI).getChar());
	}

	aacSymbols.push_back(std::vector<wchar>());
	aiMaxNumRows.push_back(2);
	for (int iI = 0; iI < NUM_COMMERCE_TYPES; iI++)
	{
		aacSymbols[aacSymbols.size() - 1].push_back((wchar) GC.getCommerceInfo((CommerceTypes) iI).getChar());
	}

	aacSymbols.push_back(std::vector<wchar>());
	aiMaxNumRows.push_back(2);
	for (int iI = 0; iI < GC.getNumReligionInfos(); iI++)
	{
		aacSymbols[aacSymbols.size() - 1].push_back((wchar) GC.getReligionInfo((ReligionTypes) iI).getChar());
		aacSymbols[aacSymbols.size() - 1].push_back((wchar) GC.getReligionInfo((ReligionTypes) iI).getHolyCityChar());
	}
	for (int iI = 0; iI < GC.getNumCorporationInfos(); iI++)
	{
		aacSymbols[aacSymbols.size() - 1].push_back((wchar) GC.getCorporationInfo((CorporationTypes) iI).getChar());
		aacSymbols[aacSymbols.size() - 1].push_back((wchar) GC.getCorporationInfo((CorporationTypes) iI).getHeadquarterChar());
	}

	aacSymbols.push_back(std::vector<wchar>());
	aiMaxNumRows.push_back(3);
	for (int iI = 0; iI < GC.getNumBonusInfos(); iI++)
	{
		aacSymbols[aacSymbols.size() - 1].push_back((wchar) GC.getBonusInfo((BonusTypes) iI).getChar());
	}

	aacSymbols.push_back(std::vector<wchar>());
	aiMaxNumRows.push_back(3);
	for (int iI = 0; iI < MAX_NUM_SYMBOLS; iI++)
	{
		aacSymbols[aacSymbols.size() - 1].push_back((wchar) gDLL->getSymbolID(iI));
	}
}

void CvGameTextMgr::assignFontIds(int iFirstSymbolCode, int iPadAmount)
{
	int iCurSymbolID = iFirstSymbolCode;

	// set yield symbols
	for (int i = 0; i < NUM_YIELD_TYPES; i++)
	{
		GC.getYieldInfo((YieldTypes) i).setChar(iCurSymbolID);
		++iCurSymbolID;
	}

	do
	{
		++iCurSymbolID;
	} while (iCurSymbolID % iPadAmount != 0);

	// set commerce symbols
	for (int i=0;i<GC.getNUM_COMMERCE_TYPES();i++)
	{
		GC.getCommerceInfo((CommerceTypes) i).setChar(iCurSymbolID);
		++iCurSymbolID;
	}

	do
	{
		++iCurSymbolID;
	} while (iCurSymbolID % iPadAmount != 0);

	if (NUM_COMMERCE_TYPES < iPadAmount)
	{
		do
		{
			++iCurSymbolID;
		} while (iCurSymbolID % iPadAmount != 0);
	}

	for (int i = 0; i < GC.getNumReligionInfos(); i++)
	{
		GC.getReligionInfo((ReligionTypes) i).setChar(iCurSymbolID);
		++iCurSymbolID;
		GC.getReligionInfo((ReligionTypes) i).setHolyCityChar(iCurSymbolID);
		++iCurSymbolID;
	}
	for (int i = 0; i < GC.getNumCorporationInfos(); i++)
	{
		GC.getCorporationInfo((CorporationTypes) i).setChar(iCurSymbolID);
		++iCurSymbolID;
		GC.getCorporationInfo((CorporationTypes) i).setHeadquarterChar(iCurSymbolID);
		++iCurSymbolID;
	}

	do
	{
		++iCurSymbolID;
	} while (iCurSymbolID % iPadAmount != 0);

	if (2 * (GC.getNumReligionInfos() + GC.getNumCorporationInfos()) < iPadAmount)
	{
		do
		{
			++iCurSymbolID;
		} while (iCurSymbolID % iPadAmount != 0);
	}

	// set bonus symbols
	int bonusBaseID = iCurSymbolID;

	/*  UNOFFICIAL_PATCH, Bugfix (GameFontFix), 06/02/10, LunarMongoose
		this erroneous extra increment command was breaking GameFont.tga files when using exactly 49 or 74 resource types in a mod */
	//++iCurSymbolID;
	for (int i = 0; i < GC.getNumBonusInfos(); i++)
	{
		int bonusID = bonusBaseID + GC.getBonusInfo((BonusTypes) i).getArtInfo()->getFontButtonIndex();
		GC.getBonusInfo((BonusTypes) i).setChar(bonusID);
		++iCurSymbolID;
	}

	do
	{
		++iCurSymbolID;
	} while (iCurSymbolID % iPadAmount != 0);

	if(GC.getNumBonusInfos() < iPadAmount)
	{
		do
		{
			++iCurSymbolID;
		} while (iCurSymbolID % iPadAmount != 0);
	}

	if(GC.getNumBonusInfos() < 2 * iPadAmount)
	{
		do
		{
			++iCurSymbolID;
		} while (iCurSymbolID % iPadAmount != 0);
	}

	// set extra symbols
	for (int i=0; i < MAX_NUM_SYMBOLS; i++)
	{
		gDLL->setSymbolID(i, iCurSymbolID);
		++iCurSymbolID;
	}
}

void CvGameTextMgr::getCityDataForAS(std::vector<CvWBData>& mapCityList, std::vector<CvWBData>& mapBuildingList, std::vector<CvWBData>& mapAutomateList)
{
	CvPlayer& kActivePlayer = GET_PLAYER(GC.getGame().getActivePlayer());

	CvWString szHelp;
	int iCost = kActivePlayer.getAdvancedStartCityCost(true);
	if (iCost > 0)
	{
		szHelp = gDLL->getText("TXT_KEY_CITY");
		szHelp += gDLL->getText("TXT_KEY_AS_UNREMOVABLE");
		mapCityList.push_back(CvWBData(0, szHelp, ARTFILEMGR.getInterfaceArtInfo("INTERFACE_BUTTONS_CITYSELECTION")->getPath()));
	}

	iCost = kActivePlayer.getAdvancedStartPopCost(true);
	if (iCost > 0)
	{
		szHelp = gDLL->getText("TXT_KEY_WB_AS_POPULATION");
		mapCityList.push_back(CvWBData(1, szHelp, ARTFILEMGR.getInterfaceArtInfo("INTERFACE_ANGRYCITIZEN_TEXTURE")->getPath()));
	}

	iCost = kActivePlayer.getAdvancedStartCultureCost(true);
	if (iCost > 0)
	{
		szHelp = gDLL->getText("TXT_KEY_ADVISOR_CULTURE");
		szHelp += gDLL->getText("TXT_KEY_AS_UNREMOVABLE");
		mapCityList.push_back(CvWBData(2, szHelp, ARTFILEMGR.getInterfaceArtInfo("CULTURE_BUTTON")->getPath()));
	}

	CvWStringBuffer szBuffer;
	for(int i = 0; i < GC.getNumBuildingClassInfos(); i++)
	{
		BuildingTypes eBuilding = (BuildingTypes) GC.getCivilizationInfo(kActivePlayer.getCivilizationType()).getCivilizationBuildings(i);

		if (eBuilding != NO_BUILDING)
		{
			if(!GC.getGame().isFreeStartEraBuilding(eBuilding)) // advc.003
			{
				// Building cost -1 denotes unit which may not be purchased
				iCost = kActivePlayer.getAdvancedStartBuildingCost(eBuilding, true);
				if (iCost > 0)
				{
					szBuffer.clear();
					setBuildingHelp(szBuffer, eBuilding);
					mapBuildingList.push_back(CvWBData(eBuilding, szBuffer.getCString(), GC.getBuildingInfo(eBuilding).getButton()));
				}
			}
		}
	}

	szHelp = gDLL->getText("TXT_KEY_ACTION_AUTOMATE_BUILD");
	mapAutomateList.push_back(CvWBData(0, szHelp, ARTFILEMGR.getInterfaceArtInfo("INTERFACE_AUTOMATE")->getPath()));
}

void CvGameTextMgr::getUnitDataForAS(std::vector<CvWBData>& mapUnitList)
{
	CvPlayer& kActivePlayer = GET_PLAYER(GC.getGame().getActivePlayer());

	CvWStringBuffer szBuffer;
	for(int i = 0; i < GC.getNumUnitClassInfos(); i++)
	{
		UnitTypes eUnit = (UnitTypes) GC.getCivilizationInfo(kActivePlayer.getCivilizationType()).getCivilizationUnits(i);
		if (eUnit != NO_UNIT)
		{
			// Unit cost -1 denotes unit which may not be purchased
			int iCost = kActivePlayer.getAdvancedStartUnitCost(eUnit, true);
			if (iCost > 0)
			{
				szBuffer.clear();
				setUnitHelp(szBuffer, eUnit);

				int iMaxUnitsPerCity = GC.getDefineINT("ADVANCED_START_MAX_UNITS_PER_CITY");
				if (iMaxUnitsPerCity >= 0 && GC.getUnitInfo(eUnit).isMilitarySupport())
				{
					szBuffer.append(NEWLINE);
					szBuffer.append(gDLL->getText("TXT_KEY_WB_AS_MAX_UNITS_PER_CITY", iMaxUnitsPerCity));
				}
				mapUnitList.push_back(CvWBData(eUnit, szBuffer.getCString(), kActivePlayer.getUnitButton(eUnit)));
			}
		}
	}
}

void CvGameTextMgr::getImprovementDataForAS(std::vector<CvWBData>& mapImprovementList, std::vector<CvWBData>& mapRouteList)
{
	CvPlayer& kActivePlayer = GET_PLAYER(GC.getGame().getActivePlayer());

	for (int i = 0; i < GC.getNumRouteInfos(); i++)
	{
		RouteTypes eRoute = (RouteTypes) i;
		if (eRoute != NO_ROUTE)
		{
			// Route cost -1 denotes route which may not be purchased
			int iCost = kActivePlayer.getAdvancedStartRouteCost(eRoute, true);
			if (iCost > 0)
			{
				mapRouteList.push_back(CvWBData(eRoute, GC.getRouteInfo(eRoute).getDescription(), GC.getRouteInfo(eRoute).getButton()));
			}
		}
	}

	CvWStringBuffer szBuffer;
	for (int i = 0; i < GC.getNumImprovementInfos(); i++)
	{
		ImprovementTypes eImprovement = (ImprovementTypes) i;
		if (eImprovement != NO_IMPROVEMENT)
		{
			// Improvement cost -1 denotes Improvement which may not be purchased
			int iCost = kActivePlayer.getAdvancedStartImprovementCost(eImprovement, true);
			if (iCost > 0)
			{
				szBuffer.clear();
				setImprovementHelp(szBuffer, eImprovement);
				mapImprovementList.push_back(CvWBData(eImprovement, szBuffer.getCString(), GC.getImprovementInfo(eImprovement).getButton()));
			}
		}
	}
}

void CvGameTextMgr::getVisibilityDataForAS(std::vector<CvWBData>& mapVisibilityList)
{
	// Unit cost -1 denotes unit which may not be purchased
	int iCost = GET_PLAYER(GC.getGame().getActivePlayer()).getAdvancedStartVisibilityCost(true);
	if (iCost > 0)
	{
		CvWString szHelp = gDLL->getText("TXT_KEY_WB_AS_VISIBILITY");
		szHelp += gDLL->getText("TXT_KEY_AS_UNREMOVABLE", iCost);
		mapVisibilityList.push_back(CvWBData(0, szHelp, ARTFILEMGR.getInterfaceArtInfo("INTERFACE_TECH_LOS")->getPath()));
	}
}

void CvGameTextMgr::getTechDataForAS(std::vector<CvWBData>& mapTechList)
{
	mapTechList.push_back(CvWBData(0, gDLL->getText("TXT_KEY_WB_AS_TECH"), ARTFILEMGR.getInterfaceArtInfo("INTERFACE_BTN_TECH")->getPath()));
}

void CvGameTextMgr::getUnitDataForWB(std::vector<CvWBData>& mapUnitData)
{
	CvWStringBuffer szBuffer;
	for (int i = 0; i < GC.getNumUnitInfos(); i++)
	{
		szBuffer.clear();
		setUnitHelp(szBuffer, (UnitTypes)i);
		mapUnitData.push_back(CvWBData(i, szBuffer.getCString(), GC.getUnitInfo((UnitTypes)i).getButton()));
	}
}

void CvGameTextMgr::getBuildingDataForWB(bool bStickyButton, std::vector<CvWBData>& mapBuildingData)
{
	int iCount = 0;
	if (!bStickyButton)
	{
		mapBuildingData.push_back(CvWBData(iCount++, GC.getMissionInfo(MISSION_FOUND).getDescription(), GC.getMissionInfo(MISSION_FOUND).getButton()));
	}

	CvWStringBuffer szBuffer;
	for (int i=0; i < GC.getNumBuildingInfos(); i++)
	{
		szBuffer.clear();
		setBuildingHelp(szBuffer, (BuildingTypes)i);
		mapBuildingData.push_back(CvWBData(iCount++, szBuffer.getCString(), GC.getBuildingInfo((BuildingTypes)i).getButton()));
	}
}

void CvGameTextMgr::getTerrainDataForWB(std::vector<CvWBData>& mapTerrainData, std::vector<CvWBData>& mapFeatureData, std::vector<CvWBData>& mapPlotData, std::vector<CvWBData>& mapRouteData)
{
	CvWStringBuffer szBuffer;

	for (int i = 0; i < GC.getNumTerrainInfos(); i++)
	{
		if (!GC.getTerrainInfo((TerrainTypes)i).isGraphicalOnly())
		{
			szBuffer.clear();
			setTerrainHelp(szBuffer, (TerrainTypes)i);
			mapTerrainData.push_back(CvWBData(i, szBuffer.getCString(), GC.getTerrainInfo((TerrainTypes)i).getButton()));
		}
	}

	for (int i = 0; i < GC.getNumFeatureInfos(); i++)
	{
		for (int k = 0; k < GC.getFeatureInfo((FeatureTypes)i).getArtInfo()->getNumVarieties(); k++)
		{
			szBuffer.clear();
			setFeatureHelp(szBuffer, (FeatureTypes)i);
			mapFeatureData.push_back(CvWBData(i + GC.getNumFeatureInfos() * k, szBuffer.getCString(), GC.getFeatureInfo((FeatureTypes)i).getArtInfo()->getVariety(k).getVarietyButton()));
		}
	}

	mapPlotData.push_back(CvWBData(0, gDLL->getText("TXT_KEY_WB_PLOT_TYPE_MOUNTAIN"), ARTFILEMGR.getInterfaceArtInfo("WORLDBUILDER_PLOT_TYPE_MOUNTAIN")->getPath()));
	mapPlotData.push_back(CvWBData(1, gDLL->getText("TXT_KEY_WB_PLOT_TYPE_HILL"), ARTFILEMGR.getInterfaceArtInfo("WORLDBUILDER_PLOT_TYPE_HILL")->getPath()));
	mapPlotData.push_back(CvWBData(2, gDLL->getText("TXT_KEY_WB_PLOT_TYPE_PLAINS"), ARTFILEMGR.getInterfaceArtInfo("WORLDBUILDER_PLOT_TYPE_PLAINS")->getPath()));
	mapPlotData.push_back(CvWBData(3, gDLL->getText("TXT_KEY_WB_PLOT_TYPE_OCEAN"), ARTFILEMGR.getInterfaceArtInfo("WORLDBUILDER_PLOT_TYPE_OCEAN")->getPath()));

	for (int i = 0; i < GC.getNumRouteInfos(); i++)
	{
		mapRouteData.push_back(CvWBData(i, GC.getRouteInfo((RouteTypes)i).getDescription(), GC.getRouteInfo((RouteTypes)i).getButton()));
	}
	mapRouteData.push_back(CvWBData(GC.getNumRouteInfos(), gDLL->getText("TXT_KEY_WB_RIVER_PLACEMENT"), ARTFILEMGR.getInterfaceArtInfo("WORLDBUILDER_RIVER_PLACEMENT")->getPath()));
}

void CvGameTextMgr::getTerritoryDataForWB(std::vector<CvWBData>& mapTerritoryData)
{
	for (int i = 0; i <= MAX_CIV_PLAYERS; i++)
	{
		CvWString szName = gDLL->getText("TXT_KEY_MAIN_MENU_NONE");
		CvString szButton = GC.getCivilizationInfo(GET_PLAYER(BARBARIAN_PLAYER).getCivilizationType()).getButton();

		if (GET_PLAYER((PlayerTypes) i).isEverAlive())
		{
			szName = GET_PLAYER((PlayerTypes)i).getName();
			szButton = GC.getCivilizationInfo(GET_PLAYER((PlayerTypes)i).getCivilizationType()).getButton();
		}
		mapTerritoryData.push_back(CvWBData(i, szName, szButton));
	}
}


void CvGameTextMgr::getTechDataForWB(std::vector<CvWBData>& mapTechData)
{
	CvWStringBuffer szBuffer;
	for (int i=0; i < GC.getNumTechInfos(); i++)
	{
		szBuffer.clear();
		setTechHelp(szBuffer, (TechTypes) i);
		mapTechData.push_back(CvWBData(i, szBuffer.getCString(), GC.getTechInfo((TechTypes) i).getButton()));
	}
}

void CvGameTextMgr::getPromotionDataForWB(std::vector<CvWBData>& mapPromotionData)
{
	CvWStringBuffer szBuffer;
	for (int i=0; i < GC.getNumPromotionInfos(); i++)
	{
		szBuffer.clear();
		setPromotionHelp(szBuffer, (PromotionTypes) i, false);
		mapPromotionData.push_back(CvWBData(i, szBuffer.getCString(), GC.getPromotionInfo((PromotionTypes) i).getButton()));
	}
}

void CvGameTextMgr::getBonusDataForWB(std::vector<CvWBData>& mapBonusData)
{
	CvWStringBuffer szBuffer;
	for (int i=0; i < GC.getNumBonusInfos(); i++)
	{
		szBuffer.clear();
		setBonusHelp(szBuffer, (BonusTypes)i);
		mapBonusData.push_back(CvWBData(i, szBuffer.getCString(), GC.getBonusInfo((BonusTypes) i).getButton()));
	}
}

void CvGameTextMgr::getImprovementDataForWB(std::vector<CvWBData>& mapImprovementData)
{
	CvWStringBuffer szBuffer;
	for (int i=0; i < GC.getNumImprovementInfos(); i++)
	{
		CvImprovementInfo& kInfo = GC.getImprovementInfo((ImprovementTypes) i);
		if (!kInfo.isGraphicalOnly())
		{
			szBuffer.clear();
			setImprovementHelp(szBuffer, (ImprovementTypes) i);
			mapImprovementData.push_back(CvWBData(i, szBuffer.getCString(), kInfo.getButton()));
		}
	}
}

void CvGameTextMgr::getReligionDataForWB(bool bHolyCity, std::vector<CvWBData>& mapReligionData)
{
	for (int i = 0; i < GC.getNumReligionInfos(); ++i)
	{
		CvReligionInfo& kInfo = GC.getReligionInfo((ReligionTypes) i);
		CvWString strDescription = kInfo.getDescription();
		if (bHolyCity)
		{
			strDescription = gDLL->getText("TXT_KEY_WB_HOLYCITY", strDescription.GetCString());
		}
		mapReligionData.push_back(CvWBData(i, strDescription, kInfo.getButton()));
	}
}


void CvGameTextMgr::getCorporationDataForWB(bool bHeadquarters, std::vector<CvWBData>& mapCorporationData)
{
	for (int i = 0; i < GC.getNumCorporationInfos(); ++i)
	{
		CvCorporationInfo& kInfo = GC.getCorporationInfo((CorporationTypes) i);
		CvWString strDescription = kInfo.getDescription();
		if (bHeadquarters)
		{
			strDescription = gDLL->getText("TXT_KEY_CORPORATION_HEADQUARTERS", strDescription.GetCString());
		}
		mapCorporationData.push_back(CvWBData(i, strDescription, kInfo.getButton()));
	}
}

// advc.003: Body cut and pasted from setCombatPlotHelp
void CvGameTextMgr::appendNegativeModifiers(CvWStringBuffer& szString,
		CvUnit const* pAttacker, CvUnit const* pDefender, CvPlot const* pPlot) {

	int iModifier = 0;
	if (!(pAttacker->isRiver()))
	{
		if (pAttacker->plot()->isRiverCrossing(directionXY(pAttacker->plot(),
				pPlot)))
		{
			iModifier = GC.getRIVER_ATTACK_MODIFIER();

			if (iModifier != 0)
			{
				szString.append(NEWLINE);
				szString.append(gDLL->getText("TXT_KEY_COMBAT_PLOT_RIVER_MOD",
						-(iModifier)));
			}
		}
	}

	if (!(pAttacker->isAmphib()))
	{
		if (!(pPlot->isWater()) && pAttacker->plot()->isWater())
		{
			iModifier = GC.getAMPHIB_ATTACK_MODIFIER();

			if (iModifier != 0)
			{
				szString.append(NEWLINE);
				szString.append(gDLL->getText("TXT_KEY_COMBAT_PLOT_AMPHIB_MOD",
						-(iModifier)));
			}
		}
	}

	iModifier = pDefender->getExtraCombatPercent();

	if (iModifier != 0)
	{
		szString.append(NEWLINE);
		szString.append(gDLL->getText("TXT_KEY_COMBAT_PLOT_EXTRA_STRENGTH",
				iModifier));
	}

	iModifier = pDefender->unitClassDefenseModifier(pAttacker->getUnitClassType());

	if (iModifier != 0)
	{
		szString.append(NEWLINE);
		szString.append(gDLL->getText("TXT_KEY_COMBAT_PLOT_MOD_VS_TYPE",
				iModifier, GC.getUnitClassInfo(pAttacker->getUnitClassType()).
				getTextKeyWide()));
	}

	if (pAttacker->getUnitCombatType() != NO_UNITCOMBAT)
	{
		iModifier = pDefender->unitCombatModifier(pAttacker->getUnitCombatType());

		if (iModifier != 0)
		{
			szString.append(NEWLINE);
			szString.append(gDLL->getText("TXT_KEY_COMBAT_PLOT_MOD_VS_TYPE",
					iModifier, GC.getUnitCombatInfo(pAttacker->getUnitCombatType()).
					getTextKeyWide()));
		}
	}

	iModifier = pDefender->domainModifier(pAttacker->getDomainType());

	if (iModifier != 0)
	{
		szString.append(NEWLINE);
		szString.append(gDLL->getText("TXT_KEY_COMBAT_PLOT_MOD_VS_TYPE",
				iModifier, GC.getDomainInfo(pAttacker->getDomainType()).
				getTextKeyWide()));
	}

	if (!pDefender->noDefensiveBonus())
	{
		// <advc.012> Show feature defense unless in a hostile tile
		if(pAttacker == NULL)
			iModifier = GET_TEAM(pDefender->getTeam()).
					AI_plotDefense(*pPlot, true);
		else iModifier = pPlot->defenseModifier(pDefender->getTeam(),
				pAttacker->ignoreBuildingDefense(), pAttacker->getTeam());
		// </advc.012>
		if (iModifier != 0)
		{
			szString.append(NEWLINE);
			szString.append(gDLL->getText("TXT_KEY_COMBAT_PLOT_TILE_MOD",
					iModifier));
		}
	}

	iModifier = pDefender->fortifyModifier();

	if (iModifier != 0)
	{
		szString.append(NEWLINE);
		szString.append(gDLL->getText("TXT_KEY_COMBAT_PLOT_FORTIFY_MOD",
				iModifier));
	}

	if (pPlot->isCity(true, pDefender->getTeam()))
	{
		iModifier = pDefender->cityDefenseModifier();

		if (iModifier != 0)
		{
			szString.append(NEWLINE);
			szString.append(gDLL->getText("TXT_KEY_COMBAT_PLOT_CITY_MOD",
					iModifier));
		}
	}

	if (pPlot->isHills())
	{
		iModifier = pDefender->hillsDefenseModifier();

		if (iModifier != 0)
		{
			szString.append(NEWLINE);
			szString.append(gDLL->getText("TXT_KEY_COMBAT_PLOT_HILLS_MOD",
					iModifier));
		}
	}

	if (pPlot->getFeatureType() != NO_FEATURE)
	{
		iModifier = pDefender->featureDefenseModifier(pPlot->getFeatureType());

		if (iModifier != 0)
		{
			szString.append(NEWLINE);
			szString.append(gDLL->getText("TXT_KEY_COMBAT_PLOT_UNIT_MOD",
					iModifier, GC.getFeatureInfo(pPlot->getFeatureType()).
					getTextKeyWide()));
		}
	}
	else
	{
		iModifier = pDefender->terrainDefenseModifier(pPlot->getTerrainType());

		if (iModifier != 0)
		{
			szString.append(NEWLINE);
			szString.append(gDLL->getText("TXT_KEY_COMBAT_PLOT_UNIT_MOD",
					iModifier, GC.getTerrainInfo(pPlot->getTerrainType()).
					getTextKeyWide()));
		}
	}
}

// advc.003: Body cut and pasted from setCombatPlotHelp
void CvGameTextMgr::appendPositiveModifiers(CvWStringBuffer& szString,
		CvUnit const* pAttacker, CvUnit const* pDefender, CvPlot const* pPlot, bool bNegative) {

	int const iSign = (bNegative ? -1 : 1);
	int iModifier = pAttacker->unitClassAttackModifier(pDefender->getUnitClassType());

	if (iModifier != 0)
	{
		szString.append(NEWLINE);
		szString.append(gDLL->getText("TXT_KEY_COMBAT_PLOT_MOD_VS_TYPE",
				iSign * iModifier, GC.getUnitClassInfo(pDefender->getUnitClassType()).
				getTextKeyWide()));
	}

	if (pDefender->getUnitCombatType() != NO_UNITCOMBAT)
	{
		iModifier = pAttacker->unitCombatModifier(pDefender->getUnitCombatType());

		if (iModifier != 0)
		{
			szString.append(NEWLINE);
			szString.append(gDLL->getText("TXT_KEY_COMBAT_PLOT_MOD_VS_TYPE",
					iSign * iModifier, GC.getUnitCombatInfo(
					pDefender->getUnitCombatType()).getTextKeyWide()));
		}
	}

	iModifier = pAttacker->domainModifier(pDefender->getDomainType());

	if (iModifier != 0)
	{
		szString.append(NEWLINE);
		szString.append(gDLL->getText("TXT_KEY_COMBAT_PLOT_MOD_VS_TYPE",
				iSign * iModifier, GC.getDomainInfo(pDefender->getDomainType()).
				getTextKeyWide()));
	}

	if (pPlot->isCity(true, pDefender->getTeam()))
	{
		iModifier = pAttacker->cityAttackModifier();

		if (iModifier != 0)
		{
			szString.append(NEWLINE);
			szString.append(gDLL->getText("TXT_KEY_COMBAT_PLOT_CITY_MOD",
					iSign * iModifier));
		}
	}

	if (pPlot->isHills())
	{
		iModifier = pAttacker->hillsAttackModifier();

		if (iModifier != 0)
		{
			szString.append(NEWLINE);
			szString.append(gDLL->getText("TXT_KEY_COMBAT_PLOT_HILLS_MOD",
					iSign * iModifier));
		}
	}

	if (pPlot->getFeatureType() != NO_FEATURE)
	{
		iModifier = pAttacker->featureAttackModifier(pPlot->getFeatureType());

		if (iModifier != 0)
		{
			szString.append(NEWLINE);
			szString.append(gDLL->getText("TXT_KEY_COMBAT_PLOT_UNIT_MOD",
					iSign * iModifier, GC.getFeatureInfo(pPlot->getFeatureType()).
					getTextKeyWide()));
		}
	}
	else
	{
		iModifier = pAttacker->terrainAttackModifier(pPlot->getTerrainType());

		if (iModifier != 0)
		{
			szString.append(NEWLINE);
			szString.append(gDLL->getText("TXT_KEY_COMBAT_PLOT_UNIT_MOD",
					iSign * iModifier, GC.getTerrainInfo(pPlot->getTerrainType()).
					getTextKeyWide()));
		}
	}

	iModifier = pAttacker->getKamikazePercent();
	if (iModifier != 0)
	{
		szString.append(NEWLINE);
		szString.append(gDLL->getText("TXT_KEY_COMBAT_KAMIKAZE_MOD",
				iSign * iModifier));
	}

	if (pDefender->isAnimal())
	{	// advc.315c: Moved into the isBarbarian block below
		//iModifier = -GC.getHandicapInfo(GC.getGame().getHandicapType()).getAnimalCombatModifier();
		iModifier += pAttacker->getUnitInfo().getAnimalCombatModifier();

		if (iModifier != 0)
		{
			szString.append(NEWLINE);
			szString.append(gDLL->getText("TXT_KEY_UNIT_ANIMAL_COMBAT_MOD",
					iSign * iModifier));
		}
	}

	if (pDefender->isBarbarian())
	{	// advc.315c:
		iModifier = -pAttacker->getUnitInfo().getBarbarianCombatModifier();
		if (iModifier != 0)
		{
			szString.append(NEWLINE);
			szString.append(gDLL->getText("TXT_KEY_UNIT_BARBARIAN_COMBAT_MOD",
					iSign * iModifier));
		}
		// <advc.315c> Show modifier from difficulty separately from unit abilities
		iModifier = GC.getHandicapInfo(
				GET_PLAYER(pAttacker->getOwner()). // K-Mod
				getHandicapType()).getBarbarianCombatModifier();
		// Moved from the isAnimal block above
		if(pDefender->isAnimal()) {
			iModifier += GC.getHandicapInfo(
					GET_PLAYER(pAttacker->getOwner()). // K-Mod
					getHandicapType()).getAnimalCombatModifier();
		}
		if (iModifier != 0)
		{
			szString.append(NEWLINE);
			szString.append(gDLL->getText("TXT_KEY_MISC_FROM_HANDICAP",
					iSign * iModifier));
		} // </advc.315c>
	}
}

/*  <advc.004w> Based on code cut from setUnitHelp and setBasicUnitHelp (and deleted from
	setBuildingHelpActual). Units and buildings have the same production speed bonuses. */
void CvGameTextMgr::setProductionSpeedHelp(CvWStringBuffer& szBuffer,
		/*  Making a concrete base class for CvUnitInfo and CvBuildingInfo would be
			possible but an unreasonable effort */
		OrderTypes eInfoType, CvInfoBase const* pInfo,
		CvCity* pCity, bool bCivilopediaText) {

	FAssert(eInfoType == ORDER_TRAIN || eInfoType == ORDER_CONSTRUCT);
	for(int i = 0; i < GC.getNumBonusInfos(); i++) {
		BonusTypes eBonus = (BonusTypes)i;
		int iProductionModifier = 0;
		if(eInfoType == ORDER_TRAIN) {
			iProductionModifier = static_cast<CvUnitInfo const*>(pInfo)->
					getBonusProductionModifier(eBonus);
		}
		else if(eInfoType == ORDER_CONSTRUCT) {
			iProductionModifier = static_cast<CvBuildingInfo const*>(pInfo)->
					getBonusProductionModifier(eBonus);
		}
		if(iProductionModifier == 0)
			continue;
		if(pCity != NULL) {
			bool bHasBonus = pCity->hasBonus(eBonus);
			if(iProductionModifier > 0 ? bHasBonus : !bHasBonus)
				szBuffer.append(gDLL->getText("TXT_KEY_COLOR_POSITIVE"));
			else szBuffer.append(gDLL->getText("TXT_KEY_COLOR_NEGATIVE"));
		}
		if(!bCivilopediaText)
			szBuffer.append(L" (");
		else {
			szBuffer.append(NEWLINE);
			szBuffer.append(CvWString::format(L"%c", gDLL->getSymbolID(BULLET_CHAR)));
		}
		wchar const* szBonus = GC.getBonusInfo(eBonus).getTextKeyWide();
		if(iProductionModifier == 100) {
			szBuffer.append(gDLL->getText(bCivilopediaText ?
					"TXT_KEY_DOUBLE_PRODUCTION_WITH" :
					"TXT_KEY_DOUBLE_PRODUCTION_WITH_SHORT", szBonus));
		}
		else {
			szBuffer.append(gDLL->getText("TXT_KEY_FASTER_PRODUCTION_WITH",
					iProductionModifier, szBonus));
		}
		if(!bCivilopediaText)
			szBuffer.append(L")");
		if(pCity != NULL)
			szBuffer.append(gDLL->getText("TXT_KEY_COLOR_REVERT"));
	}
	PlayerTypes eActivePlayer = GC.getGame().getActivePlayer();
	if(!bCivilopediaText && eActivePlayer == NO_PLAYER)
		return;
	for(int i = 0; i < GC.getNumTraitInfos(); i++) {
		TraitTypes eTrait = (TraitTypes)i;
		if(!bCivilopediaText) {
			CvLeaderHeadInfo const& lh = GC.getLeaderHeadInfo(
					GET_PLAYER(eActivePlayer).getLeaderType());
			if(!lh.hasTrait(eTrait))
				continue;
		}
		int iProductionModifier = 0;
		if(eInfoType == ORDER_TRAIN) {
			iProductionModifier = static_cast<CvUnitInfo const*>(pInfo)->
					getProductionTraits(eTrait);
		}
		else if(eInfoType == ORDER_CONSTRUCT) {
			iProductionModifier = static_cast<CvBuildingInfo const*>(pInfo)->
					getProductionTraits(eTrait);
		}
		if(iProductionModifier == 0)
			continue;
		if(!bCivilopediaText) {
			if(iProductionModifier > 0)
				szBuffer.append(gDLL->getText("TXT_KEY_COLOR_POSITIVE"));
			else szBuffer.append(gDLL->getText("TXT_KEY_COLOR_NEGATIVE"));
		}
		if(!bCivilopediaText) {
			/*  Not nice when more than one speed modifier applies (e.g. Wall).
				Should put each modifier on a separate line then. Also, "+100%"
				would be better than "double" in that case b/c the modifiers are
				additive. Awkward to implement though. :( */
			szBuffer.append(L" (");
		}
		else {
			szBuffer.append(NEWLINE);
			szBuffer.append(CvWString::format(L"%c", gDLL->getSymbolID(BULLET_CHAR)));
		}
		wchar const* szTrait = GC.getTraitInfo((TraitTypes)i).getTextKeyWide();
		if(iProductionModifier == 100) {
			szBuffer.append(gDLL->getText(bCivilopediaText ?
					"TXT_KEY_DOUBLE_SPEED_TRAIT" :
					"TXT_KEY_DOUBLE_SPEED_TRAIT_SHORT", szTrait));
		}
		else {
			szBuffer.append(gDLL->getText("TXT_KEY_PRODUCTION_MODIFIER_TRAIT",
				iProductionModifier, szTrait));
		}
		if(!bCivilopediaText) {
			szBuffer.append(L")");
			szBuffer.append(gDLL->getText("TXT_KEY_COLOR_REVERT"));
		}
	}
} // </advc.004w>
