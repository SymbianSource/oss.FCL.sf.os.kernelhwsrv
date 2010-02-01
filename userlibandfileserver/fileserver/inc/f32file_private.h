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
// WARNING: This file contains some APIs which are internal and are subject
//          to change without notice. Such APIs should therefore not be used
//          outside the Kernel and Hardware Services package.
//

/**
 @file
 @internalTechnology
 @released
*/

#if !defined(__F32FILE_PRIVATE_H__)
#define __F32FILE_PRIVATE_H__

#include <e32base.h>
#include <e32svr.h>
#include <e32ldr.h>
#include <e32ldr_private.h>


/**
@publishedPartner
@released

Bit mask used when evaluating whether or not a session gets notified of a
debug event.

@see DebugNotifySessions
*/
const TUint KDebugNotifyMask=0xFF000000; // Reserved for debug notification


/**
@publishedPartner
@released

The default blocksize value.

This value is returned when you query the blocksize for a media type that does not
support the concept of 'block' (e.g. NOR flash media).

@see TVolumeIOParamInfo
*/
const TUint KDefaultVolumeBlockSize = 512;


/**
@internalTechnology

Indicates that a TIpcArg slot 0 contains a descriptor.
This shall be ORed with the file server request aFunction.
Server should read from the location accordingly.

@note This constant is intended for use inside Kernel and Hardware Services only.
*/
const TUint KIpcArgSlot0Desc = 0x00010000;

/**
@internalTechnology

Indicates that a TIpcArg slot 1 contains a descriptor.
This shall be ORed with the file server request aFunction.
Server should read from the location accordingly.

@note This constant is intended for use inside Kernel and Hardware Services only.
*/
const TUint KIpcArgSlot1Desc = 0x00020000;

/**
@internalTechnology

Indicates that a TIpcArg slot 2 contains a descriptor.
This shall be ORed with the file server request aFunction.
Server should read from the location accordingly.

@note This constant is intended for use inside Kernel and Hardware Services only.
*/
const TUint KIpcArgSlot2Desc = 0x00040000;


/**
@internalTechnology

Flag to indicate that the Adopt request is from RFile::Adopt or from RFile::AdoptFromXXX functions

@note This constant is intended for use inside Kernel and Hardware Services only.
*/
const TInt KFileAdopt32 = 0;


/**
@internalTechnology

Flag to indicates that the Adopt request is from RFile::Duplicate.

@note This constant is intended for use inside Kernel and Hardware Services only.
*/
const TInt KFileDuplicate = 1;


/**
@internalTechnology

Flag to indicates that the Adopt request is from RFile64::AdoptFromXXX functions

@note This constant is intended for use inside Kernel and Hardware Services only.
*/
const TInt KFileAdopt64 = 2;



enum TStartupConfigurationCmd
/**
@publishedPartner
@released

Command used to set file server configuration at startup.

@see RFs::SetStartupConfiguration()
*/
    {
    /**
    Set loader thread priority
    */
    ELoaderPriority,

    /**
    Set TDrive flags. Value should be ETrue or EFalse
    */
    ESetRugged,
    /**
    Command upper boundary
    */
    EMaxStartupConfigurationCmd
    };


/**
Local drive mapping list - passed as argument to RFs::SetLocalDriveMapping().

@publishedPartner
@released
*/
class TLocalDriveMappingInfo
	{
public:
	enum TDrvMapOperation {EWriteMappingsAndSet=0,EWriteMappingsNoSet=1,ESwapIntMappingAndSet=2};
public:
	TInt iDriveMapping[KMaxLocalDrives];
	TDrvMapOperation iOperation;
    };


/**
@internalTechnology
@released

@note This class is intended for use inside Kernel and Hardware Services only.
*/
typedef TPckgBuf<TLocalDriveMappingInfo> TLocalDriveMappingInfoBuf;


/**
Client side plugin API.

@publishedPartner
@released
*/
class RPlugin : public RSubSessionBase
	{
public:
	IMPORT_C TInt Open(RFs& aFs, TInt aPos);
	IMPORT_C void Close();
protected:
	IMPORT_C void DoRequest(TInt aReqNo,TRequestStatus& aStatus) const;
	IMPORT_C void DoRequest(TInt aReqNo,TRequestStatus& aStatus,TDes8& a1) const;
	IMPORT_C void DoRequest(TInt aReqNo,TRequestStatus& aStatus,TDes8& a1,TDes8& a2) const;
	IMPORT_C TInt DoControl(TInt aFunction) const;
	IMPORT_C TInt DoControl(TInt aFunction,TDes8& a1) const;
	IMPORT_C TInt DoControl(TInt aFunction,TDes8& a1,TDes8& a2) const;
	IMPORT_C void DoCancel(TUint aReqMask) const;
	};

/**
@publishedPartner
@released

Specifies that a plugin should determine for itself which drives it attaches to.

@see RFs::MountPlugin
@see RFs::DismountPlugin
*/
const TInt KPluginAutoAttach = 0x19;

/**
@publishedPartner
@released

Specifies that a plugin should mount on drive Z.

@see RFs::MountPlugin
@see RFs::DismountPlugin
*/
const TInt KPluginMountDriveZ = 0x1A;


/**
@publishedPartner
@released

Specifies that a plugin should be mounted on all drives.

@see RFs::MountPlugin
@see RFs::DismountPlugin
*/
const TInt KPluginSupportAllDrives = 0x3FFFFFF; //All 26 bits (each corrosponding to a drive) are set to one.

/**
@publishedPartner
@released

Used to determine if a plugin is of version 2, meaning it can support drive Z.

*/
const TInt KPluginVersionTwo = 0x4000000; //bit 27 is set to one.

/**
@publishedPartner
@released

Specifies that a plugin should determine its own position in the plugin stack.

@see RFs::MountPlugin
@see RFs::DismountPlugin
*/
const TInt KPluginAutoLocate = 0xC8;


enum TSessionFlags
/**
@internalTechnology

A set of session specific configuration flags.

@note This enum definition is intended for use inside Kernel and Hardware Services only.
*/
	{
	/**
	Notify the user or write failures
	*/
	EFsSessionNotifyUser	= KBit0,

	/**
	Notify clients registered for change notification
	*/
	EFsSessionNotifyChange	= KBit1,

	/**
	Enables all session flags
	*/
	EFsSessionFlagsAll		= KSet32,
	};

/**
@internalTechnology

@note This structure is intended for use inside Kernel and Hardware Services only.
*/
struct SBlockMapArgs
	{
	TInt64 iStartPos;
	TInt64 iEndPos;
	};


/**
@internalTechnology

Validates the mask used to match drive attributes.

@note This function is intended for use inside Kernel and Hardware Services only.

@see RFs::DriveList
@see TFindFile::SetFindMask
*/
TInt ValidateMatchMask( TUint aMask);


/**
Returns the entire size of the TEntry, including the valid portion of the name string.
The returned value is aligned to 4-byte boundary.
@param aPacked If ETrue, returns the length including packed iSizeHigh and iReserved. 
			   If EFalse, returns the length including only the name.

@note This function is intended for use inside Kernel and Hardware Services only.

@internalTechnology
*/
inline TInt EntrySize(const TEntry& anEntry, TBool aPacked = EFalse)
	{
	return(sizeof(TUint)+sizeof(TInt)+sizeof(TTime)+sizeof(TInt)+sizeof(TUidType)+Align4(anEntry.iName.Size()) + (aPacked ? (2*sizeof(TInt)) : 0));
	}



#endif //__F32FILE_PRIVATE_H__
