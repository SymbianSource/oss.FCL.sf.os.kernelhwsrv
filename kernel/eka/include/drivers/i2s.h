// Copyright (c) 2008-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// e32\include\drivers\i2s.h
// 
// WARNING: This file contains some APIs which are internal and are subject
//          to change without notice. Such APIs should therefore not be used
//          outside the Kernel and Hardware Services package.
//

#ifndef __I2S_H__
#define __I2S_H__

#include <e32cmn.h> 
#include <e32def.h> 


/**
@publishedPartner
@prototype

The I2S interface configuration
*/
class TI2sConfigV01
	{
public:
	TInt iRole;
	TInt iType;
	};

typedef  TPckgBuf<TI2sConfigV01> TI2sConfigBufV01;

/**
@publishedPartner
@prototype

The I2S public API
*/
class I2s
   {
public:
	/**
	The role an interface plays in a bus configuration:
	- Master,
	- Slave
	*/
	enum TI2sInterfaceRole
		{
		EMaster,
		ESlave
		};


	/**
	The type of device this interface is with respect to data flow:
	- transmitter,
	- receiver,
	- bidirectional (by virtue of bidirectional data pin or separate pins for data input/output)
	- controller (only involved in synchronising data flow)
	*/
	enum TI2sInterfaceType
		{
		ETransmitter,
		EReceiver,
		EBidirectional,
		EController
		};


	/**
	I2S transfer directions:
	- receive,
	- transmit

	These values are bitmasks which can be OR-ed to make up a composite bitmask.
	*/
	enum TI2sDirection
		{
		ERx = 0x01,
		ETx = 0x02
		};

	/**
	I2S frame phase:
	- left,
	- right
	*/
	enum TI2sFramePhase
		{
		ELeft,
		ERight
		};

	/**
	I2S sampling rates:
	*/
	enum TI2sSamplingRate
		{
		// sparse enumeration
		E7_35KHz  = 100,
		E8KHz	  = 200,
		E8_82KHz  = 300,
		E9_6KHz	  = 400,
		E11_025KHz = 500,
		E12KHz	  = 600,
		E14_7KHz  = 700,
		E16KHz	  = 800,
		E22_05KHz = 900,
		E24KHz	  = 1000,
		E29_4KHz  = 1100,
		E32KHz	  = 1200,
		E44_1KHz  = 1300,
		E48KHz	  = 1400,
		E96KHz	  = 1500
		};

	/**
	I2S frame length:
	*/
	enum TI2sFrameLength
		{
		// sparse enumeration
		EFrame16Bit	= 16,
		EFrame24Bit	= 24,
		EFrame32Bit	= 32,
		EFrame48Bit	= 48,
		EFrame64Bit	= 64,
		EFrame96Bit	= 96,
		EFrame128Bit = 128
		};

	/**
	I2S Audio word length:
	*/
	enum TI2sSampleLength
		{
		// sparse enumeration
		ESample8Bit  = 8,
		ESample12Bit = 12,
		ESample16Bit = 16,
		ESample24Bit = 24,
		ESample32Bit = 32
		};

	/**
	I2S access mode flags:
	- Rx  full (register or FIFO, depending on access mode, for left or right frame phase)
	- Tx  empty (register or FIFO, depennding on access mode, for left or right frame phase)
	- Rx  overrun (register or FIFO, depending on access mode, for left or right frame phase)
	- Tx  underrun (register or FIFO, depending on access mode, for left or right frame phase)
	- Rx/Tx framing error

	These values are bitmasks which can be OR-ed to make up a composite bitmask.
	*/
	enum TI2sFlags
		{
		ERxFull		  = 0x01,
		ETxEmpty	  = 0x02,
		ERxOverrun	  = 0x04,
		ETxUnderrun	  = 0x08,
		EFramingError = 0x10
		};

	/**
	Configures the interface.

	@param aInterfaceId	The interface Id.
	@param aConfig		A pointer to the configuration as one of TI2sConfigBufV01 or greater.

	@return				KErrNone, if successful; 
						KErrArgument, if aInterfaceId is invalid or aConfig is NULL;
						KErrNotSupported, if the configuration is not supported by this interface;
						KErrInUse, if interface is not quiescient (a transfer is under way).
	@pre                Call from thread context (neither NULL thread nor DFC threads 0 or 1).
	*/
	IMPORT_C static TInt ConfigureInterface(TInt aInterfaceId, TDes8* aConfig);

	/**
	Reads the current configuration.

	@param aInterfaceId	The interface Id.
	@param aConfig		On return, the buffer passed is filled with the current configuration.

	@return				KErrNone, if successful; 
						KErrArgument, if aInterfaceId is invalid.
	@pre                Call from thread context (neither NULL thread nor DFC threads 0 or 1).
	*/
	IMPORT_C static TInt GetInterfaceConfiguration(TInt aInterfaceId, TDes8& aConfig);

	/**
	Sets the sampling rate.

	@param aInterfaceId	 The interface Id.
	@param aSamplingRate One of TI2sSamplingRate.

	@return 			KErrNone, if successful; 
						KErrArgument, if aInterfaceId is invalid;
						KErrNotSupported, if the sampling rate is not supported by this interface;
						KErrInUse, if interface is not quiescient (a transfer is under way).
	@pre                Call from thread context (neither NULL thread nor DFC threads 0 or 1).
	*/
	IMPORT_C static TInt SetSamplingRate(TInt aInterfaceId, TI2sSamplingRate aSamplingRate);

	/**
	Reads the sampling rate.

	@param aInterfaceId	 The interface Id.
	@param aSamplingRate On return, contains one of TI2sSamplingRate.

	@return 			KErrNone, if successful; 
						KErrArgument, if aInterfaceId is invalid.
	@pre                Call from thread context (neither NULL thread nor DFC threads 0 or 1).
	*/
	IMPORT_C static TInt GetSamplingRate(TInt aInterfaceId, TInt& aSamplingRate);


	/**
	Sets the frame length and format.

	@param aInterfaceId		  The interface Id.
	@param aFrameLength		  One of TI2sFrameLength.
	@param aLeftFramePhaseLength The length of the left frame phase (in number of data bits).

	@return 			KErrNone, if successful; 
						KErrArgument, if aInterfaceId is invalid;
						KErrNotSupported, if the frame length or format are not supported by this interface;
						KErrInUse, if interface is not quiescient (a transfer is under way).
	@pre                Call from thread context (neither NULL thread nor DFC threads 0 or 1).
				   
	The implementation calculates the Right frame phase length as (FrameLength - LeftFramePhaseLength)
	*/
	IMPORT_C static TInt SetFrameLengthAndFormat(TInt aInterfaceId, TI2sFrameLength aFrameLength, TInt aLeftFramePhaseLength);

	/**
	Reads the frame format.

	@param aInterfaceId			 The interface Id.
	@param aLeftFramePhaseLength  On return, contains the length of the left frame phase.
	@param aRightFramePhaseLength On return, contains the length of the right frame phase.

	@return 			KErrNone, if successful; 
						KErrArgument, if aInterfaceId is invalid.
	@pre                Call from thread context (neither NULL thread nor DFC threads 0 or 1).
	*/
	IMPORT_C static TInt GetFrameFormat(TInt aInterfaceId, TInt& aLeftFramePhaseLength, TInt& aRightFramePhaseLength);

	/**
	Sets the sample length for a frame phase (left or right).

	@param aInterfaceId	 The interface Id.
	@param aFramePhase	 One of TI2sFramePhase.
	@param aSampleLength One of TI2sSampleLength.

	@return 			KErrNone, if successful; 
						KErrArgument, if aInterfaceId is invalid;
						KErrNotSupported, if the sample length for the frame phase selected is not supported by this interface;
						KErrInUse, if interface is not quiescient (a transfer is under way).
	@pre                Call from thread context (neither NULL thread nor DFC threads 0 or 1).
	*/
	IMPORT_C static TInt SetSampleLength(TInt aInterfaceId, TI2sFramePhase aFramePhase, TI2sSampleLength aSampleLength);

	/**
	Reads the sample length for a frame phase (left or right).

	@param aInterfaceId	 The interface Id.
	@param aFramePhase	 One of TI2sFramePhase.
	@param aSampleLength On return, contains the sample length for the frame phase indicated by aFramePhase.

	@return 			KErrNone, if successful; 
						KErrArgument, if aInterfaceId is invalid.
	@pre                Call from thread context (neither NULL thread nor DFC threads 0 or 1).
	*/
	IMPORT_C static TInt GetSampleLength(TInt aInterfaceId, TI2sFramePhase aFramePhase, TInt& aSampleLength);

	/**
	Sets the number of delay cycles for a frame phase (left or right).

	@param aInterfaceId	The interface Id.
	@param aFramePhase	One of TI2sFramePhase.
	@param aDelayCycles The number of delay cycles to be introduced for the frame phase indicated by aFramePhase.

	@return 			KErrNone, if successful; 
						KErrArgument, if aInterfaceId is invalid;
						KErrNotSupported, if the number of delay cycles for the frame phase selected is not supported by this interface;
						KErrInUse, if interface is not quiescient (a transfer is under way).
	@pre                Call from thread context (neither NULL thread nor DFC threads 0 or 1).

	Each delay cycle has a duration of a bit clock cycle. Delay cycles are inserted between the start of the frame and the start of data.
	*/
	IMPORT_C static TInt SetDelayCycles(TInt aInterfaceId, TI2sFramePhase aFramePhase, TInt aDelayCycles);

	/**
	Reads the number of delay cycles for a frame phase (left or right).

	@param aInterfaceId	The interface Id.
	@param aFramePhase	One of TI2sFramePhase.
	@param aDelayCycles On return, contains the number of delay cycles for the frame phase indicated by aFramePhase.

	@return 			KErrNone, if successful; 
						KErrArgument, if aInterfaceId is invalid.
	@pre                Call from thread context (neither NULL thread nor DFC threads 0 or 1).
	*/
	IMPORT_C static TInt GetDelayCycles(TInt aInterfaceId, TI2sFramePhase aFramePhase, TInt& aDelayCycles);

	/**
	Reads the receive data register for a frame phase.

	@param aInterfaceId	The interface Id.
	@param aFramePhase	One of TI2sFramePhase.
	@param aData		On return, contains the receive data register contents.

	@return 			KErrNone, if successful; 
						KErrArgument, if aInterfaceId is invalid;
						KErrNotSupported, if reading the receive data register is not supported (e.g. when if DMA is enabled);
						KErrNotReady, if the interface is not ready.
	@pre                Can be called in any context.

	If the implementation has a combined receive/transmit register - half duplex operation only - this API is used to read from it.
	If the implementation only supports a single receive register for both frame phases, the aFramePhase argument shall be ignored and the 
	API shall return the contents of the single register. The user of the API shall use the ReadRegisterModeStatus()/ReadFIFOModeStatus()
	API to determine which frame phase the data corresponds to.
	*/
	IMPORT_C static TInt ReadReceiveRegister(TInt aInterfaceId, TI2sFramePhase aFramePhase, TInt& aData);

	/**
	Writes to the transmit data register for a frame phase.

	@param aInterfaceId	The interface Id.
	@param aFramePhase	One of TI2sFramePhase.
	@param aData		The data to be written.

	@return 			KErrNone, if successful; 
						KErrArgument, if aInterfaceId is invalid;
						KErrNotSupported, if writing to the receive data register is not supported (e.g. when if DMA is enabled);
						KErrNotReady, if the interface is not ready.
	@pre                Can be called in any context.

	If the implementation has a combined receive/transmit register - half duplex operation only - this API is used to write to it.
	If the implementation only supports a single transmit register for both frame phases, the aFramePhase argument shall be ignored and the 
	API shall write to the single register. The user of the API shall use the ReadRegisterModeStatus()/ReadFIFOModeStatus() API to determine
	under which frame phase the data corresponds will be transmitted.
	*/
	IMPORT_C static TInt WriteTransmitRegister(TInt aInterfaceId, TI2sFramePhase aFramePhase, TInt aData);

	/**
	Reads the transmit data register for a frame phase.

	@param aInterfaceId	The interface Id.
	@param aFramePhase	One of TI2sFramePhase.
	@param aData		On return, contains the transmit data register contents.

	@return 			KErrNone, if successful; 
						KErrArgument, if aInterfaceId is invalid;
						KErrNotSupported, if reading the transmit data register is not supported;
						KErrNotReady, if the interface is not ready.
	@pre                Can be called in any context.

	If the implementation has a combined receive/transmit register this API is used to read from it (equivalent to ReadReceiveRegister()).
	If the implementation only supports a single transmit register for both frame phases, the aFramePhase argument shall be ignored and the 
	API shall return the contents of the single register. The user of the API shall use the ReadRegisterModeStatus()/ReadFIFOModeStatus()
	API to determine which frame phase the data corresponds to.
	*/
	IMPORT_C static TInt ReadTransmitRegister(TInt aInterfaceId, TI2sFramePhase aFramePhase, TInt& aData);

	/**
	Reads the Register PIO access mode status flags for a frame phase.

	@param aInterfaceId	The interface Id.
	@param aFramePhase	One of TI2sFramePhase.
	@param aFlags		On return, contains a bitmask with the status flags for the frame phase selected (see TI2SFlags).
						A bit set to "1" indicates the condition described by the corresponding flag is occurring.

	@return 			KErrNone, if successful; 
						KErrArgument, if aInterfaceId is invalid;
						KErrNotSupported, if reading the status flags for Register PIO mode is not supported by this implementation.
	@pre                Can be called in any context.

	The client driver may use one of IS_I2S_<CONDITION> macros to determine the status of individual conditions.
	*/
	IMPORT_C static TInt ReadRegisterModeStatus(TInt aInterfaceId, TI2sFramePhase aFramePhase, TInt& aFlags);

	/**
	Enables Register PIO access mode related interrupts for a frame phase.

	@param aInterfaceId	The interface Id.
	@param aFramePhase	One of TI2sFramePhase.
	@param aInterrupt	A bitmask containing the relevant interrupt flags (see TI2sFlags).
						Bits set to "1" enable the corresponding interrupts.

	@return 			KErrNone, if successful; 
						KErrArgument, if aInterfaceId is invalid;
						KErrNotSupported, if one of the selected interrupt conditions cannot be generated by this implementation.
	@pre                Call from thread context (neither NULL thread nor DFC threads 0 or 1).

	If the implementation only supports single transmit and receive registers for both frame phases, the aFramePhase argument is 
	ignored.
	*/
	IMPORT_C static TInt EnableRegisterInterrupts(TInt aInterfaceId, TI2sFramePhase aFramePhase, TInt aInterrupt);

	/**
	Disables Register PIO access mode related interrupts for a frame phase.

	@param aInterfaceId	The interface Id.
	@param aFramePhase	One of TI2sFramePhase.
	@param aInterrupt	A bitmask containing the relevant interrupt flags (see TI2sFlags).
						Bits set to "1" disable the corresponding interrupts.

	@return 			KErrNone, if successful; 
						KErrArgument, if aInterfaceId is invalid;
						KErrNotSupported, if one of the selected interrupt conditions cannot be generated by this implementation.
	@pre                Call from thread context (neither NULL thread nor DFC threads 0 or 1).

	If the implementation only supports single transmit and receive registers for both frame phases, the aFramePhase argument is 
	ignored.
	*/
	IMPORT_C static TInt DisableRegisterInterrupts(TInt aInterfaceId, TI2sFramePhase aFramePhase, TInt aInterrupt);

	/**
	Reads the Register PIO access mode interrupt mask for a frame phase.

	@param aInterfaceId	The interface Id.
	@param aFramePhase	One of TI2sFramePhase.
	@param aEnabled		On return, contains a bitmask with the interrupts which are enabled for the frame phase selected (see TI2SFlags).
						A bit set to "1" indicates the corresponding interrupt is enabled.

	@return				KErrNone, if successful; 
						KErrArgument, if aInterfaceId is invalid;
						KErrNotSupported, if one of the selected interrupt conditions cannot be generated by this implementation.
	@pre                Can be called in any context.

	If the implementation only supports single transmit and receive registers for both frame phases, the aFramePhase argument is 
	ignored.
	*/
	IMPORT_C static TInt IsRegisterInterruptEnabled(TInt aInterfaceId, TI2sFramePhase aFramePhase, TInt& aEnabled);

	/**
	Enables receive and/or transmit FIFO on a per frame phase basis.

	@param aInterfaceId	The interface Id.
	@param aFramePhase	One of TI2sFramePhase.
	@param aFifoMask	A bitmask specifying which FIFO direction(s) - receive and/or transmit - are to be enabled for the frame 
						phase selected (see TI2sDirection).
						Bits set to "1" enable the corresponding FIFO.

	@return				KErrNone, if successful; 
						KErrArgument, if aInterfaceId is invalid;
						KErrNotSupported, if the implementation does no support FIFOs.
	@pre                Call from thread context (neither NULL thread nor DFC threads 0 or 1).

	If the implementation has a combined receive/transmit FIFO - half duplex operation only - then aFifoMask is ignored.
	If the implementation only supports a single FIFO for both frame phases then aFramePhase is ignored.
	*/
	IMPORT_C static TInt EnableFIFO(TInt aInterfaceId, TI2sFramePhase aFramePhase, TInt aFifoMask);

	/**
	Disables receive and/or transmit FIFO on a per frame phase basis.

	@param aInterfaceId	The interface Id.
	@param aFramePhase	One of TI2sFramePhase.
	@param aFifoMask	A bitmask specifying which FIFO direction(s) - receive and/or transmit - are to be disabled for the frame 
						phase selected (see TI2sDirection).
						Bits set to "1" disable the corresponding FIFO.

	@return 			KErrNone, if successful; 
						KErrArgument, if aInterfaceId is invalid;
						KErrNotSupported, if the implementation does no support FIFOs.
	@pre                Call from thread context (neither NULL thread nor DFC threads 0 or 1).

	If the implementation has a combined receive/transmit FIFO - half duplex operation only - then aFifoMask is ignored.
	If the implementation only supports a single FIFO for both frame phases then aFramePhase is ignored.
	*/
	IMPORT_C static TInt DisableFIFO(TInt aInterfaceId, TI2sFramePhase aFramePhase, TInt aFifoMask);

	/**
	Reads the enabled state of a frame phase's FIFOs.

	@param aInterfaceId	The interface Id.
	@param aFramePhase	One of TI2sFramePhase.
	@param aEnabled		On return, contains a bitmask indicating which FIFOs which are enabled for the frame phase selected (see TI2sDirection).
						A bit set to "1" indicates the corresponding FIFO is enabled.

	@return 			KErrNone, if successful; 
						KErrArgument, if aInterfaceId is invalid;
						KErrNotSupported, if the implementation does no support FIFOs.
	@pre                Call from thread context (neither NULL thread nor DFC threads 0 or 1).

	If the implementation has a combined receive/transmit FIFO - half duplex operation only - then aEnabled will have 
	both Rx and Tx bits set when the FIFO is enabled.
	If the implementation only supports a single FIFO for both frame phases then aFramePhase is ignored.
	*/
	IMPORT_C static TInt IsFIFOEnabled(TInt aInterfaceId, TI2sFramePhase aFramePhase, TInt& aEnabled);

	/**
	Sets the receive or transmit FIFO threshold on a per frame phase basis.

	@param aInterfaceId	The interface Id.
	@param aFramePhase	One of TI2sFramePhase.
	@param aDirection	One of TDirection.
	@param aThreshold	A threshold level at which a receive FIFO is considered full or a transmit FIFO is considered empty.

	@return 			KErrNone, if successful; 
						KErrArgument, if aInterfaceId is invalid;
						KErrNotSupported, if the implementation does no support FIFOs;
						KErrOverflow if the threshold level requested exceeds the FIFO length (or the admissible highest level allowed);
						KErrUnderflow if the threshold level requested is less than the minimum threshold allowed.
	@pre                Call from thread context (neither NULL thread nor DFC threads 0 or 1).

	If the implementation has a combined receive/transmit FIFO - half duplex operation only - then aDirection is ignored.
	If the implementation only supports a single FIFO for both frame phases then aFramePhase is ignored.
	*/
	IMPORT_C static TInt SetFIFOThreshold(TInt aInterfaceId, TI2sFramePhase aFramePhase, TI2sDirection aDirection, TInt aThreshold);

	/**
	Reads the FIFO PIO access mode status flags for a frame phase.

	@param aInterfaceId	The interface Id.
	@param aFramePhase	One of TI2sFramePhase.
	@param aFlags		On return, contains a bitmask with the status flags for the frame phase selected (see TI2sFlags).
						A bit set to "1" indicates the condition described by the corresponding flag is occurring.

	@return 			KErrNone, if successful; 
						KErrArgument, if aInterfaceId is invalid;
						KErrNotSupported, if reading the status flags for FIFO PIO mode is not supported by this implementation.
						KErrInUse, if interface is not quiescient (a transfer is under way).
	@pre                Can be called in any context.

	The client driver may use one of IS_I2S_<CONDITION> macros to determine the status of individual conditions.
	If the implementation has a combined receive/transmit FIFO - half duplex operation only - then aFlags will be set according
	to which operation (receive/transmit) is undergoing.
	If the implementation only supports a single FIFO for both frame phases then aFramePhase is ignored.
	*/
	IMPORT_C static TInt ReadFIFOModeStatus(TInt aInterfaceId, TI2sFramePhase aFramePhase, TInt& aFlags);

	/**
	Enables FIFO related interrupts for a frame phase.

	@param aInterfaceId	The interface Id.
	@param aFramePhase	One of TI2sFramePhase.
	@param aInterrupt	A bitmask containing the relevant interrupt flags (see TI2sFlags).
						Bits set to "1" enable the corresponding interrupts.

	@return 			KErrNone, if successful; 
						KErrArgument, if aInterfaceId is invalid;
						KErrNotSupported, if one of the selected interrupt conditions cannot be generated by this implementation.
	@pre                Call from thread context (neither NULL thread nor DFC threads 0 or 1).

	If the implementation only supports single transmit and receive FIFO for both frame phases, the aFramePhase argument is 
	ignored.
	*/
	IMPORT_C static TInt EnableFIFOInterrupts(TInt aInterfaceId, TI2sFramePhase aFramePhase, TInt aInterrupt);

	/**
	Disables FIFO related interrupts for a frame phase.

	@param aInterfaceId	The interface Id.
	@param aFramePhase	One of TI2sFramePhase.
	@param aInterrupt	A bitmask containing the relevant interrupt flags (see TI2sFlags).
						Bits set to "1" disable the corresponding interrupts.

	@return				KErrNone, if successful; 
						KErrArgument, if aInterfaceId is invalid;
						KErrNotSupported, if one of the selected interrupt conditions cannot be generated by this implementation.
	@pre                Call from thread context (neither NULL thread nor DFC threads 0 or 1).

	If the implementation only supports single transmit and receive FIFO for both frame phases, the aFramePhase argument is 
	ignored.
	*/
	IMPORT_C static TInt DisableFIFOInterrupts(TInt aInterfaceId, TI2sFramePhase aFramePhase, TInt aInterrupt);

	/**
	Reads the FIFO interrupt masks for a frame phase.

	@param aInterfaceId	The interface Id.
	@param aFramePhase	One of TI2sFramePhase.
	@param aEnabled		On return, contains a bitmask with the interrupts which are enabled for the frame phase selected (see TI2sFlags).
						A bit set to "1" indicates the corresponding interrupt is enabled.

	@return 			KErrNone, if successful; 
						KErrArgument, if aInterfaceId is invalid;
						KErrNotSupported, if one of the selected interrupt conditions cannot be generated by this implementation.
	@pre                Can be called in any context.
	*/
	IMPORT_C static TInt IsFIFOInterruptEnabled(TInt aInterfaceId, TI2sFramePhase aFramePhase, TInt& aEnabled);

	/**
	Reads the receive or transmit FIFO current level on a per frame phase basis.

	@param aInterfaceId	The interface Id.
	@param aFramePhase	One of TI2sFramePhase.
	@param aDirection	One of TDirection.
	@param aLevel		On return, contains the current level for the FIFO described by the (aFramePhase,aDirection) pair.

	@return 			KErrNone, if successful; 
						KErrArgument, if aInterfaceId is invalid;
						KErrNotSupported, if the implementation does no support FIFOs.
	@pre                Call from thread context (neither NULL thread nor DFC threads 0 or 1).

	If the implementation has a combined receive/transmit FIFO - half duplex operation only - then aDirection is ignored.
	If the implementation only supports a single FIFO for both frame phases then aFramePhase is ignored.
	*/
	IMPORT_C static TInt ReadFIFOLevel(TInt aInterfaceId, TI2sFramePhase aFramePhase, TI2sDirection aDirection, TInt& aLevel);

	/**
	Enables receive and/or transmit DMA.

	@param aInterfaceId	The interface Id.
	@param aFifoMask	A bitmask specifying which directions - receive and/or transmit - is DMA to be enabled (see TI2sDirection).
						Bits set to "1" enable DMA.

	@return 			KErrNone, if successful; 
						KErrArgument, if aInterfaceId is invalid;
						KErrNotSupported, if the implementation does no support DMA.
	@pre                Call from thread context (neither NULL thread nor DFC threads 0 or 1).

	If the implementation has a combined receive/transmit FIFO - half duplex operation only - then aFifoMask is ignored.
	*/
	IMPORT_C static TInt EnableDMA(TInt aInterfaceId, TInt aFifoMask);

	/**
	Disables receive and/or transmit DMA.

	@param aInterfaceId	The interface Id.
	@param aFifoMask	A bitmask specifying which directions - receive and/or transmit - is DMA to be disabled (see TI2sDirection).
						Bits set to "1" disable DMA.

	@return 			KErrNone, if successful; 
						KErrArgument, if aInterfaceId is invalid;
						KErrNotSupported, if the implementation does no support DMA.
	@pre                Call from thread context (neither NULL thread nor DFC threads 0 or 1).

	If the implementation has a combined receive/transmit FIFO - half duplex operation only - then aFifoMask is ignored.
	*/
	IMPORT_C static TInt DisableDMA(TInt aInterfaceId, TInt aFifoMask);

	/**
	Reads the enabled state of DMA.

	@param aInterfaceId	The interface Id.
	@param aEnabled		On return, contains a bitmask indicating if DMA enabled for the corresponding directions (see TI2sDirection).
						A bit set to "1" indicates DMA is enabled for the corresponding direction.

	@return 			KErrNone, if successful; 
						KErrArgument, if aInterfaceId is invalid;
						KErrNotSupported, if the implementation does no support FIFOs.
	@pre                Call from thread context (neither NULL thread nor DFC threads 0 or 1).

	If the implementation has a combined receive/transmit FIFO - half duplex operation only - then aEnabled will have 
	both Rx and Tx bits set when the DMA is enabled.
	*/
	IMPORT_C static TInt IsDMAEnabled(TInt aInterfaceId, TInt& aEnabled);

	/**
	Starts data transmission and/or data reception unless interface is a Controller;
	if device is also a Master, starts generation of data synchronisation signals.

	@param aInterfaceId	The interface Id.
	@param aDirection	A bitmask made of TI2sDirection values. The value is ignored if interface is a Controller.

	@return 			KErrNone, if successful; 
						KErrArgument, if aInterfaceId is invalid or if aDirection  is invalid (i.e. negative, 0 or greater than 3);
						KErrNotSupported, if one of the transfer directions selected is not supported on this interface;
						KErrInUse, if interface has a bidirectional data port and an access in the opposite direction is underway;
						KErrNotReady, if interface is not ready (e.g. incomplete configuration).
	@pre                Call from thread context (neither NULL thread nor DFC threads 0 or 1).

	Start() is idempotent, attempting to start an already started interface has no effect (returns KErrNone).
	*/
	IMPORT_C static TInt Start(TInt aInterfaceId, TInt aDirection);

	/**
	Stops data transmission and/or data reception;
	if device is also a Master, stops generation of data synchronisation signals.

	@param aInterfaceId	The interface Id.
	@param aDirection	A bitmask made of TI2sDirection values.

	@return 			KErrNone, if successful; 
						KErrArgument, if aInterfaceId is invalid or if aDirection  is invalid (i.e. negative, 0 or greater than 3);
						KErrNotSupported, if one of the transfer directions selected is not supported on this interface.
	@pre                Call from thread context (neither NULL thread nor DFC threads 0 or 1).

	Stop() is idempotent, attempting to stop an already started interface has no effect (returns KErrNone).
	*/
	IMPORT_C static TInt Stop(TInt aInterfaceId, TInt aDirection);

	/**
	Checks if a transmission or a reception is underway.

	@param aInterfaceId	The interface Id.
	@param aDirection	One of TI2sDirection.
	@param aStarted 	On return, contains ETrue if the the access is underway, EFalse otherwise.

	@return 			KErrNone, if successful; 
						KErrArgument, if aInterfaceId is invalid or if aDirection  is invalid (i.e. negative, 0 or greater than 3);
						KErrNotSupported, if one of the transfer directions selected is not supported on this interface.
	@pre                Can be called in any context.

	If the interface is a Controller and a bus operation is underway, ETrue is returned regardless of aDirection.
	*/
	IMPORT_C static TInt IsStarted(TInt aInterfaceId, TI2sDirection aDirection, TBool& aStarted);
	};

#define IS_I2S_RX_FULL(status)	(status&I2s::ERxFull)
#define IS_I2S_TX_EMPTY(status)	(status&I2s::ETxEmpty)
#define IS_I2S_RX_OVERRUN(status)	(status&I2s::ERxOverrun)
#define IS_I2S_TX_UNDERRUN(status)	(status&I2s::ETxUnderrun)
#define IS_I2S_FRAMING_ERROR(status)	(status&I2s::EFramingError)

#endif /* __I2S_H__ */

