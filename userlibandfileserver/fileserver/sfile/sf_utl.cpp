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
// f32\sfile\sf_utl.cpp
// 
//

#include "sf_std.h"
#include <hal.h>
#include <collate.h>

const TInt KLog2BufGranularity=4; // 2^5 == 32

TBool ComparePaths(const TDesC& aPath1,const TDesC& aPath2)
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

TBool CompareResource(const TDesC & aThePath)
//
//compare function for the Resource path true for match
//
	{
	TInt pathLen = aThePath.Length();
	if(pathLen < KResourceLength)	
		return EFalse;
	//if not word aligned then no less efficient than treating as TUint16 
	const TUint32 * path32 = reinterpret_cast<const TUint32*>(aThePath.Ptr());
	if( (*path32 & 0xFFDFFFFF) != 0x0052005c)			//	'/R' 
		return EFalse;
	path32++;
	if( (*path32 & 0xFFDFFFDF) != 0x00530045)			// 'ES'
		return EFalse;
	path32++;
	if( (*path32 & 0xFFDFFFDF) != 0x0055004F)			// 'OU'
		return EFalse;
	path32++;
	if( (*path32 & 0xFFDFFFDF) != 0x00430052)			// 'RC'
		return EFalse;
	path32++;
	if(pathLen > KResourceLength)
		{
		if( (*path32 & 0xFFFFFFDF) != 0x005c0045)			// 'E/'
			return EFalse;
		}
	else
		{
		if( (*path32 & 0x0000FFDF) != 0x00000045)			// 'E'
			return EFalse;
		}

	return ETrue;
	}

TBool CompareSystem(const TDesC & aThePath) 
//
//compare function for the system path true for match
//
	{
	TInt pathLen = aThePath.Length();
	if(pathLen < KSystemLength)	
		return EFalse;
	//if not word aligned then no less efficient than treating as TUint16 
	const TUint32 * path32 = reinterpret_cast<const TUint32*>(aThePath.Ptr());
	if( (*path32 & 0xFFDFFFFF) != 0x0053005c)			//	'/S' 
		return EFalse;
	path32++;
	if( (*path32 & 0xFFDFFFDF) != 0x00530059)			// 'YS
		return EFalse;
	if(pathLen == KSystemLength)
		return ETrue;
	path32++;
	if( (*path32 & 0x0000FFFF) != 0x0000005c)			// '/'
		return EFalse;

	return ETrue;
	}

TBool ComparePrivate(const TDesC & aThePath) 
//
//compare function to compare if private path being accessed true for match
//
	{
	TInt pathLen = aThePath.Length();
	if(pathLen < KPrivateLength)	
		return EFalse;
	const TUint32 * path32 = reinterpret_cast<const TUint32*>(aThePath.Ptr());

	if((*path32 & 0xFFDFFFFF) != 0x0050005c)			//	'/P' 
		return EFalse;
	path32++;
	if( (*path32 & 0xFFDFFFDF) != 0x00490052)			// 'RI
		return EFalse;
	path32++;
	if( (*path32 & 0xFFDFFFDF) != 0x00410056)			// 'VA'
		return EFalse;
	path32++;
	if( (*path32 & 0xFFDFFFDF) != 0x00450054)			// 'TE'
		return EFalse;
	if(pathLen == KPrivateLength)
		return ETrue;
	path32++;
	if( (*path32 & 0x0000FFFF) != 0x0000005c)			// '/'
		return EFalse;

	return ETrue;
	}

TBool SIDCheck(CFsRequest* aRequest, const TDesC& aThePath)
//
//	Compare the Private/XXXXXXXX/ portion of a path be accessed to make sure it matches the process SID 
//
	{
	if(aThePath.Length() >= KPrivateLengthCheck)
		{
		TSecureId appUID = aRequest->Message().SecureId();
		TBuf<KSIDLength+1> dirName;
		dirName.AppendNumFixedWidth(appUID.iId, EHex, 8);
	
		TInt match = dirName.CompareF(aThePath.Mid(KSIDPathOffset,KPrivateLength));
		if(match==KErrNone)
			return ETrue;
		else
			return EFalse;
		}
	
	return EFalse;
	}

#ifndef __REMOVE_PLATSEC_DIAGNOSTIC_STRINGS__
TInt PathCheck(CFsRequest* aRequest, const TDesC& aThePath, const TSecurityPolicy* aSysCap, const TSecurityPolicy* aPriCap, const TSecurityPolicy* aROCap, const char* aDiag)
#else //__REMOVE_PLATSEC_DIAGNOSTIC_STRINGS__
TInt PathCheck(CFsRequest* aRequest, const TDesC& aThePath, const TSecurityPolicy* aSysCap, const TSecurityPolicy* aPriCap, const TSecurityPolicy* aROCap, OnlyCreateWithNull /*aDiag*/)
#endif //!__REMOVE_PLATSEC_DIAGNOSTIC_STRINGS__
//
//	Compare the parsed path with protected path names path must be parsed b4 using
//
	{

	if(aRequest->Message().Handle() == KLocalMessageHandle)
		return KErrNone;

#ifndef __REMOVE_PLATSEC_DIAGNOSTIC_STRINGS__
#ifdef _DEBUG
	TBuf8<512> diagmsg;
	TInt len = aThePath.Length();
	diagmsg.Append((TUint8*)aThePath.Ptr(),len*2);
	diagmsg.Collapse();
	diagmsg.SetLength(len);
	diagmsg.Append(_L(" Used to call: "));
	len = User::StringLength((const TUint8*)aDiag);
	diagmsg.Append((TUint8*)aDiag, len);
	const char* const diagout = (char*)diagmsg.PtrZ();
#else //!_DEBUG
	const char* const diagout = aDiag;
#endif //_DEBUG
#endif //!__REMOVE_PLATSEC_DIAGNOSTIC_STRINGS__

	if(ComparePrivate(aThePath))
		{	
		if(SIDCheck(aRequest, aThePath))
			return KErrNone;	
		else
			{
			if(aPriCap->CheckPolicy(aRequest->Message(), __PLATSEC_DIAGNOSTIC_STRING(diagout)))
				return KErrNone;
			else
				return KErrPermissionDenied;
			}
		}
	else if(CompareSystem(aThePath))
		{
		if(aSysCap->CheckPolicy(aRequest->Message(), __PLATSEC_DIAGNOSTIC_STRING(diagout)))
			return KErrNone;
		else
			return KErrPermissionDenied;
		}
	else if(CompareResource(aThePath))
		{
		if(aROCap->CheckPolicy(aRequest->Message(), __PLATSEC_DIAGNOSTIC_STRING(diagout)))
			return KErrNone;
		else
			return KErrPermissionDenied;
		}
	else
		return KErrNone;
 	}

#ifndef __REMOVE_PLATSEC_DIAGNOSTIC_STRINGS__
TInt PathCheck(CFsRequest* aRequest, const TDesC& aThePath, const TSecurityPolicy* aSysCap, const TSecurityPolicy* aPriCap, const char* aDiag) 
#else //__REMOVE_PLATSEC_DIAGNOSTIC_STRINGS__
TInt PathCheck(CFsRequest* aRequest, const TDesC& aThePath, const TSecurityPolicy* aSysCap, const TSecurityPolicy* aPriCap, OnlyCreateWithNull /*aDiag*/) 
#endif //!__REMOVE_PLATSEC_DIAGNOSTIC_STRINGS__
//
//	Compare the parsed path with protected path names path must be parsed b4 using
//
	{

	if(aRequest->Message().Handle() == KLocalMessageHandle)
		return KErrNone;

#ifndef __REMOVE_PLATSEC_DIAGNOSTIC_STRINGS__
#ifdef _DEBUG
	TBuf8<512> diagmsg;
	TInt len = aThePath.Length();
	diagmsg.Append((TUint8*)aThePath.Ptr(),len*2);
	diagmsg.Collapse();
	diagmsg.SetLength(len);
	diagmsg.Append(_L(" Used to call: "));
	len = User::StringLength((const TUint8*)aDiag);
	diagmsg.Append((TUint8*)aDiag, len);
	const char* const diagout = (char*)diagmsg.PtrZ();
#else //!_DEBUG	
	const char* const diagout = aDiag;
#endif //_DEBUG
#endif //!__REMOVE_PLATSEC_DIAGNOSTIC_STRINGS__

	if(ComparePrivate(aThePath))
		{	
		if(SIDCheck(aRequest, aThePath))
			return KErrNone;	
		else
			{
			if(aPriCap->CheckPolicy(aRequest->Message(), __PLATSEC_DIAGNOSTIC_STRING(diagout)))
				return KErrNone;
			else
				return KErrPermissionDenied;
			}
		}
	else if(CompareSystem(aThePath))
		{
		if(aSysCap->CheckPolicy(aRequest->Message(), __PLATSEC_DIAGNOSTIC_STRING(diagout)))
			return KErrNone;
		else
			return KErrPermissionDenied;
		}
	else
		return KErrNone;
 	}

#ifndef __REMOVE_PLATSEC_DIAGNOSTIC_STRINGS__
TInt PathCheck(CFsRequest* aRequest, const TDesC& aThePath, const TSecurityPolicy* aCap, const char* aDiag, TBool aExactMatchAllowed) 
#else //__REMOVE_PLATSEC_DIAGNOSTIC_STRINGS__
TInt PathCheck(CFsRequest* aRequest, const TDesC& aThePath, const TSecurityPolicy* aCap, OnlyCreateWithNull /*aDiag*/, TBool aExactMatchAllowed) 
#endif //!__REMOVE_PLATSEC_DIAGNOSTIC_STRINGS__
//
//	Compare the parsed path with protected path names path must be parsed b4 using
//
	{

	if(aRequest->Message().Handle() == KLocalMessageHandle)
		return KErrNone;

#ifndef __REMOVE_PLATSEC_DIAGNOSTIC_STRINGS__
#ifdef _DEBUG
	TBuf8<512> diagmsg;
	TInt len = aThePath.Length();
	diagmsg.Append((TUint8*)aThePath.Ptr(),len*2);
	diagmsg.Collapse();
	diagmsg.SetLength(len);
	diagmsg.Append(_L(" Used to call: "));
	len = User::StringLength((const TUint8*)aDiag);
	diagmsg.Append((TUint8*)aDiag, len);
	const char* const diagout = (char*)diagmsg.PtrZ();
#else //!_DEBUG	
	const char* const diagout = aDiag;
#endif //_DEBUG
#endif //!__REMOVE_PLATSEC_DIAGNOSTIC_STRINGS__

	if(ComparePrivate(aThePath))
		{	
		if(SIDCheck(aRequest, aThePath))
			return KErrNone;	
		else
			{
			if(aCap->CheckPolicy(aRequest->Message(), __PLATSEC_DIAGNOSTIC_STRING(diagout)))
				return KErrNone;
			else if (aExactMatchAllowed && aThePath.Length() <= KPrivateLength + 1)
				return KErrNone;
			else
				return KErrPermissionDenied;
			}
		}
	else if(CompareSystem(aThePath))
		{
		if(aCap->CheckPolicy(aRequest->Message(), __PLATSEC_DIAGNOSTIC_STRING(diagout)))
			return KErrNone;
		else if (aExactMatchAllowed && aThePath.Length() <= KSystemLength + 1)
			return KErrNone;
		else
			return KErrPermissionDenied;
		}
	else
		return KErrNone;
	
	}




void Fault(TFsFault aFault)
//
// Fault the file server.
//
	{

	__PRINT1(_L("FAULT: TFsFault %d"),aFault);
	User::Panic(_L("Fserv fault"),aFault);
	}

/**
    Get Ptr() from the client and parse it. Allow wild cards.
    Then get the drive and if the drive is substed get the real name.
    
    @param  aP          	message parameter number
    @param  aRequest    	pointer to the reques object
    @param  aParse      	parser
    @param  aUseSessionPath flag specifying whether the session path is to be
     						used for parsing or not. Default value is ETrue meaning
     						that the session path is used while parsing.

    @return     system-wide error code.
*/
TInt ParseSubst(const TInt aP, CFsRequest* aRequest,TParse& aParse, TBool aUseSessionPath)
	{
	__ASSERT_DEBUG(aRequest->Session(),Fault(EParseSubstSession));
	
	//-- check the length of the name, passed by client.
	//-- it shall not be longer than KMaxFileName
	TInt nameLen=aRequest->GetDesLength(aP);
	if(nameLen < 0 || nameLen > KMaxFileName)
	    return KErrBadName;
	
	TFileName n;
	TRAPD(r, aRequest->ReadL(aP,n));
	
	if(r==KErrNone)
		{
		if(aUseSessionPath)
			r=aParse.Set(n,NULL,&aRequest->Session()->Path());
		else
			r=aParse.Set(n,NULL,NULL);
		}
	if (r!=KErrNone)
		return(r);
	if(!aUseSessionPath && !aParse.DrivePresent())
		return(r);
	TInt drive;
	if ((r=RFs::CharToDrive(aParse.Drive()[0],drive))!=KErrNone)
		return(r);
	aRequest->SetDrive(&TheDrives[drive]);
	if (aRequest->Drive()->Att()&KDriveAttSubsted)
		{
		if ((aRequest->Drive()->Subst().Length()+aParse.FullName().Length())>(KMaxFileName+3))
			return(KErrBadName);
		aRequest->SetSubstedDrive(aRequest->Drive());
		// and now set aParse with the full path name
		n=aRequest->Drive()->Subst().Mid(2);
		n+=aParse.FullName().Mid(3);
		TFileName n2=aRequest->SubstedDrive()->Subst().Left(2);
		r=aParse.Set(n,NULL,&n2);
		if(r!=KErrNone)
			return(r);
		aRequest->SetDrive(&aRequest->Drive()->SubstedDrive());
		}
	return(KErrNone);
	}


TInt ParseSubstPtr0(CFsRequest* aRequest,TParse& aParse, TBool aUseSessionPath)
//
// Get Ptr0() from the client and parse it. Allow wild cards.
// Then get the drive and if the drive is substed get the real name.
//
	{

	return(ParseSubst(KMsgPtr0,aRequest,aParse,aUseSessionPath));
	}

TInt ParseNoWildSubstPtr0(CFsRequest* aRequest,TParse& aParse, TBool aUseSessionPath)
//
// Get Ptr0() from the client and parse it. Dis-allow wild cards.
// Then get the drive and if the drive is substed get the real name.
//
	{
	TInt r=ParseSubst(KMsgPtr0,aRequest,aParse,aUseSessionPath);
	if (r!=KErrNone)
		return(r);	//	Returns KErrBadName if "/" in the name or length > 256 chars
	
	if (aParse.IsWild())	//	No *s or ?s allowed in the file name
		return(KErrBadName);
	return(KErrNone);
	}


TInt ParseNoWildSubstPtr1(CFsRequest* aRequest,TParse& aParse)
//
// Get Ptr0() from the client and parse it. Dis-allow wild cards.
// Then get the drive and if the drive is substed get the real name.
//
	{
	TInt r=ParseSubst(KMsgPtr1, aRequest, aParse);
	if (r!=KErrNone)
		return(r);	//	Returns KErrBadName if "/" in the name or length > 256 chars
	
	if (aParse.IsWild())	//	No *s or ?s allowed in the file name
		return(KErrBadName);
	return(KErrNone);
	}


TInt ParseNoWildSubstCheckPathPtr0(CFsRequest* aRequest,TParse& aParse)
//
// Get Ptr0() from the client and parse it.
// Then get the drive and if the drive is substed get the real name.
// Dis-allow wild cards and the root directory
//
	{

	TInt r=ParseSubst(KMsgPtr0,aRequest,aParse);
	if (r!=KErrNone)
		return(r);
	if (aParse.IsWild() || (aParse.IsRoot() && !aParse.NameOrExtPresent()))
		return(KErrBadName);
	return(KErrNone);
	}

TInt ParseNoWildSubstCheckPathPtr1(CFsRequest* aRequest,TParse& aParse)
//
// Get Ptr1() from the client and parse it.
// Then get the drive and if the drive is substed get the real name.
// Dis-allow wild cards and the root directory
//
	{

	TInt r=ParseSubst(KMsgPtr1,aRequest,aParse);
	if (r!=KErrNone)
		return(r);
	if (aParse.IsWild() || (aParse.IsRoot() && !aParse.NameOrExtPresent()))
		return(KErrBadName);

	return(KErrNone);
	}

TInt ParseNoWildSubstFileCheckPtr1(CFsRequest* aRequest,TParse& aParse)
//
// Get Ptr0() from the client and parse it.
// Then get the drive and if the drive is substed get the real name.
// Dis-allow wild cards and the root directory
// Finally check that there is a name or extension.
	{

	TInt r=ParseSubst(KMsgPtr1,aRequest,aParse);
	if (r!=KErrNone)
		return(r);
	if (aParse.IsWild() || (aParse.IsRoot() && !aParse.NameOrExtPresent()))
		return(KErrBadName);
	if(!aParse.NameOrExtPresent())
	    return (KErrBadName);
	return(KErrNone);
	}

TInt ParseNoWildSubstCheckPtr0(CFsRequest* aRequest,TParse& aParse, TBool aUseSessionPath)
//
// Get Ptr0() from the client and parse it. Dis-allow wild cards.
// Then get the drive and if the drive is substed get the real name.
// Finally check that there is a name or extension.
//
	{

	TInt r=ParseNoWildSubstPtr0(aRequest,aParse, aUseSessionPath);	
	if (r!=KErrNone)
		return(r);
	if (!aParse.NameOrExtPresent())
		return(KErrBadName);
	return(KErrNone);
	}

TInt ParseNoWildSubstCheckPtr1(CFsRequest* aRequest,TParse& aParse)
//
// Get Ptr1() from the client and parse it. Dis-allow wild cards.
// Then get the drive and if the drive is substed get the real name.
// Finally check that there is a name or extension.
//
	{

	TInt r=ParseNoWildSubstPtr1(aRequest,aParse);
	if (r!=KErrNone)
		return(r);
	if (!aParse.NameOrExtPresent())
		return(KErrBadName);
	return(KErrNone);
	}

/**
    Get Ptr0() from the client and parse it.

    @param  aRequest    pointer to the reques object
    @param  aParse      parser

    @return     system-wide error code.
*/
TInt ParsePathPtr0(CFsRequest* aRequest,TParse& aParse)
	{
	//-- check the length of the name, passed by client.
	//-- it shall not be longer than KMaxFileName
	TInt nameLen=aRequest->GetDesLength(KMsgPtr0);
	if(nameLen < 0 || nameLen > KMaxFileName)
	    return KErrBadName;

	TFileName n;
	TRAPD(r,aRequest->ReadL(KMsgPtr0,n));
	if (r==KErrNone)
		r=aParse.SetNoWild(n,NULL,&aRequest->Session()->Path());
	if (r!=KErrNone)
		return(r);
	if (aParse.NameOrExtPresent())
		return(KErrBadName);
	TInt drive;
	if ((r=RFs::CharToDrive(aParse.Drive()[0],drive))!=KErrNone)
		return(r);
	aRequest->SetDrive(&TheDrives[drive]);
	return(KErrNone);
	}


TInt ParseNotificationPath(CFsRequest* aRequest, TParse& aParse, TDes& aNotifyPath)
//
//	Called by Notify Change extended function when client has submitted a path
//	which contains a wildcarded initial character to represent the drive
//	This is required when notification over a number of drives is required
//
	{
	if ((aNotifyPath[0]==KMatchAny)||(aNotifyPath[0]==KMatchOne))
		{
	//	Use the default session drive for now
		TFileName sessionDefault=aRequest->Session()->Path();
		aNotifyPath[0]=sessionDefault[0];
		}

	TInt r=aParse.Set(aNotifyPath,NULL,&aRequest->Session()->Path());
	if (r!=KErrNone)
		return(r);
	TInt drive;
	if ((r=RFs::CharToDrive(aParse.Drive()[0],drive))!=KErrNone)
		return(r);
	aRequest->SetDrive(&TheDrives[drive]);
	if (aRequest->Drive()->Att()&KDriveAttSubsted)
		{
		if ((aRequest->Drive()->Subst().Length()+aParse.FullName().Length())>(KMaxFileName+3))
			return(KErrBadName);
		aRequest->SetSubstedDrive(aRequest->Drive());
		// and now set aParse with the full path name
		TFileName n=aRequest->Drive()->Subst().Mid(2);
		n+=aParse.FullName().Mid(3);
		TFileName n2=aRequest->SubstedDrive()->Subst().Left(2);
		r=aParse.Set(n,NULL,&n2);
		if(r!=KErrNone)
			return(r);
		aRequest->SetDrive(&aRequest->Drive()->SubstedDrive());
		}	
	if (aParse.IsWild())
		return(KErrBadName);
	return(KErrNone);
	}


CFsObject* SessionObjectFromHandle(TInt aHandle,TInt aUniqueID, CSessionFs* aSession)
//
// Lookup an object from its handle.
//
	{
	if(aUniqueID==0)
		return(aSession->Handles().At(aHandle,ETrue));
	else
		return(aSession->Handles().At(aHandle,aUniqueID,ETrue));
	}


CFileShare* GetShareFromHandle(CSessionFs* aSession, TInt aHandle)
//
// Get the share control block from its handle.
//
	{
	return((CFileShare*)(SessionObjectFromHandle(aHandle,FileShares->UniqueID(),aSession)));
	}



/**
    @return ETrue if aDes only contains spaces or is zero length
    Note that _all_ UNICODE space characters are treated as usual spaces
*/
static TBool IsSpace(const TDesC& aDes)
	{

	TInt len=aDes.Length();
	if (len==0)
		return(EFalse);
	for (TInt i=0;i<len;i++)
		{
		TChar txt=aDes[i];
		if (!txt.IsSpace())
			return(EFalse);
		}
	return(ETrue);
	}

TUint32 CalcNameHash(const TDesC& aName) 
	{
	const TUint32 KNameHashCRCInitialiser=0x12345678;
	TUint32 hash=KNameHashCRCInitialiser;
	Mem::Crc32(hash, aName.Ptr(), aName.Size());
	return hash;
	}


TBool TNameChecker::IsIllegal(TText& aChar) const
//
// Checks aChar != any of < > : " / |
// \ is also illegal in a name but will be considered a path delimiter
//
	{
	switch (aChar)
		{
	case '<':
	case '>':
	case ':':
	case '"':
	case '/':
	case '|':
	case '\000':
		return(ETrue);
	default:
		break;
		}
	return(EFalse);
	}



TBool TNameChecker::IsIllegalChar(TText& aChar)
//	
//	Checks aName for illegal components and returns the offending character if any
//	No other parsing is performed
//
	{
	TInt len=iName.Length();
	while (len--)
		{
		if (IsIllegal(iName[len]))// '<', '>', ':', '"', '/', '|' or '\000'?
			{
			aChar=iName[len];
			return(ETrue);
			}
		}
	return(EFalse);
	}


TBool TNameChecker::IsIllegalName()
//
// Checks name is not _L(".") or _L("..") and that there are no illegal characters
//
	{

	TInt pos=iName.LocateReverse(KPathDelimiter)+1;
	TPtrC fileName(iName.Ptr()+pos,iName.Length()-pos);
	SetName(fileName);
	
	if (iName==_L(".") || iName==_L("..") || IsSpace(iName))
		return ETrue;
	TInt len=iName.Length();
	
	while (len--)
		{
		if (IsIllegal(iName[len]))	//	'<', '>', ':', '"', '/', '|' or '\000'?
			return(ETrue);
		}
	return(EFalse);
	}


TBool TNameChecker::IsIllegalName(TText& aChar)
//
//	Check name and path are legal - if not, return the offending component
//
	{
	TInt r=iParse.Set(iName,NULL,NULL);
	if (r!=KErrNone)
		return(ETrue);	//	Checks for names > 256 chars etc
	
	SetName(iParse.FullName());
	if (IsIllegalPath(aChar))
		return(ETrue);	//	Checks for illegal characters in path
	
	TInt nameStart=iParse.FullName().LocateReverse(KPathDelimiter)+1;
	r=(iParse.FullName().Mid(nameStart)).LocateReverse(KPathDelimiter)+1;
	TPtrC fileName(iName.Ptr()+r,iName.Length()-r);
	SetName(fileName);

	if (iName==_L(".") || iName==_L("..") || IsSpace(iName))
		{
		aChar=iName[0]; 
		return ETrue;
		}
	
	return (IsIllegalChar(aChar));
	}

const TInt KSpace = ' ';
TBool TNameChecker::IsIllegalPath()
//
// Checks the path does not contain wildcards or directories _L(".") or _L("..") or just spaces
//
	{

	if (iName.Locate(KMatchOne)!=KErrNotFound || iName.Locate(KMatchAny)!=KErrNotFound)
		return(ETrue);

	TLex pathLex=iName;
	FOREVER
		{
		pathLex.Inc();
		if (pathLex.Remainder().Length()==0)
			break;
		TInt nextPath=pathLex.Remainder().Locate(KPathDelimiter);
		if (nextPath==0) // Reject double backslashes
			return(ETrue);
		if (nextPath==KErrNotFound)
			nextPath=pathLex.Remainder().Length();
		pathLex.Mark();
		pathLex.Inc(nextPath);
		SetName(pathLex.MarkedToken());
		if (IsIllegalName())
			return(ETrue);
		// check for tailing dots
		for(TInt i = pathLex.MarkedToken().Length() - 1; i >= 0; --i)
			{
			if (pathLex.MarkedToken()[i] == KExtDelimiter)
				{
				return ETrue;
				}
			else if (pathLex.MarkedToken()[i] == KSpace)
				{
				continue;
				}
			else
				{
				break;
				}
			}
		}
	return(EFalse);
	}


TBool TNameChecker::IsIllegalPath(TText& aChar)
//
//	Checks the path does not contain wildcards or directories _L(".") or _L("..") or just spaces
//	Returns the first offending character found (if any)
//
	{
	if (iName.Locate(KMatchOne)!=KErrNotFound)
		{
		aChar=KMatchOne;
		return (ETrue);
		}
	if (iName.Locate(KMatchAny)!=KErrNotFound)
		{
		aChar=KMatchAny;
		return (ETrue);
		}

	TLex pathLex=iName;
	TFileName name;
	FOREVER
		{
		pathLex.Inc();
		if (pathLex.Remainder().Length()==0)
			break;
		TInt nextPath=pathLex.Remainder().Locate(KPathDelimiter);
		if (nextPath==0) // Reject double backslashes
			{
			aChar=KPathDelimiter; 
			return ETrue;
			}	
		if (nextPath==KErrNotFound)
			nextPath=pathLex.Remainder().Length();
		pathLex.Mark();
		pathLex.Inc(nextPath);
		name=pathLex.MarkedToken();
		// check for tailing dots
		for(TInt i = pathLex.MarkedToken().Length() - 1; i >= 0; --i)
			{
			if (pathLex.MarkedToken()[i] == KExtDelimiter)
				{
				aChar = KExtDelimiter;
				return ETrue;
				}
			else if (pathLex.MarkedToken()[i] == KSpace)
				{
				continue;
				}
			else
				{
				break;
				}
			}
		TInt pos=name.LocateReverse(KPathDelimiter)+1;
		TPtrC fileName(name.Ptr()+pos,name.Length()-pos);
		SetName(fileName);
		if (iName==_L(".") || iName==_L("..") || IsSpace(iName))
			{
			aChar=iName[0]; 
			return ETrue;
			}
		
		if (IsIllegalChar(aChar))
			return(ETrue);
		}
	
	return(EFalse);
	}


TBool IsIllegalFullName(const TParse& aParse)
	{
	TPtrC ptr=aParse.Path();
	TNameChecker checker(ptr);
	if (checker.IsIllegalPath())
		return(ETrue);	//	Checks for illegal characters in path
	
	TInt nameStart=aParse.FullName().LocateReverse(KPathDelimiter)+1;
	checker.SetName(aParse.FullName().Mid(nameStart));
	
	if (checker.IsIllegalName())
		return(ETrue);	//	Checks illegal characters such as ></|". and ..
	return(EFalse);
	}

 TBool IsIllegalFullName(const TDesC& aName)
//
// Check name and path are legal
//
	{
	TParse parser;
	TInt r=parser.Set(aName,NULL,NULL);
	if (r!=KErrNone)
		return(ETrue);	//	Checks for wild cards, names>256 chars etc
	
	return IsIllegalFullName(parser);
	}

TBool PowerOk()
//
// Check the power is OK
//
	{

	TBool powerGood=EFalse;
	TInt r=HAL::Get(HAL::EPowerGood, powerGood);
	if (r!=KErrNone)
		return EFalse;
	return powerGood;
	}

//---------------------------------------------------------------------------------------------------------------------------------
/**
    Decrement mount's lock counter when the mount resource, like a file or directory is opened. 
    See also: CMountCB::LockStatus()   
*/
void AddResource(CMountCB& aMount)
	{
	__CHECK_DRIVETHREAD(aMount.Drive().DriveNumber());
	__ASSERT_DEBUG(aMount.LockStatus()<=0,Fault(ERawDiskBadAccessCount2));
	aMount.DecLock();
	}

/**
    Increment mount's lock counter when the mount resource, like a file or directory is closed. 
    See also: CMountCB::LockStatus()   
*/
void RemoveResource(CMountCB& aMount)
	{
	__ASSERT_DEBUG(aMount.LockStatus()<0,Fault(ERawDiskBadAccessCount1));
	aMount.IncLock();
	}


/**
    Increment mount's lock counter when the disk access (Format, Raw disk) is opened on the mount
    See also: CMountCB::LockStatus()   
*/
void AddDiskAccess(CMountCB& aMount)
	{
	aMount.IncLock();
	}

/**
    Decrement mount's lock counter when the disk access (Format, Raw disk) is closed on the mount
    See also: CMountCB::LockStatus()   
*/
void RemoveDiskAccess(CMountCB& aMount)
	{
	aMount.DecLock();
	}

EXPORT_C void AllocBufferL(HBufC*& aBuf,const TDesC& aName)
//
// Alloc or ReAlloc buffer 
//
	{

	if (aBuf==NULL)
		{
		aBuf=aName.AllocL();
		return;
		}
	if (aBuf->Length()<aName.Length())
		{
		TInt bufGranularity=(1<<KLog2BufGranularity);
		TInt size=((aName.Length()+bufGranularity-1)>>KLog2BufGranularity)<<KLog2BufGranularity;
		aBuf=aBuf->ReAllocL(size);
		}
	aBuf->Des()=aName;
	}

void NextInPath(const TDesC& aPath,TPtrC& anEntry,TInt& aPos)
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

TInt MatchUidType(const TUidType &aMatch, const TUidType &aType)
//
// Compare aType against aMatch with KNullUid as a wildcard
//
	{

	TUid mu=aMatch[2];
	TUid tu=aType[2];
	if (mu!=KNullUid && tu!=KNullUid && mu!=tu)
		return KErrNotSupported;
	mu=aMatch[1];
	tu=aType[1];
	if (mu!=KNullUid && tu!=KNullUid && mu!=tu)
		return KErrNotSupported;
	mu=aMatch[0];
	tu=aType[0];
	if (mu!=KNullUid && tu!=KNullUid && mu!=tu)
		return KErrNotSupported;
	return KErrNone;
	}

/**
 * Compare sections of filenames according to locale-independent rules as well
 * as locale-dependent rules. This is an attempt to get an ordering where
 * equivalent names sort together but the overall ordering is culturally
 * acceptable. Sadly, the ordering it gives is not consistent, and it is not
 * clear how to make it so. Hence this function is deprecated. The correct way
 * to do this is to code around it: If you need a sorted array for display that
 * you also need to search for equivalent filenames, either use a linear search
 * or keep two arrays, one sorted with CompareF, the other with CompareC.
 * @deprecated 6.1
 */
EXPORT_C TInt CompareFilenames(const TDesC& aFileName1,const TDesC& aFileName2)
// 
// Compare filenames. Case is ignored and names are normalised
// (base+accent is equal to composed character) but spaces
// and punctuation are significant for determining equality.
// Whether two filenames are identical is the same in all
// locales, but the ordering of non-identical filenames is locale-specific.
//
//
	{
	// Create a non-locale-specific collation method that doesn't ignore spaces and punctuation but folds case.
	TCollationMethod method;
	method.iFlags = TCollationMethod::EIgnoreNone | TCollationMethod::EFoldCase;
	method.iMainTable=NULL;	
	method.iOverrideTable=NULL;	
	
	// Get the non-locale-specific order and return it if the names are equal.
	TInt base_order = aFileName1.CompareC(aFileName2,3,&method);
	if (base_order == 0)
		return base_order;

	// Get the locale-specific order and use it unless it is equality, in which case the non-locale-specific order must be used.
	TInt locale_order = aFileName1.CompareC(aFileName2);
	return locale_order ? locale_order : base_order;
	}

void Get8BitDllName(TDes8& aDllName, const TDesC& aFileName)
//
//	Convert a 16 bit name to an 8 bit name
//	No data loss because UNICODE Dlls are currently restricted to 8 bit names
//	No effect in 8 bit builds - just sets aDllName to aFileName
//
	{
	aDllName.SetLength(aFileName.Length());
	aDllName.Copy(aFileName);
	}								

void Get16BitDllName(TDes& aFileName, const TDesC8& aDllName)
//
//	Convert an 8 bit name to a 16 bit name - zero padded automatically
//	No effect in 8 bit builds - just sets aFileName to aDllName
//
	{
	aFileName.SetLength(aDllName.Length());
	aFileName.Copy(aDllName);	
	}								


/**
    Checks that there is sufficient disk space available to complete a task taking into account reserved space
 
    @param  aThreshold amount of required free space, in bytes
    @param  aRequest

    @return KErrNone        on success and if there are _strictly_more_ than aThreshold bytes available on the volume (taking into account reserved space)
            KErrDiskFull    on success and if there isn't enough space
            system-wide error code otherwise
*/
TInt CheckDiskSpace(TInt64 aThreshold, CFsRequest* aRequest)
	{
    const TInt KReservedSpace = aRequest->Drive()->ReservedSpace();
    const TInt KDriveNumber = aRequest->Drive()->DriveNumber();

	if(KReservedSpace == 0 || KDriveNumber == EDriveZ)
	    return KErrNone;

    ASSERT(aThreshold);

    //-- if the drive has a reserved space, take it into account
	CSessionFs* session=aRequest->Session(); 

    if(!session || !session->ReservedAccess(KDriveNumber))
        aThreshold += KReservedSpace;

    //-- ask the corresponding file system if there is aThreshold bytes available.
    return aRequest->Drive()->RequestFreeSpaceOnMount(aThreshold);
	}								


#if defined(_DEBUG) || defined(_DEBUG_RELEASE)
EXPORT_C TUint32 DebugRegister() {return(DebugReg);}
#else
EXPORT_C TUint32 DebugRegister() {return(0);}
#endif

CLogon* CLogon::NewL()
//
// Create the CLogon active object.
//
	{

	CLogon* pL=new(ELeave) CLogon(EPriority);
	CActiveScheduler::Add(pL);
	return(pL);
	}

#pragma warning( disable : 4705 )	// statement has no effect
CLogon::CLogon(TInt aPriority)
//
// Constructor
//
	: CActive(aPriority)
	{
	}
#pragma warning( default : 4705 )

TInt CLogon::Logon(RThread aThread)
//
// Issue a request to logon to the thread.
//
	{

	iThread=aThread;
	iThread.Logon(iStatus);
		SetActive();
	if (iStatus==KErrNoMemory)
		iThread.Kill(KErrNoMemory);
	else
		iThread.Resume();
	iThread.Close();
	CActiveScheduler::Start();
	return(iStatus.Int());
	}

void CLogon::DoCancel()
//
// Cancel a pending event.
//
	{

	iThread.LogonCancel(iStatus);
	}

void CLogon::RunL()
//
// Thread has terminated.
//
	{

	CActiveScheduler::Stop();
	}


//---------------------------------------------------------------------------------------------------------------------------------
/**
    formats a string to represent a drive section in the estart.txt file by its number. Like "DriveD"
    @param  aDrvNum         drive number
    @param  aSectionName    out: formatted string
*/
void  F32Properties::GetDriveSection(TInt aDrvNum, TDes8& aSectionName)
    {
    ASSERT(aDrvNum >= EDriveA && aDrvNum <= EDriveZ);
    _LIT8(KLitSectionNameDrive,"Drive%C");
	aSectionName.Format(KLitSectionNameDrive, 'A' + aDrvNum);
    }




