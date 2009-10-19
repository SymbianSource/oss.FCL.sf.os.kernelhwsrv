// Copyright (c) 1997-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// f32test\fsstress\t_remfil.cpp
// 
//

#if defined(_UNICODE)
#if !defined(UNICODE)
#define UNICODE
#endif
#endif

/*
#define WIN32_LEAN_AND_MEAN
#pragma warning( disable : 4201 ) // nonstandard extension used : nameless struct/union
 #include <windows.h>
#pragma warning( default : 4201 ) // nonstandard extension used : nameless struct/union
#include <stdlib.h>
*/
#include <f32file.h>
#include <f32fsys.h>
#include <f32ver.h>
#include <e32twin.h>
#include <e32uid.h>

#include "t_remfsy.h"



//////////////////////////////////////////////////////////////////////////
//								CRemoteFileCB							//
//////////////////////////////////////////////////////////////////////////	

CRemoteFileCB::CRemoteFileCB()
//
// Constructor
//
	{

//	iCurrentPos=0;
//	iAttPending=EFalse;
//	iWinHandle=NULL;
	__DECLARE_NAME(_S("CRemoteFileCB"));
	}

CRemoteFileCB::~CRemoteFileCB()
//
// Destructor
//
	{

	if (iAtt&KEntryAttModified)
		{
		TRAP_IGNORE(FlushDataL());
//		if (ret!=KErrNone)		//	Can fail if floppy disk is removed
//			Panic(EFileClose);	//	Ignore error
		}
//	if (iWinHandle!=NULL && CloseHandle(iWinHandle)==FALSE)
//		Panic(EFileClose);
	}

TBool CRemoteFileCB::IsRomDrive() const
//
// Returns ETrue if the drive number == EDriveZ
//
	{
	return(((CRemoteFileCB*)this)->Mount().Drive().DriveNumber()==EDriveZ);
	}


void CRemoteFileCB::CheckPos(TInt /*aPos*/)
//
//	Check that the file is positioned correctly.
//	Dummy implementation
//
	{}


 void CRemoteFileCB::ReadL(TInt /*aPos*/,TInt& /*aLength*/,const TAny* /*aDes*/,const RMessagePtr2& /*aMessage*/)
//
//	Read from the file
//	Dummy implementation of a pure virtual function
//
	{
	User::After(200000);	//	Wait 0.2 seconds
	}


void CRemoteFileCB::WriteL(TInt /*aPos*/,TInt& /*aLength*/,const TAny* /*aDes*/,const RMessagePtr2& /*aMessage*/)
//
//	Write to the file
//
	{
	User::After(200000);	//	Wait 0.2 seconds	
/*
	TBuf8<0x100> buf;
	if (IsRomDrive())
		User::Leave(KErrAccessDenied);
	CheckPos(aPos);
	TInt pos=0;
	TInt len=aLength;
	RThread thread;
	while (len)
		{
		TInt s=Min(len,buf.MaxLength());
		thread.ReadL(aDes,buf,pos); 
	//	Reading from client thread descriptor to fileserver thread
		
	//	printf(iConsole,_L("%S\n"),&buf);
		
		len-=s;
		pos+=s;
		}
	aLength=pos;
	iCurrentPos=aPos+pos;
*/
	}

TInt CRemoteFileCB::Address(TInt& /*aPos*/) const
//
//	If ROM file, do a memory map and return the address
//	Dummy implementation
//
	{
	return(KErrNone);
	}

void CRemoteFileCB::SetSizeL(TInt /*aSize*/)
//
//	Set the file size
//	Dummy implementation of a pure virtual function
//
	{
	User::After(200000);	//	Wait 0.2 seconds
	}

void CRemoteFileCB::SetEntryL(const TTime& /*aTime*/,TUint /*aSetAttMask*/,TUint /*aClearAttMask*/)
//
//	Set the entry's attributes and modified time
//	Dummy implementation of a pure virtual function
//
	{
	User::After(200000);	//	Wait 0.2 seconds
	}


void CRemoteFileCB::FlushAllL()
//
// Commit any buffered date to the media.
//
	{
	
	FlushDataL();
	}


void CRemoteFileCB::FlushDataL()
//
//	Commit any buffered date to the media
//	Dummy implementation of a pure virtual function
//
	{}

void CRemoteFileCB::RenameL(const TDesC& /*aNewName*/)
//
//	Rename the file while open
//	Dummy implementation of a pure virtual function
//
	{
	User::After(200000);	//	Wait 0.2 seconds
	}

/*
LOCAL_C void printf(RConsole& aConsole, TRefByValue<const TDesC> aFmt,...)
//
// Print to the console
//
	{

	if (aConsole.Handle()==KNullHandle)
		{
		TInt r=aConsole.Init(_L("RemoteFSys"),TSize(KDefaultConsWidth,KDefaultConsHeight));
		__ASSERT_ALWAYS(r==KErrNone,User::Panic(_L("Open-Console"),0));
		r=aConsole.Control(_L("+Maximize +NewLine -Lock -Wrap"));
		__ASSERT_ALWAYS(r==KErrNone,User::Panic(_L("Config-Console"),0));
		}
	VA_LIST list;
	VA_START(list,aFmt);
	TBuf<0x100> aBuf;
	aBuf.AppendFormatList(aFmt,list);
	TInt r=aConsole.Write(aBuf);
	__ASSERT_ALWAYS(r==KErrNone,User::Panic(_L("Write-Console"),0));
	}
*/
