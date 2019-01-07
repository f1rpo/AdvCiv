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


bool getBugOptionBOOL(const char* id, bool bDefault)
{
	CyArgsList argsList;
	long lResult = 0;

	argsList.add(id);
	argsList.add(bDefault);

	gDLL->getPythonIFace()->callFunction(PYBugOptionsModule, "getOptionBOOL", argsList.makeFunctionArgs(), &lResult);

	return lResult != 0;
}

int getBugOptionINT(const char* id, int iDefault)
{
	CyArgsList argsList;
	long lResult = 0;

	argsList.add(id);
	argsList.add(iDefault);

	gDLL->getPythonIFace()->callFunction(PYBugOptionsModule, "getOptionINT", argsList.makeFunctionArgs(), &lResult);

	return lResult;
}
