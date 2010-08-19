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
// e32\drivers\medata\pccd_ata.cpp
// 
//

#include "pbusmedia.h"
#include "platform.h"
#include "ata.h"

const TInt KErrBadDrqOnRead=-64;
const TInt KErrBadDrqOnWrite=-65;
const TInt KErrBadDrq=-66;
const TInt KErrAta=-67;
const TInt KErrBadPccdConfig=-68;
//const TInt KErrDriveChunkNotOpen=-69;

const TInt KAtaDriverPriority=KMediaDriverPriorityNormal;
//const TInt KAtaDriverPriority=KMediaDriverPriorityHigh;

_LIT(KPddName, "Media.Ata");
//_LIT(KPddName, "Media.Ata2");

// One of these
#define SELECT_CONTIGUOUS_IO_CONFIG
//#define SELECT_PRIMARY_IO_CONFIG
//#define SELECT_MEMORY_CONFIG

//#define FORCE_8BIT_ACCESSES

// Special debug options
//#define SHOW_CARD_ERRORS
//#define DEBUG_WITH_HW_TRIGGER
//#define COUNT_TIMEOUTS

#if (defined(SHOW_CARD_ERRORS))
#define __KTRACE_CARD_ERROR(a,p) {p;}
#elif (defined(_DEBUG))
#define __KTRACE_CARD_ERROR(a,p) {if((KDebugNum(a)))p;}
#else
#define __KTRACE_CARD_ERROR(a,p)
#endif

#include <pccard.h>

const TInt KMaxSectorsPerRead=32;
const TInt KMaxSectorsPerWrite=8;				 
const TInt KMaxSectorsPerFormat=8;				 
const TInt KMaxBytesPerRead=(KMaxSectorsPerRead<<KAtaSectorShift);
const TInt KMaxBytesPerWrite=(KMaxSectorsPerWrite<<KAtaSectorShift);

// Sector buffer size must be a multiple of sector size and at least as large
// as KMaxSectorsPerWrite.
const TInt KSectBufSizeInSectors=8;
const TInt KSectBufSizeInBytes=(KSectBufSizeInSectors<<KAtaSectorShift);
const TInt KSectBufSizeInBytesMinusOneSector=(KSectBufSizeInBytes-KAtaSectorSize);

const TInt KIdleCurrentInMilliAmps=1; 
const TInt KReadCurrentInMilliAmps=39;
const TInt KWriteCurrentInMilliAmps=46;

const TInt KNotBusySyncTimeout=5;
const TInt KNotBusySyncRetryCount=10;
const TInt KDriveReadySyncTimeout=5;

#ifdef _DEBUG
const TInt KNotBusyTestInterval=30;		// Check for not-busy once every 30ms
const TInt KBusyTimeOut=67;				// Timeout after this many tests (67*30ms=2010ms)
#else
const TInt KNotBusyTestInterval=5;		// Check for not-busy once every 5ms
const TInt KBusyTimeOut=400;			// Timeout after this many tests (400*5ms=2010ms)
#endif

class DPhysicalDeviceMediaAta : public DPhysicalDevice
	{
public:
	DPhysicalDeviceMediaAta();
	virtual TInt Install();
	virtual void GetCaps(TDes8& aDes) const;
	virtual TInt Create(DBase*& aChannel, TInt aMediaId, const TDesC8* anInfo, const TVersion& aVer);
	virtual TInt Validate(TInt aDeviceType, const TDesC8* anInfo, const TVersion& aVer);
	virtual TInt Info(TInt aFunction, TAny* a1);
	};

class DPcCardMediaDriverAta : public DMediaDriver
	{
public:
	DPcCardMediaDriverAta(TInt aMediaId);
	virtual TInt Request(TLocDrvRequest& aRequest);
	virtual TInt PartitionInfo(TPartitionInfo& anInfo);
	virtual void NotifyPowerDown();
	virtual void NotifyEmergencyPowerDown();
	virtual void Close();
public:
	enum TCardStatus {ECardIdle,ECardRead,ECardWrite,ECardFormat};
	inline TUint8 AtaRegister8(TUint aReg);
	inline void SetAtaRegister8(TUint8 aValue,TUint aReg);
	void ModifyAtaRegister8(TUint aClearMask,TUint aSetMask,TUint aReg);
	void SelectDrive(TAtaDriveSelect aDrive);
	TBool WaitForNotBusy();
	TBool DriveReadyForCommand(TInt aDrive);
	void SetLbaSectorAddress(TUint aSector);
	void SetChsSectorAddress(TUint aSector);
	TInt IssueAtaCommand(TUint8 aCmd,TUint aFirstSector,TInt aSectorCount);
	TInt EmptySectBufferToTrg(TUint8 *aSrc,TUint aTrgOffset,TInt aLen);
	TInt LoadSectBufferFromSrc(TInt aLen, TUint8* aBuf);
	TInt LoadSectBufferFromDrive(TAny *aBuf);
	TInt EmptySectBufferToDrive(TUint8 *aBuf);
	TInt TransferSectBufferFromDrive(TAny *aBuf);
	TInt FinishCommand();
	TInt CheckForError();
	TInt ProcessError(TInt anError);
	TInt SectorBoundaryReadCheck(TUint aStartSector,TUint aStartSectOffset,Int64 anEndPos);
	TInt SectorRead(TUint aFirstSector,TUint8 *aBuf,TInt aSectorCount=1,TUint8 aCmd=KAtaCmdReadSectors);
	TInt CheckDevice(TBool aCheckPower);
	TInt InitiateWriteCommand(TUint aFirstSector,TInt aSectorCount,TUint8 *aSectBuffer);
	TInt ReadSectorsCommand(TUint aFirstSector,TUint aBufOffset,TInt aLen);
	TInt IdentifyDrive();
	void DumpIdentifyDriveInfo();
	TInt ConfigAutoPowerDown();
	TInt Open();
	TInt DoOpen();
	void DoClose();
	void Reset();
	TBool CardPoweredDown();
	void IncrementSectBufPtr();
	static void CardIreqDfcFunction(TAny* aPtr);
	static void TimerDfcFunction(TAny* aPtr);
	static void AsyncBusyTimerCallBack(TAny* aMediaDriver);
	static void SyncBusyTimerCallBack(TAny* aBusyFlag);
	static void CardIntCallBack(TAny* aPtr, TInt anId);
	TBool DoCardNotBusy(TInt &anErr);
	TBool CmdDfc();
	TBool DoCmdDfc(TInt &anErr);
	void Complete(TInt anError);
	TInt Caps(TLocalDriveCapsV6& anInfo);
	TInt DoRead();
	TInt DoWrite();
	TInt DoFormat();
	TInt InitiateAsyncRead();
	TInt InitiateAsyncWrite();
public:
	DPcCardSocket* iSocket;
	TLocDrvRequest* iCurrentReq;
	TPBusCallBack iCardIntCallBack;
	RPccdWindow iDriveChunk;
	TPccdMemType iMemoryType;
	TDriveParameters iDriveInfo;
	NTimer iBusyTimeout;
	TDfc iCardIreqDfc;
	TDfc iTimerDfc;
	TInt iNotBusyTickCount;
	TInt iCommandError;
	TCardStatus iCardStatus;
	TUint iCmdInOffset; 	// Progress counter for data received from src device
	TUint iCmdOutOffset;	// Progress counter for data delivered to target device
	TUint iCmdEndOffset;	// Marks point when transfer associated with command is complete
	TUint iCmdLength;		// Transfer length remaining
	TUint iNextSector;		// Next sector to transfer
	TUint8 *iSectBufPtr;	// Progress counter for tranfering data between card and sector buffer
	TUint iSectBufOffset;	// Offset within sector buffer to start of data involved in command (Reads only)
	TBool iLastSectorBufUsed;
	TUint iHiddenSectors;
	TInt iCardFuncNum;
	TUint8 iSectorBuf[KSectBufSizeInBytes]; // Keep on 4byte boundary - put this last
	TUint8 iLastSectorBuf[KAtaSectorSize];	// Holds last sector data for unaligned write
#if (defined(_DEBUG) || defined(SHOW_CARD_ERRORS))
	TInt iDbgLastError;
	TInt iDbgPos;
	TInt iDbgLen;
#endif
#ifdef COUNT_TIMEOUTS
	TInt iInts;
	TInt iTimeouts;
	TInt iImmediateNotBusy;
	TInt iChainedReads;
	TInt iChainedWrites;
	TBool iNewTimeOut;
	TInt iIndex;
	TInt iInfo[8];
#endif
	};

void DPcCardMediaDriverAta::Complete(TInt anError)
	{
	__KTRACE_OPT(KFAIL,Kern::Printf("mdrqc %08x %d",iCurrentReq,anError));
#ifdef COUNT_TIMEOUTS
	if (iNewTimeOut)
		{
		iNewTimeOut=EFalse;
		Kern::Printf("I=%d T=%d M=%d CR=%d CW=%d",iInts,iTimeouts,iImmediateNotBusy,iChainedReads,iChainedWrites);
		TInt i;
		for (i=0; i<iIndex; i+=2)
			Kern::Printf("%d: %08x %08x",i,iInfo[i],iInfo[i+1]);
		iIndex=0;
		}
#endif
	TLocDrvRequest* pR=iCurrentReq;
	if (pR)
		{
		iCurrentReq=NULL;
		DMediaDriver::Complete(*pR,anError);
		}
	}

inline TUint8 DPcCardMediaDriverAta::AtaRegister8(TUint aReg)
//
// Read from an 8 bit ATA register
//
	{

	return iDriveChunk.Read8(aReg);
	}

inline void DPcCardMediaDriverAta::SetAtaRegister8(TUint8 aValue,TUint aReg)
//
// Write to an 8 bit ATA register
//
	{

	iDriveChunk.Write8(aReg,aValue);
	}

void DPcCardMediaDriverAta::ModifyAtaRegister8(TUint aClearMask,TUint aSetMask,TUint aReg)
//
// Modify an 8 bit ATA register
//
	{

	SetAtaRegister8((TUint8)((AtaRegister8(aReg)&(~aClearMask))|aSetMask),aReg);
	}

void DPcCardMediaDriverAta::SelectDrive(TAtaDriveSelect aDrive)
//
// Modify an 8 bit ATA register
//
	{

	ModifyAtaRegister8(KAtaDrvHeadDrive1,(0xA0|aDrive),KAtaSelectDriveHeadRdWr8);
	}

TBool DPcCardMediaDriverAta::WaitForNotBusy()
//
// Poll busy flag (while card accesses the data buffer and command register). 
// 8mS timeout.
//
	{

	// Before we start a timed loop, just check it isn't already not busy
	__KTRACE_OPT(KPBUSDRV,Kern::Printf("WfnB"));
	if (!(AtaRegister8(KAtaStatusRd8)&KAtaStatusBusy))
		return(ETrue);
#if (defined(_DEBUG) || defined(SHOW_CARD_ERRORS))
	TUint c=NKern::TickCount();
#endif
	volatile TBool timedOut=EFalse;
	NTimer busyTimeout(SyncBusyTimerCallBack,(TAny*)&timedOut);
	busyTimeout.OneShot(NKern::TimerTicks(KNotBusySyncTimeout));
	FOREVER
		{
		if (!(AtaRegister8(KAtaStatusRd8)&KAtaStatusBusy)||timedOut)
			break;
		}
	if (!timedOut)
		busyTimeout.Cancel();
	else
		{
		TInt retry=KNotBusySyncRetryCount;
		while ((AtaRegister8(KAtaStatusRd8)&KAtaStatusBusy) && retry)
			{
			NKern::Sleep(1);
			retry--;
			}
		}
	TBool ret=(AtaRegister8(KAtaStatusRd8) & KAtaStatusBusy);
#if (defined(_DEBUG) || defined(SHOW_CARD_ERRORS))
	c=NKern::TickCount()-c;
	if (ret)
		{
		__KTRACE_CARD_ERROR(KPBUSDRV,Kern::Printf("<Ata:WaitForNotBusy-timeout(%xH) %d ms",AtaRegister8(KAtaStatusRd8),c));
 		}
	else
		{
		__KTRACE_CARD_ERROR(KPBUSDRV,Kern::Printf("<Ata:WaitForNotBusy-OK %d ms",c));
		}
#endif
	return(!ret);
	}

TBool DPcCardMediaDriverAta::DriveReadyForCommand(TInt aDrive)
//
// Wait until selected drive is able to accept a command (5ms timeout).
//					
	{

	// Select the drive were waiting on 
	SelectDrive((aDrive==1)?ESelectDrive1:ESelectDrive0);
	if (!WaitForNotBusy())
		return(EFalse);

	volatile TBool timedOut=EFalse;
	NTimer busyTimeout(SyncBusyTimerCallBack,(TAny*)&timedOut);
	busyTimeout.OneShot(NKern::TimerTicks(KDriveReadySyncTimeout));
	FOREVER
		{
		if (AtaRegister8(KAtaStatusRd8)&KAtaStatusRdy||timedOut)
			break;
		}
	if (!timedOut)
		busyTimeout.Cancel();
	TBool ret=(AtaRegister8(KAtaStatusRd8) & KAtaStatusRdy);
#if (defined(_DEBUG) || defined(SHOW_CARD_ERRORS))
	if (!ret)
		__KTRACE_CARD_ERROR(KPBUSDRV,Kern::Printf("<Ata:DriveReadyForCommand-Fail(%xH)",AtaRegister8(KAtaStatusRd8)));
#endif
	return(ret);
	}

void DPcCardMediaDriverAta::SetLbaSectorAddress(TUint aSector)
//
// Setup the sector address ATA registers (LBA mode)
//
	{
	__KTRACE_OPT(KPBUSDRV,Kern::Printf(">Ata:SetLbaSectorAddress (LBA: %xH)",aSector));

	SetAtaRegister8((TUint8)aSector,KAtaLba7_0RdWr8);
	SetAtaRegister8((TUint8)(aSector>>8),KAtaLba15_8RdWr8);
	SetAtaRegister8((TUint8)(aSector>>16),KAtaLba23_16RdWr8);
	TUint8 lba27_24=(TUint8)((aSector>>24)&0x0F);
	ModifyAtaRegister8(KAtaDrvHeadLba27_24,(KAtaDrvHeadLbaOn|lba27_24),KAtaSelectDriveHeadRdWr8);
	}

void DPcCardMediaDriverAta::SetChsSectorAddress(TUint aSector)
//
// Setup the sector address ATA registers (CHS mode)
//
	{

	TUint cylinder=0,head=0,sector=1;
	if (iDriveInfo.iSectorsPerCylinder>0&&iDriveInfo.iSectorsPerTrack>0)
		{
		cylinder=aSector/iDriveInfo.iSectorsPerCylinder;
		TUint remainder=aSector%iDriveInfo.iSectorsPerCylinder;
		head=remainder/iDriveInfo.iSectorsPerTrack;
		sector=(remainder%iDriveInfo.iSectorsPerTrack)+1;
		}

	__KTRACE_OPT(KPBUSDRV,Kern::Printf("Ata:SetChsSectorAddress (C: %xH H: %xH S: %xH)",cylinder,head,sector));
	SetAtaRegister8((TUint8)sector,KAtaSectorNoRdWr8);
	SetAtaRegister8((TUint8)cylinder,KAtaCylinderLowRdWr8);
	SetAtaRegister8((TUint8)(cylinder>>8),KAtaCylinderHighRdWr8);
	ModifyAtaRegister8((KAtaDrvHeadLbaOn|KAtaDrvHeadLba27_24),((TUint8)(head&0x0F)),KAtaSelectDriveHeadRdWr8);
	}

TInt DPcCardMediaDriverAta::IssueAtaCommand(TUint8 aCmd,TUint aFirstSector,TInt aSectorCount)
//
// Issue an ATA command (Drive 0 only for now).
//
	{

	__KTRACE_OPT(KPBUSDRV,Kern::Printf(">Ata:IssueAtaCommand(C:%x, FS:%x, SC:%x)",aCmd,aFirstSector,aSectorCount));
	__KTRACE_OPT(KFAIL,Kern::Printf("A%02x,%d",aCmd,aSectorCount));
	if (!WaitForNotBusy())
		return(KErrTimedOut);
	if (!DriveReadyForCommand(0))
		return KErrTimedOut;
	
	if (aSectorCount==KMaxSectorsPerCmd)
		aSectorCount=0;
	SetAtaRegister8((TUint8)aSectorCount,KAtaSectorCountRdWr8);
	if (iDriveInfo.iSupportsLba)
		SetLbaSectorAddress(aFirstSector);
	else
		SetChsSectorAddress(aFirstSector);
	__TRACE_TIMING(0x103);
	SetAtaRegister8(aCmd,KAtaCommandWr8);	// Issue the command
	Kern::NanoWait(400);					// Busy flag not asserted for 400ns
	__KTRACE_OPT(KPBUSDRV,Kern::Printf("<Ata:IssueAtaCommand"));
	return(KErrNone);
	}

TInt DPcCardMediaDriverAta::LoadSectBufferFromDrive(TAny *aBuf)
//
// Read a sector from the ATA card
//
	{

	if (!(AtaRegister8(KAtaStatusRd8)&KAtaStatusDrq))
		{
		__KTRACE_CARD_ERROR(KPBUSDRV,Kern::Printf(">Ata:LoadSectBufferFromDrive-Bad drq(%xH)",AtaRegister8(KAtaStatusRd8)));
		return KErrBadDrqOnRead;
		}

	if (__IS_COMMON_MEM(iMemoryType))
		iDriveChunk.Read(KAtaDataRdWrWinBase16,aBuf,KAtaSectorSize); // Use 1K window
	else if (iMemoryType==EPccdIo16Mem)
		iDriveChunk.ReadHWordMultiple(KAtaDataRdWr16,aBuf,(KAtaSectorSize>>1));
	else
		iDriveChunk.ReadByteMultiple(KAtaDataRdWr8,aBuf,KAtaSectorSize); // Must be EPccdIo8Mem

	return(KErrNone);
	}

TInt DPcCardMediaDriverAta::TransferSectBufferFromDrive(TAny *aBuf)
//
// Read a sector from the ATA card
//
	{

	if (!WaitForNotBusy())
		return(KErrTimedOut);
	return(LoadSectBufferFromDrive(aBuf));
	}

TInt DPcCardMediaDriverAta::EmptySectBufferToDrive(TUint8 *aBuf)
//
// Write a sector to the ATA card.
//
	{

	if (!(AtaRegister8(KAtaStatusRd8)&KAtaStatusDrq))
		{
		__KTRACE_CARD_ERROR(KPBUSDRV,Kern::Printf(">Ata:EmptySectBufferToDrive-Bad drq(%xH)",AtaRegister8(KAtaStatusRd8)));
		return KErrBadDrqOnWrite;
		}

	if (__IS_COMMON_MEM(iMemoryType))
		iDriveChunk.Write(KAtaDataRdWrWinBase16,aBuf,KAtaSectorSize); // Use 1K window
	else if (iMemoryType==EPccdIo16Mem)
		iDriveChunk.WriteHWordMultiple(KAtaDataRdWr16,aBuf,(KAtaSectorSize>>1));
	else
		iDriveChunk.WriteByteMultiple(KAtaDataRdWr8,aBuf,KAtaSectorSize); // Must be EPccdIo8Mem

	return(KErrNone); 
	}

TInt DPcCardMediaDriverAta::FinishCommand()
//
// Called each time a command has been issued to check if an error occured.
//
	{

	if (!WaitForNotBusy())
		return(KErrTimedOut);
	return(CheckForError());
	}

TInt DPcCardMediaDriverAta::CheckForError()
//
// Called each time a command has been issued to check if an error occured.
//
	{

	TUint8 status=AtaRegister8(KAtaStatusRd8);
	if (status&KAtaStatusDrq)
		{
		__KTRACE_CARD_ERROR(KPBUSDRV,Kern::Printf("Ata:CheckForError-DRQ fail"));
		return KErrBadDrq;
		}
	if (status&KAtaStatusDwf)
		{
		__KTRACE_CARD_ERROR(KPBUSDRV,Kern::Printf("Ata:CheckForError-DWF fail"));
		return(KErrWrite);	
		}
	if (status&KAtaStatusErr)
		{
		__KTRACE_CARD_ERROR(KPBUSDRV,Kern::Printf("Ata:CheckForError-ERR fail"));
		return KErrAta;
		}
	return(KErrNone);
	}

TInt DPcCardMediaDriverAta::ProcessError(TInt anError)
//
// An error has occured - lets get more information
//
	{

	if (anError==KErrAta&&AtaRegister8(KAtaStatusRd8)&KAtaStatusErr)
		{
		TUint8 basic=AtaRegister8(KAtaErrorRd8);
#if (defined(__EPOC32__) && (defined(_DEBUG) || defined(SHOW_CARD_ERRORS)))
		TUint8 extended=0xFF;		// invalid
		SetAtaRegister8(KAtaCmdRequestSense,KAtaCommandWr8);	// Issue command - request sense
		Kern::NanoWait(400);					// Busy flag not asserted for 400ns
		WaitForNotBusy();
		if (!(AtaRegister8(KAtaStatusRd8)&KAtaStatusErr))
			extended=AtaRegister8(KAtaErrorRd8);
		__KTRACE_CARD_ERROR(KPBUSDRV,Kern::Printf("Ata:ProcessError-Basic:%xH Ext:%xH)",basic,extended));
#endif 
		if (basic&KAtaErrorUnc)
			return(KErrDied);
		else if (basic&(KAtaErrorBbk|KAtaErrorIdnf))
			return(KErrCorrupt);
		else if (basic&KAtaErrorAbort)
			return(KErrCancel);
		else
			return(KErrUnknown);
		}
	else
		return(anError);
	}

TInt DPcCardMediaDriverAta::EmptySectBufferToTrg(TUint8 *aSrc,TUint aTrgOffset,TInt aLen)
//
// Empty data from sector buffer (at specified offset within buffer) into
// destination descriptor.
//
	{

	TPtrC8 buf(aSrc,aLen);
	return iCurrentReq->WriteRemote(&buf,aTrgOffset);
	}

TInt DPcCardMediaDriverAta::LoadSectBufferFromSrc(TInt aLen, TUint8* aBuf)
//
// Load data from source descriptor into sector buffer
// Always called within exec. function.
//
	{

	TPtr8 buf(aBuf,aLen);
	TInt r=iCurrentReq->ReadRemote(&buf,iCmdInOffset);
	return r;
	}

TInt DPcCardMediaDriverAta::ReadSectorsCommand(TUint aFirstSector,TUint aBufOffset,TInt aLen)
//
// Start off a read of multiple sectors from the drive (command completed under interrupt/dfc).
//
	{
	__KTRACE_OPT(KPBUSDRV,Kern::Printf(">Ata:ReadSectors F:%x Off:%x L:%x",aFirstSector,aBufOffset,aLen));

	__ASSERT_DEBUG(aBufOffset<(TUint)KAtaSectorSize,Kern::PanicCurrentThread(_L("ReadSectorsCommand"),0));
	__ASSERT_DEBUG(aLen>0,Kern::PanicCurrentThread(_L("ReadSectorsCommand"),0));

	// Sector count - allow for not starting on sector boundary by adding buffer offset.  Allow for
	// not ending on sector boundary by adding sectorsize-1. Then divide by sector size.
	TInt tferSectorCount=(aLen+aBufOffset+KAtaSectorSizeMinusOne)>>KAtaSectorShift;
	iNextSector+=tferSectorCount;
	__ASSERT_DEBUG(tferSectorCount<=KMaxSectorsPerRead,Kern::PanicCurrentThread(_L("ReadSectorsCommand"),0));

	if (!iSocket->CardIsReady())
		return(KErrNotReady);
	AtaRegister8(KAtaStatusRd8);	// Clear any pending interrupt
	iSocket->InterruptEnable(EPccdIntIReq,0); // Enable card interrupt
	TInt err=IssueAtaCommand(KAtaCmdReadSectors,aFirstSector,tferSectorCount);
	if (err!=KErrNone)
		{
		iSocket->InterruptDisable(EPccdIntIReq); // Disable card interrupt
		iCardIreqDfc.Cancel();
		return err;
		}
	iCommandError=KErrNone;
	iNotBusyTickCount=KBusyTimeOut;
	iBusyTimeout.OneShot(NKern::TimerTicks(KNotBusyTestInterval));
	return(KErrNone);
	}

TInt DPcCardMediaDriverAta::InitiateWriteCommand(TUint aFirstSector,TInt aSectorCount,TUint8 *aSectBuffer)
//
// Start off an asynchronous write to the card. If successful, it leaves with a 
// timeout(ms timer) queued and card interrupts enabled.
//
	{
	__KTRACE_OPT(KPBUSDRV,Kern::Printf(">Ata:InitWrCmd F:%x C:%x",aFirstSector,aSectorCount));

	__ASSERT_DEBUG((aSectorCount>0 && aSectorCount<=KMaxSectorsPerWrite),Kern::PanicCurrentThread(_L("InitiateWriteCommand"),0));

	TInt err=IssueAtaCommand(KAtaCmdWriteSectors,aFirstSector,aSectorCount);
	if (err!=KErrNone)
		return(err);
	if (!iSocket->CardIsReady())
		return(KErrNotReady);
	if (!WaitForNotBusy())
		return(ProcessError(KErrTimedOut));
	AtaRegister8(KAtaStatusRd8);	// Clear any pending interrupt
	iSocket->InterruptEnable(EPccdIntIReq,0); // Enable card interrupt
	err=EmptySectBufferToDrive(aSectBuffer);
	if (err!=KErrNone)
		{
		iSocket->InterruptDisable(EPccdIntIReq); // Disable card interrupt
		iCardIreqDfc.Cancel();
		return(ProcessError(err));
		}
	iCommandError=KErrNone;
	iNotBusyTickCount=KBusyTimeOut;
	iBusyTimeout.OneShot(NKern::TimerTicks(KNotBusyTestInterval));
	iNextSector+=aSectorCount;
	return(KErrNone);
	}

TInt DPcCardMediaDriverAta::SectorBoundaryReadCheck(TUint aStartSector,TUint aStartSectOffset,Int64 anEndPos)
//
// Check for write request which doesn't lie entirely on a sector boundary and perform
// the appropriate sectors reads prior to writing if necessary
//
	{

	TInt err;
	TUint endSectOffset=((TUint)anEndPos&(~KAtaSectorMask));
	anEndPos-=1;
	anEndPos>>=KAtaSectorShift;
	TUint endSector=(TUint)anEndPos; // Sector number can't exceed 32bits
	TInt sectors=(endSector-aStartSector)+1;
	iLastSectorBufUsed=EFalse;

	if (aStartSectOffset||endSectOffset)
		{
		// If it requires a read of two consecutive sectors then read them in one go
		if (aStartSectOffset && endSectOffset && sectors==2)
			return(SectorRead(aStartSector,&iSectorBuf[0],2,KAtaCmdReadSectors));
		else 
			{
			// If write starts off a sector boundary or is a single sector write ending 
			// off a sector boundary then read first sector
			if (aStartSectOffset || (endSectOffset && sectors==1))
				{
				if ((err=SectorRead(aStartSector,&iSectorBuf[0]))!=KErrNone)
					return(err);
				}
			// If write doesn't end on a sector boundary then read the last sector
			if (endSectOffset && sectors>1)
				{
				TUint8* p=iSectorBuf;
				if (sectors<=KMaxSectorsPerWrite)
					p+=(sectors-1)<<KAtaSectorShift;
				else
					{
					p=iLastSectorBuf;
					iLastSectorBufUsed=ETrue;
					}
				return(SectorRead(endSector,p));
				}
			}
		}
	return(KErrNone);
	}

TInt DPcCardMediaDriverAta::SectorRead(TUint aFirstSector,TUint8 *aBuf,TInt aSectorCount,TUint8 aCmd)
//
// Read either 1 or 2 sectors into the sector buffer (synchronously)
//
	{

	TInt err;
	if ( (err=IssueAtaCommand(aCmd,aFirstSector,aSectorCount))==KErrNone)
		{
		if (
			(err=TransferSectBufferFromDrive(aBuf))!=KErrNone||
			(aSectorCount>1&&(err=TransferSectBufferFromDrive((TAny*)(aBuf+KAtaSectorSize)))!=KErrNone)||
			(err=FinishCommand())!=KErrNone
		   )
			err=ProcessError(err);
		}
	if (!iSocket->CardIsReady())
		err=KErrNotReady; // If media change - return not ready rather than anything else.
	return(err);
	}

void DPcCardMediaDriverAta::DumpIdentifyDriveInfo()
//
// Debug option to display the drive indentification info.
//
	{
#ifdef _DEBUG
	if (KDebugNum(KPBUSDRV))
		{
		for (TInt i=0;i<128;i+=8)
			{
			Kern::Printf("%02x%02x %02x%02x %02x%02x %02x%02x",iSectorBuf[i],iSectorBuf[i+1],iSectorBuf[i+2],iSectorBuf[i+3],iSectorBuf[i+4],\
						iSectorBuf[i+5],iSectorBuf[i+6],iSectorBuf[i+7]);
			}
		}
#endif 
	}

TInt DPcCardMediaDriverAta::IdentifyDrive()
//
// Get drive charateristics
//
	{
	__KTRACE_OPT(KPBUSDRV,Kern::Printf(">Ata:IdentifyDrive"));

	TInt err;
	if ((err=SectorRead(0,&iSectorBuf[0],1,KAtaCmdIdentifyDrive))!=KErrNone)
		return(err);

	DumpIdentifyDriveInfo();
	TBool lba=EFalse;
	Int64 s=0;
	if (*(TUint16*)(&iSectorBuf[KAtaIdCapabilities])&KAtaIdCapLbaSupported)
		{
		// Card indicates it supports LBA
		// Media size (in sectors) - 2 x halfwords @ KAtaIdTotalSectorsInLba.
		iDriveInfo.iTotalSectorsInLba=*(TInt*)(&iSectorBuf[KAtaIdTotalSectorsInLba]);
		s=iDriveInfo.iTotalSectorsInLba;
		s<<=KAtaSectorShift;
		if (s>0)
			lba=ETrue; // Epson PC card reports LBA supported but LBA size in bytes becomes zero
		} 

	iDriveInfo.iSupportsLba = lba;
	if (!lba)
		{
		// LBA not supported
		if (*(TUint16*)(&iSectorBuf[KAtaIdTranslationParams])&KAtaIdTrParamsValid)
			{
			// Current translation parameters are valid
			iDriveInfo.iCylinders=*(TUint16*)(&iSectorBuf[KAtaIdCurrentCylinders]);
			iDriveInfo.iHeads=*(TUint16*)(&iSectorBuf[KAtaIdCurrentHeads]);
			iDriveInfo.iSectorsPerTrack=*(TUint16*)(&iSectorBuf[KAtaIdCurrentSectorsPerTrack]);
			}
		else
			{ 
			// Use defaults
			iDriveInfo.iCylinders=*(TUint16*)(&iSectorBuf[KAtaIdDefaultCylinders]);
			iDriveInfo.iHeads=*(TUint16*)(&iSectorBuf[KAtaIdDefaultHeads]);
			iDriveInfo.iSectorsPerTrack=*(TUint16*)(&iSectorBuf[KAtaIdDefaultSectorsPerTrack]);
			} 
		iDriveInfo.iSectorsPerCylinder=(iDriveInfo.iHeads*iDriveInfo.iSectorsPerTrack);
		s=iDriveInfo.iCylinders;
		s*=iDriveInfo.iSectorsPerCylinder;
		s<<=KAtaSectorShift;
		if (iDriveInfo.iSectorsPerCylinder<=0||iDriveInfo.iSectorsPerTrack<=0)
			return(KErrCorrupt);
		}

	__KTRACE_OPT(KPBUSDRV,Kern::Printf("LBA      : %xH",iDriveInfo.iSupportsLba));
	__KTRACE_OPT(KPBUSDRV,Kern::Printf("Cylinders: %xH",iDriveInfo.iCylinders));
	__KTRACE_OPT(KPBUSDRV,Kern::Printf("Heads    : %xH",iDriveInfo.iHeads));
	__KTRACE_OPT(KPBUSDRV,Kern::Printf("Sectors  : %xH",iDriveInfo.iSectorsPerTrack));
	__KTRACE_OPT(KPBUSDRV,Kern::Printf("<Ata:IdentifyDrive (TS:%lxH)",s));
	SetTotalSizeInBytes(s);
	return(KErrNone);
	}

TInt DPcCardMediaDriverAta::ConfigAutoPowerDown()
//
// Set auto power down period to something sensible
//
	{

	TInt err;
	if ( (err=IssueAtaCommand(KAtaCmdIdle,0,200))==KErrNone) // 200x5mS=1S (aFirstSector doesn't matter).
		{
		if ((err=FinishCommand())!=KErrNone)
			err=ProcessError(err);
		}
	__KTRACE_OPT(KPBUSDRV,Kern::Printf("<Ata:ConfigAutoPowerDown-%d",err));
	return(err);
	}

TInt DPcCardMediaDriverAta::DoOpen()
//
// Open the media driver.
//
	{

	__KTRACE_OPT(KPBUSDRV,Kern::Printf(">Ata:DoOpen"));
	iCardIreqDfc.SetDfcQ(&iSocket->iDfcQ);
	iTimerDfc.SetDfcQ(&iSocket->iDfcQ);

	// Card ought to be ready but check we haven't had media change (this causes creation of card functions).
	TInt err=iSocket->CardIsReadyAndVerified();	// this may also fail with OOM or corrupt
	if (err!=KErrNone)
		return err;

	// Perform CIS validation - get back info. on card functions present. 
	TPccdType pt;
	if (iSocket->VerifyCard(pt)!=KErrNone)
		return(KErrUnknown);

	__KTRACE_OPT(KPBUSDRV,Kern::Printf("Ata:DoOpen-Check for ATA function, FuncCount=%d",pt.iFuncCount));

	// Check for ATA fixed disk function.
	TCisReader cisRd; 
	TInt f;
	TBool isAta=EFalse;
	for (f=0;f<pt.iFuncCount;f++)
		{
		if (pt.iFuncType[f]==EFixedDiskCard)
			{
			// Fixed disk function - check its an ATA. 
			TBuf8<KLargeTplBufSize> tpl;
			cisRd.SelectCis(iSocket->iSocketNumber,f);
			// Must start just after FuncId tuple.
			if (cisRd.FindReadTuple(KCisTplFuncId,tpl)!=KErrNone)
				continue;
			while (cisRd.FindReadTuple(KCisTplFunce,tpl)==KErrNone)
				{
				if (tpl[2]==0x01 && tpl[3]==0x01)	// Interface type - ATA
					isAta=ETrue;
				}
			if (isAta)
				break;
			}
		}
	if (!isAta)
		return(KErrUnknown);
	__KTRACE_OPT(KPBUSDRV,Kern::Printf("Ata:DoOpen-Is ATA"));

	// Determine best card configuration
	cisRd.Restart();
	TPcCardConfig cf;
	TInt opt=KInvalidConfOpt;
	while(cisRd.FindReadConfig(cf)==KErrNone)
		{
		if (
			 cf.IsMachineCompatible(iSocket->iSocketNumber,KPccdCompatNoVccCheck) && // Hitachi card has no 3.3V config entry
#if defined (SELECT_MEMORY_CONFIG)
			 (cf.iValidChunks==1 && __IS_COMMON_MEM(cf.iChnk[0].iMemType) && cf.iChnk[0].iMemLen==0x800)
#elif defined (SELECT_PRIMARY_IO_CONFIG)
			 (cf.iValidChunks==2 && __IS_IO_MEM(cf.iChnk[0].iMemType) && __IS_IO_MEM(cf.iChnk[1].iMemType) && cf.iChnk[0].iMemBaseAddr==0x1F0)
#else
			 // Choose 16 byte -  contiguous i/o option
			 (cf.iValidChunks==1 && __IS_IO_MEM(cf.iChnk[0].iMemType) && cf.iChnk[0].iMemLen==0x10)
#endif
		   ) 
			{
			opt=cf.iConfigOption;
			break;
			}
		}
	if (opt==KInvalidConfOpt)
		return(KErrNotSupported);
	__KTRACE_OPT(KPBUSDRV,Kern::Printf("Ata:DoOpen-ConfigOpt(%d)",opt));
	__KTRACE_OPT(KPBUSDRV,Kern::Printf("Ata:DoOpen-Pulsed Ints-%d",(cf.iInterruptInfo&KPccdIntPulse)));

	// Now configure the card. First configure it to memory mode. 
	cf.iConfigOption=0;
	err=iSocket->RequestConfig(f,this,cf,0);
	if (err!=KErrNone)
		return(err);
	if (cf.iRegPresent&KConfigAndStatusRegM)
		iSocket->WriteConfigReg(f,KConfigAndStatusReg,0); 
	if (cf.iRegPresent&KSocketAndCopyRegM)
		iSocket->WriteConfigReg(f,KSocketAndCopyReg,0);	// No twin cards
	iSocket->ReleaseConfig(f,this);

	cf.iConfigOption=(cf.iInterruptInfo&KPccdIntPulse)?opt:(opt|KConfOptLevIReqM);
//	cf.iConfigOption=(opt|KConfOptLevIReqM); // Force level mode interrupts
#if defined (FORCE_8BIT_ACCESSES)
#if defined (SELECT_MEMORY_CONFIG)
	cf.iChnk[0].iMemType=EPccdCommon8Mem;	// Force it to 8bit Common.
#else
	cf.iChnk[0].iMemType=EPccdIo8Mem;		// Force it to 8bit I/O.
#endif
#endif

	if ((err=iSocket->RequestConfig(f,this,cf,0))!=KErrNone)
		return(err);

	// Read back the config option register to verify it has been setup
	TUint8 v;
	if ((err=iSocket->ReadConfigReg(f,0,v))!=KErrNone||v!=(TUint8)cf.iConfigOption)
		return KErrBadPccdConfig;
	iCardFuncNum=f;
	__KTRACE_OPT(KPBUSDRV,Kern::Printf("Ata:DoOpen-Configured(%1xH) OK",v));

	TUint flag=(cf.iActiveSignals&KSigWaitRequired)?KPccdRequestWait:0;
	err=iDriveChunk.Create(iSocket,cf.iChnk[0],cf.iAccessSpeed,flag);
	if (err!=KErrNone)
		return(err);
	iDriveChunk.SetupChunkHw();
	iMemoryType=cf.iChnk[0].iMemType;
	__KTRACE_OPT(KPBUSDRV,Kern::Printf("Ata:DoOpen-Requested Mem OK"));

	iSocket->InterruptDisable(EPccdIntIReq);
	iCardIntCallBack.iSocket=iSocket;
	iCardIntCallBack.Add();
	SetAtaRegister8(0,KAtaDeviceCtlWr8);	// Enable interrupts

	SetCurrentConsumption(KIdleCurrentInMilliAmps);
	__KTRACE_OPT(KPBUSDRV,Kern::Printf("<Ata:DoOpen-OK"));
	return(KErrNone);
	}

TBool DPcCardMediaDriverAta::CardPoweredDown()
//
// Returns EFalse as long as card hasn't received power down (emergency or normal).
//
	{

	// Emergency power down may result in card being powered down before PC Card
	// Controller status is updated - check for EPD as well as PC Card Contoller.
	return(!Kern::PowerGood() || !iSocket->CardIsReady());
	}

void DPcCardMediaDriverAta::IncrementSectBufPtr()
//
//
//
	{

	iSectBufPtr+=KAtaSectorSize;
	if ((iSectBufPtr-&iSectorBuf[0])>=KSectBufSizeInBytes)
		iSectBufPtr=&iSectorBuf[0];
	}

TInt DPcCardMediaDriverAta::Request(TLocDrvRequest& m)
	{
	__KTRACE_OPT(KLOCDRV,Kern::Printf("Ata:Req %08x id %d",&m,m.Id()));
	TInt r=KErrNotSupported;
	TInt id=m.Id();
	if (id==DLocalDrive::ECaps)
		{
		TLocalDriveCapsV6& c=*(TLocalDriveCapsV6*)m.RemoteDes();
		r=Caps(c);
		c.iSize=m.Drive()->iPartitionLen;
		c.iPartitionType=m.Drive()->iPartitionType;
		return r;
		}
	if (iCurrentReq)
		{
		// a request is already in progress, so hold on to this one
		__KTRACE_OPT(KLOCDRV,Kern::Printf("Ata:Req %08x ret 1",&m));
		return KMediaDriverDeferRequest;
		}
	iCurrentReq=&m;
	switch (id)
		{
		case DLocalDrive::ERead:
			r=DoRead();
			break;
		case DLocalDrive::EWrite:
			r=DoWrite();
			break;
		case DLocalDrive::EFormat:
			r=DoFormat();
			break;
		case DLocalDrive::EEnlarge:
		case DLocalDrive::EReduce:
		default:
			r=KErrNotSupported;
			break;
		}
	__KTRACE_OPT(KLOCDRV,Kern::Printf("Ata:Req %08x cmp %d",&m,r));
	if (r!=KErrNone)
		iCurrentReq=NULL;
	return r;
	}

void DPcCardMediaDriverAta::NotifyPowerDown()
	{
	__KTRACE_OPT(KPBUSDRV,Kern::Printf(">Ata:NotifyPowerDown"));
	Complete(KErrNotReady);
	Reset();
	}

void DPcCardMediaDriverAta::NotifyEmergencyPowerDown()
	{
	__KTRACE_OPT(KPBUSDRV,Kern::Printf(">Ata:NotifyEmergencyPowerDown"));
	TInt r=KErrNotReady;
	if (iCritical)
		r=KErrAbort;
	EndInCritical();
	Complete(r);
	Reset();
	}

void DPcCardMediaDriverAta::TimerDfcFunction(TAny* aPtr)
//
// DFC callback from not-busy timer
//
	{
	DPcCardMediaDriverAta &md=*(DPcCardMediaDriverAta*)aPtr;
	__KTRACE_OPT2(KPBUSDRV,KFAIL,Kern::Printf(">Ata:TimerDfcFunction TC:%d",md.iNotBusyTickCount));
	TInt r=1;
	if (!md.CardPoweredDown())
		{
		md.iDriveChunk.SetupChunkHw();
		TUint8 status=md.AtaRegister8(KAtaStatusRd8); // Clears interrupt
		if (!(status&KAtaStatusBusy))
			{
			// It is not-busy so process it as we would from an interrupt
			__KTRACE_OPT2(KPBUSDRV,KFAIL,Kern::Printf("Card not busy"));
			r=KErrNone;
#ifdef COUNT_TIMEOUTS
			++md.iTimeouts;
			md.iNewTimeOut=ETrue;
			if (md.iIndex<=6)
				{
				md.iInfo[md.iIndex++]=md.iCmdInOffset;
				md.iInfo[md.iIndex++]=md.iCmdEndOffset;
				}
#endif
			}
		else
			{
			// still busy so count tick and restart timer
			if (--md.iNotBusyTickCount==0)
				{
				__KTRACE_OPT2(KPBUSDRV,KFAIL,Kern::Printf("Timed out"));
				r=KErrTimedOut;		// time out the request
				}
			else
				{
				__KTRACE_OPT2(KPBUSDRV,KFAIL,Kern::Printf("Restart timer"));
				md.iBusyTimeout.OneShot(NKern::TimerTicks(KNotBusyTestInterval));
				}
			}
		}
	else
		r=KErrNotReady;
	__KTRACE_OPT2(KPBUSDRV,KFAIL,Kern::Printf("<Ata:TimerDfcFunction r=%d",r));
	if (r<=0)
		{
		// card is not-busy, powered down or timed out
		md.iSocket->InterruptDisable(EPccdIntIReq); // Disable IREQ in controller
		md.iCardIreqDfc.Cancel();	// so we don't run this twice
		md.iCommandError=r;
		CardIreqDfcFunction(&md);
		}
	}

void DPcCardMediaDriverAta::CardIntCallBack(TAny* aMediaDriver, TInt)
//
// Card Interrupt callback
//
	{
	__TRACE_TIMING(0x104);
	DPcCardMediaDriverAta &md=*(DPcCardMediaDriverAta*)aMediaDriver;
	md.iSocket->InterruptDisable(EPccdIntIReq); // Disable IREQ in controller
	md.iCardIreqDfc.Add();
#ifdef COUNT_TIMEOUTS
	++md.iInts;
#endif
	}

void DPcCardMediaDriverAta::CardIreqDfcFunction(TAny* aPtr)
	{

	DPcCardMediaDriverAta &md=*(DPcCardMediaDriverAta*)aPtr;
	TInt err=md.iCommandError;
	TBool queueDfc=md.CardPoweredDown();
	if (queueDfc)
		err=KErrNotReady;
	else if (err!=KErrNone)
		queueDfc=ETrue;		// timed out
	else
		{
		md.iDriveChunk.SetupChunkHw();
		TUint8 status=md.AtaRegister8(KAtaStatusRd8); // Clears interrupt
		if (!(status&KAtaStatusBusy))
			queueDfc=md.DoCardNotBusy(err);
		}
	TBool cmd_done=EFalse;
	if (queueDfc)
		{
		md.iCommandError=err;
		cmd_done=md.CmdDfc();
		}
	if (!queueDfc)
		{
		// command still executing so reenable interrupts
		md.iSocket->InterruptEnable(EPccdIntIReq,0);
		}
	if (!cmd_done)
		{
		// quickly check if card is already not busy
		// this handles cards which return data very quickly
		md.iDriveChunk.SetupChunkHw();
		TUint8 status=md.AtaRegister8(KAtaStatusRd8); // Clears interrupt
		if (!(status&KAtaStatusBusy))
			{
			md.iSocket->InterruptDisable(EPccdIntIReq);
			md.iCardIreqDfc.Enque();
#ifdef COUNT_TIMEOUTS
			++md.iImmediateNotBusy;
#endif
			}
		else
			{
			md.iBusyTimeout.Cancel();
			md.iTimerDfc.Cancel();
			md.iBusyTimeout.OneShot(NKern::TimerTicks(KNotBusyTestInterval));
			}
		}
	}

TBool DPcCardMediaDriverAta::DoCardNotBusy(TInt &anErr)
//
// Card not busy interrupt - return ETrue when need to queue dfc
//
	{

	anErr=KErrNone;
	if (iCardStatus==ECardWrite || iCardStatus==ECardFormat)
		{
		// For write/format commands we only update the out-pointer when the card becomes
		// not-busy following the point we transfered the block of data to the card.
		iCmdOutOffset+=(KAtaSectorSize-iSectBufOffset);
		iSectBufOffset=0;
		if (iCmdOutOffset<iCmdEndOffset)
			{
			TUint8* pB=iSectBufPtr;
			if (iLastSectorBufUsed && iCmdLength-iCmdOutOffset<=(TUint)KAtaSectorSize)
				pB=iLastSectorBuf;	// use different buffer for last sector of unaligned write
			anErr=EmptySectBufferToDrive(pB);
			if (anErr!=KErrNone)
				return(ETrue);
			if (iCardStatus==ECardWrite)
				iSectBufPtr+=KAtaSectorSize;
			}
		else
			{
			anErr=CheckForError();
			return(ETrue);
			}
		}
	if (iCardStatus==ECardRead)
		{
		// For read commands we only update the in-pointer here. The out-pointer is
		// updated by the DFC as data is written to target thread

		// Before we read the data from the card, perform the in-pointer incremeting now. 
		// If we are about to queue a DFC then let the DFC read from the card. Otherwise,
		// reading from the card may result in the next sector of data being available
		// before we're ready for it.

		// If first interrupt after command initiated then allow for sector buffer offset
		TInt toEOBuf=(iCmdInOffset==0)?(KAtaSectorSize-iSectBufOffset):KAtaSectorSize;
		TInt remaining=(TInt)(iCmdEndOffset-iCmdInOffset);
		iCmdInOffset+=Min(remaining,toEOBuf);
		if ((iSectBufPtr-&iSectorBuf[0])>=KSectBufSizeInBytesMinusOneSector || iCmdInOffset>=iCmdEndOffset)
			return(ETrue); // Queue a DFC
		else
			{
			anErr=LoadSectBufferFromDrive(iSectBufPtr);
			if (anErr!=KErrNone)
				return(ETrue);
			IncrementSectBufPtr();
			}
		}
	return(EFalse);
	}

void DPcCardMediaDriverAta::SyncBusyTimerCallBack(TAny* aBusyFlag)
//
// Wait for not busy timeout callback
//
	{

	*(TBool*)aBusyFlag=ETrue;
	}

void DPcCardMediaDriverAta::AsyncBusyTimerCallBack(TAny* aMediaDriver)
//
// Wait for not busy timeout callback
//
	{

	__KTRACE_OPT(KFAIL,Kern::Printf("!,"));
	DPcCardMediaDriverAta &md=*(DPcCardMediaDriverAta*)aMediaDriver;
	md.iTimerDfc.Add();
	}

TBool DPcCardMediaDriverAta::CmdDfc()
//
// Dfc to complete an asynchronous command
//
	{

	TInt err;

	__KTRACE_OPT(KPBUSDRV,Kern::Printf(">Ata:CmdDfc status %d in %x out %x len %x",iCardStatus,iCmdInOffset,iCmdOutOffset,iCmdLength));

	iBusyTimeout.Cancel();
	iTimerDfc.Cancel();
	if (DoCmdDfc(err))	// this requeues timer and reenables interrupts if command still executing
		{
		// Command has been completed for one reason or other
		EndInCritical();
		if (err!=KErrNone)
			err=ProcessError(err);

		// If emergency power down on write/format - return abort rather than anything else.
		if (!Kern::PowerGood() && iCardStatus!=ECardRead)
			err=KErrAbort;
		// If media change - return not ready rather than anything else.
		else if (!iSocket->CardIsReady())
			err=KErrNotReady;
		__KTRACE_OPT(KFAIL,Kern::Printf("a%d",err));
		if (err==KErrNone)
			{
			// may need to issue another command here
			if (iCmdOutOffset<iCmdLength)
				{
				if (iCardStatus==ECardRead)
					{
					__KTRACE_OPT(KFAIL,Kern::Printf("+R"));
					__KTRACE_OPT(KPBUSDRV,Kern::Printf(">Ata:NewReadCmd"));
					iSectBufOffset=0;
#ifdef COUNT_TIMEOUTS
					++iChainedReads;
#endif
					err=InitiateAsyncRead();
					if (err==KErrNone)
						return ETrue;
					}
				else
					{
					__KTRACE_OPT(KFAIL,Kern::Printf("+W"));
					__KTRACE_OPT(KPBUSDRV,Kern::Printf(">Ata:NewWriteCmd"));
#ifdef COUNT_TIMEOUTS
					++iChainedWrites;
#endif
					err=InitiateAsyncWrite();
					if (err==KErrNone)
						return ETrue;
					}
				}
			}
		iCardStatus=ECardIdle;		// Command now complete
		SetCurrentConsumption(KIdleCurrentInMilliAmps);

#if (defined(_DEBUG) || defined(SHOW_CARD_ERRORS))
		if (err!=KErrNone&&err!=KErrTooBig)
			{
			__KTRACE_CARD_ERROR(KPBUSDRV,Kern::Printf("<DFC(L:%d P:%xH)-%d",iDbgLen,iDbgPos,err));
			iDbgLastError=err;
			}
#endif
		__TRACE_TIMING(0x109);
		Complete(err);
		return ETrue;
		}
	return EFalse;
	}

TBool DPcCardMediaDriverAta::DoCmdDfc(TInt &anErr)
//
// Return ETrue when complete
//
	{

	if (CardPoweredDown())
		{
		anErr=KErrNotReady;
		return(ETrue);
		}
	anErr=iCommandError;
	if (iCommandError!=KErrNone)
		return(ETrue);

	if (iCardStatus==ECardWrite||iCardStatus==ECardFormat)
		return(ETrue);
	else
		{
		// Read command, we postponed loading the last sector
		__TRACE_TIMING(0x105);
		if ((anErr=LoadSectBufferFromDrive(iSectBufPtr))!=KErrNone)
			return(ETrue);

		// In case command still executing, queue another timeout and re-enable interrupts
		iSocket->InterruptEnable(EPccdIntIReq,0);
		iNotBusyTickCount=KBusyTimeOut;
		iBusyTimeout.OneShot(NKern::TimerTicks(KNotBusyTestInterval));

		__TRACE_TIMING(0x106);
		IncrementSectBufPtr();
		if ((anErr=EmptySectBufferToTrg(&iSectorBuf[0]+iSectBufOffset,iCmdOutOffset,iCmdInOffset-iCmdOutOffset))!=KErrNone)
			{
			iSocket->InterruptDisable(EPccdIntIReq); // Disable IREQ in controller
			iBusyTimeout.Cancel();
			iTimerDfc.Cancel();
			iCardIreqDfc.Cancel();
			return(ETrue);
			}
		__TRACE_TIMING(0x107);
		iSectBufOffset=0;		// From now on we always start on sector boundary
		iCmdOutOffset=iCmdInOffset;
		if (iCmdOutOffset<iCmdEndOffset)
			return(EFalse);
		iSocket->InterruptDisable(EPccdIntIReq); // Disable IREQ in controller
		iBusyTimeout.Cancel();
		iTimerDfc.Cancel();
		iCardIreqDfc.Cancel();
		anErr=FinishCommand();	// This functions involves polling for not busy
		return(ETrue);
		}
	__TRACE_TIMING(0x108);
	}

DPhysicalDeviceMediaAta::DPhysicalDeviceMediaAta()
//
// Constructor
//
	{
	iUnitsMask=0x1;
	iVersion=TVersion(KMediaDriverInterfaceMajorVersion,KMediaDriverInterfaceMinorVersion,KMediaDriverInterfaceBuildVersion);
	}

TInt DPhysicalDeviceMediaAta::Install()
//
// Install the PC Card ATA Media PDD.
//
	{
	return SetName(&KPddName);
	}

void DPhysicalDeviceMediaAta::GetCaps(TDes8 &/* aDes */) const
//
// Return the media drivers capabilities.
//
	{
	}
								 
TInt DPhysicalDeviceMediaAta::Create(DBase*& aChannel, TInt aMediaId, const TDesC8* /*anInfo*/, const TVersion& aVer)
//
// Create a PC Card ATA media driver.
//
	{
	if (!Kern::QueryVersionSupported(iVersion,aVer))
		return KErrNotSupported;
	DPcCardMediaDriverAta* pD=new DPcCardMediaDriverAta(aMediaId);
	aChannel=pD;
	TInt r=KErrNoMemory;
	if (pD)
		r=pD->Open();
	if (r==KErrNone)
		pD->OpenMediaDriverComplete(KErrNone);
	return r;
	}

TInt DPhysicalDeviceMediaAta::Validate(TInt aDeviceType, const TDesC8* anInfo, const TVersion& aVer)
	{
	if (!Kern::QueryVersionSupported(iVersion,aVer))
		return KErrNotSupported;
	if (aDeviceType!=MEDIA_DEVICE_PCCARD)
		return KErrNotSupported;
	return KErrNone;
	}

TInt DPhysicalDeviceMediaAta::Info(TInt aFunction, TAny*)
//
// Return the priority of this media driver
//
	{
	if (aFunction==EPriority)
		return KAtaDriverPriority;
	return KErrNotSupported;
	}

DPcCardMediaDriverAta::DPcCardMediaDriverAta(TInt aMediaId)
//
// Constructor.
//
	:	DMediaDriver(aMediaId),
		iCardIntCallBack(CardIntCallBack,this,KPccdIntMaskIReq),
		iBusyTimeout(AsyncBusyTimerCallBack,this),
		iCardIreqDfc(CardIreqDfcFunction,this,2),
		iTimerDfc(TimerDfcFunction,this,2)
	{

	iMemoryType=EPccdIo8Mem;
//	iCommandError=KErrNone;
//	TUint iCmdInOffset=0;
//	TUint iCmdOutOffset=0;
//	iCmdEndOffset=0;
//	iSectBufPtr=NULL;
//	iSectBufOffset=0;
	iCardStatus=ECardIdle;
//	iHiddenSectors=0;
	iCardFuncNum=-1;
	}

TInt DPcCardMediaDriverAta::Open()
//
// Open the media driver.
//
	{

	// Open the driver and get drive characteristics
	iSocket=((DPcCardSocket*)((DPBusPrimaryMedia*)iPrimaryMedia)->iSocket);
	__KTRACE_OPT(KPBUSDRV,Kern::Printf(">Ata:Open on socket %d",iSocket->iSocketNumber));
	TInt r=DoOpen();
	if (r==KErrNone)
		r=IdentifyDrive();
	if (r!=KErrNone)
		{
		DoClose();
		if (!iSocket->CardIsReady())
			r=KErrNotReady; // If media change - return not ready rather than anything else.
		}
	__KTRACE_OPT(KPBUSDRV,Kern::Printf("<Ata:Open-%d",r));
	return r;
	}

TInt DPcCardMediaDriverAta::CheckDevice(TBool aCheckPower)
//
// Check the device before initiating a command
//
	{
	
	if (iSocket->CardIsReadyAndVerified()!=KErrNone)
		return KErrNotReady;
	if (aCheckPower && Kern::MachinePowerStatus()<ELow)
		return KErrBadPower;
	return KErrNone;
	}

TInt DPcCardMediaDriverAta::DoRead()
//
// Read from specified area of media.
//
	{

	Int64 aPos=iCurrentReq->Pos();
	TInt aLength=(TInt)iCurrentReq->Length();
	TInt err;
	iCmdOutOffset=0;	// Progress monitor - data delivered to target
#if (defined(_DEBUG) || defined(SHOW_CARD_ERRORS))
	iDbgPos=(TUint)aPos;
	iDbgLen=aLength;
#endif
	err=CheckDevice(EFalse);
	if (err==KErrNone)
		{
		iDriveChunk.SetupChunkHw(); // Enable our h/w chunk
	
		TUint sectorBufOffset=(TUint)aPos&(~KAtaSectorMask);
		Int64 firstSector=(aPos>>KAtaSectorShift);
		iCmdLength=aLength;

		SetCurrentConsumption(KReadCurrentInMilliAmps);

		// Start an asynchronous read
		iCmdInOffset=0; 					// Data received from card
		iNextSector=(TUint)firstSector;
		iSectBufOffset=sectorBufOffset;
		err=InitiateAsyncRead();
		if (err==KErrNone)
			return KErrNone;

		iCardStatus=ECardIdle;
		SetCurrentConsumption(KIdleCurrentInMilliAmps);
		if (!iSocket->CardIsReady())
			err=KErrNotReady; // If media change - return not ready rather than anything else.
		}

#if (defined(_DEBUG) || defined(SHOW_CARD_ERRORS))
	__KTRACE_CARD_ERROR(KPBUSDRV,Kern::Printf("<Ata:Read(L:%d P:%xH)-%d",iDbgLen,iDbgPos,err));
	iDbgLastError=err;
#endif
	return err;
	}

TInt DPcCardMediaDriverAta::InitiateAsyncRead()
	{
	// Start an asynchronous read
	TInt cmdLen=Min(TInt(iCmdLength-iCmdInOffset),TInt(KMaxBytesPerRead-iSectBufOffset)); // Leave it on sector boundary if another read required
	iCmdEndOffset=iCmdInOffset+cmdLen;	// Marks point when transfer is complete
	iSectBufPtr=&iSectorBuf[0];
	iCardStatus=ECardRead;
	__TRACE_TIMING(0x102);
	return ReadSectorsCommand(iNextSector,iSectBufOffset,cmdLen);	// Sector number can't exceed 32bits
	}

TInt DPcCardMediaDriverAta::DoWrite()
//
// Write to specified area of media.
//
	{

	Int64 aPos=iCurrentReq->Pos();
	TInt aLength=(TInt)iCurrentReq->Length();
	TInt err;
#if (defined(_DEBUG) || defined(SHOW_CARD_ERRORS))
	iDbgPos=(TUint)aPos;
	iDbgLen=aLength;
#endif
	err=CheckDevice(ETrue);
	if (err==KErrNone)
		{
		iDriveChunk.SetupChunkHw(); // Enable our h/w chunk
		TUint sectorBufOffset=(TUint)aPos&(~KAtaSectorMask);
		Int64 firstSector=(aPos>>KAtaSectorShift);
		iCmdLength=aLength;

		// for unaligned writes, first need to read the first and/or last sector
		err=SectorBoundaryReadCheck((TUint)firstSector,sectorBufOffset,aPos+iCmdLength);
		if (err==KErrNone) // Sector number can't exceed 32bits
			{
			// Time to actually start the write. First alter the current consumption 
			// and save the data required to complete the write (in the ISR)
			SetCurrentConsumption(KWriteCurrentInMilliAmps);
			iSectBufOffset=sectorBufOffset;
			iCmdInOffset=0;
			iCmdOutOffset=0;		// Progress monitor - data delivered to card
			iNextSector=(TUint)firstSector;
			iCardStatus=ECardWrite;
			err=InitiateAsyncWrite();
			if (err==KErrNone)
				return KErrNone;
			SetCurrentConsumption(KIdleCurrentInMilliAmps);
			}
		iCardStatus=ECardIdle;
		if (!Kern::PowerGood())
			err=KErrAbort; // If emergency power down - return abort rather than anything else.
		else if (!iSocket->CardIsReady())
			err=KErrNotReady; // If media change - return not ready rather than anything else.
		}

#if (defined(_DEBUG) || defined(SHOW_CARD_ERRORS))
	__KTRACE_CARD_ERROR(KPBUSDRV,Kern::Printf("<Ata:Write(L:%d P:%xH)-%d",iDbgLen,iDbgPos,err));
	iDbgLastError=err;
#endif
	return err;
	}

TInt DPcCardMediaDriverAta::InitiateAsyncWrite()
	{
	iCmdOutOffset=iCmdInOffset;
	TInt remain=iCmdLength-iCmdInOffset;
	TInt cmdLen=Min(remain, KMaxBytesPerWrite-iSectBufOffset);
	TInt sectCount=(cmdLen+iSectBufOffset+KAtaSectorSize-1)>>KAtaSectorShift;
	iCmdEndOffset=iCmdOutOffset+cmdLen;
	iSectBufPtr=iSectorBuf;
	TUint8* pB=iSectorBuf;
	TInt r=KErrNone;
	if (iCardStatus==ECardWrite)
		{
		if (iLastSectorBufUsed && cmdLen==remain)
			{
			// load data for sectors other than last into sector buffer
			TInt lengthExcludingLastSector=cmdLen &~ (KAtaSectorSize-1);
			if (lengthExcludingLastSector==cmdLen)
				lengthExcludingLastSector-=KAtaSectorSize;
			if (lengthExcludingLastSector)
				{
				r=LoadSectBufferFromSrc(lengthExcludingLastSector,iSectorBuf+iSectBufOffset);
				if (r!=KErrNone)
					return r;
				iCmdInOffset+=lengthExcludingLastSector;	// make sure we get right data for last sector
				}
			else
				pB=iLastSectorBuf;
			// load last sector data into last sector buffer
			r=LoadSectBufferFromSrc(cmdLen-lengthExcludingLastSector,iLastSectorBuf);
			if (r!=KErrNone)
				return r;
			iCmdInOffset+=(cmdLen-lengthExcludingLastSector);
			}
		else
			{
			// Load the the data from source in one go
			r=LoadSectBufferFromSrc(cmdLen,iSectorBuf+iSectBufOffset);
			if (r!=KErrNone)
				return r;
			iCmdInOffset+=cmdLen;
			}
		}
	else
		iCmdInOffset+=cmdLen;	// format command
	r=InCritical();		// this returns KErrNotReady if we are about to do postponed media change or power down
	if (r==KErrNone)
		{
		r=InitiateWriteCommand(iNextSector,sectCount,pB);
		__KTRACE_OPT(KPBUSDRV,Kern::Printf("InitWrCmd ret %d",r));
		if (iCardStatus==ECardWrite)
			iSectBufPtr+=KAtaSectorSize;
		}
	if (r!=KErrNone)
		{
		if (!Kern::PowerGood())
			r=KErrAbort; // If emergency power down - return abort rather than anything else.
		else if (!iSocket->CardIsReady())
			r=KErrNotReady; // If media change - return not ready rather than anything else.
		EndInCritical();
		}
	return r;
	}

TInt DPcCardMediaDriverAta::DoFormat()
//
// Format the specified area of the media. 
//
	{
	
	Int64 aPos=iCurrentReq->Pos();
	TInt aLength=(TInt)iCurrentReq->Length();
	TInt err;
#if (defined(_DEBUG) || defined(SHOW_CARD_ERRORS))
	iDbgPos=(TUint)aPos;
	iDbgLen=aLength;
#endif
	err=CheckDevice(ETrue);
	if (err==KErrNone)
		{
		iDriveChunk.SetupChunkHw(); // Enable our h/w chunk
		memset(iSectorBuf,0xff,KAtaSectorSize);
		Int64 firstSector=(aPos>>KAtaSectorShift);
		TInt sectCount=(aLength+KAtaSectorSizeMinusOne)>>KAtaSectorShift;
		iCmdLength=(sectCount<<KAtaSectorShift);
		sectCount=Min(KMaxSectorsPerFormat,(aLength+KAtaSectorSizeMinusOne)>>KAtaSectorShift);
		SetCurrentConsumption(KWriteCurrentInMilliAmps);

		iLastSectorBufUsed=EFalse;
		iCmdInOffset=0;
		iCmdOutOffset=0;		// Progress monitor - data delivered to card
		iSectBufOffset=0;
		iSectBufPtr=&iSectorBuf[0];
		iNextSector=(TUint)firstSector;
		iCardStatus=ECardFormat;
		err=InitiateAsyncWrite();
		if (err==KErrNone)
			return KErrNone;

		iCardStatus=ECardIdle;
		SetCurrentConsumption(KIdleCurrentInMilliAmps);

		if (!Kern::PowerGood())
			err=KErrAbort; // If emergency power down - return abort rather than anything else.
		else if (!iSocket->CardIsReady())
			err=KErrNotReady; // If media change - return not ready rather than anything else.
		}

#if (defined(_DEBUG) || defined(SHOW_CARD_ERRORS))
	__KTRACE_CARD_ERROR(KPBUSDRV,Kern::Printf("<Ata:Format(L:%d P:%xH)-%d",iDbgLen,iDbgPos,err));
	iDbgLastError=err;
#endif
	return err;
	}

void DPcCardMediaDriverAta::Close()
//
// Close the media driver - also called on media change
//
	{

	__KTRACE_OPT(KPBUSDRV,Kern::Printf(">Ata:Close"));
	EndInCritical();
	Complete(KErrNotReady);
	DoClose();
	DMediaDriver::Close();
	}

void DPcCardMediaDriverAta::DoClose()
//
// Close the media driver
//
	{

	__KTRACE_OPT(KPBUSDRV,Kern::Printf(">Ata:DoClose"));
	iCardIntCallBack.Remove();
	iDriveChunk.Close();
	if (iCardFuncNum>=0)
		{
		iSocket->ReleaseConfig(iCardFuncNum,this);
		iCardFuncNum=-1;
		}
	Reset();
	__KTRACE_CARD_ERROR(KPBUSDRV,Kern::Printf("<Ata:DoClose(%d)",iDbgLastError));
	}

void DPcCardMediaDriverAta::Reset()
	{
	__KTRACE_OPT(KPBUSDRV,Kern::Printf(">Ata:Reset"));
	iBusyTimeout.Cancel();			// In case its currently queued
	iCardIreqDfc.Cancel();			// In case its currently queued
	iTimerDfc.Cancel();
	iCardStatus=ECardIdle;
	SetCurrentConsumption(0);
	}

#ifdef KLOCDRV
void DebugDump(const TMBRPartitionEntry& a)
	{
	Kern::Printf("BootInd =%02x StartHead=%02x StartSect=%02x StartCyl=%02x",
			a.iX86BootIndicator, a.iStartHead, a.iStartSector, a.iStartCylinder);
	Kern::Printf("PartType=%02x EndHead  =%02x EndSect  =%02x EndCyl  =%02x",
			a.iPartitionType, a.iEndHead, a.iEndSector, a.iEndCylinder);
	Kern::Printf("FirstSect=%08x NumSectors=%08x", a.iFirstSector, a.iNumSectors);
	}

void DebugDump(const TPartitionInfo& a)
	{
	Kern::Printf("PartitionInfo: (C:%d)",a.iPartitionCount);
	TInt i;
	for (i=0; i<KMaxPartitionEntries; ++i)
		{
		const TPartitionEntry& e=a.iEntry[i];
		Kern::Printf("   Partition %d: B=%lxH L=%lxH I=%04x T=%04x", i, e.iPartitionBaseAddr,
						e.iPartitionLen, e.iBootIndicator, e.iPartitionType );
		}
	}
#endif

void SetPartitionEntry(TPartitionEntry* aDest, const TMBRPartitionEntry* aSrc)
//
// Set the partition entry details
//
	{

	aDest->iPartitionBaseAddr=aSrc->iFirstSector;
	aDest->iPartitionBaseAddr<<=KAtaSectorShift;
	aDest->iPartitionLen=aSrc->iNumSectors;
	aDest->iPartitionLen<<=KAtaSectorShift;
	aDest->iBootIndicator=aSrc->iX86BootIndicator;
	aDest->iPartitionType=aSrc->iPartitionType;
	}

TInt DPcCardMediaDriverAta::PartitionInfo(TPartitionInfo& anInfo)
//
// Return partition information on the media.
//
	{

	__KTRACE_OPT(KPBUSDRV,Kern::Printf(">Ata:PartitionInfo"));
	if (iSocket->CardIsReadyAndVerified()!=KErrNone)
		return(KErrNotReady);
	TInt partitionCount=anInfo.iPartitionCount=0;

	// Read the first sector and check for a Master Boot Record
	TInt err;
	if ((err=SectorRead(0,&iSectorBuf[0]))!=KErrNone)
		return(err);
	if (*(TUint16*)(&iSectorBuf[KMBRSignatureOffset])!=KMBRSignature)
		return(KErrCorrupt);

	// Move the partition entries to a 4 byte boundary
	memmove(&iSectorBuf[0],&iSectorBuf[KMBRFirstPartitionOffset],(sizeof(TMBRPartitionEntry)*KMBRMaxPrimaryPartitions));

	// Search for a x86 default boot partition - let this be the first
	TMBRPartitionEntry *pe=(TMBRPartitionEntry*)(&iSectorBuf[0]);
	TInt i;
	TInt defaultPartitionNumber=-1;
	for (i=0;i<KMBRMaxPrimaryPartitions;i++,pe++)
		{
		if (pe->IsDefaultBootPartition())
			{
			SetPartitionEntry(anInfo.iEntry, pe);
			defaultPartitionNumber=i;
			iHiddenSectors=pe->iFirstSector;
			partitionCount++;
			break;
			}
		}

	// Now add any other partitions
	pe=(TMBRPartitionEntry*)(&iSectorBuf[0]);	// Reset it
	for (i=0;i<KMBRMaxPrimaryPartitions;i++,pe++)
		{
		__KTRACE_OPT(KLOCDRV, Kern::Printf("Partition %d:",i));
		__KTRACE_OPT(KLOCDRV, DebugDump(*pe));
		if (defaultPartitionNumber==i)
			continue;	// Already sorted
		if (pe->IsValidDosPartition() || pe->IsValidFAT32Partition())
			{
			SetPartitionEntry(anInfo.iEntry+partitionCount, pe);
			partitionCount++;
			}
		}
	anInfo.iPartitionCount=partitionCount;
	anInfo.iMediaSizeInBytes=TotalSizeInBytes();

	__KTRACE_OPT(KLOCDRV, DebugDump(anInfo));

	PartitionInfoComplete(KErrNone);
	return KErrNone;
	}

TInt DPcCardMediaDriverAta::Caps(TLocalDriveCapsV6& aInfo)
//
// Return the capabilities of the media
//
	{
	aInfo.iType=EMediaHardDisk;
	aInfo.iConnectionBusType=EConnectionBusInternal;
	aInfo.iDriveAtt=KDriveAttLocal|KDriveAttRemovable;
	aInfo.iMediaAtt=KMediaAttFormattable;
	aInfo.iFileSystemId=KDriveFileSysFAT;
	aInfo.iHiddenSectors=iHiddenSectors;
	aInfo.iBlockSize=KAtaSectorSize;
	SetTotalSizeInBytes(aInfo);
	return KErrCompletion;	// synchronous completion
	}

DECLARE_STANDARD_PDD()
	{
	return new DPhysicalDeviceMediaAta;
	}

