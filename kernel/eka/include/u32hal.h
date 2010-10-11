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
// e32\include\u32hal.h
// 
// WARNING: This file contains some APIs which are internal and are subject
//          to change without notice. Such APIs should therefore not be used
//          outside the Kernel and Hardware Services package.
//

#ifndef __U32HAL_H__
#define __U32HAL_H__

#include <e32cmn.h>

//
// Classes used for in functions
//

/**
Used for TVariantInfoV01::iLedCapabilities
@publishedPartner
@released
*/
const TUint KLedMaskRed1=0x00000001;

/**
Used for TVariantInfoV01::iLedCapabilities
@publishedPartner
@released
*/
const TUint KLedMaskGreen1=0x00000002;

/**
Used for TVariantInfoV01::iLedCapabilities
@publishedPartner
@released
*/
const TUint KLedMaskYellow1=0x00000004;

/**
Used for TSupplyInfoV1.iFlags
@publishedPartner
@deprecated Unused by Symbian code
*/
const TUint KSupplyFlagSoundWarning=0x00000001;

/**
Used for TSupplyInfoV1.iFlags
@publishedPartner
@deprecated Unused by Symbian code
*/
const TUint KSupplyFlagBacklightWarning=0x00000002;

/**
Used for TSupplyInfoV1.iFlags
@publishedPartner
@deprecated Unused by Symbian code
*/
const TUint KSupplyFlagTimeChanged=0x80000000;

/**
@internalComponent
@deprecated Unused by Symbian code
*/
class TClearSetMask
	{
public:
	TUint iClearMask;
	TUint iSetMask;
	};

/**
@internalTechnology
@deprecated Unused by Symbian code
*/
enum TSupplyStatus
	{
	EZero,EVeryLow,ELow,EGood
	};

/**
@internalTechnology
@deprecated Unused by Symbian code
*/
class TSupplyInfoV1
	{
public:
	SInt64 iMainBatteryInsertionTime;
	TSupplyStatus iMainBatteryStatus;
	SInt64 iMainBatteryInUseMicroSeconds;
	TInt iCurrentConsumptionMilliAmps;
	TInt iMainBatteryConsumedMilliAmpSeconds;
	TInt iMainBatteryMilliVolts;
	TInt iMainBatteryMaxMilliVolts;
	TSupplyStatus iBackupBatteryStatus;
	TInt iBackupBatteryMilliVolts;
	TInt iBackupBatteryMaxMilliVolts;
	TBool iExternalPowerPresent;
	SInt64 iExternalPowerInUseMicroSeconds;
	TUint iFlags;
	};

/**
@internalTechnology
@deprecated Unused by Symbian code
*/
typedef TPckgBuf<TSupplyInfoV1> TSupplyInfoV1Buf;

/**
@publishedPartner
@deprecated Unused by Symbian code
*/
class TMouseProperties
	{
public:
	TUint8 iSpeed;
	TUint8 iAcceleration;
	};

/**
@internalTechnology Used by Symbian base ports
@deprecated Unused by Symbian code
*/
class TSoundInfoV1
	{
public:
	TBool iKeyClickEnabled;
	TBool iKeyClickLoud;
	TBool iKeyClickOverridden;
	TBool iPointerClickEnabled;
	TBool iPointerClickLoud;
	TBool iBeepEnabled;
	TBool iBeepLoud;
	TBool iSoundDriverEnabled;
	TBool iSoundDriverLoud;
	TBool iSoundEnabled;
	};

/**
@internalTechnology
@deprecated Unused by Symbian code
*/
typedef TPckgBuf<TSoundInfoV1> TSoundInfoV1Buf;

/**
@internalTechnology Used by Symbian base ports
*/
class TOnOffInfoV1
	{
public:
	TBool iPointerSwitchesOn;
	TBool iCaseOpenSwitchesOn;
	TBool iCaseCloseSwitchesOff;
	};

/**
@internalTechnology
@deprecated Unused by Symbian code
*/
typedef TPckgBuf<TOnOffInfoV1> TOnOffInfoV1Buf;

/**
@internalTechnology
@removed Unused by Symbian code
*/
enum TUserMediaFunction {EUserMediaRemount,EUserMediaNotifyChange};




//
// Hal function enumerations
//

/**
@publishedPartner
@released

Defines the set of HAL groups.

Each HAL group has an associated HAL handler. Note that the
HAL handlers for EHalGroupKernel and EHalGroupEmulator are internal
to Symbian OS.

See the Base Porting Guide documentation in the developer library for
more information on HAL groups.
*/
enum THalFunctionGroup
	{
	/**
	@internalComponent
	
	The HAL group associated with kernel related information; for example
	the reason for the most recent system boot.
	
	The function-ids associated with this HAL group are defined by the set
    of TKernelHalFunction enum values.
    */
	EHalGroupKernel=0,


    /**
    The HAL group associated with Variant specific hardware.
    
    This HAL handler is implemented by the Variant, specifically by
    the base port's implementation of Asic::VariantHal().
    
    The function-ids associated with this HAL group are defined by the set
    of TVariantHalFunction enum values. See the individual function-ids for
    the associated capabilities.

    @see TVariantHalFunction
    @see Asic::VariantHal()
    */
	EHalGroupVariant=1,


    /**
    The HAL group associated with the media driver.
    
    It is used internally by the media driver LDD.
    
    The function-ids associated with this HAL group are defined by the set
    of TMediaHalFunction enum values. See the individual function-ids for
    the associated capabilities.
    
    @see TMediaHalFunction
    */
	EHalGroupMedia=2,


    /**
    The HAL group associated with power handling.
    
    This HAL handler is implemented by the power model.
    
    The function-ids associated with this HAL group are defined by the set
    of TPowerHalFunction enum values. See the individual function-ids for
    the associated capabilities.
		
	@see TPowerHalFunction
    */
	EHalGroupPower=3,
	
	
	/**
	The HAL group associated with the main screen display.
	
	This HAL handler is implemented by the screen (i.e. LCD or video driver).
	
    The function-ids associated with this HAL group are defined by the set
    of TDisplayHalFunction enum values. See the individual function-ids for
    the associated capabilities.
		
	@see TDisplayHalFunction
	*/
	EHalGroupDisplay=4,


    /**
    The HAL group associated with the digitiser (i.e. pen or stylus).
    
    This HAL handler is implemented by the digitiser.
    
    The function-ids associated with this HAL group are defined by the set
    of TDigitiserHalFunction enum values. See the individual function-ids for
    the associated capabilities.
    
    @see TDigitiserHalFunction
    */
	EHalGroupDigitiser=5,


    /**
    The HAL group associated with a sound driver.
    	
   	This group is provided by Symbian OS for backwards compatibility with
   	the Psion Series 5MX devices.
   	
   	The function-ids associated with this HAL group are defined by the set
    of TSoundHalFunction enum values. See the individual function-ids for
    the associated capabilities.
    
    @see TSoundHalFunction
    */
	EHalGroupSound=6,


    /**
	The HAL group associated with a mouse-type device.
	
	In Symbian OS, the the only example of a mouse device is in the emulator,
	and this is a port of Symbian OS maintained by Symbian.
	
    The function-ids associated with this HAL group are defined by the set
    of TMouseHalFunction enum values. See the individual function-ids for
    the associated capabilities.
		
	@see TMouseHalFunction
    */
	EHalGroupMouse=7,


	/**
	@internalComponent
	
	The HAL group associated with the emulator.
	
	The function-ids associated with this HAL group are defined by the set
    of TEmulatorHalFunction enum values.
	*/
	EHalGroupEmulator=8,


    /**
    The HAL group associated with the keyboard.
    
    This HAL handler is implemented by the keyboard driver.
    
    The function-ids associated with this HAL group are defined by the set
    of TKeyboardHalFunction enum values. See the individual function-ids for
    the associated capabilities.
    
    @see TKeyboardHalFunction
    */
	EHalGroupKeyboard=9,

	/*
    The HAL group associated with the virtual memory system.

	The function-ids associated with this HAL group are defined by the set
    of TVMHalFunction enum values.

    @see TVMHalFunction
	*/
	EHalGroupVM=10,

	/*
    The HAL group associated with the RAM Zone configuration.

	The function-ids associated with this HAL group are defined by the set
    of TRamHalFunction enum values.

    @see TRamHalFunction
	*/
	EHalGroupRam=11,

	/**
	Reserved for platform specific use.
	*/
	EHalGroupPlatformSpecific1=29,

	/**
	Reserved for platform specific use.
	*/
	EHalGroupPlatformSpecific2=30
	};


/**
@internalComponent
*/
struct SCpuStates
	{
	TUint32	iTA;
	TUint32 iIA;
	TUint32	iCU;
	TUint32	iGD;
	TInt	iDC;
	TUint32	iSC;
	TUint32	iRC;
	TUint32	iCCS;
	TUint8	iPO;
	TUint8	iSpare1;
	TUint8	iSpare2;
	TUint8	iSpare3;
	TUint32	iPODC;
	TInt	iDS[8];
	TUint32	iUDC[8];
	TUint32	iUAC[8];
	TUint32	iOP[8];
	TUint32	iF[8];
	};


/**
@internalComponent
*/
enum TKernelHalFunction
	{
	EKernelHalMemoryInfo,
	EKernelHalRomInfo,
	EKernelHalStartupReason,
	EKernelHalFaultReason,
	EKernelHalExceptionId,
	EKernelHalExceptionInfo,
	EKernelHalCpuInfo,
	EKernelHalPageSizeInBytes,
	EKernelHalTickPeriod,
	EKernelHalMemModelInfo,
	EKernelHalFastCounterFrequency,
	EKernelHalNTickPeriod,
	EKernelHalHardwareFloatingPoint,
	EKernelHalGetNonsecureClockOffset,
	EKernelHalSetNonsecureClockOffset,
	EKernelHalSmpSupported,
	EKernelHalNumLogicalCpus,
	EKernelHalSupervisorBarrier,
	EKernelHalFloatingPointSystemId,
	EKernelHalLockThreadToCpu,
	EKernelHalConfigFlags,
	EKernelHalCpuStates,
	EKernelHalSetNumberOfCpus,
	};




/**
@publishedPartner
@released

The set of function-ids that are  associated with the EHalGroupVariant
HAL group.

Each enum value represents a specific characteristic of the Variant,
and is passed as the second parameter to
the HAL handler function dealing with this group.
 
@see EHalGroupVariant
*/
enum TVariantHalFunction
	{
	/**
	Gets Variant specifc information.
	
	Requirements of the HAL function handler's 2nd, 3rd and 4th parameters:
	
	- TInt aFunction : This enum value.
	    
	- TAny* a1 : This is a pointer to a package buffer (TPckgBuf) containing a
	TVariantInfoV01 structure. The HAL function needs to fill the members of
	this structure with the appropriate information.
									
	- TAny* a2 : NULL.

	An example of this function can be found in the Variant template;
	see the function: 
	@code
	Template::VariantHal()
	@endcode
	in  
	@code
	...\template\template_variant\specific\variant.cpp
	@endcode
	
	@see TVariantInfoV01
    @see TPckgBuf
	*/
	EVariantHalVariantInfo,


	/**
	Sets the debug port number.
	
	Requirements of the HAL function handler's 2nd, 3rd and 4th parameters:
	
	- TInt aFunction : This enum value.

	- TAny* a1 : A TInt value containing the debug port number.
									
	- TAny* a2 : NULL.

	An example of this function can be found in the Variant template;
	see the function: 
	@code
	Template::VariantHal()
	@endcode
	in  
	@code
	...\template\template_variant\specific\variant.cpp
	@endcode
	*/
	EVariantHalDebugPortSet,


	/**
	Gets the debug port number.
	
	Requirements of the HAL function handler's 2nd, 3rd and 4th parameters:
	
	- TInt aFunction : This enum value.

    - TAny* a1 : A TInt value into which the HAL handler puts the debug port value
    									
	- TAny* a2 : NULL.

	An example of this function can be found in the Variant template;
	see the function: 
	@code
	Template::VariantHal()
	@endcode
	in  
	@code
	...\template\template_variant\specific\variant.cpp
	@endcode
	*/
	EVariantHalDebugPortGet,


	/**
	Sets the current state of each LED. 
	
	Requirements of the HAL function handler's 2nd, 3rd and 4th parameters:
	
	- TInt aFunction : This enum value.

    - TAny* a1 : A TUint value containing the bitmask that
    describes the state of the LEDs. For each bit in the mask, 0 = OFF, 1 = ON.
    									
	- TAny* a2 : NULL.
	
	An example of this function can be found in the Variant template;
	see the function: 
	@code
	Template::VariantHal()
	@endcode
	in  
	@code
	...\template\template_variant\specific\variant.cpp
	@endcode
	*/
	EVariantHalLedMaskSet,


	/**
	Gets the current state of each LED.
	
	Requirements of the HAL function handler's 2nd, 3rd and 4th parameters:
	
	- TInt aFunction : This enum value.

    - TAny* a1 : A TUint value into which the HAL handler puts the bitmask that
    describes the state of the LEDs. For each bit in the mask, 0 = OFF, 1 = ON.
    									
	- TAny* a2 : NULL.

	An example of this function can be found in the Variant template;
	see the function: 
	@code
	Template::VariantHal()
	@endcode
	in  
	@code
	...\template\template_variant\specific\variant.cpp
	@endcode
	*/
	EVariantHalLedMaskGet,


	/**
	Gets the current state of any Variant specific switches. 
	
	Requirements of the HAL function handler's 2nd, 3rd and 4th parameters:
	
	- TInt aFunction : This enum value.

    - TAny* a1 : A TUint value into which the HAL handler puts the bits that
    describe the state of the switches. The meaning of the switches is entirely
    hardware dependent.
    									
	- TAny* a2 : NULL.

	An example of this function can be found in the Variant template;
	see the function: 
	@code
	Template::VariantHal()
	@endcode
	in  
	@code
	...\template\template_variant\specific\variant.cpp
	@endcode
	*/
	EVariantHalSwitches,


	/**
	Restarts the system.
	
	Requirements of the HAL function handler's 2nd, 3rd and 4th parameters:
	
	- TInt aFunction : This enum value.

    - TAny* a1 : A TUint value containing the bits defining the custom restart reasons
    that can be passed to Kern::Restart().
    									
	- TAny* a2 : NULL.

	An example of this function can be found in the Variant template;
	see the function: 
	@code
	Template::VariantHal()
	@endcode
	in  
	@code
	...\template\template_variant\specific\variant.cpp
	@endcode
		
	@capability PowerMgmt
	
	@see Kern::Restart()
	*/
	EVariantHalCustomRestart,


	/**
	Gets the reason for the system restart.
			
	Requirements of the HAL function handler's 2nd, 3rd and 4th parameters:
	
	- TInt aFunction : This enum value.

    - TAny* a1 : A TInt value into which the handler will put a value
    representing the restart reason.
        									
	- TAny* a2 : NULL.
	
	An example of this function can be found in the Variant template;
	see the function: 
	@code
	Template::VariantHal()
	@endcode
	in the file
	@code
	...\template\template_variant\specific\variant.cpp
	@endcode
	*/
	EVariantHalCustomRestartReason,


	/**
	Gets the current state of the case.
	
	Requirements of the HAL function handler's 2nd, 3rd and 4th parameters:
	
	- TInt aFunction : This enum value.

    - TAny* a1 : A TUint value into which the HAL handler puts a value representing the case state.
      Possible values are 0 for closed, and 1 for open.
    									
	- TAny* a2 : NULL.

	An example of this function can be found in the Variant template;
	see the function: 
	@code
	Template::VariantHal()
	@endcode
	in  
	@code
	...\template\template_variant\specific\variant.cpp
	@endcode
	*/
	EVariantHalCaseState,


	/**
	Gets the number of screens on this platform.
	
	Requirements of the HAL function handler's 2nd, 3rd and 4th parameters:
	
	- TInt aFunction : This enum value.
	    
	- TAny* a1 : This is a pointer to TInt that will contain the number of screens
									
	- TAny* a2 : NULL.

	*/
	EVariantHalCurrentNumberOfScreens,

	/**
	Sets the startup reason for the system restart.
	
	Requirements of the HAL function handler's 2nd, 3rd and 4th parameters:
	
	- TInt aFunction : This enum value.

    - TAny* a1 : A TUint value containing the bits defining the startup mode
    that will be passed to Kern::Restart().
    									
	- TAny* a2 : NULL.
	
	An example of this function can be found in the Variant template;
	see the function: 
	@code
	Template::VariantHal()
	@endcode
	in  
	@code
	...\template\template_variant\specific\variant.cpp
	@endcode
		
	@capability WriteDeviceData
	
	@see Kern::Restart()
	*/	
	EVariantHalPersistStartupMode,

	/**
	Gets the startup mode after a system restart.
			
	Requirements of the HAL function handler's 2nd, 3rd and 4th parameters:
	
	- TInt aFunction : This enum value.

    - TAny* a1 : A TInt that will hold the returned value
        									
	- TAny* a2 : NULL.
	
	An example of this function can be found in the Variant template;
	see the function: 
	@code
	Template::VariantHal()
	@endcode
	in the file
	@code
	...\template\template_variant\specific\variant.cpp
	@endcode
	*/
	EVariantHalGetPersistedStartupMode,

	/**
	Returns the maximum number of values that can be used to store the startup reason required for a custom restart.
	
	Requirements of the HAL function handler's 2nd, 3rd and 4th parameters:
	
	- TInt aFunction : This enum value.

    - TAny* a1 : A TInt that will hold the returned value
    									
	- TAny* a2 : NULL.

	An example of this function can be found in the Variant template;
	see the function: 
	@code
	Template::VariantHal()
	@endcode
	in  
	@code
	...\template\template_variant\specific\variant.cpp
	@endcode
	*/
	EVariantHalGetMaximumCustomRestartReasons,

	/**
	Returns the maximum number of values that can be used to store the startup mode required for a transition to
	a EPwRestart power state.
	
	Requirements of the HAL function handler's 2nd, 3rd and 4th parameters:
	
	- TInt aFunction : This enum value.

    - TAny* a1 : A TInt that will hold the returned value
    									
	- TAny* a2 : NULL.

	An example of this function can be found in the Variant template;
	see the function: 
	@code
	Template::VariantHal()
	@endcode
	in  
	@code
	...\template\template_variant\specific\variant.cpp
	@endcode
	*/
	EVariantHalGetMaximumRestartStartupModes,

	/**
	Returns a factor by which timeouts in test code should be expanded for
	exceptionally slow platforms (e.g. with cores implemented in FPGA).

	If this is not supported the factor should be assumed to be 1.
	*/
	EVariantHalTimeoutExpansion,

	/**
	Returns a serial number as an Int
	*/
	EVariantHalSerialNumber,
	
	/**
	Returns the interrupt used by sampling profiler - applicable for SMP only.
	Each CPU_i is interrupted by interrupt number ECpuProfilingInterrupt + i
	*/	
	EVariantHalProfilingDefaultInterruptBase

	};




/**
@publishedPartner
@released

The set of function-ids that are  associated with the EHalGroupMedia
HAL group.

Each enum value represents a specific characteristic of the media driver,
and is passed as the second parameter to
the HAL handler function dealing with this group.
 
@see EHalGroupMedia
*/
enum TMediaHalFunction
	{
	/**
	Gets drive information.

	Requirements of the HAL function handler's 2nd, 3rd and 4th parameters:
	
	- TInt aFunction : This enum value.
	    
	- TAny* a1 : This is a pointer to a package buffer (TPckgBuf) containing a
    TDriveInfoV1 structure. The HAL function needs to fill the members of
	this structure with the appropriate information.
									
	- TAny* a2 : NULL.

	Note that the HAL handler is implemented by Symbian OS
	
	@see TDriveInfoV1
    @see TPckgBuf
	*/
	EMediaHalDriveInfo,

	/**
	@internalTechnology
	@test
	*/
	EMediaHalGetROMConcurrencyInfo,

	/**
	@internalTechnology
	@test
	*/
	EMediaHalGetCodeConcurrencyInfo,

	/**
	@internalTechnology
	@test
	*/
	EMediaHalResetConcurrencyInfo,

	/**
	@internalTechnology
	@test
	*/
	EMediaHalGetROMPagingBenchmark,

	/**
	@internalTechnology
	@test
	*/
	EMediaHalGetCodePagingBenchmark,

	/**
	@internalTechnology
	@test
	*/
	EMediaHalResetPagingBenchmark,

	/**
	@internalTechnology
	@test
	*/
	EMediaHalGetDataConcurrencyInfo,

	/**
	@internalTechnology
	@test
	*/
	EMediaHalGetDataInPagingBenchmark,

	/**
	@internalTechnology
	@test
	*/
	EMediaHalGetDataOutPagingBenchmark,

	/**
	@internalTechnology
	@test
	*/
	EMediaHalGetPagingInfo,
	};




/**
@publishedPartner
@released

The set of function-ids that are  associated with the EHalGroupPower
HAL group.

Each enum value represents a specific aspect of power handling on the device.

@see EHalGroupPower
*/
enum TPowerHalFunction
	{
	
	/**
	This is used internally by Symbian
	*/
	EPowerHalOnOffInfo,
	
	
	/**
	This is not currently used.
	*/
	EPowerHalSupplyInfo,


	/**
	This is not currently used.
	
	@capability PowerMgmt
	*/
	EPowerHalSetAutoSwitchOffBehavior,
    
    
    /**
    This is not currently used.
    */
	EPowerHalAutoSwitchOffBehavior,


	/**
	This is not currently used.
	
	@capability PowerMgmt
	*/
	EPowerHalSetAutoSwitchOffTime,


    /**
    This is not currently used.
    */
	EPowerHalAutoSwitchOffTime,


	/**
	This is not currently used.
	
	@capability PowerMgmt
	*/
	EPowerHalResetAutoSwitchOffTimer,


	/** 
	Switches the device off.
	
	Requirements of the HAL function handler's 2nd, 3rd and 4th parameters:
	
	- TInt aFunction : This enum value.
	    
    - TAny* a1 : NULL.
									
	- TAny* a2 : NULL.
	
	@capability PowerMgmt
	*/
	EPowerHalSwitchOff,


	/**
	Sets whether touching the screen with a pen or stylus will cause
    the device to switch on.
    	
	Requirements of the HAL function handler's 2nd, 3rd and 4th parameters:
	
	- TInt aFunction : This enum value.
	    
    - TAny* a1 : A TBool that needs to be set to ETrue (if touching the screen
    is to cause a power on), or EFalse (if touching the screen is not to cause
    a power on).
									
	- TAny* a2 : NULL.

	@capability WriteDeviceData
	*/
	EPowerHalSetPointerSwitchesOn,


    /**
    Tests whether touching the screen with a pen or stylus will cause
    the device to switch on.
    
    Requirements of the HAL function handler's 2nd, 3rd and 4th parameters:
	
	- TInt aFunction : This enum value.
	    
    - TAny* a1 : A TBool into which the HAL handler puts ETrue (if touching
    the screen will cause a power on), or EFalse (if touching the screen will
    not cause a power on).
									
	- TAny* a2 : NULL.
    */
	EPowerHalPointerSwitchesOn,


	/**
	Sets whether the opening of a 'device case' will cause
    the device to switch on.
    
	Note that the meaning attached to a 'device case' depends on the hardware.
	
	Requirements of the HAL function handler's 2nd, 3rd and 4th parameters:
	
	- TInt aFunction : This enum value.
	    
    - TAny* a1 : A TBool that needs to be set to ETrue (if opening
    the case is to cause a power on), or EFalse (if opening
    the case is not to cause a power on).
									
	- TAny* a2 : NULL.

	@capability WriteDeviceData
	*/
	EPowerHalSetCaseOpenSwitchesOn,


    /**
    Tests whether the opening of a 'device case' will cause
    the device to switch on.
    
    Note that the meaning attached to a 'device case' depends on the hardware.
    
    Requirements of the HAL function handler's 2nd, 3rd and 4th parameters:
	
	- TInt aFunction : This enum value.
	    
    - TAny* a1 : A TBool into which the HAL handler puts ETrue (if opening
    the case will cause a power on), or EFalse (if opening
    the case will not cause a power on).
									
	- TAny* a2 : NULL.
    */
	EPowerHalCaseOpenSwitchesOn,


	/**
	Sets whether the closing of a 'device case' will cause
    the device to switch off.
    
	Note that the meaning attached to a 'device case' depends on the hardware.
	
	Requirements of the HAL function handler's 2nd, 3rd and 4th parameters:
	
	- TInt aFunction : This enum value.
	    
    - TAny* a1 : A TBool that needs to be set to ETrue (if closing
    the case is to cause a power off), or EFalse (if closing
    the case is not to cause a power off).
									
	- TAny* a2 : NULL.
	
	@capability WriteDeviceData
	*/
	EPowerHalSetCaseCloseSwitchesOff,
    
    
    /**
    Tests whether the closing of a 'device case' will cause
    the device to switch off.
    
    Note that the meaning attached to a 'device case' depends on the hardware.
    
    Requirements of the HAL function handler's 2nd, 3rd and 4th parameters:
	
	- TInt aFunction : This enum value.
	    
    - TAny* a1 : A TBool into which the HAL handler puts ETrue (if closing
    the case will cause a power off), or EFalse (if closing
    the case will not cause a power off).
									
	- TAny* a2 : NULL.
	*/
	EPowerHalCaseCloseSwitchesOff,


	/**
	This is not currently used.
	
	@capability PowerMgmt
	*/
	EPowerHalSetBatteryType,
    
    
    /**
    This is not currently used.
    */
	EPowerHalBatteryType,


	/**
	This is not currently used.
	
	@capability PowerMgmt
	*/
	EPowerHalSetBatteryCapacity,


    /**
    This is not currently used.
    */
	EPowerHalBatteryCapacity,


    /**
    This is not currently used.
    */
	EPowerHalAutoSwitchOffType,
	
	
	/**
    This is used internally by Symbian.
	*/
	EPowerHalTestBootSequence,

	
	/**
    Tests whether a backup power supply is present or not.
    
    Requirements of the HAL function handler's 2nd, 3rd and 4th parameters:
	
	- TInt aFunction : This enum value.
	    
    - TAny* a1 : A TBool into which the HAL handler puts ETrue (if backup 
    power is present), or EFalse (if backup power is not present).
									
	- TAny* a2 : NULL.
	*/
	EPowerHalBackupPresent,

	
	/**
    Tests whether accessory power is present or not.
    	
	Requirements of the HAL function handler's 2nd, 3rd and 4th parameters:
	
	- TInt aFunction : This enum value.
	    
    - TAny* a1 : A TBool into which the HAL handler puts ETrue (if accessory 
    power is present), or EFalse (if accessory power is not present).
									
	- TAny* a2 : NULL.
	*/
	EPowerHalAcessoryPowerPresent,


	/**
	Used for testing purposes.

	Sets a member in the power manager depending on which testing of some API's are skipped	
	*/
	EPowerHalPowerManagerTestMode,
	};




/**
@publishedPartner
@released

The set of function-ids that are  associated with the EHalGroupDisplay
HAL group.

Each enum value represents a specific characteristic of the hardware display,
i.e. the screen or the LCD, and is passed as the second parameter to
the HAL handler function dealing with this group.
 
@see EHalGroupDisplay
*/
enum TDisplayHalFunction
	{
	
	/**
	Gets the screen display properties.
	
	Requirements of the HAL function handler's 2nd, 3rd and 4th parameters:

    - TInt aFunction : This enum value.
	
	- TAny* a1 : This is a pointer to a package buffer (TPckgBuf) containing a
	TScreenInfoV01 structure. The HAL function needs to fill the members of
	this structure with the appropriate information.
	
	- TAny* a2 : NULL.
	
	For example:
	@code
	...
	case EDisplayHalScreenInfo:
	    TPckgBuf<TScreenInfoV01> vPckg;
	    // Fill details
	    Kern::InfoCopy(*(TDes8*)a1,vPckg);
	...
	@endcode
	
	@see TScreenInfoV01
    @see TPckgBuf
	*/
	EDisplayHalScreenInfo,
	
	
	/**
	Registers whether it is the Window Server's responsibility to turn
	the screen display on or off.
	
	Requirements of the HAL function handler's 2nd, 3rd and 4th parameters:
	
	- TInt aFunction : This enum value.
	
	- TAny* a1 : This is a TBool value that needs to be set to ETrue or EFalse.
	
	- TAny* a2 : NULL.
	
	For example:
	@code
	...
	case EDisplayHalWsRegisterSwitchOnScreenHandling:
	    iWsSwitchOnScreen=(TBool)a1;
	...
	@endcode
	
	@see TBool
	*/
	EDisplayHalWsRegisterSwitchOnScreenHandling,
	
	
	/**
	Requests that the Window Server turn the screen display on.
	
	Requirements of the HAL function handler's 2nd, 3rd and 4th parameters:
	
	- TInt aFunction : This enum value.
	
	- TAny* a1 : NULL
		
	- TAny* a2 : NULL
	*/
	EDisplayHalWsSwitchOnScreen,
	
	
	/**
	Gets the maximum screen display contrast value.
	
	Requirements of the HAL function handler's 2nd, 3rd and 4th parameters:
	
	- TInt aFunction : This enum value.
	
	- TAny* a1 : A TInt value into which the HAL handler puts the maximum
	display contrast value.
		
	- TAny* a2 : NULL
	*/
	EDisplayHalMaxDisplayContrast,


	/**
	Sets the screen display contrast value.
	
	Requirements of the HAL function handler's 2nd, 3rd and 4th parameters:
	
	- TInt aFunction : This enum value.
	
	- TAny* a1 : A TInt value containing the display contrast value.
	Typically, this needs to lie within a range that is valid for the device.
			
	- TAny* a2 : NULL
	
	@capability WriteDeviceData 
	*/
	EDisplayHalSetDisplayContrast,
    
    
    /**
    Gets the screen display's current contrast value.
    
    Requirements of the HAL function handler's 2nd, 3rd and 4th parameters:
	
	- TInt aFunction : This enum value.
	
	- TAny* a1 : A TInt value into which the HAL handler puts the
	display contrast value.
			
	- TAny* a2 : NULL
	*/
	EDisplayHalDisplayContrast,


	/**
	Sets the backlight behaviour.
	
	Requirements of the HAL function handler's 2nd, 3rd and 4th parameters:
	
	- TInt aFunction : This enum value.
	
	- TAny* a1 : Not specified; depends on the manufacturer's implementation.
				
	- TAny* a2 : Not specified; depends on the manufacturer's implementation.
	
	@capability WriteDeviceData
	*/
	EDisplayHalSetBacklightBehavior,


	/**
	Gets information about the backlight behaviour.
	
	Requirements of the HAL function handler's 2nd, 3rd and 4th parameters:
	
	- TInt aFunction : This enum value.
	
	- TAny* a1 : Not specified; depends on the manufacturer's implementation.
				
	- TAny* a2 : Not specified; depends on the manufacturer's implementation.
	*/
	EDisplayHalBacklightBehavior,


	/**
	Sets the backlight timeout value, i.e. the length of time that the backlight
	will stay on.
	
	Requirements of the HAL function handler's 2nd, 3rd and 4th parameters:
	
	- TInt aFunction : This enum value.
	
	- TAny* a1 : Not specified; depends on the manufacturer's implementation.
				
	- TAny* a2 : Not specified; depends on the manufacturer's implementation.
	
	@capability WriteDeviceData
	*/
	EDisplayHalSetBacklightOnTime,


    /**
    Gets information about the backlight timeout value, i.e. the length
    of time that the backlight will stay on.
    
    Requirements of the HAL function handler's 2nd, 3rd and 4th parameters:
	
	- TInt aFunction : This enum value.
	
	- TAny* a1 : Not specified; depends on the manufacturer's implementation.
				
	- TAny* a2 : Not specified; depends on the manufacturer's implementation.
    */
	EDisplayHalBacklightOnTime,


	/**
	Sets the backlight on or off.
	
	Requirements of the HAL function handler's 2nd, 3rd and 4th parameters:
	
	- TInt aFunction : This enum value.
	
    - TAny* a1 : A TBool that needs to be set to ETrue or EFalse.
			
	- TAny* a2 : NULL
	
	@capability WriteDeviceData
	*/
	EDisplayHalSetBacklightOn,


	/**
	Gets the current state of the backlight.
	
	Requirements of the HAL function handler's 2nd, 3rd and 4th parameters:
	
	- TInt aFunction : This enum value.
	
    - TAny* a1 : A TBool into which the HAL handler
    puts ETrue (for On) or EFalse (for Off).
				
	- TAny* a2 : NULL
	*/
	EDisplayHalBacklightOn,
	
	
	/**
	Gets the screen display's maximum brightness value.
	
	Requirements of the HAL function handler's 2nd, 3rd and 4th parameters:
	
	- TInt aFunction : This enum value.
	
	- TAny* a1 : A TInt value into which the HAL handler puts the
	maximum brightness value.
			
	- TAny* a2 : NULL
	*/
	EDisplayHalMaxDisplayBrightness,


	/**
	Sets the screen display's brightness value.
	
	Requirements of the HAL function handler's 2nd, 3rd and 4th parameters:
	
	- TInt aFunction : This enum value.
	
	- TAny* a1 : A TInt value containing the brightness value.
	Typically, this needs to lie within a range that is valid for the device.
			
	- TAny* a2 : NULL
	
	@capability WriteDeviceData
	*/
	EDisplayHalSetDisplayBrightness,


    /**
    Gets the screen display's current brightness value.
	
	Requirements of the HAL function handler's 2nd, 3rd and 4th parameters:
	
	- TInt aFunction : This enum value.
	
	- TAny* a1 : A TInt value into which the HAL handler puts the
	brightness value.
			
	- TAny* a2 : NULL
    */
	EDisplayHalDisplayBrightness,
//

    /**
    Gets the number of available display modes.
    
    Requirements of the HAL function handler's 2nd, 3rd and 4th parameters:
	
	- TInt aFunction : This enum value.
	
	- TAny* a1 : A TInt value into which the HAL handler puts the
	number of display modes value.
			
	- TAny* a2 : NULL
    */
	EDisplayHalModeCount,


	/**
	Sets the display mode.
	
	This will normally update the information maintained by the screen driver
	in the form of a TScreenInfoV01 struct.
		
	Requirements of the HAL function handler's 2nd, 3rd and 4th parameters:
	
	- TInt aFunction : This enum value.
	
	- TAny* a1 : A TInt value containing a number representing the display mode.
	Typically, this needs to lie within a range that is valid for the device.
	For example, the value usually needs to be positive, and be less than
	the number of display modes.
			
	- TAny* a2 : NULL
	
	@capability MultimediaDD
	
	@see TScreenInfoV01
	*/
	EDisplayHalSetMode,


    /**
    Gets the current screen display mode.
    
    Requirements of the HAL function handler's 2nd, 3rd and 4th parameters:
    
    - TInt aFunction : This enum value.
	
	- TAny* a1 : A TInt value into which the HAL handler puts the 
	the current display mode value.
			
	- TAny* a2 : NULL
    */
	EDisplayHalMode,


	/**
	Sets a colour value for the specified palette entry. 
	
	Requirements of the HAL function handler's 2nd, 3rd and 4th parameters:
    
    - TInt aFunction : This enum value.
	
	- TAny* a1 : A TInt value that identifies the palette entry. The value
	occupies the junior 8 bits of the integer. The senior 24 bits are all 0.
					
	- TAny* a2 : A TInt value containing the colour value; it represents
	a standard Symbian OS RGB colour value, and occupies the junior 24 bits
	of the integer. The senior 8 bits are not defined.
	
	@capability MultimediaDD
	*/
	EDisplayHalSetPaletteEntry,
    
    
    /**
    Gets the colour value for the specified palette entry.
    
    Requirements of the HAL function handler's 2nd, 3rd and 4th parameters:
    
    - TInt aFunction : This enum value.
	
	- TAny* a1 : A TInt value that identifies the palette entry. The value
	is expected to occupy the junior 8 bits of the integer. The senior 24 bits
	are not defined.
					
	- TAny* a2 : On return from the HAL function, a TInt value containing
    the colour value. The value occupies the junior 24 bits of the integer.
    The senior 8 bits are not defined.
    */
	EDisplayHalPaletteEntry,


	/**
	Sets the screen display on or off.

    Requirements of the HAL function handler's 2nd, 3rd and 4th parameters:
    
    - TInt aFunction : This enum value.
	
	- TAny* a1 : A TBool that needs to be set to ETrue (for On),
	or EFalse (for Off).
					
	- TAny* a2 : NULL.
	
	@capability PowerMgmt
	*/
	EDisplayHalSetState,


    /**
    Gets the state of the screen display, i.e. whether the display is on or off.
    
    Requirements of the HAL function handler's 2nd, 3rd and 4th parameters:
    
    - TInt aFunction : This enum value.
	
    - TAny* a1 : A TBool into which the HAL handler puts ETrue (for On),
    or EFalse (for Off).
									
	- TAny* a2 : NULL.
    */
	EDisplayHalState,
	
	
	/**
	Gets the maximum number of colours that the screen display supports.
	
	Requirements of the HAL function handler's 2nd, 3rd and 4th parameters:
	
	- TInt aFunction : This enum value.
	
	- TAny* a1 : A TInt value into which the HAL handler puts the maximum
	number of colours.
		
	- TAny* a2 : NULL
	*/
	EDisplayHalColors,
	
	
	/**
	Gets information about the current display mode.
	
	Requirements of the HAL function handler's 2nd, 3rd and 4th parameters:
	
	- TInt aFunction : This enum value.
	
	- TAny* a1 : This is a pointer to a package buffer (TPckgBuf) containing a
	TVideoInfoV01 structure. The HAL function needs to fill the members of
	this structure with the appropriate information.
	
    - TAny* a2 : NULL.
		
	@see TVideoInfoV01
    @see TPckgBuf
	*/
	EDisplayHalCurrentModeInfo,
	
	
	/**
	Gets information about the specified display mode.
	
	Requirements of the HAL function handler's 2nd, 3rd and 4th parameters:
	
	- TInt aFunction : This enum value.
	
	- TAny* a1 : A TInt value containing the number of the display mode for which
	information is to be fetched.
	
	- TAny* a2 : This is a pointer to a package buffer (TPckgBuf) containing a
	TVideoInfoV01 structure. The HAL function needs to fill the members of
	this structure with the appropriate information.
			
	@see TVideoInfoV01
    @see TPckgBuf
	*/
	EDisplayHalSpecifiedModeInfo,

//	EDisplaySwitchOffScreen,


    /**
    Fills/copies an area of the screen display with a rectangle of a specified colour.
    
    Requirements of the HAL function handler's 2nd, 3rd and 4th parameters:
	
	- TInt aFunction : This enum value.
			
	- TAny* a1 : This is a pointer to a package buffer (TPckgBuf) containing a
	SRectOpInfo structure. The structure contains the information that
	is needed for the operation.
    
    - TAny* a1 : NULL.
    
	@see SRectOpInfo
    @see TPckgBuf
    */
	EDisplayHalBlockFill,
	
	
    /**
    Fills/copies an area of the screen display with a rectangle of a specified colour.
    
    Requirements of the HAL function handler's 2nd, 3rd and 4th parameters:
	
	- TInt aFunction : This enum value.
			
	- TAny* a1 : This is a pointer to a package buffer (TPckgBuf) containing a
	SRectOpInfo structure. The structure contains the information that
	is needed for the operation.
    
    - TAny* a1 : NULL.
    
	@see SRectOpInfo
    @see TPckgBuf
    */
	EDisplayHalBlockCopy,
	
	
	/**
	Tests whether the screen display is secure.
	
	Requirements of the HAL function handler's 2nd, 3rd and 4th parameters:
	
	- TInt aFunction : This enum value.
	    
    - TAny* a1 : A TBool into which the HAL handler puts ETrue (for secure mode),
    or EFalse (for non-secure mode).
									
	- TAny* a2 : NULL.
	*/
	EDisplayHalSecure,


	/**
    Sets the screen display into secure or non-secure mode.
    
    Requirements of the HAL function handler's 2nd, 3rd and 4th parameters:
	
	- TInt aFunction : This enum value.
	
	- TAny* a1 : A TBool that needs to be set to ETrue (for secure mode),
	or EFalse (for non-secure mode).
									
	- TAny* a2 : NULL.
	
	@capability MultimediaDD
	*/
	EDisplayHalSetSecure,

	
	/**
	Gets the address of the DSA buffer
	
	Requirements of the HAL function handler's 2nd, 3rd and 4th parameters:
	
	- TInt aFunction : This enum value.
	
	- TAny* a1 : A TInt value into which the HAL handler puts the address
		
	- TAny* a2 : NULL
	*/
	EDisplayHalGetDisplayMemoryAddress,
	/**
	Gets a handle to the DSA buffer memory
	
	Requirements of the HAL function handler's 2nd, 3rd and 4th parameters:
	
	- TInt aFunction : This enum value.
	
	- TAny* a1 : A TInt value into which the HAL handler puts the handle value
		
	- TAny* a2 : NULL
	*/
	EDisplayHalGetDisplayMemoryHandle,
	
	
	/**
    Ask how many resolutions there are on a specific screen
    
    Requirements of the HAL function handler's 2nd, 3rd and 4th parameters:
	
	- TInt aFunction : This enum value.
	
	- TAny* a1 : A TInt reference to pass back the number of resolutions.
									
	- TAny* a2 : NULL.
	
	@capability MultimediaDD
	*/
	EDisplayHalNumberOfResolutions,
	
	
	/**
    Gets information on a specific configuration for a particular screen
    
    Requirements of the HAL function handler's 2nd, 3rd and 4th parameters:
	
	- TInt aFunction : This enum value.
	
	- TAny* a1 : A TInt configuration index to get back.
									
	- TAny* a2 : A TVideoInfoV01 descriptor to get the info back.
	
	@capability MultimediaDD
	*/
	EDisplayHalSpecificScreenInfo,
	
	
	/**
    Gets info on the current configuration for a particular screen
    
    Requirements of the HAL function handler's 2nd, 3rd and 4th parameters:
	
	- TInt aFunction : This enum value.
	
	- TAny* a1 : A TVideoInfoV01 descriptor to get the info back.
									
	- TAny* a2 : NULL.
	
	@capability MultimediaDD
	*/
	EDisplayHalCurrentScreenInfo,

	
	/**
    Sets the screen display into a predefined state.
    
    Requirements of the HAL function handler's 2nd, 3rd and 4th parameters:
	
	- TInt aFunction : This enum value.
	
	- TAny* a1 : A TInt that is one of the value of TDisplayState enum
											
	- TAny* a2 : NULL.
	
	@capability MultimediaDD
	*/
	EDisplayHalSetDisplayState,
	
	/**
    Gets the spinner of screen display state.
    
    Requirements of the HAL function handler's 2nd, 3rd and 4th parameters:
	
	- TInt aFunction : This enum value.
	
	- TAny* a1 : A TInt which is set to the spinner value on return
											
	- TAny* a2 : NULL.
	
	@capability MultimediaDD
	*/
	EDisplayHalGetStateSpinner,
	};

/**
@internalComponent
@test
The set of display state ids that are associated with EDisplayHalSetDisplayState

@see EDisplayHalSetDisplayState
*/

enum TDisplayConnectState
	{
	ENormalResolution,
	ENoResolution,
	EDisconnect,
	ESingleResolution,
	EDisplayStateTooHigh
	};

/**
@publishedPartner
@released

The set of function-ids that are  associated with the EHalGroupDigitiser
HAL group.

Each enum value represents a specific characteristic of the digitiser,
and is passed as the second parameter to the HAL handler function
dealing with this group.

@see EHalGroupDigitiser
*/
enum TDigitiserHalFunction
	{
	
	/**
	Sets the calibration data (i.e. the values of the digitiser to
	screen constants) for the digitiser device.
	
	Requirements of the HAL function handler's 2nd, 3rd and 4th parameters:
	
	- TInt aFunction : This enum value.
	
    - TAny* a1 : A pointer to a TDigitizerCalibration object.
				
	- TAny* a2 : NULL	
	
	@see TDigitizerCalibration
	
	@capability WriteDeviceData
	*/
	EDigitiserHalSetXYInputCalibration,


    /**
    Gets the calibration data (i.e. the values of the digitiser to
	screen constants) for the digitiser device.
	
	Requirements of the HAL function handler's 2nd, 3rd and 4th parameters:
	
	- TInt aFunction : This enum value.
	
    - TAny* a1 : A pointer to a TDigitizerCalibration object; the HAL function
    needs to fill the members of this structure with
    the appropriate information.
				
	- TAny* a2 : NULL	
	
	@see TDigitizerCalibration
    */
	EDigitiserHalCalibrationPoints,


    /**
    Saves the calibration data (i.e. the values of the digitiser to
	screen constants) for the digitiser device in a persistent memory area so
	that they can be restored after a power cycle.
	
	Requirements of the HAL function handler's 2nd, 3rd and 4th parameters:
	
	- TInt aFunction : This enum value.
	
    - TAny* a1 : NULL.
				
	- TAny* a2 : NULL	
    */
	EDigitiserHalSaveXYInputCalibration,


	/**
	Restores the calibration data (i.e. the values of the digitiser to
	screen constants) for the digitiser device from a persistent memory area so
	that they can be restored after a power cycle.

    Requirements of the HAL function handler's 2nd, 3rd and 4th parameters:
	
	- TInt aFunction : This enum value.
	
    - TAny* a1 : NULL.
				
	- TAny* a2 : NULL	
	
	@capability WriteDeviceData
	*/
	EDigitiserHalRestoreXYInputCalibration,


    /**
    Gets digitiser information as defined by the content of
    a TDigitiserInfoV01 (or TDigitiserInfoV02) struct.
    
    Requirements of the HAL function handler's 2nd, 3rd and 4th parameters:
	
	- TInt aFunction : This enum value.
	
   	- TAny* a1 : This is a pointer to a package buffer (TPckgBuf) containing a
	TDigitiserInfoV01 (or TDigitiserInfoV02) structure. The HAL function needs 
	to fill the members of this structure with the appropriate information.
				
	- TAny* a2 : NULL	
    
    @see TPckgBuf
    @see TDigitiserInfoV01
    */
	EDigitiserHalXYInfo,


    /**
    Tests whether the digitiser is on or off.
    
    Requirements of the HAL function handler's 2nd, 3rd and 4th parameters:
	
	- TInt aFunction : This enum value.
	
   	- TAny* a1 : A TBool value that needs to be set to ETrue (On), or EFalse (Off).
	
	- TAny* a2 : NULL	
    */
	EDigitiserHalXYState,


	/**
	Sets the digitiser either on or off.
	
	Requirements of the HAL function handler's 2nd, 3rd and 4th parameters:
	
	- TInt aFunction : This enum value.
	
	- TAny* a1 : A TBool that needs to be set to ETrue (On), or EFalse (Off).
	
	- TAny* a2 : NULL	
	
	@capability PowerMgmt
	*/
	EDigitiserHalSetXYState,

    /**
	Checks whether the pointing device supports 3rd dimension.
    
    Requirements of the HAL function handler's 2nd, 3rd and 4th parameters:
	
	- TInt aFunction : This enum value.
	
   	- TAny* a1 : A TBool value that needs to be set to ETrue (3D supported), or EFalse (3D not supported).
	
	- TAny* a2 : NULL	
    */
	EDigitiserHal3DPointer,

    /**	
    Sets the detection range above the screen.
    
    Requirements of the HAL function handler's 2nd, 3rd and 4th parameters:
	
	- TInt aFunction : This enum value.
	
   	- TAny* a1 : A TInt value that is the detection range in units of distance above the screen.
	
	- TAny* a2 : NULL	
    */
	EDigitiserHalSetZRange,
    
	/**
	@prototype
	
    Sets the  number of pointers it supports on the digitiser.
    
    Requirements of the HAL function handler's 2nd, 3rd and 4th parameters:
	
	- TInt aFunction : This enum value.
	
   	- TAny* a1 : A TInt value that 	sets the number of pointer to be supported by the digitiser driver
	
	- TAny* a2 : NULL	
    */
	EDigitiserHalSetNumberOfPointers,
	
	/**
	@prototype

    Gets digitiser information as defined by the content of
    a  TDigitiserInfoV02 struct.
    
    Requirements of the HAL function handler's 2nd, 3rd and 4th parameters:
	
	- TInt aFunction : This enum value.
	
   	- TAny* a1 : This is a pointer to a package buffer (TPckgBuf) containing a
	 TDigitiserInfoV02 structure. The HAL function needs 
	to fill the members of this structure with the appropriate information.
				
	- TAny* a2 : NULL	
    
    @see TPckgBuf
    @see TDigitiserInfoV02
    */
	EDigitiserHal3DInfo,

	/**
	Get or sets the digitiser driver's current orientation property.
	Requirements of the HAL function handler's 2nd, 3rd and 4th parameters:
	
	- TInt aFunction : This enum value.
    - TAny* a1 : if Set, a TDigitiserOrientation value the driver should now use	
			   : If Get, a pointer to a TDigitiserOrientation. The HAL function 
			   : needs to set its value to the current value used in the driver.	
	- TAny* a2 : Cast to a TInt. Should be assigned the value 
			   : EFalse - Get property; ETrue - Set property 
	
	@see HALData::TDigitiserOrientation
	@capability WriteDeviceData To set the property, None to read
	*/
	EDigitiserOrientation

	};




/**
@publishedPartner
@released

The set of function-ids that are  associated with the EHalGroupSound
HAL group.

Each enum value represents a specific characteristic of the sound driver,
and is passed as the second parameter to the HAL handler function
dealing with this group.

Note that a sound driver in Symbian OS is only maintained for backwards
compatibility with the Psion Series 5MX devices.

@see EHalGroupSound
*/
enum TSoundHalFunction
	{
	/**
	Gets sound driver information.
	
	Requirements of the HAL function handler's 2nd, 3rd and 4th parameters:
	
	- TInt aFunction : This enum value.
	    
	- TAny* a1 : This is a pointer to a package buffer (TPckgBuf) containing a
    TSoundInfoV1 structure. The HAL function needs to fill the members of
	this structure with the appropriate information.
									
	- TAny* a2 : NULL.
	
	@see TSoundInfoV1
    @see TPckgBuf
	*/
	ESoundHalSoundInfo,


	/**
	Sets whether a click is to be sounded on a key press.
	
	Requirements of the HAL function handler's 2nd, 3rd and 4th parameters:
	
	- TInt aFunction : This enum value.
	    
	- TAny* a1 : A TBool that needs to be set to ETrue (click is to be sounded),
	or EFalse (click is not to be sounded).
									
	- TAny* a2 : NULL.
	
	@capability WriteDeviceData
	*/
	ESoundHalSetKeyClickEnabled,


	/**
	Sets whether a click that is to be sounded on a key press, is loud or not.

	Requirements of the HAL function handler's 2nd, 3rd and 4th parameters:
	
	- TInt aFunction : This enum value.
	    
	- TAny* a1 : A TBool that needs to be set to ETrue (click is to be loud),
	or EFalse (click is not to be loud).
									
	- TAny* a2 : NULL.
	
	@capability WriteDeviceData
	*/
	ESoundHalSetKeyClickLoud,


	/**
	Sets whether a click is to be sounded when a pointing device touches
	the screen.

	Requirements of the HAL function handler's 2nd, 3rd and 4th parameters:
	
	- TInt aFunction : This enum value.
	    
	- TAny* a1 : A TBool that needs to be set to ETrue (click is to be sounded),
	or EFalse (click is not to be sounded).
									
	- TAny* a2 : NULL.
	
	@capability WriteDeviceData
	*/
	ESoundHalSetPointerClickEnabled,


	/**
	Sets whether a click that is to be sounded when a pointing device touches
	the screen, is loud or not.
	
	Requirements of the HAL function handler's 2nd, 3rd and 4th parameters:
	
	- TInt aFunction : This enum value.
	    
	- TAny* a1 : A TBool that needs to be set to ETrue (click is to be loud),
	or EFalse (click is not to be loud).
									
	- TAny* a2 : NULL.
	
	@capability WriteDeviceData
	*/
	ESoundHalSetPointerClickLoud,


	/**
	Sets whether the beep sound is enabled.
	
	Requirements of the HAL function handler's 2nd, 3rd and 4th parameters:
	
	- TInt aFunction : This enum value.
	    
	- TAny* a1 : A TBool that needs to be set to ETrue (beep is enabled),
	or EFalse (beep is not enabled).
									
	- TAny* a2 : NULL.
	
	@capability WriteDeviceData
	*/
	ESoundHalSetBeepEnabled,


	/**
	Sets whether the beep sound is to be loud.
	
	Requirements of the HAL function handler's 2nd, 3rd and 4th parameters:
	
	- TInt aFunction : This enum value.
	    
	- TAny* a1 : A TBool that needs to be set to ETrue (beep is loud),
	or EFalse (beep is not loud).
									
	- TAny* a2 : NULL.
	
	@capability WriteDeviceData
	*/
	ESoundHalSetBeepLoud,


	/**
	Sets whether the sound driver is enabled.
	
	Requirements of the HAL function handler's 2nd, 3rd and 4th parameters:
	
	- TInt aFunction : This enum value.
	    
	- TAny* a1 : A TBool that needs to be set to ETrue (driver is enabled),
	or EFalse (driver is not enabled).
									
	- TAny* a2 : NULL.
	
	@capability PowerMgmt
	*/
	ESoundHalSetSoundDriverEnabled,


	/**
	Sets whether the sound driver is to generate loud sounds.
	
	Requirements of the HAL function handler's 2nd, 3rd and 4th parameters:
	
	- TInt aFunction : This enum value.
	    
	- TAny* a1 : A TBool that needs to be set to ETrue (driver is to generate loud sounds),
	or EFalse (driver is not to generate loud sounds).
									
	- TAny* a2 : NULL.
	
	@capability WriteDeviceData
	*/
	ESoundHalSetSoundDriverLoud,


	/**
	Sets whether sound generation is enabled.
	
	Requirements of the HAL function handler's 2nd, 3rd and 4th parameters:
	
	- TInt aFunction : This enum value.
	    
	- TAny* a1 : A TBool that needs to be set to ETrue (sound generation is enabled),
	or EFalse (sound generation is not enabled).
									
	- TAny* a2 : NULL.
	
	@capability WriteDeviceData
	*/
	ESoundHalSetSoundEnabled,


	/**
	Sets whether generation of key clicks is to be overridden.
	
	Requirements of the HAL function handler's 2nd, 3rd and 4th parameters:
	
	- TInt aFunction : This enum value.
	    
	- TAny* a1 : A TBool that needs to be set to ETrue (key clicks to be overridden),
	or EFalse (key clicks not to be overridden).
									
	- TAny* a2 : NULL.
		
	@capability WriteDeviceData
	*/
	ESoundHalSetKeyClickOverridden,


    /**
    Tests whether the generation of key clicks is overridden.
    	
	Requirements of the HAL function handler's 2nd, 3rd and 4th parameters:
	
	- TInt aFunction : This enum value.
	    
	- TAny* a1 : A TBool into which the HAL handler puts ETrue (key clicks are overridden),
	or EFalse (key clicks are not overridden).
									
	- TAny* a2 : NULL.
    */
	ESoundHalKeyClickOverridden,


    /**
    Tests whether a click that is to be sounded on a key press, is loud or not.

	Requirements of the HAL function handler's 2nd, 3rd and 4th parameters:
	
	- TInt aFunction : This enum value.
	    
	- TAny* a1 : A TBool into which the HAL handler puts ETrue (click is loud),
	or EFalse (click is not loud).
									
	- TAny* a2 : NULL.
    */
	ESoundHalKeyClickLoud,


    /**
    Tests whether a click is to be sounded on a key press.
	
	Requirements of the HAL function handler's 2nd, 3rd and 4th parameters:
	
	- TInt aFunction : This enum value.
	    
	- TAny* a1 : A TBool into which the HAL handler puts ETrue (click is sounded),
	or EFalse (click is not sounded).
									
	- TAny* a2 : NULL.
    */
	ESoundHalKeyClickEnabled,


    /**
    Tests whether a click that is to be sounded when a pointing device touches
	the screen, is loud or not.
	
	Requirements of the HAL function handler's 2nd, 3rd and 4th parameters:
	
	- TInt aFunction : This enum value.
	    
	- TAny* a1 : A TBool into which the HAL handler puts ETrue (click is loud),
	or EFalse (click is not loud).
									
	- TAny* a2 : NULL.
    */
	ESoundHalPointerClickLoud,


    /**
    Tests whether a click is to be sounded when a pointing device touches
	the screen.

	Requirements of the HAL function handler's 2nd, 3rd and 4th parameters:
	
	- TInt aFunction : This enum value.
	    
	- TAny* a1 : A TBool into which teh HAL handler puts ETrue (click is to be sounded),
	or EFalse (click is not to be sounded).
									
	- TAny* a2 : NULL.
    */
	ESoundHalPointerClickEnabled,


    /**
    Generates a key click sound.
    
    Requirements of the HAL function handler's 2nd, 3rd and 4th parameters:
	
	- TInt aFunction : This enum value.
	    
	- TAny* a1 : NULL.
									
	- TAny* a2 : NULL.
    */
	ESoundHalKeyClick,


    /**
    Generates a pointer click sound.
    
    Requirements of the HAL function handler's 2nd, 3rd and 4th parameters:
	
	- TInt aFunction : This enum value.
	    
	- TAny* a1 : NULL.
									
	- TAny* a2 : NULL.
    */
	ESoundHalPointerClick,


    /**
    Generates a beep sound.
    
    Requirements of the HAL function handler's 2nd, 3rd and 4th parameters:
	
	- TInt aFunction : This enum value.
	    
	- TAny* a1 : NULL.
									
	- TAny* a2 : NULL.
    */
	ESoundHalBeep,


    /**
    Gets the maximum volume of the key click.
    
    Requirements of the HAL function handler's 2nd, 3rd and 4th parameters:
	
	- TInt aFunction : This enum value.
	    
	- TAny* a1 : A TInt value into which the HAL handler puts the maximum volume value.
									
	- TAny* a2 : NULL.
    */
	ESoundHalKeyClickVolumeMax,


    /**
    Gets the maximum volume of the pointer click.
    
    Requirements of the HAL function handler's 2nd, 3rd and 4th parameters:
	
	- TInt aFunction : This enum value.
	    
	- TAny* a1 : A TInt value into which the HAL handler puts the maximum volume value.
									
	- TAny* a2 : NULL.
    */
	ESoundHalPointerClickVolumeMax,
	};




/**
@publishedPartner
@released

The set of function-ids that are  associated with the EHalGroupMouse
HAL group.

Each enum value represents a specific characteristic of the mouse device,
and is passed as the second parameter to the HAL handler function
dealing with this group.
 
@see EHalGroupMouse
*/
enum TMouseHalFunction
	{
	
	
	/**
	Tests whether a mouse device is visible.
	
	Requirements of the HAL function handler's 2nd, 3rd and 4th parameters:
	
	- TInt aFunction : This enum value.
	
    - TAny* a1 : A TBool into which the HAL handler
    puts ETrue (for visible) or EFalse (for not-visible).
				
	- TAny* a2 : NULL
	*/
	EMouseHalMouseState,


	/**
	Sets the state of the mouse device, i.e. whether it is visible or invisible.
	
	Requirements of the HAL function handler's 2nd, 3rd and 4th parameters:
	
	- TInt aFunction : This enum value.
	
    - TAny* a1 : A TBool that needs to be set to ETrue (for visible)
    or EFalse (for not-visible).
				
	- TAny* a2 : NULL
	
	@capability MultimediaDD
	*/
	EMouseHalSetMouseState,


    /**
    Gets the mouse speed, i.e. how fast the mouse pointer moves.
    
    Requirements of the HAL function handler's 2nd, 3rd and 4th parameters:
	
	- TInt aFunction : This enum value.
	
    - TAny* a1 : A TInt value into which the HAL handler puts the speed value.
    This is expected to be a value in the range 0 to 255.
				
	- TAny* a2 : NULL
    */
	EMouseHalMouseSpeed,


	/**
	Sets the mouse speed, i.e. how fast the mouse pointer moves.
	
	Requirements of the HAL function handler's 2nd, 3rd and 4th parameters:
	
	- TInt aFunction : This enum value.
	
    - TAny* a1 : A TInt value containing the speed value to be set
    This is expected to be a value in the range 0 to 255.
				
	- TAny* a2 : NULL
	
	@capability MultimediaDD
	*/
	EMouseHalSetMouseSpeed,
 
 
    /**
    Gets the mouse acceleration, i.e. how much the mouse pointer accelerates
    as the user moves it faster.
    
    Requirements of the HAL function handler's 2nd, 3rd and 4th parameters:
	
	- TInt aFunction : This enum value.
	
    - TAny* a1 : A TInt value into which the HAL handler puts the acceleration value.
    This is expected to be a value in the range 0 to 255.
				
	- TAny* a2 : NULL
    */
	EMouseHalMouseAcceleration,


	/**
	Sets the mouse acceleration, i.e. how much the mouse pointer accelerates
    as the user moves it faster.
	
	Requirements of the HAL function handler's 2nd, 3rd and 4th parameters:
	
	- TInt aFunction : This enum value.
	
    - TAny* a1 : A TInt value containing the acceleration value to be set
    This is expected to be a value in the range 0 to 255.
				
	- TAny* a2 : NULL
	
	@capability MultimediaDD
	*/
	EMouseHalSetMouseAcceleration,


    /**
    Gets the state of the mouse buttons.
    
    This is not currently supported.
	*/
	EMouseHalMouseButtonState,
	
	
	/**
	Gets information about the mouse display characteristics. 
	
	Requirements of the HAL function handler's 2nd, 3rd and 4th parameters:
	
	- TInt aFunction : This enum value.
	
	- TAny* a1 : This is a pointer to a package buffer (TPckgBuf) containing a
	TMouseInfoV01 structure. The HAL function needs to fill the members of
	this structure with the appropriate information.
	
    - TAny* a2 : NULL.
    
    @see TMouseInfoV01
    @see TPckgBuf
	*/
	EMouseHalMouseInfo,
	};

/**
@internalComponent
*/
enum TEmulatorHalFunction
	{
	EEmulatorHalStringProperty,
	EEmulatorHalIntProperty,
	EEmulatorHalBoolProperty,
	EEmulatorHalMapFilename,
	EEmulatorHalColorDepth,
	EEmulatorHalSetFlip,
	EEmulatorHalCPUSpeed,
	EEmulatorHalNumberOfScreens,
	EEmulatorHalSetDisplayChannel,
	};

/**
@publishedPartner
@released

The set of function-ids that are  associated with the EHalGroupKeyboard
HAL group.

Each enum value represents a specific characteristic of the keyboard,
and is passed as the second parameter to the HAL handler function
dealing with this group.
  
@see EHalGroupKeyboard
*/
enum TKeyboardHalFunction
	{
	
	/**
    Tests whether the keyboard is on or off.
    
    Requirements of the HAL function handler's 2nd, 3rd and 4th parameters:
	
	- TInt aFunction : This enum value.
	
   	- TAny* a1 : A TBool into which the HAL handler puts ETrue (for On),
    or EFalse (for Off).
	
	- TAny* a2 : NULL	
	*/
	EKeyboardHalKeyboardState,


	/**
	Sets the keyboard either on or off.
	
	Requirements of the HAL function handler's 2nd, 3rd and 4th parameters:
	
	- TInt aFunction : This enum value.
	
	- TAny* a1 :A TBool that needs to be set to ETrue (On), or EFalse (Off).
	
	- TAny* a2 : NULL	
	
	@capability PowerMgmt
	*/
	EKeyboardHalSetKeyboardState,


    /**
    Gets information about the keyboard.
    
    Requirements of the HAL function handler's 2nd, 3rd and 4th parameters:
	
	- TInt aFunction : This enum value.
	
	- TAny* a1 : This is a pointer to a package buffer (TPckgBuf) containing a
	TKeyboardInfoV01 structure. The HAL function needs to fill the members of
	this structure with the appropriate information.
	
    - TAny* a2 : NULL.
    
    @see TKeyboardInfoV01
    @see TPckgBuf
    */
	EKeyboardHalKeyboardInfo,
	};

/**
@publishedPartner
@released

The set of function-ids that are  associated with the EHalGroupVM
HAL group.

@see EHalGroupVM
*/
enum TVMHalFunction
	{
	/**
	Evict the contents of the virtual memory cache and reduce it to its minimum size.

	@capability WriteDeviceData
	@test
	*/
	EVMHalFlushCache,

	/**
	Change the minimum and maximum RAM sizes used for the virtual memory cache.
	These values may be silently restricted to platforn specific limits.
	If required, GetCacheSize can be used to verify sizes actually applied to the system.

	If there is not enough memory to set the specified cache size then KErrNoMemory is
	returned, however the cache size may still have been modified in an attempt to 
	service the request.

	This hal function expects two TUint arguments.
	The first argument (a1)	is the minimum size for cache in bytes.
	The second argument (a2) is the maximum size for cache in bytes; using zero for
	this value will restore cache sizes to the initial values used after boot.

	The return value from the function is one of:
			KErrNone, if successful; 
			KErrNoMemory if there is not enough memory;
			KErrArgument if a1>a2;
			otherwise one of the other system wide error codes.

	Example usage:
	@code
	TUint minRam = 128*1024; // 128k
	TUint maxRam = KMaxTUint; // unlimited
	TInt r = UserSvr::HalFunction(EHalGroupVM,EVMHalSetCacheSize,(TAny*)minRam,(TAny*)maxRam));
	if(r!=KErrNone)
		anErrorOccured();
	@endcode

	@capability WriteDeviceData
	@test
	*/
	EVMHalSetCacheSize,

	/**
	Get the sizes for the virtual memory cache.
	The first argument (a1) is a pointer to a #SVMCacheInfo object.
	The second argument (a2) must be zero.
	@test
	*/
	EVMHalGetCacheSize,

	/**
	Get paging event information.
	The first argument (a1) is a pointer to a descriptor whose contents will be filled
	with a #SVMEventInfo object.
	The second argument (a2) must be zero.
	@test
	*/
	EVMHalGetEventInfo,

	/**
	Reset the counters obtained with EVMHalGetEventInfo.
	@test
	*/
	EVMHalResetEventInfo,

	/**
	@internalTechnology
	@test
	*/
	EVMHalGetOriginalRomPages,

	/**
	@internalTechnology
	@test
	*/
	EVMPageState,

	/**
	@internalTechnology
	@test
	*/
	EVMHalGetConcurrencyInfo,

	/**
	@internalTechnology
	@test
	*/
	EVMHalResetConcurrencyInfo,

	/**
	@internalTechnology
	@test
	*/
	EVMHalGetPagingBenchmark,

	/**
	@internalTechnology
	@test
	*/
	EVMHalResetPagingBenchmark,

	/**
	Gets information about the size of the swap partition, and how much is currently used.
	The first argument (a1) should be a pointer to the SVMSwapInfo structure to write.
	@return KErrNone if successful, or KErrNotSupported if data paging is not supported.
	@internalTechnology
	@prototype
	*/
	EVMHalGetSwapInfo,

	/**
	Gets information about the current thrashing level.

	This is a number between 0 and 255 representing how close the system is to thrashing, where 0
	indicates no thrashing and 255 indicates severe thrashing.

	@return The current thrashing level.
	@internalTechnology
	@prototype
	*/
	EVMHalGetThrashLevel,

	/**
	Set the available swap space thresholds at which low memory notifications will be generated.

	Notifications are generated through the RChangeNotifier API, using the enumeration value
	EChangesFreeMemory.
	
	The first argument (a1) should contain a pointer to a SVMSwapThresholds structure. The
	thresholds are specified in bytes.

	@return KErrNone if successful, KErrNotSupported if data paging is not supported or KErrArgument
	if the thresholds are larger than the size of the swap partition, or if the low threshold is
	larger than the good threshold.
	
	@see RChangeNotifier
	@see TChanges
	*/ 
	EVMHalSetSwapThresholds,
	
	/**
	Set the thrash level thresholds at which thrash level notifications will be generated.

	Notifications are generated through the RChangeNotifier API, using the enumeration value
	EChangesThrashLevel.
	
	The first argument (a1) should contain the thrashing threshold, and the seond one (a2) should
	contain the good threshold.

	The thresholds are specified as numbers between 0 and 255 inclusive.

	@return KErrNone if successful, KErrArgument if the thresholds are out of range or if the
	thrashing threshold is smaller than the good threshold.
	
	@see RChangeNotifier
	@see TChanges
	*/ 
	EVMHalSetThrashThresholds,

	/**
	Indicate whether the data paging media driver supports access by physical address.

	@return 1 if it does, 0 if it does not and KErrNotSupported if data paging is not enabled.
	*/
	EVMHalGetPhysicalAccessSupported,

	/**
	Indicate whether the data paging media driver currently uses physical access for writing out
	dirty data pages to swap.

	@return 1 if it does, 0 if it does not and KErrNotSupported if data paging is not enabled.
	*/
	EVMHalGetUsePhysicalAccess,

	/**
	Set whether physical access is used for writing out dirty data pages to swap.

	The first argument (a1) should contain zero or one to indicate whether to disable or enable
	physical access respectively.
	
	@return KErrNone if successful, KErrNotSupported if data paging is not enabled.
	*/
	EVMHalSetUsePhysicalAccess,

	/**
	Get the data paging media driver's preferred write size.

	@return Log2 of the preferred write size in pages, or KErrNotSupported if data paging is not
	enabled.
	*/
	EVMHalGetPreferredDataWriteSize,

	/**
	Get the number of pages that the pager attempts to write at a time when paging out dirty pages
	to swap.

	@return Log2 of the current write size in pages, or KErrNotSupported if data paging is not
	enabled.
	*/
	EVMHalGetDataWriteSize,

	/**
	Set the number of pages that the pager attempts to write at a time when paging out dirty pages
	to swap.
	
	The first argument (a1) should contain log2 of the write size in pages.

	@return KErrArgument if the value is out of range, or KErrNotSupported if data paging is not
	enabled.
	*/
	EVMHalSetDataWriteSize,

	/**
	@internalTechnology
	@test

	Simulates a paging error in a specified context.
	
	For testing purposes, this causes the paging system to report an error the next time an
	operation occurs that could generate an error with the specified context.

	The first argument (a1) contains one of the contexts described by TPagingErrorContext.

	@return KErrArgument if the value is out of range, or KErrNotSupported on memory models that do
	not support this.
	*/
	EVMHalDebugSetFail,
	};


/**
Paging event information.
@publishedPartner
@test
*/
struct SVMEventInfo
	{
	/**
	The total number of page faults which have occurred.
	*/
	TUint64 iPageFaultCount;

	/**
	The total number of page faults which resulted in reading a page
	from storage media.
	*/
	TUint64 iPageInReadCount;

	// do not add new members to this struct.
	// instead, derive a new SVMEventInfoV2 class from this and add new members to that.
	};

	
/**
VM cache information.
@publishedPartner
@test
*/
struct SVMCacheInfo
	{
	/**
	The minimum size of virtual memory cache, in bytes.
	*/
	TUint32 iMinSize;

	/**
	The maximum size of virtual memory cache, in bytes.
	*/
	TUint32 iMaxSize;

	/**
	The current size of virtual memory cache, in bytes.
	This may be larger than iMaxSize.
	*/
	TUint32 iCurrentSize;

	/**
	The current size of the freeable memory in the virtual memory cache, in bytes.
	*/
	TUint32 iMaxFreeSize;

	// do not add new members to this struct, this is a compatability break
	};


/**
Enumeration defining the bitmask returned from a call to the demnd paging HAL function
EDemandPagingGetPageState.

@internalComponent
@test
*/
enum TDemandPagingPageState
	{
	// Values common to both moving and multiple memory models
	EPageStateInRom					= 1<<16,
	EPageStateInRamCode				= 1<<17,
	EPageStatePaged					= 1<<18,
	EPageStatePageTablePresent		= 1<<19,
	EPageStatePtePresent			= 1<<20,
	EPageStatePteValid				= 1<<21,
	// Values specific to multiple memory model
	EPageStateCodeChunkPresent		= 1<<22,		
	EPageStatePhysAddrPresent		= 1<<23
	};


/**
Information about concurrency in the demand paging system.

@internalComponent
@test
*/
struct SPagingConcurrencyInfo
	{
	/**
	The maximum observed number of threads waiting to page in.
	*/
	TInt iMaxWaitingCount;

	/**
	The maximum observed number of threads paging in.
	*/
	TInt iMaxPagingCount;
	};


/**
Enumeration of demand paging benchmarks.

@internalComponent
@test
*/
enum TPagingBenchmark
	{
	EPagingBmReadRomPage,
	EPagingBmReadCodePage,
	EPagingBmDecompress,
	EPagingBmSetCodePageFree,
	EPagingBmSetCodePageOld,
	EPagingBmReadMedia,
	EPagingBmFixupCodePage,
	EPagingBmReadDataPage,
	EPagingBmWriteDataPage,
	EPagingBmDeleteNotifyDataPage,
	EPagingBmReadDataMedia,
	EPagingBmWriteDataMedia,
	EPagingBmRejuvenate,  // only implemented on FMM
		
	EMaxPagingBm
	};


/**
Benchmark information for a specific operation.  All times are in terms of the system's fast timer.

@internalComponent
@test
*/
struct SPagingBenchmarkInfo
	{
	/**
	Number of times the operation has been executed.
	*/
	TInt iCount;
	
	/**
	Total time elapsed while executing the operation.
	*/
	TInt64 iTotalTime;

	/**
	Maximum time recorded for a single execution.
	*/
	TInt iMaxTime;

	/**
	Minimum time recorded for a single execution.
	*/
	TInt iMinTime;
	};

/**
Information about concurrency of ROM demand paging requests in the media subsystem.

@internalComponent
@test
*/
struct SMediaROMPagingConcurrencyInfo
	{
	/**
	The maximum number of pending page in requests in the main queue any time during this session.
	*/
	TUint8 iMaxReqsInPending;

	/**
	The maximum number of pending page in requests in the deferred queue any time during this session.
	*/
	TUint8 iMaxReqsInDeferred;

	/**
	The maximum number of deferrals of any single page in request during this session.
	*/
	TUint8 iMaxDeferrals;

	/**
	The total number of times the page in DFC run with an empty main queue during this session.
	*/
	TUint8 iTotalRunDry;

	/**
	The total number of page in requests first-time deferred during this session.
	*/
	TUint iTotalFirstTimeDeferrals;

	/**
	The total number of page in requests re-deferred during this session (deferred again after being picked out of deferred queue).
	*/
	TUint iTotalReDeferrals;

	/**
	The total number of page in requests serviced from main queue when completing an asynchronous request.
	*/
	TUint iTotalSynchServicedFromMainQ;

	/**
	The total number of page in requests deferred after being picked out of main queue when completing an asynchronous request.
	*/
	TUint iTotalSynchDeferredFromMainQ;
	
	/**
	The total number of page in requests issued whilst processing other page in requests.
	*/
	TUint iTotalConcurrentReqs;

	/**
	The total number of page in requests issued with at least one queue not empty.
	*/
	TUint iTotalReqIssuedNonEmptyQ;

	/**
	The total number of times the main queue was emptied when completing an asynchronous request during this session.
	*/
	TUint iTotalSynchEmptiedMainQ;

	/**
	The total number of times the page in DFC was cancelled because the main queue was synchronously emptied.
	*/
	TUint iTotalDryRunsAvoided;
	};

/**
Information about concurrency of Code demand paging requests in the media subsystem.

@internalComponent
@test
*/
struct SMediaCodePagingConcurrencyInfo
	{
	/**
	The maximum number of pending page in requests in the main queue any time during this session.
	*/
	TUint8 iMaxReqsInPending;

	/**
	The maximum number of pending page in requests in the deferred queue any time during this session.
	*/
	TUint8 iMaxReqsInDeferred;

	/**
	The maximum number of deferrals of any single page in request during this session.
	*/
	TUint8 iMaxDeferrals;

	/**
	Spare field
	*/
	TUint8 iSpare;

	/**
	The total number of page in requests first-time deferred during this session.
	*/
	TUint iTotalFirstTimeDeferrals;

	/**
	The total number of page in requests re-deferred during this session (deferred again after being picked out of deferred queue).
	*/
	TUint iTotalReDeferrals;

	/**
	The total number of page in requests serviced from main queue when completing an asynchronous request.
	*/
	TUint iTotalSynchServicedFromMainQ;

	/**
	The total number of page in requests deferred after being picked out of main queue when completing an asynchronous request.
	*/
	TUint iTotalSynchDeferredFromMainQ;
	
	/**
	The total number of page in requests issued whilst processing other page in requests.
	*/
	TUint iTotalConcurrentReqs;

	/**
	The total number of page in requests issued with at least one queue not empty.
	*/
	TUint iTotalReqIssuedNonEmptyQ;
	};


/**
Information about concurrency of Data demand paging requests in the media subsystem.

@internalComponent
@test
*/
struct SMediaDataPagingConcurrencyInfo
	{
	/**
	The maximum number of pending page in requests in the main queue any time during this session.
	*/
	TUint8 iMaxReqsInPending;

	/**
	The maximum number of pending page in requests in the deferred queue any time during this session.
	*/
	TUint8 iMaxReqsInDeferred;

	/**
	The maximum number of deferrals of any single page in request during this session.
	*/
	TUint8 iMaxDeferrals;

	/**
	The total number of times the page in DFC run with an empty main queue during this session.
	*/
	TUint8 iTotalRunDry;

	/**
	The total number of page in requests first-time deferred during this session.
	*/
	TUint iTotalFirstTimeDeferrals;

	/**
	The total number of page in requests re-deferred during this session (deferred again after being picked out of deferred queue).
	*/
	TUint iTotalReDeferrals;

	/**
	The total number of page in requests serviced from main queue when completing an asynchronous request.
	*/
	TUint iTotalSynchServicedFromMainQ;

	/**
	The total number of page in requests deferred after being picked out of main queue when completing an asynchronous request.
	*/
	TUint iTotalSynchDeferredFromMainQ;
	
	/**
	The total number of page in requests issued whilst processing other page in requests.
	*/
	TUint iTotalConcurrentReqs;

	/**
	The total number of page in requests issued with at least one queue not empty.
	*/
	TUint iTotalReqIssuedNonEmptyQ;

	/**
	The total number of times the main queue was emptied when completing an asynchronous request during this session.
	*/
	TUint iTotalSynchEmptiedMainQ;

	/**
	The total number of times the page in DFC was cancelled because the main queue was synchronously emptied.
	*/
	TUint iTotalDryRunsAvoided;
	};


/**
Information about paging requests in the media subsystem.

@internalComponent
@test
*/
struct SMediaPagingInfo
	{
	/**
	The total number of ROM page in requests
	*/
	TInt	iRomPageInCount;
	/**
	The total number of Code page in requests
	*/
	TInt	iCodePageInCount;
	/**
	The total number of Data page in requests
	*/
	TInt	iDataPageInCount;
	/**
	The total number of Data page out requests
	*/
	TInt	iDataPageOutCount;
	/**
	The total number of "background" Data page out requests
	i.e. a page out which only occurs when the media is otherwise idle
	*/
	TInt	iDataPageOutBackgroundCount;
	};



/**
Swap partition information.
@internalAll
@prototype
*/
struct SVMSwapInfo
	{
	/**
	The size of the swap partition, in bytes.
	*/
	TUint64 iSwapSize;

	/**
	The amount of swap currently free, in bytes.
	*/
	TUint64 iSwapFree;

	// do not add new members to this struct, this is a compatability break
	};



/**
Free swap notification thresholds.
@internalAll
@prototype
*/
struct SVMSwapThresholds
	{
	/**
	The low threshold, in bytes
	*/
	TUint64 iLowThreshold;

	/**
	The good threshold, in bytes.
	*/
	TUint64 iGoodThreshold;

	// do not add new members to this struct, this is a compatability break
	};


/**
@internalComponent
@test

Error context information for use by #DPager::EmbedErrorContext and #DPager::ExtractErrorContext.
*/
enum TPagingErrorContext
	{
	EPagingErrorContextNone = 0,
	EPagingErrorContextRomRead,
	EPagingErrorContextRomDecompress,
	EPagingErrorContextCodeRead,
	EPagingErrorContextCodeDecompress,
	EPagingErrorContextDataRead,
	EPagingErrorContextDataWrite,

	EMaxPagingErrorContext
	};


/**
@internalComponent

The set of function-ids that are  associated with the EHalGroupRam
HAL group.

@see EHalGroupRam
@test
*/
enum TRamHalFunction
	{
	/**
	Retrieve the number of RAM Zones configured for this variant.

	The first argument (a1) is a TUint in which to store the zone count.
	The second argument (a2) must be zero.
	@test
	*/
	ERamHalGetZoneCount,
	
	/**
	Retrieve configuration information about a zone.

	The first argument (a1) is a TUint indicating the index of the zone into the zone array.
	The second argument (a2) is a pointer to a struct SRamZoneConfig in which to store the data.
	@test
	*/
	ERamHalGetZoneConfig,
	
	/**
	Retrieve utilisation information about a zone.

	The first argument (a1) is a TUint indicating the index of the zone into the zone array.
	The second argument (a2) is a pointer to a struct SRamZoneUtilisation in which to store the data.
	@test
	*/
	ERamHalGetZoneUtilisation,

	};

/**
Ram zone configuration information.
@internalComponent
@test
*/
struct SRamZoneConfig
	{
	/**
	ID number of the RAM zone
	*/
	TUint iZoneId;

	/**
	The index of the RAM zone in the RAM zone array 
	*/
	TUint8 iZoneIndex;

	/**
	Physical base address of the RAM zone
	*/
	TUint32 iPhysBase;

	/**
	Physical end address of the RAM zone
	*/
	TUint32 iPhysEnd;

	/**
	The total number of pages that are in the RAM zone
	*/
	TUint32 iPhysPages;
	
	/**
	Preference value for the RAM zone, lower preference RAM zones are used first
	*/
	TUint8 iPref;	

	/**
	Zone flags - specify whether the RAM zone should be reserved for contiguous buffer or h/w etc
	*/
	TUint iFlags;

	};

/**
Ram zone utilisation information.
@internalComponent
@test
*/
struct SRamZoneUtilisation
	{
	/**
	ID number of the RAM zone
	*/
	TUint iZoneId;			

	/**
	The index of the RAM zone in the zone array 
	*/
	TUint8 iZoneIndex;		

	/**
	The total number of pages that are in the RAM zone
	*/
	TUint32 iPhysPages;		

	/**
	The number of pages free in the RAM zone 
	*/
	TUint32 iFreePages;	
	
	/**
	The number of 'unknown' (reserved at startup or holes in the zone) pages allocated in the RAM zone
	*/
	TUint32 iAllocUnknown;

	/**
	The number of fixed pages allocated in the RAM zone 
	*/
	TUint32 iAllocFixed; 

	/**
	The number of movable pages allocated in the RAM zone 
	*/
	TUint32 iAllocMovable; 

	/**
	The number of discardable pages allocated in the RAM zone 
	*/
	TUint32 iAllocDiscardable; 

	/**
	The number of other pages allocated in the RAM zone 
	*/
	TUint32 iAllocOther; 

	};
#endif
