// Copyright (c) 1995-2010 Nokia Corporation and/or its subsidiary(-ies).
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
// e32\include\d32comm.h
// 
//

//#define _DEBUG_DEVCOMM

/**
@file
@publishedPartner
@released
*/

#ifndef __D32COMM_H__
#define __D32COMM_H__
#include <e32cmn.h>
#include <e32ver.h>
#include <d32public.h>

/**
 Enumeration of Fifo status (enable and disable) for serial port configuration.
 Typically, these values are used to initialize the iFifo of TCommConfigV01 
 before calling DComm::Configure() or any other serial comm API to configure
 the serial port's fifo.
 */
enum TFifo
	{
	EFifoEnable,EFifoDisable,
	};
/**
 Enumeration of SIR status (enable and disable) for serial comm configuration.
 Typically, these values are used to initialize the iSIREnable of TCommConfigV01 
 before calling DComm::Configure() or any other serial comm API to configure
 the serial port's SIR (infrared) settings.
 */
enum TSir 
	{
	ESIREnable,ESIRDisable,
	};

const TInt KConfigMaxTerminators=4;
// DTE Constants
const TUint KConfigObeyXoff=0x01;
const TUint KConfigSendXoff=0x02;
const TUint KConfigObeyCTS=0x04;
const TUint KConfigFailCTS=0x08;
const TUint KConfigObeyDSR=0x10;
const TUint KConfigFailDSR=0x20;
const TUint KConfigObeyDCD=0x40;
const TUint KConfigFailDCD=0x80;
const TUint KConfigFreeRTS=0x100;
const TUint KConfigFreeDTR=0x200;
// DCE Constants
const TUint KConfigObeyDTR=0x400;
const TUint KConfigFailDTR=0x800;
const TUint KConfigObeyRTS=0x1000;
const TUint KConfigFailRTS=0x2000;
const TUint KConfigFreeDSR=0x4000;
const TUint KConfigFreeCTS=0x8000;
const TUint KConfigFreeDCD=0x10000;
const TUint KConfigFreeRI=0x20000;
const TUint KConfigWriteBufferedComplete=0x80000000;
//
const TUint KConfigParityErrorFail=0;
const TUint KConfigParityErrorIgnore=0x01;
const TUint KConfigParityErrorReplaceChar=0x02;
const TUint KConfigXonXoffDebug=0x80000000;
//
const TUint KSignalCTS=0x01;
const TUint KSignalDSR=0x02;
const TUint KSignalDCD=0x04;
const TUint KSignalRNG=0x08;
const TUint KSignalRTS=0x10; 
const TUint KSignalDTR=0x20;
const TUint KSignalBreak=0x40;

const TUint KSignalChanged=0x1000;
const TUint KCTSChanged=KSignalCTS*KSignalChanged;
const TUint KDSRChanged=KSignalDSR*KSignalChanged;
const TUint KDCDChanged=KSignalDCD*KSignalChanged;
const TUint KRNGChanged=KSignalRNG*KSignalChanged;
const TUint KRTSChanged=KSignalRTS*KSignalChanged;
const TUint KDTRChanged=KSignalDTR*KSignalChanged;
const TUint KBreakChanged=KSignalBreak*KSignalChanged;

const TUint KSignalDTEOutputs=KSignalRTS|KSignalDTR;
const TUint KSignalDTEInputs=KSignalCTS|KSignalDSR|KSignalDCD|KSignalRNG;
const TUint KSignalDCEInputs=KSignalDTEOutputs;
const TUint KSignalDCEOutputs=KSignalDTEInputs;

const TUint KConfigSIRPulseWidthMaximum=0x01;
const TUint KConfigSIRPulseWidthMinimum=0x02;

// more SIRSettings for selecting the IR range
const TUint KConfigSIRShutDown=0x10;
const TUint KConfigSIRMinimumRange=0x20;
const TUint KConfigSIRMediumRange=0x40;
const TUint KConfigSIRMaximumRange=0x80;

/**
 Comms configuration structure.
 Class to hold the configuration settings for serial comm port
 
 This class provides the serial port configuration block interface of serial comms (c32).
 A serial comm client sets up a serial port before use, by providing a configuration block. 
 TCommConfigV01 is initialized with settings for serial port and used to configure the 
 serial port by calling DComm::Configure(TCommConfigV01 &aConfig) or any other serial comm 
 API to configure the serial port.
 */
class TCommConfigV01
    {
public:
	/** 
	 Data rate in bits per second.
	 @see TBps 	
	 */
	TBps iRate;
	/** 
	 Character width in bits.
	 @see TDataBits
	 */	
	TDataBits iDataBits;
	/**
	 Number of stop bits.
	 @see TStopBits
	 */
	TStopBits iStopBits;
	/**
	 Type of parity.
	 @see TParity 
	 */
	TParity iParity;
	/**
	 Type of Handshaking control.
	 Possible values can be KConfigObeyXXX or KConfigSendXXX or KConfigFailXXX or KConfigFreeXXX
	 */
	TUint iHandshake;
	/**
	 Type of error to generate on a parity failure.
	 Possible values can be KConfigParityErrorFail or KConfigParityErrorIgnore or KConfigParityErrorReplaceChar
	 */
	TUint iParityError;
	/**
	 FIFO status, enabled or disabled.
	 @see TFifo 
	 */
	TUint iFifo;
	/**
	 Special data rate, not listed under TBps. Use this, when iRate is set to EBpsSpecial 
	 */
	TInt iSpecialRate;
	/**
	 Count of number of special characters used as terminators (<=KConfigMaxTerminators) 
	 */				
	TInt iTerminatorCount;
	/**
	 Array of special characters which can be used as terminators 
	 */
	TText8 iTerminator[KConfigMaxTerminators];
	/**
	 Character used to signal the transmitter to resume sending when using XON/XOFF handshaking
	 i.e character used as XON - software flow control
	 */
	TText8 iXonChar;
	/**
	 Character used to signal the transmitter to suspend sending when using XON/XOFF handshaking
	 i.e character used as XOFF - software flow control
	 */
	TText8 iXoffChar;
	/**
	 Character used to replace the characters received with a parity error.
	 This is used when iParityError is set to KConfigParityErrorReplaceChar 
	 */	
	TText8 iParityErrorChar;
	/**
	 Switch the SIR encoding hardware on and off.
	 @see TSir
	 */
	TSir iSIREnable;
	/**
	 SIR hardware control setting. Possible value can be one of KConfigSIRXXX
	 */
	TUint iSIRSettings;
	};
/** 
 Package buffer for a TCommConfigV01 object.
 
 TCommConfig packages the comms configuration structure TCommConfigV01 to a buffer. 
 This is used with API of serial comms like RComm::Config(TDes8 &aConfig) and 
 RComm::SetConfig(TDesC8 &aConfig) where config structure is passed as buffer.
 
 @see TCommConfigV01
 */		
typedef TPckgBuf<TCommConfigV01> TCommConfig;

// TCommConfigV02 is deprecated.
//
class TCommConfigV02: public TCommConfigV01
	{
public:		
	TInt iTxShutdownTimeout;
	};

// TCommConfig2 is deprecated
// 
typedef TPckgBuf<TCommConfigV02> TCommConfig2;

//
const TUint KCapsBps50=0x00000001;
const TUint KCapsBps75=0x00000002;
const TUint KCapsBps110=0x00000004;
const TUint KCapsBps134=0x00000008;
const TUint KCapsBps150=0x00000010;
const TUint KCapsBps300=0x00000020;
const TUint KCapsBps600=0x00000040;
const TUint KCapsBps1200=0x00000080;
const TUint KCapsBps1800=0x00000100;
const TUint KCapsBps2000=0x00000200;
const TUint KCapsBps2400=0x00000400;
const TUint KCapsBps3600=0x00000800;
const TUint KCapsBps4800=0x00001000;
const TUint KCapsBps7200=0x00002000;
const TUint KCapsBps9600=0x00004000;
const TUint KCapsBps19200=0x00008000;
const TUint KCapsBps38400=0x00010000;
const TUint KCapsBps57600=0x00020000;
const TUint KCapsBps115200=0x00040000;
const TUint KCapsBps230400=0x00080000;
const TUint KCapsBps460800=0x00100000;
const TUint KCapsBps576000 =0x00200000;
const TUint KCapsBps1152000=0x00400000;
const TUint KCapsBps4000000=0x00800000;
const TUint KCapsBps921600=0x01000000;
const TUint KCapsBpsAutobaud=0x40000000;
const TUint KCapsBpsSpecial=0x80000000;
//
const TUint KCapsData5=0x01;
const TUint KCapsData6=0x02;
const TUint KCapsData7=0x04;
const TUint KCapsData8=0x08;
//
const TUint KCapsStop1=0x01;
const TUint KCapsStop2=0x02;
//
const TUint KCapsParityNone=0x01;
const TUint KCapsParityEven=0x02;
const TUint KCapsParityOdd=0x04;
const TUint KCapsParityMark=0x08;
const TUint KCapsParitySpace=0x10;
//
const TUint KCapsSignalCTSSupported=0x01;
const TUint KCapsSignalDSRSupported=0x02;
const TUint KCapsSignalDCDSupported=0x04;
const TUint KCapsSignalRNGSupported=0x08;
const TUint KCapsSignalRTSSupported=0x10;
const TUint KCapsSignalDTRSupported=0x20;
//
const TUint KCapsObeyXoffSupported=0x01;
const TUint KCapsSendXoffSupported=0x02;
const TUint KCapsObeyCTSSupported=0x04;
const TUint KCapsFailCTSSupported=0x08;
const TUint KCapsObeyDSRSupported=0x10;
const TUint KCapsFailDSRSupported=0x20;
const TUint KCapsObeyDCDSupported=0x40;
const TUint KCapsFailDCDSupported=0x80;
const TUint KCapsFreeRTSSupported=0x100;
const TUint KCapsFreeDTRSupported=0x200;
// DCE Constants
const TUint KCapsObeyRTSSupported=0x400;
const TUint KCapsObeyDTRSupported=0x800;
//
const TUint KCapsHasFifo=0x01;
//
const TUint KCapsSIR115kbps=0x01;
const TUint KCapsSIR2400bpsOnly=0x02;
const TUint KCapsSIR4Mbs=0x04;
//
const TUint KNotifySignalsChangeSupported=0x01;
const TUint KNotifyRateChangeSupported=0x02;
const TUint KNotifyDataFormatChangeSupported=0x04;
const TUint KNotifyHandshakeChangeSupported=0x08;
const TUint KNotifyBreakSupported=0x10;
const TUint KNotifyFlowControlChangeSupported=0x20;
const TUint KNotifyDataAvailableSupported=0x40;
const TUint KNotifyOutputEmptySupported=0x80;
//
const TUint KCapsRoleSwitchSupported=0x01;
//
const TUint KCapsFlowControlStatusSupported=0x01;
//
const TUint KRateChanged=0x01;
const TUint KDataFormatChanged=0x02;
const TUint KHandshakeChanged=0x04;
//

/**
 Comms capability structure.
 Class to query the capability settings for serial comm port device.
 Members of this class are filled with the capabilities of the comm port device.
 */

class TCommCapsV01
	{
public:
	/** 
	 Data rates supported, in bits per second.
	 The value is a bitmask made by OR-ing KCapsBpsXXX values.
	 Each set bit corresponds to a supported bit rate.
	 */
	TUint iRate;
	/**
	 Character widths supported, in bits.
	 The value is a bitmask made by OR-ing a combination of KCapsData5, KCapsData6, KCapsData7 and KCapsData8 values.
	 Each set bit corresponds to a supported character width. 
	 */	
	TUint iDataBits;
	/**
	 Number of stop bits supported.
	 The value is one of KCapsStop1, KCapsStop2 or KCapsStop1|KCapsStop2.
	 Each set bit corresponds to a supported number of stop bit.
	 */
	TUint iStopBits;
	/**
	 Parity types supported.
	 The value is a bitmask made by OR-ing a combination of KCapsParityNone, KCapsParityEven, KCapsParityOdd, KCapsParityMark and KCapsParitySpace values.
	 Each set bit corresponds to a supported parity type. 
	 */
	TUint iParity;
	/**
	 Type of Handshaking protocols supported by the device.
	 The value is a bitmask made by OR-ing a combination of KCapsObeyXXX, KCapsSendXXX, KCapsFailXXX and KCapsFreeXXX values.
	 Each set bit corresponds to a supported handshaking protocol.
	 */
	TUint iHandshake;
	/**
	 Type of Signals supported by the device.
	 The value is a bitmask made by OR-ing a combination of KCapsSignalXXX values.
	 Each set bit corresponds to a supported signal.
	 */
	TUint iSignals;
	/**
	 Whether Fifo is enabled or disabled.
	 Value is KCapsHasFifo if enabled, 0 otherwise
	 */
	TUint iFifo;
	/**
	 Capabilities of the SIR encoding hardware.
	 The value is a bitmask made by OR-ing a combination of KCapsSIR115kbps, KCapsSIR2400bpsOnly and KCapsSIR4Mbps values.
	 Each set bit corresponds to a supported SIR capability.
	 */
	TUint iSIR;
	};
/** 
 Package buffer for a TCommCapsV01 object.
 
 TCommCaps packages the comms capability structure TCommCapsV01 in a buffer. 
 This is used by serial comms APIs to pass the capability structure as a buffer,
 for example in RComm::Caps(TDes8 &aCaps).
  
 @see TCommCapsV01
 */		
typedef TPckgBuf<TCommCapsV01> TCommCaps;

/**
 Comms capability structure.
 Class to query the capability settings for serial comm port.
 Members of this class are filled with the capabilities of the comm port.
 
 @see TCommCapsV01
 */
class TCommCapsV02 : public TCommCapsV01
	{
public:
	/**
	 Specifies the notifications that are supported for the serial comm port.
	 The field is a bitmask made by OR-ing a combination of:
	 	-KNotifySignalsChangeSupported
		-KNotifyRateChangeSupported
		-KNotifyDataFormatChangeSupported
		-KNotifyHandshakeChangeSupported
		-KNotifyBreakSupported
		-KNotifyFlowControlChangeSupported
		-KNotifyDataAvailableSupported
		-KNotifyOutputEmptySupported
	 Each set bit corresponds to a supported notification type.
	 */
	TUint iNotificationCaps;
	/**
	 Specifies whether Role Switch is supported or not.
	 If supported the value is KCapsRoleSwitchSupported, otherwise Zero
	 */
	TUint iRoleCaps;
	/**
	 Specifies whether Flow Control Status is supported or not.
	 If supported the value is KCapsFlowControlStatusSupported, otherwise Zero
	 */
	TUint iFlowControlCaps;
	};
	
/** 
 Package buffer for a TCommCapsV02 object.
 
 TCommCaps2 packages the comms capability structure TCommCapsV02 in a buffer. 
 This is used by serial comms to pass the capability structure as a buffer,
 for example in RComm::Caps(TDes8 &aCaps) 
 
 @see TCommCapsV02
 */		
typedef TPckgBuf<TCommCapsV02> TCommCaps2;


/**
 Comms capability structure.
 Class to hold the capability settings for serial comm port.
 
 This class is used to query the capabilities of the serial comm port.
 Members of this class are filled with the capabilities of the comm port.
 
 @see TCommCapsV02
 */
class TCommCapsV03 : public TCommCapsV02
	{
public:
	/**
	 Specifies whether break is supported or not.
	 ETrue if Supported, EFalse otherwise.
	 */
	TBool iBreakSupported;
	};
	
/** 
 Package buffer for a TCommCapsV03 object.
 
 TCommCaps3 packages the comms capability structure TCommCapsV03 in a buffer. 
 This is used by serial comms APIs to pass the capability structure as a buffer,
 for example in RComm::Caps(TDes8 &aCaps) 
 
 @see TCommCapsV03
 */	
typedef TPckgBuf<TCommCapsV03> TCommCaps3;

/**
 Structure that holds the capabilities of the Comms factory object. Only Version is supported.
 This structure is packaged within a descriptor when passed to methods such as RDevice::GetCaps()
 */
class TCapsDevCommV01
	{
public:
	/**
	Version of the device
	@see TVersion
	*/
	TVersion version;
	};

/**
 Comms Notification configuration structure
 Class to hold the notification configuration of the device.
 Notifications are only used with DCE (modem) comms ports.
 */
class TCommNotificationV01
	{
public:
	/**
	 Specifies which of the configuration members have changed
	 This value is a bitmask made by OR-ing a combination of KRateChanged,KDataFormatChanged and KHandshakeChanged values.
	 Each set bit corresponds to a change in the configuration notification.
	 @see TCommCapsV01
	 */
	TUint iChangedMembers;
	/** 
	 Data rate in bits per second.
	 @see TBps 	
	 */
	TBps iRate;
	/** 
	 Character width in bits.
	 @see TDataBits
	 */	
	TDataBits iDataBits;
	/**
	 Number of stop bits.
	 @see TStopBits
	 */
	TStopBits iStopBits;
	/**
	 Type of parity.
	 @see TParity 
	 */
	TParity iParity;
	/**
	 Type of Handshaking control.
	 Possible values can be any combination of KConfigObeyXXX, KConfigSendXXX, KConfigFailXXX and KConfigFreeXXX.
	 */
	TUint iHandshake;
	};
/** 
 Package buffer for a TCommNotificationV01 object.
 Packages TCommNotificationV01 within a buffer. 
 @see TCommNotificationV01
 */	
typedef TPckgBuf<TCommNotificationV01> TCommNotificationPckg;
//
const TUint KDataAvailableNotifyFlag=0x80000000;
//
#ifdef _DEBUG_DEVCOMM
class TCommDebugInfo
	{
public:
	TBool iRxBusy;
	TBool iRxHeld;
	TInt iRxLength;
	TInt iRxOffset;
	TInt iRxIntCount;
	TInt iRxErrCount;
	TInt iRxBufCount;
	TBool iTxBusy;
	TBool iTxHeld;
	TInt iTxLength;
	TInt iTxOffset;
	TInt iTxIntCount;
	TInt iTxErrCount;
	TInt iTxBufCount;
	TBool iDrainingRxBuf;
	TBool iFillingTxBuf;
	TBool iRunningDfc;
	TInt iDfcCount;
	TInt iDfcReqSeq;
	TInt iDfcHandlerSeq;
	TInt iDoDrainSeq;
	TBool iTxDfcPend;
	TBool iRxDfcPend;
	TInt iTxChars, iRxChars;
	TInt iTxXon, iTxXoff, iRxXon, iRxXoff;
	};
typedef TPckgBuf<TCommDebugInfo> TCommDebugInfoPckg;
#endif
//

/**
 The externally visible interface through which the clients can access serial devices.
 It also represents a user side handle to the serial device driver. 
 */
class RBusDevComm : public RBusLogicalChannel
	{
public:
	/**
	 Serial device driver build version.
	 */
	enum TVer
		{
		/** Major Version */
		EMajorVersionNumber=1,
		/** Minor Version */
		EMinorVersionNumber=0,
		/** Build Version */
		EBuildVersionNumber=KE32BuildVersionNumber
		};
	
	/**
	 Asynchronous request types
	 */
	enum TRequest
		{
		/** Read request */
		ERequestRead=0x0,
		/** Cancel read request */
		ERequestReadCancel=0x1,
		/** Write reqeust */
		ERequestWrite=0x1,
		/** Cancel write request */
		ERequestWriteCancel=0x2,
		/** Break request */
		ERequestBreak=0x2,
		/** Cancel break request */
		ERequestBreakCancel=0x4,
		/** Signal change notification request */
		ERequestNotifySignalChange=0x3,
		/** Cancel signal change notification request */
		ERequestNotifySignalChangeCancel=0x8,
		};
		
	/**
	 Synchronous request types
	 */
	enum TControl
		{
		/** Get the current configuration */
		EControlConfig,
		/** Set the device configuration */
		EControlSetConfig,
		/** Get the device capabilities */
		EControlCaps,
		/** Read the state of Modem control signals supported */
		EControlSignals,
		/** Set the state of output modem control signals */
		EControlSetSignals,
		/** Query the driver receive buffer for data availability */
		EControlQueryReceiveBuffer,
		/** Reset the driver buffers */
		EControlResetBuffers,
		/** Get the driver receive buffer length */
		EControlReceiveBufferLength,
		/** Set the driver receive buffer length */
		EControlSetReceiveBufferLength,
		/** Get the minimum turnaround time between a receive and subsequent transmission operation */
		EControlMinTurnaroundTime,
		/** Set the minimum turnaround time between a receive and subsequent transmission operation */
		EControlSetMinTurnaroundTime,
#ifdef _DEBUG_DEVCOMM
        /** Get debug information from the driver */
		EControlDebugInfo
#endif
		};
				
public:
#ifndef __KERNEL_MODE__

	/**
	 This function opens a channel and creates a handle to the serial driver.
	 @param aUnit The unit number of the serial device.
	 @return KErrNone, if successful;
	         otherwise one of the other system-wide error codes.
	         KErrPermissionDenied if the port given in aName is wrong or if the request fails the CSY's own security check; 
	         KErrNotSupported if this port is not supported by the CSY or the hardware; 
	         KErrLocked if the port has already been opened; 
	         KErrAccessDenied if the device driver encounteres a problem opening the hardware port.
	 */
	inline TInt Open(TInt aUnit);
	
	/**
	 Get the version number required by the driver 
	 @return The version number required by the driver
	 @see TVersion
	 */
	inline TVersion VersionRequired() const;
	
	/**
	 Read from the channel
	 @param aStatus The asynchronous request status 
	 @param aDes Buffer to be filled in by the driver
	 */
	inline void Read(TRequestStatus &aStatus,TDes8 &aDes);
	
	/**
	 Read from the channel
	 @param aStatus The asynchronous request status 
	 @param aDes Buffer to be filled in by the driver
	 @param aLength The length of the data to be read
	 */
	inline void Read(TRequestStatus &aStatus,TDes8 &aDes,TInt aLength);
	
	/**
	 Read one or more characters from the channel.
	 If there is data in the serial driver's buffer when ReadOneOrMore() is called it will 
	 read as much data as possible (up to the maximum length of the supplied buffer) 
	 and then return.
	 If there is no data in the buffer the request will complete as soon as one or more bytes arrive at the serial hardware.
	 @param aStatus The asynchronous request status 
	 @param aDes Buffer to be filled in by the driver
	 */
	inline void ReadOneOrMore(TRequestStatus &aStatus,TDes8 &aDes);
	
	/**
	 Cancel a pending read request
	 */
	inline void ReadCancel();
	
	/**
	 Write to the channel
	 @param aStatus The asynchronous request status 
	 @param aDes Buffer containing the data to be sent
	 */
	inline void Write(TRequestStatus &aStatus,const TDesC8 &aDes);

	/**
	 Write to the channel
	 @param aStatus The asynchronous request status 
	 @param aDes Buffer containing the data to be sent
	 @param aLength The length of the data to be sent
	 */        
	inline void Write(TRequestStatus &aStatus,const TDesC8 &aDes,TInt aLength);
	
	/**
	 Cancel a pending write request
	 */
	inline void WriteCancel();
	
	/**
	 Causes a break condition to be transmitted to the receiving device
	 @param aStatus The asynchronous request status
	 @param aTime The time interval in microseconds after which the break condition will be released	
	 */
	inline void Break(TRequestStatus &aStatus,TInt aTime);
	
	/**
	 Cancel a pending break request
	 */
	inline void BreakCancel();
	
	/**
	 Get the current configuration of the serial device
	 @param aConfig A packaged object to be filled with the configuration information by the driver
	 @see TCommConfigV02
	 */
	inline void Config(TDes8 &aConfig);
	
	/**
	 Set the cofiguration of the serial device
	 @param aConfig A packaged object containing the configuration information
	 @see TCommConfigV02
	 */
	inline TInt SetConfig(const TDesC8 &aConfig);
	
	/**
	 Get the capabilities of the serial device.
	 @param aCaps A packaged object to be filled with the capabilities of the device.
	 @see TCommCapsV03
	*/
	inline void Caps(TDes8 &aCaps);
	
	/**
	 Get the status of the control lines
	 @return  A bitmask of KSignalCTS, KSignalDSR, KSignalDCD, KSignalRNG,
              KSignalRTS, KSignalDTR, KSignalBreak
	 */
	inline TUint Signals();
	
	/**
	 Set and clear the control lines
	 @param aSetMask    Bitmask of signals to set
	 @param aClearMask  Bitmask of signals to clear
	 @see Signals for a list of signals
	 */
	inline void SetSignals(TUint aSetMask,TUint aClearMask);
	
	/**
	 Get the number of unread characters in the receive buffer of the driver
	 @return The number of unread characters
	 */
	inline TInt QueryReceiveBuffer();
	
	/**
	 Reset the receive and transmit buffers.
	 */
	inline void ResetBuffers();
	
	/**
	 Get the length of the receive buffer
	 @return The length of the receive buffer
	 */	
	inline TInt ReceiveBufferLength();
	
	/**
	 Set the length of the receive buffer
	 @param aLength The length of the receive buffer to be set
	 */
	inline TInt SetReceiveBufferLength(TInt aLength);
		
	/**
	 Request notification when one of the signals change.
	 The signals that could change are KSignalCTS, KSignalDSR, KSignalDCD, KSignalRNG,
	 KSignalRTS, KSignalDTR, KSignalBreak.
	 @param aStatus  The asynchronous request status
	 @param aSignals Pointer to the bitmask containing the changed signals
	 @param aSignalMask Bitmask of signals to be monitored
	 */
	inline void NotifySignalChange(TRequestStatus& aStatus,TUint& aSignals,TUint aSignalMask=0x3F);
	
	/**
	 Cancel a pending signal change notification request
	 */
	inline void NotifySignalChangeCancel();
	
	/**
	 Request notification when there is data available to be read from the driver receive buffer
	 @param aStatus The asynchronous request status
	 */
	inline void NotifyReceiveDataAvailable(TRequestStatus& aStatus);
	
	/**
	 Cancel a pending data notification request
	 */
	inline void NotifyReceiveDataAvailableCancel();
	
	/**
	 Get the minimum turnaround time before a transmission can begin after a receive operation
	 @return The turnaround time in microseconds 
	 */
	inline TUint MinTurnaroundTime();
	
	/**
	 Set the minimum turnaround time between a receive and the next transmission operation
	 @param aMicroSeconds The turnaround time in microseconds	               	
	 */
	inline TInt SetMinTurnaroundTime(TUint aMicroSeconds);
	
#ifdef _DEBUG_DEVCOMM
	/**
	 Get the debug information
	 @param aInfo a packaged object to be filled by the driver with debug information
	 @see TCommDebugInfo
	 */
	inline void DebugInfo(TDes8 &aInfo);
#endif
#endif
	};

class RBusDevCommDCE : public RBusLogicalChannel
	{
public:
	enum TVer {EMajorVersionNumber=1,EMinorVersionNumber=0,EBuildVersionNumber=KE32BuildVersionNumber};
	enum TRequest
		{
		ERequestRead=0x0,ERequestReadCancel=0x1,
		ERequestWrite=0x1,ERequestWriteCancel=0x2,
		ERequestBreak=0x2,ERequestBreakCancel=0x4,
		ERequestNotifySignalChange=0x3,ERequestNotifySignalChangeCancel=0x8,
		ERequestNotifyFlowControlChange=0x4,ERequestNotifyFlowControlChangeCancel=0x10,
		ERequestNotifyConfigChange=0x5,ERequestNotifyConfigChangeCancel=0x20
		};
	enum TControl
		{
		EControlConfig,EControlSetConfig,EControlCaps,
		EControlSignals,EControlSetSignals,
		EControlQueryReceiveBuffer,EControlResetBuffers,
		EControlReceiveBufferLength,EControlSetReceiveBufferLength,
		EControlFlowControlStatus,
#ifdef _DEBUG_DEVCOMM
		EControlDebugInfo
#endif
		};
public:
#ifndef __KERNEL_MODE__
	inline TInt Open(TInt aUnit);
	inline TVersion VersionRequired() const;
	inline void Read(TRequestStatus &aStatus,TDes8 &aDes);
	inline void Read(TRequestStatus &aStatus,TDes8 &aDes,TInt aLength);
	inline void ReadOneOrMore(TRequestStatus &aStatus,TDes8 &aDes);
	inline void ReadCancel();
	inline void Write(TRequestStatus &aStatus,const TDesC8 &aDes);
	inline void Write(TRequestStatus &aStatus,const TDesC8 &aDes,TInt aLength);
	inline void WriteCancel();
	inline void Break(TRequestStatus &aStatus,TInt aTime);
	inline void BreakCancel();
	inline void Config(TDes8 &aConfig);
	inline TInt SetConfig(const TDesC8 &aConfig);
	inline void Caps(TDes8 &aCaps);
	inline TUint Signals();
	inline void SetSignals(TUint aSetMask,TUint aClearMask);
	inline TInt QueryReceiveBuffer();
	inline void ResetBuffers();
	inline TInt ReceiveBufferLength();
	inline TInt SetReceiveBufferLength(TInt aLength);
	inline void NotifySignalChange(TRequestStatus& aStatus,TUint& aSignals,TUint aSignalMask=0x3F);
	inline void NotifySignalChangeCancel();
	inline void NotifyReceiveDataAvailable(TRequestStatus& aStatus);
	inline void NotifyReceiveDataAvailableCancel();
	inline void NotifyFlowControlChange(TRequestStatus& aStatus);
	inline void NotifyFlowControlChangeCancel();
	inline void GetFlowControlStatus(TFlowControl& aFlowControl);
	inline void NotifyConfigChange(TRequestStatus& aStatus, TDes8& aNewConfig);
	inline void NotifyConfigChangeCancel();
#ifdef _DEBUG_DEVCOMM
	inline void DebugInfo(TDes8 &aInfo);
#endif
#endif
	};

#include <d32comm.inl>
#endif
