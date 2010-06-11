// Copyright (c) 1998-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// e32\drivers\pbus\pccard\socket.cpp
// 
//

#include <pccard.h>
#include "cis.h"

const TInt KFuncGranularity=(KMaxFuncPerCard+1);
const TInt KMemGranularity=2;

TPccdFuncType FuncType(TUint aFuncCode)
	{

	if (aFuncCode<=KCisTplFuncIdScsi)
		return((TPccdFuncType)(aFuncCode+1));
	else if (aFuncCode==KCisTplFuncIdVendorSpecific)
		return(EVendorSpecificCard);
	else
		return(EUnknownCard);
	}

/********************************************
 * PC card power supply
 ********************************************/
DPcCardVcc::DPcCardVcc(TInt aPsuNum, TInt aMediaChangeNum)
	:	DPBusPsuBase(aPsuNum, aMediaChangeNum)
	{
	}

TInt DPcCardVcc::SocketVccToMilliVolts(TPccdSocketVcc aVcc)
//
// Converts a TPccdSocketVcc into a integer value - units mV.
//
	{
	switch (aVcc)
		{
		case EPccdSocket_5V0: return(5000);
		case EPccdSocket_3V3: return(3300);
		default: return(0);
		}
	}

TBool DPcCardVcc::IsLocked()
	{
/*	TInt i;
	Kern::EnterCS();
	for (i=0; i<KMaxPccdSockets; i++)
		{
		DPcCardSocket* pS=(DPcCardSocket*)TheSockets[i];
		if (pS && pS->iVcc==this)
			{
			if (pS->iClientWindows || pS->iActiveConfigs)
				break;
			}
		}
	Kern::LeaveCS();
	return (i<KMaxPccdSockets);
*/
	DPcCardSocket* pS=(DPcCardSocket*)iSocket;
	return (pS->iClientWindows || pS->iActiveConfigs);
	}

void DPcCardVcc::ReceiveVoltageCheckResult(TInt anError)
	{
	__KTRACE_OPT(KPBUS1,Kern::Printf("DPcCardVcc(%d)::ReceiveVoltageCheckResult(%d)",iPsuNum,anError));
	DPcCardSocket* pS=(DPcCardSocket*)iSocket;
	TInt s=pS->iCardPowerUpState;
	if (s==DPcCardSocket::EWaitForVccReading)
		{
		if (anError==KErrNone)
			{
			SetState(EPsuOnFull);
			pS->iCardPowerUpState=DPcCardSocket::EWaitForReady;
			}
		else
			pS->TerminatePowerUpSequence(KErrCorrupt);
		}
	else if (s!=DPcCardSocket::EInit && s!=DPcCardSocket::EApplyingReset && s!=DPcCardSocket::ECheckVcc)
		DPBusPsuBase::ReceiveVoltageCheckResult(anError);
	}

/********************************************
 * PC card media change
 ********************************************/
DPcCardMediaChange::DPcCardMediaChange(TInt aMediaChangeNum)
	: DMediaChangeBase(aMediaChangeNum)
	{
	}

TInt DPcCardMediaChange::Create()
	{
	return DMediaChangeBase::Create();
	}

/********************************************
 * PC card socket
 ********************************************/
void cardPowerUpTick(TAny* aPtr)
	{
	((DPcCardSocket*)aPtr)->iCardPowerUpDfc.Enque();
	}

void cardPowerUpDfc(TAny* aPtr)
	{
	((DPcCardSocket*)aPtr)->CardPowerUpTick();
	}

DPcCardSocket::DPcCardSocket(TSocket aSocketNum)
//
// Constructor.
//
	:	DPBusSocket(aSocketNum),
		iCardFuncArray(KFuncGranularity),
		iMemChunks(KMemGranularity),
		iCardPowerUpDfc(cardPowerUpDfc, this, 1)
	{

	iType=EPBusTypePcCard;
//	iCardPowerUpState=EIdle;
//	iClientWindows=0;
//	iActiveConfigs=0;
	}

TInt DPcCardSocket::Create(const TDesC* aName)
//
// Create a new Socket. Only created once on kernel initialization so don't
// worry about cleanup if it fails.
//
	{

	__KTRACE_OPT(KPBUS1,Kern::Printf(">Skt(%d):Create(%S)",iSocketNumber,aName));

	TInt r=DPBusSocket::Create(aName);
	if (r!=KErrNone)
		return r;
	iCardPowerUpDfc.SetDfcQ(&iDfcQ);

	// Create card function array - add and remove a dummy function to pre-allocate array memory.
	// This way, adding new functions to array never causes any memory allocation.
	r=AddNewFunc(0,EPccdAttribMem);				// Add dummy function
	if (r!=KErrNone)
		return r;
	delete iCardFuncArray[0];					// Destroy dummy function
	iCardFuncArray.Remove(0);					// Remove pointer to dummy from array

	// Now allocate the permanent attribute memory chunk. Don't bother about what
	// access speed we asign, it gets set each time we subsequently access the chunk.
	TPccdChnk chnk(EPccdAttribMem,0,KDefaultAttribMemSize);
	r=iAttribWin.Create(this,chnk,EAcSpeed300nS,KPccdChunkPermanent|KPccdChunkShared|KPccdChunkSystemOwned);
	return r;
	}

void DPcCardSocket::ResetPowerUpState()
	{
	__KTRACE_OPT(KPBUS1,Kern::Printf(">Skt(%d):ResetPowerUpState",iSocketNumber));
	if (iCardPowerUpState!=EIdle)
		{
		iCardPowerUpTimer.Cancel();
		iCardPowerUpDfc.Cancel();
		iCardPowerUpState=EIdle;
		}
	}

void DPcCardSocket::Reset1()
	{
	__KTRACE_OPT(KPBUS1,Kern::Printf(">Skt(%d):Reset1",iSocketNumber));
	ResetPowerUpState();
	}

void DPcCardSocket::Reset2()
//
// Reset the socket (called to remove any allocated memory following a
// media change event).
//
	{

	__KTRACE_OPT(KPBUS1,Kern::Printf(">Skt(%d):Rst2",iSocketNumber));
	// Destroy all the function objects
	TInt i;
	for (i=CardFuncCount()-1;i>=0;i--)
		{
		delete iCardFuncArray[i];
		iCardFuncArray.Remove(i);		// Now remove from array (doesn't cause memory dealloc).
		}
	iActiveConfigs=0;

	// Destroy all the non-permanent Pc Card chunks
	for (i=(iMemChunks.Count()-1);i>=0;i--)
		{
		if ( iMemChunks[i]->IsRemovable() )
			iMemChunks[i]->Close();
		}
	iMemChunks.Compress();
	}

void DPcCardSocket::Restore()
//
// Restore the socket. Normally called when restoring a socket after it has been powered 
// down due to inactivity (but not media change)
//
	{	
	TInt i;
	TPcCardFunction *cf;
	for (i=CardFuncCount()-1;i>=0;i--)
		{
		cf=iCardFuncArray[i];

		TUint8 index;
		if ((index=(TUint8)cf->ConfigOption())!=KInvalidConfOpt && cf->IsRestorableConfig())
			WriteConfigReg(i,KConfigOptionReg,index);
		}
	}

void DPcCardSocket::RemoveChunk(DPccdChunkBase *aChunk)
//
// Remove a chunk from this socket. 
//	
	{
	TInt i;
	for (i=0;i<iMemChunks.Count();i++)
		{
		if (iMemChunks[i]==aChunk)
			{
			iMemChunks.Remove(i);
			iMemChunks.Compress();
			return;
			}
		}
	}

EXPORT_C TInt DPcCardSocket::RequestConfig(TInt aCardFunc,DBase *aClientID,TPcCardConfig &anInfo,TUint aFlag)
//
// Configure the card.
//
	{

	__KTRACE_OPT(KPBUS1,Kern::Printf(">Skt(%d):RequestConfig(F:%d O:%xH B:%xH L:%xH)",iSocketNumber,aCardFunc,\
										   anInfo.iConfigOption,anInfo.iConfigBaseAddr,anInfo.iRegPresent));
	if (!IsValidCardFunc(aCardFunc))
		return(KErrArgument);
	// Check that this function isn't configured already
	TPcCardFunction *cf=iCardFuncArray[aCardFunc];
	if (cf->IsConfigured())
		return(KErrInUse);	   // Its already configured.

	// If configuration registers are within attribute chunk then configure the
	// card (rather than use the registers present info, assume all registers are
	// present - ie size of mask).
	if (anInfo.iConfigBaseAddr+(sizeof(anInfo.iRegPresent)<<1)>KDefaultAttribMemSize)
		return(KErrNotSupported);
	anInfo.iConfigOption&=(KConfOptLevIReqM|KConfOptConfM); // Mustn't allow msb - KInvalidConfOpt
	TPccdAccessSpeed sp=(VccSetting()==EPccdSocket_3V3)?EAcSpeed600nS:EAcSpeed300nS;
	iAttribWin.SetAccessSpeed(sp);
	iAttribWin.SetupChunkHw();
	iAttribWin.Write(anInfo.iConfigBaseAddr,(TUint8*)&anInfo.iConfigOption,1);

	cf->SetConfigRegMask(anInfo.iRegPresent);
	cf->SetConfigBaseAddr(anInfo.iConfigBaseAddr);
	cf->SetConfigOption(anInfo.iConfigOption,aClientID,aFlag);
	__e32_atomic_add_ord32(&iActiveConfigs, 1);
	return(KErrNone);
	}

EXPORT_C void DPcCardSocket::ReleaseConfig(TInt aCardFunc,DBase *aClientID)
//
// Return card to memory only config.
//
	{

	__KTRACE_OPT(KPBUS1,Kern::Printf(">Skt(%d):ReleaseConfig(F:%d)",iSocketNumber,aCardFunc));
	if (IsValidCardFunc(aCardFunc))
		{
		TPcCardFunction *cf=iCardFuncArray[aCardFunc];
		if (cf->IsConfiguredByClient(aClientID))
			{
			if (iState==EPBusOn && Kern::PowerGood())
				WriteConfigReg(aCardFunc,KConfigOptionReg,0); // Restore Config. Option register

			cf->SetConfigRegMask(0);
			cf->SetConfigBaseAddr(0);	
			cf->SetConfigOption(KInvalidConfOpt,NULL,0);
			__e32_atomic_add_ord32(&iActiveConfigs, TUint32(-1));
			}
		}
	}

EXPORT_C TInt DPcCardSocket::ReadConfigReg(TInt aCardFunc,TInt aRegOffset,TUint8 &aVal)
//
// Read from a specified configuration register. (We return an error if the RegPres mask
// indicates the register isn't present but still attempt the read).
//
	{
	TInt offset, err;
	if (!IsValidCardFunc(aCardFunc)|| iState!=EPBusOn || !Kern::PowerGood())
		err=KErrArgument;
	else
		{
		if ((err=iCardFuncArray[aCardFunc]->ConfigRegAddress(aRegOffset,offset))==KErrNone||err==KErrNotSupported)
			{
			TPccdAccessSpeed sp=(VccSetting()==EPccdSocket_3V3)?EAcSpeed600nS:EAcSpeed300nS;
			iAttribWin.SetAccessSpeed(sp);
			iAttribWin.SetupChunkHw();
			iAttribWin.Read(offset,&aVal,1);
			}
		}
	__KTRACE_OPT(KPBUS1,Kern::Printf("<Skt(%d):ReadConfigReg-%d",iSocketNumber,err));
	return(err);
	}

EXPORT_C TInt DPcCardSocket::WriteConfigReg(TInt aCardFunc,TInt aRegOffset,const TUint8 aVal)
//
// Write to a specified configuration register. (We return an error if the RegPres mask
// indicates the register isn't present but still attempt the write).
//
	{
	TInt offset, err;
	if (!IsValidCardFunc(aCardFunc)|| iState!=EPBusOn || !Kern::PowerGood())
		err=KErrArgument;
	else
		{
		if ((err=iCardFuncArray[aCardFunc]->ConfigRegAddress(aRegOffset,offset))==KErrNone||err==KErrNotSupported)
			{
			TPccdAccessSpeed sp=(VccSetting()==EPccdSocket_3V3)?EAcSpeed600nS:EAcSpeed300nS;
			iAttribWin.SetAccessSpeed(sp);
			iAttribWin.SetupChunkHw();
			iAttribWin.Write(offset,&aVal,1);
			}
		}
	__KTRACE_OPT(KPBUS1,Kern::Printf("<Skt(%d):WriteConfigReg-%d",iSocketNumber,err));
	return(err);
	}

const TInt KReadCisBufferSize=0x80;   // 128 Bytes
TInt DPcCardSocket::ReadCis(TPccdMemType aMemType,TInt aPos,TDes8 &aDes,TInt aLen)
//
// Read from CIS 
//
	{

	__KTRACE_OPT(KPBUS2,Kern::Printf(">Skt(%d):ReadCis(LE:%xH PO:%d TY:%d)",iSocketNumber,aLen,aPos,aMemType));
	RPccdWindow newWin;
	RPccdWindow* win=&newWin;
	TBool needNewChunk=ETrue;
	TInt cisE=(aPos+aLen);
	TInt incrm=1;
	if (aMemType==EPccdAttribMem)
		{
		incrm=2;		// Read every other byte
		cisE<<=1;		
		aPos<<=1;
		if (cisE<=(TInt)KDefaultAttribMemSize)
			{
			needNewChunk=EFalse;
			win=&iAttribWin;
			}
		}

	if (needNewChunk)
		{
		TPccdChnk chnk(aMemType,aPos,(cisE-aPos));
		TInt r=newWin.Create(this,chnk,EAcSpeed300nS,KPccdChunkShared|KPccdChunkSystemOwned);
		if (r!=KErrNone)
			return(r);
		cisE-=aPos;
		aPos=0;
		}

	TPccdAccessSpeed sp=(VccSetting()==EPccdSocket_3V3)?EAcSpeed600nS:EAcSpeed300nS;
	win->SetAccessSpeed(sp);
	win->SetupChunkHw();
	aDes.Zero();
	TText8 buf[KReadCisBufferSize];
	TInt s;
	for (;aPos<cisE;aPos+=s)
		{
		s=Min(KReadCisBufferSize,(cisE-aPos));
		win->Read(aPos,&buf[0],s);
		for (TInt i=0;i<s;i+=incrm)
			aDes.Append((TChar)buf[i]);   
		} 

	if (needNewChunk)
		newWin.Close();
	return(KErrNone);
	}

TInt DPcCardSocket::AddNewFunc(TUint32 anOffset,TPccdMemType aMemType)
//
// Create a new card function and append it to the function array
//
	{

	__KTRACE_OPT(KPBUS1,Kern::Printf(">Skt(%d):AddNewFunc(T:%d)",iSocketNumber,aMemType));
	TInt r=KErrNoMemory;
	TPcCardFunction* cf=new TPcCardFunction(anOffset,aMemType);
	if (cf)
		{
		r=iCardFuncArray.Append(cf);
		if (r!=KErrNone)
			delete cf;
		}
	return r;
	}

TPcCardFunction *DPcCardSocket::CardFunc(TInt aCardFunc)
// 
// Get a reference to a specific card function from the function array 
//
	{
												   
	__ASSERT_ALWAYS(IsValidCardFunc(aCardFunc),PcCardPanic(EPcCardBadFunctionNumber));
	return iCardFuncArray[aCardFunc];
	}

TBool DPcCardSocket::IsConfigLocked()
//
// Returns ETrue if this socket contains a card function which is currently configured.
//
	{
//	TInt i;
//	for (i=CardFuncCount()-1;i>=0;i--)
//		{
//		if (iCardFuncArray[i]->IsConfigured())
//			return(ETrue);
//		}
//	return(EFalse);
	return (iActiveConfigs!=0);
	}

TBool DPcCardSocket::IsMemoryLocked()
//
// Returns ETrue if any PC Card memory chunks are allocated on this socket.
//
	{

//	TInt i;
//	for (i=iMemChunks.Count()-1;i>=0;i--)
//		{
//		if ( iMemChunks[i]->IsLocked() )
//			return(ETrue);
//		}
//	return(EFalse);
	return (iClientWindows!=0);
	}

TPccdSocketVcc DPcCardSocket::VccSetting()
//
// Return voltage setting that this socket is currently set for
//
	{

	return ((DPcCardVcc*)iVcc)->VoltageSetting();
	}

EXPORT_C TInt DPcCardSocket::VerifyCard(TPccdType &aType)
//
// Return information about the type of card present 
//
	{

	__KTRACE_OPT(KPBUS1,Kern::Printf(">Cntrl:VerifyCard(S:%d)",iSocketNumber));
	// The data we want is stored off-card, so it doesn't actually need to be
	// powered but we need to have read CIS format.
	TInt err=KErrNone;
    if (CardIsReadyAndVerified()==KErrNone)
		{
		aType.iFuncCount=CardFuncCount();
		for (TInt i=(aType.iFuncCount-1);i>=0;i--)
			aType.iFuncType[i]=CardFunc(i)->FuncType();
		}
	else
		err=KErrNotReady;

	__KTRACE_OPT(KPBUS1,Kern::Printf("<Cntrl:VerifyCard(S:%d T:%d)-%d",iSocketNumber,(TInt)aType.iFuncType[0],err));
	return(err);
	}

TInt DPcCardSocket::CardIsReadyAndVerified()
//
// Returns KErrNone when specified card is powered and ready (ie has had h/w reset) and
// a basic parsing of CIS has been performed (card functions detected).
//
	{

	TInt r=KErrNotReady;
	if (CardIsReady())
		{
		r=KErrNone;
		// Check if card function(s) have been determined (there is always at
		// least a global function record if basic parsing performed).
		if (!IsVerified())
			r=GetCisFormat();
		}

	__KTRACE_OPT(KPBUS1,Kern::Printf("<Cntrl:CardRdyAndVerif(S:%d)-%xH",iSocketNumber,r));
	return r;
	}

TBool DPcCardSocket::CardIsReady()
	{
	TBool r=(iState==EPBusOn && Kern::PowerGood());
	__KTRACE_OPT(KPBUS1,Kern::Printf("CardIsReady: %d",r));
	return r;
	}

TBool DPcCardSocket::CardIsPowered()
	{
	return !iVcc->IsOff();
	}

TInt DPcCardSocket::GetCisFormat()
//
// Determine the type of card present by parsing the entire CIS. If a 
// Multi-function card is present then parse each CIS.
//
	{

	__KTRACE_OPT(KPBUS1,Kern::Printf(">GetCisFormat (S:%d)",iSocketNumber));

	TInt r=AddNewFunc(0,EPccdAttribMem);		// We always have 1st CIS
	if (r!=KErrNone)
		return r;
	if (ValidateCis(0)!=KErrNone)				// Can't use this until func added
		return KErrCorrupt;
	TCisReader cisRd;
	cisRd.iSocket=this;
	cisRd.DoSelectCis(0);
	TPccdFuncType firstFuncType;
	// Check for a multi-function card, search the global CIS (attribute 
	// memory - addr 0) for a KCisTplLongLinkMfc tuple.
	TBuf8<KLargeTplBufSize> tpl;
	if (cisRd.DoFindReadTuple(KCisTplLongLinkMfc,tpl,KPccdReturnLinkTpl)==KErrNone)
		{
		// Multi-Function card 
		firstFuncType=EGlobalCard;
		const TUint8 *tplPtr=tpl.Ptr()+2; // First tuple after link
		TInt funcCount=*tplPtr++;

		// Add a card function object to the socket for each entry in KCisTplLongLinkMfc tuple
		TPccdMemType memType;
		TUint32 lnkAdr;
		TInt i;
		for (i=0;i<funcCount;i++)
			{
			memType=(*tplPtr++)?EPccdCommon8Mem:EPccdAttribMem;
			TInt j;
			for (lnkAdr=0,j=0;j<4;j++)		// Convert link address from string to unsigned long
				lnkAdr += (*tplPtr++) << (8*j);
			r=AddNewFunc(lnkAdr,memType);
			if (r!=KErrNone)
				return r;
			if (ValidateCis(i+1)!=KErrNone) // Can't use this until func added
				return KErrCorrupt;
			}
		// Parse the CIS of each card function looking for a KCisTplFuncId tuple
		for (i=1;i<=funcCount;i++)
			{
			cisRd.DoSelectCis(i);
			TPccdFuncType ft;
			if (cisRd.DoFindReadTuple(KCisTplFuncId,tpl,0)==KErrNone)
				ft=FuncType(tpl[2]);
			else
				ft=EUnknownCard;
			CardFunc(i)->SetFuncType(ft);
			}
		}
	else
		{
		// Single Function card 
		cisRd.Restart();
		if (cisRd.DoFindReadTuple(KCisTplFuncId,tpl,0)==KErrNone)
			firstFuncType=FuncType(tpl[2]); 
		else
			firstFuncType=EUnknownCard; 
		}

	CardFunc(0)->SetFuncType(firstFuncType);
	__KTRACE_OPT(KPBUS1,Kern::Printf("<GetCisFormat(T:%d)",firstFuncType));
	return KErrNone;
	}

TInt DPcCardSocket::ValidateCis(TInt aCardFunc)
//
// Attempt to walk though entire CIS.
//
	{

	TCisReader cisRd;
	cisRd.iSocket=this;
	TBuf8<KLargeTplBufSize> tpl;
	TInt j=0,err;
	if ((err=cisRd.DoSelectCis(aCardFunc))==KErrNone)
		{
		for (j=0;j<KMaxTuplesPerCis;j++)
			{
			err=cisRd.DoFindReadTuple(KPccdNonSpecificTpl,tpl,(KPccdFindOnly|KPccdReturnLinkTpl|KPccdReportErrors));
			if (err!=KErrNone)
				break;
			}
		if (j>=KMaxTuplesPerCis)
			err=KErrCorrupt;
		if (err==KErrNotFound)
			err=KErrNone;
		}
	__KTRACE_OPT(KPBUS1,Kern::Printf("<Skt:ValidateCis(S:%d F:%d Tuples:%d)-%d",iSocketNumber,aCardFunc,j,err));
	return(err);
	}

void DPcCardSocket::InitiatePowerUpSequence()
	{
	__KTRACE_OPT(KPBUS1,Kern::Printf("DPcCardSocket(%d)::InitiatePowerUpSequence",iSocketNumber));
	// Check if battery is too low
//	TSupplyStatus ss=(TSupplyStatus)iMachineInfo.iDisableOnLowBattery;
//	if (ss!=EZero)
//		{
//		TSupplyInfoV1 info;
//		Hal::SupplyInfo(info); 
//		if (info.iMainBatteryStatus<ss && !info.iExternalPowerPresent)
//			{
//			iSocket[aSocket]->SetSocketStatus(ESocketBatTooLow);
//			rs=KErrBadPower;
//			break;
//			}
//		}

	// Check the state of the Voltage sense line
	TSocketIndicators ind;
	Indicators(ind);
	TUint v=(TUint)ind.iVoltSense & ((DPcCardVcc*)iVcc)->VoltageSupported();
	if (v==0)
		{
		__KTRACE_OPT(KPBUS1,Kern::Printf("InitiatePowerUpSequence(S:%d)-Voltage sense problem(%d)",iSocketNumber,ind.iVoltSense));
		iVcc->SetCurrLimited();   // Not totally true but has effect.
		PowerUpSequenceComplete(KErrCorrupt);
		return;
		}
	TPccdSocketVcc sVcc=(v&KPccdVcc_3V3)?EPccdSocket_3V3:EPccdSocket_5V0; // ??? What about xVx / yVy 
	((DPcCardVcc*)iVcc)->SetVoltage(sVcc);

	// Power up card (current limited).
	__KTRACE_OPT(KPBUS1,Kern::Printf("InitiatePowerUpSequence(S:%d)-Apply Vcc",iSocketNumber));
	if (iVcc->SetState(EPsuOnCurLimit) != KErrNone)
		{
		__KTRACE_OPT(KPBUS1,Kern::Printf("InitiatePowerUpSequence(S:%d)-Vcc problem",iSocketNumber));
		iVcc->SetState(EPsuOff);
		iVcc->SetCurrLimited();
		PowerUpSequenceComplete(KErrGeneral);
		return;
		}
	iCardPowerUpState=EInit;
	iCardPowerUpTickCount=0;
	iCardPowerUpResetLen=KResetOnDefaultLen;
	iCardPowerUpPauseLen=KResetOffDefaultLen;
	iCardPowerUpTimer.Periodic(KPccdPowerUpReqInterval,cardPowerUpTick,this);
	}

void DPcCardSocket::TerminatePowerUpSequence(TInt aResult)
	{
	__KTRACE_OPT(KPBUS1,Kern::Printf("DPcCardSocket(%d)::TerminatePowerUpSequence result %d",iSocketNumber,aResult));
	ResetPowerUpState();
	if (aResult==KErrNone)
		Restore();
	PowerUpSequenceComplete(aResult);
	}

void DPcCardSocket::CardPowerUpTick()
	{
	__KTRACE_OPT(KPBUS1,Kern::Printf("CardPowerUpTick S:%d Elapsed %d State %d",iSocketNumber,iCardPowerUpTickCount,iCardPowerUpState));
	if (++iCardPowerUpTickCount>KPwrUpTimeOut)
		{
		iVcc->SetState(EPsuOff);	// should leave this to timeout
		TerminatePowerUpSequence(KErrTimedOut);
		return;
		}
	switch (iCardPowerUpState)
		{
		case EInit:
			HwReset(ETrue);		// Apply reset - Turns on interface
			iCardPowerUpState=EApplyingReset;
			break;
		case EApplyingReset:
			if (iCardPowerUpTickCount>iCardPowerUpResetLen)
				{
				HwReset(EFalse);	// remove reset
				iCardPowerUpState=ECheckVcc;
				}
			break;
		case ECheckVcc:
			{
			iCardPowerUpState=EWaitForVccReading;
			TInt cv=iVcc->CheckVoltage(KPsuChkOnPwrUp);
			if (cv==KErrNotSupported)
				iCardPowerUpState=EWaitForReady;
			else if (cv!=KErrNone)
				TerminatePowerUpSequence(cv);
			break;
			}
		case EWaitForVccReading:
			break;
		case EWaitForReady:
			if (Ready())
				{
				iCardPowerUpState=EPauseAfterReady; // Card is ready
				// Its effectively powered up now so reset the elapsed time and use it 
				// to measure pause after reset (ie this is limited to KPwrUpTimeOut too).
				iCardPowerUpTickCount=0;
				}
			break;
		case EPauseAfterReady:
			if (iCardPowerUpTickCount>=iCardPowerUpPauseLen)
				{
				// power-up sequence is complete
				TerminatePowerUpSequence(KErrNone);
				}
			break;
		}
	}

/********************************************
 * PC card memory chunk
 ********************************************/
DPccdChunkBase::DPccdChunkBase()
//
// Constructor
//
	{
//	iSocket=NULL;
//	iCacheable=EFalse;
	}

TInt DPccdChunkBase::Create(DPcCardSocket* aSocket, TPccdChnk aChunk, TUint aFlag)
//
// Create a chunk of Pc Card h/w.
//
	{
	iSocket=aSocket;
	iChnk=aChunk;
	iCacheable=(aFlag&KPccdChunkCacheable);
	return DoCreate(aChunk,aFlag);
	}

DPccdChunkBase::~DPccdChunkBase()
//
// Destructor
//
	{
	__KTRACE_OPT(KPBUS1,Kern::Printf(">DPccdChunkBase destruct %08x",this));
	}

void DPccdChunkBase::Close()
//
// Destructor
//
	{
	__KTRACE_OPT(KPBUS1,Kern::Printf(">DPccdChunkBase::Close() %08x",this));

	// Disconnect all the Pc Card windows and then delete chunk
	SDblQueLink* pW=iWindowQ.iA.iNext;
	while (pW!=&iWindowQ.iA)
		{
		RPccdWindow& w=*(RPccdWindow*)pW;
		pW=pW->iNext;
		w.Close();	// closing last window deletes chunk
		}
	__KTRACE_OPT(KPBUS1,Kern::Printf("<DPccdChunkBase::Close() %08x",this));
	}

TBool DPccdChunkBase::IsRemovable()
//
// Check if this chunk has any permanent windows.
//
	{
	return (iPermanentWindows==0);
	}

TBool DPccdChunkBase::IsLocked()
//
// Check if this chunk has any windows which are allocated to clients of the PC Card
// Controller (as opposed to the Controller itself).
//
	{
	return (iWindows>iSystemWindows);
	}

TInt DPccdChunkBase::AllocateWinCheck(TPccdChnk aWin,TUint aFlag)
//
// Check if it is possible to create the specified window from this chunk.
//
	{
	// Check if they are of compatible type
	if (!IsTypeCompatible(aWin.iMemType))
		return(KErrNotFound);

	// For a success, the requested window must lie entirely within this chunk.
	TUint32 chnkEnd=(iChnk.iMemBaseAddr+iChnk.iMemLen-1);
	TUint32 winEnd=(aWin.iMemBaseAddr+aWin.iMemLen-1);
	TBool startIsInChnk=(aWin.iMemBaseAddr>=iChnk.iMemBaseAddr && aWin.iMemBaseAddr<=chnkEnd);
	TBool endIsInChnk=(winEnd>=iChnk.iMemBaseAddr && winEnd<=chnkEnd);
	if (startIsInChnk&&endIsInChnk)
		{
		// Possible success - first check the cache options are compatible
		if (!(aFlag|KPccdChunkCacheable)&&iCacheable)
			return(KErrAccessDenied);

		// Now check that the requested window isn't already allocated
		SDblQueLink* pW=iWindowQ.iA.iNext;
		while (pW!=&iWindowQ.iA)
			{
			RPccdWindow& w=*(RPccdWindow*)pW;
			pW=pW->iNext;
			if (w.Overlap(aWin.iMemBaseAddr-iChnk.iMemBaseAddr,aWin.iMemLen) )
				return(KErrAccessDenied);
			}
		return(KErrNone);
		}
	if (startIsInChnk||endIsInChnk)
		return(KErrAccessDenied);	// Requested window is partly in this chunk.
	return(KErrNotFound);
	}

void DPccdChunkBase::AddWindow(RPccdWindow *aWindow)
//
// Add a window to this chunk.
//
	{
	iWindowQ.Add(aWindow);
//	Kern::EnterCS();		Not needed since a single thread is used
	iWindows++;
	if (aWindow->IsPermanent())
		iPermanentWindows++;
	if (aWindow->IsShareable())
		iShareableWindows++;
	if (aWindow->IsSystemOwned())
		iSystemWindows++;
	else
		iSocket->iClientWindows++;
//	Kern::LeaveCS();
	aWindow->iChunk=this;
	}

void DPccdChunkBase::RemoveWindow(RPccdWindow *aWindow)
//
// Remove a window from this chunk (even if it's permanent). 
//	
	{

	if (aWindow->iNext && aWindow->iChunk==this)
		{
		aWindow->Deque();
		aWindow->iNext=NULL;
//		Kern::EnterCS();	Not needed since a single thread is used
		iWindows--;
		if (aWindow->IsPermanent())
			iPermanentWindows--;
		if (aWindow->IsShareable())
			iShareableWindows--;
		if (aWindow->IsSystemOwned())
			iSystemWindows--;
		else
			iSocket->iClientWindows--;
//		Kern::LeaveCS();
		if (iWindows==0)
			{
			iSocket->RemoveChunk(this);
			delete this;
			}
		}
	}

/********************************************
 * PC card memory window
 ********************************************/
EXPORT_C RPccdWindow::RPccdWindow()
//
// Constructor
//
	: iAccessSpeed(EAcSpeedInValid),iMemType(EPccdAttribMem),iOffset(0),iLen(0),iType(0)
	{
	iNext=NULL;
	iChunk=NULL;
	}

EXPORT_C TInt RPccdWindow::Create(DPcCardSocket* aSocket, TPccdChnk aChnk, TPccdAccessSpeed aSpeed, TUint aFlag)
//
// Create a block of memory (IO, Common or Attribute memory).
//
	{

	DPccdChunkBase *chunk=NULL;
	TBool chunkExists=EFalse;
	TInt r;

	// See if requested window is actually part of a chunk already created
	TInt i;
	for (i=0;i<aSocket->iMemChunks.Count();i++)
		{
		if ((r=aSocket->iMemChunks[i]->AllocateWinCheck(aChnk,aFlag))==KErrNone)
			{
			chunk=aSocket->iMemChunks[i];
			chunkExists=ETrue;
			break;
			}
		if (r==KErrAccessDenied)
			return r;
		}

	// If necesary, create a chunk
	if (!chunkExists)
		{
		// Create the memory chunk
		chunk=aSocket->NewPccdChunk(aChnk.iMemType);
		if (!chunk)
			return KErrNoMemory;
		TInt r=chunk->Create(aSocket, aChnk, aFlag);
		if (r==KErrNone)
			r=aSocket->iMemChunks.Append(chunk);
		if (r!=KErrNone)
			{
			delete chunk;
			return r;
			}
		}
	__KTRACE_OPT(KPBUS2,Kern::Printf("Skt:CreateMemWindowL-got chunk(existing-%d)",chunkExists));

	// Create the memory window
	iOffset=aChnk.iMemBaseAddr-chunk->BaseAddr();
	iLen=aChnk.iMemLen;
	iAccessSpeed=aSpeed;
	iMemType=aChnk.iMemType;
	iWaitSig=(aFlag&KPccdRequestWait);
	iType=aFlag&(KPccdChunkShared|KPccdChunkPermanent|KPccdChunkSystemOwned); // Save flag settings
	chunk->AddWindow(this);
	__KTRACE_OPT(KPBUS2,Kern::Printf("Skt:CreateMemWindowL-created window"));
	return KErrNone;
	}

EXPORT_C void RPccdWindow::Close()
	{
	if (iNext && iChunk)
		iChunk->RemoveWindow(this);
	}

EXPORT_C TInt RPccdWindow::SetupChunkHw(TUint aFlag)
//
// Config h/w in preparation for accessing window. Flag is for platform dependant info.
//
	{

	if (!iChunk)
		return(KErrNotReady);
	iChunk->SetupChunkHw(iAccessSpeed,iMemType,iWaitSig,aFlag);
//	iVcc->ResetInactivityTimer();
	return(KErrNone);
	}

EXPORT_C TLinAddr RPccdWindow::LinearAddress()
//
// Return linear address of window
//
	{
	return iChunk->LinearAddress()+iOffset;
	}

TBool RPccdWindow::Overlap(TUint32 anOffset,TUint aLen)
//
//
//
	{
	// If this window is sharable then it doesn't matter if they overlap or not.
	if (IsShareable())
		return(EFalse);

	TUint32 winEnd=(anOffset+aLen-1);
	TUint32 thisEnd=(iOffset+iLen-1);
	if ((anOffset>=iOffset && anOffset<=thisEnd) ||
		(winEnd>=iOffset && winEnd<=thisEnd) )
		return(ETrue);

	return(EFalse);
	}

