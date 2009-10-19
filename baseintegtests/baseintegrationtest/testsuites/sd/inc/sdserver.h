// Copyright (c) 2007-2009 Nokia Corporation and/or its subsidiary(-ies).
// All rights reserved.
// This component and the accompanying materials are made available
// under the terms of the License "Eclipse Public License v1.0"
// which accompanies this distribution, and is available
// at the URL "http://www.eclipse.org/legal/epl-v10.html".
//
// Initial Contributors:
// Nokia Corporation - initial contribution.
//
// Contributors:
//
// Description:
//

#ifndef SDSERVER_H
#define SDSERVER_H

#include <testexecuteserverbase.h>

/*
SD Test Server class. Contains data that can be shared between several calls
to a same Test Step under one Test Script.
*/
class TBaseTestSDSharedData
	{
	public:
		TBaseTestSDSharedData();

		// These are set by FS1()
		TUint32 iPartitionBootSector;
		TUint32 iTotalSector;
		TUint8 iFsType;
	
		// And these by FS2()
		TUint32 iNumberOfClusters;
		TUint32 iSectorsPerFat;
		TUint32 iReservedSectorCount;
	};

/*
SD Test Server
*/
class CBaseTestSDServer : public CTestServer
	{
public:
	static CBaseTestSDServer* NewL();
	virtual CTestStep* CreateTestStep(const TDesC& aStepName);
	
	TBaseTestSDSharedData iSharedData;
	};

#endif // SDSERVER_H
