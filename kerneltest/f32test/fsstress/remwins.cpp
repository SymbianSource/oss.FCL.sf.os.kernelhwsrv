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
// f32test\fsstress\remwins.cpp
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

#include "t_remfsy.h"


/*
#error Following code assumes %EPOCROOT% == '\\'
#if (defined(_DEBUG) && defined(_UNICODE))
GLDEF_D const TFileName ZPath=_L("\\EPOC32\\RELEASE\\WINS\\UDEB\\Z");
#elif defined(_DEBUG)
GLDEF_D const TFileName ZPath=_L("\\EPOC32\\RELEASE\\WINS\\DEB\\Z");
#elif defined(_UNICODE)
GLDEF_D const TFileName ZPath=_L("\\EPOC32\\RELEASE\\WINS\\UREL\\Z");
#else
GLDEF_D const TFileName ZPath=_L("\\EPOC32\\RELEASE\\WINS\\REL\\Z");
#endif
*/


GLDEF_C TBool GetEnvValue(TInt /*aDrive*/,TDes& /*aDes*/)
//
// Return a pointer to the value of the environment variable selected by aDrive
//
	{
	return(ETrue);
	}

GLDEF_C TBool MapDrive(TDes& aFileName,TInt aDrive)
//
// Map aDrive to a path given by environment variables
//
	{
	TFileName aName;
//	TBuf<16> dumName=_L("Dum.txt");
//	TText* dumPtr;
///	GetFullPathName((_STRC)dumName.PtrZ(),aName.MaxLength(),(_STR)aName.Ptr(),(_STR*)&dumPtr);
	aName.SetLength(aName.MaxLength());
	aFileName=_L("?:");
	aFileName[0]=aName[0];
//
//	TFileName envValue;
//	if (GetEnvValue(aDrive,envValue))
//		{
//		if (envValue.Length()<=0 || envValue[0]!='\\')
//			aFileName=envValue;
//		else
//			aFileName+=envValue;
//		return(ETrue);
//		}
//
    //switch (aDrive)
	//	{
//	case EDriveC:
//#error Following code assumes %EPOCROOT% == '\\'
//		aFileName+=_L("\\EPOC32\\WINS\\C");
//		return(ETrue);
//	case EDriveZ:
//		aFileName+=ZPath;
//		return(ETrue);
	//default:

	TChar drive;
	RFs::DriveToChar(aDrive,drive);
	aFileName=_L("?:");
	aFileName[0]=(TUint8)drive;
//	break;
//		}
	
	return(EFalse);
	}

GLDEF_C TBool MapDriveInfo(TDriveInfo& anInfo,TInt aDrive)
//
// Get Fake drive info.
//
	{

	if (aDrive==EDriveZ)
		{
		anInfo.iType=EMediaRom;
		anInfo.iMediaAtt=KMediaAttWriteProtected;
		return(ETrue);
		}
	if (aDrive==EDriveC)
		{
		anInfo.iType=EMediaHardDisk;
		anInfo.iMediaAtt=KMediaAttVariableSize|KMediaAttFormattable;
		return(ETrue);
		}
	TFileName envValue;
	if (GetEnvValue(aDrive,envValue))
		{
		anInfo.iType=EMediaRemote;
		anInfo.iMediaAtt=0;
		return(ETrue);		
		}
	return(EFalse);
	}

GLDEF_C TBool MapDriveAttributes(TUint& aDriveAtt,TInt aDrive)
//
// Get Fake drive attributes.
//
	{
	if (aDrive==EDriveZ)
		{
		aDriveAtt=KDriveAttRom|KDriveAttInternal;
		return(ETrue);
		}
	if (aDrive==EDriveC)
		{
		aDriveAtt=KDriveAttRemovable|KDriveAttInternal;
		return(ETrue);
		}
	TFileName envValue;
	if (GetEnvValue(aDrive,envValue))
		{
		aDriveAtt=KDriveAttRemote;
		return(ETrue);		
		}
	return(EFalse);
	}


GLDEF_C void CheckAppendL(TDes& aTarget,const TDesC& aSrc)
//
// Leaves KErrBadName if aSrc cannot be appended to aTarget
//
	{

	if (aTarget.MaxLength()<=aTarget.Length()+aSrc.Length())
		User::Leave(KErrBadName);
	}

