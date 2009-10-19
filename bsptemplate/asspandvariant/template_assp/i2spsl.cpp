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
// template\template_assp\i2spsl.cpp
// 
//

#include <kernel/kernel.h>
#include <drivers/i2s.h>

// TO DO: (mandatory)
// If your ASIC supports multiple I2S interfaces you need to design the most appropriate way of handling that:
//	- it is possible that a common register per function is used on some of the functions, e.g. a single Control
//	  Register is used to select Master/Slave roles, Transmitter/Receiver/Bidirectional/Controller mode, word 
//	  length etc for all interfaces supported. In this case handling the interface Id typically involves the use
//	  of shifts and masks;
//	- some functions can never be covered by a single register common to all interfaces (e.g. the transmit/receive 
//	  registers). Even if it was possible to use single registers to cover a number of interfaces the ASIC designer
//	  may decide to have separate registers for each interface. In this case each of the below APIs could be implemented
//	  as a switch(interface)-case and then use different sets of register addresses for each interface. This model makes
//	  sense when a single developer is responsible for implementing all interfaces (typically in a single source file).
//	- when each interface is implemented independently it makes sense to separate the implementation into a interface 
//	  independent layer and a specific layer and redirect each call from the interface independent layer into the relavant
//	  interface. This is exemplified with the NAVIENGINE implementation.
//

enum TIs2Panic
	{
	ECalledFromIsr
	};

EXPORT_C TInt I2s::ConfigureInterface(TInt aInterfaceId, TDes8* aConfig)
//
// Configures the interface: its type (Transmitter/Receiver/Bidirectional/Controller) and the role played by it (Master/Slave).
//
	{
	__ASSERT_DEBUG(NKern::CurrentContext() == NKern::EThread, Kern::Fault("I2s Interface", ECalledFromIsr));
	// TO DO: (mandatory)
	//
	// Extracts the configuration information from aConfig and programs the relevant registers for the interface identified by aInterfaceId.
	//
	return KErrNone;
	}

EXPORT_C TInt I2s::GetInterfaceConfiguration(TInt aInterfaceId, TDes8& aConfig)
//
// Reads the current configuration.
//
	{
	__ASSERT_DEBUG(NKern::CurrentContext() == NKern::EThread, Kern::Fault("I2s Interface", ECalledFromIsr));
	// TO DO: (optional)
	//
	// Reads the relevant registers and assembles configuration information to be returned in aConfig.
	//
	return KErrNotSupported;
	}

EXPORT_C TInt I2s::SetSamplingRate(TInt aInterfaceId, TI2sSamplingRate aSamplingRate)
//
// Sets the sampling rate.
//
	{
	__ASSERT_DEBUG(NKern::CurrentContext() == NKern::EThread, Kern::Fault("I2s Interface", ECalledFromIsr));
	// TO DO: (mandatory)
	//
	// Programs the required sampling rate onto the relevant registers for the interface identified by aInterfaceId .
	//
	return KErrNone;
	}

EXPORT_C TInt I2s::GetSamplingRate(TInt aInterfaceId, TInt& aSamplingRate)
//
// Reads the sampling rate.
//
	{
	__ASSERT_DEBUG(NKern::CurrentContext() == NKern::EThread, Kern::Fault("I2s Interface", ECalledFromIsr));
	// TO DO: (optional)
	//
	// Reads the relevant registers to obtain the currently programmed sampling rate to be returned in aSamplingRate.
	//
	return KErrNotSupported;
	}

EXPORT_C TInt I2s::SetFrameLengthAndFormat(TInt aInterfaceId, TI2sFrameLength aFrameLength, TInt aLeftFramePhaseLength)
//
// Sets the frame format.
//
	{
	__ASSERT_DEBUG(NKern::CurrentContext() == NKern::EThread, Kern::Fault("I2s Interface", ECalledFromIsr));
	// TO DO: (mandatory)
	//
	// If the interface only allows symmetrical frame lengths this function programs the required
	// overall frame length onto the relevant registers for the interface identified by aInterfaceId.
	// In this case aLeftFramePhaseLength can be ignored.
	// If the interface supports asymmetrical frame lengths, calculates the righ frame length as
	// (aFrameLength-aLeftFramePhaseLength) and programs both the left and right frame lengths onto
	// the relevant registers for the interface identified by aInterfaceId.
	//
	return KErrNone;
	}

EXPORT_C TInt I2s::GetFrameFormat(TInt aInterfaceId, TInt& aLeftFramePhaseLength, TInt& aRightFramePhaseLength)
//
// Reads the frame format.
//
	{
	__ASSERT_DEBUG(NKern::CurrentContext() == NKern::EThread, Kern::Fault("I2s Interface", ECalledFromIsr));
	// TO DO: (optional)
	//
	// If the interface only supports symmetrical frame lengths this function reads the relevant registers to obtain 
	// the currently programmed overall frame length for the interface identified by aInterfaceId: it returns the same
	// value in both aLeftFramePhaseLength and aRightFramePhaseLength (that is overal frame length/2).
	// If the interface supports asymmetrical frame lngths, reads the appropriate registers to obtain the left and right
	// frame lengths to be returned in aLeftFramePhaseLength and aRightFramePhaseLength.
	//
	return KErrNotSupported;
	}

EXPORT_C TInt I2s::SetSampleLength(TInt aInterfaceId, TI2sFramePhase aFramePhase, TI2sSampleLength aSampleLength)
//
// Sets the sample length for a frame phase.
//
	{
	__ASSERT_DEBUG(NKern::CurrentContext() == NKern::EThread, Kern::Fault("I2s Interface", ECalledFromIsr));
	// TO DO: (mandatory)
	//
	// Programs the required sample length for the frame phase specified (left or right) onto the relevant registers for the interface identified by aInterfaceId .
	//
	return KErrNone;
	}

EXPORT_C TInt I2s::GetSampleLength(TInt aInterfaceId, TI2sFramePhase aFramePhase, TInt& aSampleLength)
//
// Reads the sample length for a frame phase.
//
	{
	__ASSERT_DEBUG(NKern::CurrentContext() == NKern::EThread, Kern::Fault("I2s Interface", ECalledFromIsr));
	// TO DO: (optional)
	//
	// Reads the relevant registers to obtain the sample length for the frame phase specified (left or right) to be returned in aSampleLength.
	//
	return KErrNotSupported;
	}

EXPORT_C TInt I2s::SetDelayCycles(TInt aInterfaceId, TI2sFramePhase aFramePhase, TInt aDelayCycles)
//
// Sets the number of delay cycles for a frame phase.
//
	{
	__ASSERT_DEBUG(NKern::CurrentContext() == NKern::EThread, Kern::Fault("I2s Interface", ECalledFromIsr));
	// TO DO: (optional)
	//
	// If the interface supports delaying the start of a frame by a specified number of bit clock cycles this function programs the required
	// delay cycles for the frame phase specified (left or right) onto the relevant registers for the interface identified by aInterfaceId .
	//
	return KErrNotSupported;
	}

EXPORT_C TInt I2s::GetDelayCycles(TInt aInterfaceId, TI2sFramePhase aFramePhase, TInt& aDelayCycles)
//
// Reads the sample length for a frame phase.
//
	{
	__ASSERT_DEBUG(NKern::CurrentContext() == NKern::EThread, Kern::Fault("I2s Interface", ECalledFromIsr));
	// TO DO: (optional)
	//
	// If the interface supports delaying the start of a frame by a specified number of bit clock cycles this function reads the relevant
	// registers to obtain the number of delay cycles for the frame phase specified (left or right) to be returned in aSampleLength.
	//
	return KErrNotSupported;
	}

EXPORT_C TInt I2s::ReadReceiveRegister(TInt aInterfaceId, TI2sFramePhase aFramePhase, TInt& aData)
//
// Reads the receive data register for a frame phase.
//
	{
	// TO DO: (mandatory)
	//
	// Reads the contents of the receive register to obtain the data for the frame phase specified (left or right) to be returned in aData.
	// If the implementation only supports a single receive register for both frame phases, the aFramePhase argument can be ignored and the 
	// function returns the contents of the single register.
	//
	return KErrNone;
	}

EXPORT_C TInt I2s::WriteTransmitRegister(TInt aInterfaceId, TI2sFramePhase aFramePhase, TInt aData)
//
// Writes to the transmit data register for a frame phase.
//
	{
	// TO DO: (mandatory)
	//
	// Writes the Audio data passed in aData to the transmit register for the frame phase specified (left or right) for the interface identified
	// by aInterfaceId.
	// If the implementation only supports a single transmit register for both frame phases, the aFramePhase argument can be ignored and the 
	// function writes to the single register.
	//
	return KErrNone;
	}

EXPORT_C TInt I2s::ReadTransmitRegister(TInt aInterfaceId, TI2sFramePhase aFramePhase, TInt& aData)
//
// Reads the transmit data register for a frame phase.
//
	{
	// TO DO: (optional)
	//
	// Reads the contents of the transmit register to obtain the data for the frame phase specified (left or right) to be returned in aData.
	// If the implementation only supports a single receive register for both frame phases, the aFramePhase argument can be ignored and the 
	// function returns the contents of the single transmit register.
	// If the implementation does not support reading the transmit register simply return KErrNotSupported.
	//
	return KErrNotSupported;
	}

EXPORT_C TInt I2s::ReadRegisterModeStatus(TInt aInterfaceId, TI2sFramePhase aFramePhase, TInt& aFlags)
//
// Reads the Register PIO access mode status flags for a frame phase.
//
	{
	// TO DO: (optional)
	//
	// If the implementation supports Register PIO mode this function reads the contents of the Register PIO mode status register to obtain
	// the status flags  for the frame phase specified (left or right) to be returned in aFlags. The mode flags are described in TI2sFlags.
	// If the implementation does not support Register PIO mode  simply return KErrNotSupported.
	//
	return KErrNotSupported;
	}

EXPORT_C TInt I2s::EnableRegisterInterrupts(TInt aInterfaceId, TI2sFramePhase aFramePhase, TInt aInterrupt)
//
// Enables Register PIO access mode related interrupts for a frame phase.
//
	{
	__ASSERT_DEBUG(NKern::CurrentContext() == NKern::EThread, Kern::Fault("I2s Interface", ECalledFromIsr));
	// TO DO: (optional)
	//
	// If the implementation supports Register PIO mode this function enables the mode interrupts specified by the bitmask aInterrupt
	// for the frame phase specified (left or right). The mode interrupts are described in TI2sFlags. Bits set to "1" enable the 
	// corresponding interrupts
	// If the implementation only supports a single transmit register for both frame phases, the aFramePhase argument can be ignored.
	// If the implementation does not support Register PIO mode  simply return KErrNotSupported.
	//
	return KErrNotSupported;
	}

EXPORT_C TInt I2s::DisableRegisterInterrupts(TInt aInterfaceId, TI2sFramePhase aFramePhase, TInt aInterrupt)
//
// Disables Register PIO access mode related interrupts for a frame phase.
//
	{
	__ASSERT_DEBUG(NKern::CurrentContext() == NKern::EThread, Kern::Fault("I2s Interface", ECalledFromIsr));
	// TO DO: (optional)
	//
	// If the implementation supports Register PIO mode this function disables the mode interrupts specified by the bitmask aInterrupt
	// for the frame phase specified (left or right). The mode interrupts are described in TI2sFlags. Bits set to "1" disable the 
	// corresponding interrupts
	// If the implementation only supports a single transmit register for both frame phases, the aFramePhase argument can be ignored.
	// If the implementation does not support Register PIO mode  simply return KErrNotSupported.
	//
	return KErrNotSupported;
	}

EXPORT_C TInt I2s::IsRegisterInterruptEnabled(TInt aInterfaceId, TI2sFramePhase aFramePhase, TInt& aEnabled)
//
// Reads the Register PIO access mode interrupt mask for a frame phase.
//
	{
	// TO DO: (optional)
	//
	// If the implementation supports Register PIO mode this function reads the relevant registers to find out which mode interrupts 
	// are enabled for the frame phase specified (left or right), and returns a bitmask of enabled interrupts in aEnabled.
	// The mode interrupts are described in TI2sFlags. A bit set to "1" indicates the corresponding interrupt is enabled 
	// If the implementation only supports a single transmit register for both frame phases, the aFramePhase argument can be ignored.
	// If the implementation does not support Register PIO mode  simply return KErrNotSupported.
	//
	return KErrNotSupported;
	}

EXPORT_C TInt I2s::EnableFIFO(TInt aInterfaceId, TI2sFramePhase aFramePhase, TInt aFifoMask)
//
// Enables receive and/or transmit FIFO on a per frame phase basis.
//
	{
	__ASSERT_DEBUG(NKern::CurrentContext() == NKern::EThread, Kern::Fault("I2s Interface", ECalledFromIsr));
	// TO DO: (optional)
	//
	// If the implementation supports FIFO mode this function enables the FIFOs for the directions specified in the bitmask aFifoMask
	// (Transmit and/or Receive) for the frame phase specified (left or right). Bits set to "1" enable the corresponding FIFO.
	// If the implementation has a combined receive/transmit FIFO - half duplex operation only - then aFifoMask can be ignored.
	// If the implementation only supports a single FIFO for both frame phases then aFramePhase can be ignored.
	//
	return KErrNotSupported;
	}

EXPORT_C TInt I2s::DisableFIFO(TInt aInterfaceId, TI2sFramePhase aFramePhase, TInt aFifoMask)
//
// Disables receive and/or transmit FIFO on a per frame phase basis.
//
	{
	__ASSERT_DEBUG(NKern::CurrentContext() == NKern::EThread, Kern::Fault("I2s Interface", ECalledFromIsr));
	// TO DO: (optional)
	//
	// If the implementation supports FIFO mode this function disables the FIFOs for the directions specified in the bitmask aFifoMask
	// (Transmit and/or Receive) for the frame phase specified (left or right). Bits set to "1" disable the corresponding FIFO.
	// If the implementation has a combined receive/transmit FIFO - half duplex operation only - then aFifoMask can be ignored.
	// If the implementation only supports a single FIFO for both frame phases then aFramePhase can be ignored.
	//
	return KErrNotSupported;
	}

EXPORT_C TInt I2s::IsFIFOEnabled(TInt aInterfaceId, TI2sFramePhase aFramePhase, TInt& aEnabled)
//
// Reads the enabled state of a frame phase's FIFOs.
//
	{
	__ASSERT_DEBUG(NKern::CurrentContext() == NKern::EThread, Kern::Fault("I2s Interface", ECalledFromIsr));
	// TO DO: (optional)
	//
	// If the implementation supports FIFO mode this function reads the relevant registers to find out which FIFOs 
	// are enabled (Transmit and/or Receive FIFO) for the frame phase specified (left or right), and returns a bitmask of enabled FIFOs in aEnabled.
	// The mode interrupts are described in TI2sFlags. A bit set to "1" indicates the corresponding interrupt is enabled 
	// If the implementation has a combined receive/transmit FIFO then aEnabled should have both Rx and Tx bits set when the FIFO is enabled.
	// If the implementation only supports a single FIFO for both frame phases then aFramePhase is ignore.
	//
	return KErrNotSupported;
	}

EXPORT_C TInt I2s::SetFIFOThreshold(TInt aInterfaceId, TI2sFramePhase aFramePhase, TI2sDirection aDirection, TInt aThreshold)
//
// Sets the receive or transmit FIFO threshold on a per frame phase basis.
//
	{
	__ASSERT_DEBUG(NKern::CurrentContext() == NKern::EThread, Kern::Fault("I2s Interface", ECalledFromIsr));
	// TO DO: (optional)
	//
	// If the implementation supports FIFO mode this function sets the FIFO threshold for the direction specified in aDirection
	// (Transmit or Receive) for the frame phase specified (left or right).
	// If the implementation has a combined receive/transmit FIFO - half duplex operation only - then aDirection can be ignored.
	// If the implementation only supports a single FIFO for both frame phases then aFramePhase can be ignored.
	//
	return KErrNotSupported;
	}

EXPORT_C TInt I2s::ReadFIFOModeStatus(TInt aInterfaceId, TI2sFramePhase aFramePhase, TInt& aFlags)
//
// Reads the FIFO PIO access mode status flags for a frame phase.
//
	{
	// TO DO: (optional)
	//
	// If the implementation supports FIFO mode this function reads the contents of the FIFO mode status register to obtain
	// the status flags for the frame phase specified (left or right) to be returned in aFlags. The mode flags are described in TI2sFlags.
	// A bit set to "1" indicates the condition described by the corresponding flag is occurring.
	// If the implementation has a combined receive/transmit FIFO then aFlags should be set according to which operation (receive/transmit) is 
	// currently undergoing.
	// If the implementation only supports a single FIFO for both frame phases then aFramePhase is ignored.
	//
	return KErrNotSupported;
	}

EXPORT_C TInt I2s::EnableFIFOInterrupts(TInt aInterfaceId, TI2sFramePhase aFramePhase, TInt aInterrupt)
//
// Enables FIFO related interrupts for a frame phase.
//
	{
	__ASSERT_DEBUG(NKern::CurrentContext() == NKern::EThread, Kern::Fault("I2s Interface", ECalledFromIsr));
	// TO DO: (optional)
	//
	// If the implementation supports FIFO mode this function enables the mode interrupts specified by the bitmask aInterrupt
	// for the frame phase specified (left or right). The mode interrupts are described in TI2sFlags. Bits set to "1" enable the 
	// corresponding interrupts
	// If the implementation only supports a single transmit FIFO for both frame phases, the aFramePhase argument can be ignored.
	//
	return KErrNotSupported;
	}

EXPORT_C TInt I2s::DisableFIFOInterrupts(TInt aInterfaceId, TI2sFramePhase aFramePhase, TInt aInterrupt)
//
// Disables FIFO related interrupts for a frame phase.
//
	{
	__ASSERT_DEBUG(NKern::CurrentContext() == NKern::EThread, Kern::Fault("I2s Interface", ECalledFromIsr));
	// TO DO: (optional)
	//
	// If the implementation supports FIFO mode this function disables the mode interrupts specified by the bitmask aInterrupt
	// for the frame phase specified (left or right). The mode interrupts are described in TI2sFlags. Bits set to "1" disable the 
	// corresponding interrupts
	// If the implementation only supports a single transmit FIFO for both frame phases, the aFramePhase argument can be ignored.
	//
	return KErrNotSupported;
	}

EXPORT_C TInt I2s::IsFIFOInterruptEnabled(TInt aInterfaceId, TI2sFramePhase aFramePhase, TInt& aEnabled)
//
// Reads the FIFO interrupt masks for a frame phase.
//
	{
	// TO DO: (optional)
	//
	// If the implementation supports FIFO mode this function reads the relevant registers to find out which mode interrupts 
	// are enabled for the frame phase specified (left or right), and returns a bitmask of enabled interrupts in aEnabled.
	// The mode interrupts are described in TI2sFlags. A bit set to "1" indicates the corresponding interrupt is enabled 
	// If the implementation only supports a single transmit FIFO for both frame phases, the aFramePhase argument can be ignored.
	//
	return KErrNotSupported;
	}

EXPORT_C TInt I2s::ReadFIFOLevel(TInt aInterfaceId, TI2sFramePhase aFramePhase, TI2sDirection aDirection, TInt& aLevel)
//
// Reads the receive or transmit FIFO current level on a per frame phase basis.
//
	{
	__ASSERT_DEBUG(NKern::CurrentContext() == NKern::EThread, Kern::Fault("I2s Interface", ECalledFromIsr));
	// TO DO: (optional)
	//
	// If the implementation supports FIFO mode this function reads the relevant registers to find out the current FIFO level 
	// for the direction specified and for the frame phase specified (left or right), and returns it in aLevel.
	// If the implementation has a combined receive/transmit FIFO then aDirection is ignored.
	// If the implementation only supports a single transmit FIFO for both frame phases, the aFramePhase argument can be ignored.
	//
	return KErrNotSupported;
	}

EXPORT_C TInt I2s::EnableDMA(TInt aInterfaceId, TInt aFifoMask)
//
// Enables receive and/or transmit DMA.
//
	{
	__ASSERT_DEBUG(NKern::CurrentContext() == NKern::EThread, Kern::Fault("I2s Interface", ECalledFromIsr));
	// TO DO: (optional)
	//
	// If the implementation supports FIFO DMA mode this function enables DMA in the directions (Transmit and/or Receive) specified
	// by the bitmask aFifoMask for the frame phase specified (left or right). Bits set to "1" enable DMA.
	// If the implementation has a combined receive/transmit FIFO then aFifoMask can be ignored.
	//
	return KErrNotSupported;
	}

EXPORT_C TInt I2s::DisableDMA(TInt aInterfaceId, TInt aFifoMask)
//
// Disables receive and/or transmit DMA.
//
	{
	__ASSERT_DEBUG(NKern::CurrentContext() == NKern::EThread, Kern::Fault("I2s Interface", ECalledFromIsr));
	// TO DO: (optional)
	//
	// If the implementation supports FIFO DMA mode this function disables DMA in the directions (Transmit and/or Receive) specified
	// by the bitmask aFifoMask for the frame phase specified (left or right). Bits set to "1" disable DMA.
	// If the implementation has a combined receive/transmit FIFO then aFifoMask can be ignored.
	//
	return KErrNotSupported;
	}

EXPORT_C TInt I2s::IsDMAEnabled(TInt aInterfaceId, TInt& aEnabled)
//
// Reads the enabled state of DMA.
//
	{
	__ASSERT_DEBUG(NKern::CurrentContext() == NKern::EThread, Kern::Fault("I2s Interface", ECalledFromIsr));
	// TO DO: (optional)
	//
	// If the implementation supports FIFO DMA mode this function reads the relevant registers to find out which directions 
	// (Transmit and/or Receive) DMA is  enabled for the frame phase specified (left or right), and returns a bitmask of enabled 
	// directions in aEnabled. A bit set to "1" indicates DMA is enabled for the corresponding direction.
	// If the implementation has a combined receive/transmit FIFO then aEnabled should have both Rx and Tx bits set when the DMA is enabled.
	//
	return KErrNotSupported;
	}

EXPORT_C TInt I2s::Start(TInt aInterfaceId, TInt aDirection)
//
// Starts data transmission and/or data reception unless interface is a Controller;
// if the device is also a Master, starts generation of data synchronisation signals.
//
	{
	__ASSERT_DEBUG(NKern::CurrentContext() == NKern::EThread, Kern::Fault("I2s Interface", ECalledFromIsr));
	// TO DO: (optional)
	//
	// Programs the appropriate registers to start operation in the direction specified by aDirection.
	// Should check if the interface has been configured coherently.
	//
	return KErrNotSupported;
	}

EXPORT_C TInt I2s::Stop(TInt aInterfaceId, TInt aDirection)
//
// Stops data transmission and/or data reception;
// if device is also a Master, stops generation of data synchronisation signals.
//
	{
	__ASSERT_DEBUG(NKern::CurrentContext() == NKern::EThread, Kern::Fault("I2s Interface", ECalledFromIsr));
	// TO DO: (optional)
	//
	// If the interface has been started, programs the appropriate registers to stop operation in the direction specified by aDirection.
	//
	return KErrNotSupported;
	}

EXPORT_C TInt I2s::IsStarted(TInt aInterfaceId, TI2sDirection aDirection, TBool& aStarted)
//
// Checks if a transmission or a reception is underway.
//
	{
	// TO DO: (optional)
	//
	// Reads the appropriate registers to check if the interface speficied by aInterfaceId is started in the direction
	// specified by aDirection. Returns teh result (as TRUE or FALSE) in aStarted.
	// If the interface is a Controller and a bus operation is underway, ETrue should be returned regardless of aDirection.
	//
	return KErrNotSupported;
	}

// dll entry point..
DECLARE_STANDARD_EXTENSION()
	{
	// TO DO: (optional)
	//
	// The Kernel extension entry point: if your interface requires any early intialisation do it here.
	//
	return KErrNone;
	}
