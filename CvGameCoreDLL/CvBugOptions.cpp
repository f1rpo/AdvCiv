/**********************************************************************

File:		CvBugOptions.cpp
Author:		EmperorFool
Created:	2009-01-21

		Copyright (c) 2009 The BUG Mod. All rights reserved.

**********************************************************************/

// This file has been edited for K-Mod

#include "CvGameCoreDLL.h"
#include "CyArgsList.h"
#include "CvDLLPythonIFaceBase.h"
#include "FVariableSystem.h"

#include "CvBugOptions.h"

bool getDefineBOOL(const char* xmlKey, bool bDefault)
{
	int iResult = 0;
	if (GC.getDefinesVarSystem()->GetValue(xmlKey, iResult))
	{
		return iResult != 0;
	}
	else
	{
		return bDefault;
	}
}

int getDefineINT(const char* xmlKey, int iDefault)
{
	int iResult = 0;
	if (GC.getDefinesVarSystem()->GetValue(xmlKey, iResult))
	{
		return iResult;
	}
	else
	{
		return iDefault;
	}
}

// <advc.003>
bool checkBUGStatus(const char* optionKey, bool bWarn) {

	if(!GC.IsGraphicsInitialized() || GC.getGameINLINE().getActivePlayer() == NO_PLAYER) {
		if(!bWarn)
			return false;
		CvString szMsg = "BUG option ";
		szMsg.append(optionKey);
		szMsg.append(" accessed before BUG initialization");
		FAssertMsg(GC.IsGraphicsInitialized(), szMsg.c_str());
		FAssertMsg(GC.getGameINLINE().getActivePlayer() != NO_PLAYER, szMsg.c_str());
		return false;
	}
	return true;
} // </advc.003>


bool getBugOptionBOOL(const char* id, bool bDefault, bool bWarn)
{	// <advc.003>
	PROFILE_FUNC();
	if(!checkBUGStatus(id, bWarn))
		return bDefault; // </advc.003>
	CyArgsList argsList;
	long lResult = 0;

	argsList.add(id);
	argsList.add(bDefault);

	gDLL->getPythonIFace()->callFunction(PYBugOptionsModule, "getOptionBOOL", argsList.makeFunctionArgs(), &lResult);

	return lResult != 0;
}

int getBugOptionINT(const char* id, int iDefault, bool bWarn)
{	// <advc.003>
	PROFILE_FUNC();
	if(!checkBUGStatus(id, bWarn))
		return iDefault; // </advc.003>
	CyArgsList argsList;
	long lResult = 0;

	argsList.add(id);
	argsList.add(iDefault);

	gDLL->getPythonIFace()->callFunction(PYBugOptionsModule, "getOptionINT", argsList.makeFunctionArgs(), &lResult);

	return lResult;
}
