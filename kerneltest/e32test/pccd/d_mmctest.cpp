// Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
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
// This LDD provides a set of direct interface functions with the kernel 
// MultiMediaCard Controller
// 
//

#include <kernel/kernel.h>
#include <drivers/mmc.h>
#include "d_mmctest.h"

const TInt KMajorVersionNumber=1;
const TInt KMinorVersionNumber=0;
const TInt KBuildVersionNumber=0;

const TInt KStackNumber  = 0;

const TInt KMaxMMCCardsPerStack = 4;

// global Dfc Que
TDynamicDfcQue* gDfcQ;

class DLddFactoryMmcCntrlInterface : public DLogicalDevice
	{
public:
	DLddFactoryMmcCntrlInterface();
	virtual ~DLddFactoryMmcCntrlInterface();
	virtual TInt Install();
	virtual void GetCaps(TDes8 &aDes) const;
	virtual TInt Create(DLogicalChannelBase*& aChannel); 	//overriding pure virtual
	};

class DLddMmcCntrlInterface : public DLogicalChannel
	{
public:
	DLddMmcCntrlInterface();
	~DLddMmcCntrlInterface();
protected:
	virtual TInt DoCreate(TInt aUnit, const TDesC8* anInfo, const TVersion& aVer);
	virtual void HandleMsg(class TMessageBase *);
private:
	void DoCancel(TInt aReqNo);
	TInt DoRequest(TInt aReqNo, TRequestStatus* aStatus, TAny* a1, TAny* a2);
	TInt DoControl(TInt aFunction, TAny* a1, TAny* a2);
	TInt PrintCardInfo();	
private:
    void Reset();
    static void SessionEndCallBack(TAny *aPtr);
    static void SessionEndDfc(TAny *aPtr);
	static void EventCallBack(TAny* aPtr, TInt aReason, TAny* a1, TAny* a2);

private:
    enum TPanic {EReadDes,EWriteDes,EWriteTInt,EUnknownMmcSes};
    enum TMmcSessionCmd {EMmcSesNone,EMmcSesReadBlk,EMmcSesWriteBlk, EMmcSesReadExtCSD};
    TInt iStackNum;
	DMMCSocket* iSocketP;
    DMMCStack* iStack;
    TMMCard* iCard;
    DMMCSession* iSession;
    TMmcSessionCmd iMmcSessionCmd;
    TAny* iClientDesPtr;
	TUint32 iBlkOffet;
	TRequestStatus* iReadWriteStatusP;
    TMMCCallBack iSessionEndCallBack;
	TDfc iSessionEndDfc;
	TPBusCallBack iBusEventCallback;
	TRequestStatus* iPowerUpStatusP;
//	TUint8 iBuf[KDrvBufSizeInBytes];	// iBuf now uses the MMC DMA buffer, until DT issue with H4 is resolved
	TUint8* iBuf;
	DThread* iClient;
	TExtendedCSD iExtendedCSD;
    };

DECLARE_STANDARD_LDD()
	{
	return new DLddFactoryMmcCntrlInterface;
	}

DLddFactoryMmcCntrlInterface::DLddFactoryMmcCntrlInterface()
//
// Constructor
//
	{

    iParseMask=KDeviceAllowUnit;  // Pass stack number as unit
	iUnitsMask=0xffffffff;
	iVersion=TVersion(KMajorVersionNumber,KMinorVersionNumber,KBuildVersionNumber);
	}

TInt DLddFactoryMmcCntrlInterface::Create(DLogicalChannelBase*& aChannel)
//
// Create a new DLddFactoryMmcCntrlInterface on this logical device
//
	{
	aChannel=new DLddMmcCntrlInterface;
	return aChannel ? KErrNone : KErrNoMemory;
	}

const TInt KDMmcThreadPriority = 27;
_LIT(KDMmcThread,"DMmcTestThread");

TInt DLddFactoryMmcCntrlInterface::Install()
//
// Install the device driver.
//
	{
	// Allocate a kernel thread to run the DFC 
	TInt r = Kern::DynamicDfcQCreate(gDfcQ, KDMmcThreadPriority, KDMmcThread);

	if (r != KErrNone)
		return r; 	

    TPtrC name=_L("MmcTest");
	return(SetName(&name));
	}

void DLddFactoryMmcCntrlInterface::GetCaps(TDes8 &aDes) const
//
// Return the Pc Card Contoller Interface ldd's capabilities.
//
	{

    TCapsMmcIfV01 b;
	b.version=TVersion(KMajorVersionNumber,KMinorVersionNumber,KBuildVersionNumber);
    Kern::InfoCopy(aDes,(TUint8*)&b,sizeof(b));
	}

/**
  Destructor
*/
DLddFactoryMmcCntrlInterface::~DLddFactoryMmcCntrlInterface()
	{
	if (gDfcQ)
		gDfcQ->Destroy();
	}

#pragma warning( disable : 4355 )	// this used in initializer list
DLddMmcCntrlInterface::DLddMmcCntrlInterface()
//
// Constructor
//
	: iSessionEndCallBack(DLddMmcCntrlInterface::SessionEndCallBack,this),
	  iSessionEndDfc(DLddMmcCntrlInterface::SessionEndDfc, this, 1),
	  iBusEventCallback(DLddMmcCntrlInterface::EventCallBack, this)
	{

//	iMmcController=NULL;
//  iStackNum=0;
//  iStack=NULL;
//  iCard=NULL;
//  iSession=NULL;
//  iMmcSessionCmd=EMmcSesNone;
//  iClientDesPtr=NULL;
//	iBlkOffet=0;

	iClient=&Kern::CurrentThread();
	((DObject*)iClient)->Open();	// can't fail since thread is running
    }
#pragma warning( default : 4355 )

DLddMmcCntrlInterface::~DLddMmcCntrlInterface()
//
// Destructor
//
	{

    Reset();
	iBusEventCallback.Remove();
	delete iSession;
	Kern::SafeClose((DObject*&)iClient,NULL);
    }

TInt DLddMmcCntrlInterface::DoCreate(TInt aUnit, const TDesC8* /*aInfo*/, const TVersion& aVer)
//
// Create channel.
//
	{    
	if (!Kern::QueryVersionSupported(TVersion(KMajorVersionNumber,KMinorVersionNumber,KBuildVersionNumber),aVer))
		return(KErrNotSupported);

	//
	// Obtain the appropriate card from the socket/stack
	//
	iSocketP = static_cast<DMMCSocket*>(DPBusSocket::SocketFromId(aUnit));
	if(iSocketP == NULL)
		return(KErrNoMemory);

	iStack = static_cast<DMMCStack*>(iSocketP->Stack(KStackNumber));
	if(iStack == NULL)
		return(KErrNoMemory);

    // Create an MMC session object
	iSession = iStack->AllocSession(iSessionEndCallBack);
    if (iSession==NULL)
		return(KErrNoMemory);
		
    iSession->SetStack(iStack);
   
	TUint8* buf;
	TInt bufLen;
	TInt minorBufLen;
	iStack->BufferInfo(buf, bufLen, minorBufLen);
	iBuf = buf;

	SetDfcQ(gDfcQ);
	iMsgQ.Receive();
	
	iSessionEndDfc.SetDfcQ(gDfcQ);

	iBusEventCallback.SetSocket(aUnit);
	iBusEventCallback.Add();

    return(KErrNone);
	}

void DLddMmcCntrlInterface::DoCancel(TInt /*aReqNo*/)
//
// Cancel an outstanding request.
//
	{
	}

void DLddMmcCntrlInterface::HandleMsg(TMessageBase* aMsg)
    {
    TThreadMessage& m=*(TThreadMessage*)aMsg;
    TInt id=m.iValue;
    
	if (id==(TInt)ECloseMsg)
		{
		m.Complete(KErrNone, EFalse);
		return;
		}
    else if (id==KMaxTInt)
		{
		// DoCancel
		m.Complete(KErrNone,ETrue);
		return;
		}

    if (id<0)
		{
		// DoRequest
		TRequestStatus* pS=(TRequestStatus*)m.Ptr0();
		TInt r=DoRequest(~id, pS, m.Ptr1(), m.Ptr2());
		if (r!=KErrNone)
	    	Kern::RequestComplete(iClient, pS, r);
		m.Complete(KErrNone,ETrue);
		}
    else
		{
		// DoControl
		TInt r=DoControl(id,m.Ptr0(),m.Ptr1());
		if(r != KErrCompletion)
			{
			m.Complete(r,ETrue);
			}
		}
	}

TInt DLddMmcCntrlInterface::PrintCardInfo()
    {
    if(iCard == NULL)
        {
        return(KErrNotReady);
        }
    const TCSD& csd = iCard->CSD();
    Kern::Printf("CSD");    
    Kern::Printf("CSDStructure():                 %u",csd.CSDStructure());
    Kern::Printf("SpecVers():                     %u",csd.SpecVers());
    switch (csd.MediaType())
        {
        case EMultiMediaROM   : Kern::Printf("Read Only Media"); break;
        case EMultiMediaFlash : Kern::Printf("Writable Media"); break;
        case EMultiMediaIO    : Kern::Printf("IO Media Device"); break;
        case EMultiMediaOther : Kern::Printf("UNKNOWN Media type"); break;
        default : Kern::Printf("Media NOT SUPPORTED");
        }    
    Kern::Printf("----------------------------------");
    Kern::Printf("Reserved120():                  %u",csd.Reserved120());
    Kern::Printf("TAAC():                         %u", csd.TAAC());
    Kern::Printf("NSAC():                         %u", csd.NSAC());
    Kern::Printf("TranSpeed():                    %u", csd.TranSpeed());
    Kern::Printf("CCC():                          %u",csd.CCC());
    Kern::Printf("ReadBlLen():                    %u", csd.ReadBlLen());
    Kern::Printf("ReadBlPartial():                %u", (TUint)csd.ReadBlPartial());
    Kern::Printf("WriteBlkMisalign():             %u", (TUint) csd.WriteBlkMisalign());
    Kern::Printf("ReadBlkMisalign():              %u", (TUint) csd.ReadBlkMisalign());
    Kern::Printf("DSRImp():                       %u", (TUint) csd.DSRImp());
    Kern::Printf("Reserved74():                   %u", csd.Reserved74());
    Kern::Printf("CSize():                        %u", csd.CSize());
    Kern::Printf("VDDRCurrMin():                  %u", csd.VDDRCurrMin());
    Kern::Printf("VDDRCurrMax():                  %u", csd.VDDRCurrMax());
    Kern::Printf("VDDWCurrMin():                  %u", csd.VDDWCurrMin());
    Kern::Printf("VDDWCurrMax():                  %u", csd.VDDWCurrMax());
    Kern::Printf("CSizeMult():                    %u", csd.CSizeMult());
    Kern::Printf("EraseGrpSize():                 %u", csd.EraseGrpSize());
    Kern::Printf("EraseGrpMult():                 %u", csd.EraseGrpMult());
    Kern::Printf("WPGrpSize():                    %u", csd.WPGrpSize());
    Kern::Printf("WPGrpEnable():                  %u", csd.WPGrpEnable());            
    Kern::Printf("DefaultECC():                   %u", csd.DefaultECC());
    Kern::Printf("R2WFactor():                    %u", csd.R2WFactor());
    Kern::Printf("WriteBlLen():                   %u", csd.WriteBlLen());
    Kern::Printf("WriteBlPartial():               %u", (TUint) csd.WriteBlPartial());
    Kern::Printf("Reserved16():                   %u", csd.Reserved16());
    Kern::Printf("FileFormatGrp():                %u", (TUint) csd.FileFormatGrp());
    Kern::Printf("Copy():                         %u", (TUint) csd.Copy());
    Kern::Printf("PermWriteProtect():             %u", (TUint) csd.PermWriteProtect());
    Kern::Printf("TmpWriteProtect():              %u", (TUint) csd.TmpWriteProtect());
    Kern::Printf("FileFormat():                   %u", csd.FileFormat());
    Kern::Printf("ECC():                          %u", csd.ECC());
    Kern::Printf("CRC():                          %u", csd.CRC());
    Kern::Printf("DeviceSize():                   %u", csd.DeviceSize());   
    Kern::Printf("ReadBlockLength():              %u", csd.ReadBlockLength());
    Kern::Printf("WriteBlockLength():             %u", csd.WriteBlockLength());
    Kern::Printf("EraseSectorSize():              %u", csd.EraseSectorSize());
    Kern::Printf("EraseGroupSize():               %u", csd.EraseGroupSize());
    Kern::Printf("MinReadCurrentInMilliamps():    %u", csd.MinReadCurrentInMilliamps());
    Kern::Printf("MinWriteCurrentInMilliamps():   %u", csd.MinWriteCurrentInMilliamps());
    Kern::Printf("MaxReadCurrentInMilliamps():    %u", csd.MaxReadCurrentInMilliamps());
    Kern::Printf("MaxWriteCurrentInMilliamps():   %u", csd.MaxWriteCurrentInMilliamps());
    Kern::Printf("MaxTranSpeedInKilohertz():      %u", csd.MaxTranSpeedInKilohertz());
    
    const TExtendedCSD& extcsd = iCard->iExtendedCSD;
    Kern::Printf("\nExtended CSD");
    Kern::Printf("CSDStructureVer:                %u", extcsd.CSDStructureVer());
    Kern::Printf("ExtendedCSDRev:                 %u", extcsd.ExtendedCSDRev());
    Kern::Printf("----------------------------------");
    Kern::Printf("SupportedCmdSet:                %u", extcsd.SupportedCmdSet());
    Kern::Printf("SectorCount:                    %u", extcsd.SectorCount());
    Kern::Printf("MinPerfWrite8Bit52Mhz:          %u", extcsd.MinPerfWrite8Bit52Mhz());
    Kern::Printf("MinPerfRead8Bit52Mhz:           %u", extcsd.MinPerfRead8Bit52Mhz());
    Kern::Printf("MinPerfRead8Bit26Mhz_4Bit52Mhz: %u", extcsd.MinPerfRead8Bit26Mhz_4Bit52Mhz());
    Kern::Printf("MinPerfWrite4Bit26Mhz:          %u", extcsd.MinPerfWrite4Bit26Mhz());
    Kern::Printf("MinPerfRead4Bit26Mhz:           %u", extcsd.MinPerfRead4Bit26Mhz());
    Kern::Printf("PowerClass26Mhz360V:            0x%02X", extcsd.PowerClass26Mhz360V());
    Kern::Printf("PowerClass52Mhz360V:            0x%02X", extcsd.PowerClass52Mhz360V());
    Kern::Printf("PowerClass26Mhz195V:            0x%02X", extcsd.PowerClass26Mhz195V());
    Kern::Printf("PowerClass52Mhz195V:            0x%02X", extcsd.PowerClass52Mhz195V());
    Kern::Printf("CardType:                       %u", extcsd.CardType());
    Kern::Printf("CmdSet:                         %u", extcsd.CmdSet());
    Kern::Printf("CmdSetRev:                      %u", extcsd.CmdSetRev());
    Kern::Printf("PowerClass:                     %u", extcsd.PowerClass());
    Kern::Printf("HighSpeedTiming:                %u", extcsd.HighSpeedTiming());
    Kern::Printf("BusWidthMode:                   %u", extcsd.BusWidthMode());
    Kern::Printf("HighCapacityEraseGroupSize:     %u", extcsd.HighCapacityEraseGroupSize());
    Kern::Printf("AccessSize:                     %u", extcsd.AccessSize());
    Kern::Printf("BootInfo:                       %u", extcsd.BootInfo() );
    Kern::Printf("BootSizeMultiple:               %u", extcsd.BootSizeMultiple() );
    Kern::Printf("EraseTimeoutMultiple:           %u", extcsd.EraseTimeoutMultiple() );
    Kern::Printf("ReliableWriteSector:            %u", extcsd.ReliableWriteSector() );
    Kern::Printf("HighCapWriteProtGroupSize:      %u", extcsd.HighCapacityWriteProtectGroupSize() );
    Kern::Printf("SleepCurrentVcc:                %u", extcsd.SleepCurrentVcc() );
    Kern::Printf("SleepCurrentVccQ:               %u", extcsd.SleepCurrentVccQ());
    Kern::Printf("SleepAwakeTimeout:              %u", extcsd.SleepAwakeTimeout());
    Kern::Printf("BootConfig:                     %u", extcsd.BootConfig());
    Kern::Printf("BootBusWidth:                   %u", extcsd.BootBusWidth());
    Kern::Printf("EraseGroupDef:                  %u", extcsd.EraseGroupDef());
    
    return KErrNone;
    }

TInt DLddMmcCntrlInterface::DoRequest(TInt aFunction, TRequestStatus* aStatus, TAny* a1, TAny* a2)
//
// Most Async requests
//
	{

    if (iMmcSessionCmd!=EMmcSesNone)
		{
		return(KErrInUse);
		}

    switch (aFunction)
		{
        case RMmcCntrlIf::EReqPwrUp:
			{
			if(!iSocketP->CardIsPresent())
				{
				Kern::RequestComplete(iClient, aStatus, KErrNotReady);
				}
			else if(iSocketP->State() == EPBusOn)
				{
				Kern::RequestComplete(iClient, aStatus, KErrNone);
				}
			else
				{
				iPowerUpStatusP = aStatus;
				iSocketP->PowerUp();
				}
			break;
			}
        case RMmcCntrlIf::EReqReadSect:
            {
			if(iCard == NULL)
				{
				return(KErrNotReady);
				}

			//TCSD csd=iCard->CSD();
			iReadWriteStatusP = aStatus;
			TUint32 srcAddr=((TUint32)a1)<<KSectorSizeShift;
			TUint readBlLen = 1 << iCard->MaxReadBlLen();
			TUint readBlMask=(readBlLen-1);
			iBlkOffet=srcAddr-(srcAddr&(~readBlMask));
			iClientDesPtr=a2;
			iMmcSessionCmd=EMmcSesReadBlk;
			srcAddr&=(~readBlMask);
			TMMCArgument da(srcAddr);
			iSession->SetupCIMReadBlock(da,readBlLen,&iBuf[0]);
			iSession->Engage();
			break;
            }
        case RMmcCntrlIf::EReqWriteSect:
            {
			if(iCard == NULL)
				{
				return(KErrNotReady);
				}

			iReadWriteStatusP = aStatus;
			TUint32 destAddr=((TUint32)a1)<<KSectorSizeShift;
			TUint writeBlLen=1 << iCard->MaxWriteBlLen();
			TUint writeBlMask=(writeBlLen-1);
			iBlkOffet=destAddr-(destAddr&(~writeBlMask));
			TPtr8* srcDes = (TPtr8*)a2;		
			TPtr8 ptr(&iBuf[iBlkOffet],KSectorSizeInBytes,KSectorSizeInBytes);
			TInt r = Kern::ThreadDesRead(iClient, srcDes, ptr, 0, KChunkShiftBy0);
			if(r != KErrNone)
				{
				return(r);
				}
				
			iMmcSessionCmd=EMmcSesWriteBlk;
			destAddr&=(~writeBlMask);
			iSession->SetupCIMWriteBlock(TMMCArgument(destAddr),writeBlLen,&iBuf[0]);
			iSession->Engage();
			break;
			}

        case RMmcCntrlIf::EReqReadExtCSD:
            {
			if(iCard == NULL)
				{
				return(KErrNotReady);
				}

			iClientDesPtr = a1;
			iReadWriteStatusP = aStatus;
			iMmcSessionCmd = EMmcSesReadExtCSD;

			iSession->SetupDTCommand(
				ECmdSendExtendedCSD, 
				TMMCArgument(0),
				KMMCExtendedCSDLength,
				(TUint8*) &iExtendedCSD);

			iSession->Engage();
			break;
            }
            
        case RMmcCntrlIf::EReqMMCInfoPrint:
            {
            // Print CSD & Extended CSD values
            TInt r = PrintCardInfo();            
            Kern::RequestComplete(iClient, aStatus, r);
            break;
            }

        }
	return(KErrNone);
	}

TInt DLddMmcCntrlInterface::DoControl(TInt aFunction,TAny* a1,TAny* /*a2*/)
//
// Mostly requests (but some kernel server async ones)
//
	{

	TInt r=KErrNotSupported;
	switch (aFunction)
		{
        case RMmcCntrlIf::ESvReset:
            {
            Reset();
			r=KErrNone;
			break;
            }
        case RMmcCntrlIf::ESvPwrDown:
			{
//		    iStack->PowerDown(); ???
//          iMmcController->SetPowerEvent(0,EPEventPwrDownNormal,0);
			r=KErrNone;
			break;
			}
        case RMmcCntrlIf::EExecStackInfo:
            {
            // Determine the number of cards present
            TUint cardsPresentMask=0x00000000;
	        TMMCard* card;
            for (TInt i=0;i<KMaxMMCCardsPerStack;i++)
                {
                card=iStack->CardP(i);
                if (card!=NULL && card->IsPresent())
                    cardsPresentMask|=(0x1<<i);
                }
			r = Kern::ThreadRawWrite(iClient,a1,&cardsPresentMask,sizeof(TUint));
			break;
            }
        case RMmcCntrlIf::ESvRegisterEvent:
            {
			return(KErrNotSupported);
            }
        case RMmcCntrlIf::EExecSelectCard:
            {
            iCard=iStack->CardP((TUint)a1);
            iSession->SetCard(iCard);
			r=KErrNone;
			break;
            }
        case RMmcCntrlIf::EExecCardInfo:
            {
            if (iCard)
                {
				TMmcCardInfo ci;
	            ci.iIsReady=iCard->IsPresent();
	            ci.iIsLocked=iCard->IsLocked();
				TCID* cid=(TCID*)&(iCard->CID());
				TInt i;
				for (i=0;i<16;i++)
					ci.iCID[i]=cid->At(i);
				const TCSD& csd = iCard->CSD();
				for (i=0;i<16;i++)
					ci.iCSD[i]=csd.At(i);
	            ci.iRCA=TUint16(iCard->RCA());
	            ci.iMediaType=(TMmcMediaType)iCard->MediaType();
                ci.iCardSizeInBytes=iCard->DeviceSize64();
	            ci.iReadBlLen=csd.ReadBlockLength();
	            ci.iWriteBlLen=csd.WriteBlockLength();
	            ci.iReadBlPartial=csd.ReadBlPartial();
	            ci.iWriteBlPartial=csd.WriteBlPartial();
	            ci.iReadBlkMisalign=csd.ReadBlkMisalign();
	            ci.iWriteBlkMisalign=csd.WriteBlkMisalign();
                ci.iReadCurrentInMilliAmps=csd.MaxReadCurrentInMilliamps();
                ci.iWriteCurrentInMilliAmps=csd.MaxWriteCurrentInMilliamps();
	            ci.iSpecVers=csd.SpecVers();
	            ci.iTAAC=csd.TAAC();
	            ci.iNSAC=csd.NSAC();
	            ci.iTransferSpeed=csd.TranSpeed();
	            ci.iCommandRegister=csd.CCC();
	            ci.iHighCapacity = iCard->IsHighCapacity();
				r = Kern::ThreadRawWrite(iClient, a1/*TAny *aDest*/, &ci/*const TAny *aSrc*/, sizeof(TMmcCardInfo));
                }
            else
                r=KErrGeneral;
			break;
            }
		}
	return(r);
	}

void DLddMmcCntrlInterface::Reset()
//
// Release any resources 
//
	{
	iSessionEndDfc.Cancel();
    }

void DLddMmcCntrlInterface::SessionEndCallBack(TAny *aPtr)
//
// Session end callback
//
	{
	DLddMmcCntrlInterface &mci=*(DLddMmcCntrlInterface*)aPtr;

	// Signal request complete using DFC 
	if (!mci.iSessionEndDfc.Queued())
		mci.iSessionEndDfc.Enque();
	}

void DLddMmcCntrlInterface::SessionEndDfc(TAny *aPtr)
//
// Session end dfc
//
	{
	DLddMmcCntrlInterface &mci=*(DLddMmcCntrlInterface*)aPtr;
    TInt err=mci.iSession->EpocErrorCode();
    switch (mci.iMmcSessionCmd)
		{
        case EMmcSesReadBlk:
            {
			TPtr8 ptr(&mci.iBuf[mci.iBlkOffet],KSectorSizeInBytes,KSectorSizeInBytes);
			TPtrC8* srcDes = (TPtrC8*)mci.iClientDesPtr;
			TInt r = Kern::ThreadDesWrite(mci.iClient,srcDes,ptr,0,mci.iClient);

	    	Kern::RequestComplete(mci.iClient, mci.iReadWriteStatusP, (r == KErrNone) ? err : r);
            break;
            }
        case EMmcSesWriteBlk:
			{
			Kern::Printf("EMmcSesWriteBlk Complete");
	    	Kern::RequestComplete(mci.iClient, mci.iReadWriteStatusP, err);
            break;
			}
		case EMmcSesReadExtCSD:
			{
			TPtr8 ptr((TUint8*) &mci.iExtendedCSD, KMMCExtendedCSDLength, KMMCExtendedCSDLength);
			TPtrC8* dstDes = (TPtrC8*)mci.iClientDesPtr;
			TInt r = Kern::ThreadDesWrite(mci.iClient, dstDes, ptr, 0,mci.iClient);

	    	Kern::RequestComplete(mci.iClient, mci.iReadWriteStatusP, (r == KErrNone) ? err : r);
			break;
			}
        default:
        	break;
		}
	mci.iMmcSessionCmd=EMmcSesNone;
	}

void DLddMmcCntrlInterface::EventCallBack(TAny* aPtr, TInt aReason, TAny* a1, TAny* a2)
	{
	DLddMmcCntrlInterface &mci=*(DLddMmcCntrlInterface*)aPtr;

	if(mci.iPowerUpStatusP)
		{
		TInt retCode = KErrCompletion;

		switch(aReason)
			{
			case TPBusCallBack::EPBusStateChange:
				{
				TPBusState newState = (TPBusState)(TInt)a1;
				TInt errorCode = (TInt)a2;

				switch(newState)
					{
					case EPBusCardAbsent:	retCode = KErrNotFound;		break;			
					case EPBusOff:			retCode = errorCode;		break;
					case EPBusPsuFault:		retCode = KErrBadPower;		break;
					case EPBusOn:			retCode = KErrNone;			break;
					case EPBusPowerUpPending:
					case EPBusPoweringUp:
					default:	
						break;
					}

				break;
				}
			}

		if(retCode != KErrCompletion)
			{
   			Kern::RequestComplete(mci.iClient, mci.iPowerUpStatusP, retCode);
			mci.iPowerUpStatusP = NULL;
			}
		}
	}


