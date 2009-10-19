// Copyright (c) 2002-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// f32test\ext\emptyext.cpp
// extension which simply records/prints out accesses to each function
// 
//

#include <f32fsys.h>


class CEmptyExtProxyDrive : public CBaseExtProxyDrive
	{
public:
	static CEmptyExtProxyDrive* NewL(CProxyDrive* aProxyDrive, CMountCB* aMount);
	~CEmptyExtProxyDrive();
public:
	virtual TInt Initialise();
	virtual TInt Dismounted();
	virtual TInt Enlarge(TInt aLength);
	virtual TInt ReduceSize(TInt aPos, TInt aLength);
	virtual TInt Read(TInt64 aPos,TInt aLength,const TAny* aTrg,TInt aThreadHandle,TInt anOffset);
	virtual TInt Read(TInt64 aPos,TInt aLength,TDes8& aTrg);
	virtual TInt Write(TInt64 aPos,TInt aLength,const TAny* aSrc,TInt aThreadHandle,TInt anOffset);
	virtual TInt Write(TInt64 aPos,const TDesC8& aSrc);
	virtual TInt Caps(TDes8& anInfo);
	virtual TInt Format(TFormatInfo& anInfo);
private:
	CEmptyExtProxyDrive(CProxyDrive* aProxyDrive, CMountCB* aMount);
private:
	TInt iInitialise;
	TInt iDismounted;
	TInt iEnlarge;
	TInt iReduceSize;
	TInt iReadThread;
	TInt iRead;
	TInt iWriteThread;
	TInt iWrite;
	TInt iCaps;
	TInt iFormat;
	};

class CEmptyProxyDriveFactory : public CProxyDriveFactory
	{
public:
	CEmptyProxyDriveFactory();
	virtual TInt Install();			
	virtual CProxyDrive* NewProxyDriveL(CProxyDrive* aProxy,CMountCB* aMount);
	};

CEmptyExtProxyDrive* CEmptyExtProxyDrive::NewL(CProxyDrive* aProxyDrive, CMountCB* aMount)
//
//
//
	{
	CEmptyExtProxyDrive* temp=new(ELeave) CEmptyExtProxyDrive(aProxyDrive,aMount);
	return(temp);
	}


CEmptyExtProxyDrive::CEmptyExtProxyDrive(CProxyDrive* aProxyDrive, CMountCB* aMount):CBaseExtProxyDrive(aProxyDrive,aMount) 
	{
	RDebug::Print(_L("CEmptyExtProxyDrive::CEmptyExtProxyDrive"));
	}

CEmptyExtProxyDrive::~CEmptyExtProxyDrive()
//
//
//
	{
	// print out the figures obtained
	RDebug::Print(_L("Initialise = %d"),iInitialise);
	RDebug::Print(_L("Dismounted = %d"),iDismounted);
	RDebug::Print(_L("Enlarge = %d"),iEnlarge);
	RDebug::Print(_L("ReduceSize = %d"),iReduceSize);
	RDebug::Print(_L("Read Thread = %d"),iReadThread);
	RDebug::Print(_L("Read = %d"),iRead);
	RDebug::Print(_L("Write Thread = %d"),iWriteThread);
	RDebug::Print(_L("Write = %d"),iWrite);
	RDebug::Print(_L("Format = %d"),iFormat);
	}

TInt CEmptyExtProxyDrive::Initialise()
//
//
//
	{
	RDebug::Print(_L("CEmptyExtProxyDrive::Initialise()"));
	++iInitialise;
	return(CBaseExtProxyDrive::Initialise());
	}

TInt CEmptyExtProxyDrive::Dismounted()
//
//
//
	{
	RDebug::Print(_L("CEmptyExtProxyDrive::Dismounted()"));
	++iDismounted;
	return(CBaseExtProxyDrive::Dismounted());
	}

TInt CEmptyExtProxyDrive::Enlarge(TInt aLength)
//
//
//
	{
	++iEnlarge;
	return(CBaseExtProxyDrive::Enlarge(aLength));
	}


TInt CEmptyExtProxyDrive::ReduceSize(TInt aPos, TInt aLength)
//
//
//
	{
	++iReduceSize;
	return(CBaseExtProxyDrive::ReduceSize(aPos,aLength));
	}

TInt CEmptyExtProxyDrive::Read(TInt64 aPos,TInt aLength,const TAny* aTrg,TInt aThreadHandle,TInt anOffset)
//
//
//
	{
	++iReadThread;
	return(CBaseExtProxyDrive::Read(aPos,aLength,aTrg,aThreadHandle,anOffset));
	}

TInt CEmptyExtProxyDrive::Read(TInt64 aPos,TInt aLength,TDes8& aTrg)
//
//
//
	{
	++iRead;
	return(CBaseExtProxyDrive::Read(aPos,aLength,aTrg));
	}

TInt CEmptyExtProxyDrive::Write(TInt64 aPos,TInt aLength,const TAny* aSrc,TInt aThreadHandle,TInt anOffset)
//
//
//
	{
	++iWriteThread;
	return(CBaseExtProxyDrive::Write(aPos,aLength,aSrc,aThreadHandle,anOffset));
	}

TInt CEmptyExtProxyDrive::Write(TInt64 aPos,const TDesC8& aSrc)
//
//
//
	{
	++iWrite;
	return(CBaseExtProxyDrive::Write(aPos,aSrc));
	}

TInt CEmptyExtProxyDrive::Caps(TDes8& anInfo)
//
//
//
	{
	++iCaps;
	return(CBaseExtProxyDrive::Caps(anInfo));
	}

TInt CEmptyExtProxyDrive::Format(TFormatInfo& anInfo)
//
//
//
	{
	++iFormat;
	return(CBaseExtProxyDrive::Format(anInfo));
	}


CEmptyProxyDriveFactory::CEmptyProxyDriveFactory()
//
//
//
	{
	RDebug::Print(_L("CEmptyProxyDriveFactory::CEmptyProxyDriveFactory"));
	}

TInt CEmptyProxyDriveFactory::Install()
//
//
//
	{
	_LIT(KEmptyName,"Empty");
	return(SetName(&KEmptyName));
	}

CProxyDrive* CEmptyProxyDriveFactory::NewProxyDriveL(CProxyDrive* aProxy,CMountCB* aMount)
//
//
//
	{
	return(CEmptyExtProxyDrive::NewL(aProxy,aMount));
	}

extern "C" {

EXPORT_C CProxyDriveFactory* CreateFileSystem()
//
// Create a new file system
//
	{
	return(new CEmptyProxyDriveFactory());
	}
}


