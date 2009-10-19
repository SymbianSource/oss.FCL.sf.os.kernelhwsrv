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
// e32\drivers\ecomm\uart16550.cpp
// PDD for 16550 UART
// 
//

#include <drivers/comm.h>
#include <assp.h>
#include <var_defs.h>
#include <uart16550.h>
#include <e32hal.h>

_LIT(KPddName,"Comm.16550");

#define __COMMS_MACHINE_CODED__
#ifdef __COMMS_MACHINE_CODED__
#define DBASE_VPTR_OFFSET	4
#define RX_ISR_VT_OFFSET	0x24
#define CHK_TXB_VT_OFFSET	0x28
#define STATE_ISR_VT_OFFSET	0x2C
#endif

// needs ldd version..
const TInt KMinimumLddMajorVersion=1;
const TInt KMinimumLddMinorVersion=1;
const TInt KMinimumLddBuild=122;

// configuration data
static const TUint16 BaudRateDivisor[19] =
	{
	2304,	1536,	1047,	860,	768,	384,	192,	96,
	64,		58,		48,		32,		24,		16,		12,		6,
	3,		2,		1
	};

class DDriverComm : public DPhysicalDevice
	{
public:
	DDriverComm();
	virtual TInt Install();
	virtual void GetCaps(TDes8 &aDes) const;
	virtual TInt Create(DBase*& aChannel, TInt aUnit, const TDesC8* anInfo, const TVersion &aVer);
	virtual TInt Validate(TInt aUnit, const TDesC8* anInfo, const TVersion &aVer);
public:
	TDynamicDfcQue* iDfcQ;
	};

class DComm16550 : public DComm
	{
public:
	DComm16550();
	~DComm16550();
	TInt DoCreate(TInt aUnit, const TDesC8* anInfo);
public:
	virtual TInt Start();
	virtual void Stop(TStopMode aMode);
	virtual void Break(TBool aState);
	virtual void EnableTransmit();
	virtual TUint Signals() const;
	virtual void SetSignals(TUint aSetMask,TUint aClearMask);
	virtual TInt ValidateConfig(const TCommConfigV01 &aConfig) const;
	virtual void Configure(TCommConfigV01 &aConfig);
	virtual void Caps(TDes8 &aCaps) const;
	virtual TInt DisableIrqs();
	virtual void RestoreIrqs(TInt aIrq);
	virtual TDfcQue* DfcQ(TInt aUnit);
	virtual void CheckConfig(TCommConfigV01& aConfig);
public:
	static void Isr(TAny* aPtr);
public:
	TInt iInterruptId;
	TInt iUnit;
	T16550Uart* iUart;
	};


DDriverComm::DDriverComm()
//
// Constructor
//
	{
	iUnitsMask=~(0xffffffffu<<KNum16550Uarts);
	iVersion=TVersion(KCommsMajorVersionNumber,KCommsMinorVersionNumber,KCommsBuildVersionNumber);
	}


TInt DDriverComm::Install()
//
// Install the driver
//
	{
	// Allocate a kernel thread to run the DFC 
	TInt r = Kern::DynamicDfcQCreate(iDfcQ, KUart16550ThreadPriority, KUar16550tDriverThread);
	if (r == KErrNone)
		{
		iDfcQ->SetRealtimeState(ERealtimeStateOff);  
		r = SetName(&KPddName);
		}
	return r;
	}

/**
  Destructor
*/
DDriverComm::~DDriverComm()
	{
	if (iDfcQ)
		iDfcQ->Destroy();
	}

void Get16550CommsCaps(TDes8& aCaps, TInt aUnit)
	{
	TCommCaps3 capsBuf;
	TCommCapsV03 &c=capsBuf();
	c.iRate=KCapsBps110|KCapsBps150|KCapsBps300|KCapsBps600\
		|KCapsBps1200|KCapsBps2400|KCapsBps4800|KCapsBps9600\
		|KCapsBps19200|KCapsBps38400|KCapsBps57600|KCapsBps115200;
	c.iDataBits=KCapsData5|KCapsData6|KCapsData7|KCapsData8;
	c.iStopBits=KCapsStop1|KCapsStop2;
	c.iParity=KCapsParityNone|KCapsParityEven|KCapsParityOdd|KCapsParityMark|KCapsParitySpace;
	c.iHandshake=KCapsObeyXoffSupported|KCapsSendXoffSupported|
					KCapsObeyCTSSupported|KCapsFailCTSSupported|
					KCapsObeyDSRSupported|KCapsFailDSRSupported|
					KCapsObeyDCDSupported|KCapsFailDCDSupported|
					KCapsFreeRTSSupported|KCapsFreeDTRSupported;
	c.iSIR=0;
	c.iSignals=KCapsSignalCTSSupported|KCapsSignalRTSSupported|KCapsSignalDTRSupported|
						KCapsSignalDSRSupported|KCapsSignalDCDSupported|KCapsSignalRNGSupported;
	c.iFifo=KCapsHasFifo;
	c.iNotificationCaps=KNotifyDataAvailableSupported|KNotifySignalsChangeSupported;
	c.iRoleCaps=0;
	c.iFlowControlCaps=0;
	c.iBreakSupported=ETrue;
	aCaps.FillZ(aCaps.MaxLength());
	aCaps=capsBuf.Left(Min(capsBuf.Length(),aCaps.MaxLength()));
	}

void DDriverComm::GetCaps(TDes8 &aDes) const
//
// Return the drivers capabilities
//
	{
	Get16550CommsCaps(aDes, 0);
	}

TInt DDriverComm::Create(DBase*& aChannel, TInt aUnit, const TDesC8* anInfo, const TVersion& aVer)
//
// Create a driver
//
	{
	DComm16550* pD=new DComm16550;
	aChannel=pD;
	TInt r=KErrNoMemory;
	if (pD)
		r=pD->DoCreate(aUnit,anInfo);
	return r;
	}

TInt DDriverComm::Validate(TInt aUnit, const TDesC8* /*anInfo*/, const TVersion& aVer)
//
//	Validate the requested configuration
//
	{
	if ((!Kern::QueryVersionSupported(iVersion,aVer)) || (!Kern::QueryVersionSupported(aVer,TVersion(KMinimumLddMajorVersion,KMinimumLddMinorVersion,KMinimumLddBuild))))
		return KErrNotSupported;
	if (aUnit<0 || aUnit>=KNum16550Uarts)
		return KErrNotSupported;
	return KErrNone;
	}

DComm16550::DComm16550()
//
// Constructor
//
	{
//	iTransmitting=EFalse;
	iInterruptId=-1;		// -1 means not bound
	}

DComm16550::~DComm16550()
//
// Destructor
//
	{
	if (iInterruptId>=0)
		Interrupt::Unbind(iInterruptId);
	}

TInt DComm16550::DoCreate(TInt aUnit, const TDesC8* /*anInfo*/)
//
// Sets up the PDD
//
	{
	iUnit=aUnit;
	TInt irq=IrqFromUnit(aUnit);

	// bind to UART interrupt
	TInt r=Interrupt::Bind(irq,Isr,this);
	if (r==KErrNone)
		{
		iInterruptId=irq;
		iUart=UartFromUnit(aUnit);
		iUart->SetIER(0);
		iUart->SetLCR(0);
		iUart->SetFCR(0);
		iUart->SetMCR(0);
		}
	return r;
	}

TDfcQue* DComm16550::DfcQ(TInt /*aUnit*/)
//
// Return the DFC queue to be used for this device
// For PC cards, use the PC card controller thread for the socket in question.
//

	{
	return iDfcQ;
	}

TInt DComm16550::Start()
//
// Start receiving characters
//
	{
	// if EnableTransmit() called before Start()
	iTransmitting=EFalse;
	iUart->SetIER(K16550IER_RDAI|K16550IER_RLSI|K16550IER_MSI);
	iLdd->UpdateSignals(Signals());
	Interrupt::Enable(iInterruptId);
	return KErrNone;
	}

TBool FinishedTransmitting(TAny* aPtr)
	{
	DComm16550& d=*(DComm16550*)aPtr;
	return d.iUart->TestLSR(K16550LSR_TxIdle);
	}

void DComm16550::Stop(TStopMode aMode)
//
// Stop receiving characters
//
	{
	switch (aMode)
		{
		case EStopNormal:
		case EStopPwrDown:
			iUart->SetIER(0);
			Interrupt::Disable(iInterruptId);
			iTransmitting=EFalse;

			// wait for uart to stop tranmitting
			Kern::PollingWait(FinishedTransmitting,this,3,100);
			break;
		case  EStopEmergency:
			iUart->SetIER(0);
			Interrupt::Disable(iInterruptId);
			iTransmitting=EFalse;
			break;
		}
	}

void DComm16550::Break(TBool aState)
//
// Start or stop the uart breaking
//
	{
	if (aState)
		iUart->ModifyLCR(0,K16550LCR_TxBreak);
	else
		iUart->ModifyLCR(K16550LCR_TxBreak,0);
	}

void DComm16550::EnableTransmit()
//
// Start sending characters.
//
	{
	TBool tx = (TBool)__e32_atomic_swp_ord32(&iTransmitting, 1);
	if (tx)
		return;
	iUart->ModifyIER(0,K16550IER_THREI);
	}

TUint DComm16550::Signals() const
//
// Read and translate the modem lines
//
	{
	TUint msr=iUart->MSR();
	msr=((msr>>4)&0x0f);			// true input signals
	TUint sig=msr & 3;				// CTS,DSR OK
	if (msr & 4)
		sig|=KSignalRNG;			// swap DCD,RNG
	if (msr & 8)
		sig|=KSignalDCD;
	return sig;
	}

void DComm16550::SetSignals(TUint aSetMask, TUint aClearMask)
//
// Set signals.
//
	{
	TUint set=0;
	TUint clear=0;
	if (aSetMask & KSignalRTS)
		set|=K16550MCR_RTS;
	if (aSetMask & KSignalDTR)
		set|=K16550MCR_DTR;
	if (aClearMask & KSignalRTS)
		clear|=K16550MCR_RTS;
	if (aClearMask & KSignalDTR)
		clear|=K16550MCR_DTR;
	iUart->ModifyMCR(clear,set);
	}

TInt DComm16550::ValidateConfig(const TCommConfigV01 &aConfig) const
//
// Check a config structure.
//
	{
	if (aConfig.iSIREnable==ESIREnable)
		return KErrNotSupported;
	switch (aConfig.iParity)
		{
		case EParityNone:
		case EParityOdd:
		case EParityEven:
		case EParityMark:
		case EParitySpace:
			break;
		default:
			return KErrNotSupported;
		}
	if (TUint(aConfig.iRate)>TUint(EBps115200))
		return KErrNotSupported;
	return KErrNone;
	}

void DComm16550::CheckConfig(TCommConfigV01& aConfig)
	{
	// do nothing
	}

TInt DComm16550::DisableIrqs()
//
// Disable normal interrupts
//
	{
	
	return NKern::DisableInterrupts(1);
	}

void DComm16550::RestoreIrqs(TInt aLevel)
//
// Restore normal interrupts
//
	{
	
	NKern::RestoreInterrupts(aLevel);
	}

void DComm16550::Configure(TCommConfigV01 &aConfig)
//
// Set up the Uart
//
	{
	// wait for uart to stop tranmitting
	Kern::PollingWait(FinishedTransmitting,this,3,100);

	TUint lcr=0;
	switch (aConfig.iDataBits)
		{
		case EData5: lcr=K16550LCR_Data5; break;
		case EData6: lcr=K16550LCR_Data6; break;
		case EData7: lcr=K16550LCR_Data7; break;
		case EData8: lcr=K16550LCR_Data8; break;
		}
	switch (aConfig.iStopBits)
		{
		case EStop1: break;
		case EStop2: lcr|=K16550LCR_Stop2; break;
		}
	switch (aConfig.iParity)
		{
		case EParityNone: break;
		case EParityEven: lcr|=K16550LCR_ParityEnable|K16550LCR_ParityEven; break;
		case EParityOdd: lcr|=K16550LCR_ParityEnable; break;
		case EParityMark: lcr|=K16550LCR_ParityEnable|K16550LCR_ParityMark; break;
		case EParitySpace: lcr|=K16550LCR_ParityEnable|K16550LCR_ParitySpace; break;
		}
	iUart->SetLCR(lcr|K16550LCR_DLAB);
	iUart->SetBaudRateDivisor(BaudRateDivisor[(TInt)aConfig.iRate]);
	iUart->SetLCR(lcr);
	iUart->SetFCR(K16550FCR_Enable|K16550FCR_RxReset|K16550FCR_TxReset|K16550FCR_RxTrig8);
	}

void DComm16550::Caps(TDes8 &aCaps) const
//
// return our caps
//
	{
	Get16550CommsCaps(aCaps,iUnit);
	}

void DComm16550::Isr(TAny* aPtr)
//
// Service the UART interrupt
//
	{
	DComm16550& d=*(DComm16550*)aPtr;
	T16550Uart& u=*d.iUart;
	TUint rx[32];
	TUint xon=d.iLdd->iRxXonChar;
	TUint xoff=d.iLdd->iRxXoffChar;

	TUint isr=u.ISR();
	if (isr & K16550ISR_NotPending)
		return;
	isr&=K16550ISR_IntIdMask;

	// if receive data available or line status interrupt
	if (isr==K16550ISR_RDAI || isr==K16550ISR_RLSI)
		{
		TInt rxi=0;
		TInt x=0;
		while(u.TestLSR(K16550LSR_RxReady|K16550LSR_RxParityErr|K16550LSR_RxOverrun|K16550LSR_RxFrameErr|K16550LSR_RxBreak) && Kern::PowerGood())
			{
			TUint lsr=0;
			// checks for EIF flag
			if (isr==K16550ISR_RLSI)
				lsr=u.LSR()&(K16550LSR_RxParityErr|K16550LSR_RxOverrun|K16550LSR_RxFrameErr);
			TUint ch=u.RxData();
			// if error in this character
			if(lsr)
				{
				if (lsr & K16550LSR_RxParityErr)
					ch|=KReceiveIsrParityError;
				if (lsr & K16550LSR_RxBreak)
					ch|=KReceiveIsrBreakError;
				if (lsr & K16550LSR_RxFrameErr)
					ch|=KReceiveIsrFrameError;
				if (lsr & K16550LSR_RxOverrun)
					ch|=KReceiveIsrOverrunError;
				}
			if (ch==xon)
				x=1;
			else if (ch==xoff)
				x=-1;
			else
				rx[rxi++]=ch;
			}
		d.ReceiveIsr(rx,rxi,x);
		return;
		}
	// if TFS flag and TIE
	if (isr==K16550ISR_THREI)
		{
		TInt n;
		for (n=0; n<16; ++n)
			{
			TInt r=d.TransmitIsr();
			if(r<0)
				{
				//no more to send
				// Disable the TX interrupt
				u.ModifyIER(K16550IER_THREI,0);
				d.iTransmitting=EFalse;
				break;
				}
			u.SetTxData(r);
			}
		d.CheckTxBuffer();
		return;
		}
	// must be signal change
	d.StateIsr(d.Signals());
	}


const TInt KUart16550ThreadPriority = 27;
_LIT(KUar16550tDriverThread,"UART16550_Thread");

DECLARE_STANDARD_PDD()
	{
	return new DDriverComm;
	}

