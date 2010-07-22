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
// e32\include\d32locd.h
// 
// WARNING: This file contains some APIs which are internal and are subject
//          to change without notice. Such APIs should therefore not be used
//          outside the Kernel and Hardware Services package.
//

#ifndef __D32LOCD_H__
#define __D32LOCD_H__
#include <e32cmn.h>
#include <partitions.h>


/**
@publishedPartner
@released

Local media IDs.

A media ID is passed to LocDrv::RegisterMediaDevice() when registering
a media driver with the local media subsystem.
*/
enum TMediaDevice { EFixedMedia0, EFixedMedia1, EFixedMedia2, EFixedMedia3,
					EFixedMedia4, EFixedMedia5, EFixedMedia6, EFixedMedia7,
					ERemovableMedia0, ERemovableMedia1, ERemovableMedia2, ERemovableMedia3,
					EInvalidMedia,
					EMediaExtension0, EMediaExtension1, EMediaExtension2, EMediaExtension3,
					EMediaExtension4, EMediaExtension5, EMediaExtension6, EMediaExtension7,
				};

#define __IS_REMOVABLE(aDevice) (aDevice>=ERemovableMedia0 && aDevice<=ERemovableMedia3)
#define __IS_FIXED(aDevice) ((TUint)aDevice<=EFixedMedia7)
#define __IS_EXTENSION(aDevice) (aDevice>=EMediaExtension0 && aDevice<=EMediaExtension7)

#define MEDIA_DEVICE_IRAM EFixedMedia0
#define MEDIA_DEVICE_LFFS EFixedMedia1
#define MEDIA_DEVICE_NAND EFixedMedia2
#define MEDIA_DEVICE_MMC ERemovableMedia0
#define MEDIA_DEVICE_PCCARD ERemovableMedia1
#define MEDIA_DEVICE_CSA ERemovableMedia2
#define MEDIA_DEVICE_NFE EMediaExtension0

typedef signed int TSocket;

class TLDriveAssignInfo
/**
No longer used
@internalComponent
@removed
*/
	{
public:
	TMediaDevice iDevice;
	TInt iPriority;
	};

class TMediaDeviceAssignInfo
/**
No longer used
@internalComponent
@removed
*/
	{
public:
	TInt iFirstMedia;
	TInt iLastMedia;	
	};

class TLocalDriveCaps
/**
Drives media capabilities fields

@publishedPartner
@released
*/
	{
public:
	IMPORT_C TLocalDriveCaps();
public:
	/**
	Size of drive
	
	Note : This is the size of the partition, not the entire media.
		   The entire media size can be obtained from TLocalDriveCapsV4::MediaSizeInBytes()
	*/
	TInt64 iSize;
	/**
	Media Type of drive
	*/
	TMediaType iType;
	/**
	Connection type used to interface to the media
	*/
	TConnectionBusType iConnectionBusType;
	/**
	Attributes of the drive
	*/
	TUint iDriveAtt;
	/**
	Attributes of underlying media 
	*/
	TUint iMediaAtt;
	/**
	Base address of media
	*/
    TUint8* iBaseAddress;
	/**
	Identity of the file system used for this media
	*/
	TUint16 iFileSystemId;
	/**
	Partition type of media
	*/
	TUint16	iPartitionType;
	};
typedef TPckgBuf<TLocalDriveCaps> TLocalDriveCapsBuf;
//
class TLocalDriveCapsV2 : public TLocalDriveCaps
/**
Extension to Capabilities fields mainly to support Nor flash

@publishedPartner
@released
*/
	{
public:
	/**
	Number of hidden sectors on drive
	*/
	TUint iHiddenSectors;
	/**
	Size of erase unit on media
	*/
	TUint iEraseBlockSize;
    };
typedef TPckgBuf<TLocalDriveCapsV2> TLocalDriveCapsV2Buf;
//



/**
This class is used to provide file system-specific parameters for media formatting.
@see TInt RFormat::Open(RFs &aFs, const TDesC &aName, TUint aFormatMode, TInt &aCount, const TDesC8 &anInfo);"

For example, for FAT file system it is possible to specify FAT type(12/16/32), 
cluster size, number of FATs and number of reserved sectors. Note that not all 
combinations of custom parameters can be compatible; some media types, like SD 
can reject formatting with user-defined parameters.

@publishedPartner
@released

*/
class TLDFormatInfo
	{
public:
    inline TLDFormatInfo();
public:

    TInt64 iCapacity;				///< Format Capacity
	TUint16 iSectorsPerCluster;		///< Specified sectors per cluster
	TUint16 iSectorsPerTrack;		///< Specified sectors per track
	TUint16 iNumberOfSides;			///< Specified number of sides

	enum TFATBits {EFBDontCare, EFB12 = 12, EFB16 = 16, EFB32 = 32};
	TFATBits iFATBits;				///< Specified bits per fat table entry

	TUint16 iReservedSectors;	///< Reserved sector count, required for SD card compliance
	
	// Flags field. Allows the number of FAT tables to be specified: set both bits to zero to use the default.
	enum TFlags {EOneFatTable = 0x01, ETwoFatTables = 0x02};
	TUint8 iFlags;

	TUint8 iPad;				///< Padding for offset alignment  between kernel and user code

	};
typedef TPckgBuf<TLDFormatInfo> TSpecialFormatInfoBuf;


//
class TLocalDriveCapsV3 : public TLocalDriveCapsV2
/**
Extension to Capabilities fields mainly to support removable media format specifications

@publishedPartner
@released
*/
	{
public:
	/**
	Format specification
	*/
	TLDFormatInfo iFormatInfo;
	/**
	Set if iFormatInfo is valid
	*/
	TBool iExtraInfo;
	/**
	*/
	TInt iMaxBytesPerFormat;
    };
//The following ASSERTs checks for offset of any TInt64 member be a multiple of 8 as per DEF045510
__ASSERT_COMPILE(_FOFF(TLocalDriveCaps,iSize)%8 == 0);
__ASSERT_COMPILE(_FOFF(TLocalDriveCapsV3,iFormatInfo.iCapacity) % 8 == 0);


typedef TPckgBuf<TLocalDriveCapsV3> TLocalDriveCapsV3Buf;
//
class TLocalDriveCapsV4 : public TLocalDriveCapsV3
/**
Extension to Capabilities fields mainly to support Nand flash

@publishedPartner
@released
*/
	{
public:
	union
		{
		/**
		Number of blocks (for NAND flash)
		 - The entire capacity of NAND media can be calculated using:
		   capacity = iNumOfBlocks * iNumPagesPerBlock * iNumBytesMain
		*/
		TInt iNumOfBlocks;
		/**
		Number of sectors (for other types of media)
		 - The entire capacity of sector based media can be calculated using:
		   capacity = iNumberOfSectors * iSectorSizeInBytes
		*/
		TUint32 iNumberOfSectors;
		};

	/**
	Number of pages per block
	*/
	TInt iNumPagesPerBlock;

	union
		{
		/**
		Number of Bytes in the main array (for NAND flash)
		 - The entire capacity of NAND media can be calculated using:
		   capacity = iNumOfBlocks * iNumPagesPerBlock * iNumBytesMain
		*/
		TInt iNumBytesMain;
		/**
		Number of Bytes in a sector (for other types of media)
		 - The entire capacity of sector based media can be calculated using:
		   capacity = iNumberOfSectors * iSectorSizeInBytes
		*/
		TUint32 iSectorSizeInBytes;
		};

	/**
	Number of Bytes in the spare array
	*/
	TInt iNumBytesSpare;
	/**
	Effective usable blocks
	*/
	TInt iEffectiveBlks;
	/**
	Start page of drive
	*/
	TInt iStartPage;
public:
	inline TInt64 MediaSizeInBytes();
    };
typedef TPckgBuf<TLocalDriveCapsV4> TLocalDriveCapsV4Buf;
//
class TLocalDriveCapsV5 : public TLocalDriveCapsV4
/**
Add serial number support for certain media.

@publishedPartner
@released
*/
	{
public:
	/**
    Serial number buffer length, 0 if not supported
	*/
    TUint iSerialNumLength;
	/**
    Serial number buffer
	*/
	TUint8 iSerialNum[KMaxSerialNumLength];
	};
typedef TPckgBuf<TLocalDriveCapsV5> TLocalDriveCapsV5Buf;

//
/**
Extension to Capabilities fields mainly to support reporting block size 
information of underlying media.
    
@publishedPartner
@released
*/
class TLocalDriveCapsV6 : public TLocalDriveCapsV5
	{
public:
	/**
	Block size information of underlying media is required to assist in chosing 
	efficient size of buffers for reading and writing.
	This information can also be accessed through the function
	- RFs::VolumeIOParam

	These functions retrieve this blocksize information by calling the media
	driver's Caps() function.
	*/
    TUint 	iBlockSize;
    
private:
    /**
    Reserved space for future use.
    */
    TUint32	iSpare[4];
	};
typedef TPckgBuf<TLocalDriveCapsV6> TLocalDriveCapsV6Buf;


class TLocalDriveCapsV7 : public TLocalDriveCapsV6
/**
Add control mode, object mode, partition size for M18 NOR Flash.
*/
	{
public:
    TUint32 iWriteBufferSize;
	TUint32 iPartitionSize;
    TUint32 iControlModeSize;
    TUint32 iObjectModeSize;
	};

typedef TPckgBuf<TLocalDriveCapsV7> TLocalDriveCapsV7Buf;

//
class TFormatInfo
/**
Format information class

@publishedPartner
@released
*/
	{
public:
	IMPORT_C TFormatInfo();
public:
	TBool iFormatIsCurrent;
	TInt i512ByteSectorsFormatted; 
	TInt iMaxBytesPerFormat;
	};
//
class TErrorInfo
/**
@internalTechnology
  */
	{
public:
	enum TReasonCode
		{
		ENoError=0,			//No error
		EBadSector=1,	//Error due to corrupt sector
		};
public:
    TReasonCode iReasonCode;
	union
		{
		// holds position error occurred for KErrCorrupt
		// It numerically equals number of bytes succesfully read/written during
		// last disk operation.
		TInt64 iErrorPos;
        TInt iOtherInfo;
		};
    };
typedef TPckgBuf<TErrorInfo> TErrorInfoBuf;
//
class TLocalDriveMessageData
/**
@internalTechnology
*/
	{
public:
	inline TLocalDriveMessageData()
		{}
	inline TLocalDriveMessageData(TInt64 aPos, TInt aLength, const TAny* aPtr, TInt aHandle, TInt anOffset, TInt aFlags)
		: iPos(aPos), iLength(aLength), iPtr(aPtr), iHandle(aHandle), iOffset(anOffset), iFlags(aFlags)
		{}
public:
	TInt64 iPos;
	TInt iLength;
	const TAny* iPtr;
	TInt iHandle;
	TInt iOffset;
	TInt iFlags;
	};

class TLocalDriveControlIOData
/**
Control IO data class

@internalTechnology
*/
	{
public:
	inline TLocalDriveControlIOData()
		{}
	inline TLocalDriveControlIOData(TInt aCommand, TAny* aParam1, TAny* aParam2, TInt aHandle)
		: iCommand(aCommand), iParam1(aParam1), iParam2(aParam2), iHandle(aHandle)
		{}
	inline TLocalDriveControlIOData(TInt aCommand, TUint8* aBuf, TInt aParam, TInt aLength)
		: iCommand(aCommand), iParam1(aBuf), iParam2((TAny*) aParam), iHandle(aLength)
		{}
public:
	TInt iCommand;
	TAny* iParam1;
	TAny* iParam2;
	TInt iHandle;
	};

class TLocalDrivePasswordData
/**
Password store for password protected media

@internalTechnology
*/
	{
public:
	TLocalDrivePasswordData(const TDesC8& aOldPasswd, const TDesC8& aNewPasswd, TBool aStorePasswd)
		: iOldPasswd(&aOldPasswd), iNewPasswd(&aNewPasswd), iStorePasswd(aStorePasswd)
		{}
	TLocalDrivePasswordData()
		: iOldPasswd(&KNullDesC8), iNewPasswd(&KNullDesC8), iStorePasswd(EFalse)
		{}
public:
	const TDesC8 *iOldPasswd;
	const TDesC8 *iNewPasswd;
	TBool iStorePasswd;
	};
typedef TPckgBuf<TLocalDrivePasswordData> TLocalDrivePasswordDataPckg;


class TPasswordStore
/**
@internalTechnology
*/
	{
public:
	virtual TInt Init()=0;
	virtual TInt ReadPasswordData(TDes8 &aBuf)=0;
	virtual TInt WritePasswordData(TDesC8 &aBuf)=0;
	virtual TInt PasswordStoreLengthInBytes()=0;
	enum {EMaxPasswordLength=1024}; /**< Size of the password store write buffer, which may contain multiple password mappings */
	};

class TMountInfoData
/**
@internalTechnology
*/
	{
public:
	TDesC8* iInfo;
	TAny* iThread;
	};


const TInt KLocalDriveMajorVersion=1;
const TInt KLocalDriveMinorVersion=0;
const TInt KLocalDriveBuildVersion=160;
_LIT(KLitLocalDriveLddName,"LocDrv");

const TInt KLocalMessageHandle=-1;


enum TMediaPagingStats
	{
	EMediaPagingStatsAll,
	EMediaPagingStatsRomAndCode = EMediaPagingStatsAll,	// @deprecated - use EMediaPagingStatsAll instead
	EMediaPagingStatsRom,
	EMediaPagingStatsCode,
	EMediaPagingStatsDataIn,
	EMediaPagingStatsDataOut,
	};

/**
Paging media details - for testing purposes only
This is a structure used to communicate paging-related information
from the paging media driver to an application.

@internalTechnology
@prototype
*/
class TPageDeviceInfo
	{
public:
	TInt	iReservoirBlockCount;	// number of blocks in replacement reservoir (used & unused)
	TInt	iBadBlockCount;			// numer of bad blocks
	};
typedef TPckgBuf<TPageDeviceInfo> TPageDeviceInfoBuf;

class TLocalDriveFinaliseInfo
/**
@internalTechnology
*/
	{
public:
	TInt iMode;		// @see RFs::TFinaliseDrvMode
	};
typedef TPckgBuf<TLocalDriveFinaliseInfo> TLocalDriveFinaliseInfoBuf;

class RLocalDrive : public RBusLogicalChannel
/**
Interface class to local media

@internalTechnology
*/
	{
public:
	enum TControl
		{
		EControlRead=0,							/**< Read request */
		EControlWrite=1,						/**< Write request */
		EControlCaps=2,							/**< Caps request */
		EControlFormat=3,						/**< Format request */
		EControlEnlarge=4,						/**< Enlarge drive request */
		EControlReduce=5,						/**< Reduce Drive  request */
		EControlForceMediaChange=6,				/**< Force media change request */
		EControlMediaDevice=7,					/**< Return the media device request */
		EControlPasswordLock=8,					/**< Password lock media request */
		EControlPasswordUnlock=9,				/**< Password unlock media request */
		EControlPasswordClear=10,				/**< Password clear request */
		EControlNotifyChange=11,				/**< Notify change request */
		EControlNotifyChangeCancel=12,			/**< Notify change cancel request */
		EControlReadPasswordStore=13,			/**< Read password request */
		EControlWritePasswordStore=14,			/**< Write password request */
		EControlPasswordStoreLengthInBytes=15,	/**< Password request */
		EControlIsRemovable=16,					/**< Query removable request */
		EControlSetMountInfo=17,				/**< Set mount info request */
		EControlControlIO=18,					/**< Control IO request */
		EControlPasswordErase=19,				/**< Password erase media request */
		EControlDeleteNotify=20,				/**< Delete notification */
		EControlGetLastErrorInfo=21,			/**< Get last error info request */
		EControlQueryDevice=22,					/**< Query device  request*/
		};

	enum TRemountFlags
		{
		ELocDrvRemountNotifyChange	   = 0,		/**< Notify clients of media change but don't remount */
		ELocDrvRemountForceMediaChange = 1,		/**< Notify clients of media change and remount	*/
		};

	enum TReadWriteFlags
		{
		ELocDrvMetaData					= 0x80000000,	/**< Set if read/write request is for metadata */
		ELocDrvWholeMedia				= 0x40000000,	/**< Set to access whole media, rather than partition */
		ELocDrvDirectIO					= 0x20000000
		};

	// @see TBusLocalDrive::QueryDevice()
	enum TQueryDevice
		{ 
		// Symbian publishedPartner range
		EQuerySymbianPublishedPartnerFirst	= 0x0000,
		EQuerySymbianPublishedPartnerLast	= 0x3FFF,
		
		// Symbian test range
		EQuerySymbianTestFirst				= 0x4000,
		EQuerySymbianTestLast				= 0x7FFF,
		
		// Licensee range
		EQueryLicenseeFirst					= 0x8000,
		EQueryLicenseeLast					= 0xBFFF,
		
		// Finalize Drive - called as a result of a call to RFs::FinaliseDrives()
		EQueryFinaliseDrive					= EQuerySymbianPublishedPartnerFirst + 0,	// @internalTechnology

		EQueryPageDeviceInfo = EQuerySymbianTestFirst,	/**< @see TPageDeviceInfo */
		
		// NFE test driver
		EQuerySymbianNfeTestFirst = EQuerySymbianTestFirst+0x10,
		EQuerySymbianNfeTestEnd = EQuerySymbianTestFirst+0x1F,
		};
public:
	inline TVersion VersionRequired() const;
	inline TInt Connect(TInt aDriveNumber, TBool& aChangedFlag);
	inline TInt Enlarge(TInt aLength);
	inline TInt Reduce(TInt aPos, TInt aLength);
	inline TInt Read(TInt64 aPos, TInt aLength, const TAny* aTrg, TInt aMessageHandle, TInt aOffset, TInt aFlags);
	inline TInt Read(TInt64 aPos, TInt aLength, const TAny* aTrg, TInt aMessageHandle, TInt anOffset);
	inline TInt Read(TInt64 aPos, TInt aLength, TDes8& aTrg);
	inline TInt Write(TInt64 aPos, TInt aLength, const TAny* aSrc, TInt aMessageHandle, TInt aOffset, TInt aFlags);
	inline TInt Write(TInt64 aPos, TInt aLength, const TAny* aSrc, TInt aMessageHandle, TInt anOffset);
	inline TInt Write(TInt64 aPos, const TDesC8& aSrc);
	inline TInt Caps(TDes8& anInfo);
	inline TInt Format(TInt64 aPos, TInt aLength);
	inline TInt ForceMediaChange(TInt aMode=0);
	inline void NotifyChange(TRequestStatus* aStatus);
	inline void NotifyChangeCancel();
	inline TInt SetMountInfo(const TDesC8* aInfo,TInt aMessageHandle);
	inline TMediaDevice MediaDevice();
	inline TInt IsRemovable(TInt& aSocketNum);
	inline TInt ControlIO(TInt aCommand, TAny* aParam1, TAny* aParam2);
	inline TInt ControlIO(TInt aCommand, TDes8& aBuf, TInt aParam);
	inline TInt ControlIO(TInt aCommand, TDesC8& aBuf, TInt aParam);
	inline TInt ControlIO(TInt aCommand, TInt aParam1, TInt aParam2);
	inline TInt Unlock(const TDesC8& aPassword, TBool aStorePassword);
	inline TInt SetPassword(const TDesC8& aOldPassword, const TDesC8& aNewPassword, TBool aStorePassword);
	inline TInt Clear(const TDesC8& aPassword);
	inline TInt ErasePassword();
	inline TInt ReadPasswordData(TDesC8& aStoreData);
	inline TInt WritePasswordData(const TDesC8& aStoreData);
	inline TInt PasswordStoreLengthInBytes();
	inline TInt DeleteNotify(TInt64 aPos, TInt aLength);
	inline TInt GetLastErrorInfo(TDesC8& aErrorInfo);
	inline TInt QueryDevice(TQueryDevice aQueryDevice, TDes8 &aBuf);
	};
//
#ifndef __KERNEL_MODE__
class TBusLocalDrive : public RLocalDrive
/**
@internalTechnology
*/
	{
public:
	IMPORT_C TBusLocalDrive();
	IMPORT_C TInt Connect(TInt aDriveNumber, TBool& aChangedFlag);
	IMPORT_C void Disconnect();
	IMPORT_C TInt Enlarge(TInt aLength);
	IMPORT_C TInt ReduceSize(TInt aPos, TInt aLength);
	IMPORT_C TInt Read(TInt64 aPos,TInt aLength,const TAny* aTrg,TInt aMessageHandle,TInt aOffset,TInt aFlags);
	IMPORT_C TInt Read(TInt64 aPos,TInt aLength,const TAny* aTrg,TInt aMessageHandle,TInt anOffset);
	IMPORT_C TInt Read(TInt64 aPos,TInt aLength,TDes8& aTrg);
	IMPORT_C TInt Write(TInt64 aPos,TInt aLength,const TAny* aSrc,TInt aMessageHandle,TInt aOffset,TInt aFlags);
	IMPORT_C TInt Write(TInt64 aPos,TInt aLength,const TAny* aSrc,TInt aMessageHandle,TInt anOffset);
	IMPORT_C TInt Write(TInt64 aPos,const TDesC8& aSrc);
	IMPORT_C TInt Caps(TDes8& anInfo);
	IMPORT_C TInt Format(TFormatInfo& anInfo);
	IMPORT_C TInt Format(TInt64 aPos,TInt aLength);
	IMPORT_C TInt SetMountInfo(const TDesC8* aMountInfo,TInt aMessageHandle);
	IMPORT_C TInt ForceRemount(TUint aFlags=0);
	IMPORT_C TInt ControlIO(TInt aCommand, TAny* aParam1, TAny* aParam2);
	IMPORT_C TInt ControlIO(TInt aCommand, TDes8& aBuf, TInt aParam);
	IMPORT_C TInt ControlIO(TInt aCommand, TDesC8& aBuf, TInt aParam);
	IMPORT_C TInt ControlIO(TInt aCommand, TInt aParam1, TInt aParam2);

	IMPORT_C TInt Unlock(const TDesC8& aPassword, TBool aStorePassword);
	IMPORT_C TInt SetPassword(const TDesC8& aOldPassword, const TDesC8& aNewPassword, TBool aStorePassword);
	IMPORT_C TInt Clear(const TDesC8& aPassword);
	IMPORT_C TInt ErasePassword();
	IMPORT_C TInt ReadPasswordData(TDes8 &aBuf);
	IMPORT_C TInt WritePasswordData(const TDesC8 &aBuf);
	IMPORT_C TInt PasswordStoreLengthInBytes();
	IMPORT_C TInt DeleteNotify(TInt64 aPos, TInt aLength);
	IMPORT_C TInt GetLastErrorInfo(TDes8& aErrorInfo);

	IMPORT_C TInt QueryDevice(TQueryDevice aQueryDevice, TDes8 &aBuf);

public:
	inline TInt& Status() {return(iStatus);}
private:
	TInt CheckMount();
private:
	TInt iStatus;
	};
#endif

#define _LOCKABLE_MEDIA

#include <d32locd.inl>
#endif

