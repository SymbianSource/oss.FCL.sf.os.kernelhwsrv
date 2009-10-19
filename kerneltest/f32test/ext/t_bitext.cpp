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
// f32test\ext\bitext.cpp
// extension to do XOR on every byte on 32 byte boundary read or written to media subsystem in same thread
// therefore RFile::Read/Write does not have this operation carried out on it
// 
//

#include <f32fsys.h>

class CBitExtProxyDrive : public CBaseExtProxyDrive
	{
public:
	static CBitExtProxyDrive* NewL(CProxyDrive* aProxyDrive, CMountCB* aMount);
	~CBitExtProxyDrive();
public:
	virtual TInt Initialise();
	virtual TInt Dismounted();
	virtual TInt Enlarge(TInt aLength);
	virtual TInt ReduceSize(TInt aPos, TInt aLength);
	virtual TInt Read(TInt64 aPos,TInt aLength,const TAny* aTrg,TInt aThreadHandle,TInt anOffset);
	virtual TInt Read(TInt64 aPos,TInt aLength,TDes8& aTrg);
	virtual TInt Write(TInt64 aPos,TInt aLength,const TAny* aSrc,TInt aThreadHandle,TInt anOffset);
	virtual TInt Write(TInt64 aPos,const TDesC8& aSrc);
	virtual TInt Format(TFormatInfo& anInfo);
	virtual TInt GetInterface(TInt aInterfaceId,TAny*& aInterface,TAny* aInput);
private:
	CBitExtProxyDrive(CProxyDrive* aProxyDrive, CMountCB* aMount);
private:
	TInt iReadThread;
	TInt iRead;
	TInt iWriteThread;
	TInt iWrite;
	TInt iFormat;
	};

class CBitProxyDriveFactory : public CProxyDriveFactory
	{
public:
	CBitProxyDriveFactory();
	virtual TInt Install();			
	virtual CProxyDrive* NewProxyDriveL(CProxyDrive* aProxy,CMountCB* aMount);
	};

const TUint8 KBitMask=0xcc;
const TInt KChangePosMask=0x0000001f;

LOCAL_C void DoXOR(TInt aPos,TInt aLength,TUint8* aPtr)
//
//
//
	{
	for(TInt i=0;i<aLength;++i)
		{
		if(((i+aPos)&KChangePosMask)==0)
			aPtr[i]^=KBitMask;
		}
	}


CBitExtProxyDrive* CBitExtProxyDrive::NewL(CProxyDrive* aProxyDrive, CMountCB* aMount)
//
//
//
	{
	CBitExtProxyDrive* temp=new(ELeave) CBitExtProxyDrive(aProxyDrive,aMount);
	return(temp);
	}


CBitExtProxyDrive::CBitExtProxyDrive(CProxyDrive* aProxyDrive, CMountCB* aMount):CBaseExtProxyDrive(aProxyDrive,aMount)
	{
	RDebug::Print(_L("CBitExtProxyDrive::CBitExtProxyDrive"));
	}

CBitExtProxyDrive::~CBitExtProxyDrive()
//
//
//
	{
	RDebug::Print(_L("Read Thread = %d"),iReadThread);
	RDebug::Print(_L("Read = %d"),iRead);
	RDebug::Print(_L("Write Thread = %d"),iWriteThread);
	RDebug::Print(_L("Write = %d"),iWrite);
	RDebug::Print(_L("Format = %d"),iFormat);
	}

TInt CBitExtProxyDrive::Initialise()
//
//
//
	{
	return(CBaseExtProxyDrive::Initialise());
	}

TInt CBitExtProxyDrive::Dismounted()
//
//
//
	{
	return(CBaseExtProxyDrive::Dismounted());
	}

TInt CBitExtProxyDrive::Enlarge(TInt aLength)
//
//
//
	{
	return(CBaseExtProxyDrive::Enlarge(aLength));
	}


TInt CBitExtProxyDrive::ReduceSize(TInt aPos, TInt aLength)
//
//
//
	{
	return(CBaseExtProxyDrive::ReduceSize(aPos,aLength));
	}

TInt CBitExtProxyDrive::Read(TInt64 aPos,TInt aLength,const TAny* aTrg,TInt aThreadHandle,TInt anOffset)
//
//
//
	{
	++iReadThread;
	return(CBaseExtProxyDrive::Read(aPos,aLength,aTrg,aThreadHandle,anOffset));
	}

TInt CBitExtProxyDrive::Read(TInt64 aPos,TInt aLength,TDes8& aTrg)
//
//
//
	{
	++iRead;
	TInt r=CBaseExtProxyDrive::Read(aPos,aLength,aTrg);
	DoXOR(I64INT(aPos),aLength,&aTrg[0]);
	return(r);
	}


TInt CBitExtProxyDrive::Write(TInt64 aPos,TInt aLength,const TAny* aSrc,TInt aThreadHandle,TInt anOffset)
//
//
//
	{
	++iWriteThread;
	return(CBaseExtProxyDrive::Write(aPos,aLength,aSrc,aThreadHandle,anOffset));
	}

TInt CBitExtProxyDrive::Write(TInt64 aPos,const TDesC8& aSrc)
//
//
//
	{
	++iWrite;

	// data may be read-only, so need to copy to a local buffer
	// before XOR-ing...
	HBufC8* buf = HBufC8::New( aSrc.Length() );
	if( !buf )
		return KErrNoMemory;
	TPtr8 ptr = buf->Des();
	ptr.Copy(aSrc);

	DoXOR(I64INT(aPos), ptr.Length(), (TUint8*)ptr.Ptr());
	TInt r = CBaseExtProxyDrive::Write(aPos, ptr);
	
	delete buf;

	return(r);
	}

TInt CBitExtProxyDrive::Format(TFormatInfo& anInfo)
//
//
//
	{
	++iFormat;
	return(CBaseExtProxyDrive::Format(anInfo));
	}


TInt CBitExtProxyDrive::GetInterface(TInt aInterfaceId,TAny*& aInterface,TAny* aInput)
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

CBitProxyDriveFactory::CBitProxyDriveFactory()
//
//
//
	{
	RDebug::Print(_L("CBitProxyDriveFactory::CBitProxyDriveFactory"));
	}

TInt CBitProxyDriveFactory::Install()
//
//
//
	{
	_LIT(KBitName,"Bitchange");
	return(SetName(&KBitName));
	}


CProxyDrive* CBitProxyDriveFactory::NewProxyDriveL(CProxyDrive* aProxy,CMountCB* aMount)
//
//
//
	{
	return(CBitExtProxyDrive::NewL(aProxy,aMount));
	}

extern "C" {

EXPORT_C CProxyDriveFactory* CreateFileSystem()
//
// Create a new file system
//
	{
	return(new CBitProxyDriveFactory());
	}
}

