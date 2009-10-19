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
// f32test\ext\logext.cpp
// extension that records/prints out accesses to each function and length of reads and writes
// 
//

#include <f32fsys.h>

struct TAccessLength
	{
	TInt iless512;
	TInt i512;
	TInt imore512;
	};

class CLoggerExtProxyDrive : public CBaseExtProxyDrive
	{
public:
	static CLoggerExtProxyDrive* NewL(CProxyDrive* aProxyDrive, CMountCB* aMount);
	~CLoggerExtProxyDrive();
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
	virtual TInt GetInterface(TInt aInterfaceId,TAny*& aInterface,TAny* aInput);
private:
	CLoggerExtProxyDrive(CProxyDrive* aProxyDrive, CMountCB* aMount);
private:
	TInt iInitialise;
	TInt iDismounted;
	TInt iEnlarge;
	TInt iReduceSize;
	TInt iReadThread;
	TAccessLength iReadThreadLength;
	TInt iRead;
	TAccessLength iReadLength;
	TInt iWriteThread;
	TAccessLength iWriteThreadLength;
	TInt iWrite;
	TAccessLength iWriteLength;
	TInt iCaps;
	TInt iFormat;
	};

class CLoggerProxyDriveFactory : public CProxyDriveFactory
	{
public:
	CLoggerProxyDriveFactory();
	virtual TInt Install();			
	virtual CProxyDrive* NewProxyDriveL(CProxyDrive* aProxy,CMountCB* aMount);
	};



LOCAL_C void LogLength(TAccessLength& aAccess,TInt aLength)
//
//
//
	{
	if(aLength<512)
		{
		aAccess.iless512++;
		}
	else if(aLength==512)
		{
		aAccess.i512++;
		}
	else
		aAccess.imore512++;
	}

LOCAL_C void PrintLength(TAccessLength& aAccess)
//
//
//
	{
	RDebug::Print(_L("\t<512 = %d"),aAccess.iless512);
	RDebug::Print(_L("\t 512 = %d"),aAccess.i512);
	RDebug::Print(_L("\t>512 = %d"),aAccess.imore512);
	}


CLoggerExtProxyDrive* CLoggerExtProxyDrive::NewL(CProxyDrive* aProxyDrive, CMountCB* aMount)
//
//
//
	{
	CLoggerExtProxyDrive* temp=new(ELeave) CLoggerExtProxyDrive(aProxyDrive,aMount);
	return(temp);
	}


CLoggerExtProxyDrive::CLoggerExtProxyDrive(CProxyDrive* aProxyDrive, CMountCB* aMount):CBaseExtProxyDrive(aProxyDrive,aMount)
	{
	RDebug::Print(_L("CLoggerExtProxyDrive::CLoggerExtProxyDrive"));
	}

CLoggerExtProxyDrive::~CLoggerExtProxyDrive()
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
	if(iReadThread)
		PrintLength(iReadThreadLength);
	RDebug::Print(_L("Read = %d"),iRead);
	if(iRead)
		PrintLength(iReadLength);
	RDebug::Print(_L("Write Thread = %d"),iWriteThread);
	if(iWriteThread)
		PrintLength(iWriteThreadLength);
	RDebug::Print(_L("Write = %d"),iWrite);
	if(iWrite)
		PrintLength(iWriteLength);
	RDebug::Print(_L("Caps = %d"),iCaps);
	RDebug::Print(_L("Format = %d"),iFormat);
	}

TInt CLoggerExtProxyDrive::Initialise()
//
//
//
	{
	RDebug::Print(_L("CLoggerExtProxyDrive::Initialise()"));
	++iInitialise;
	return(CBaseExtProxyDrive::Initialise());
	}

TInt CLoggerExtProxyDrive::Dismounted()
//
//
//
	{
	RDebug::Print(_L("CLoggerExtProxyDrive::Dismounted()"));
	++iDismounted;
	return(CBaseExtProxyDrive::Dismounted());
	}

TInt CLoggerExtProxyDrive::Enlarge(TInt aLength)
//
//
//
	{
	++iEnlarge;
	return(CBaseExtProxyDrive::Enlarge(aLength));
	}


TInt CLoggerExtProxyDrive::ReduceSize(TInt aPos, TInt aLength)
//
//
//
	{
	++iReduceSize;
	return(CBaseExtProxyDrive::ReduceSize(aPos,aLength));
	}

TInt CLoggerExtProxyDrive::Read(TInt64 aPos,TInt aLength,const TAny* aTrg,TInt aThreadHandle,TInt anOffset)
//
//
//
	{
	++iReadThread;
	LogLength(iReadThreadLength,aLength);
	return(CBaseExtProxyDrive::Read(aPos,aLength,aTrg,aThreadHandle,anOffset));
	}

TInt CLoggerExtProxyDrive::Read(TInt64 aPos,TInt aLength,TDes8& aTrg)
//
//
//
	{
	++iRead;
	LogLength(iReadLength,aLength);
	return(CBaseExtProxyDrive::Read(aPos,aLength,aTrg));
	}

TInt CLoggerExtProxyDrive::Write(TInt64 aPos,TInt aLength,const TAny* aSrc,TInt aThreadHandle,TInt anOffset)
//
//
//
	{
	++iWriteThread;
	LogLength(iWriteThreadLength,aLength);
	return(CBaseExtProxyDrive::Write(aPos,aLength,aSrc,aThreadHandle,anOffset));
	}

TInt CLoggerExtProxyDrive::Write(TInt64 aPos,const TDesC8& aSrc)
//
//
//
	{
	++iWrite;
	LogLength(iWriteLength,aSrc.Length());
	return(CBaseExtProxyDrive::Write(aPos,aSrc));
	}

TInt CLoggerExtProxyDrive::Caps(TDes8& anInfo)
//
//
//
	{
	++iCaps;
	return(CBaseExtProxyDrive::Caps(anInfo));
	}

TInt CLoggerExtProxyDrive::Format(TFormatInfo& anInfo)
//
//
//
	{
	++iFormat;
	return(CBaseExtProxyDrive::Format(anInfo));
	}


TInt CLoggerExtProxyDrive::GetInterface(TInt aInterfaceId,TAny*& aInterface,TAny* aInput)
	{
	switch(aInterfaceId)
		{
		// file caching supported, so pass query on to next extension
		case ELocalBufferSupport:
			return CBaseExtProxyDrive::LocalBufferSupport();

		default:
			return CBaseExtProxyDrive::GetInterface(aInterfaceId, aInterface, aInput);
		}
	}

CLoggerProxyDriveFactory::CLoggerProxyDriveFactory()
//
//
//
	{
	RDebug::Print(_L("CLoggerProxyDriveFactory::CLoggerProxyDriveFactory"));
	}

TInt CLoggerProxyDriveFactory::Install()
//
//
//
	{
	_LIT(KLoggerName,"Logger");
	return(SetName(&KLoggerName));
	}


CProxyDrive* CLoggerProxyDriveFactory::NewProxyDriveL(CProxyDrive* aProxy,CMountCB* aMount)
//
//
//
	{
	return(CLoggerExtProxyDrive::NewL(aProxy,aMount));
	}

extern "C" {

EXPORT_C CProxyDriveFactory* CreateFileSystem()
//
// Create a new file system
//
	{
	return(new CLoggerProxyDriveFactory());
	}
}

