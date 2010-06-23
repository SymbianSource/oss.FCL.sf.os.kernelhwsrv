// Copyright (c) 1995-2009 Nokia Corporation and/or its subsidiary(-ies).
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

#include "sf_std.h"

TInt TFsMkDir::DoRequestL(CFsRequest* aRequest)
//
// Make a directory path.
//
	{
	__PRINT(_L("TFsMkDir::DoRequestL(CFsRequest* aRequest)"));

    TInt r = CheckDiskSpace(KMinFsCreateObjTreshold, aRequest);
    if(r != KErrNone)
        return r;

	r=aRequest->Drive()->CheckMount();
	if (r!=KErrNone)
		return(r);
	
    if (aRequest->Src().IsRoot())
		return(KErrAlreadyExists);
	
    TPtrC ptr;
	ptr.Set(aRequest->Src().Path().Ptr(), aRequest->Src().Path().Length()-1);
	if (ptr.Length()<=1)
		return(KErrBadName);
	
    if (IsIllegalFullName(aRequest->Src().FullName()))
		return(KErrBadName);
	
    if (aRequest->Message().Int1())
		{
		TLex lex(ptr);
		TFileName filename;
		FOREVER
			{
			lex.Inc(); // Skip a delimiter
			TInt res=lex.Remainder().Locate(KPathDelimiter);
			if (res==KErrNotFound)
				break;
	
    		lex.Inc(res);
			filename+=lex.MarkedToken();	
			lex.Mark();
			TEntry entry;
			if ((r=aRequest->Drive()->Entry(filename,entry))==KErrNone)
				{
				if (!entry.IsDir())
					return(KErrAccessDenied);
				continue;
				}
	
    		if (r!=KErrNotFound)
				return(r);
	
    		if ((r=aRequest->Drive()->MkDir(filename))!=KErrNone)
				return(r);
			}
		}
	
    return(aRequest->Drive()->MkDir(ptr));
	}

TInt TFsMkDir::Initialise(CFsRequest* aRequest)
//
//
//
	{
	TInt r=ParseSubstPtr0(aRequest,aRequest->Src());
	if (r!=KErrNone)
		return(r);
	r=PathCheck(aRequest,aRequest->Src().FullName().Mid(2),&KCapFsSysMkDir,&KCapFsPriMkDir,&KCapFsROMkDir, __PLATSEC_DIAGNOSTIC_STRING("Make Directory"));
	return(r);
	}


TInt TFsRmDir::DoRequestL(CFsRequest* aRequest)
//
// Remove a directory.
//
	{
	__PRINT(_L("TFsRmDir::DoRequestL(CFsRequest* aRequest)"));
	if (aRequest->Src().IsRoot())
		return(KErrInUse);
	if (IsIllegalFullName(aRequest->Src().FullName()))
		return(KErrBadName);
	TPtrC ptr;
	ptr.Set(aRequest->Src().Path().Ptr(), aRequest->Src().Path().Length()-1);
	return(aRequest->Drive()->RmDir(ptr));
	}

TInt TFsRmDir::Initialise(CFsRequest* aRequest)
//
//
//
	{
	TInt r=ParseSubstPtr0(aRequest,aRequest->Src());
	if (r!=KErrNone)
		return(r);
	r=PathCheck(aRequest,aRequest->Src().FullName().Mid(2),&KCapFsSysRmDir,&KCapFsPriRmDir,&KCapFsRORmDir, __PLATSEC_DIAGNOSTIC_STRING("Remove Directory"));
	return(r);
	}


TInt TFsDelete::DoRequestL(CFsRequest* aRequest)
//
// Delete a file.
//
	{
	__PRINT(_L("TFsDelete::DoRequestL(CFsRequest* aRequest)"));
	return(aRequest->Drive()->Delete(aRequest->Src().FullName().Mid(2)));
	}

TInt TFsDelete::Initialise(CFsRequest* aRequest)
//
//
//
	{
	TInt r=ParseNoWildSubstCheckPtr0(aRequest,aRequest->Src());
	if (r!=KErrNone)
		return(r);
	r=PathCheck(aRequest,aRequest->Src().FullName().Mid(2),&KCapFsSysDelete,&KCapFsPriDelete,&KCapFsRODelete, __PLATSEC_DIAGNOSTIC_STRING("Delete"));
	return(r);
	}


TInt TFsRename::DoRequestL(CFsRequest* aRequest)
//
// Rename a file or directory. Wild cards not allowed.
//
	{
	__PRINT(_L("TFsRename::DoRequestL(CFsRequest* aRequest)"));
    TInt r = CheckDiskSpace(KMinFsCreateObjTreshold, aRequest);
    if(r != KErrNone)
        return r;
	
    r = aRequest->Drive()->Rename(aRequest->Src().FullName().Mid(2),aRequest->Dest().FullName().Mid(2));
	return (r);
	}

TInt TFsRename::Initialise(CFsRequest* aRequest)
//
//
//
	{
	TInt r=ParseNoWildSubstCheckPathPtr0(aRequest,aRequest->Src());
	if (r!=KErrNone)
		return(r);
	r=PathCheck(aRequest,aRequest->Src().FullName().Mid(2),&KCapFsSysRename,&KCapFsPriRename,&KCapFsRORename, __PLATSEC_DIAGNOSTIC_STRING("File Server Rename"));
	if(r!=KErrNone)
		return(r);
	TDrive* pOldDrive=aRequest->Drive();
	if ((r=ParseNoWildSubstCheckPathPtr1(aRequest,aRequest->Dest()))!=KErrNone)
		return(r);
	r=PathCheck(aRequest,aRequest->Dest().FullName().Mid(2),&KCapFsSysRename,&KCapFsPriRename,&KCapFsRORename, __PLATSEC_DIAGNOSTIC_STRING("File Server Rename"));	
	if(r == KErrNone)
		{
		if (pOldDrive!=aRequest->Drive())
			r=KErrArgument;
		}
	return(r);
	}


TInt TFsReplace::DoRequestL(CFsRequest* aRequest)
//
// Replace an old file with a new file atomically
//
	{
	__PRINT(_L("TFsReplace::DoRequestL(CFsRequest* aRequest)"));

    TInt r = CheckDiskSpace(KMinFsCreateObjTreshold, aRequest);
    if(r != KErrNone)
        return r;

	return(aRequest->Drive()->Replace(aRequest->Src().FullName().Mid(2),aRequest->Dest().FullName().Mid(2)));
	}

TInt TFsReplace::Initialise(CFsRequest* aRequest)
//
//
//
	{
	TInt r=ParseNoWildSubstCheckPtr0(aRequest,aRequest->Src());
	if (r!=KErrNone)
		return(r);
	r=PathCheck(aRequest,aRequest->Src().FullName().Mid(2),&KCapFsSysReplace,&KCapFsPriReplace,&KCapFsROReplace, __PLATSEC_DIAGNOSTIC_STRING("File Server Replace"));	
	if(r!=KErrNone)
		return(r);
	TDrive* pOldDrive=aRequest->Drive();
	if ((r=ParseNoWildSubstCheckPtr1(aRequest,aRequest->Dest()))!=KErrNone)
		return(r);
	r=PathCheck(aRequest,aRequest->Dest().FullName().Mid(2),&KCapFsSysReplace,&KCapFsPriReplace,&KCapFsROReplace, __PLATSEC_DIAGNOSTIC_STRING("File Server Replace"));	
	if(r == KErrNone)
		{
		if (pOldDrive!=aRequest->Drive())
			r=KErrArgument;
		}
	return(r);
	}


TInt TFsEntry::DoRequestL(CFsRequest* aRequest)
//
// Get all the entry details.
//
	{
	__PRINT(_L("TInt TFsEntry::DoRequestL(CFsRequest* aRequest)"));

	TEntry t; 
	TPtrC filePath = aRequest->Src().FullName().Mid(2);
	TInt r=aRequest->Drive()->Entry(filePath,t);
	if (r!=KErrNone)
		return(r);

	TPckgC<TEntry> p(t);
	aRequest->WriteL(KMsgPtr1,p);
	return(KErrNone);
	}

TInt TFsEntry::Initialise(CFsRequest* aRequest)
//
//
//
	{
	TInt r=ParseNoWildSubstCheckPathPtr0(aRequest,aRequest->Src());
	if (r!=KErrNone)
		return(r);

	// Check the capabilites but always allow the entry to be read for private, system and 
	// resource directories as long as there are no sub folders or files specified
	r=PathCheck(aRequest,aRequest->Src().FullName().Mid(2),&KCapFsEntry, __PLATSEC_DIAGNOSTIC_STRING("Entry"), ETrue);	

	return(r);
	}


TInt TFsSetEntry::DoRequestL(CFsRequest* aRequest)
//
// Set the attributes and the modified date and time.
//
	{
	__PRINT(_L("TFsSetEntry::DoRequestL(CFsRequest* aRequest)"));

    TInt r = CheckDiskSpace(KMinFsCreateObjTreshold, aRequest);
    if(r != KErrNone)
        return r;

	TTime entryTime;
	TPckgBuf<TTime> timeBuf;
	aRequest->ReadL(KMsgPtr1,timeBuf);
	entryTime=timeBuf();
	const RMessage2& msg = aRequest->Message();

	return(aRequest->Drive()->SetEntry(aRequest->Src().FullName().Mid(2),entryTime,msg.Int2(),msg.Int3()));
	}


TInt TFsSetEntry::Initialise(CFsRequest* aRequest)
//
//
//
	{
	TInt r=ParseNoWildSubstCheckPathPtr0(aRequest,aRequest->Src());
	if (r!=KErrNone)
		return(r);
 	r=PathCheck(aRequest,aRequest->Src().FullName().Mid(2),&KCapFsSysSetEntry,&KCapFsPriSetEntry,&KCapFsROSetEntry, __PLATSEC_DIAGNOSTIC_STRING("Set Entry"));	
	return(r);
	}

LOCAL_C void FsReadFileSectionFileClose(CFsRequest* aRequest, CFileShare* aShare)
	{
	aRequest->Session()->DecResourceCount();
	TInt handle = aRequest->Session()->Handles().At(aShare, EFalse);
	aRequest->Session()->Handles().Remove(handle,ETrue);
	// close the file share
	aRequest->SetScratchValue64(0);
	}

LOCAL_C CFileShare* FsReadFileSectionFileOpen(CFsRequest* aRequest)
	{
	TInt handle;
	TUint32 mode = EFileShareReadersOrWriters | EFileRead | EFileReadAheadOff;

	TInt r = aRequest->Drive()->FileOpen(aRequest,handle,aRequest->Src().FullName().Mid(2),mode,EFileOpen);
	if (r!=KErrNone)
		return(NULL);

	CFileShare* share = (CFileShare*) SessionObjectFromHandle(handle, FileShares->UniqueID(), aRequest->Session());
	
	aRequest->Session()->IncResourceCount();

	CFileCB& file = share->File();
	if (!file.FileCache())
		{
		FsReadFileSectionFileClose(aRequest, share);
		share = NULL;
		}
	return share;
	}

TInt TFsReadFileSection::Complete(CFsRequest* aRequest)
	{
	FsReadFileSectionFileClose(aRequest, (CFileShare*) aRequest->ScratchValue());

	return CFsRequest::EReqActionComplete;
	}

TInt TFsReadFileSection::DoRequestL(CFsRequest* aRequest)
//
//	Read from a file regardless of lock state
//
	{
	__PRINT(_L("TFsReadFileSection::DoRequestL(CFsRequest* aRequest)"));
	
	TAny* pDes=(TAny*)(aRequest->Message().Ptr0());
	TInt64 pos;
	if(aRequest->IsDescData(KMsgPtr2))
		{
		TPckg<TInt64> pkPos(pos);
		aRequest->ReadL(KMsgPtr2, pkPos);
		}
	else
		{
		pos = MAKE_TINT64(0,aRequest->Message().Int2());
		}
	TInt len=aRequest->Message().Int3();

	// Try to open file so we can take advantage of file caching
	CFileShare* share = FsReadFileSectionFileOpen(aRequest);
	if (share)
		{
		CFsMessageRequest& msgRequest = *(CFsMessageRequest*) aRequest;
		__ASSERT_DEBUG(msgRequest.CurrentOperationPtr() == NULL, Fault(EBadOperationIndex));
		CFileCB& file = share->File();
		TInt64 size = file.Size64();
		TInt r = KErrNone;
		if (pos > size)
			pos = size;
		if (pos + len > size)
		// filesize - pos shall of TInt size
		// Hence to suppress warning
			len = (TInt)(size - pos);
		if (len <= 0)
			r = KErrArgument;
		if (r == KErrNone)
			r = msgRequest.PushOperation(pos, len, (TDesC8*) pDes, 0, TFsReadFileSection::Complete, 0, EFsFileRead);
		msgRequest.SetState(CFsRequest::EReqStatePostInitialise);	// DO call PostInitialise()
		if (r == KErrNone)
			{
			aRequest->SetScratchValue64( MAKE_TINT64(ETrue, (TUint) share) );
			return CFsRequest::EReqActionBusy; // dispatch the request again
			}
		FsReadFileSectionFileClose(aRequest, share);
		}

	TInt r = KErrNone;
	if (len < 0)
		r = KErrArgument;
	else if (len > 0)
		r = aRequest->Drive()->ReadFileSection64(aRequest->Src().FullName().Mid(2),pos,pDes,len,aRequest->Message());

	// zero return buffer, but only if we haven't written any data to it already, otherwise kernel's
	// writeback of descriptor length will conflict with that wrttien by media driver
	if (r != KErrNone || len == 0)
		aRequest->WriteL(KMsgPtr0,KNullDesC8);
	
	return r == KErrEof ? KErrNone : r;  
	}


TInt TFsReadFileSection::Initialise(CFsRequest* aRequest)
//
//
//
	{
	TInt r=ParseNoWildSubstFileCheckPtr1(aRequest,aRequest->Src());
	if (r!=KErrNone)
		return(r);
	r=PathCheck(aRequest,aRequest->Src().FullName().Mid(2),&KCapFsSysReadFileSection,&KCapFsPriReadFileSection, __PLATSEC_DIAGNOSTIC_STRING("Read File Section"));	
	return(r);
	}



TInt TFsCheckDisk::DoRequestL(CFsRequest* aRequest)
//
// Check the disk's integrity
//
	{

	__PRINT(_L("TFsCheckDisk::DoRequestL(CFsRequest* aRequest)"));

	// flush all files on this drive
	TInt r = aRequest->Drive()->FlushCachedFileInfo();
	if (r == CFsRequest::EReqActionBusy)	// ignore any flush errors
		return(r);

	return(aRequest->Drive()->CheckDisk());
	}


TInt TFsCheckDisk::Initialise(CFsRequest* aRequest)
//
//
//
	{
	if (!KCapFsCheckDisk.CheckPolicy(aRequest->Message(), __PLATSEC_DIAGNOSTIC_STRING("CheckDisk")))						
		return KErrPermissionDenied;
	return ParseSubstPtr0(aRequest,aRequest->Src());	//may make the rfs function call take a drive instead of path
	}

TInt TFsScanDrive::DoRequestL(CFsRequest* aRequest)
//
//
//
	{
	__PRINT(_L("TFsScanDrive::DoRequestL(CFsRequest* aRequest)"));

	// flush all files on this drive
	TInt r = aRequest->Drive()->FlushCachedFileInfo();
	if (r == CFsRequest::EReqActionBusy)	// ignore any flush errors
		return(r);

	r=aRequest->Drive()->CheckMount();
	if(r!=KErrNone)
		return(r);
	if (aRequest->Drive()->CurrentMount().LockStatus()<0)
		return(KErrInUse);
	r=aRequest->Drive()->ScanDrive();
	// notify sessions since drive contents may have changed
	if(r==KErrNone)
		FsNotify::DiskChange(aRequest->DriveNumber());
	return(r);
	}

TInt TFsScanDrive::Initialise(CFsRequest* aRequest)
//
//
//
	{
	TInt r;
	if(!KCapFsScanDrive.CheckPolicy(aRequest->Message(), __PLATSEC_DIAGNOSTIC_STRING("ScanDrive")))	
		return KErrPermissionDenied;
	r=ParseSubstPtr0(aRequest,aRequest->Src());		//may make the rfs function call take a drive instead of path
	return(r);										//then  no need for parse, just needs to check for susted and reject them
	}

TInt TFsGetShortName::DoRequestL(CFsRequest* aRequest)
//
// Get the short name associated with a long file name
//
	{
	__PRINT(_L("TFsGetShortName::DoRequestL(CFsRequest* aRequest)"));
	TBuf<0x10> shortName;
	TInt r=aRequest->Drive()->GetShortName(aRequest->Src().FullName().Mid(2),shortName);
	if (r!=KErrNone)
		return(r);
	aRequest->WriteL(KMsgPtr1,shortName);
	return(KErrNone);
	}

TInt TFsGetShortName::Initialise(CFsRequest* aRequest)
//
//
//
	{
	TInt r=ParseNoWildSubstCheckPathPtr0(aRequest,aRequest->Src());	
	if (r!=KErrNone)
		return(r);
	r=PathCheck(aRequest,aRequest->Src().FullName().Mid(2),&KCapFsSysGetShortName,&KCapFsPriGetShortName, __PLATSEC_DIAGNOSTIC_STRING("Get Short Name"));	
	return(r);
	}


TInt TFsGetLongName::DoRequestL(CFsRequest* aRequest)
//
// Get the long name associated with a short file name
//
	{
	__PRINT(_L("TFsGetLongName::DoRequestL(CFsRequest* aRequest)"));
	TFileName longName;
	TInt r=aRequest->Drive()->GetLongName(aRequest->Src().FullName().Mid(2),longName);
	if (r!=KErrNone)
		return(r);
	aRequest->WriteL(KMsgPtr1,longName);
	return(KErrNone);
	}

TInt TFsGetLongName::Initialise(CFsRequest* aRequest)
//
//
//
	{
	TInt r=ParseNoWildSubstCheckPathPtr0(aRequest,aRequest->Src());	
	if (r!=KErrNone)
		return(r);
	r=PathCheck(aRequest,aRequest->Src().FullName().Mid(2),&KCapFsSysGetLongName,&KCapFsPriGetLongName, __PLATSEC_DIAGNOSTIC_STRING("Get Long Name"));	
	return(r);
	}


TInt TFsIsFileInRom::DoRequestL(CFsRequest* aRequest)
//
// Return the address of the file
//
	{

	__PRINT(_L("DoIsFileInRom"));
	TUint8* fileStart;
	TInt r=aRequest->Drive()->IsFileInRom(aRequest->Src().FullName().Mid(2),fileStart);
	if (r!=KErrNone)
		return(r);
	TPckgBuf<TUint8*> buf(fileStart);
	aRequest->WriteL(KMsgPtr1,buf);
	return(KErrNone);
	}

TInt TFsIsFileInRom::Initialise(CFsRequest* aRequest)
//
//	
//
	{	
	TInt r=ParseNoWildSubstCheckPathPtr0(aRequest,aRequest->Src());	
	if (r!=KErrNone)
		return (r);
	r=PathCheck(aRequest,aRequest->Src().FullName().Mid(2),&KCapFsSysIsFileInRom,&KCapFsPriIsFileInRom, __PLATSEC_DIAGNOSTIC_STRING("Is File In Rom"));	
	return(r);
	}


TInt TFsIsValidName::Initialise(CFsRequest* /*aRequest*/)
//
//	
//
	{
	return(KErrNone);
	}


TInt TFsIsValidName::DoRequestL(CFsRequest* aRequest)
//
//	Determine whether the name is valid
//	If returnInvalidChar flag is set - determine the invalid character and
//	return it to the client
//  If TNameValidParam is specified determine the position of the invalid character
//  and return it to the client.
	{
	__PRINT(_L("DoFsIsValidName"));

	TParse parse; 
	const TAny* pParam = aRequest->Message().Ptr3();
	if(pParam == NULL)
		{
		//	We need to call DoFsIsValidName	to determine what the invalid character is
		TInt r=ParseSubstPtr0(aRequest,parse);
		if ((r!=KErrNone) && (r!=KErrBadName))
			return(r);
		TBool retInvChar;
		TPckg<TBool> bPckg(retInvChar);
		aRequest->ReadL(KMsgPtr1,bPckg);
		if (!retInvChar)
			//	Determine whether the name is invalid but don't bother finding out which
			//	character (if any) is illegal
			{  
			TInt r=ParseNoWildSubstCheckPtr0(aRequest,parse);	
			if ((r!=KErrNone) || IsIllegalFullName(parse.FullName().Mid(2)))
				return(KErrBadName);
			return(KErrNone);
			}
		else	
			{
			//	Determine whether the name is invalid and return any offending character 	
			TText badChar=' ';
			TPckg<TText> pText(badChar);
				
			TInt r=ParseSubstPtr0(aRequest,parse);
			if (r==KErrBadName)	//	Name is > 256 characters or an illegal character 
				{				//	was encountered before path was completely parsed
				TFileName name;
				aRequest->ReadL(KMsgPtr0,name);
				TNameChecker checkName(name);
				checkName.IsIllegalChar(badChar);								
				aRequest->WriteL(KMsgPtr2,pText);
				return(KErrBadName);
				}
			//	Weed out invalid * or ? in the filename (or no name and extension at all)
			r=ParseNoWildSubstCheckPtr0(aRequest,parse);	
			if (r==KErrBadName)
				{
				if (parse.IsKMatchAny())
					badChar=KMatchAny;		//	Offending character is '*'
				else if (parse.IsKMatchOne())
					badChar=KMatchOne;		//	Offending character is '?'
				aRequest->WriteL(KMsgPtr2,pText);
				return(KErrBadName);
				}

			//	Weed out any other outstanding illegal characters in filename or path	
			TNameChecker checkName(parse.FullName().Mid(2));
			if (checkName.IsIllegalName(badChar))
				{
				aRequest->WriteL(KMsgPtr2,pText);
				return(KErrBadName);
				}
			else					//	No invalid characters in name
				{					//	Set badChar to '\0'		
				badChar='\0';		
				aRequest->WriteL(KMsgPtr2,pText);
				return(KErrNone);	
				}
			}
		}
	else //new API implementation
		{
		RFs::TNameValidParam param;
		TPckg<RFs::TNameValidParam> paramPckg(param);
		aRequest->ReadL(KMsgPtr3,paramPckg);
		TUint nameLen=(TUint)aRequest->GetDesLength(KMsgPtr0);
		if(nameLen == 0)// a name must be specified
			{
			param.iError = RFs::TNameValidParam::ErrBadName;
			param.iInvalidCharPos = 0;
			aRequest->WriteL(KMsgPtr3,paramPckg);
			return KErrBadName;
			}
		if(param.iUseSessionPath)
			nameLen += aRequest->Session()->Path().Length();
		if(nameLen > (TUint)KMaxFileName)
			{
			param.iError = RFs::TNameValidParam::ErrNameTooLong;
			param.iInvalidCharPos = 0;
			aRequest->WriteL(KMsgPtr3,paramPckg);
		   	return KErrBadName;
			}
		TFileName fileName;
		TInt driveLength = 2;
		TText badChar;
		TRAPD(r, aRequest->ReadL(KMsgPtr0,fileName));
		if(KErrNone == r)
			{
			r=ParseSubstPtr0(aRequest,parse,param.iUseSessionPath);
			if (KErrBadName  == r) 	//	An illegal character 
				{					//	was encountered before path was completely parsed
				TNameChecker checkName(fileName);
				if(checkName.IsIllegalChar(badChar))
					{
					param.iError = RFs::TNameValidParam::ErrBadCharacter;
					param.iInvalidCharPos = fileName.LocateReverse(badChar)+1;
					aRequest->WriteL(KMsgPtr3,paramPckg);
					return KErrBadName;
					}
				param.iError = RFs::TNameValidParam::ErrBadName;
				param.iInvalidCharPos = 0;
				aRequest->WriteL(KMsgPtr3,paramPckg);
				return KErrBadName;
				}
			//	Weed out invalid * or ? in the filename
			r=ParseNoWildSubstPtr0(aRequest,parse,param.iUseSessionPath);
			if(KErrBadName == r)
				{
				if(parse.IsWild())	
					{
					if (parse.IsKMatchAny())
						{// offending character is '*'
						param.iError = RFs::TNameValidParam::ErrBadCharacter;
						param.iInvalidCharPos = fileName.LocateReverse(KMatchAny)+1;
						aRequest->WriteL(KMsgPtr3,paramPckg);
						return KErrBadName;
						}
					else if (parse.IsKMatchOne())
						{//offending character is '?'
						param.iError = RFs::TNameValidParam::ErrBadCharacter;
						param.iInvalidCharPos = fileName.LocateReverse(KMatchOne)+1;
						aRequest->WriteL(KMsgPtr3,paramPckg);
			    		return KErrBadName;
						}
					}
				param.iError = RFs::TNameValidParam::ErrBadName;
				param.iInvalidCharPos = 0;
				aRequest->WriteL(KMsgPtr3,paramPckg);
				return KErrBadName;
				}
			if(!param.iUseSessionPath && !parse.DrivePresent())
				driveLength = 0;
			//	Weed out any other outstanding illegal characters in filename or path
			TNameChecker checkName(parse.FullName().Mid(driveLength));
			badChar = ' ';	
			if (checkName.IsIllegalName(badChar))
				{
				if(badChar == ' ')
					{
					param.iError = RFs::TNameValidParam::ErrBadName;
					param.iInvalidCharPos = 0;
					aRequest->WriteL(KMsgPtr3,paramPckg);
					return KErrBadName;
					}
				else
					{
					if(badChar == '.')
						{
						param.iError = RFs::TNameValidParam::ErrBadCharacter;
						param.iInvalidCharPos = fileName.Locate(badChar)+1;
						}
					else
						{
						param.iError = RFs::TNameValidParam::ErrBadCharacter;
						param.iInvalidCharPos = fileName.LocateReverse(badChar)+1;
						}
					aRequest->WriteL(KMsgPtr3,paramPckg);
					return KErrBadName;
					}
				}
			else					//	No invalid characters in name
				{
				param.iError = RFs::TNameValidParam::ErrNone;
				param.iInvalidCharPos = 0;
				aRequest->WriteL(KMsgPtr3,paramPckg);
				return KErrNone;		
				}
			}
		else
			return r;//read is not successful.
		}
	}
				
	
TInt TFsLockDrive::DoRequestL(CFsRequest* aRequest)
//
// Lock media device
//
	{
	__PRINT(_L("TFsLockDrive::DoRequestL(CFsRequest* aRequest)"));	
	TMediaPassword oldPw;
	TMediaPassword newPw;
	aRequest->ReadL(KMsgPtr1,oldPw);
	aRequest->ReadL(KMsgPtr2,newPw);
	return(aRequest->Drive()->LockDevice(oldPw,newPw,aRequest->Message().Int3()));
	}


TInt TFsLockDrive::Initialise(CFsRequest* aRequest)
//
// 
//
	{
	if (!KCapFsLockDrive.CheckPolicy(aRequest->Message(), __PLATSEC_DIAGNOSTIC_STRING("Lock Drive")))
		return KErrPermissionDenied;
	TInt r=ValidateDriveDoSubst(aRequest->Message().Int0(),aRequest);
	return(r);
	}

TInt TFsUnlockDrive::DoRequestL(CFsRequest* aRequest)
//
// Unlock media device
//
	{
	__PRINT(_L("TFsUnlockDrive::DoRequestL(CFsRequest* aRequest)"));
	TMediaPassword pW;
	aRequest->ReadL(KMsgPtr1,pW);
	return(aRequest->Drive()->UnlockDevice(pW,aRequest->Message().Int2()));
	}


TInt TFsUnlockDrive::Initialise(CFsRequest* aRequest)
//
// 
//
	{
	if (!KCapFsUnlockDrive.CheckPolicy(aRequest->Message(), __PLATSEC_DIAGNOSTIC_STRING("Unlock Drive")))
		return KErrPermissionDenied;
	TInt r=ValidateDriveDoSubst(aRequest->Message().Int0(),aRequest);
	return(r);
	}

TInt TFsClearPassword::DoRequestL(CFsRequest* aRequest)
//
// Clears password of media device
//
	{
	__PRINT(_L("TFsClearPassowrd::DoRequestL(CFsRequest* aRequest)"));	
	TMediaPassword pW;
	aRequest->ReadL(KMsgPtr1,pW);
	return(aRequest->Drive()->ClearDevicePassword(pW));
	}


TInt TFsClearPassword::Initialise(CFsRequest* aRequest)
//
//
//
	{
	if (!KCapFsClearPassword.CheckPolicy(aRequest->Message(), __PLATSEC_DIAGNOSTIC_STRING("Clear Password")))
		return KErrPermissionDenied;
	return ValidateDriveDoSubst(aRequest->Message().Int0(),aRequest);
	}

TInt TFsErasePassword::DoRequestL(CFsRequest* aRequest)
//
// Erase the password from the media device
//
	{
	__PRINT(_L("TFsErasePassword::DoRequestL(CFsRequest* aRequest)"));
	return(aRequest->Drive()->EraseDevicePassword());
	}


TInt TFsErasePassword::Initialise(CFsRequest* aRequest)
//
// 
//
	{
	if (!KCapFsErasePassword.CheckPolicy(aRequest->Message(), __PLATSEC_DIAGNOSTIC_STRING("Erase Password")))
		return KErrPermissionDenied;
	return ValidateDriveDoSubst(aRequest->Message().Int0(),aRequest);

	}


TInt TFsSessionToPrivate::DoRequestL(CFsRequest* aRequest)
//
//	Set session path to the private directory (does not need to exist)
//
	{
	TBuf<30> pPath;	
	pPath=_L("?:\\Private\\");	
	pPath[0]= (TText)('A'+ aRequest->Message().Int0());
	pPath.AppendNumFixedWidth(aRequest->Message().SecureId().iId, EHex, 8);
	pPath += KSlash;
	HBufC* pP = pPath.Alloc();
	if (pP==NULL)
		return KErrNoMemory;
	delete &aRequest->Session()->Path();
	aRequest->Session()->SetPath(pP);
	return KErrNone;
	}

	
TInt TFsSessionToPrivate::Initialise(CFsRequest* /*aRequest*/) 
//
//	standard initialisation
//
	{
	return KErrNone;
	}

TInt TFsPrivatePath::DoRequestL(CFsRequest* aRequest)
//
//	may be able to do this user side !!!
//
	{
	TSecureId appUID = aRequest->Message().SecureId();	
	TFileName pPath(KPrivateSlash);	
	pPath.AppendNumFixedWidth(appUID.iId, EHex, 8);
	pPath += KSlash;
	aRequest->WriteL(KMsgPtr0,pPath);

	return KErrNone;	

	}

TInt TFsPrivatePath::Initialise(CFsRequest* /*aRequest*/) 
//
//
//
	{
	return KErrNone;
	}


TInt TFsCreatePrivatePath::DoRequestL(CFsRequest* aRequest)
//
//	create the private path unless it already exists
//
	{
    TInt ret = CheckDiskSpace(KMinFsCreateObjTreshold, aRequest);
    if(ret != KErrNone)
        return ret;

	TBuf<30> pPath(KPrivate);	
	pPath += KSlash;
	pPath.AppendNumFixedWidth(aRequest->Message().SecureId().iId, EHex, 8);
	TEntry e;
	ret=aRequest->Drive()->CheckMount();
	if (ret!=KErrNone)
		return ret;

	//no point going mad on sanity check we know it's not root or a bad name
	ret =aRequest->Drive()->Entry(KPrivate(),e);
	if(ret != KErrNone)
		{
		if(ret==KErrNotFound)
			{
			ret = aRequest->Drive()->MkDir(KPrivate());	
			if(ret != KErrNone)
				return ret;
			}
		else
			return ret;
		}
	ret =aRequest->Drive()->Entry(pPath,e);
	if(ret==KErrPathNotFound || ret==KErrNotFound || !e.IsDir())	
		{	
		if((ret=aRequest->Drive()->MkDir(pPath ))!=KErrNone)
			return ret;			
		}
	return ret;
	}


TInt TFsCreatePrivatePath::Initialise(CFsRequest* aRequest) 
//
//
//
	{
	TInt r=ValidateDriveDoSubst(aRequest->Message().Int0(),aRequest);
	return(r);
	}

TInt TFsFinaliseDrive::DoRequestL(CFsRequest* aRequest)
	{
	__PRINT(_L("TFsFinaliseDrive::DoRequestL"));

	TInt r=aRequest->Drive()->CheckMount();
	if (r!=KErrNone)
		return r;

    const TInt nMode = aRequest->Message().Int1(); //-- finalisation mode, see RFs::TFinaliseDrvMode

	return aRequest->Drive()->FinaliseMount(nMode);
	}

/**
    Finalise the drive
*/
TInt TFsFinaliseDrive::Initialise(CFsRequest* aRequest)
	{
	if (!KCapFsFinaliseDrive.CheckPolicy(aRequest->Message(), __PLATSEC_DIAGNOSTIC_STRING("Finalise Drive")))
		return KErrPermissionDenied;

	const TInt nDrv  = aRequest->Message().Int0(); //-- the drive number to be finalised

    TInt r = ValidateDriveDoSubst(nDrv, aRequest);
    if (r != KErrNone)
        return r;

	TDrive& d = TheDrives[nDrv];
    if(!d.GetFSys())
	    return KErrNotFound; //-- something wrong with this drive 

    
    //-- check the drive attributes
    const TUint KDrvAttExclude = KDriveAttRom | KDriveAttRedirected; //-- the drive attributes to exlcude from the finalisation
    if(d.Att() & KDrvAttExclude) 
        return KErrNotSupported; //-- finalisation isn't supported for these drives

	return KErrNone;
	}

/**  Set System Drive (used by GetSystemDrive and SystemDriveChar) */
TInt TFsSetSystemDrive::DoRequestL(CFsRequest* aRequest)
	{
    __PRINT(_L("TFsSetSystemDrive::DoRequestL()"));
    TInt drive;
	TInt err = RProperty::Get(TSecureId(KFileServerUidValue), KSystemDriveKey, drive);
	if(err == KErrNone)
		{
		__PRINT1(_L("TFsSetSystemDrive::DoRequestL() drv:%d is already defined as system!"), drive);
        return KErrAlreadyExists;
		}
	
    drive = aRequest->Message().Int0();    
	__ASSERT_ALWAYS(drive>=EDriveA && drive<=EDriveZ, Fault(EInvalidDrive));

	//Calculates the total number of valid drives present and set RAM as a
	//system drive only if one valid drive is present.
	TInt total_drive = 0;
	for (TInt driveNumber=0; driveNumber<KMaxDrives; driveNumber++)
		{
		if(TheDrives[driveNumber].GetFSys() != NULL)  // is a valid drive
			total_drive++;
		}
	aRequest->SetDrive(&TheDrives[drive]);
	FsThreadManager::LockDrive(aRequest->DriveNumber());
	TDriveInfo info;
	aRequest->Drive()->DriveInfo(info);
	FsThreadManager::UnlockDrive(aRequest->DriveNumber());

	__PRINT1(_L("TFsSetSystemDrive::DoRequestL() total drives:%d"), total_drive);
    __PRINT3(_L("TFsSetSystemDrive::DoRequestL() setting sys:%d, Media:0x%x, DrvAtt:0x%x"), drive, info.iType, info.iDriveAtt);

    //-- 1. check media type of the system drive.
    __ASSERT_ALWAYS(info.iType==EMediaHardDisk || info.iType==EMediaFlash || info.iType==EMediaNANDFlash || info.iType==EMediaRam, Fault(EInvalidDrive));
    
    if(info.iType == EMediaRam)
		{//-- It is not nice to use RAM drive as a system one because it might non be persistent, but acceptable though. 
         __PRINT(_L("TFsSetSystemDrive::DoRequestL() WARNING: RAM drive is set as system drive!"));
        }
   
    //-- 2. check drive attributes for the system drive
    const TUint requiredDrvAtt   = KDriveAttLocal|KDriveAttInternal;
    const TUint prohibitedDrvAtt = KDriveAttRom|KDriveAttRedirected|KDriveAttSubsted|KDriveAttRemovable;

    ASSERT(!(prohibitedDrvAtt & requiredDrvAtt));
    if( !(info.iDriveAtt & requiredDrvAtt) || (info.iDriveAtt & prohibitedDrvAtt))
        {
        __PRINT2(_L("TFsSetSystemDrive::DoRequestL(), Wrong Drive attributes! req:0x%x, disallowed:0x%x"), requiredDrvAtt, prohibitedDrvAtt);
        Fault(EInvalidDrive);
        }


	RProcess p;
	TSecurityPolicy policy(p.SecureId());
	err = RProperty::Define(TSecureId(KFileServerUidValue), KSystemDriveKey, RProperty::EInt, TSecurityPolicy::EAlwaysPass, policy);
	if(err == KErrNone)
		{
		err = RProperty::Set(TSecureId(KFileServerUidValue), KSystemDriveKey, drive);
		}
	__ASSERT_ALWAYS(err==KErrNone, User::Invariant());
    
    return err;
	}
	
/** Checks if setting System Drive is allowed */
TInt TFsSetSystemDrive::Initialise(CFsRequest* aRequest)
	{
	if (!KCapFsSetSystemDrive.CheckPolicy(aRequest->Message(), __PLATSEC_DIAGNOSTIC_STRING("Set System Drive")))
		{
		return KErrPermissionDenied;
		}
    return KErrNone;
	}
