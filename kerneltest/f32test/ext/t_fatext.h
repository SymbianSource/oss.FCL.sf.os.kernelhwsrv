// Copyright (c) 2006-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// f32test\ext\t_fatext.h
// 
//

#ifndef __FATTEST_EXT_H__
#define __FATTEST_EXT_H__

#include "t_testext.h"
#include "fat_utils.h"

using namespace Fat_Test_Utils;

/**
This is the FAT test extension class.
@internalTechnology
*/
class CFatTestProxyDrive : public CTestProxyDrive
	{
public:
    enum TFatTestCmd 
        {
        EGetDataPosition = ETestCmdEnd + 1,
        ESectorsPerCluster,
        EFatSectors,
        EFirstFatSector,
        EFatType
        };

public:
	static CFatTestProxyDrive* NewL(CProxyDrive* aProxyDrive, CMountCB* aMount);
private:
	CFatTestProxyDrive(CProxyDrive* aProxyDrive, CMountCB* aMount);

public:
    // Derived functions
	virtual TInt Read(TInt64 aPos,TInt aLength,const TAny* aTrg,TInt aThreadHandle,TInt aOffset,TInt aFlags);
	virtual TInt Read(TInt64 aPos,TInt aLength,const TAny* aTrg,TInt aThreadHandle,TInt anOffset);
	virtual TInt Read(TInt64 aPos,TInt aLength,TDes8& aTrg);
	virtual TInt Write(TInt64 aPos,TInt aLength,const TAny* aSrc,TInt aThreadHandle,TInt aOffset,TInt aFlags);
	virtual TInt Write(TInt64 aPos,TInt aLength,const TAny* aSrc,TInt aThreadHandle,TInt anOffset);
	virtual TInt Write(TInt64 aPos,const TDesC8& aSrc);
	virtual TInt Format(TFormatInfo& anInfo);
	virtual TInt Format(TInt64 aPos,TInt aLength);

private:
    TInt ReadBootSector();
    TBool CheckMount();
private:
    virtual void DoInitL();
    virtual TBool DoCheckEvent(TInt64 aPos, TInt aLength);
    virtual TInt DoControlIO(const RMessagePtr2& aMessage,TInt aCommand,TAny* aParam1,TAny* aParam2);

private:
    TFatBootSector  iBootSector;
	};


/**
Factory class for FAT test extension.
@internalTechnology
*/
class CFatTestProxyDriveFactory : public CProxyDriveFactory
	{
public:
	CFatTestProxyDriveFactory();
	virtual TInt Install();			
	virtual CProxyDrive* NewProxyDriveL(CProxyDrive* aProxy,CMountCB* aMount);
	};

#endif













