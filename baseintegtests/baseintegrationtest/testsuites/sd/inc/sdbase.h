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

#ifndef SDBASE_H
#define SDBASE_H

#include <testexecuteserverbase.h>
#include <testexecutestepbase.h>
#include "d_mmcif.h"

#define SYMBIAN_TEST_LE2(a,b) (((b) << 8) + (a))
#define SYMBIAN_TEST_LE4(a,b,c,d) (((d) << 24) + ((c) << 16) + ((b) << 8) + (a))
#define SYMBIAN_TEST_CEIL(a,b) ((a) % (b) ? 1 + ((a) - (a) % (b)) / (b)  : (a) / (b))
#define SYMBIAN_TEST_IP(a,b) ((a) % (b) ? 1 + ((a) - (a) % (b)) / (b)  : (a) / (b))

/*
Base class for all test steps
*/
class CBaseTestSDBase : public CTestStep
	{
public:
	virtual TVerdict doTestStepPreambleL() = 0;
	virtual TVerdict doTestStepPostambleL();

protected:
// The following four functions can be called in the test preamble.
	TBool InitDriveLetter();	// Initialise attribute iDrive
	TBool InitDeviceDriver();   // Load D_MMCIF device driver for direct disk access
	TBool InitFileServer();     // Initialise attribute iFs
	TBool InitFileMan();        // Initialise attribute iFileMan

	// Direct disk access. InitDeviceDriver() must be called first
	TInt ReadSector(TInt aSector, TDes8& aSectorBuffer);
	TInt WriteSector(TInt aSector, const TDesC8& aSectorBuffer);

	TMmcCardInfo iCardInfo;
	TInt iCardSizeInSectors;
	
	// Call InitFileServer() before using iFs
	RFs iFs;
	
	// Call InitFileMan() before using iFileMan
	CFileMan *iFileMan;

	// The drive number assigned to the SD card. Call InitDriveLetter() first
	TInt iDrive;

private:
	RMmcCntrlIf iDriver;
	};

#endif // SDBASE_H
