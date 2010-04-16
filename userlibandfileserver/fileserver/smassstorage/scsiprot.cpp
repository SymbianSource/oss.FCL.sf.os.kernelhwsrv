// Copyright (c) 2004-2010 Nokia Corporation and/or its subsidiary(-ies).
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

#include "scsiprot.h"
#ifdef MSDC_MULTITHREADED 
#include "rwdrivethread.h"
#endif // MSDC_MULTITHREADED
#include "massstoragedebug.h"

// Helper macros
#define LBA(x) static_cast<TUint32>((x[3] << 24) | (x[4] << 16) | (x[5] << 8) | x[6])
#define LEN(x) static_cast<TUint16>((x[8] << 8) | x[9])


LOCAL_D const TUint KDefaultBlockSize = 0x200;  //default block size for FAT

LOCAL_D const TUint KUndefinedLun = 0xFFFF;

LOCAL_D const TUint8 KAllPages = 0x3F;

LOCAL_D const TUint8 KChangeableValues = 0x1;
LOCAL_D const TUint8 KDefaultValues = 0x2;

/**
Default constructor for TSenseInfo
*/
TSenseInfo::TSenseInfo()
	: iSenseCode(ENoSense),
	  iAdditional(EAscNull),
	  iQualifier(EAscqNull)
	{}


/**
Set sense with no additional info.

@param aSenseCode sense key
*/
void TSenseInfo::SetSense(TSenseCode aSenseCode)
	{
	iSenseCode = static_cast<TUint8>(aSenseCode);
	iAdditional = EAscNull;
	iQualifier = EAscqNull;
	}


/**
Set sense with additional info.

@param aSenseCode sense key
@param aAdditional additional sense code (ASC) 
*/
void TSenseInfo::SetSense(TSenseCode aSenseCode, TAdditionalCode aAdditional)

	{
	iSenseCode = static_cast<TUint8>(aSenseCode);
	iAdditional = static_cast<TUint8>(aAdditional);
	iQualifier = EAscqNull;
	}


/**
Set sense with additional info and qualifier.

@param aSenseCode sense key 
@param aAdditional additional sense code (ASC) 
@param aQualifier additional sense code qualifier (ASCQ)
*/
void TSenseInfo::SetSense(TSenseCode aSenseCode,
						  TAdditionalCode aAdditional,
						  TAdditionalSenseCodeQualifier aQualifier)
	{
	iSenseCode = static_cast<TUint8>(aSenseCode);
	iAdditional = static_cast<TUint8>(aAdditional);
	iQualifier = static_cast<TUint8>(aQualifier);
	}


//-----------------------------------------------

/**
Creates the CScsiProtocol object.  Called during controller initialisation.

@param aDriveManager reference to the drive manager object
*/
CScsiProtocol* CScsiProtocol::NewL(CDriveManager& aDriveManager)
	{
	CScsiProtocol* self = new (ELeave) CScsiProtocol(aDriveManager);
	CleanupStack::PushL(self);
	self->ConstructL();
	CleanupStack::Pop();
	return self;
	}

/**
c'tor

@param aDriveManager reference to the drive manager object
*/
CScsiProtocol::CScsiProtocol(CDriveManager& aDriveManager):
	iDriveManager(aDriveManager),
	iLastCommand(EUndefinedCommand),
	iLastLun(KUndefinedLun),
	iMediaWriteSize(KDefaultMediaWriteSize)
	{
	__FNLOG("CScsiProtocol::CScsiProtocol");

#ifdef USB_TRANSFER_PUBLISHER
	iWriteTransferPublisher = CUsbWriteTransferPublisher::NewL(iBytesWritten);
	iReadTransferPublisher = CUsbReadTransferPublisher::NewL(iBytesRead);

	for (TUint i = 0; i < KUsbMsMaxDrives; i++)
		{
		iBytesRead[i] = 0;
		iBytesWritten[i] = 0;
		}
#else
	iWriteTransferPublisher = CDriveWriteTransferPublisher::NewL(aDriveManager.iDrives);
	iReadTransferPublisher = CDriveReadTransferPublisher::NewL(aDriveManager.iDrives);
#endif
	}


CScsiProtocol::~CScsiProtocol()
	{
	__FNLOG("CScsiProtocol::~CScsiProtocol");
#ifdef MSDC_MULTITHREADED
	__PRINT(_L("Deleting WriteDrive Thread"));
	delete iWriteDriveThread;
	__PRINT(_L("Deleting ReadDrive Thread"));
	delete iReadDriveThread;
#endif // MSDC_MULTITHREADED

	delete iWriteTransferPublisher;
	delete iReadTransferPublisher;
	}


void CScsiProtocol::ConstructL()
	{
	__FNLOG("CScsiProtocol::ConstructL");
#ifdef MSDC_MULTITHREADED
	__PRINT(_L("Creating WriteDrive Thread"));
	iWriteDriveThread = CWriteDriveThread::NewL();
	__PRINT(_L("Creating ReadDrive Thread"));
	iReadDriveThread = CReadDriveThread::NewL();
#endif // MSDC_MULTITHREADED
	}


/**
Associates the transport with the protocol. Called during initialisation of the controller.

@param aTransport pointer to the transport object
*/
void CScsiProtocol::RegisterTransport(MTransportBase* aTransport)
	{
	__FNLOG("CScsiProtocol::RegisterTransport");
	iTransport = aTransport;
	}


/**
Called by the Transport when it detects that the USB device is either running
at High Speed or is at least capable of HS operation. The Protocol can use this
information (for instance) to select the optimal write block size to use.

This function is preferably called before actual MS data transfer operation
starts, and usually only once.

*/
void CScsiProtocol::ReportHighSpeedDevice()
	{
	__FNLOG("CScsiProtocol::ReportHighSpeedDevice");
	iMediaWriteSize = KHsMediaWriteSize;
	__PRINT1(_L("HS Device reported: SCSI will use %d bytes disk write size"), iMediaWriteSize);
	}


TInt CScsiProtocol::SetScsiParameters(TMassStorageConfig aConfig)
	{
	__FNLOG("CScsiProtocol::SetScsiParameters");
	iConfig = aConfig;
	return KErrNone;
	}

#ifdef MSDC_MULTITHREADED

void CScsiProtocol::ProcessWriteComplete (TUint8* aAddress, TAny* aPtr)
	{
	((CScsiProtocol*)aPtr)->iTransport->ProcessReadData(aAddress);
	}

void CScsiProtocol::InitializeBufferPointers(TPtr8& aDes1, TPtr8& aDes2) // Todo Change name later - InitializeReadBufferSomething
	{
	iReadDriveThread->iThreadContext->iBuffer.SetUpReadBuf(aDes1, aDes2);
	}
#endif

/**
Called by the transport layer when a packet is available for decoding.
If an error occurs, the sense code is updated and EFalse is returned.

@param aData

@return  ETrue if command was decoded and executed successfully
*/
TBool CScsiProtocol::DecodePacket(TPtrC8& aData, TUint aLun)
	{
	__FNLOG("CScsiProtocol::DecodePacket");

	TUint8 command = aData[1];

	if (command != ERequestSense)
		{
		iSenseInfo.SetSense(TSenseInfo::ENoSense);
		}

	__PRINT2(_L("command = 0x%x lun=%d"), command, aLun);
	switch (command)
		{
		case ETestUnitReady:
			HandleUnitReady(aLun);
			break;

		case ERequestSense:
			HandleRequestSense(aData);
			break;

		case EInquiry:
			HandleInquiry(aData, aLun);
			break;

		case EModeSense6:
			HandleModeSense6(aData, aLun);
			break;

        case EModeSense10:
            HandleModeSense10(aData, aLun);
            break;

		case EStartStopUnit:
			HandleStartStopUnit(aData, aLun);
			break;

		case EPreventMediaRemoval:
			HandlePreventMediaRemoval(aData, aLun);
			break;

		case EReadCapacity:
			HandleReadCapacity(aData, aLun);
			break;

		case ERead10:
			HandleRead10(aData, aLun);
			break;

		case EWrite10:
			HandleWrite10(aData,aLun);
			break;

		case EVerify10:
			HandleVerify10(aData, aLun);
			break;

		case EReadFormatCapacities:
			HandleReadFormatCapacities(aLun);
			break;

		default:
			iSenseInfo.SetSense(TSenseInfo::EIllegalRequest, TSenseInfo::EInvalidCmdCode);
		}
	__PRINT1(_L("DecodePacket result = %d"), iSenseInfo.SenseOk());
	return(iSenseInfo.SenseOk());
	}


/**
Checks if drive ready

@param aLun Logic unit number 
@return pointer to drive correspondent to LUN if drive mounted and ready, NULL otherwise
*/
CMassStorageDrive* CScsiProtocol::GetCheckDrive(TUint aLun)
	{
	__FNLOG("CScsiProtocol::GetCheckDrive");
	TInt err=KErrNone;

#ifdef MSDC_MULTITHREADED
	// check for deferred errors
	if (iWriteDriveThread->DeferredError())
		{
		iWriteDriveThread->ClearDeferredError();
		iDeferredSenseInfo.SetSense(TSenseInfo::EMediumError);
		return NULL;
		}

#endif

	CMassStorageDrive* drive= iDriveManager.Drive(aLun, err);

	if (err !=KErrNone || drive == NULL)
		{
		__PRINT(_L("No drive available\n"));
		iSenseInfo.SetSense(TSenseInfo::EIllegalRequest, TSenseInfo::ELuNotSupported);
		return NULL;
		}

	CMassStorageDrive::TMountState mountState = drive->MountState();

	if (mountState == CMassStorageDrive::EDisconnected || mountState == CMassStorageDrive::EConnecting)
		{
		__PRINT(_L("Drive disconnected\n"));
		iSenseInfo.SetSense(TSenseInfo::ENotReady,
							TSenseInfo::EMediaNotPresent);
		return NULL;
		}

	CMassStorageDrive::TDriveState state = drive->CheckDriveState();
	if (state == CMassStorageDrive::EMediaNotPresent || state == CMassStorageDrive::ELocked)
		{
		__PRINT1(_L("Media not present or locked. (state =0x%X)\n"),state);
		iSenseInfo.SetSense(TSenseInfo::ENotReady, TSenseInfo::EMediaNotPresent);
		return NULL;
		}

	if (drive->IsMediaChanged(ETrue))  //reset "media changed" status
		{
		__PRINT(_L("Media was changed\n"));
		// SAM-2 Section 5.9.5 Unit Attention Condition
		iSenseInfo.SetSense(TSenseInfo::EUnitAttention, TSenseInfo::ENotReadyToReadyChange);
		iDriveManager.Connect(aLun);   //publish event to USB app
		return NULL;
		}

	if (mountState == CMassStorageDrive::EDisconnecting)
		{
		__PRINT(_L("Drive disconnecting\n"));
		iSenseInfo.SetSense(TSenseInfo::ENotReady,
							TSenseInfo::EMediaNotPresent);
		return NULL;
		}

	return drive;
	}


/**
Command Parser for the UNIT READY command (0x00)

@param aLun Logic unit number 
@return ETrue if successful, 
*/
TBool CScsiProtocol::HandleUnitReady(TUint aLun)
	{
	__FNLOG("CScsiProtocol::HandleUnitReady");
#ifdef MSDC_MULTITHREADED
	iWriteDriveThread->WaitForWriteEmpty();
#endif
	return GetCheckDrive(aLun) ? (TBool)ETrue : (TBool)EFalse;
	}


/**
Command Parser for the REQUEST SENSE command (0x03)

@return ETrue if successful, 
*/
TBool CScsiProtocol::HandleRequestSense(TPtrC8& aData)
	{
	__FNLOG("CScsiProtocol::HandleRequestSense");
	TUint length = aData[5];
	__PRINT1(_L("length = %d\n"), length);
	
	TPtr8 writeBuf(NULL, 0);
	iTransport->GetCommandBufPtr(writeBuf, KRequestSenseCommandLength);
	writeBuf.FillZ(KRequestSenseCommandLength);

	TSenseInfo* senseInfo;
#ifdef MSDC_MULTITHREADED
	if (!iDeferredSenseInfo.SenseOk())
		{
		writeBuf[00] = 0x71; //(deferred errors)
		senseInfo = &iDeferredSenseInfo;
		}
	else
		{
		writeBuf[00] = 0x70; //(current errors)
		senseInfo = &iSenseInfo;
		}
#else
	senseInfo = &iSenseInfo;
	writeBuf[00] = 0x70; //(current errors)
#endif

	writeBuf[02] = static_cast<TUint8>(senseInfo->iSenseCode & 0x0F);

	writeBuf[12] = senseInfo->iAdditional;
	writeBuf[13] = senseInfo->iQualifier;
	if (length<18 && length >=8) 
		{
		writeBuf.SetLength(length);  //length of response code data
		writeBuf[07] = TUint8(length - 8);  //additional sence length
		}
	else if (length >= KRequestSenseCommandLength)
		{
		writeBuf[07] = KRequestSenseCommandLength - 8;	// we have max 18 byte to send
		}

	__PRINT4(_L("Response=0x%x Sense=0x%x, Additional=0x%x, Qualifier=0x%x\n"),
				writeBuf[0], writeBuf[02], writeBuf[12], writeBuf[13]);
 
	TPtrC8 writeBuf1 = writeBuf.Left(length);

	iTransport->SetupWriteData(writeBuf1);

	// clear the sense info
	iSenseInfo.SetSense(TSenseInfo::ENoSense);

#ifdef MSDC_MULTITHREADED
	iDeferredSenseInfo.SetSense(TSenseInfo::ENoSense);
#endif

	return ETrue;
	}


/**
Command Parser for the INQUIRY command (0x12)

@param aLun Logic unit number 
@return ETrue if successful, 
*/
TBool CScsiProtocol::HandleInquiry(TPtrC8& aData, TUint  aLun )
	{
	__FNLOG("CScsiProtocol::HandleInquiry");

	TBool cmdDt = aData[2] & 0x2;
	TBool evpd  = aData[2] & 0x1;
	TUint8 page = aData[3];
	if (cmdDt || evpd || page || aLun >= KUsbMsMaxDrives)
		{
		iSenseInfo.SetSense(TSenseInfo::EIllegalRequest, TSenseInfo::EInvalidFieldInCdb); 
		return EFalse;
		}

	TPtr8 writeBuf(NULL, 0);
	iTransport->GetCommandBufPtr(writeBuf, KInquiryCommandLength);
	writeBuf.FillZ(KInquiryCommandLength);

	writeBuf[1] = 0x80;	// MSB: RMB : Removable
	writeBuf[3] = 0x02;	// AERC, TrmTsk, NormACA, Response Data Format
	writeBuf[4] = 0x1F;	// Additional Length

	TPtr8 vendorId(&writeBuf[8], 8, 8);		// Vendor ID (Vendor Specific/Logged by T10)
	vendorId.Fill(' ', 8);
	vendorId.Copy(iConfig.iVendorId);

	TPtr8 productId(&writeBuf[16], 16, 16);	// Product ID (Vendor Specific)
	productId.Fill(' ', 16);
	productId.Copy(iConfig.iProductId);

	TPtr8 productRev(&writeBuf[32], 4, 4);		// Product Revision Level (Vendor Specific)
	productRev.Fill(' ', 4);
	productRev.Copy(iConfig.iProductRev);

    TUint length = aData[5];

	TPtrC8 writeBuf1 = writeBuf.Left(length);
	iTransport->SetupWriteData(writeBuf1);

	iSenseInfo.SetSense(TSenseInfo::ENoSense); 
	return ETrue;
	}


/**
 Command Parser for the START STOP UNIT command (0x1B)
 
 @param aData command data (started form position 1)
 @param aLun Logic unit number 
 @return ETrue if successful, TFalse otherwise
 */
TBool CScsiProtocol::HandleStartStopUnit(TPtrC8& aData, TUint aLun)
	{
	__FNLOG("CScsiProtocol::HandleStartStopUnit");

	const TUint8 KStartMask = 0x01;
	const TUint8 KImmedMask = 0x01;
	const TUint8 KLoejMask = 0x02;

	TBool immed = aData[2] & KImmedMask ? (TBool)ETrue : (TBool)EFalse;
	TBool start = aData[5] & KStartMask ? (TBool)ETrue : (TBool)EFalse;
	TBool loej = aData[5] & KLoejMask ? (TBool)ETrue : (TBool)EFalse;

	__PRINT2(_L("Data %X %X\n"), aData[2], aData[5]);
	__PRINT1(_L("IMMED = %d\n"), immed);
	__PRINT1(_L("START = %d\n"), start);

	__PRINT1(_L("LOEJ = %d\n"), loej);

	TInt err(KErrNone);
	if (loej)
		{
		if(start)	//Start unit
			{
			err = iDriveManager.Connect(aLun);
			__PRINT(_L("Load media\n"));

#ifdef USB_TRANSFER_PUBLISHER
			iBytesRead[aLun] = 0;
			iBytesWritten[aLun] = 0;
#endif
			// publish the initial values
			iWriteTransferPublisher->DoPublishDataTransferredEvent();
			iReadTransferPublisher->DoPublishDataTransferredEvent();
			}
		else		//Stop unit 
			{
			iDriveManager.SetCritical(aLun, EFalse);
			err = iDriveManager.Disconnect(aLun);
			__PRINT(_L("Unload media\n"));
			}
		}

	if (err !=KErrNone)  //actually we have error here only if the LUN is incorrect 
		{
		iSenseInfo.SetSense(TSenseInfo::EIllegalRequest, TSenseInfo::ELuNotSupported);
		return EFalse;
		}
	if (immed)
		{
		return ETrue;
		}

	CMassStorageDrive* drive= iDriveManager.Drive(aLun, err);

	if (err !=KErrNone || drive == NULL)
		{
		iSenseInfo.SetSense(TSenseInfo::EIllegalRequest, TSenseInfo::ELuNotSupported);
		return EFalse;
		}

	TInt  timeLeft (20);   // 1 sec timeout
	CMassStorageDrive::TMountState mountState;

	do 
		{
		User::After(1000 * 50);		// 50 mSec
		--timeLeft;
		mountState = drive->MountState();

		if ((!start && mountState != CMassStorageDrive::EConnected)
			 ||
			 (start &&
				(mountState == CMassStorageDrive::EDisconnecting || 
				mountState == CMassStorageDrive::EConnected))
			)
			{
			return ETrue;
			}
		} while (timeLeft>0);

	//timeout happend
	iSenseInfo.SetSense(TSenseInfo::ENotReady,
						TSenseInfo::EAscLogicalUnitDoesNotRespondToSelection);
	return EFalse;
	}


/**
Command Parser for the PREVENT/ALLOW MEDIA REMOVAL command (0x1E)

@param aData command data (started form position 1)
@param aLun Logic unit number 
@return ETrue if successful.
*/
TBool CScsiProtocol::HandlePreventMediaRemoval(TPtrC8& aData, TUint aLun)
	{
	__FNLOG("CScsiProtocol::HandlePreventMediaRemoval");
	CMassStorageDrive* drive=GetCheckDrive(aLun);

	if (drive == NULL)
		{
		return EFalse;
		}

	TInt prevent = aData[5] & 0x01;
	__PRINT1(_L("prevent = %d\n"), prevent);
	iDriveManager.SetCritical(aLun, prevent);

	return ETrue;
	}


/** Cancel active state, Invoked by transnport when it stops */
TInt CScsiProtocol::Cancel()
	{
	iDriveManager.SetCritical(CDriveManager::KAllLuns, EFalse);
	return KErrNone;
	}


TBool CScsiProtocol::HandleReadFormatCapacities(TUint aLun)
/**
 * Command Parser for the READ FORMAT CAPACITIES command (0x23)
 *
 * @return ETrue if successful, else a standard Symbian OS error code.
 */
	{
	__FNLOG("CScsiProtocol::HandleReadFormatCapacities");

	CMassStorageDrive* drive=GetCheckDrive(aLun);

	if (drive == NULL)
		{
		return EFalse;
		}

	TLocalDriveCapsV4 driveInfo;

	TInt err = drive->Caps(driveInfo);

	if(err != KErrNone)
		{
		__PRINT1(_L("Can't obtain drive Caps. Err=%d \n"),err);
		iSenseInfo.SetSense(TSenseInfo::ENotReady, TSenseInfo::EMediaNotPresent);
		return EFalse;
		}

	TInt64 driveBlocks = (driveInfo.iDriveAtt & KDriveAttLogicallyRemovable) ? driveInfo.iSize : driveInfo.MediaSizeInBytes();
	driveBlocks /= MAKE_TINT64(0, KDefaultBlockSize);

	TPtr8 writeBuf(NULL, 0);
	iTransport->GetCommandBufPtr(writeBuf, KReadFormatCapacitiesCommandLength);
	writeBuf.FillZ(KReadFormatCapacitiesCommandLength);

	writeBuf[3] = 0x08;	// Capacity List Length

	TUint32 numBlocks = I64LOW(driveBlocks);

	writeBuf[4] = static_cast<TUint8>((numBlocks & 0xFF000000) >> 24);	// Number of blocks
	writeBuf[5] = static_cast<TUint8>((numBlocks & 0x00FF0000) >> 16);	//
	writeBuf[6] = static_cast<TUint8>((numBlocks & 0x0000FF00) >> 8);	//
	writeBuf[7] = static_cast<TUint8>((numBlocks & 0x000000FF));		//

	writeBuf[8] = 0x02;	// Formatted size

	writeBuf[9]  = 0x00;	// 512 Byte Blocks
	writeBuf[10] = 0x02;	// 
	writeBuf[11] = 0x00;	// 

	TPtrC8 writeBuf1 = writeBuf;

	iTransport->SetupWriteData(writeBuf1);

	return ETrue;
	}


/**
Command Parser for the READ CAPACITY(10) command (0x25)

@param aData command data (started form position 1)
@param aLun Logic unit number 
@return ETrue if successful.
*/
TBool CScsiProtocol::HandleReadCapacity(TPtrC8& aData, TUint aLun)
	{
	__FNLOG("CScsiProtocol::HandleReadCapacity");
	CMassStorageDrive* drive=GetCheckDrive(aLun);

	if (drive == NULL)
		{
		return EFalse;
		}

	TInt pmi = aData[9] & 0x01;
	TInt lba = aData[3] | aData[4] | aData[5] | aData[6];

	if (pmi || lba)   //do not support partial medium indicator
		{
		iSenseInfo.SetSense(TSenseInfo::EIllegalRequest, TSenseInfo::EInvalidFieldInCdb);
		return EFalse;
		} 

	TLocalDriveCapsV4 driveInfo;

	TInt err = drive->Caps(driveInfo);

	if(err != KErrNone)
		{
		__PRINT1(_L("Can't obtain drive Caps. Err=%d \n"),err);
		iSenseInfo.SetSense(TSenseInfo::ENotReady, TSenseInfo::EMediaNotPresent);
		return EFalse;
		}

	TInt64 driveBlocks = 0;
	if (driveInfo.iDriveAtt & KDriveAttLogicallyRemovable)
		{
		// Partition Access only 
		driveBlocks = driveInfo.iSize / MAKE_TINT64(0, KDefaultBlockSize);
		}
	else
		{
		// whole Media Access
		driveBlocks = driveInfo.MediaSizeInBytes() / MAKE_TINT64(0, KDefaultBlockSize) - 1;
		}
	

	TPtr8 writeBuf(NULL, 0);
	iTransport->GetCommandBufPtr(writeBuf, KReadCapacityCommandLength);
	writeBuf.FillZ(KReadCapacityCommandLength);

	if (I64HIGH(driveBlocks) == 0)
		{
		TUint32 numBlocks = I64LOW(driveBlocks);

		__PRINT2(_L("Block size=%d, NumBlocks=%d\n"), KDefaultBlockSize, numBlocks);
		writeBuf[0] = static_cast<TUint8>((numBlocks & 0xFF000000) >> 24);	// Number of blocks
		writeBuf[1] = static_cast<TUint8>((numBlocks & 0x00FF0000) >> 16);
		writeBuf[2] = static_cast<TUint8>((numBlocks & 0x0000FF00) >> 8);
		writeBuf[3] = static_cast<TUint8>((numBlocks & 0x000000FF));
		}
	else   
		{
		writeBuf[0] = writeBuf[1] = writeBuf[2] = writeBuf[3] = 0xFF;  // indicate that size more then )0xFFFFFFFF
		}

	writeBuf[4] = static_cast<TUint8>((KDefaultBlockSize & 0xFF000000) >> 24);	// Block Size
	writeBuf[5] = static_cast<TUint8>((KDefaultBlockSize & 0x00FF0000) >> 16);
	writeBuf[6] = static_cast<TUint8>((KDefaultBlockSize & 0x0000FF00) >> 8);
	writeBuf[7] = static_cast<TUint8>((KDefaultBlockSize & 0x000000FF));

	TPtrC8 writeBuf1 = writeBuf;
	iTransport->SetupWriteData(writeBuf1);

	return KErrNone;
	}


/**
Command Parser for the READ10 command (0x28)

@param aData command data (started form position 1)
@param aLun Logic unit number
@return ETrue if successful.
*/
TBool CScsiProtocol::HandleRead10(TPtrC8& aData, TUint aLun)
	{
	__FNLOG("CScsiProtocol::HandleRead10");
	CMassStorageDrive* drive = GetCheckDrive(aLun);
	if (drive == NULL)
		{
		return EFalse;
		}
	TInt rdProtect = aData[2] >> 5;
	if (rdProtect)
		{
		iSenseInfo.SetSense(TSenseInfo::EIllegalRequest, TSenseInfo::EInvalidFieldInCdb);
		return EFalse;
		}

	const TUint32 lba = LBA(aData);
	const TUint16 len = LEN(aData);

	__PRINT2(_L("READ(10) : LBA = 0x%x, Length = %d  (blocks)\n"), lba, len);

	if (!len)
		{
		return ETrue; // do nothing - this is not an error
		}

	TLocalDriveCapsV4 driveInfo;
	TInt err = drive->Caps(driveInfo);
	if (err != KErrNone)
		{
		__PRINT1(_L("Can't obtain drive Caps. Err=%d \n"), err);
		iSenseInfo.SetSense(TSenseInfo::ENotReady, TSenseInfo::EMediaNotPresent);
		return EFalse;
		}

	const TInt64 bOffset = MAKE_TINT64(0, lba) * KDefaultBlockSize;
	const TInt bLength = len * KDefaultBlockSize;
	const TInt64 theEnd = bOffset + MAKE_TINT64(0, bLength);
	const TInt64 mediaSize = (driveInfo.iDriveAtt & KDriveAttLogicallyRemovable) ? driveInfo.iSize : driveInfo.MediaSizeInBytes() ;
	
	if (theEnd > mediaSize)  //check if media big enough for this request
		{
		__PRINT(_L("err - Request ends out of media\n"));
		iSenseInfo.SetSense(TSenseInfo::EIllegalRequest, TSenseInfo::ELbaOutOfRange);
		return EFalse;
		}

#ifdef MSDC_MULTITHREADED
	iWriteDriveThread->WaitForWriteEmpty();

	// check if our buffer can hold requested data
	if (iReadDriveThread->iThreadContext->MaxBufferLength() < bLength)
		{
		__PRINT(_L("err - Buffer too small\n"));
		iSenseInfo.SetSense(TSenseInfo::EIllegalRequest, TSenseInfo::EInvalidFieldInCdb);
		return EFalse;
		}

    // Optimisation note : If the host is reading from sectors it just wrote to,
    // then we have to force a cache-miss so that the real data is read from the
    // drive. It would be possible to service the read from the write buffers, 
    // but as the host is probably trying to verify the write data, we don't do 
    // that for now. 
	if (!iReadDriveThread->ReadDriveData(drive, bOffset, bLength, iWriteDriveThread->IsRecentlyWritten(bOffset,bLength)))
		{
		iSenseInfo.SetSense(TSenseInfo::ENotReady, TSenseInfo::EMediaNotPresent);
		return EFalse;
		}

	iWriteDriveThread->SetCommandWrite10(EFalse);
	TBlockDesc* &desc = iReadDriveThread->iThreadContext->iBuffer.iDescReadPtr;
	TPtrC8 writeBuf1 = desc->iBuf;
#else
	
	TPtr8 writeBuf(NULL, 0);
	iTransport->GetReadDataBufPtr(writeBuf);
	// check if our buffer can hold requested data
	if (writeBuf.MaxLength() < bLength)
		{
		__PRINT(_L("err - Buffer too small\n"));
		iSenseInfo.SetSense(TSenseInfo::EIllegalRequest, TSenseInfo::EInvalidFieldInCdb);
		return EFalse;
		}

	err = drive->Read(bOffset, bLength, writeBuf, drive->IsWholeMediaAccess());

	if (err != KErrNone)
		{
		__PRINT1(_L("Read failed, err=%d\n"), err);
		iSenseInfo.SetSense(TSenseInfo::ENotReady, TSenseInfo::EMediaNotPresent);
		return EFalse;
		}

	TPtrC8 writeBuf1 = writeBuf;
#endif // MSDC_MULTITHREADED
#ifdef USB_TRANSFER_PUBLISHER
	iBytesRead[aLun] += writeBuf1.Length();
#endif
	iReadTransferPublisher->StartTimer();

	// Set up data write to the host
	iTransport->SetupWriteData(writeBuf1);

	return ETrue;
	}


/**
Command Parser for the WRITE(10) command (0x2A)

@param aData command data (started form position 1)
@param aLun Logic unit number
@return ETrue if successful.
*/
TBool CScsiProtocol::HandleWrite10(TPtrC8& aData, TUint aLun)
	{
	__FNLOG("CScsiProtocol::HandleWrite10");
	CMassStorageDrive* drive = GetCheckDrive(aLun);
	if (drive == NULL)
		{
		return EFalse;
		}
	TInt wrProtect = aData[2] >> 5;
	if (wrProtect)
		{
		iSenseInfo.SetSense(TSenseInfo::EIllegalRequest, TSenseInfo::EInvalidFieldInCdb);
		return EFalse;
		}

	const TUint32 lba = LBA(aData);
	const TUint16 len = LEN(aData);
	__PRINT2(_L("WRITE(10) : LBA = 0x%x, Length = %d (blocks)\n"), lba, len);
	if (!len)
		{
		return ETrue; // do nothing - this is not an error
		}

	TLocalDriveCapsV4 driveInfo;
	TInt err = drive->Caps(driveInfo);
	if (err != KErrNone)
		{
		__PRINT1(_L("Can't obtain drive Caps. Err=%d \n"), err);
		iSenseInfo.SetSense(TSenseInfo::ENotReady, TSenseInfo::EMediaNotPresent);
		return EFalse;
		}
	if (driveInfo.iMediaAtt & KMediaAttWriteProtected ||
		driveInfo.iMediaAtt & KMediaAttLocked)
		{
		iSenseInfo.SetSense(TSenseInfo::EDataProtection, TSenseInfo::EWriteProtected);
		return EFalse;
		}

	const TInt64 bOffset = MAKE_TINT64(0, lba) * KDefaultBlockSize;
	iBytesRemain = len * KDefaultBlockSize;
	const TInt64 theEnd = bOffset + MAKE_TINT64(0, iBytesRemain);
	const TInt64 mediaSize = (driveInfo.iDriveAtt & KDriveAttLogicallyRemovable) ? driveInfo.iSize : driveInfo.MediaSizeInBytes() ;

	if (theEnd > mediaSize)  //check if media big enough for this request
		{
		__PRINT(_L("err - Request ends out of media\n"));
		iSenseInfo.SetSense(TSenseInfo::EIllegalRequest, TSenseInfo::ELbaOutOfRange);
		return EFalse;
		}

#ifdef MSDC_MULTITHREADED
	iWriteDriveThread->SetCommandWrite10(ETrue);
#endif

	// Set up the first request for data from the host - either
	// KMaxBufSize or the entire transfer length, whichever is smallest.
	TUint thisLength = (iBytesRemain > KMaxBufSize) ? KMaxBufSize : iBytesRemain;
	thisLength = (thisLength > iMediaWriteSize) ? iMediaWriteSize : thisLength;

	iOffset = bOffset;
	iLastCommand = EWrite10;
	iLastLun = aLun;

	iWriteTransferPublisher->StartTimer();
	iTransport->SetupReadData(thisLength);

	return ETrue;
	}


/**
Command Parser for the VERIFY(10) command (0x2F)

@param aData command data (started form position 1)
@param aLun Logic unit number
@return ETrue if successful.
*/
TBool CScsiProtocol::HandleVerify10(TPtrC8& aData, TUint aLun)
	{
	__FNLOG("CScsiProtocol::HandleVerify10");
	CMassStorageDrive* drive = GetCheckDrive(aLun);
	if (drive == NULL)
		{
		return EFalse;
		}

	TInt vrProtect = aData[2] >> 5;
	if (vrProtect)
		{
		iSenseInfo.SetSense(TSenseInfo::EIllegalRequest, TSenseInfo::EInvalidFieldInCdb);
		return EFalse;
		}

	const TUint32 lba = LBA(aData);
	const TUint16 len = LEN(aData);
	__PRINT2(_L("VERIFY(10) : LBA = %d, Length = %d  (blocks)\n"), lba, len);

	TInt bytChk = aData[2] & 0x02;
	if (!len)
		{
		return ETrue; // do nothing - this is not an error
		}

	TLocalDriveCapsV4 driveInfo;
	TInt err = drive->Caps(driveInfo);
	if (err != KErrNone)
		{
		__PRINT1(_L("Can't obtain drive Caps. Err=%d \n"), err);
		iSenseInfo.SetSense(TSenseInfo::ENotReady, TSenseInfo::EMediaNotPresent);
		return EFalse;
		}

	const TInt64 bOffset = MAKE_TINT64(0, lba) * KDefaultBlockSize;
	const TInt bLength = len * KDefaultBlockSize;
	const TInt64 theEnd = bOffset + MAKE_TINT64(0, bLength);
	const TInt64 mediaSize = (driveInfo.iDriveAtt & KDriveAttLogicallyRemovable) ? driveInfo.iSize : driveInfo.MediaSizeInBytes() ;

	// check if media big enough for this request
	if (theEnd > mediaSize)
		{
		iSenseInfo.SetSense(TSenseInfo::EIllegalRequest, TSenseInfo::ELbaOutOfRange);
		return EFalse;
		}

	// check if our buffer can hold requested data
#ifdef MSDC_MULTITHREADED
	if (iWriteDriveThread->iThreadContext->MaxBufferLength() < bLength)
#else
	TPtr8 writeBuf(NULL, 0);
	iTransport->GetReadDataBufPtr(writeBuf);
	if (writeBuf.MaxLength() < bLength)
#endif
		{
		iSenseInfo.SetSense(TSenseInfo::EIllegalRequest, TSenseInfo::EInvalidFieldInCdb);
		return EFalse;
		}

	if (!bytChk)
		{
		// BYTCHK==0 : Perform a medium verification with no data comparison and not transfer any data from the application client data-out buffer.
		// The device should attempt to read from the specified locations
#ifdef MSDC_MULTITHREADED
		TPtr8 writeBuf = iWriteDriveThread->iThreadContext->GetReadBuffer(bLength);
#else
		writeBuf.SetLength(bLength);
#endif
		err = drive->Read(bOffset, bLength, writeBuf, drive->IsWholeMediaAccess());
		if (err != KErrNone)
			{
			iSenseInfo.SetSense(TSenseInfo::EMisCompare);
			return EFalse;
			}
		return ETrue;
		}

	// BYTCHK==1 : perform a byte-by-byte comparison of user data read from the medium & user data transferred from the application client data-out buffer.
	// The host sends data in the data-transport phase, and the device should verify that the received data matches what is stored in the device.

	iOffset = bOffset;
	iLastCommand = EVerify10;
	iLastLun = aLun;

	iTransport->SetupReadData(bLength);

	return ETrue;
	}


/**
Called by the transport when the requested data has been read or an error has
occurred during the read.

@param aError Indicate if an error occurs during reading data by transport.
@return KErrAbort if command processing is complete but has failed,
        KErrCompletion if sufficient data is available in the buffer to process
        the transfer immediately, KErrNotReady if insufficient data is
        available in the buffer so the transport should wait for it to arrive,
        KErrNone if command processing is complete and was successful.
*/
TInt CScsiProtocol::ReadComplete(TInt aError)
	{
	__FNLOG("CScsiProtocol::ReadComplete");
	__PRINT1(_L("Error = 0x%X \n"), aError);
	const TInt64 bOffset = iOffset;
	TUint8 lastCommand = iLastCommand;
	TUint lastLun = iLastLun;

	iOffset = 0;
	iLastCommand = EUndefinedCommand;
	iLastLun = KUndefinedLun;

	__PRINT1(_L("Last command was: %s\n"),
			 (lastCommand == EUndefinedCommand) ? _S("Undefined") :
			 ((lastCommand == EWrite10) ? _S("EWrite10") :
			  ((lastCommand == EVerify10) ? _S("EVerify10") :
			   _S("Unknown"))));

	if (aError != KErrNone ||
		lastCommand == EUndefinedCommand ||
		lastLun == KUndefinedLun)
		{
		iSenseInfo.SetSense(TSenseInfo::EAbortedCommand);
		return KErrAbort;
		}

	CMassStorageDrive* drive = GetCheckDrive(lastLun);
	if (drive == NULL)
		{
		return KErrAbort;
		}

	if (lastCommand == EWrite10)
		{
		TPtrC8 writeBuf(NULL, 0);
		iTransport->GetWriteDataBufPtr(writeBuf);
		
#ifdef USB_TRANSFER_PUBLISHER
	iBytesWritten[lastLun] += writeBuf.Length();
#endif

#ifdef MSDC_MULTITHREADED
		TInt err = iWriteDriveThread->WriteDriveData(drive, bOffset, writeBuf, ProcessWriteComplete, this);

		if (err != KErrNone)
			{
			iDeferredSenseInfo.SetSense(TSenseInfo::EMediumError);
			}

		TUint thisLength = iWriteDriveThread->WriteBufferLength();
#else
#ifdef MEASURE_AND_DISPLAY_WRITE_TIME
		RDebug::Print(_L("SCSI: writing %d bytes\n"), writeBuf.Length());
		TTime t0, t1;
		t0.HomeTime();
#else
		__PRINT1(_L("SCSI: writing %d bytes\n"), writeBuf.Length());
#endif

#ifdef INJECT_ERROR
		if (writeBuf[0] == '2')
		{
			writeBuf[0] = 'x';
			RDebug::Printf("Injecting error");
		}

		RDebug::Printf("%08lx %x [%x] [%x]", bOffset,writeBuf.Length(), 
			writeBuf[0],
			writeBuf[writeBuf.Length()-1]);
#endif
		
		TInt err = drive->Write(bOffset, writeBuf, drive->IsWholeMediaAccess());

#ifdef INJECT_ERROR
		if (writeBuf[0] == 'x')
		{
			err = KErrUnknown;
		}
#endif

#ifdef MEASURE_AND_DISPLAY_WRITE_TIME
		t1.HomeTime();
		const TTimeIntervalMicroSeconds time = t1.MicroSecondsFrom(t0);
		const TUint time_ms = I64LOW(time.Int64() / 1000);
		RDebug::Print(_L("SCSI: write took %d ms\n"), time_ms);
#endif
		if (err != KErrNone)
			{
			__PRINT1(_L("Error after write = 0x%X \n"), err);
			iSenseInfo.SetSense(TSenseInfo::EAbortedCommand);
			return KErrAbort;
			}

		TUint thisLength = writeBuf.Length();
#endif // MSDC_MULTITHREADED
		iOffset = bOffset + MAKE_TINT64(0, thisLength);
		iBytesRemain -= thisLength;
		if ((TInt)iBytesRemain > 0)
			{
			// More data is expected - set up another request to read from the host
			iLastCommand = EWrite10;
			iLastLun = lastLun;

			TUint minLength = (iBytesRemain < iMediaWriteSize) ? iBytesRemain : iMediaWriteSize;
			TUint bytesAvail = iTransport->BytesAvailable() & ~(KDefaultBlockSize-1);

			TBool wait = EFalse;
			thisLength = bytesAvail ? bytesAvail : minLength;
			if (thisLength < minLength)
				{
				// Not enough data is available at the transport to satisfy the request,
				// so return KErrNotReady to indicate that the transport should wait.
				thisLength = minLength;
				wait = ETrue;
				}

			thisLength = (thisLength > KMaxBufSize) ? KMaxBufSize : thisLength;

			iTransport->SetupReadData(thisLength);

			return wait ? KErrNotReady : KErrCompletion;
			}
		}
	else if (lastCommand == EVerify10)
		{
		HBufC8* hostData = NULL;
		TPtrC8 writeBuf(NULL, 0);
		iTransport->GetWriteDataBufPtr(writeBuf);
#ifdef MSDC_MULTITHREADED
		TRAPD(err, hostData = HBufC8::NewL(writeBuf.Length()));
#else
		TRAPD(err, hostData = HBufC8::NewL(writeBuf.Length()));
#endif
		if (err != KErrNone || hostData == NULL)
			{
			iSenseInfo.SetSense(TSenseInfo::EAbortedCommand, TSenseInfo::EInsufficientRes);
			return KErrAbort;
			}

#ifdef MSDC_MULTITHREADED
		// copy the data
		*hostData = writeBuf;
		TPtr8 readBuf = iWriteDriveThread->iThreadContext->GetReadBuffer(); 
		err = drive->Read(bOffset, writeBuf.Length(), readBuf, drive->IsWholeMediaAccess());
		if (err == KErrNone)
			{
			err = (hostData->Compare(readBuf) == 0) ? KErrNone : KErrCorrupt;
			}
#else
		*hostData = writeBuf;
		TPtr8 readBuf((TUint8*) writeBuf.Ptr(), writeBuf.Length()); 
		err = drive->Read(bOffset, writeBuf.Length(), readBuf, drive->IsWholeMediaAccess());
		if (err == KErrNone)
			{
			err = (hostData->Compare(readBuf) == 0) ? KErrNone : KErrCorrupt;
			}
#endif

		if (err != KErrNone)
			{
			iSenseInfo.SetSense(TSenseInfo::EMisCompare);
			}

		delete hostData;
		}
	else // unknown command
		{
		iSenseInfo.SetSense(TSenseInfo::EAbortedCommand);
		}
	return iSenseInfo.SenseOk() ? KErrNone : KErrAbort;
	}


/**
Command Parser for the MODE SENSE(06) command (0x1A)

@return ETrue if successful.
*/
TBool CScsiProtocol::HandleModeSense6(TPtrC8& aData, TUint aLun)
	{
	__FNLOG("CScsiProtocol::HandleModeSense6");

	TInt pageCode = aData[3] & 0x3F;
	TUint8 pageControl= static_cast<TUint8>(aData[3] >>6);

	if (pageCode != KAllPages || pageControl == KChangeableValues) 
		{
		__PRINT(_L("TSenseInfo::EIllegalRequest,TSenseInfo::EInvalidFieldInCdb"));
		iSenseInfo.SetSense(TSenseInfo::EIllegalRequest,TSenseInfo::EInvalidFieldInCdb);
		return EFalse;
		}

	// reserve 4 bytes for Length, Media type, Device-specific parameter and Block descriptor length
	TPtr8 writeBuf(NULL, 0);
	iTransport->GetCommandBufPtr(writeBuf, KModeSense6CommandLength);
	writeBuf.FillZ(KModeSense6CommandLength);

	if (pageControl != KDefaultValues)
		{
		//check if drive write protected
		CMassStorageDrive* drive=GetCheckDrive(aLun);
		if (drive == NULL)
			{
			__PRINT(_L("drive == null"));
			return EFalse;
			}

		TLocalDriveCapsV4 driveInfo;
		TInt err = drive->Caps(driveInfo);
		if (err != KErrNone)
			{
			__PRINT(_L("TSenseInfo::ENotReady"));
			iSenseInfo.SetSense(TSenseInfo::ENotReady, TSenseInfo::EMediaNotPresent);
			return EFalse ;
			}

		if (driveInfo.iMediaAtt & KMediaAttWriteProtected)
			{
			writeBuf[2] = 1<<7;  // set SWP bit at the Device Specific parameters
			}
		}

	writeBuf[0]=3;  //Sending only Mode parameter header

	TPtrC8 writeBuf1 = writeBuf;

	iTransport->SetupWriteData(writeBuf1);

	return (iSenseInfo.SenseOk());
	}


/**
Command Parser for the MODE SENSE(10) command (0x5A)

@return ETrue if successful.
*/
TBool CScsiProtocol::HandleModeSense10(TPtrC8& aData, TUint aLun)
	{
	__FNLOG("CScsiProtocol::HandleModeSense10");

	TInt pageCode = aData[3] & 0x3F;
	TUint8 pageControl= static_cast<TUint8>(aData[3] >>6);

	if (pageCode != KAllPages || pageControl == KChangeableValues) 
		{
		__PRINT(_L("TSenseInfo::EIllegalRequest,TSenseInfo::EInvalidFieldInCdb"));
		iSenseInfo.SetSense(TSenseInfo::EIllegalRequest,TSenseInfo::EInvalidFieldInCdb);
		return EFalse;
		}

	// reserve 8 bytes for Length, Media type, Device-specific parameter and Block descriptor length
	TPtr8 writeBuf(NULL, 0);
	iTransport->GetCommandBufPtr(writeBuf, KModeSense10CommandLength);
	writeBuf.FillZ(KModeSense10CommandLength);

	if (pageControl != KDefaultValues)
		{
		//check if drive write protected
		CMassStorageDrive* drive=GetCheckDrive(aLun);
		if (drive == NULL)
			{
			__PRINT(_L("drive == null"));
			return EFalse;
			}

		TLocalDriveCapsV4 driveInfo;
		TInt err = drive->Caps(driveInfo);
		if (err != KErrNone)
			{
			__PRINT(_L("TSenseInfo::ENotReady"));
			iSenseInfo.SetSense(TSenseInfo::ENotReady, TSenseInfo::EMediaNotPresent);
			return EFalse ;
			}

		if (driveInfo.iMediaAtt & KMediaAttWriteProtected)
			{
			writeBuf[3] = 1<<7;  // set SWP bit at the Device Specific parameters
			}
		}

	writeBuf[1]=6;  //Sending only Mode parameter header

	TPtrC8 writeBuf1 = writeBuf;

	iTransport->SetupWriteData(writeBuf1);

	return (iSenseInfo.SenseOk());
	}



