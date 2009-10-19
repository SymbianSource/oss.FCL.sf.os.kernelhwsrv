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
// e32\drivers\pbus\pccard\epoc\pccd_socket.cpp
// 
//

#include <pccd_socket.h>

DPlatPcCardSocket::DPlatPcCardSocket(TSocket aSocketNum)
//
// Constructor.
//
	: DPcCardSocket(aSocketNum)
	{
	}

TInt DPlatPcCardSocket::Create(const TDesC* aName)
//
// Allocate any resources.
//
	{

	__KTRACE_OPT(KPBUS1,Kern::Printf(">PlatSkt:Create"));
	TInt r=DPcCardSocket::Create(aName);
	if (r!=KErrNone)
		return r;

	// Bind to the PC card interrupts for this socket
	iCardIReqIntId=ThePccdCntrlInterface->IntIdIReq(iSocketNumber);
	iStatusChangeIntId=ThePccdCntrlInterface->IntIdStsC(iSocketNumber);
	iReadyChangeIntId=ThePccdCntrlInterface->IntIdRdyC(iSocketNumber);
	if (iCardIReqIntId!=-1)
		{
		r=Interrupt::Bind(iCardIReqIntId,CardIReqIsr,this);
		if (r!=KErrNone)
			return r;
		}
	if (iStatusChangeIntId!=-1)
		{
		r=Interrupt::Bind(iStatusChangeIntId,StatusChangeIsr,this);
		if (r!=KErrNone)
			return r;
		}
	if (iReadyChangeIntId!=-1)
		{
		r=Interrupt::Bind(iReadyChangeIntId,ReadyChangeIsr,this);
		if (r!=KErrNone)
			return r;
		}
	return KErrNone;
	}

void DPlatPcCardSocket::Reset1()
//
// Reset the socket
//
	{

	__KTRACE_OPT(KPBUS1,Kern::Printf(">PlatSkt(%d):Rst1",iSocketNumber));
	if (iCardIReqIntId!=-1)
		Interrupt::Disable(iCardIReqIntId);
	ThePccdCntrlInterface->InterfaceOff(iSocketNumber);
	DPcCardSocket::Reset1();
	}

DPccdChunkBase *DPlatPcCardSocket::NewPccdChunk(TPccdMemType aType)
//
// Create a new pc card hw chunk.
//
	{
	return PccdIfc::NewChunk(aType);
	}

TInt DPlatPcCardSocket::Indicators(TSocketIndicators &anInd)
//
// Return the current state of this sockets indicators
//
	{

	ThePccdCntrlInterface->Indicators(iSocketNumber,anInd);
	__KTRACE_OPT(KPBUS1,Kern::Printf("<PlatSkt:Indicators(C:%dV:%dW:%dB:%d)",anInd.iCardDetected,anInd.iVoltSense,anInd.iBatState,anInd.iWriteProtected));
	return(KErrNone) ;
	}

void DPlatPcCardSocket::HwReset(TBool anAssert)
//
// Apply/remove h/w reset.
//
	{

	__KTRACE_OPT(KPBUS1,Kern::Printf(">PlatSkt:HwRst(%d)",anAssert));
    ThePccdCntrlInterface->CardReset(iSocketNumber,anAssert);
	}

TBool DPlatPcCardSocket::Ready(TInt aCardFunc)
//
// Return the current state of the Card Ready signal. When a function
// isn't specified then it always reads the Ready pin.
//
	{
	// If a function has been specified and this is in I/O mode then read from
	// the configuration register (Pin replacement register).
//	if (aCardFunc!=KInvalidFuncNum)
//		{
//		if ( iStatus==ESocketReady && CardFunc(aCardFunc)->IsConfigured() )
//			{
//			TUint8 reg;
//			if ( ReadConfigReg(aCardFunc,KPinReplacementReg,reg) != KErrNone)
//				return(EFalse);
//			else
//				return(reg&KPinRepReadyM);
//			}
//		}
	TBool rdy=ThePccdCntrlInterface->CardReady(iSocketNumber);
	__KTRACE_OPT(KPBUS1,Kern::Printf("<PlatSkt:Rdy-%d",(TInt)rdy));
	return(rdy);
	}

TInt DPlatPcCardSocket::InterruptEnable(TPccdInt anInt, TUint aFlag)
//
// Enable the specified interrupt
//
	{

	TInt err=KErrNone;
	switch(anInt)
		{
		case EPccdIntIReq:
			{
			if (iCardIReqIntId==-1)
				break;
			if (aFlag&KPccdEvFlagReserved)
				iIReqLevelMode=(aFlag&KPccdEvFlagIReqLevelMode);
			TInt irqLevel=NKern::DisableAllInterrupts(); // Disable FIQs while we enable IREQ.
			// IREQ must never be active when Vpc off. Need to check that emergency
			// power down hasn't just happened since this removes Vpc without altering iStatus.
			if (iState==EPBusOn && Kern::PowerGood())
				Interrupt::Enable(iCardIReqIntId);
			else
				err=KErrGeneral;
			NKern::RestoreInterrupts(irqLevel);
			break;
			}
		case EPccdIntRdyChange:
			if (iReadyChangeIntId!=-1)
				Interrupt::Enable(iReadyChangeIntId);
			break;
		case EPccdIntIndChange:
			if (iStatusChangeIntId!=-1)
				Interrupt::Enable(iStatusChangeIntId);
			break;
		default:
			break;
		}
	return(err);
	}

void DPlatPcCardSocket::InterruptDisable(TPccdInt anInt)
//
// Disable the specified event callback
//
	{

	switch(anInt)
		{
		case EPccdIntIReq:
			if (iCardIReqIntId!=-1)
				Interrupt::Disable(iCardIReqIntId);
			break;
		case EPccdIntRdyChange:
			if (iReadyChangeIntId!=-1)
				Interrupt::Disable(iReadyChangeIntId);
			break;
		case EPccdIntIndChange:
			if (iStatusChangeIntId!=-1)
				Interrupt::Disable(iStatusChangeIntId);
			break;
		default:
			break;
		}
	}


void DPlatPcCardSocket::SocketInfo(TPcCardSocketInfo& anInfo)
//
// Return machine info relating to a particular Pc Card socket 
//
	{

	ThePccdCntrlInterface->SocketInfo(iSocketNumber,anInfo);
	}

TBool DPlatPcCardSocket::CardIsPresent()
	{

	return ThePccdCntrlInterface->CardIsPresent(iSocketNumber);
	}

void DPlatPcCardSocket::CardIReqIsr(TAny* aPtr)
	{
	DPlatPcCardSocket* pS=(DPlatPcCardSocket*)aPtr;
	if (pS->iIReqLevelMode)
		{
		pS->Isr(EPccdIntIReq);
		Interrupt::Clear(pS->iCardIReqIntId);
		}
	else
		{
		Interrupt::Clear(pS->iCardIReqIntId);
		pS->Isr(EPccdIntIReq);
		}
	}

void DPlatPcCardSocket::ReadyChangeIsr(TAny* aPtr)
	{
	DPlatPcCardSocket* pS=(DPlatPcCardSocket*)aPtr;
	Interrupt::Clear(pS->iReadyChangeIntId);
	pS->Isr(EPccdIntRdyChange);
	}

void DPlatPcCardSocket::StatusChangeIsr(TAny* aPtr)
	{
	DPlatPcCardSocket* pS=(DPlatPcCardSocket*)aPtr;
	Interrupt::Clear(pS->iStatusChangeIntId);
	pS->Isr(EPccdIntIndChange);
	}

