// Copyright (c) 1996-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// f32\sfat\sl_dir.cpp
// 
//

#include "sl_std.h"

CFatDirCB* CFatDirCB::NewL()
//
// Static constructor
//
	{

	CFatDirCB* fatDirCB=new(ELeave) CFatDirCB;
	return fatDirCB;
	}

CFatDirCB::CFatDirCB()
//
// Constructor
//
	{
	}

CFatDirCB::~CFatDirCB()
//
// Destructor
//
	{
	
	delete iMatch;
	delete iLongNameBuf;
	}

void CFatDirCB::SetDirL(const TFatDirEntry& anEntry,const TDesC& aName)
//
// Set the current entry to anEntryAddr
//
	{

	__PRINT(_L("CFatDirCB::SetDirL"));	
//	iEntryAddr=0;
//	iPending=EFalse;
	iEntry=anEntry;
	iCurrentPos.iCluster= FatMount().StartCluster(iEntry);
	iMatch=aName.AllocL();
	if (iUidType[0]!=TUid::Null() || iUidType[1]!=TUid::Null() || iUidType[2]!=TUid::Null())
		iMatchUid=ETrue;
	}

LOCAL_C TBool CompareUid(const TUidType& aUidTrg, const TUidType& aUidSuitor)
//
// Compare the suitor to the target pattern
//
	{
	
	if (aUidTrg[0]!=TUid::Null() && aUidTrg[0]!=aUidSuitor[0])
		return(EFalse);
	if (aUidTrg[1]!=TUid::Null() && aUidTrg[1]!=aUidSuitor[1])
		return(EFalse);
	if (aUidTrg[2]!=TUid::Null() && aUidTrg[2]!=aUidSuitor[2])
		return(EFalse);
	return(ETrue);
	}

void CFatDirCB::ReadL(TEntry &anEntry)
//
// Read the next entry from the directory.
//
	{

	__PRINT(_L("CFatDirCB::ReadL"));
    
    FatMount().CheckStateConsistentL();
    

	Mem::FillZ(&anEntry.iType,sizeof(TUidType));

	TPtr entryName(anEntry.iName.Des());
	FOREVER
		{
		if (iPending)
			entryName=(*iLongNameBuf);
		else
			{
            FatMount().FindDosNameL(*iMatch,iAtt,iCurrentPos,iEntry,entryName,KErrEof);
			FatMount().MoveToNextEntryL(iCurrentPos);
			}
		iPending=EFalse;
		if (iEntry.Attributes()&~KEntryAttMaskSupported)
			continue; // Ignore illegal entries
		anEntry.iAtt=iEntry.Attributes();
		anEntry.iSize=iEntry.Size();
		anEntry.iModified=iEntry.Time(FatMount().TimeOffset() );
	
		if (entryName.Length()==0)
			{
			//	VFAT entry names are always created for UNICODE at an earlier stage
			//	However, UNICODE builds may still encounter genuine FAT filenames through
			//	the introduction of files created using a narrow (ASCII) build
			TBuf8<0x20> dosName(DosNameFromStdFormat(iEntry.Name()));
			LocaleUtils::ConvertToUnicodeL(entryName, dosName);
			}
		TBool matchedUid=ETrue;
		if (iMatchUid && (anEntry.iAtt&KEntryAttDir)==EFalse)
			{
			if ((TUint)anEntry.iSize>=sizeof(TCheckedUid))
				FatMount().ReadUidL(FatMount().StartCluster(iEntry),anEntry);
			if (CompareUid(iUidType,anEntry.iType)==EFalse)
				matchedUid=EFalse;
			}
					
		if (matchedUid)
			break;
		}
	if ((iAtt&KEntryAttAllowUid)==0 || (anEntry.iAtt&KEntryAttDir) || (TUint)anEntry.iSize<sizeof(TCheckedUid))
		return;
	if (iMatchUid==EFalse)
		{
		TInt r;
		TRAP(r,FatMount().ReadUidL(FatMount().StartCluster(iEntry),anEntry));
		if(r!=KErrNone && r!=KErrCorrupt)
			User::Leave(r);
		}
	}

void CFatDirCB::StoreLongEntryNameL(const TDesC& aName)
//
// Store aName while next read is pending
//
	{

	AllocBufferL(iLongNameBuf,aName);
	}
