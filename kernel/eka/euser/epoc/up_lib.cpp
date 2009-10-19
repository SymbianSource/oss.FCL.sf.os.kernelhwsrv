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
// e32\euser\epoc\up_lib.cpp
// 
//

#include "up_std.h"
#include <e32uid.h>
#include "u32std.h"
#include <e32svr.h>
#include <e32panic.h>

//#define __DEBUG_IMAGE__ 1

#if defined(__DEBUG_IMAGE__) && defined (__EPOC32__)
#include "e32svr.h" 

extern RDebug debug;
#define __IF_DEBUG(t) {debug.t;}
#else
#define __IF_DEBUG(t)
#endif

#if defined(_UNICODE)
#define __SIZE(len) ((len)<<1)
#else
#define __SIZE(len) (len)
#endif




/**
@internalComponent
*/
EXPORT_C TInt RLibrary::LoadRomLibrary(const TDesC& aFileName, const TDesC& aPath)
	{
	return Load(aFileName,aPath);
	}




/**
Loads the named DLL which matches the specified UID type.

If successful, the function increments the usage count by one.
No additional search paths can be specified with this function.

@param aFileName A descriptor containing the name of the DLL to be loaded.
                 The length of the file name must not be greater than KMaxFileName.
@param aType     A UID type (a triplet of UIDs) which the DLL must match.
                 Individual UIDs can contain the KNullUid wild card. 

@return KErrNone, if successful;
        KErrBadName, if the length of aFileName is greater than KMaxFileName;
        otherwise one of the other system wide error codes.
*/
EXPORT_C TInt RLibrary::Load(const TDesC& aFileName, const TUidType& aType)
	{

	return Load(aFileName, KNullDesC, aType);
	}




/**
Loads the named DLL.

If successful, the function increments the usage count by one.

@param aFileName A descriptor containing the name of the DLL to be loaded.
                 The length of the file name must not be greater than KMaxFileName.
@param aPath     A descriptor containing a list of path names, each separated by
                 a semicolon. When specified, these paths are searched for the DLL
                 before the standard search locations. By default,
                 no pathnames are specified. 

@return KErrNone, if successful;
        KErrBadName, if the length of aFileName is greater than KMaxFileName;
        otherwise one of the other system wide error codes.
*/
EXPORT_C TInt RLibrary::Load(const TDesC& aFileName, const TDesC& aPath)
	{

	return Load(aFileName, aPath, TUidType());
	}




/**
Loads the named DLL that matches the specified UID type.

If successful, the function increments the usage count by one.

@param  aFileName A descriptor containing the name of the DLL to be loaded.
                  The length of the file name must not be greater than KMaxFileName.
@param aPath      A descriptor containing a list of path names, each separated by
                  a semicolon. When specified, these paths are searched for the DLL
                  before the standard search locations.
@param aType      A UID type (a triplet of UIDs) that the DLL must match. Individual UIDs
                  can contain the KNullUid wild card. 

@return KErrNone, if successful;
        KErrBadName, if the length of aFileName is greater than KMaxFileName;
        otherwise one of the other system wide error codes.
*/
EXPORT_C TInt RLibrary::Load(const TDesC& aFileName, const TDesC& aPath, const TUidType& aType)
	{
	return Load(aFileName, aPath, aType, KModuleVersionWild);
	}




/**
Loads the named DLL that matches the specified UID type and version.

If successful, the function increments the usage count by one.

@param aFileName      A descriptor containing the name of the DLL to be loaded.
                      The length of the file name must not be greater
                      than KMaxFileName.
@param aPath          A descriptor containing a list of path names, each
                      separated by a semicolon. When specified, these paths
                      are searched for the DLL before the standard search locations.
@param aType          A UID type (a triplet of UIDs) that the DLL must match.
                      Individual UIDs can contain the KNullUid wild card. 
@param aModuleVersion A version specification that the DLL must match. Major
                      version must match exactly and minor version must be >= the 
                      specified minor version.

@return KErrNone, if successful;
        KErrBadName, if the length of aFileName is greater than KMaxFileName;
        otherwise one of the other system wide error codes.
*/
EXPORT_C TInt RLibrary::Load(const TDesC& aFileName, const TDesC& aPath, const TUidType& aType, TUint32 aModuleVersion)
	{

	__IF_DEBUG(Print(_L("RLibrary::Load ##")));

	RLoader loader;
	TInt r=loader.Connect();
	if (r!=KErrNone)
		return r;

	r=loader.LoadLibrary(iHandle, aFileName, aPath, aType, aModuleVersion);
	loader.Close();
	if (r==KErrNone)
		r=Init();
	return r;
	}




/**
Gets information about the specified DLL.

@param aFileName A descriptor containing the name of the DLL to be checked.
                 The length of the file name must not be greater than KMaxFileName.
@param aInfoBuf  On return, contains information about the DLL (RLibrary::TInfo)

@return KErrNone, if successful;
        KErrBadName, if the length of aFileName is greater than KMaxFileName;
        otherwise one of the other system wide error codes.
*/
EXPORT_C TInt RLibrary::GetInfo(const TDesC& aFileName, TDes8& aInfoBuf)
	{
	__IF_DEBUG(Print(_L("RLibrary::GetInfo ##")));
	RLoader loader;
	TInt r=loader.Connect();
	if (r!=KErrNone)
		return r;
	r=loader.GetInfo(aFileName, aInfoBuf);
	loader.Close();
	return r;
	}




/**
Gets information about an executable binary, (DLL or EXE), based on header data
from that binaries image.

@param aHeader	A descriptor containing the data from the start of the binaries image.
				This data should be of size RLibrary::KRequiredImageHeaderSize or the
				total length of the binary image, whichever is smallest.
@param aInfoBuf	A descriptor which will be filled with the extracted information.
				This information will be in the form of a RLibrary::TInfo structure.
				This should usually be an object of type RLibrary::TInfoBuf.

@return KErrNone, if successful;
		KErrUnderflow, if the size of aHeader is too small;
		KErrCorrupt, if the data in aHeader isn't a valid executable image header;
		Otherwise one of the other system wide error codes.

@internalTechnology
*/
EXPORT_C TInt RLibrary::GetInfoFromHeader(const TDesC8& aHeader, TDes8& aInfoBuf)
	{
	RLoader loader;
	TInt r=loader.Connect();
	if (r!=KErrNone)
		return r;
	r=loader.GetInfoFromHeader(aHeader, aInfoBuf);
	loader.Close();
	return r;
	}




TInt RLibrary::InitL()
//
// Initialise any static data following a DLL load
//
	{
	TLinAddr ep[KMaxLibraryEntryPoints];
	TInt numEps=KMaxLibraryEntryPoints;
	E32Loader::LibraryAttach(iHandle, numEps, ep);
	if (numEps==0)
		return KErrNone;
	TInt i;
	for (i=0; i<numEps; ++i)
		{
		TLibraryEntry f=(TLibraryEntry)ep[i];
		TInt r = (*f)(KModuleEntryReasonProcessAttach);
		if (r != KErrNone)
			return r;
		}
	return E32Loader::LibraryAttached(iHandle);
	}




GLREF_C void Panic(TCdtPanic aPanic);




/**
@internalComponent
*/ 
EXPORT_C TInt RLibrary::Init()
	{
	TInt r=KErrNone;
	TRAPD(s,r=InitL());	// catch attempts to leave from constructors
	__ASSERT_ALWAYS(s==KErrNone, Panic(EDllStaticConstructorLeave));
	return r;
	}




/**
Closes the DLL.

The function decrements the usage count by one.

This handle must have been used to load the DLL using
one of the Load() functions.
*/
EXPORT_C void RLibrary::Close()

	{

	RHandleBase::Close();
	}

