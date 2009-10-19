// Copyright (c) 2004-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// template\Template_Variant\Specific\lffsdev.cpp
// Implementation of a Logging Flash file system (LFFS) physical device driver 
// for a standard Common Flash Interface (CFI) based NOR flash chip.
// This file is part of the Template Base port
// N.B. This sample code assumes that:
// (1)	the device does not provide an interrupt i.e. it needs to be polled using a timer 
// to ascertain when an Erase/Write operation has completed.
// (2) the flash chip does not have 'read-while-write' support.
// 
//

#include "lffsdev.h"
#include "variant.h"

#ifdef _DEBUG
#define CHANGE_ERASE_STATE(x)	{TUint32 s=iEraseState; iEraseState=x; __KTRACE_OPT(KLOCDRV,Kern::Printf("ErSt: %d->%d",s,x));}
#else
#define CHANGE_ERASE_STATE(x)	iEraseState=x
#endif

//
// TO DO: (mandatory)
//
// Define the pyhsical base address of the NOR-Flash
// This is only example code... you will need to modify it for your hardware
const TPhysAddr KFlashPhysicalBaseAddress = 0x04000000;


/********************************************
 * Common Flash Interface (CFI) query stuff
 ********************************************/

/**
Read an 8-bit value from the device at the specified offset

@param	aOffset	the address in device words 
*/
TUint32 DMediaDriverFlashTemplate::ReadQueryData8(TUint32 aOffset)
	{
	volatile TUint8* pF=(volatile TUint8*)(iBase+FLASH_ADDRESS_IN_BYTES(aOffset));
	return pF[0];
	}

/**
Read a 16-bit value from the device at the specified offset

@param	aOffset	the address in device words 
*/
TUint32 DMediaDriverFlashTemplate::ReadQueryData16(TUint32 aOffset)
	{
	volatile TUint8* pF=(volatile TUint8*)(iBase);
	return 
		 pF[FLASH_ADDRESS_IN_BYTES(aOffset+0)] | 
		(pF[FLASH_ADDRESS_IN_BYTES(aOffset+1)] << 8);
	}

/**
 Put the device into query mode to read the flash parameters.
 */
void DMediaDriverFlashTemplate::ReadFlashParameters()
	{
	volatile TFLASHWORD* pF=(volatile TFLASHWORD*)iBase + KCmdReadQueryOffset;
	*pF=KCmdReadQuery;

	TUint32 qd=ReadQueryData16(KQueryOffsetQRY)|(ReadQueryData8(KQueryOffsetQRY+2)<<16);
	__KTRACE_OPT(KLOCDRV,Kern::Printf("Flash:Query QRY=%08x",qd));
	__ASSERT_ALWAYS(qd==0x595251,FLASH_FAULT());

	qd = FLASH_BUS_DEVICES << ReadQueryData8(KQueryOffsetSizePower);

	__KTRACE_OPT(KLOCDRV,Kern::Printf("Flash:Query Size=%08x",qd));
	iTotalSize=qd;

	qd = FLASH_BUS_DEVICES << ReadQueryData16(KQueryOffsetWriteBufferSizePower);
	__KTRACE_OPT(KLOCDRV,Kern::Printf("Flash:Query WBSize=%08x",qd));
	iWriteBufferSize=qd;

	qd = (ReadQueryData16(KQueryOffsetEraseBlockSize)) << (8 + FLASH_BUS_DEVICES-1);
	__KTRACE_OPT(KLOCDRV,Kern::Printf("Flash:Query EBSize=%08x",qd));
	iEraseBlockSize=qd;

	*pF=KCmdReadArray;
	}


/********************************************
 * Common Flash Interface (CFI) main code
 ********************************************/

/**
NOR flash LFFS constructor.

@param aMediaId            Media id number from ELOCD
*/
DMediaDriverFlashTemplate::DMediaDriverFlashTemplate(TInt aMediaId)
	:	DMediaDriverFlash(aMediaId),
		iHoldOffTimer(HoldOffTimerFn,this),
		iEventDfc(EventDfc,this,NULL,2)
	{
	// iWriteState = EWriteIdle;
	// iEraseState = EEraseIdle;
	}

/**
Device specific implementation of the NOR LFFS initialisation routine.

@see DMediaDriverFlash::Initialise
@return KErrNone unless the write data buffer couldn't be allocated or the
         timer interrupt could not be bound.
 */
TInt DMediaDriverFlashTemplate::Initialise()
	{
	iEventDfc.SetDfcQ(iPrimaryMedia->iDfcQ);
	iData=(TUint8*)Kern::Alloc(KDataBufSize);
	if (!iData)
		return KErrNoMemory;

	// Create temporary HW chunk to read FLASH device parameters (especially size)
	DPlatChunkHw* pC = NULL;
	TInt r = DPlatChunkHw::New(pC, KFlashPhysicalBaseAddress, 0x1000, EMapAttrSupRw|EMapAttrFullyBlocking);
	if (r!=KErrNone)
		return r;
	iBase = pC->LinearAddress();
	ReadFlashParameters();
	// close temporary chunk and open chunk with correct size
	pC->Close(NULL);
	r = DPlatChunkHw::New(iFlashChunk, KFlashPhysicalBaseAddress, iTotalSize, EMapAttrSupRw|EMapAttrFullyBlocking);
	if (r!=KErrNone)
		return r;
	iBase = iFlashChunk->LinearAddress();

	r=Interrupt::Bind(KIntIdTimer1, Isr, this);
	if (r!=KErrNone)
		{
		__KTRACE_OPT(KLOCDRV, Kern::Printf("Flash:Isr Bind failed"));
		return r;
		}

	// TO DO: (mandatory)
	// Write to the appropriate hardware register(s) to
	// configure (if necessary) and enable the timer hardware
	//

	// Enable the timer interrupt
	Interrupt::Enable(KIntIdTimer1);
	
	return KErrNone;
	}

/**
Used by the generic flash media driver code to get the erase block size in
bytes. 
 */
TUint32 DMediaDriverFlashTemplate::EraseBlockSize()
	{
	return iEraseBlockSize;
	}

/**
@return Return size of lffs in bytes
*/
TUint32 DMediaDriverFlashTemplate::TotalSize()
	{
	return iTotalSize;
	}

/**
Read at the location indicated by DMediaDriverFlash::iReadReq. 
Where Pos() is the read location

@return >0			Defer request to ELOCD. A write is in progress
@return KErrNone	Erase has been started
@return <0			An error has occured.
*/
TInt DMediaDriverFlashTemplate::DoRead()
	{
	if (iWriteReq)
		return KMediaDriverDeferRequest;	// write in progress so defer read
	__KTRACE_OPT(KLOCDRV,Kern::Printf("Flash:DoRead"));
	if (iEraseState==EEraseIdle || iEraseState==ESuspended)
		{
		// can do the read now
		TInt pos=(TInt)iReadReq->Pos();
		TInt len=(TInt)iReadReq->Length();

		__KTRACE_OPT(KLOCDRV,Kern::Printf("Flash:DoRead ibase: %x, pos: %x, len: %x",iBase,pos,len));

		// Issue a read array command
		// Porting note: Some devices may work without this step.
		// Ensure that the write is always dword aligned
		volatile TFLASHWORD* pF=(volatile TFLASHWORD*)((iBase+pos)&0xFFFFFFF0);
		*pF=KCmdReadArray;

		TPtrC8 des((const TUint8*)(iBase+pos),len);
		TInt r=iReadReq->WriteRemote(&des,0);
		Complete(EReqRead,r);

		// resume erase if necessary
		if (iEraseState==ESuspended)
			StartErase();
		}
	else if (iEraseState==EErase)
		{
		// erase in progress - suspend it
		SuspendErase();
		}
	else if (iEraseState==EEraseNoSuspend)
		CHANGE_ERASE_STATE(ESuspendPending);	// wait for suspend to complete
	
	
	return KErrNone;
	}

/**
Write at the location indicated by DMediaDriverFlash::iWriteReq

@return >0			Defer request to ELOCD. A read is in progress
@return KErrNone	Erase has been started
@return <0			An error has occured.
 */
TInt DMediaDriverFlashTemplate::DoWrite()
	{
	if (iReadReq)
		return KMediaDriverDeferRequest;	// read in progress so defer write

	TInt pos=(TInt)iWriteReq->Pos();
	TInt len=(TInt)iWriteReq->Length();
	if (len==0)
		return KErrCompletion;
	TUint32 wb_mask=iWriteBufferSize-1;
	iWritePos=pos & ~wb_mask;	// round media position down to write buffer boundary
	TInt wb_off=pos & wb_mask;	// how many bytes of padding at beginning
	TInt start_len=Min(len,KDataBufSize-(TInt)wb_off);
	TInt write_len=(start_len+wb_off+wb_mask)&~wb_mask;
	memset(iData,0xff,iWriteBufferSize);
	memset(iData+write_len-iWriteBufferSize,0xff,iWriteBufferSize);
	TPtr8 des(iData+wb_off,0,start_len);
	TInt r=iWriteReq->ReadRemote(&des,0);
	if (r!=KErrNone)
		return r;
	iWriteReq->RemoteDesOffset()+=start_len;
	iWriteReq->Length()-=start_len;
	iDataBufPos=0;
	iDataBufRemain=write_len;
	iWriteError=KErrNone;

	__KTRACE_OPT(KLOCDRV,Kern::Printf("Flash:Write iWritePos=%08x iDataBufRemain=%x",iWritePos,iDataBufRemain));
	__KTRACE_OPT(KLOCDRV,Kern::Printf("Flash:Write Pos=%08x Length=%08x RemDesOff=%08x",
										(TInt)iWriteReq->Pos(),(TInt)iWriteReq->Length(),iWriteReq->RemoteDesOffset()));

	if (iEraseState==EEraseIdle || iEraseState==ESuspended)
		{
		// can start the write now
		iWriteState=EWriting;
		WriteStep();
		}
	else if (iEraseState==EErase)
		{
		// erase in progress - suspend it
		SuspendErase();
		}
	else if (iEraseState==EEraseNoSuspend)
		CHANGE_ERASE_STATE(ESuspendPending);	// wait for suspend to complete
	
	return KErrNone;
	}

void DMediaDriverFlashTemplate::WriteStep()
	{
	__KTRACE_OPT(KLOCDRV,Kern::Printf("Flash:WriteStep @%08x",iWritePos));
	if (iDataBufRemain)
		{
		// still data left in buffer
		volatile TFLASHWORD* pF=(volatile TFLASHWORD*)(iBase+iWritePos);
		TInt i=KMaxWriteSetupAttempts;
		*pF=KCmdClearStatusRegister;
		TUint32 s=0;
		for (; i>0 && ((s&KStsReady)!=KStsReady); --i)
			{
			*pF=KCmdWriteToBuffer;		// send write command
			*pF=KCmdReadStatusRegister;	// send read status command
			s=*pF;						// read status reg
			}
		__KTRACE_OPT(KLOCDRV,Kern::Printf("i=%d, s=%08x",i,s));

		// calculate the buffer size in words -1 
		TFLASHWORD l = (FLASH_BYTES_TO_WORDS(iWriteBufferSize)) - 1;

#if FLASH_BUS_DEVICES == 2		// 2x16bit or 2x8bit devices
		l|= l<< BUS_WIDTH_PER_DEVICE;
#elif FLASH_BUS_DEVICES == 4	// 4x8bit device
		l|= (l<<BUS_WIDTH_PER_DEVICE) | (l<<BUS_WIDTH_PER_DEVICE*2) (l<<BUS_WIDTH_PER_DEVICE*3);
#endif

		// write the data length in words to the device(s)
		*pF=l;

		const TFLASHWORD* pS=(const TFLASHWORD*)(iData+iDataBufPos);

		// write the data
		TInt len;
		for (len = l; len>=0; len--)
			{
			*pF++=*pS++;
			}
	
		*(volatile TFLASHWORD *)(iBase+iWritePos) = KCmdConfirm; 

		// set up timer to poll for completion
		StartPollTimer(KFlashWriteTimerPeriod,KFlashWriteTimerRetries);

		iWritePos+=iWriteBufferSize;
		iDataBufPos+=iWriteBufferSize;
		iDataBufRemain-=iWriteBufferSize;
		if (!iDataBufRemain)
			{
			// refill buffer
			TInt len=(TInt)iWriteReq->Length();
			if (!len)
				return;	// all data has been written, complete request next time
			TUint32 wb_mask=iWriteBufferSize-1;
			TInt block_len=Min(len,KDataBufSize);
			TInt write_len=(block_len+wb_mask)&~wb_mask;
			memset(iData+write_len-iWriteBufferSize,0xff,iWriteBufferSize);
			TPtr8 des(iData,0,block_len);
			TInt r=iWriteReq->ReadRemote(&des,0);
			if (r!=KErrNone)
				{
				iWriteError=r;
				return;	// leave iDataBufRemain=0 so request is terminated when write completes
				}
			iWriteReq->RemoteDesOffset()+=block_len;
			iWriteReq->Length()-=block_len;
			iDataBufPos=0;
			iDataBufRemain=write_len;
			}
		}
	else
		{
		// write request should have completed, maybe with an error
		__ASSERT_ALWAYS(iWriteReq->Length()==0 || iWriteError,FLASH_FAULT());
		iWriteState=EWriteIdle;
		Complete(EReqWrite,iWriteError);
		if (iEraseState==ESuspended)
			StartErase();
		}
	}

/**
Erase at the location indicated by DMediaDriverFlash::iEraseReq

@return >0			Defer request to ELOCD. Read or a write is in progress
@return KErrNone	Erase has been started
@return <0			An error has occured.
 */
TInt DMediaDriverFlashTemplate::DoErase()
	{
	if (iReadReq || iWriteReq)
		return KMediaDriverDeferRequest;		// read or write in progress so defer this request
	TUint32 pos=(TUint32)iEraseReq->Pos();
	TUint32 len=(TUint32)iEraseReq->Length();
	__KTRACE_OPT(KLOCDRV,Kern::Printf("Flash:DoErase %d@%08x",len,pos));
	if (len!=iEraseBlockSize)
		return KErrArgument;	// only allow single-block erase
	if (pos & (iEraseBlockSize-1))
		return KErrArgument;	// start position must be on erase block boundary
	iErasePos=pos;
	__ASSERT_ALWAYS(iEraseState==EEraseIdle,FLASH_FAULT());
	StartErase();
	return KErrNone;
	}

void DMediaDriverFlashTemplate::StartHoldOffTimer()
	{
	// if this is a retry, don't allow suspends
	if (iEraseAttempt==0)
		iHoldOffTimer.OneShot(KEraseSuspendHoldOffTime);
	}

void DMediaDriverFlashTemplate::CancelHoldOffTimer()
	{
	iHoldOffTimer.Cancel();
	ClearEvents(EHoldOffEnd);
	}

void DMediaDriverFlashTemplate::ClearEvents(TUint32 aEvents)
	{
	__e32_atomic_and_ord32(&iEvents, ~aEvents);
	}

void DMediaDriverFlashTemplate::HoldOffTimerFn(TAny* aPtr)
	{
	DMediaDriverFlashTemplate* p=(DMediaDriverFlashTemplate*)aPtr;
	p->IPostEvents(EHoldOffEnd);
	}

void DMediaDriverFlashTemplate::StartPollTimer(TUint32 aPeriod, TUint32 aRetries)
	{
	__KTRACE_OPT(KLOCDRV,Kern::Printf("Flash:Tmr %d * %d",aPeriod,aRetries));

	ClearEvents(EPollTimer);
	iPollPeriod=aPeriod;
	iPollRetries=aRetries;
	StartPollTimer();
	}

void DMediaDriverFlashTemplate::StartPollTimer()
	{
	// TO DO: (mandatory)
	// Configure the hardware timer to expire after iPollPeriod ticks
	// and start the timer
	
	}

void DMediaDriverFlashTemplate::EventDfc(TAny* aPtr)
	{
	DMediaDriverFlashTemplate* p=(DMediaDriverFlashTemplate*)aPtr;
	TUint32 e = __e32_atomic_swp_ord32(&p->iEvents, 0);
	if (e)
		p->HandleEvents(e);
	}

void DMediaDriverFlashTemplate::HandleEvents(TUint32 aEvents)
	{
	__KTRACE_OPT(KLOCDRV,Kern::Printf("Flash:Events %x",aEvents));
	if (aEvents & EHoldOffEnd)
		{
		if (iEraseState==ESuspendPending)
			{
			SuspendErase();
			}
		else if (iEraseState==EEraseNoSuspend)
			{
			CHANGE_ERASE_STATE(EErase);	// can now be suspended
			}
		else
			{
			__KTRACE_OPT(KPANIC,Kern::Printf("iEraseState=%d",iEraseState));
			FLASH_FAULT();
			}
		}
	if (aEvents & EPollTimer)
		{
		volatile TFLASHWORD* pF=(volatile TFLASHWORD*)iBase;
		*pF=KCmdReadStatusRegister;
		if ((*pF & KStsReady)!=KStsReady)
			{
			// not ready yet
			if (--iPollRetries)
				{
				// try again
				StartPollTimer();
				}
			else
				// timed out
				aEvents|=ETimeout;
			}
		else
			{
			// ready
			TFLASHWORD s=*pF;	// read full status value
			*pF=KCmdClearStatusRegister;
			DoFlashReady(s);
			}
		}
	if (aEvents & ETimeout)
		{
		DoFlashTimeout();
		}
	}

void DMediaDriverFlashTemplate::StartErase()
	{
	TFLASHWORD s=KStsReady;
	TInt i;
	volatile TFLASHWORD* pF=(volatile TFLASHWORD*)(iBase+iErasePos);
	__KTRACE_OPT(KLOCDRV,Kern::Printf("Flash:StartErase %08x",pF));
	switch (iEraseState)
		{
		case EEraseIdle:	// first attempt to erase
			iEraseAttempt=-1;
			// coverity[fallthrough]
			// fallthrough after attempt
		case EErase:	// retry after verify failed
		case EEraseNoSuspend:
			++iEraseAttempt;
			*pF=KCmdBlockErase;
			*pF=KCmdConfirm;
			CHANGE_ERASE_STATE(EEraseNoSuspend);
			iEraseError=0;
			StartHoldOffTimer();
			break;
		case ESuspended:
			*pF=KCmdClearStatusRegister;
			*pF=KCmdEraseResume;
			CHANGE_ERASE_STATE(EEraseNoSuspend);
			i=KMaxEraseResumeAttempts;
			for (; i>0 && ((s&KStsReady)!=0); --i)
				{
				*pF=KCmdReadStatusRegister;	// send read status command
				s=*pF;						// read status reg
				s=*pF;						// read status reg
				}
			__KTRACE_OPT(KLOCDRV,Kern::Printf("RESUME: i=%d, s=%08x",i,s));
			StartHoldOffTimer();
			break;
		default:
			__KTRACE_OPT(KPANIC,Kern::Printf("iEraseState=%d",iEraseState));
			FLASH_FAULT();
		}
	StartPollTimer(KFlashEraseTimerPeriod,KFlashEraseTimerRetries);
	}

void DMediaDriverFlashTemplate::SuspendErase()
	{
	__ASSERT_ALWAYS(iEraseState==EErase || iEraseState==ESuspendPending,FLASH_FAULT());
	volatile TFLASHWORD* pF=(volatile TFLASHWORD*)(iBase+iErasePos);
	__KTRACE_OPT(KLOCDRV,Kern::Printf("Flash:SuspendErase %08x",pF));
	*pF=KCmdEraseSuspend;
	CHANGE_ERASE_STATE(ESuspending);
	StartPollTimer(KFlashSuspendTimerPeriod,KFlashSuspendTimerRetries);
	}

void DMediaDriverFlashTemplate::StartPendingRW()
	{
	// start any pending read or write requests
	if (iReadReq)
		DoRead();
	if (iWriteReq)
		{
		// can start the write now
		iWriteState=EWriting;
		WriteStep();
		}
	}

void DMediaDriverFlashTemplate::DoFlashReady(TUint32 aStatus)
	{
	// could be write completion, erase completion or suspend completion
	if (iWriteState==EWriting)
		{
		// write completion
		__KTRACE_OPT(KLOCDRV,Kern::Printf("Flash:WriteComplete %08x",aStatus));
		TUint32 err=aStatus & (KStsWriteError|KStsVppLow|KStsLocked);
		if (err)
			{
			iWriteState=EWriteIdle;
			Complete(EReqWrite,KErrGeneral);
			if (iEraseState==ESuspended)
				StartErase();
			}
		else
			WriteStep();
		return;
		}

	// put the FLASH back into read mode
	volatile TFLASHWORD* pF=(volatile TFLASHWORD*)(iBase+iErasePos);
	*pF=KCmdReadArray;

	if (iEraseState==ESuspending)
		{
		// erase suspend completion
		__KTRACE_OPT(KLOCDRV,Kern::Printf("Flash:SuspendComplete %08x",aStatus));

		// accumulate errors during erase
		iEraseError|=(aStatus & (KStsEraseError|KStsVppLow|KStsLocked));

		if (aStatus & KStsSuspended)
			{
			// at least one of the two FLASH devices has suspended
			CHANGE_ERASE_STATE(ESuspended);

			// start any pending read or write requests
			StartPendingRW();
			return;					// in case erase has been resumed by DoRead()
			}

		// erase completed before we suspended it
		CHANGE_ERASE_STATE(EErase);
		}
	if (iEraseState==EErase || iEraseState==EEraseNoSuspend)
		{
		// erase completion
		__KTRACE_OPT(KLOCDRV,Kern::Printf("Flash:EraseComplete %08x",aStatus));
		CancelHoldOffTimer();

		// accumulate errors during erase
		iEraseError|=(aStatus & (KStsEraseError|KStsVppLow|KStsLocked));

		TFLASHWORD x = FLASH_ERASE_WORD_VALUE;

		// if no device error, verify that erase was successful
		if (!iEraseError)
			{
			volatile TFLASHWORD* p=pF;
			volatile TFLASHWORD* pE=p + FLASH_BYTES_TO_WORDS(iEraseBlockSize);
			while(p<pE)
				x&=*p++;
			}
		else
			{
			}
		if (x == FLASH_ERASE_WORD_VALUE)
			{
			// erase OK
			__KTRACE_OPT(KLOCDRV,Kern::Printf("Flash:VerifyErase OK"));
			CHANGE_ERASE_STATE(EEraseIdle);

			// complete the erase request
			TInt r=iEraseError?KErrGeneral:KErrNone;
			Complete(EReqErase,r);

			// start any pending read or write requests
			StartPendingRW();
			}
		else
			{
			// erase failed, so retry
			__KTRACE_OPT(KLOCDRV,Kern::Printf("Flash:VerifyErase BAD"));
			StartErase();
			}
		}
	}

void DMediaDriverFlashTemplate::DoFlashTimeout()
	{
	// TO DO: (optional)
	// Take appropriate action to handle a timeout.
	FLASH_FAULT();	// // EXAMPLE ONLY:
	}

DMediaDriverFlash* DMediaDriverFlash::New(TInt aMediaId)
	{
	return new DMediaDriverFlashTemplate(aMediaId);
	}

void DMediaDriverFlashTemplate::Isr(TAny* aPtr)
	{
	DMediaDriverFlashTemplate& d=*(DMediaDriverFlashTemplate*)aPtr;

	
	// TO DO: (mandatory)
	// Write to the timer hardware register(s) to
	// clear the timer interrupt
	//
	
	d.IPostEvents(EPollTimer);
	}

void DMediaDriverFlashTemplate::IPostEvents(TUint32 aEvents)
	{
	iEvents|=aEvents;
	iEventDfc.Add();
	}
