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
//

#include "cl_std.h"

#ifdef OST_TRACE_COMPILER_IN_USE
#include "cl_fmanTraces.h"
#endif


const TUint KRecurseFlag	=	0x40000000;
const TUint KScanDownFlag	=	0x20000000;
const TUint KFManBusyFlag	=	0x10000000;
const TUint KOverWriteFlag	=	0x08000000;
const TUint KMoveRenameFlag	=	0x04000000;
const TUint KCopyFromHandle	=	0x00000001;

const TInt KPathIncGran=32;

const TUint KMovingFilesMask = KEntryAttMatchExclude | KEntryAttDir;

TInt ShrinkNames(RFs& aFs, TFileName& aParent, TFileName& aItem, TBool aAppend);

LOCAL_C HBufC8* AllocateBuffer(TInt64 aLength)
	{
const TInt KBigBufSize = 512 * 1024;
const TInt KMediumBufSize = 32 * 1024;
const TInt KSmallBufSize = 4 * 1024;
	// Min result shall be of TInt size
	// Hence to suppress warning
	TInt big = (TInt)(Min(aLength,(TInt64)KBigBufSize));
	HBufC8* bufPtr=HBufC8::New(big);
	if (bufPtr==NULL)
		bufPtr=HBufC8::New(KMediumBufSize);
	if (bufPtr==NULL)
		bufPtr=HBufC8::New(KSmallBufSize);
	return(bufPtr);
	}

LOCAL_C TInt IncPathLength(TInt aLen)
	{
	return(((aLen+KPathIncGran-1)/KPathIncGran)*KPathIncGran);
	}

LOCAL_C TInt CreateTargetNameFromSource(TDes& aTrgName, const TDesC& aTrgMask, const TDesC& aSrcName)
// Replace the wildcards with letters from the matched file.
	{
	TFileName destName;
	TParsePtrC trg(aTrgMask);
	TParsePtrC src(aSrcName);
	TPtrC mask(trg.NameAndExt());
	TPtrC source(src.NameAndExt());
	TInt steps = 1;
	TBool starCharFound = EFalse;
	if(mask.LocateReverse('.')!=KErrNotFound || aTrgMask.Right(1)==_L("*"))
		{
		mask.Set(trg.Name());
		source.Set(src.Name());
		steps = 2;
		}
	for(TInt i = 0; i < steps;
	    i++, mask.Set(trg.ExtPresent() ? trg.Ext() : _L(".*")) , source.Set(src.Ext()))
		{
		TInt offset = 0;
		starCharFound = EFalse;
		while(offset < mask.Length())
			{
			TChar currentChar = mask[offset];
			switch(currentChar)
				{
				case KMatchAny:
					if(offset < source.Length() && !starCharFound)
						{
						destName.Append(source.Mid(offset));
						starCharFound = ETrue;
						}
					break;
				case KMatchOne:
					if(offset < source.Length())
				    {
				    destName.Append(source[offset]);
				    }
					break;
				default:
					destName.Append(currentChar);
					break;
				}
			offset++;
			}
		}
	if(destName.Right(1) == _L("."))
		{
		destName.SetLength(destName.Length()-1);
		}
	if(aTrgName.Length()+destName.Length() > KMaxFileName)
		{
		return KErrBadName;
		}
	aTrgName.Append(destName);
	return KErrNone;
	}

EXPORT_C MFileManObserver::TControl MFileManObserver::NotifyFileManStarted()
/**
Inform the observer that an operation is about to start.

This is done immediately before each entry is processed.

@return The implementation of this function should return:
        MFileManObserver::EContinue, to allow processing of the current file
        to proceed;
        MFileManObserver::ECancel, to skip processing the current file and move
        to the next file;
        MFileManObserver::EAbort to abort the entire operation.
        The default return value is MFileManObserver::EContinue.
*/
	{

	return(EContinue);
	}




EXPORT_C MFileManObserver::TControl MFileManObserver::NotifyFileManOperation()
/**
Informs the observer that an operation, i.e. a copy or a move, is proceeding.

Large files are copied and moved in stages.
After each portion of the source file has been copied to the target, this
function is called.

It may be useful to call CFileMan::BytesTransferredByCopyStep() from this
function to retrieve the current status of the operation.

@return The implementation of this function should return:
        MFileManObserver::ECancel, to cancel the current operation, closing the current source
        and target files, the current target file is deleted.
        If the operation is performed on several files, cancelling one will not abort whole batch.

        MFileManObserver::EContinue, to continue with the operation.
        The default return value is MFileManObserver::EContinue.

@see CFileMan
*/
	{

	return(EContinue);
	}




EXPORT_C MFileManObserver::TControl MFileManObserver::NotifyFileManEnded()
/**
Informs the observer that an operation is complete.

This is done immediately after a directory entry has been processed.

It may be useful to call CFileBase::GetLastError()
and CFileBase::GetMoreInfoAboutError() from this function to retrieve
information about how the operation ended.

@return The implementation of this function should return:
        MFileManObserver::EContinue or MFileManObserver::ECancel, to proceed
        with processing the next entry. MFileManObserver::ECancel will not
        cancel processing the current entry;
        MFileManObserver::ERetry, to re-attempt processing the previous file;
        MFileManObserver::EAbort, to abort the entire operation.
        The default return value is MFileManObserver::EContinue.

@see CFileBase
*/
	{

	return(EContinue);
	}




EXPORT_C CFileBase::CFileBase(RFs& aFs)
/**
Protected default constructor.

Note that the class is intended only as an abstract base for other classes.

@param aFs File server session.
*/
	: iFs(aFs)
	{
	}




EXPORT_C void CFileBase::ConstructL()
/**
Second phase constructor.
*/
	{

	iScanner=CDirScan::NewL(iFs);
	User::LeaveIfError(iSynchronizer.CreateLocal(0));
	}




EXPORT_C CFileBase::~CFileBase()
/**
Destructor.

Frees resources prior to destruction of the object.
*/
	{

	delete iScanner;
	delete iDirList;
	iSynchronizer.Close();
	User::Free(iSessionPath);
	}




GLDEF_C void DoFManBaseOperationL(TAny* aPtr)
//
// File manager asynchronous thread
//
	{

	CFileBase& fMan=*(CFileBase*)aPtr;
	User::LeaveIfError(fMan.iFs.Connect());
	User::LeaveIfError(fMan.iFs.SetSessionPath(*fMan.iSessionPath));
	fMan.iNumberOfFilesProcessed = 0;
	fMan.RunL();
	}

GLDEF_C TInt FManBaseThreadFunction(TAny* aPtr)
//
// Initialise New thread
//
	{
	TInt r = KErrNoMemory;	// return value if a trap harness cannot be created
	CTrapCleanup* cleanup=CTrapCleanup::New();
	CFileBase& fMan=*(CFileBase*)aPtr;

	if (cleanup != NULL)
		{
		fMan.iSynchronizer.Wait();
		TRAP(r,DoFManBaseOperationL(aPtr));
		if (r == KErrNone)
			r = fMan.iLastError;
		delete cleanup;
		}

	fMan.iSwitches=0;
	fMan.iFs=fMan.iFsOld;
	fMan.iStatus=NULL;
	fMan.iFManThread.Close();
	return (r);
	}




EXPORT_C void CFileBase::RunInSeparateThreadL(TThreadFunction aThreadFunction)
/**
Creates a separate thread to run the command.

@param aThreadFunction The thread function.
*/
	{
	TFileName sessionPath;
	User::LeaveIfError(iFs.SessionPath(sessionPath));
	if (iSessionPath==NULL)
		iSessionPath=HBufC::NewL((sessionPath.Length()));
	else if (iSessionPath->Des().MaxLength()<sessionPath.Length())
		iSessionPath=iSessionPath->ReAllocL(IncPathLength(sessionPath.Length()));
	iSessionPath->Des()=sessionPath;

	User::LeaveIfError(iFManThread.Create(KNullDesC,aThreadFunction,KDefaultStackSize,NULL,this));

	// The code won't leave anymore after this.
	// The effect of any further state changes to this instance
	// should be undone / completed by the thread function.
	iFManThread.SetPriority(EPriorityMuchLess);
	iSwitches|=KFManBusyFlag;
	iFsOld=iFs;
	iLastError=KErrNone;
	iFManThread.Logon(*iStatus);
	iFManThread.Resume();
	}




EXPORT_C void CFileBase::RunL()
/**
Executes a command.

@capability Dependent the capabilities required by this method, of the abstract class CFileBase,
					  will be dependent on and provided by the concrete-class implementation of
					  the DoOperationL method

*/
	{
	if (iStatus && (iSwitches&KFManBusyFlag)==EFalse)
		{
		RunInSeparateThreadL(FManBaseThreadFunction);
		return;
		}

	TBool copyFromHandle = (iSwitches & KCopyFromHandle)?(TBool)ETrue:(TBool)EFalse;

	CDirScan::TScanDirection scanDir=(iSwitches&KScanDownFlag) ? CDirScan::EScanDownTree : CDirScan::EScanUpTree;

	if (!copyFromHandle)
		{
		TRAP(iLastError,iScanner->SetScanDataL(iSrcFile.FullName(),iMatchEntry,ESortByName|EAscending,scanDir));
		if (iLastError==KErrNone)
			TRAP(iLastError,iScanner->NextL(iDirList));

		if (iLastError!=KErrNone)
			{
			iErrorInfo=EInitializationFailed;
			User::Leave(iLastError);
			}
		}

	FOREVER
		{
		if (copyFromHandle || iDirList->Count())
			{
			iLastError=KErrNone;
			iErrorInfo=ENoExtraInformation;
			TInt action=(iObserver) ? iObserver->NotifyFileManStarted() : MFileManObserver::EContinue;
			// Check if NotifyFileManStarted returned ECancel.
			if ( action == MFileManObserver::ECancel)
				iLastError=KErrCancel;
			if (action==MFileManObserver::EContinue)
				{
				iNumberOfFilesProcessed++;
				TRAP(iLastError,DoOperationL());
				action=(iObserver) ? iObserver->NotifyFileManEnded() : MFileManObserver::EContinue;
				}
			else if(action==MFileManObserver::ERetry)
			  {
			  Panic(EFManBadValueFromObserver);
			  }


			switch(action)
				{
			case MFileManObserver::EContinue:
			case MFileManObserver::ECancel:
				break;
			case MFileManObserver::ERetry:
				continue;
			case MFileManObserver::EAbort:
				delete iDirList;
				iDirList=NULL;
				iCurrentEntry = 0;
				User::Leave(KErrCancel);
			default:
				Panic(EFManBadValueFromObserver);
				}
			}
		iCurrentEntry++;
		if (copyFromHandle || iCurrentEntry>=iDirList->Count())
			{
			delete iDirList;
			iDirList=NULL;
			iCurrentEntry=0;

			if (iSwitches&KRecurseFlag)
				{
				TRAPD(ret,iScanner->NextL(iDirList));
				if (ret!=KErrNone && ret!=KErrPathNotFound)
					{
					iErrorInfo=EScanNextDirectoryFailed;
					iLastError = ret;
					User::Leave(iLastError);
					}
				}
			if (iDirList==NULL)
				{
				CompleteOperationL();
				return;
				}
			}
		}
	}




EXPORT_C void CFileBase::SetObserver(MFileManObserver* anObserver)
/**
Sets the observer.

Use this function to provide CFileMan with an MFileManObserver, or, if one
already exists, to change the observer.

@param anObserver File management observer.

@see CFileMan
@see MFileManObserver
*/
	{

	iObserver=anObserver;
	}




EXPORT_C const TEntry& CFileBase::CurrentEntry()
/**
Gets the entry currently being processed.

@return Contains the current entry.
*/
	{

	__ASSERT_ALWAYS(iDirList && iCurrentEntry>=0 && iCurrentEntry<iDirList->Count(),Panic(EFManCurrentEntryInvalid));
	return (*iDirList)[iCurrentEntry];
	}




EXPORT_C TPtrC CFileBase::AbbreviatedPath()
/**
Gets the abbreviated path of the file or directory currently being processed.

The abbreviated path is its path relative to the top level directory
specified in the operation.

@return The abbreviated path.
*/
	{

	return iScanner->AbbreviatedPath();
	}




EXPORT_C TPtrC CFileBase::FullPath()
/**
Gets the full path of the file or directory currently being processed.

The full path includes the drive letter, path and filename.

@return The full path.
*/
	{

	return iScanner->FullPath();
	}




EXPORT_C TInt CFileBase::GetLastError()
/**
Gets the latest error code returned during a CFileMan
operation.

This function may be called from MFileManObserver::NotifyFileManEnded().

@return KErrNone, or another error code that might have been
        returned by a CFileMan operation.
*/
	{

	return(iLastError);
	}




EXPORT_C TFileManError CFileBase::GetMoreInfoAboutError()
/**
Gets additional information about the latest error returned during
a CFileMan operation.

For example, if a rename fails, this function
can be used to report whether the source or target name caused the problem.
This information supplements that provided GetLastError().

@return The extra information about the last CFileMan error.

@see CFileMan
@see CFileBase::GetLastError()
*/
	{

	return(iErrorInfo);
	}




EXPORT_C CFileMan* CFileMan::NewL(RFs& aFs)
/**
Constructs and allocates memory for a new CFileMan object.

@param aFs File server session.

@return Newly created CFileMan object.
*/
	{
	OstTrace1(TRACE_BORDER, EFSRV_ECFILEMANNEWL1, "sess %x", aFs.Handle());

	CFileMan* fileMan=new(ELeave) CFileMan(aFs);
	CleanupStack::PushL(fileMan);
	fileMan->CFileBase::ConstructL();
	CleanupStack::Pop();

	OstTrace1(TRACE_BORDER, EFSRV_ECFILEMANNEWL1RETURN, "CFileMan* %x", fileMan);
	return fileMan;
	}




EXPORT_C CFileMan* CFileMan::NewL(RFs& aFs,MFileManObserver* anObserver)
/**
Constructs and allocates memory for a new CFileMan object with an observer.

@param aFs        File server session.
@param anObserver File management observer.

@return Newly created CFileMan object.
*/
	{
	OstTraceExt2(TRACE_BORDER, EFSRV_ECFILEMANNEWL2, "sess %x anObserver %x", (TUint) aFs.Handle(), (TUint) anObserver);

	CFileMan* fileMan=new(ELeave) CFileMan(aFs);
	CleanupStack::PushL(fileMan);
	fileMan->CFileBase::ConstructL();
	CleanupStack::Pop();
	fileMan->SetObserver(anObserver);

	OstTrace1(TRACE_BORDER, EFSRV_ECFILEMANNEWL2RETURN, "CFileMan* %x", fileMan);
	return fileMan;
	}




CFileMan::CFileMan(RFs& aFs)
//
// Constructor and destructor
//
	: CFileBase(aFs)
	{
	}
CFileMan::~CFileMan()
	{
	OstTrace1(TRACE_BORDER, EFSRV_ECFILEMANDESTRUCTOR, "this %x", this);

	OstTrace0(TRACE_BORDER, EFSRV_ECFILEMANDESTRUCTORRETURN, "");
	}


EXPORT_C CFileMan::TAction CFileMan::CurrentAction()
/**
Gets the action which CFileMan is currently carrying out.

@return The action which CFileMan is carrying out.
*/
	{
	OstTrace1(TRACE_BORDER, EFSRV_ECFILEMANCURRENTACTION, "this %x", this);

	TAction action = ENone;

	// Mapping table between internal and external action indicators.
	switch(iAction)
		{
	case EInternalNone:
		action = ENone;
		break;
	case EInternalAttribs:
		action = EAttribs;
		break;
	case EInternalCopy:
		action = ECopy;
		break;
	case EInternalCopyForMove:
		action = EMove;
		break;
	case EInternalDelete:
		action = EDelete;
		break;
	case EInternalRenameInvalidEntry:
		action = ERenameInvalidEntry;
		break;
	case EInternalRenameForMove:
	case EInternalRename:
		action = ERename;
		break;
	case EInternalRmDir:
		action = ERmDir;
		break;
	case EInternalCopyFromHandle:
		action = ECopyFromHandle;
		break;
	default:
		Panic(EFManUnknownAction);
		}

	OstTrace1(TRACE_BORDER, EFSRV_ECFILEMANCURRENTACTIONRETURN, "action %d", action);
	return (action);
	}




EXPORT_C void CFileMan::GetCurrentTarget(TFileName& aTrgName)
/**
Gets the name of the target file for the CFileMan operation currently
being carried out.

This function is relevant when copying, moving or renaming files.

@param aTrgName The full path and filename of the target file for
                the current CFileMan operation
*/
	{
	OstTrace1(TRACE_BORDER, EFSRV_ECFILEMANGETCURRENTTARGET, "this %x", this);

	GetSrcAndTrg(iTmpParse, aTrgName);

	OstTrace0(TRACE_BORDER, EFSRV_ECFILEMANGETCURRENTTARGETRETURN, "");
	OstTraceData(TRACE_BORDER, EFSRV_ECFILEMANGETCURRENTTARGET_EFILENAME, "FileName %S", aTrgName.Ptr(), aTrgName.Length()<<1);
	}




EXPORT_C void CFileMan::GetCurrentSource(TFileName& aSrcName)
/**
Gets the name of the source file or directory for the CFileMan operation
currently being carried out.

The source is the file or directory which is being copied, moved or deleted.

@param aSrcName The full path and filename of the source file for the current
                CFileMan operation.
*/
	{
	OstTrace1(TRACE_BORDER, EFSRV_ECFILEMANGETCURRENTSOURCE, "this %x", this);

    TPtrC fullPath(FullPath());
	iTmpParse.Set(CurrentEntry().iName, &fullPath, NULL);
	aSrcName = iTmpParse.FullName();

	OstTrace0(TRACE_BORDER, EFSRV_ECFILEMANGETCURRENTSOURCERETURN, "");
	OstTraceData(TRACE_BORDER, EFSRV_ECFILEMANGETCURRENTSOURCE_EFILENAME, "FileName %S", aSrcName.Ptr(), aSrcName.Length()<<1);
	}

void CFileMan::GetSrcAndTrg(TParse& aSrcName,TFileName& aTrgName)
//
// Get the current target for the operation
//
	{
	TFileName fullpath = FullPath();
	TInt ret = aSrcName.Set(CurrentEntry().iName, &fullpath, NULL);
	if(ret == KErrBadName)
		{
		// Try heap variables first
		TBool done = EFalse;
		TFileName* current = new TFileName;
		if (current != NULL)
			{
			current->Copy(CurrentEntry().iName);

			ret = ShrinkNames(iFs, fullpath, *current, EFalse);
			if(ret == KErrNone)
				{
				ret = aSrcName.Set(*current, &fullpath, NULL);
				done = ETrue;
				}
			delete current;
			}

		if (!done) //heap method failed
			{
			TFileName current = CurrentEntry().iName;
			ret = ShrinkNames(iFs, fullpath, current, EFalse);
			if(ret == KErrNone)
				{
				ret = aSrcName.Set(current, &fullpath, NULL);
				}
			}
		}
	__ASSERT_DEBUG(ret == KErrNone, Panic(EBadLength));
	aTrgName=iTrgFile.DriveAndPath();
	TPtrC relPath=iScanner->AbbreviatedPath();
	aTrgName.Append(relPath.Right(relPath.Length()-1));
	CreateTargetNameFromSource(aTrgName,iTrgFile.NameAndExt(),aSrcName.NameAndExt());
	}




EXPORT_C TInt CFileMan::BytesTransferredByCopyStep()
/**
Gets the number of bytes transferred during a copy or move operation.

Large files are copied and moved in stages. After each portion of
the source file has been copied to the target, the number of bytes
transferred is updated. This function may be called
from MFileManObserver::NotifyFileManOperation()
and may be used to support the increment of progress bars.

@return The number of bytes transferred.
*/
	{
	OstTraceExt2(TRACE_BORDER, EFSRV_ECFILEMANBYTESTRANSFERREDBYCOPYSTEP, "this %x BytesTransferred %d", (TUint) this, (TUint) iBytesTransferred);

	return(iBytesTransferred);
	}




LOCAL_C void MakeParseWild(TParse& aParse, TFileName& aName)
//
// Append _L("\\*") or _L("*") to aParse
//
	{
	if(!aParse.IsWild())
		{
		aName = aParse.FullName();
		if (aParse.NamePresent() || aParse.ExtPresent())
			{
			if (aName.Length()<=254)
				aName.Append(_L("\\*"));
			}
		else
			{
			if (aName.Length()<=255)
				aName.Append(_L("*"));
			}
		aParse.Set(aName,NULL,NULL);
		}
	}


void CFileMan::CheckForDirectory()
//
// If iTrgFile is a directory set target to iTrgFile\\*
//
	{
	TInt trg = iFs.Entry(iTrgFile.FullName(), iTmpEntry);
	if (trg==KErrNone && iTmpEntry.iAtt&KEntryAttDir)
		MakeParseWild(iTrgFile, iTmpName1);
	TInt src = iFs.Entry(iSrcFile.FullName(), iTmpEntry);
	if (src==KErrNone && iTmpEntry.iAtt&KEntryAttDir)
		{
		MakeParseWild(iSrcFile, iTmpName1);
		if (trg==KErrNotFound && (iSwitches&KRecurseFlag))
			MakeParseWild(iTrgFile, iTmpName1);
		}
	}

void CFileMan::DoSynchronize(TInt aRetValue)
//
// Synchronise with fmanthread
//
	{

	if (iStatus && aRetValue==KErrNone)
		iSynchronizer.Signal(); // FManThread started
	if (iStatus && aRetValue!=KErrNone)
		iStatus=NULL; // FManThread failed to start
	}

LOCAL_C void NextInPath(const TDesC& aPath,TPtrC& anEntry,TInt& aPos)
//
// Returns the next entry in the path
//
	{

	anEntry.Set(NULL,0);
	if ((aPos+1)>=aPath.Length())
		return;
	TPtrC path(aPath.Mid(aPos+1)); // Skip delimiter
	TInt delimiterPos=path.Locate(KPathDelimiter);
	if (delimiterPos==KErrNotFound)
		delimiterPos=aPath.Length()-(aPos+1);
	if (delimiterPos<=0)
		return;

	if (path[delimiterPos-1]==KExtDelimiter) // return "F32." as "F32"
		anEntry.Set(aPath.Mid(aPos+1,delimiterPos-1));
	else
		anEntry.Set(aPath.Mid(aPos+1,delimiterPos));
	aPos+=delimiterPos+1;
	}

LOCAL_C TBool ComparePaths(const TDesC& aPath1,const TDesC& aPath2)
//
// Return ETrue if the paths are identical
// To catch case "\\F32.\\GROUP\\" == "\\F32\\GROUP\\"
//
	{

	TPtrC entry1(NULL,0);
	TPtrC entry2(NULL,0);
	TInt pos1=0;
	TInt pos2=0;

	do {
		NextInPath(aPath1,entry1,pos1);
		NextInPath(aPath2,entry2,pos2);
		if (entry1.MatchF(entry2)==KErrNotFound)
			return(EFalse);
		} while (entry1.Length() && entry2.Length());

	return(ETrue);
	}

EXPORT_C TBool FileNamesIdentical(const TDesC& aFileName1,const TDesC& aFileName2)
//
// Return ETrue if the filenames (and paths) are identical
// NB "Agenda" == "AGENda."
//
	{

	TParsePtrC file1(aFileName1);
	TParsePtrC file2(aFileName2);
	if (file1.Drive().MatchF(file2.Drive())==KErrNotFound)
		return(EFalse);
	if (file1.Name().MatchF(file2.Name())==KErrNotFound)
		return(EFalse);
	if (ComparePaths(file1.Path(),file2.Path())==EFalse)
		return(EFalse);
	if (file1.Ext().Length()==0 || file2.Ext().Length()==0)
		{ // Agenda == Agenda.
		if (file1.Ext().Length()==1 || file2.Ext().Length()==1)
			return(ETrue);
		}
	if (file1.Ext().MatchF(file2.Ext())==KErrNotFound &&
		file1.NameAndExt().MatchF(file2.NameAndExt())==KErrNotFound)
		return(EFalse);
	return(ETrue);
	}




EXPORT_C TInt CFileMan::Attribs(const TDesC& aName,TUint aSetMask,TUint aClearMask,const TTime& aTime,TUint aSwitches,TRequestStatus& aStatus)
/**
Sets or clears attributes for one or more files using two bitmasks.

This is an asynchronous function.
Its behaviour is the same as the synchronous overload.

@param aName      Path indicating the file(s) whose attributes are to be
                  changed. Any path components which are not specified
                  here will be taken from the session path.
                  Use wildcards to specify more than one file.
@param aSetMask   Bitmask indicating the attributes to be set.
@param aClearMask Bitmask indicating the attributes to be cleared.
                  For more information, see KEntryAttNormal and the other
                  file/directory attributes.
@param aTime      Contains the new modification date and time for the files, in UTC.
                  To preserve the file's timestamps, specify a TTime value of 0.
@param aSwitches  Specify zero for no recursion;
                  CFileMan::ERecurse for recursion.
                  By default, the synchronous variant of this function operates
                  non-recursively.
@param aStatus    The request status object. On request completion,
                  indicates how the request completed:
                  KErrNone, if successful, otherwise one of the other system-wide error
                  codes.

@return KErrNone if the asynchronous request is made successfully; KErrInUse if an asynchronous request
					is still pending; otherwise one of the other system-wide error codes

@capability Dependent If aName is /Sys then Tcb capability is required.
@capability Dependent If aName begins with /Private and does not match
					  this process' SID then AllFiles capability is required.
@capability Dependent If aName is /Resource then Tcb capability is required.

*/
	{
	OstTraceExt5(TRACE_BORDER, EFSRV_ECFILEMANATTRIBS1A, "this %x aSetMask %x aClearMask %x aSwitches %d status %x", (TUint) this, (TUint) aSetMask, (TUint) aClearMask, (TUint) aSwitches, (TUint) &aStatus);
	OstTraceExt2(TRACE_BORDER, EFSRV_ECFILEMANATTRIBS1B, "aTime %x:%x ", (TUint) I64HIGH(aTime.Int64()), (TUint) I64LOW(aTime.Int64()));
	OstTraceData(TRACE_BORDER, EFSRV_ECFILEMANATTRIBS1A_EFILEPATH, "FilePath %S", aName.Ptr(), aName.Length()<<1);

	TInt r;
	if (iSwitches&KFManBusyFlag)
		{
		r = KErrInUse;
		}
	else
		{
		iStatus=&aStatus;
		r = Attribs(aName,aSetMask,aClearMask,aTime,aSwitches);
		}

	OstTrace1(TRACE_BORDER, EFSRV_ECFILEMANATTRIBS1RETURN, "r %d", r);

	return r;
	}




EXPORT_C TInt CFileMan::Attribs(const TDesC& aName,TUint aSetMask,TUint aClearMask,const TTime& aTime,TUint aSwitches)
/**
Sets or clears attributes for one or more files using two bitmasks

This is a synchronous function.

The first bitmask specifies the attributes to be set.
The second specifies the attributes to be cleared.
The date and time of the files' last modification can also be changed.

The function can operate recursively or non-recursively.
When operating non-recursively, only the matching files located in the directory
specified in aName are affected. When operating recursively, all matching files
in the directory hierarchy below the directory specified in aName will be affected.

Notes:

1. A panic is raised if any attribute is specified in both bitmasks.

2. Attempting to change the attributes for an open file results in an error
   for that file, as retrieved by CFileBase::GetLastError().

3. An attempt to set or clear the KEntryAttDir or KEntryAttVolume attribute
   for a file or directory will have no effect.

@param aName      Path indicating the file(s) whose attributes are to be
                  changed. Any path components which are not specified
                  here will be taken from the session path.
                  Use wildcards to specify more than one file.
@param aSetMask   Bitmask indicating the attributes to be set.
@param aClearMask Bitmask indicating the attributes to be cleared.
                  For more information, see KEntryAttNormal and the other
                  file/directory attributes.
@param aTime      Contains the new modification date and time for the files, in UTC.
                  To preserve the file's timestamps, specify a TTime value of 0.
@param aSwitches  Specify zero for no recursion;
                  CFileMan::ERecurse for recursion.
                  By default, the synchronous variant of this function operates
                  non-recursively.

@return KErrNone if successful, otherwise one of the other system-wide error codes.

@capability Dependent If aName is /Sys then Tcb capability is required.
@capability Dependent If aName begins with /Private and does not match
					  this process' SID then AllFiles capability is required.
@capability Dependent If aName is /Resource then Tcb capability is required.

*/
	{
	OstTraceExt4(TRACE_BORDER, EFSRV_ECFILEMANATTRIBS2A, "this %x aSetMask %x aClearMask %x aSwitches %x", (TUint) this, (TUint) aSetMask, (TUint) aClearMask, (TUint) aSwitches);
	OstTraceExt2(TRACE_BORDER, EFSRV_ECFILEMANATTRIBS2B, "aTime %x:%x ", (TUint) I64HIGH(aTime.Int64()), (TUint) I64LOW(aTime.Int64()));
	OstTraceData(TRACE_BORDER, EFSRV_ECFILEMANATTRIBS2A_EFILEPATH, "FilePath %S", aName.Ptr(), aName.Length()<<1);

	TInt ret;
	if (iSwitches&KFManBusyFlag)
		{
		ret = KErrInUse;
		}
	else
		{
		SetFlags(aSwitches&EOverWrite,aSwitches&ERecurse,ETrue,EFalse);
		TInt r;
		if ((r = iFs.Parse(aName,iSrcFile)) != KErrNone)
			{
			if(iStatus)
				User::RequestComplete(iStatus,r);
			OstTrace1(TRACE_BORDER, EFSRV_ECFILEMANATTRIBS2RETURN1, "r %d", r);
			return r;
			}

		iSetMask=aSetMask;
		iClearMask=aClearMask;
		iTime=aTime;
		iAction = EInternalAttribs;
		iMatchEntry=KEntryAttMaskSupported; // all entries
		iNumberOfFilesProcessed = 0;
		TRAP(r,RunL());
		ret=(r==KErrNone) ? iLastError : r;
		DoSynchronize(r);
		}

	OstTrace1(TRACE_BORDER, EFSRV_ECFILEMANATTRIBS2RETURN2, "r %d", ret);

	return(ret);
	}




EXPORT_C TInt CFileMan::Copy(const TDesC& anOld,const TDesC& aNew,TUint aSwitches,TRequestStatus& aStatus)
/**
Copies one or more files.

This is an asynchronous function.
Its behaviour is the same as the synchronous overload.

@param anOld     Path indicating the file(s) to be copied.
                 Any path components which are not specified here will be
                 taken from the session path.
@param aNew      Path indicating the directory into which the file(s) are to be copied.
				 Any path components which are not specified here will be
                 taken from the session path
@param aSwitches Specify zero for no overwriting and no recursion;
                 CFileMan::EOverWrite to overwrite files with the same name;
                 CFileMan::ERecurse for recursion.
                 By default, the synchronous variant of this function operates
                 non-recursively and with overwriting.
@param aStatus   The request status object. On request completion,
                 indicates how the request completed:
                 KErrNone, if successful, otherwise one of the other system-wide error
                 codes.

@return KErrNone if the asynchronous request is made successfully; KErrInUse if an asynchronous request
					is still pending; otherwise one of the other system-wide error codes

@capability AllFiles

@capability Dependent If the path for aNew begins with /Sys then Tcb capability is required.
@capability Dependent If the path for aNew begins with /Resource then Tcb capability is required

*/
	{
	OstTraceExt3(TRACE_BORDER, EFSRV_ECFILEMANCOPY1, "this %x aSwitches %x status %x", (TUint) this, (TUint) aSwitches, (TUint) &aStatus);
	OstTraceData(TRACE_BORDER, EFSRV_ECFILEMANCOPY1_EOLDNAME, "OldName %S", anOld.Ptr(), anOld.Length()<<1);
	OstTraceData(TRACE_BORDER, EFSRV_ECFILEMANCOPY1_ENEWNAME, "NewName %S", aNew.Ptr(), aNew.Length()<<1);

	TInt r;
	if (iSwitches&KFManBusyFlag)
		r = KErrInUse;
	else
		{
		iStatus=&aStatus;
		r = Copy(anOld,aNew,aSwitches);
		}

	OstTrace1(TRACE_BORDER, EFSRV_ECFILEMANCOPY1RETURN, "r %d", r);

	return(r);
	}




EXPORT_C TInt CFileMan::Copy(const TDesC& anOld,const TDesC& aNew,TUint aSwitches)
/**
Copies one or more files.

This is a synchronous function.

NB the following applies to files greater than or equal to 2GBytes in size
(2,147,483,648 bytes) :
- Only files smaller than 2GBytes will be copied; any larger files will be skipped and
processing will continue with the next file.
- If at least one file is smaller than 2GBytes, then KErrNone will be returned.
- If all files are greater than or equal to 2GBytes ,then KErrTooBig will be returned.

One way to detect the presence of any large file(s) is to use an observer: calling
CFileBase::GetLastError() from MFileManObserver::NotifyFileManEnded() will return
KErrToBig for any file >= 2GBytes in size.

Note: the copy operation behaves differently when MFileManObserver is used.
MFileManObserver should be used with multiple files as it enables you to capture the results of all file copy operations.

If MFileManObserver is NOT used then only the result of the last
file copy operation is returned because the results of previous file copy operations are overwritten.

Optionally, this function can be set to overwrite any files with the same name
which exist in the target directory. If the flag is set for no overwriting,
then any files with the same name will not be overwritten, and an error
(KErrAlreadyExists) will be returned for that file. Error codes may be retrieved
using CFileBase::GetLastError().

If recursive operation is set, all intermediate directories will be created,
including any directories in the path specified by aNew which do not
already exist.

If the source (anOld) is a FILE and the recursive operation is set,
then all the files with the same name as anOld in the source directory
including those in subdirectories will be copied to the destination.

For example, the initial directory structure is as follows:
C:\dir1\file.txt
C:\dir1\subdirA\file.txt
C:\dir1\subdirB\file.txt

@code
CFileMan* fm(CFileMan::NewL(iFs));	// Where iFs is an RFs handle
fm->Copy(_L("C:\\dir1\\file.txt"), _L("C:\\dir2\\file.txt"), CFileMan::ERecurse);
// OR without specifying the filename in aNew:
fm->Copy(_L("C:\\dir1\\file.txt"), _L("C:\\dir2\\"), CFileMan::ERecurse);
@endcode

Because of the recursive behaviour, the final directory structure after
either one of the copy operations above will be as follows:
C:\dir1\file.txt
C:\dir1\subdirA\file.txt
C:\dir1\subdirB\file.txt
C:\dir2\file.txt
C:\dir2\subdirA\file.txt
C:\dir2\subdirB\file.txt

If recursive operation is not set, only the matching files located in
the single directory specified in anOld are copied.
No intermediate directories will be created; if any directories in
the destination path do not exist, no files will be copied, and this function
will return KErrPathNotFound.

 Notes:
 1.	This function operates on files only, therefore:
 1.1	In contrast to the way CFileMan::Move() and CFileMan::Rename()
 	behave, the behaviour of the copy operation does not depend on the presence
 	or absence of a trailing backslash ("\") character. Therefore it is only
 	possible to copy the content of the source path. It is NOT
 	possible by use of a trailing backslash ("\") character to request that the
 	last directory level plus its content be copied to the target path.

 	This means that the following two example copy operations will behave
 	identically

 	@code
 	CFileMan* fm(CFileMan::NewL(iFs)); // Where iFs is an RFs handle
 	...
 	fm->Copy(_L("C:\\SRC\\"), _L("C:\\TRG\\"), CFileMan::ERecurse);
 	fm->Copy(_L("C:\\SRC"), _L("C:\\TRG\\"), CFileMan::ERecurse);
 	@endcode

 	because they will be interpreted as follows:
 	@code
 	fm->Copy(_L("C:\\SRC\\*"),_L("C:\\TRG\\"), CFileMan::ERecurse);
 	@endcode

 1.2	If there is no file to operate on i.e. if source directory is empty, the
 	function will do nothing and return error code KErrNotFound.

 2.	Files can be copied across drives.

 3.	Open files can be copied if they have been opened using
	the EFileShareReadersOnly file share mode.

 4.	Read-only, hidden and system files can be copied and
	the source file's attributes are preserved in the target file.

@param anOld     Path indicating the file(s) to be copied.
                 Any path components which are not specified here will be
                 taken from the session path.
@param aNew      Path indicating the directory into which the file(s) are to be copied.
				 Any path components which are not specified here will be
                 taken from the session path
@param aSwitches Specify zero for no overwriting and no recursion;
                 CFileMan::EOverWrite to overwrite files with the same name;
                 CFileMan::ERecurse for recursion.
                 By default, the synchronous variant of this function operates
                 non-recursively and with overwriting.

@return KErrNone if successful, KErrNotFound if source directory is empty, otherwise one of the other system-wide error codes.

@see CFileBase::GetLastError()

@capability AllFiles

@capability Dependent If the path for anOld begins with /Sys then Tcb capability is required.
@capability Dependent If the path for anOld begins with /Resource then Tcb capability is required

*/
	{
	OstTraceExt2(TRACE_BORDER, EFSRV_ECFILEMANCOPY2, "this %x aSwitches %d", (TUint) this, (TUint) aSwitches);
	OstTraceData(TRACE_BORDER, EFSRV_ECFILEMANCOPY2_EOLDNAME, "OldName %S", anOld.Ptr(), anOld.Length()<<1);
	OstTraceData(TRACE_BORDER, EFSRV_ECFILEMANCOPY2_ENEWNAME, "NewName %S", aNew.Ptr(), aNew.Length()<<1);

	if (iSwitches&KFManBusyFlag)
		{
		OstTrace1(TRACE_BORDER, EFSRV_ECFILEMANCOPY2RETURN1, "r %d", KErrInUse);
		return(KErrInUse);
		}
	SetFlags(aSwitches&EOverWrite,aSwitches&ERecurse,ETrue,EFalse);
	TInt r;
	if ((r = iFs.Parse(anOld,iSrcFile)) != KErrNone)
		{
		if(iStatus)
			User::RequestComplete(iStatus,r);
		OstTrace1(TRACE_BORDER, EFSRV_ECFILEMANCOPY2RETURN2, "r %d", r);
		return r;
		}

	if ((r = iFs.Parse(aNew,_L("*"),iTrgFile)) != KErrNone)
		{
		if(iStatus)
			User::RequestComplete(iStatus,r);
		OstTrace1(TRACE_BORDER, EFSRV_ECFILEMANCOPY2RETURN3, "r %d", r);
		return r;
		}

	CheckForDirectory();

	if((iSwitches&KRecurseFlag) && iTrgFile.DriveAndPath().MatchF(iSrcFile.FullName()) != KErrNotFound)
		{
		OstTrace1(TRACE_BORDER, EFSRV_ECFILEMANCOPY2RETURN4, "r %d", KErrArgument);
		return(KErrArgument);
		}

	iAction = EInternalCopy;
	iMatchEntry=KEntryAttMaskSupported;
	iNumberOfFilesProcessed = 0;
	TRAP(r,RunL());
	TInt ret=(r==KErrNone) ? iLastError : r;
	DoSynchronize(r);

	OstTrace1(TRACE_BORDER, EFSRV_ECFILEMANCOPY2RETURN5, "r %d", ret);

	return(ret);
	}




EXPORT_C TInt CFileMan::Delete(const TDesC& aName,TUint aSwitches,TRequestStatus& aStatus)
/**
Deletes one or more files.

This is an asynchronous function.
Its behaviour is the same as the synchronous overload.

@param aName     Path indicating the file(s) to be deleted.
                 May either be a full path, or relative to the session path.
                 Use wildcards to specify more than one file.
                 NOTE: if you pass KNullDesC, the empty (or null) descriptor,
                 then the function interprets this to mean \\*.*
@param aSwitches Specify:
                 zero for no recursion;
                 CFileMan::ERecurse for recursion.
                 By default, the synchronous variant of this function
                 operates non-recursively.
@param aStatus   The request status object. On request completion,
                 indicates how the request completed:
                 KErrNone, if successful, otherwise one of the other system-wide error
                 codes.

@return KErrNone if the asynchronous request is made successfully; KErrInUse if an asynchronous request
					is still pending; otherwise one of the other system-wide error codes

@capability Dependent If aName is /Sys then Tcb capability is required.
@capability Dependent If aName begins with /Private and does not match this process' SID
					  then AllFiles capability is required.
@capability Dependent If aName is /Resource then Tcb capability is required.

@see KNullDesC
*/
	{
	OstTraceExt3(TRACE_BORDER, EFSRV_ECFILEMANDELETE1, "this %x aSwitches %x status %x", (TUint) this, (TUint) aSwitches, (TUint) &aStatus);
	OstTraceData(TRACE_BORDER, EFSRV_ECFILEMANDELETE1_EFILEPATH, "FilePath %S", aName.Ptr(), aName.Length()<<1);

	TInt r;
	if (iSwitches&KFManBusyFlag)
		{
		r = KErrInUse;
		}
	else
		{
		iStatus=&aStatus;
		r = Delete(aName,aSwitches);
		}

	OstTrace1(TRACE_BORDER, EFSRV_ECFILEMANDELETE1RETURN, "r %d", r);

	return(r);
	}




EXPORT_C TInt CFileMan::Delete(const TDesC& aName,TUint aSwitches)
/**
Deletes one or more files.

This is a synchronous function.

This function can operate recursively or non-recursively.
When operating non-recursively, only the matching files located in
the directory specified in aName are affected.
When operating recursively, all matching files in the directory hierarchy
below the directory specified in aName will be deleted.

Note that read-only and open files may not be deleted.
Attempting to do so will return an error for that file.
Error codes may be retrieved using CFileBase::GetLastError().

@param aName     Path indicating the file(s) to be deleted.
                 May either be a full path, or relative to the session path.
                 Use wildcards to specify more than one file.
                 NOTE: if you pass KNullDesC, the empty (or null) descriptor,
                 then the function interprets this to mean \\*.*
@param aSwitches Specify:
                 zero for no recursion;
                 CFileMan::ERecurse for recursion.
                 By default, the synchronous variant of this function
                 operates non-recursively.

@return KErrNone if successful, otherwise one of the other system-wide error
        codes.

@see CFileBase::GetLastError()

@capability Dependent If aName is /Sys then Tcb capability is required.
@capability Dependent If aName begins with /Private and does not match this process' SID
					  then AllFiles capability is required.
@capability Dependent If aName is /Resource then Tcb capability is required.

@see KNullDesC
*/
	{
	OstTraceExt2(TRACE_BORDER, EFSRV_ECFILEMANDELETE2, "this %x aSwitches %d", (TUint) this, (TUint) aSwitches);
	OstTraceData(TRACE_BORDER, EFSRV_ECFILEMANDELETE2_EFILEPATH, "FilePath %S", aName.Ptr(), aName.Length()<<1);

	TInt ret;
	if (iSwitches&KFManBusyFlag)
		{
		ret = KErrInUse;
		}
	else
		{
		SetFlags(aSwitches&EOverWrite,aSwitches&ERecurse,ETrue,EFalse);
		TInt r;
		if ((r = iFs.Parse(aName,iSrcFile)) != KErrNone)
			{
			if(iStatus)
				User::RequestComplete(iStatus,r);
			OstTrace1(TRACE_BORDER, EFSRV_ECFILEMANDELETE2RETURN1, "() r %d", r);
			return r;
			}

		iAction = EInternalDelete;
		iMatchEntry=KEntryAttHidden|KEntryAttMatchExclude|KEntryAttDir;
	//	Exclude directories and system files - include hidden files
		iNumberOfFilesProcessed = 0;
		TRAP(r,RunL());
		ret=(r==KErrNone) ? iLastError : r;
		DoSynchronize(r);
		}

	OstTrace1(TRACE_BORDER, EFSRV_ECFILEMANDELETE2RETURN2, "() r %d", ret);

	return(ret);
	}




EXPORT_C TInt CFileMan::Move(const TDesC& anOld,const TDesC& aNew,TUint aSwitches,TRequestStatus& aStatus)
/**
Moves one or more files.

This is an asynchronous function.
Its behaviour is the same as the synchronous overload.

@param anOld     Path indicating the files to be moved. May be either
                 a full path, or relative to the session path. Any path
				 components which are not specified here will be taken
				 from the session path.
@param aNew      Path indicating the directory into which the file(s) are
                 to be moved. Any path components which are not specified
                 here will be taken from the session path.
@param aSwitches Specify zero for no overwriting and no recursion;
                 CFileMan::EOverWrite to overwrite files with the same name;
                 CFileMan::ERecurse for recursion.
                 By default, the synchronous variant of this function operates
                 non-recursively and with overwriting.
@param aStatus   The request status object. On request completion,
                 indicates how the request completed:
                 KErrNone, if successful, otherwise one of the other system-wide error
                 codes.

@return KErrNone if the asynchronous request is made successfully; KErrInUse if an asynchronous request
					is still pending; otherwise one of the other system-wide error codes

@capability Dependent If the path in aNew starts with /Sys then capability Tcb is required
@capability Dependent If the path in aNew starts with /Resource then capability Tcb is required

@capability AllFiles

@capability Dependent If the path in anOld starts with /Sys then Tcb capability is required.
@capability Dependent If the path in anOld starts with /Resource then Tcb capability is required.

*/
	{
	OstTraceExt3(TRACE_BORDER, EFSRV_ECFILEMANMOVE1, "this %x aSwitches %x status %x", (TUint) this, (TUint) aSwitches, (TUint) &aStatus);
	OstTraceData(TRACE_BORDER, EFSRV_ECFILEMANMOVE1_EOLDNAME, "OldName %S", anOld.Ptr(), anOld.Length()<<1);
	OstTraceData(TRACE_BORDER, EFSRV_ECFILEMANMOVE1_ENEWNAME, "NewName %S", aNew.Ptr(), aNew.Length()<<1);

	TInt r;
	if (iSwitches&KFManBusyFlag)
		{
		r = KErrInUse;
		}
	else
		{
		iStatus=&aStatus;
		r = Move(anOld,aNew,aSwitches);
		}

	OstTrace1(TRACE_BORDER, EFSRV_ECFILEMANMOVE1RETURN, "r %d", r);

	return r;
	}




EXPORT_C TInt CFileMan::Move(const TDesC& anOld,const TDesC& aNew,TUint aSwitches)
/**
Moves one or more files.

This is a synchronous function.

Optionally, this function can be set to overwrite any files with the same name
which exist in the target directory. If the flag is set for no overwriting,
then any files with the same name will not be overwritten, and
an error (KErrAlreadyExists) will be returned for that file.
Error codes may be retrieved using CFileBase::GetLastError().
By default, when the function is operating synchronously, files are overwritten.

When this function is operating recursively, all intermediate directories will
be created, including all directories in the destination path specified
by aNew which do not already exist.

If recursive operation is not set, only the matching files located in
the single directory specified in anOld are moved. No intermediate directories
will be created; if any directories in the destination path do not exist,
no files will be moved, and this function will return KErrPathNotFound.

The behaviour of the move operation is sensitive to the presence (or absence)
of a trailing backslash ("\") character on the end of the source path:
- if there is a trailing backslash ("\") character, then the operation moves
  the content of the last directory level only.
- if there is no trailing backslash ("\") character, then the operation behaves
  recursively by default and moves both the last directory level and all of its content.
  Notice that no trailing backslash ("\") implies moving files recursively automatically.

For example, if the directory level "b" contains the files F1, F2 and F3, then:
@code
CFileMan* fm(CFileMan::NewL(iFs)); // Where iFs is an RFs handle
...
fm->Move(_L("C:\\a\\b\\"), _L("C:\\x\\y\\"), CFileMan::ERecurse);
@endcode

results in files F1, F2 and F3 being moved from C:\\a\\b to C:\\x\\y, leaving the
path C:\\a\\b unchanged, except that it no longer contains the files
F1, F2 and F3.

If there is no trailing backslash character, for example:
@code
CFileMan* fm(CFileMan::NewL(iFs)); // Where iFs is an RFs handle
...
fm->Move(_L("C:\\a\\b"), _L("C:\\x\\y\\"), CFileMan::ERecurse);
@endcode

then both the directory level "b" and its contents are moved. This means that
there is no longer a directory "b" under C:\\a. Instead there is a new
directory structure C:\\x\\y\\b and the files F1, F2, and F3 now exist
under C:\\x\\y\\b. Also if "b" contains subdirectories, then these are also
moved along with "b".

If there is no trailing backslash character and the switch is not set, i.e.
0 is passed as an argument, the operation behaves the same way as by passing
CFileMan::ERecurse flag.

For example:
@code
CFileMan* fm(CFileMan::NewL(iFs)); // Where iFs is an RFs handle
...
fm->Move(_L("C:\\a\\b"), _L("C:\\x\\y\\"), 0);
@endcode

The example above produces the same output as:

@code
CFileMan* fm(CFileMan::NewL(iFs)); // Where iFs is an RFs handle
...
fm->Move(_L("C:\\a\\b"), _L("C:\\x\\y\\"), CFileMan::ERecurse);
@endcode

If the source (anOld) is a FILE and the recursive operation is set,
then all the files with the same name as anOld in the source directory
including those in subdirectories will be moved to the destination.

For example, the initial directory structure is as follows:
C:\src\file.txt
C:\src\subdirA\file.txt
C:\src\subdirB\file.txt

@code
CFileMan* fm(CFileMan::NewL(iFs));	// Where iFs is an RFs handle
fm->Move(_L("C:\\src\\file.txt"), _L("C:\\dest\\file.txt"), CFileMan::ERecurse);
// OR without specifying the filename in aNew:
fm->Move(_L("C:\\src\\file.txt"), _L("C:\\dest\\"), CFileMan::ERecurse);
@endcode

Because of the recursive behaviour, the final directory structure after
either one of the move operations above will be as follows:
C:\src\
C:\src\subdirA\
C:\src\subdirB\
C:\dest\file.txt
C:\dest\subdirA\file.txt
C:\dest\subdirB\file.txt

Notes:

-# Read-only, hidden and system files can be moved and the source file's
   attributes are preserved in the target file, but open files cannot
   be moved. Attempting to move an open file will return an error for
   that file, as retrieved by CFileBase::GetLastError().

@param anOld	 Path indicating the directory/files to be moved. May be either a full path, or
				 relative to the session path. Note that if you specify a directory level,
				 then the behaviour of the move operation is sensitive to the presence
				 (or absence) of a trailing backslash ("\") character. Any path components
				 which are not specified here will be taken from the session path. See the
				 main description for the detailed explanation.
@param aNew      Path indicating the directory into which the file(s) are to be moved.
				 Any path components which are not specified here will be taken from the session path.
@param aSwitches CFileMan::EOverWrite to overwrite files with the same name;
                 CFileMan::ERecurse for recursion.
                 By default, the synchronous variant of this function operates non-recursively and
				 with overwriting. And no trailing backslash ("\") character at the end of source path
				 always means CFileMan::ERecurse.

@return KErrNone if successful, otherwise one of the other system-wide error
        codes.

@capability Dependent If the path in aNew starts with /Sys then capability Tcb is required
@capability Dependent If the path in aNew starts with /Resource then capability Tcb is required

@capability AllFiles

@capability Dependent If the path in anOld starts with /Sys then Tcb capability is required.
@capability Dependent If the path in anOld starts with /Resource then Tcb capability is required.

@see CFileBase::GetLastError()
*/
	{
	OstTraceExt2(TRACE_BORDER, EFSRV_ECFILEMANMOVE2, "this %x aSwitches %d", (TUint) this, (TUint) aSwitches);
	OstTraceData(TRACE_BORDER, EFSRV_ECFILEMANMOVE2_EOLDNAME, "OldName %S", anOld.Ptr(), anOld.Length()<<1);
	OstTraceData(TRACE_BORDER, EFSRV_ECFILEMANMOVE2_ENEWNAME, "NewName %S", aNew.Ptr(), aNew.Length()<<1);


	if (iSwitches&KFManBusyFlag)
		{
		OstTrace1(TRACE_BORDER, EFSRV_ECFILEMANMOVE2RETURN1, "r %d", KErrInUse);
		return(KErrInUse);
		}

	iNumberOfFilesProcessed = 0;

	TInt r;
	if ((r = iFs.Parse(anOld,iSrcFile)) != KErrNone)
		{
		if(iStatus)
			User::RequestComplete(iStatus,r);
		OstTrace1(TRACE_BORDER, EFSRV_ECFILEMANMOVE2RETURN2, "r %d", r);
		return r;
		}

	if ((r = iFs.Parse(aNew,_L("*"),iTrgFile)) != KErrNone)
		{
		if(iStatus)
			User::RequestComplete(iStatus,r);
		OstTrace1(TRACE_BORDER, EFSRV_ECFILEMANMOVE2RETURN3, "r %d", r);
		return r;
		}


	TInt ret = KErrNone;
	TBool aComplete = EFalse;
	if(SrcTrgDrivesIdentical())
		{
		ret = SetupMoveOnSameDrive(aSwitches, aComplete);
		}
	else
		{
		ret = SetupMoveAcrossDrives(aSwitches);
		}

	if(ret != KErrNone || aComplete)
		{
		if (iStatus)
			{
			User::RequestComplete(iStatus, ret);
			}
		OstTrace1(TRACE_BORDER, EFSRV_ECFILEMANMOVE2RETURN4, "r %d", ret);
		return(ret);
		}

	iMatchEntry = KEntryAttMaskSupported;
	if((aSwitches&ERecurse)==0 && iMovingContents)
		{
		iMatchEntry = KMovingFilesMask;
		}

	// Do the Move or Rename Operation
	TRAP(r,RunL());
	ret = (r==KErrNone) ? iLastError : r;
	DoSynchronize(r);

	OstTrace1(TRACE_BORDER, EFSRV_ECFILEMANMOVE2RETURN5, "r %d", ret);

	return(ret);
	}


TBool CFileMan::SrcTrgDrivesIdentical()
//
// Returns ETrue if the source and target drives are the same
//	- Used by CFileMan::Move operations to determine whether to rename or move the files
//
	{
	return iSrcFile.Drive().MatchF(iTrgFile.Drive()) != KErrNotFound;
	}


TInt CFileMan::SetupDirectoryForMove(TBool& aSrcIsDir)
/**
 * Sets up the target specification to include the new target directory if required.
 * 
 * @param aSrcIsDir	Set to ETrue if the source specifies that the directory is to be moved in its entirety,
 * 					or EFalse if only the contents of the directory need to be moved.
 * 
 * @return KErrNone if successful, otherwise one of the system wide error codes.
 */
	{
	iMovingContents = ETrue;
	aSrcIsDir = EFalse;
	
	TPtrC nameAndExt(iSrcFile.NameAndExt());
	if (nameAndExt == _L("*") || nameAndExt == _L("*.*"))
		{
		// Wildcard specification - Move the entire contents of the directory to the target
		aSrcIsDir = ETrue;
		}
	else
		{
		TInt src = iFs.Entry(iSrcFile.FullName(), iTmpEntry);
		if ((src == KErrNone && iTmpEntry.iAtt&KEntryAttDir) || (!iSrcFile.NamePresent() && iSrcFile.IsRoot()))
			{
			aSrcIsDir = ETrue;

			// A directory is specified.
			//  - Mandatory recursion with Wildcard Copy
			//	- Target is a directory (Enforced by MakeParseWild)

			MakeParseWild(iTrgFile, iTmpName1);

			// Construct the target name by parsing
			// the source path for the directory name.
			TPtrC srcPath(iSrcFile.FullName());
			TInt srcPathLen = srcPath.Length() - 1;

			iMovingContents = (srcPath[srcPathLen] == KPathDelimiter);	// Moving the directory itself, or the contents?

			if(!iMovingContents)
				{
				// No path delimiter specified
				//	- move the whole directory (if specified)
				TInt len = srcPath.Length();

				TInt idx = srcPath.Left(len).LocateReverse(KPathDelimiter);

				if((idx >= 2) && (idx != KErrNotFound))
					{
					// Source path is a directory (not just the drive)
					TPtrC mid(srcPath.Left(len).Mid(1+idx));
					TInt r = iTrgFile.AddDir(mid);
					if (r != KErrNone)
						return r;
					}
				}
			}
		}

	return KErrNone;
	}


TInt CFileMan::SetupTargetDirectory(TBool aOverWrite, TBool& aComplete)
	{
	aComplete = EFalse;

	TInt trgErr = iFs.Entry(iTrgFile.DriveAndPath(), iTmpEntry);

	TEntry srcEntry;
	TInt srcErr = iFs.Entry(iSrcFile.FullName(), srcEntry);

	if(srcErr == KErrNone && trgErr == KErrNone)
		{
        if ((srcEntry.iAtt&KEntryAttDir) != (iTmpEntry.iAtt&KEntryAttDir))
        	{
        	// return KErrAccessDenied if it is trying to overwrite a file to a dir or vice versa.
        	return KErrAccessDenied;
        	}
		}

	if(trgErr == KErrNone)
		{
		// Already Exists - Overwrite if flags set
		if(!aOverWrite)
			{
			trgErr = KErrAlreadyExists;
			}
		else
			{
			iNumberOfFilesProcessed++;
			}
		}
	else if((trgErr == KErrNotFound) || (trgErr == KErrPathNotFound))
		{
		if(SrcTrgDrivesIdentical())
			{
			// When moving a directory on the same drive, the directory can simply be renamed...
			TParse& midDir = iTmpParse;
			midDir = iTrgFile;
			if(midDir.PopDir() == KErrNone)
				{
				// ...before renaming, ensure that all intermediate directories exist
				trgErr = iFs.MkDirAll(midDir.DriveAndPath());
				if(trgErr == KErrAlreadyExists)
					{
					trgErr = KErrNone;
					}
				}

			if (trgErr == KErrNone)
				{
				// ...and finally rename the source directory
				trgErr = iFs.Rename(iSrcFile.FullName(),iTrgFile.DriveAndPath());
				aComplete = ETrue;
				}
			}
		else
			{
			trgErr = iFs.MkDirAll(iTrgFile.FullName());
			}
		iNumberOfFilesProcessed++;
		}

	return(trgErr);
	}


TInt CFileMan::SetupMoveOnSameDrive(TUint aSwitches, TBool& aComplete)
	{
	// Moving on the same drive.

	aComplete = EFalse;

	TBool srcIsDir = EFalse;
    TInt ret = SetupDirectoryForMove(srcIsDir);
    if (ret != KErrNone)
		{
        return ret;
		}

	TBool scanDown = ETrue;
	TBool recurse = (aSwitches & ERecurse);

	iAction = EInternalRenameForMove;

	TFileName& srcpath = iTmpName1;
	srcpath.Copy(iSrcFile.FullName());
	if(srcpath.Length()<KMaxFileName && srcpath.Length()>1 && srcpath[srcpath.Length()-1]!=KPathDelimiter)
		{
		srcpath.Append(KPathDelimiter);
		}

	// If the source path is a subset of the target path then Move operation is not allowed
	if((srcIsDir && recurse) || (srcIsDir && !iTrgFile.IsRoot() && !iMovingContents))
		{
		if(iTrgFile.FullName().Left(srcpath.Length()).MatchF(srcpath)==0)
			{
			aComplete = ETrue;
			return KErrInUse;
			}
		}
	// if any of the SRC folders already existing in TRG, scan upwards
	if(iMovingContents)
		{
		CDirScan* srcScanDir = NULL;
		CDirScan* trgScanDir = NULL;
		CDir* srcEntryList = NULL;
		CDir* trgEntryList = NULL;
		TInt trgCnt = 0;
		TInt srcCnt = 0;

		TRAP(ret,(srcScanDir = CDirScan::NewL(iFs)));
		if (ret!=KErrNone)
			{
			goto CleanUp;
			}
		TRAP(ret,srcScanDir->SetScanDataL(iSrcFile.FullName(),KEntryAttMaskSupported,ESortByName));
		if (ret!=KErrNone)
			{
			goto CleanUp;
			}
		TRAP(ret,srcScanDir->NextL(srcEntryList));
		if(ret!=KErrNone)
			{
			goto CleanUp;
			}
		TRAP(ret,(trgScanDir=CDirScan::NewL(iFs)));
		if (ret!=KErrNone)
			{
			goto CleanUp;
			}
		TRAP(ret,trgScanDir->SetScanDataL(iTrgFile.FullName(),KEntryAttMaskSupported,ESortByName));
		if (ret!=KErrNone)
			{
			goto CleanUp;
			}
		TRAP(ret,trgScanDir->NextL(trgEntryList));
		if(ret!=KErrNone)
			{
			goto CleanUp;
			}
		for(trgCnt=trgEntryList->Count()-1; trgCnt>-1; trgCnt--)
			{
			for(srcCnt=srcEntryList->Count()-1; srcCnt>-1; srcCnt--)
				{
				if( (*srcEntryList)[srcCnt].iName == (*trgEntryList)[trgCnt].iName
					&& ((*srcEntryList)[srcCnt].iAtt & KEntryAttDir)
					&& ((*trgEntryList)[trgCnt].iAtt & KEntryAttDir))
					{
					// Set scan upwards
					scanDown = EFalse;
					goto CleanUp;
					}
				}// end inner for loop
			} // end outer for loop
CleanUp:
		// clean up
		if(srcEntryList!=NULL)
			delete srcEntryList;
		if(trgEntryList!=NULL)
			delete trgEntryList;
		if(srcScanDir!=NULL)
			delete srcScanDir;
		if(trgScanDir!=NULL)
			delete trgScanDir;
		}// end if(iMovingContents)

	if(srcIsDir && !iTrgFile.IsRoot() && !iMovingContents)
		{
		ret = SetupTargetDirectory(aSwitches & EOverWrite, aComplete);
		if(ret != KErrNone || aComplete)
			{
			return(ret);
			}
		}
	if(!iMovingContents)
		{
		recurse = ETrue;
		scanDown = EFalse;
		}
	if(srcIsDir)
		{
		MakeParseWild(iSrcFile, iTmpName1);
		}

	SetFlags(aSwitches & EOverWrite, recurse, scanDown, ETrue);
	return(KErrNone);
	}


TInt CFileMan::SetupMoveAcrossDrives(TUint aSwitches)
	{
	// Moving across drives.  We may need to recurse,
	// depending on the supplied source path.

	TBool srcIsDir = EFalse;
    TInt ret = SetupDirectoryForMove(srcIsDir);
    if (ret != KErrNone)
		{
        return ret;
		}
    
	TBool recurse = (aSwitches & ERecurse);
	TBool scanDown = (recurse) ? (TBool)EFalse : (TBool)ETrue;

	if(srcIsDir)
		{
		if(!iMovingContents)
			{
			recurse = ETrue;
			if(!iTrgFile.IsRoot())
				{
				TBool complete = EFalse;
				ret = SetupTargetDirectory(aSwitches & EOverWrite, complete);
				if(ret != KErrNone || complete)
					{
					return(ret);
					}
				}
			}
		}

	CheckForDirectory();
	iAction = EInternalCopyForMove;
	SetFlags(aSwitches & EOverWrite, recurse, scanDown, EFalse);
	return(KErrNone);
	}



EXPORT_C TInt CFileMan::Rename(const TDesC& aName,const TDesC& aNewName,TUint aSwitches,TRequestStatus& aStatus)
/**
Renames one or more files.

This is an asynchronous function.
Its behaviour is the same as the synchronous overload.

@param aName     Path specifying the file(s) to be renamed. Any path components
				 which are not specified
                 here will be taken from the session path.
@param aNewName  Path specifying the new name for the files and/or
                 the new directory. Any directories specified in this path
                 that do not exist, will be created. Any path components
				 which are not specified here will be taken from the session path.
@param aSwitches Specify zero for no overwriting;
                 CFileMan::EOverWrite to overwrite files with the same name.
                 This function cannot operate recursively.
@param aStatus   The request status object. On request completion,
                 indicates how the request completed:
                 KErrNone, if successful, otherwise one of the other system-wide error
                 codes.

@return KErrNone if the asynchronous request is made successfully; KErrInUse if an asynchronous request
					is still pending; otherwise one of the other system-wide error codes

@capability Dependent If either aName or aNewName is /Sys then Tcb capability is required.
@capability Dependent If either aName or aNewName begins with /Private and does not match
					  this process' SID then AllFiles capability is required.
@capability Dependent If either aName or aNewName is /Resource then Tcb capability is required.

*/
	{
	OstTraceExt3(TRACE_BORDER, EFSRV_ECFILEMANRENAME1, "this %x aSwitches %x status %x", (TUint) this, (TUint) aSwitches, (TUint) &aStatus);
	OstTraceData(TRACE_BORDER, EFSRV_ECFILEMANRENAME1_EOLDNAME, "OldName %S", aName.Ptr(), aName.Length()<<1);
	OstTraceData(TRACE_BORDER, EFSRV_ECFILEMANRENAME1_ENEWNAME, "NewName %S", aNewName.Ptr(), aNewName.Length()<<1);

	TInt r;
	if (iSwitches&KFManBusyFlag)
		{
		r = KErrInUse;
		}
	else
		{
		iStatus=&aStatus;
		r = Rename(aName,aNewName,aSwitches);
		}

	OstTrace1(TRACE_BORDER, EFSRV_ECFILEMANRENAME1RETURN, "r %d", r);

	return(r);
	}




EXPORT_C TInt CFileMan::Rename(const TDesC& aName,const TDesC& aNewName,TUint aSwitches)
/**
Renames one or more files, or a directory

This is a synchronous function.

The function can also be used to move files by specifying different destination
and source directories.

Some rules for using CFileMan::Rename():

1. General rules:

1.1. Trailing backslash ("\") in either source path (aName) or target path (aNewName)
will be interpreted to "\*.*";

For example, following code should behave identically:
@code
CFileMan* fm(CFileMan::NewL(iFs)); // Where iFs is an RFs handle
...
fm->Rename(_L("C:\\SRC\\"), _L("C:\\TRG\\"));
fm->Rename(_L("C:\\SRC\\*.*"), _L("C:\\TRG\\"));
fm->Rename(_L("C:\\SRC\\"), _L("C:\\TRG\\*.*"));
fm->Rename(_L("C:\\SRC\\*.*"), _L("C:\\TRG\\*.*"));
@endcode

1.2 The behaviour of the rename operation is sensitive to the presence (or absence) of
a trailing backslash ("\") character on the end of the target path (aNewName);

For example, under all other constraints (see rules 2. and 3.),
@code
CFileMan* fm(CFileMan::NewL(iFs)); // Where iFs is an RFs handle
...
fm->Rename(_L("C:\\SRC"), _L("C:\\TRG\"));
@endcode
will result in renaming "C:\\SRC" to "C:\\TRG\\SRC", while
@code
CFileMan* fm(CFileMan::NewL(iFs)); // Where iFs is an RFs handle
...
fm->Rename(_L("C:\\SRC"), _L("C:\\TRG"));
@endcode
will result in renaming "C:\\SRC" to "C:\\TRG".

2. Renaming file(s):

2.1 Wildcards:

A file's name and extension are interpreted separately, for example:

@code
CFileMan* fm(CFileMan::NewL(iFs)); // Where iFs is an RFs handle
...
fm->Rename(_L("C:\\SRC\\1234.567"), _L("C:\\TRG\\AB*CD.TXT"));
@endcode
renames the source file to file "C:\\TRG\\AB34CD.TXT".

Wildcards can be used for renaming multiple files, for example;
@code
CFileMan* fm(CFileMan::NewL(iFs)); // Where iFs is an RFs handle
...
fm->Rename(_L("C:\\SRC\\*.567"), _L("C:\\TRG\\*.TXT"));
@endcode
renames all the file under "C:\\SRC\\" having extension ".567" to the files under
"C:\\TRG\\" having extension ".TXT".

2.2 An option is provided to allow the user to overwrite any files with the same
name which may exist in the target directory; If the flag is set for no overwriting,
any files with the same name will not be overwritten, and an error (KErrAlreadyExists)
will be returned for that file, as retrieved by CFileBase::GetLastError().

2.3 It can only operate non-recursively, so that only the matching files located
in the single directory specified by anOld may be renamed.

2.4 Trying to rename file(s) to existing directory(ies) will fail;

For example, giving following directory structure:
@code
C:\SRC\ITEM01
C:\SRC\ITEM02
C:\TRG\ITEM01\
C:\TRG\ITEM02\
@endcode

Following code will fail:
@code
CFileMan* fm(CFileMan::NewL(iFs)); // Where iFs is an RFs handle
...
fm->Rename(_L("C:\\SRC\\ITEM01"), _L("C:\\TRG\\ITEM01"));
fm->Rename(_L("C:\\SRC\\ITEM*"), _L("C:\\TRG\\ITEM*"));
fm->Rename(_L("C:\\SRC\\"), _L("C:\\TRG\\"));
@endcode

3. When renamnig a directory:

3.1. Only when the trailing backslash ("\") is missing from the source path (aName),
will the source directory be renamed, otherwise, see rule 1.1.

For example, following code will result in moving "C:\SRC" directory including all
its contents:
@code
CFileMan* fm(CFileMan::NewL(iFs)); // Where iFs is an RFs handle
...
fm->Rename(_L("C:\\SRC"), _L("C:\\TRG"));
fm->Rename(_L("C:\\SRC"), _L("C:\\TRG\\"));
fm->Rename(_L("C:\\SRC"), _L("C:\\TRG\\*.*"));
@endcode

3.2. Wildcards can not be used for moving directories;

3.3. Overwriting is not permitted;

For example, giving directory structure as following:
@code
C:\SRC\FILE.TXT
C:\TRG\
C:\TRG\SRC\
@endcode

following code will fail:
@code
CFileMan* fm(CFileMan::NewL(iFs)); // Where iFs is an RFs handle
...
fm->Rename(_L("C:\\SRC"), _L("C:\\TRG"));
fm->Rename(_L("C:\\SRC"), _L("C:\\TRG\\"));
fm->Rename(_L("C:\\SRC"), _L("C:\\TRG\\*.*"));
@endcode

4. Notes:

4.1. The target and source directories must be on the same drive.

4.2. Read-only, hidden and system files can be moved and the source file's
attributes are preserved in the target file, but open files cannot
be moved. Attempting to move an open file will return an error for
that file, as retrieved by CFileBase::GetLastError().

@param aName     Path specifying the file(s) to be renamed. Any path components
				 which are not specified
                 here will be taken from the session path.
@param aNewName  Path specifying the new name for the files and/or
                 the new directory. Any directories specified in this path
                 that do not exist, will be created. Any path components which
				 are not specified here will be taken from the session path.
@param aSwitches Specify zero for no overwriting;
                 CFileMan::EOverWrite to overwrite files with the same name.
                 This function cannot operate recursively.

@return KErrNone if successful, otherwise one of the other system-wide error
        codes.

@see CFileBase::GetLastError()

@capability Dependent If either aName or aNewName is /Sys then Tcb capability is required.
@capability Dependent If either aName or aNewName begins with /Private and does not match
					  this process' SID then AllFiles capability is required.
@capability Dependent If either aName or aNewName is /Resource then Tcb capability is required.

*/
	{
	OstTraceExt2(TRACE_BORDER, EFSRV_ECFILEMANRENAME2, "this %x aSwitches %d", (TUint) this, (TUint) aSwitches);
	OstTraceData(TRACE_BORDER, EFSRV_ECFILEMANRENAME2_EOLDNAME, "OldName %S", aName.Ptr(), aName.Length()<<1);
	OstTraceData(TRACE_BORDER, EFSRV_ECFILEMANRENAME2_ENEWNAME, "NewName %S", aNewName.Ptr(), aNewName.Length()<<1);

	TInt ret;
	if (iSwitches&KFManBusyFlag)
		{
		ret = KErrInUse;
		}
	else
		{
		SetFlags(aSwitches&EOverWrite,EFalse,ETrue,EFalse);
		TInt r;
		if ((r = iFs.Parse(aName,iSrcFile)) != KErrNone)
			{
			if(iStatus)
				User::RequestComplete(iStatus,r);
			OstTrace1(TRACE_BORDER, EFSRV_ECFILEMANRENAME2RETURN1, "r %d", r);
			return r;
			}

		if ((r = iFs.Parse(aNewName,_L("*"),iTrgFile)) != KErrNone)
			{
			if(iStatus)
				User::RequestComplete(iStatus,r);
			OstTrace1(TRACE_BORDER, EFSRV_ECFILEMANRENAME2RETURN2, "r %d", r);
			return r;
			}

		iAction = EInternalRename;
		iMatchEntry=KEntryAttMaskSupported;
		iNumberOfFilesProcessed = 0;
		TRAP(r,RunL());
		ret=(r==KErrNone) ? iLastError : r;
		DoSynchronize(r);
		}

	OstTrace1(TRACE_BORDER, EFSRV_ECFILEMANRENAME2RETURN3, "r %d", ret);

	return(ret);
	}


EXPORT_C TInt CFileMan::RmDir(const TDesC& aDirName,TRequestStatus& aStatus)
/**
Deletes a directory and all files and directories contained in the
directory structure below it.

Other than being asynchronous, the behaviour of this function is the same
as is documented in its synchronous overload.

@param aDirName Path specifying the directory to be deleted. Any path components
				which are not specified here will be taken from the session path.
@param aStatus  The request status object. On request completion, indicates how
                the request completed:
                KErrNone if successful, otherwise one of the other system-wide
                error codes.

@return KErrNone if the asynchronous request is made successfully; KErrInUse if an asynchronous request
					is still pending; otherwise one of the other system-wide error codes

@capability Dependent If aDirName starts with /Sys then Tcb capability is required.
@capability Dependent If aDirName begins with /Private and does not match this process' SID
					  then AllFiles capability is required.
@capability Dependent If aDirName starts with /Resource then Tcb capability is required.

*/
	{
	OstTraceExt2(TRACE_BORDER, EFSRV_ECFILEMANRMDIR1, "this %x status %x", (TUint) this, (TUint) &aStatus);
	OstTraceData(TRACE_BORDER, EFSRV_ECFILEMANRMDIR1_EDIRNAME, "Dir %S", aDirName.Ptr(), aDirName.Length()<<1);

	TInt r;
	if (iSwitches&KFManBusyFlag)
		{
		r = KErrInUse;
		}
	else
		{
		iStatus=&aStatus;
		r = RmDir(aDirName);
		}

	OstTrace1(TRACE_BORDER, EFSRV_ECFILEMANRMDIR1RETURN, "r %d", r);

	return r;
	}


EXPORT_C TInt CFileMan::RmDir(const TDesC& aDirName)
/**
Deletes a directory and all files and directories contained in the
directory structure below it.

This is a synchronous function.

The function cannot be used non-recursively. For a non-recursive
directory deletion, use RFs::RmDir().

Note:

1. All files in the directory hierarchy to be deleted must be closed and
   none may have the read-only attribute. Otherwise, not all of the hierarchy will
   be deleted, and this function will return KErrInUse.

@param aDirName Path specifying the directory to be deleted. Any path components
				which are not specified here will be taken from the session path.

@return KErrNone if successful, otherwise one of the other system-wide error
        codes.

@capability Dependent If aDirName starts with /Sys then Tcb capability is required.
@capability Dependent If aDirName begins with /Private and does not match this process' SID
					  then AllFiles capability is required.
@capability Dependent If aDirName starts with /Resource then Tcb capability is required.


*/
	{
	OstTrace1(TRACE_BORDER, EFSRV_ECFILEMANRMDIR2, "this %x", (TUint) this);
	OstTraceData(TRACE_BORDER, EFSRV_ECFILEMANRMDIR2_EDIRNAME, "Dir %S", aDirName.Ptr(), aDirName.Length()<<1);

	TInt ret;
	if (iSwitches&KFManBusyFlag)
		{
		ret = KErrInUse;
		}
	else
		{
		SetFlags(ETrue,ETrue,EFalse,EFalse);
		TInt r;
		if ((r = iFs.Parse(aDirName,iTrgFile)) != KErrNone)
			{
			if(iStatus)
				User::RequestComplete(iStatus,r);
			OstTrace1(TRACE_BORDER, EFSRV_ECFILEMANRMDIR2RETURN1, "r %d", r);
			return r;
			}

		iSrcFile.Set(iTrgFile.DriveAndPath(),NULL,NULL);
		iAction = EInternalRmDir;
		iMatchEntry=KEntryAttMaskSupported;
		iNumberOfFilesProcessed = 0;
		TRAP(r,RunL());
		DoSynchronize(r);
		ret = (r!=KErrNone) ? iLastError : KErrNone;
		}

	OstTrace1(TRACE_BORDER, EFSRV_ECFILEMANRMDIR2RETURN2, "r %d", ret);

	return ret;
	}


void CFileMan::DoOperationL()
// Call the action in progress.
	{
	switch (iAction)
		{
	case EInternalAttribs:
		DoAttribsL();
		break;
	case EInternalCopy:
	case EInternalCopyForMove:
		DoCopyOrMoveL();
		break;
	case EInternalDelete:
		DoDeleteL();
		break;
	case EInternalRenameInvalidEntry:
	case EInternalRenameForMove:
	case EInternalRename:
		DoRenameL();
		break;
	case EInternalRmDir:
		DoRmDirL();
		break;
	case EInternalCopyFromHandle:
		DoCopyFromHandleL();
		break;
	default:
		Panic(EFManUnknownAction);
		}
	}

void CFileMan::DoAttribsL()
//
// Do attribs operation step
//
	{
	TPtrC fullPath(FullPath());
	iTmpParse.Set(CurrentEntry().iName, &fullPath, NULL);
	User::LeaveIfError(iFs.SetEntry(iTmpParse.FullName(), iTime, iSetMask, iClearMask));
	}

void CFileMan::DoCopyOrMoveL()
//
// Do copy or move operation
//
	{
	// Following 'if' statements are to prevent incorrect recursive Move() or Copy() from "destination"
	//  to "destination", this problem occurs when the initial source directory contains destination
	//  directory.
	//  (e.g. CFileMan::Move(_L("C:\\SRC\\*.TXT"), _L("C:\\SRC\\Sub\\"), CFileMan::ERecurse);)
	// Note that CFileMan::Rename() does not suffer from this particular case, as CFileMan::Rename() API
	//  can only operate non-recursively.
	if (iSrcFile.DriveAndPath().Length() < iTrgFile.DriveAndPath().Length())
		{
		if (iTrgFile.DriveAndPath().Left(iSrcFile.DriveAndPath().Length()) == iSrcFile.DriveAndPath())
		// If source directory path contains destination directory path, including drive number, we consider
		//  this is "...\\ROOT\\" -> "...\\ROOT\\SUB\\" type of operation. Therefore skips all the items we
		//  found in "...\\ROOT\\SUB\\". We achieve this by checking current scanning directory path:
			{
			if (iTrgFile.DriveAndPath() == iScanner->FullPath().Left(iTrgFile.DriveAndPath().Length()))
				{
				return;
				}
			}
		}

	TParse& srcName = iTmpParse;
	TFileName& trgName = iTmpName1;
	GetSrcAndTrg(srcName,trgName);

	// Handle case when source is directory
	if (CurrentEntry().iAtt&KEntryAttDir)
		{
		if(!(iSwitches&KRecurseFlag))
			{
			User::Leave(KErrNone);
			}
		trgName.Append(KPathDelimiter);
		TInt r = iFs.MkDirAll(trgName);
		if (r!=KErrNone && r!=KErrAlreadyExists)
			User::Leave(r);

		if(iAction == EInternalCopyForMove)
			{
			// Move operation - Attempt to delete the source directory.
			if((iMatchEntry & KMovingFilesMask) != KMovingFilesMask)
				{
				iTmpName2 = srcName.FullName();
				iTmpName2.Append(KPathDelimiter);
				TInt rdErr = iFs.RmDir(iTmpName2);
				if(rdErr != KErrNone && rdErr != KErrInUse)
					{
					User::Leave(rdErr);
					}
				}
			}
		return;
		}

#ifndef SYMBIAN_ENABLE_64_BIT_FILE_SERVER_API
	RFile srcFile,trgFile;
#else
	RFile64 srcFile,trgFile;
#endif
	TInt r=KErrNone;
	if (FileNamesIdentical(srcName.FullName(),trgName))
		{
		if (iSwitches & KOverWriteFlag)
			// Source and target are identical, KOverWriteFlag makes copying
			// having no effect.
			return;
		else
			User::Leave(KErrAlreadyExists);
		}

	r=srcFile.Open(iFs, srcName.FullName(),
	                iAction==EInternalCopy ? EFileRead|EFileShareReadersOnly  // Copy access
	                                : EFileWrite|EFileWriteDirectIO|EFileShareExclusive); // Move access
	TBool isRO = EFalse;
	if(r==KErrAccessDenied && iAction==EInternalCopyForMove)
		{
		TEntry& entry = iTmpEntry;
		r = iFs.Entry(srcName.FullName(), entry);
		if(r==KErrNone && (entry.iAtt&KEntryAttReadOnly))
			{
			isRO = ETrue;
			r = iFs.SetAtt(srcName.FullName(), 0, KEntryAttReadOnly);
			if(r==KErrNone)
				{
				r = srcFile.Open(iFs, srcName.FullName(), EFileWrite|EFileWriteDirectIO|EFileShareExclusive);
				}
			}
		}
	if (r!=KErrNone)
		{
		iErrorInfo=ESrcOpenFailed;
		if(isRO)
			{
			iFs.SetAtt(srcName.FullName(), KEntryAttReadOnly, 0);
			}
		User::Leave(r);
		}

	if ((iSwitches&KOverWriteFlag)==0)
		r=trgFile.Create(iFs,trgName,EFileWrite|EFileWriteDirectIO|EFileShareExclusive);
	else
		r=trgFile.Replace(iFs,trgName,EFileWrite|EFileWriteDirectIO|EFileShareExclusive);

	if (r==KErrPathNotFound && (iSwitches&KRecurseFlag))
		{
		r=iFs.MkDirAll(trgName);
		if (r==KErrNone)
			r=trgFile.Create(iFs,trgName,EFileWrite|EFileWriteDirectIO|EFileShareExclusive);
		}

	if (r!=KErrNone)
		iErrorInfo=ETrgOpenFailed;

	TInt ret=0;
	if (r == KErrNone)
		r = DoCopy(srcFile, trgFile, ret);

	srcFile.Close();
	trgFile.Close();
	if ((r!=KErrNone && (r!=KErrAlreadyExists && iErrorInfo!=ETrgOpenFailed)) || (ret==MFileManObserver::ECancel))
		iFs.Delete(trgName);
	if(r==KErrNone && isRO)
		{
		r = iFs.SetAtt(trgName, KEntryAttReadOnly, 0);
		}
	User::LeaveIfError(r);

	//
	// Move operation
	//
	if (iAction == EInternalCopyForMove && ret != MFileManObserver::ECancel)
		{
		r=iFs.Delete(srcName.FullName());
		if (r==KErrNone)
			return;
		iFs.Delete(trgName);
		User::Leave(r);
		}
	}

void CFileMan::DoDeleteL()
//
// Do delete operation step
//
	{
	TFileName& pathname = iTmpName1;
	TFileName& filename = iTmpName2;
	pathname.Copy(FullPath());
	filename.Copy(CurrentEntry().iName);
	if(CurrentEntry().iName.Length() + pathname.Length() > KMaxFileName)
		{
		User::LeaveIfError(ShrinkNames(iFs, pathname, filename, EFalse));
		}
	iTmpParse.Set(filename, &pathname, NULL);
	User::LeaveIfError(iFs.Delete(iTmpParse.FullName()));
	}

void CFileMan::DoRenameL()
//
// Do rename operation step
//
	{
	// Following 'if' statements are to prevent incorrect recursive Move() or Copy() from "destination"
	//  to "destination", this problem occurs when the initial source directory contains destination
	//  directory.
	//  (e.g. CFileMan::Move(_L("C:\\SRC\\*.TXT"), _L("C:\\SRC\\Sub\\"), CFileMan::ERecurse);)
	// Note that CFileMan::Rename() does not suffer from this particular case, as CFileMan::Rename() API
	//  can only operate non-recursively.
	if (iSrcFile.DriveAndPath().Length() < iTrgFile.DriveAndPath().Length())
		{
		if (iTrgFile.DriveAndPath().Left(iSrcFile.DriveAndPath().Length()) == iSrcFile.DriveAndPath())
		// If source directory path contains destination directory path, including drive number, we consider
		//  this is "...\\ROOT\\" -> "...\\ROOT\\SUB\\" type of operation. Therefore skips all the items we
		//  found in "...\\ROOT\\SUB\\". We achieve this by checking current scanning directory path:
			{
			if (iTrgFile.DriveAndPath() == iScanner->FullPath().Left(iTrgFile.DriveAndPath().Length()))
				{
				return;
				}
			}
		}

	TParse& srcName = iTmpParse;
	TFileName& trgName = iTmpName1;
	GetSrcAndTrg(srcName, trgName);

	TInt r = iFs.Rename(srcName.FullName(),trgName);
	if (r==KErrAlreadyExists && (iSwitches&KOverWriteFlag)!=0)
		{
		// Target already exists, with the overwrite flag enabled
		if((CurrentEntry().iAtt & KEntryAttDir) == 0)
			{
			// Renaming a file
			r=iFs.Replace(srcName.FullName(),trgName);
			}
		else if (iAction == EInternalRenameForMove)
			{
			trgName = srcName.FullName();
			trgName.Append(KPathDelimiter);
			r = iFs.RmDir(trgName); // remove empty directory after move
			if(r == KErrInUse)
				{
				r = KErrNone;
				}
			}
		}

	if (r==KErrPathNotFound)
		{
		if((iSwitches&KMoveRenameFlag) && !(iSwitches&KRecurseFlag))
			User::Leave(r);
		r=iFs.MkDirAll(trgName);
		if (r==KErrNone)
			 r=iFs.Rename(srcName.FullName(),trgName);
		}
	if (r==KErrBadName)
		{
		TEntry& entry = iTmpEntry;
		TInt retcode=iFs.Entry(srcName.FullName(), entry);
		if (retcode!=KErrNone)
			iErrorInfo=ESrcOpenFailed;
		else
			iErrorInfo=ETrgOpenFailed;
		}
	User::LeaveIfError(r);
	}

void CFileMan::DoRmDirL()
//
// Do rmdir operation step
//
	{
	TFileName& srcName = iTmpName1;
	srcName.Copy(FullPath());
	if (srcName.Length() + CurrentEntry().iName.Length() > KMaxFileName)
		{
		TFileName& current = iTmpName2;
		current.Copy(CurrentEntry().iName);
		User::LeaveIfError(ShrinkNames(iFs, srcName, current, ETrue));
		}
	else
		{
		srcName.Append(CurrentEntry().iName);
		}

	if ((CurrentEntry().iAtt&KEntryAttDir)==0)
		User::LeaveIfError(iFs.Delete(srcName));
	else
		{
		srcName.Append(KPathDelimiter);
		User::LeaveIfError(iFs.RmDir(srcName));
		}
	}


void CFileMan::CompleteOperationL()
//
// Tidy up after an operation
// The last step to remove directory or to a move directory operation
// is to remove the source directory...
//
	{
	TInt r=KErrNotFound;
	if (iAction == EInternalRmDir ||
	    (iAction == EInternalCopyForMove && ((iMatchEntry & KMovingFilesMask) != KMovingFilesMask) && !iMovingContents && !iSrcFile.IsRoot()) ||
	    iAction == EInternalRenameForMove && !iMovingContents && iNumberOfFilesProcessed)
		{
		r=iFs.RmDir(iSrcFile.FullName());
		if ((r!=KErrNone && r!=KErrNotFound && iAction!=EInternalRenameForMove && r!=KErrInUse) || (iAction == EInternalRmDir && r == KErrInUse))
			{
			iLastError=r;
			User::Leave(r);
			}
		}

	if (iLastError == KErrCancel && iNumberOfFilesProcessed==0 )
	{
		iLastError=KErrCancel;
		iErrorInfo=ENoFilesProcessed;
		User::Leave(KErrCancel);
	}

	if (iLastError==KErrNone && r==KErrNotFound && iNumberOfFilesProcessed==0)
		{
		iLastError=KErrNotFound;
		iErrorInfo=ENoFilesProcessed;
		User::Leave(KErrNotFound);
		}
	}

void CFileMan::SetFlags(TBool anOverWrite,TBool aRecurse,TBool aScanDownTree,TBool aMoveRename)
//
// Set or clear flags
//
	{

	iSwitches=0;
	if (aRecurse)
		iSwitches|=KRecurseFlag;
	if (anOverWrite)
		iSwitches|=KOverWriteFlag;
	if (aScanDownTree)
		iSwitches|=KScanDownFlag;
	if (aMoveRename)
		iSwitches|=KMoveRenameFlag;
	}


EXPORT_C TInt CFileMan::Copy(const RFile& anOld, const TDesC& aNew, TUint aSwitches)
/**
Copies from an open file handle to a destination file name.

This is a synchronous function.

Optionally, this function can be set to overwrite the target file.
If the flag is set for no overwriting and the target file already exists,
then the target file will not be overwritten, and an error (KErrAlreadyExists)
will be returned.
Error codes may be retrieved using CFileBase::GetLastError().

Notes:

-# The file can be copied across drives.
-# Read-only, hidden and system files can be copied and
   the source file's attributes are preserved in the target file.

@param anOld     Open file handle indicating the file to be copied.
@param aNew      Path indicating the directory (and optionally the filename)
				 into which the file is to be copied.
				 Any path components which are not specified here will be
                 taken from the session path
@param aSwitches Specify zero for no overwriting;
                 CFileMan::EOverWrite to overwrite files with the same name;
                 Any other flags are illegal
                 By default, the synchronous variant of this function operates
                 with overwriting.

@return KErrNone if successful, otherwise one of the other system-wide error codes.

@see CFileBase::GetLastError()
@see CFileMan::Move()

@capability Dependent If the path for aNew begins with /Sys then Tcb capability is required.
@capability Dependent If the path for aNew begins with /Private and does not match
					  this process' SID then AllFiles capability is required.
@capability Dependent If the path for aNew begins with /Resource then Tcb capability is required.
*/
	{
	OstTraceExt3(TRACE_BORDER, EFSRV_ECFILEMANCOPY3, "this %x anOldSubs %x aSwitches %x", (TUint) this, (TUint) anOld.SubSessionHandle(), (TUint) aSwitches);
	OstTraceData(TRACE_BORDER, EFSRV_ECFILEMANCOPY3_ENEWNAME, "NewName %S", aNew.Ptr(), aNew.Length()<<1);

	TInt ret;
	if (iSwitches&KFManBusyFlag)
		{
		ret = KErrInUse;
		}
		// The only switch that is legal for single file copies is EOverWrite
	else if ((aSwitches & ~EOverWrite) != 0)
		{
		ret = KErrArgument;
		}
	else
		{

		SetFlags(aSwitches & EOverWrite, EFalse, EFalse, EFalse);

		// need to signal to CFileBase that we're copying from a handle
		// and that iSrcFile is invalid
		iSwitches|= KCopyFromHandle;

		TInt r;
		if ((r = iFs.Parse(aNew, iTrgFile)) != KErrNone)
			{
			if(iStatus)
				User::RequestComplete(iStatus,r);
			OstTrace1(TRACE_BORDER, EFSRV_ECFILEMANCOPY3RETURN1, "r %d", r);
			return r;
			}

		// Need to duplicate the RFile handle so that any threads owned
		// by this process can use it - i.e. the worker thread
		if ((r = iSrcFileHandle.Duplicate(anOld, EOwnerProcess)) != KErrNone)
			{
			if(iStatus)
				User::RequestComplete(iStatus,r);
			OstTrace1(TRACE_BORDER, EFSRV_ECFILEMANCOPY3RETURN2, "r %d", r);
			return r;
			}

		iAction = EInternalCopyFromHandle;
		iNumberOfFilesProcessed = 0;
		TRAP(r,RunL());
		ret=(r==KErrNone) ? iLastError : r;
		DoSynchronize(r);
		}

	OstTrace1(TRACE_BORDER, EFSRV_ECFILEMANCOPY3RETURN3, "r %d", ret);

	return(ret);
	}

EXPORT_C TInt CFileMan::Copy(const RFile& anOld,const TDesC& aNew,TUint aSwitches,TRequestStatus& aStatus)
/**
Copies from an open file handle to a destination file name.

This is an asynchronous function.
Its behaviour is the same as the synchronous overload.

@param anOld     Open file handle indicating the file to be copied.
@param aNew      Path indicating the directory (and optionally the filename)
				 into which the file is to be copied.
				 Any path components which are not specified here will be
                 taken from the session path
@param aSwitches Specify zero for no overwriting;
                 CFileMan::EOverWrite to overwrite files with the same name;
                 Any other flags are illegal.

@param aStatus   The request status object. On request completion,
                 indicates how the request completed:
                 KErrNone, if successful, otherwise one of the other system-wide error
                 codes.

@return KErrNone if the asynchronous request is made successfully; KErrInUse if an asynchronous request
					is still pending; otherwise one of the other system-wide error codes

@see CFileBase::GetLastError()

@capability Dependent If the path for aNew begins with /Sys then Tcb capability is required.
@capability Dependent If the path for aNew begins with /Private and does not match
					  this process' SID then AllFiles capability is required.
@capability Dependent If the path for aNew begins with /Resource then Tcb capability is required.
*/
	{
	OstTraceExt4(TRACE_BORDER, EFSRV_ECFILEMANCOPY4, "this %x anOldSubs %x aSwitches %dstatus %x", (TUint) this, (TUint) anOld.SubSessionHandle(), (TUint) aSwitches, (TUint) &aStatus);
	OstTraceData(TRACE_BORDER, EFSRV_ECFILEMANCOPY4_ENEWNAME, "NewName %S", aNew.Ptr(), aNew.Length()<<1);

	TInt r;
	if (iSwitches&KFManBusyFlag)
		{
		r = KErrInUse;
		}
	else
		{
		iStatus=&aStatus;
		r = Copy(anOld,aNew,aSwitches);
		}

	OstTrace1(TRACE_BORDER, EFSRV_ECFILEMANCOPY4RETURN, "r %d", r);

	return(r);
	}

void CFileMan::DoCopyFromHandleL()
//
// Copy from open file handle
//
	{
	TInt ret=0;
	TFileName& trgName = iTmpName1;

	if (iTrgFile.NamePresent())
		{
		trgName = iTrgFile.FullName();
		}
	else
		{
		iSrcFileHandle.Name(trgName);
		if ((trgName.Length() + iTrgFile.DriveAndPath().Length()) > KMaxFileName)
			{
			iSrcFileHandle.Close();
			User::Leave(KErrBadName);
			}
		trgName.Insert(0, iTrgFile.DriveAndPath());
		}

#ifndef SYMBIAN_ENABLE_64_BIT_FILE_SERVER_API
	RFile trgFile;
#else
	RFile64 trgFile;
#endif
	TInt r=KErrNone;

	if ((iSwitches&KOverWriteFlag)==0)
		r=trgFile.Create(iFs,trgName,EFileWrite|EFileWriteDirectIO|EFileShareExclusive);
	else
		r=trgFile.Replace(iFs,trgName,EFileWrite|EFileWriteDirectIO|EFileShareExclusive);
	if (r!=KErrNone)
		iErrorInfo = ETrgOpenFailed;

	if (r == KErrNone)
		r = DoCopy(iSrcFileHandle, trgFile, ret);

	// close the (duplicated) source file handle
	iSrcFileHandle.Close();

	trgFile.Close();
	if (ret == MFileManObserver::ECancel || (r!=KErrNone && r!=KErrAlreadyExists && iErrorInfo!=ETrgOpenFailed))
		iFs.Delete(trgName);
	User::LeaveIfError(r);
	}

#ifndef SYMBIAN_ENABLE_64_BIT_FILE_SERVER_API
TInt CFileMan::DoCopy(const RFile& aSrcFile, RFile& aDstFile, TInt& aRet)
	{
	TInt rem;
#else
TInt CFileMan::DoCopy(const RFile64& aSrcFile, RFile64& aDstFile, TInt& aRet)
	{
	TInt64 rem;
#endif
	TInt r;
	if ((r = aSrcFile.Size(rem)) != KErrNone)
		{
		if(iStatus)
			User::RequestComplete(iStatus,r);
		return r;
		}


	if ((r = aDstFile.SetSize(rem)) != KErrNone)
		{
		if(iStatus)
			User::RequestComplete(iStatus,r);
		return r;
		}

	HBufC8* bufPtr = NULL;
	bufPtr = AllocateBuffer(rem);
	if (bufPtr == NULL)
		return KErrNoMemory;
	TPtr8 copyBuf=bufPtr->Des();

#ifndef SYMBIAN_ENABLE_64_BIT_FILE_SERVER_API
	TInt pos=0;
#else
	TInt64 pos=0;
#endif
	aRet = MFileManObserver::EContinue;
	while(rem && aRet == MFileManObserver::EContinue)
		{
#ifndef SYMBIAN_ENABLE_64_BIT_FILE_SERVER_API
		TInt s=Min(rem,copyBuf.MaxSize());
#else
		// Min result shall be of TInt size
		TInt s=(TInt)(Min(rem,(TInt64)copyBuf.MaxSize()));
#endif
		r=aSrcFile.Read(pos,copyBuf,s);
		if (r==KErrNone && copyBuf.Length()!=s)
			r = KErrCorrupt;
		if (r==KErrNone)
			r=aDstFile.Write(pos,copyBuf,s);
		if (r!=KErrNone)
			break;
		pos+= s;
		rem-= s;
		iBytesTransferred = s;
		aRet = (iObserver) ? iObserver->NotifyFileManOperation() : MFileManObserver::EContinue;
		if (aRet != MFileManObserver::EContinue && aRet != MFileManObserver::ECancel)
			Panic(EFManBadValueFromObserver);
		}

	// need to flush the target file - otherwise if there is any dirty data this will be flushed
	// when the file is closed and this will set the archive attribute, resulting in the file
	// having potentially a different attribute from the source file
	if (r == KErrNone)
		r = aDstFile.Flush();

	if (aRet != MFileManObserver::ECancel)
		{
		TTime lastMod;
		if (r == KErrNone)
			r = aSrcFile.Modified(lastMod);
		if (r == KErrNone)
			r = aDstFile.SetModified(lastMod);

		TUint fileAttributes=0;
		if (r == KErrNone)
			r = aSrcFile.Att(fileAttributes);
		if (r == KErrNone)
			r = aDstFile.SetAtt(fileAttributes,(~fileAttributes)&KEntryAttMaskSupported);

		if(r == KErrNone)
  			r = aDstFile.Flush();
		}

	delete bufPtr;

	return r;
	}
