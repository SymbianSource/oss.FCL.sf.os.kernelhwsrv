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
// template\template_variant\specific\uart.cpp
// pdd for serial ports
// assume Modem Control Signals change cause an interrupt
// 
//


#include <drivers/comm.h>
#include <template_assp.h>
#include "iolines.h"
#include <e32hal.h>

_LIT(KPddName,"Comm.Template");

// needs ldd version..
const TInt KMinimumLddMajorVersion=1;
const TInt KMinimumLddMinorVersion=1;
const TInt KMinimumLddBuild=122;

//
// TO DO: (mandatory)
//
// Define here the UART enumeration data for the serial ports.
// It is a good idea to define each enumerated value as a bit mask that can be written to (or OR-ed or AND-ed to)
// a hardware register to provide the configuration option desired (the following are EXAMPLES ONLY):
//
// EXAMPLE ONLY
enum TUartBaudRate
	{
	EUartBaudRate115200/* =bitmask for 115200 Baud */,
	EUartBaudRate76800/* =bitmask for 76800 Baud */,
	EUartBaudRate57600/* =bitmask for 57600 Baud */,
	EUartBaudRate38400/* =bitmask for 38400 Baud */,
	EUartBaudRate19200/* =bitmask for 19200 Baud */,
	EUartBaudRate14400/* =bitmask for 14400 Baud */,
	EUartBaudRate9600/* =bitmask for 9600 Baud */,
	EUartBaudRate4800/* =bitmask for 4800 Baud */,
	EUartBaudRate2400/* =bitmask for 2400 Baud */,
	EUartBaudRate1200/* =bitmask for 1200 Baud */,
	EUartBaudRate600/* =bitmask for 600 Baud */,
	EUartBaudRate300/* =bitmask for 300 Baud */,
	EUartBaudRate150/* =bitmask for 150 Baud */,
	EUartBaudRate110/* =bitmask for 110 Baud */
	};
// EXAMPLE ONLY
enum TUartBreak
	{
	EUartBreakOff/* =bitmask for turning Break Off */,
	EUartBreakOn/* =bitmask for turning Break On */
	};
// EXAMPLE ONLY
enum TUartParity
	{
	EUartParityNone/* =bitmask for no Parity */,
	EUartParityOdd/* =bitmask for odd Parity */,
	EUartParityEven/* =bitmask for even Parity */
	};
// EXAMPLE ONLY
enum TUartStopBit
	{
	EUartStopBitOne/* =bitmask for one stop bit */,
	EUartStopBitTwo/* =bitmask for two stop bits */
	};
enum TUartDataLength
	{
	EUartDataLength5/* =bitmask for five data bits */,
	EUartDataLength6/* =bitmask for six data bits */,
	EUartDataLength7/* =bitmask for seven data bits */,
	EUartDataLength8/* =bitmask for eight data bits */
	};

//Remove the #if block if this code needs to be used or referenced.
#if 0	//RVCT40 warning
//
// TO DO: (mandatory)
//
// Lookup table to convert EPOC baud rates into hardware-specific baud rate values
// Unsupported baud rates select the nearest lower rate.
//
// EXAMPLE ONLY
static const TUartBaudRate BaudRate[19] =
	{
	EUartBaudRate110,EUartBaudRate110,EUartBaudRate110,
	EUartBaudRate110,EUartBaudRate150,EUartBaudRate300,
	EUartBaudRate600,EUartBaudRate1200,EUartBaudRate110,
	EUartBaudRate110,EUartBaudRate2400,EUartBaudRate110,
	EUartBaudRate4800,EUartBaudRate110,EUartBaudRate9600,
	EUartBaudRate19200,EUartBaudRate38400,EUartBaudRate57600,
	EUartBaudRate115200
	};

//
// TO DO: (mandatory)
//
// Lookup table to convert EPOC parity settings into hardware-specific values
//
// EXAMPLE ONLY

static const TUartParity Parity[3] =
	{
	EUartParityNone,EUartParityEven,EUartParityOdd
	};

//
// TO DO: (mandatory)
//
// Lookup table to convert EPOC stop bit values into hardware-specific values
//
// EXAMPLE ONLY

static const TUartStopBit StopBit[2] =
	{
	EUartStopBitOne,EUartStopBitTwo
	};

//
// TO DO: (mandatory)
//
// Lookup table to convert EPOC data bit settings into hardware-specific values
//
// EXAMPLE ONLY

static const TUartDataLength DataLength[4] =
	{
	EUartDataLength5,EUartDataLength6,	
	EUartDataLength7,EUartDataLength8
	};

#endif	//RVCT40 warning


class DDriverComm : public DPhysicalDevice
	{
public:
	DDriverComm();
	virtual TInt Install();
	virtual void GetCaps(TDes8 &aDes) const;
	virtual TInt Create(DBase*& aChannel, TInt aUnit, const TDesC8* anInfo, const TVersion &aVer);
	virtual TInt Validate(TInt aUnit, const TDesC8* anInfo, const TVersion &aVer);
	};

class DCommTemplate : public DComm
	{
public:
	DCommTemplate();
	~DCommTemplate();
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
	TLinAddr iPortAddr;
	TInt iInInterrupt;
	TUint iSignals;
	TDynamicDfcQue*	iDfcQ;
	};


DDriverComm::DDriverComm()
//
// Constructor
//
	{
	//
	// TO DO: (mandatory)
	//
	// Set up iUnitMask with the number of Units (Serial Ports) supported by this PDD,
	// 1 bit set per Unit supported e.g.:
	// iUnitsMask=0x7;	->  supports units 0, 1, 2
	//
	iVersion=TVersion(KCommsMajorVersionNumber,KCommsMinorVersionNumber,KCommsBuildVersionNumber);
	}

TInt DDriverComm::Install()
//
// Install the driver
//
	{

	return SetName(&KPddName);
	}

void GetTemplateCommsCaps(TDes8 &aCaps, TInt aUnit)
	{
	TCommCaps2 capsBuf;
	//
	// TO DO: (mandatory)
	//
	// Fill in the Caps structure with the relevant information for this Unit, e.g
	//  TCommCapsV02 &c=capsBuf();
	//	c.iRate=(OR in as many KCapsBpsXXX as bit rates supported);
	//	c.iDataBits=(OR in as many KCapsDataXXX as data length configuration supported);
	//	c.iStopBits=(OR in as many KCapsStopXXX as the number of stop bits configurations supported);
	//	c.iParity=(OR in as many KCapsParityXXX as parity configuration supported);
	//	c.iHandshake=(OR in all KCapsObeyXXXSupported, KCapsSendXXXSupported, KCapsFailXXXSupported, KCapsFreeXXXSupported
	//				  as required for this Unit's configuration);.
	//	c.iSignals=(OR in as many KCapsSignalXXXSupported as Modem control signals controllable by this Unit);
	//	c.iSIR=(0 or OR in as many KCapsSIRXXX as IR bit rates supported);
	//	c.iNotificationCaps=(OR in as many KNotifyXXXSupported as notifications supported by this Unit);
	//	c.iFifo=(0 or KCapsHasFifo);
	//	c.iRoleCaps=(0 or KCapsRoleSwitchSupported);
	//	c.iFlowControlCaps=(0 or KCapsFlowControlStatusSupported);
	/** @see TCommCapsV02 */
	//
	aCaps.FillZ(aCaps.MaxLength());
	aCaps=capsBuf.Left(Min(capsBuf.Length(),aCaps.MaxLength()));
	}

void DDriverComm::GetCaps(TDes8 &aDes) const
//
// Return the drivers capabilities
//
	{
	GetTemplateCommsCaps(aDes, 0);
	}

TInt DDriverComm::Create(DBase*& aChannel, TInt aUnit, const TDesC8* anInfo, const TVersion& aVer)
//
// Create a driver
//
	{
	DCommTemplate* pD=new DCommTemplate;
	aChannel=pD;
	TInt r=KErrNoMemory;
	if (pD)
		r=pD->DoCreate(aUnit,anInfo);
	return r;
	}

TInt DDriverComm::Validate(TInt aUnit, const TDesC8* /*anInfo*/, const TVersion& aVer)
//
//	Validate the requested configuration (Version and Unit)
//
	{
	if ((!Kern::QueryVersionSupported(iVersion,aVer)) || (!Kern::QueryVersionSupported(aVer,TVersion(KMinimumLddMajorVersion,KMinimumLddMinorVersion,KMinimumLddBuild))))
		return KErrNotSupported;
	//
	// TO DO: (mandatory)
	//
	// Return KErrNotSupported if aUnit is not in the supported range for this driver, KErrNone if it is
	//
	return KErrNone;
	}

DCommTemplate::DCommTemplate()
//
// Constructor
//
	{
	iInterruptId=-1;		// -1 means not bound
	}

DCommTemplate::~DCommTemplate()
//
// Destructor
//
	{
	if (iInterruptId>=0)
		Interrupt::Unbind(iInterruptId);
	
	if (iDfcQ)
		{
		iDfcQ->Destroy();
		}
	}

const TInt KDCommTemplDfcThreadPriority = 24;
_LIT(KDCommTemplDfcThread,"DCommTemplDfcThread");

TInt DCommTemplate::DoCreate(TInt aUnit, const TDesC8* /*anInfo*/)
//
// Sets up the PDD
//
	{
	iUnit=aUnit;
	TInt irq=-1;
	//
	// TO DO: (mandatory)
	//
	//  Create own DFC queue
	TInt r = Kern::DynamicDfcQCreate(iDfcQ, KDCommTemplDfcThreadPriority, KDCommTemplDfcThread);

	if (r != KErrNone)
		return r; 	

	// Set iPortAddr and irq with the Linear Base address of the UART and the Interrupt ID coresponding to aUnit
	//

	// bind to UART interrupt
	r=Interrupt::Bind(irq,Isr,this);
	if (r==KErrNone)
		iInterruptId=irq;

	//
	// TO DO: (optional)
	//
	// Any other setting up of UART hardware registers, required for:
	//  - Disabling the UART operation
	//  - disabling all UART Interrupts
	//  - clearing all Rx errors
	//  - clearing all UART interrupts
	//  - de-activating output Modem Control signals
	//

	Variant::MarkDebugPortOff();
	return r;
	}

TDfcQue* DCommTemplate::DfcQ(TInt aUnit)
//
// Return the DFC queue to be used for this device
// For UARTs just use the standard low priority DFC queue
// For Serial PC cards, use the PC card controller thread for the socket in question.
//
	{
	return aUnit==iUnit ? iDfcQ : NULL;
	}

TInt DCommTemplate::Start()
//
// Start receiving characters
//
	{
	iTransmitting=EFalse;			// if EnableTransmit() called before Start()

	//
	// TO DO: (mandatory)
	//
	// Set up hardware registers to enable the UART and switch receive mode on
	//

	// if (iUnit!=IR Port)											TO DO: (mandatory): Implement
		{
		iSignals=Signals();
		iLdd->UpdateSignals(iSignals);
		}

	//
	// TO DO: (optional)
	//
	// If Unit is IR Port may need to start the IR port
	//
	Interrupt::Enable(iInterruptId);
	return KErrNone;
	}

TBool FinishedTransmitting(TAny* aPtr)
	{
	//
	// TO DO: (mandatory)
	//
	// Return ETrue if UART is still transmitting, EFalse Otherwise
	//
	return EFalse;		// EXAMPLE ONLY
	}

void DCommTemplate::Stop(TStopMode aMode)
//
// Stop receiving characters
//
	{
	switch (aMode)
		{
		case EStopNormal:
		case EStopPwrDown:
			Interrupt::Disable(iInterruptId);
			iTransmitting=EFalse;

			// wait for uart to stop tranmitting
			Kern::PollingWait(FinishedTransmitting,this,3,100);

			//
			// TO DO: (optional)
			//
			// Any other setting up of UART hardware registers, required for:
			//  - Disabling the UART operation
			//  - disabling all UART Interrupts
			//  - disabling Transmit and Receive pathes
			//  - clearing all UART interrupts
			//
			break;
		case  EStopEmergency:
			Interrupt::Disable(iInterruptId);
			iTransmitting=EFalse;
			break;
		}
	//
	// TO DO: (optional)
	//
	// If Unit is IR Port may need to stop the IR port
	//
	Variant::MarkDebugPortOff();
	}

void DCommTemplate::Break(TBool aState)
//
// Start or stop the uart breaking
//
	{
	if (aState)
		{
		//
		// TO DO: (mandatory)
		//
		// Enable sending a Break (space) condition
		//
		}
	else
		{
		//
		// TO DO: (mandatory)
		//
		// Stop sending a Break (space) condition
		//
		}
	}

void DCommTemplate::EnableTransmit()
//
// Start sending characters.
//
	{
	TBool tx = (TBool)__e32_atomic_swp_ord32(&iTransmitting, 1);
	if (tx)
		return;
		TInt r = 0;
	while (/* (Transmit FIFO Not full) && */ Kern::PowerGood())				// TO DO: (mandatory): Implement
		{
		TInt r=TransmitIsr();
		if(r<0)
			{
			//no more to send
			iTransmitting=EFalse;
			break;
			}
		//
		// TO DO: (mandatory)
		//
		// Write transmit character into transmit FIFO or output register
		//
		}
	TInt irq=0;
	if (!iInInterrupt)					// CheckTxBuffer adds a Dfc: can only run from ISR or with NKernel locked
		{
		NKern::Lock();
		irq=NKern::DisableAllInterrupts();
		}
	CheckTxBuffer();
	if (!iInInterrupt)
		{
		NKern::RestoreInterrupts(irq);
		NKern::Unlock();
		}
	//
	// TO DO: (mandatory)
	//
	// Enable transmission of data
	//
	if (r>=0)											// only enable interrupt if there's more data to send
		{
		//
		// TO DO: (mandatory)
		//
		// Enable transmit interrupt in the Hardware (Interrupt::Enable() has already been called in Start())
		//
		}
	}

TUint DCommTemplate::Signals() const
//
// Read and translate the modem lines
//
	{
	TUint signals=0;
	//
	// TO DO: (mandatory)
	//
	// If the UART corresponding to iUnit supports Modem Control Signals, read them and return a bitmask with one or 
	// more of the following OR-ed in:
	// - KSignalDTR,
	// - KSignalRTS,
	// - KSignalDSR,
	// - KSignalCTS,
	// - KSignalDCD.
	// 
	return signals;
	}

void DCommTemplate::SetSignals(TUint aSetMask, TUint aClearMask)
//
// Set signals.
//
	{
	//
	// TO DO: (mandatory)
	//
	// If the UART corresponding to iUnit supports Modem Control Signals, converts the flags in aSetMask and aClearMask 
	// into hardware-specific bitmasks to write to the UART modem/handshake output register(s). 
	// aSetMask, aClearMask will have one or more of the following OR-ed in:
	// - KSignalDTR,
	// - KSignalRTS,
	//
	}

TInt DCommTemplate::ValidateConfig(const TCommConfigV01 &aConfig) const
//
// Check a config structure.
//
	{
	//
	// TO DO: (mandatory)
	//
	// Checks the the options in aConfig are supported by the UART corresponding to iUnit
	// May need to check:
	//  - aConfig.iParity (contains one of EParityXXX)
	/** @see TParity */
	//  - aConfig.iRate (contains one of EBpsXXX)
	/** @see TBps */
	//  - aConfig.iDataBits (contains one of EDataXXX)
	/** @see TDataBits */
	//  - aConfig.iStopBits (contains one of EStopXXX)
	/** @see TDataBits */
	//  - aConfig.iHandshake (contains one of KConfigObeyXXX or KConfigSendXXX or KConfigFailXXX or KConfigFreeXXX)
	//  - aConfig.iParityError (contains KConfigParityErrorFail or KConfigParityErrorIgnore or KConfigParityErrorReplaceChar)
	//  - aConfig.iFifo (contains ether EFifoEnable or EFifoDisable)
	/** @see TFifo */
	//  - aConfig.iSpecialRate (may contain a rate not listed under TBps)
	//  - aConfig.iTerminatorCount (conatains number of special characters used as terminators)
	//  - aConfig.iTerminator[] (contains a list of special characters which can be used as terminators)
	//  - aConfig.iXonChar (contains the character used as XON - software flow control)
	//  - aConfig.iXoffChar (contains the character used as XOFF - software flow control)
	//  - aConfig.iParityErrorChar (contains the character used to replace bytes received with a parity error)
	//  - aConfig.iSIREnable (contains either ESIREnable or ESIRDisable)
	/** @see TSir */
	//  - aConfig.iSIRSettings (contains one of KConfigSIRXXX)
	// and returns KErrNotSupported if the UART corresponding to iUnit does not support this configuration
	//
	return KErrNone;
	}

void DCommTemplate::CheckConfig(TCommConfigV01& aConfig)
	{
	//
	// TO DO: (optional)
	//
	// Validates the default configuration that is defined when a channel is first opened
	//
	}

TInt DCommTemplate::DisableIrqs()
//
// Disable normal interrupts
//
	{
	
	return NKern::DisableInterrupts(1);
	}

void DCommTemplate::RestoreIrqs(TInt aLevel)
//
// Restore normal interrupts
//
	{
	
	NKern::RestoreInterrupts(aLevel);
	}

void DCommTemplate::Configure(TCommConfigV01 &aConfig)
//
// Configure the UART from aConfig
//
	{
	Kern::PollingWait(FinishedTransmitting,this,3,100);	// wait for uart to stop tranmitting

	//
	// TO DO: (optional)
	//
	// Ensure Tx, Rx and the UART are disabled and disable sending Break (space) condition.
	// May need to modify clocks settings, pin functions etc.
	//

	//
	// TO DO: (mandatory)
	//
	// Set communications parameters such as:
	//  - Baud rate
	//  - Parity
	//  - Stop bits
	//  - Data bits
	// These can be obtained from aConfig using the look-up tables above, e.g.
	// TUint baudRate=BaudRate[aConfig.iRate];
	// TUint parity=Parity[aConfig.iParity];
	// TUint stopBits=StopBit[aConfig.iStopBits];
	// TUint dataBits=DataLength[aConfig.iDataBits];
	// Write these to the appropriate hardware registers using iPortAddr to identify which ste of register to modify
	//

	//
	// TO DO: (optional)
	//
	// If the UART corresponding to iUnit supports IR may need to set up IR transceiver
	//
	}

void DCommTemplate::Caps(TDes8 &aCaps) const
//
// return our caps
//
	{
	GetTemplateCommsCaps(aCaps,iUnit);
	}

void DCommTemplate::Isr(TAny* aPtr)
//
// Service the UART interrupt
//
	{
	DCommTemplate& d=*(DCommTemplate*)aPtr;
	d.iInInterrupt=1;										// going in...
	// TUint portAddr=d.iPortAddr;										TO DO: (mandatory): Uncomment this

	//
	// TO DO: (mandatory)
	//
	// Read the interrupt source register to determine if it is a Receive, Transmit or Modem Signals change interrupt.
	// If required also, clear interrupts at the source.
	// Then process the interrupt condition as in the following pseudo-code extract:
	//
	// if((Received character Interrupts) || (Error in received character Interupt))		TO DO: (mandatory): Implement
		{
		TUint rx[32];
		TUint xon=d.iLdd->iRxXonChar;
		TUint xoff=d.iLdd->iRxXoffChar;
		TInt rxi=0;
		TInt x=0;
		TUint ch=0;
	//	while((Receive FIFO not empty) && Kern::PowerGood())								TO DO: (mandatory): Implement
			{
			TUint regStatus1=0;
				// NOTE: for some hardware the order the following 2 operations is performed may have to be reversed
	//		if(Error in received character interrupt)										TO DO: (mandatory): Implement
	//			regStatus1=(Read receive error bitmask off appropriate register);
	//		ch=(Read received character);
			
			// coverity[dead_error_condition]
			// The next line should be reachable when this template file is edited for use
			if(regStatus1!=0)						// if error in this character
				{
	//			if (ch & (Parity Error))													TO DO: (mandatory): Implement
					ch|=KReceiveIsrParityError;
	//			if (ch & (Framing Error))													TO DO: (mandatory): Implement
					ch|=KReceiveIsrFrameError;
	//			if (ch & (Overrun))															TO DO: (mandatory): Implement
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
		}
	// if((Transmitted character Interrupt))												TO DO: (mandatory): Implement
		{
		while(/* (Transmit FIFO Not full) && */ Kern::PowerGood())							// TO DO: (mandatory): Implement
			{
			TInt r=d.TransmitIsr();
			if(r<0)
				{
				//no more to send
				//
				// TO DO: (mandatory)
				//
				// Disable the Transmit Interrupt in Hardware
				d.iTransmitting=EFalse;
				break;
				}
			// (write transmit character to output FIFO or Data register)					TO DO: (mandatory): Implement
			}
		d.CheckTxBuffer();
		}
	// if((Modem Signals changed Interrupt))												TO DO: (mandatory): Implement
		{
		TUint signals=d.Signals()&KDTEInputSignals;
		if (signals != d.iSignals)
			{
			d.iSignals=signals;
			d.iLdd->StateIsr(signals);
			}
		}
	d.iInInterrupt=0;										// going out...
	}

DECLARE_STANDARD_PDD()
	{
	return new DDriverComm;
	}

