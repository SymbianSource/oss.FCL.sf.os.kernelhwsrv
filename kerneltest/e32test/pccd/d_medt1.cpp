// Copyright (c) 1998-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// e32test\pccd\d_medt1.cpp
// 
//

#if defined(_UNICODE)
#if !defined(UNICODE)
#define UNICODE
#endif
#endif
#include <kernel/kernel.h>
const TInt KTestDriverBufferMaxSize=0x1000;	// 4K

class DPhysicalDeviceMediaTest : public DPhysicalDeviceMedia
	{
public:
	DPhysicalDeviceMediaTest();
	virtual TInt Install();
	virtual TInt Remove();
	virtual void GetCaps(TDes8 &aDes) const;
	virtual CBase *CreateL(TInt aDevice,const TDesC *anInfo,const TVersion &aVer);
	virtual TInt Priority();
	};
								
class DMediaDriverTest : public DMediaDriver
	{
public:
	DMediaDriverTest();
	TInt DoCreate(TMediaDevice aMediaDevice);
	void DoRead(TInt64 &aPos,TInt aLength,TMediaDrvDescData &aTrg);
	void DoWrite(TInt64 &aPos,TInt aLength,TMediaDrvDescData &aSrc);
	void DoFormat(TInt64 &aPos,TInt aLength);
	TInt Enlarge(TInt aLength);
	TInt ReduceSize(TInt64 &aPos,TInt aLength);
	void Close();
	TInt PartitionInfo(TPartitionInfo &anInfo);
	void Caps(TDes8& aCapsBuf);
private:
	static void WriteCompleteCallBack(TAny *aMediaDriver,TInt aDelay);
	static void FormatCompleteCallBack(TAny *aMediaDriver,TInt aDelay);
private:
	TUint8 iBuf[KTestDriverBufferMaxSize];
	TTickLink iWriteTickLink;
	TTickLink iFormatTickLink;
	};

DPhysicalDeviceMediaTest::DPhysicalDeviceMediaTest()
//
// Constructor
//
	{
	iUnitsMask=0x1; // Support only one 
	iVersion=TVersion(KMediaDriverInterfaceMajorVersionNumber,KMediaDriverInterfaceMinorVersionNumber,KMediaDriverInterfaceBuildVersionNumber);
	}

TInt DPhysicalDeviceMediaTest::Install()
//
// Install the test Media PDD.
//
	{

    TPtrC name=_L("Media.Tst1");
	return(SetName(&name));
	}

TInt DPhysicalDeviceMediaTest::Remove()
//
// Remove the test Media PDD.
//
	{
	return(KErrNone);
	}

void DPhysicalDeviceMediaTest::GetCaps(TDes8 &/* aDes */) const
//
// Return the media drivers capabilities.
//
	{
	}
								 
CBase *DPhysicalDeviceMediaTest::CreateL(TInt aDevice,const TDesC * /* anInfo */,const TVersion &aVer)
//
// Create a test media driver.
//
	{
	if (User::QueryVersionSupported(iVersion,aVer)==EFalse)
		User::Leave(KErrNotSupported);
	DMediaDriverTest *mD=new(ELeave) DMediaDriverTest;
	TInt ret=mD->DoCreate((TMediaDevice)aDevice);
	if (ret!=KErrNone)
		{
		delete mD;
		User::Leave(ret);
		}
	return(mD);
	}

TInt DPhysicalDeviceMediaTest::Priority()
//
// Return the priority of this media driver
//
	{

	return(KMediaDriverPriorityLow); 
	}

DMediaDriverTest::DMediaDriverTest()
//
// Constructor.
//
	{

//	Mem::FillZ(&iBuf[0],sizeof(iBuf));
	}

TInt DMediaDriverTest::DoCreate(TMediaDevice aMediaDevice)
//
// Create the media driver.
//
	{

   	if (!__IS_REMOVABLE(aMediaDevice))
		return(KErrNotSupported);
	SetTotalSizeInBytes(KTestDriverBufferMaxSize);
	return(KErrNone);
	}

void DMediaDriverTest::DoRead(TInt64 &aPos,TInt aLength,TMediaDrvDescData &aTrg)
//
// Read from specified area of media.
//
	{

	TInt ret=KErrNone;
	if (!aTrg.IsCurrentThread())
		ret=KErrGeneral;
	else
		{
		if ((ret=aTrg.CurrentThreadDescCheck(aLength))==KErrNone)
            {
			TDes8 &t=*((TDes8*)aTrg.iPtr);
			Mem::Copy((TAny*)(t.Ptr()+aTrg.iOffset),&iBuf[aPos.Low()],aLength);
			t.SetLength(aLength+aTrg.iOffset);
			}
		}
    Complete(KMediaDrvReadReq,ret);
	}

const TInt KWriteAsyncTime=100000;	// 100mS
void DMediaDriverTest::DoWrite(TInt64 &aPos,TInt aLength,TMediaDrvDescData &aSrc)
//
// Write to specifed area of media.
//
	{

	if (!aSrc.IsCurrentThread())
        Complete(KMediaDrvWriteReq,KErrGeneral);
	else
        {
		Mem::Copy(&iBuf[aPos.Low()],((TDesC8*)aSrc.iPtr)->Ptr()+aSrc.iOffset,aLength);
	    iWriteTickLink.OneShotInMicroSeconds(KWriteAsyncTime,DMediaDriverTest::WriteCompleteCallBack,this);
        }
	}

const TInt KFormatAsyncTime=5000000;	// 5S
void DMediaDriverTest::DoFormat(TInt64 &aPos,TInt aLength)
//
// Format the specified area of the media.
//
	{

    Mem::Fill(&iBuf[aPos.Low()],aLength,0xFF);
	iFormatTickLink.OneShotInMicroSeconds(KFormatAsyncTime,DMediaDriverTest::FormatCompleteCallBack,this);
	}

TInt DMediaDriverTest::Enlarge(TInt /*aLength*/)
//
// Enlarge the drive
//
	{

	return(KErrNotSupported);
	}

TInt DMediaDriverTest::ReduceSize(TInt64& /*aPos*/,TInt /*aLength*/)
//
// Reduce in size the drive
//
	{

	return(KErrNotSupported);
	}

void DMediaDriverTest::Close()
//
// Close the media driver
//
	{}

TInt DMediaDriverTest::PartitionInfo(TPartitionInfo &anInfo)
//
// Return partition information on the media.
//
	{

	anInfo.iPartitionCount=1;
	anInfo.iEntry[0].iPartitionBaseAddr=0;
	anInfo.iEntry[0].iPartitionLen=anInfo.iMediaSizeInBytes=TotalSizeInBytes();
	return(KErrNone);
	}

void DMediaDriverTest::Caps(TDes8& aCapsBuf)
//
// Return the capabilities of the media
	{

	TLocalDriveCapsV2 caps;
	caps.iType=EMediaRam;
	caps.iConnectionBusType=EConnectionBusInternal;
	caps.iDriveAtt=KDriveAttLocal|KDriveAttRemovable;
	caps.iMediaAtt=KMediaAttFormattable;
	caps.iFileSystemId=KDriveFileSysFAT;
	caps.iHiddenSectors=0;
	aCapsBuf.FillZ(aCapsBuf.MaxLength());
	aCapsBuf.Copy((TUint8 *)&caps,Min(aCapsBuf.MaxLength(),sizeof(caps)));
	}

void DMediaDriverTest::WriteCompleteCallBack(TAny *aMediaDriver,TInt /*aDelay*/)
//
// Complete a write request
//
	{

	DMediaDriverTest &md=*(DMediaDriverTest*)aMediaDriver;
    md.Complete(KMediaDrvWriteReq,KErrNone);
	return;
	}

void DMediaDriverTest::FormatCompleteCallBack(TAny *aMediaDriver,TInt /*aDelay*/)
//
// Complete a format request
//
	{

	DMediaDriverTest &md=*(DMediaDriverTest*)aMediaDriver;
    md.Complete(KMediaDrvFormatReq,KErrNone);
	return;
	}

DECLARE_STANDARD_PDD()
	{
	return(new DPhysicalDeviceMediaTest);
	}

