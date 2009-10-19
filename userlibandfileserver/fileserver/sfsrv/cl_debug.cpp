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
// f32\sfsrv\cl_debug.cpp
// 
//

#include "cl_std.h"
#include <f32dbg.h>


EXPORT_C TInt RFs::LoaderHeapFunction(TInt, TAny *, TAny *)
/**
This member function is not implemented in this version of Symbian OS. It always returns KErrNotSupported.

@param aFunction
@param *aArg1
@param *aArg2

@return KErrNotSupported is returned always.

*/
	{
	return KErrNotSupported;
	}


#if defined(_DEBUG) || defined(_DEBUG_RELEASE)
EXPORT_C TInt RFs::SetErrorCondition(TInt anError,TInt aCount)
/**

Sets the failure condition.

This function can only be used in debug builds or if _DEBUG or _DEBUG_RELEASE is defined. In release build, this method is not implemented and it always returns KErrNotSupported.

@param anError An error condition to be simulated
@param aCount  An error condition is set after aCount instances of occurence.

@return On completion, KErrNone if successful, otherwise one of the system wide error codes in debug builds.
        KErrNotSupported in release build.

*/
	{
	return(SendReceive(EFsDebugFunction,TIpcArgs(EFsSetErrorCondition,anError,aCount)));
	}

EXPORT_C TInt RFs::SetDebugRegister(TInt aVal)
/**

Sets the debug register to the given value.

This function can only be used in debug builds or if _DEBUG or _DEBUG_RELEASE is defined. In release build, this method is not implemented and it always returns KErrNotSupported.

@param aVal Value to be set

@return On completion, KErrNone if successful, otherwise one of the system wide error codes in debug builds.
        KErrNotSupported in release build.

*/
	{
	return(SendReceive(EFsDebugFunction,TIpcArgs(EFsSetDebugRegister,aVal)));
	}

EXPORT_C TInt RFs::SetAllocFailure(TInt aAllocNum)
/**

Fails an allocation after aAllocNum successes.

This function can only be used in debug builds or if _DEBUG or _DEBUG_RELEASE is defined. In release build, this method is not implemented and it always returns KErrNotSupported.

@param aAllocNum Count after which allocation failure is expected.

@return On completion, KErrNone if successful, otherwise one of the system wide error codes in debug builds.
        KErrNotSupported in release build.

*/
	{
	return(SendReceive(EFsDebugFunction,TIpcArgs(EFsSetAllocFailure,aAllocNum)));
	}

EXPORT_C void RFs::DebugNotify(TInt aDrive,TUint aNotifyType,TRequestStatus& aStat)
/**

Request notification of a file server event - file system specific.

This function can only be used in debug builds or if _DEBUG or _DEBUG_RELEASE is defined. In release build, this method is not implemented and it always returns KErrNotSupported.

@param aDrive      A drive on which the file server event notification is requested; this can be one of the values defined by TDriveNumber.
@param aNotifyType A number identifying the event that should cause notification;
@param aStat       The request status used to contain completion information for the function. On completion, contains a system-wide error code.

@see RFs::TDriveNumber()
@see TRequestStatus

*/
	{
	aStat=KRequestPending;
	RSessionBase::SendReceive(EFsDebugFunction, TIpcArgs(EFsDebugNotify,aDrive,aNotifyType,&aStat), aStat);
//	if (r!=KErrNone)
//		{
//		TRequestStatus* pStat=&aStat;
//		User::RequestComplete(pStat,r);
//		}
	}


#else
EXPORT_C TInt RFs::SetErrorCondition(TInt,TInt)
/**

Sets the failure condition.

This function can only be used in debug builds or if _DEBUG or _DEBUG_RELEASE is defined. In release build, this method is not implemented and it always returns KErrNotSupported.

@param anError An error condition to be simulated
@param aCount  An error condition is set after aCount instances of occurence.

@return On completion, KErrNone if successful, otherwise one of the system wide error codes in debug builds.
        KErrNotSupported in release build.

*/
	{
	return KErrNotSupported;
	}

EXPORT_C TInt RFs::SetDebugRegister(TInt)
/**

Sets the debug register to the given value.

This function can only be used in debug builds or if _DEBUG or _DEBUG_RELEASE is defined. In release build, this method is not implemented and it always returns KErrNotSupported.

@param aVal Value to be set

@return On completion, KErrNone if successful, otherwise one of the system wide error codes in debug builds.
        KErrNotSupported in release build.

*/
	{
	return KErrNotSupported;
	}

EXPORT_C TInt RFs::SetAllocFailure(TInt)
/**

Fails an allocation after aAllocNum successes.

This function can only be used in debug builds or if _DEBUG or _DEBUG_RELEASE is defined. In release build, this method is not implemented and it always returns KErrNotSupported.

@param aAllocNum Count after which allocation failure is expected.

@return On completion, KErrNone if successful, otherwise one of the system wide error codes in debug builds.
        KErrNotSupported in release build.

*/
	{
	return KErrNotSupported;
	}

EXPORT_C void RFs::DebugNotify(TInt,TUint,TRequestStatus& aStat)
/**

Request notification of a file server event - file system specific.

This function can only be used in debug builds or if _DEBUG or _DEBUG_RELEASE is defined. In release build, this method is not implemented and it always returns KErrNotSupported.

@param aDrive           A drive on which the file server event notification is requested; this can be one of the values defined by TDriveNumber.
@param aNotifyType     A number identifying the event that should cause notification;
@param aStat The request status used to contain completion information for the function. On completion, contains a system-wide error code.

@see RFs::TDriveNumber()
@see TRequestStatus

*/
	{
	TRequestStatus* pStat=&aStat;
	User::RequestComplete(pStat,KErrNotSupported);
	}
#endif


#if defined(_USE_CONTROLIO) || defined(_DEBUG) || defined(_DEBUG_RELEASE)

EXPORT_C TInt RFs::ControlIo(TInt aDrive,TInt aCommand)
/**

General purpose test interface - file system specific.

This function can only be used in debug builds or if _USE_CONTROLIO, _DEBUG or _DEBUG_RELEASE is defined. In release build, this method is not implemented and it always returns KErrNotSupported.

There are drive specific and non-drive specific commands. Following are non-drive specific commands

@param aDrive    A drive on which aCommand is to be executed; this can be one of the values defined by TDriveNumber.
@param aCommand  A command to be executed, specific to the file system implementation.

@return On completion, KErrNone if successful, otherwise one of the system wide error codes in debug builds.
        KErrNotSupported in release build.

@see RFs::TDriveNumber()

*/
	{
	return(SendReceive(EFsControlIo,TIpcArgs(aDrive,aCommand)));
	}

EXPORT_C TInt RFs::ControlIo(TInt aDrive,TInt aCommand,TDes8& aParam1)
/**

General purpose test interface - file system specific.

This function can only be used in debug builds or if _USE_CONTROLIO, _DEBUG or _DEBUG_RELEASE is defined. In release build, this method is not implemented and it always returns KErrNotSupported.

@param aDrive    A drive on which aCommand is to be executed; this can be one of the values defined by TDriveNumber.
@param aCommand  A command to be executed, specific to the file system implementation.
@param aParam1 Placeholder for a parameter, specific to the file system implementation.

@return TInt On completion, KErrNone if successful, otherwise one of the system wide error codes in debug builds.
             KErrNotSupported in release build.

@see RFs::TDriveNumber()

*/
	{
	return(SendReceive(EFsControlIo,TIpcArgs(aDrive,aCommand,&aParam1)));
	}

EXPORT_C TInt RFs::ControlIo(TInt aDrive,TInt aCommand,TDes8& aParam1,TDes8& aParam2)
/**

General purpose test interface - file system specific.

This function can only be used in debug builds or if _USE_CONTROLIO, _DEBUG or _DEBUG_RELEASE is defined. In release build, this method is not implemented and it always returns KErrNotSupported.

@param aDrive    A drive on which aCommand is to be executed; this can be one of the values defined by TDriveNumber.
@param aCommand  A command to be executed, specific to the file system implementation.
@param aParam1 Placeholder for a parameter, specific to the file system implementation.
@param aParam2 Placeholder for a parameter, specific to the file system implementation.

@return On completion, KErrNone if successful, otherwise one of the system wide error codes in debug builds.
        KErrNotSupported in release build.

@see RFs::TDriveNumber()

*/
	{
	return(SendReceive(EFsControlIo,TIpcArgs(aDrive,aCommand,&aParam1,&aParam2)));
	}

EXPORT_C TInt RFs::ControlIo(TInt aDrive,TInt aCommand,TAny* aParam1,TAny* aParam2)
/**

General purpose test interface - file system specific.

This function can only be used in debug builds or if _USE_CONTROLIO, _DEBUG or _DEBUG_RELEASE is defined. In release build, this method is not implemented and it always returns KErrNotSupported.

@param aDrive   A drive on which aCommand is to be executed; this can be one of the values defined by TDriveNumber.
@param aCommand A command to be executed, specific to the file system implementation.
@param aParam1 Placeholder for a parameter, specific to the file system implementation.
@param aParam2 Placeholder for a parameter, specific to the file system implementation.

@return On completion, KErrNone if successful, otherwise one of the system wide error codes in debug builds.
        KErrNotSupported in release build.

@see RFs::TDriveNumber()

*/
	{
	return(SendReceive(EFsControlIo,TIpcArgs(aDrive,aCommand,aParam1,aParam2)));
	}

#else
EXPORT_C TInt RFs::ControlIo(TInt,TInt)
/**

General purpose test interface - file system specific.

This function can only be used in debug builds or if _USE_CONTROLIO, _DEBUG or _DEBUG_RELEASE is defined. In release build, this method is not implemented and it always returns KErrNotSupported.

There are drive specific and non-drive specific commands. Following are non-drive specific commands

@param aDrive    A drive on which aCommand is to be executed; this can be one of the values defined by TDriveNumber.
@param aCommand  A command to be executed, specific to the file system implementation.

@return On completion, KErrNone if successful, otherwise one of the system wide error codes in debug builds.
        KErrNotSupported in release build.

@see RFs::TDriveNumber()

*/
	{
	return KErrNotSupported;
	}

EXPORT_C TInt RFs::ControlIo(TInt,TInt,TDes8&)
/**

General purpose test interface - file system specific.

This function can only be used in debug builds or if _USE_CONTROLIO, _DEBUG or _DEBUG_RELEASE is defined. In release build, this method is not implemented and it always returns KErrNotSupported.

@param aDrive    A drive on which aCommand is to be executed; this can be one of the values defined by TDriveNumber.
@param aCommand  A command to be executed, specific to the file system implementation.
@param aParam1 Placeholder for a parameter, specific to the file system implementation.

@return On completion, KErrNone if successful, otherwise one of the system wide error codes in debug builds.
        KErrNotSupported in release build.

@see RFs::TDriveNumber()

*/
	{
	return KErrNotSupported;
	}

EXPORT_C TInt RFs::ControlIo(TInt,TInt,TDes8&,TDes8&)
/**

General purpose test interface - file system specific.

This function can only be used in debug builds or if _USE_CONTROLIO, _DEBUG or _DEBUG_RELEASE is defined. In release build, this method is not implemented and it always returns KErrNotSupported.

@param aDrive    A drive on which aCommand is to be executed; this can be one of the values defined by TDriveNumber.
@param aCommand  A command to be executed, specific to the file system implementation.
@param aParam1 Placeholder for a parameter, specific to the file system implementation.
@param aParam2 Placeholder for a parameter, specific to the file system implementation.

@return On completion, KErrNone if successful, otherwise one of the system wide error codes in debug builds.
        KErrNotSupported in release build.

@see RFs::TDriveNumber()

*/
	{
	return KErrNotSupported;
	}

EXPORT_C TInt RFs::ControlIo(TInt,TInt,TAny*,TAny*)
/**

General purpose test interface - file system specific.

This function can only be used in debug builds or if _USE_CONTROLIO, _DEBUG or _DEBUG_RELEASE is defined. In release build, this method is not implemented and it always returns KErrNotSupported.

@param aDrive   A drive on which aCommand is to be executed; this can be one of the values defined by TDriveNumber.
@param aCommand A command to be executed, specific to the file system implementation.
@param aParam1 Placeholder for a parameter, specific to the file system implementation.
@param aParam2 Placeholder for a parameter, specific to the file system implementation.

@return On completion, KErrNone if successful, otherwise one of the system wide error codes in debug builds.
        KErrNotSupported in release build.

@see RFs::TDriveNumber()

*/
	{
	return KErrNotSupported;
	}
#endif
