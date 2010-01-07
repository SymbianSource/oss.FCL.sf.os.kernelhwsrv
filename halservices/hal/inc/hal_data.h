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
// hal\inc\hal_data.h
// 
//


#ifndef __HAL_DATA_H__
#define __HAL_DATA_H__
#define bitmask enum

class HALData
/**
@publishedPartner
@released

Sets of attributes and values used by HAL functions.
*/
	{
public:
    /**
    A set of enumerators that identifies hardware attributes.
    The enumerators are passed to HAL::Get() and HAL::Set().
    
    They are also used by the HAL accessor functions.
    
    @see HAL::Get()
    @see HAL::Set()
    */
	enum TAttribute
		{
		/**
		Identifies the manufacturer of a device.
        If this is not enumerated in TManufacturer, then the manufacturer must
        obtain a value from the Symbian registry.
        
        @see HALData::TManufacturer
		*/
		EManufacturer,
		
		
		/**
		The device specific hardware version number, as defined by
		the device manufacturer.
		*/
		EManufacturerHardwareRev,
		
		
		/**
		The device specific version number, as defined by
		the device manufacturer.
		*/
		EManufacturerSoftwareRev,
		
		
		/**
		The device specific software version number, as defined by
		the device manufacturer.
		*/
		EManufacturerSoftwareBuild,

		
		/**
		The device specific model number, as defined by
		the device manufacturer.
		*/
		EModel,
		
				
		/**
		This is the device specific UID, It is unique to the class /model
		of device. A value must be obtained from Symbian's UID registry for
		this attribute.
		*/
		EMachineUid,
		
		
		/**
		The Symbian OS specified device family identifier.
		If the device family is not one of those enumerated by TDeviceFamily,
		then the licensee must obtain a UID from Symbian for this attribute.
		
		@see HALData::TDeviceFamily
		*/
		EDeviceFamily,

		
		/**
		The Symbian OS specified device family version.
		*/
		EDeviceFamilyRev,

		
		/**
		The CPU architecture used by this device. The values are enumerated
		by TCPU.
		
		@see HALData::TCPU
		*/
		ECPU,
		
		
		/**
		A revision number for the CPU architecture.
		*/
		ECPUArch,

		
		/**
		This is the default ABI used by CPU for user applications.
		The values are enumerated by HALData::TCPUABI.
		*/
		ECPUABI,

		
		/**
		The processor speed in KHz.
		*/
		ECPUSpeed,

		
		/**
		The reason for most recent system boot.
        This is dynamic and readonly; the values are enumerated by
        TSystemStartupReason.

		@see HALData::TSystemStartupReason
		*/
		ESystemStartupReason,

		
		/**
		This is the last exception code, in the case of system reboot.
		This is dynamic and readonly.
		*/
		ESystemException,
		
		
		/**
		The time between system ticks, in microseconds.
		*/
		ESystemTickPeriod,
		
		
		/** 
		The total system RAM, in bytes.
		*/
		EMemoryRAM,
		
		
		/**
		The currently free system RAM.
		
		This is dynamic and readonly.
		*/
		EMemoryRAMFree,

		
		/**
		The total System ROM, in bytes.
		*/
		EMemoryROM,
	
		
		/**
		The MMU page size in bytes.
		*/
		EMemoryPageSize,
	
		
		/**
		Indicates the state of the power supply.
        
        It has the values:
        1 = Power is good (i.e. external power is available,
        or the 'power' battery is >= low);
        0 = otherwise.
        
        This is dynamic and readonly.
		*/
		EPowerGood,
	
		
		/**
        The System (or 'Main') battery power level.
        The allowable values are enumerated by TPowerBatteryStatus

		This is dynamic and readonly,
		
		@see HALData::TPowerBatteryStatus
		*/
		EPowerBatteryStatus,
	
		
		/**
		Indicates whether a backup power supply is available.
        It has the values:
        0 = the device does not support (or need) a backup battery source;
        1 = a backup batter source is present.
		This is dynamic and readonly
		*/
		EPowerBackup,
	
		
		/**
        The power level for backup power.
        
        It has the values enumerated by TPowerBackupStatus.

		This is dynamic and readonly.
		
		@see HALData::TPowerBackupStatus
		*/
		EPowerBackupStatus,
	
		
		/**
		Indicates the state of the external power.

		It has the values:
		0 = external power is not in use;
		1 = external power is in use.
		        
        This is dynamic and readonly.
		*/
		EPowerExternal,
	
		
		/**
		A bitmask that describes the available keyboard types (it may support
		more than one).

        @see HALData::TKeyboard
		*/
		EKeyboard,
	
		
		/**
		*/
		EKeyboardDeviceKeys,
	
		
		/**
		*/
		EKeyboardAppKeys,
	
		
		/**
		Indicates whether the device can produce a click sound for
		each keypress.
		
		It has the values:
		0 = the device cannot produce a click sound for each keypress;
		1 = the device can produce a click sound.
		*/
		EKeyboardClick,
	
		
		/**
		The state of keyboard clicking.

        It has the values:
        0 = key click disabled;
        1 = key click enabled.
        
		This is dynamic and writeable.

		@capability WriteDeviceData needed to Set this attribute
		*/
		EKeyboardClickState,
	
		
		/**
		The keyboard click volume level.

		It can take a value in the range 0 to EKeyboardClickVolumeMax.
        
        This is dynamic and writeable.
        
        @see HALData::EKeyboardClickVolumeMax

		@capability WriteDeviceData needed to Set this attribute
		*/
		EKeyboardClickVolume,
	
		
		/**
		The maximum value for EKeyboardClickVolume.
		
		@see HALData::EKeyboardClickVolume
		*/
		EKeyboardClickVolumeMax,
	
		
		/**
		The screen horizontal dimension in pixels.
		*/
		EDisplayXPixels,
	
		
		/**
		The screen vertical dimension in pixels.
		*/
		EDisplayYPixels,
	
		
		/**
		The screen horizontal dimension in twips.
		*/
		EDisplayXTwips,
	
		
		/**
		The screen vertical dimension in twips.
		*/
		EDisplayYTwips,
	
		
		/**
		The number of hues (colors or shades of grey) displayable on
		the screen.
		*/
		EDisplayColors,
	
		
		/**
		The state of the display.
		
		It has the values:
		0 = screen is turned off;
		1 = screen is on.
		
		This is dynamic and writeable.

		@capability PowerMgmt needed to Set this attribute
		*/
		EDisplayState,
	
		
		/**
		The screen contrast level.
   		It can take a value in the range 0 to EDisplayContrastMax.
        
        This is dynamic and writeable

		@see HALData::EDisplayContrastMax

		@capability WriteDeviceData needed to Set this attribute
		*/
		EDisplayContrast,
	
		
		/**
		The maximum value for EDisplayContrast
		
		@see HALData::EDisplayContrast
		*/
		EDisplayContrastMax,
		
		
		/**
		Indicates whether there is a backlight on the device.

		It has the values:
		0 = there is no screen backlight;
		1 = a screen backlight is present.
		*/
		EBacklight,
			
		
		/**
		The current status of the backlight.

		It has the values:
		0 = off;
		1 = on.
        
        This is dynamic and writeable.

		@capability WriteDeviceData needed to Set this attribute
		*/
		EBacklightState,
			
		
		/**
		Indicates whether a pen or digitizer is available for input.

		It has the values:
		0 = a pen/digitizer is not available for input;
		1 = a pen/digitizeris present.
		*/
		EPen,
			
		
		/**
		The pen/digitizer horizontal resolution, in pixels.
		*/
		EPenX,
			
		
		/**
		The pen/digitizer vertical resolution, in pixels.
		*/
		EPenY,
			
		
		/**
		Indicates whether a pen tap will turn the display on.

        It has the values:
        0 = a pen tap has no effect;
        1 = a pent tap or press enables the display.
        
        This is dynamic and writeable.

		@capability WriteDeviceData needed to Set this attribute
		*/
		EPenDisplayOn,
			
		
		/**
		Indicates whether the device can produce a click sound for
		each pen tap.

		It has the values:
        0 = the device cannot produce a click sound
        1 = production of a click sound is supported by the device.
		*/
		EPenClick,
			
		
		/**
		The state of pen clicking.
		
		It has the values:
		0 = pen clicking is disabled;
		1 = pen clicking is enabled.
		
        This is dynamic and writable.

		@capability WriteDeviceData needed to Set this attribute
		*/
		EPenClickState,
			
		
		/**
		The pen click volume level.
        It can take a value in the range 0 to EPenClickVolumeMax.
        
        This value is dynamic and writable.
        
        @see HALData::EPenClickVolumeMax

		@capability WriteDeviceData needed to Set this attribute
		*/
		EPenClickVolume,
			
		
		/**
		The maximum value for EPenClickVolume.
		
		@see HALData::EPenClickVolume
		*/
		EPenClickVolumeMax,
			
		
		/**
		Indicates whether a mouse is available for input.
		
		It has the values:
		0 = there is no mouse available pen/digitizer is present;
		1 = a mouse is available for input.
		*/
		EMouse,
			
		
		/**
		The mouse horizontal resolution, in pixels.
		*/
		EMouseX,
			
		
		/**
		The mouse vertical resolution, in pixels.
		*/
		EMouseY,
			
		
		/**
		Describes the mouse cursor visibility.

        The value is enumerated by TMouseState.
        
		This is dynamic and writable.
		@see HALData::TMouseState
		
		@capability MultimediaDD needed to Set this attribute
		*/
		EMouseState,
			
		
		/**
        Reserved for future use.
		@capability MultimediaDD needed to Set this attribute
		*/
		EMouseSpeed,
		
		
		/**
		Reserved for future use.
		@capability MultimediaDD needed to Set this attribute
		*/
		EMouseAcceleration,
		
		
		/**
		The number of buttons on the mouse.
		*/
		EMouseButtons,
		
		
		/**
        A bitmask defining the state of each button.

        For each bit, it has values:
        0 = up;
        1 = down.
        
		This is dynamic and read only.
		*/
		EMouseButtonState,
		
		
		/**
		Defines the state of the case.
		
        It has the values:
        0 = case closed;
        1 = case opened.
        
        This is dynamic and read only.
		*/
		ECaseState,
		
		
		/**
		Indicates whether the device has a case switch, that actions when
		the case opens and closes.
		
        It has values:
        0 = no;
        1 = yes.
		*/
		ECaseSwitch,
		
		
		/**
		Indicates whether the device is to switch on when case opens.
		
		It has the values:
		0 = disable device switchon when the case opens;
		1 = enable device  switchon when the case opens.
        
        This is dynamic and writeable.

		@capability WriteDeviceData needed to Set this attribute
		*/
		ECaseSwitchDisplayOn,
		
		
		/**
        Indicates whether the device is to switch off when case close.

		It has the values:
		0 = disable device switchoff when the case closes;
		1 = enable device switchoff when the case closes.
        
        This is dynamic and writeable.

		@capability WriteDeviceData needed to Set this attribute
		*/
		ECaseSwitchDisplayOff,
		
		
		/**
		The number of LEDs on the device.
		*/
		ELEDs,
		
		
		/**
        A bitmask defining the state of each LED.

        For each bit, it has values:
        0 = off;
        1 = on.
		
		This is dynamic and writeable.
		*/
		ELEDmask,
		
		
		/**
		Indicates how the phone hardware is connected.
		
		It has the values:
		0 = phone hardware is not permanently connected;
		1 = phone hardware is permanently connected.
		*/
		EIntegratedPhone,
		
		
		/**
		@capability WriteDeviceData needed to Set this attribute
		*/
		EDisplayBrightness,
	
		
		/**
		*/
		EDisplayBrightnessMax,
	
		
		/**
		Indicates the state of the keyboard backlight.
        
        It has the values:
        0 = keyboard backlight is off;
        1 = keyboard backlight is on.

		This is dynamic and writeable.

		@capability PowerMgmt needed to Set this attribute
		*/
		EKeyboardBacklightState,
	
		
		/**
		Power supply to an accessory port.

        It has the values:
        0 = turn off power to an accessory port on the device;
        1 = turn on power.
        
        This is dynamic and writeable.

		@capability PowerMgmt needed to Set this attribute
		*/
		EAccessoryPower,
	
		
		/**
		A 2 decimal digit language index. 
		
		It is used as the two digit language number that is the suffix of
		language resource DLLs, e.g ELOCL.01.

		The locale with this language index is loaded the next time that
		the device boots.

        This is dynamic and writeable.

		@see TLanguage

		@capability WriteDeviceData needed to Set this attribute
		*/
		ELanguageIndex,
	
		
		/**
		A 2 decimal digit (decimal) language keyboard index.
		It is used as the two digit language number that is the suffix of
		language resource DLLs, e.g. EKDATA.01.
		
		@see TLanguage

		@capability WriteDeviceData needed to Set this attribute
		*/
		EKeyboardIndex,
	
		
		/**
		The maximum allowable size of RAM drive, in bytes.
		*/
		EMaxRAMDriveSize,
	
		
		/**
		Indicates the state of the keyboard.
		
		It has the values:
		0 = keyboard is disabled;
		1 = Keyboard is enabled.
        
        This is dynamic and writeable.

		@capability PowerMgmt needed to Set this attribute
		*/
		EKeyboardState,
	
		/**
  		Defines the system drive & custom resource drive. 
        Legacy attribute which is no longer supported.
  
  		@deprecated Attribute is no longer the primary mechanism to define the 
  		System Drive or the Custom Resource Drive.
        @see RFs::GetSystemDrive.
        @see BaflUtils::NearestLanguageFile
        @see HALData::ECustomResourceDrive
  		*/
		ESystemDrive,	
		
		/**
		Indicates the state of the pen or digitiser.

		It has the values:
		1 = pen/digitiser is enabled;
		0 = pen/digitiser is disabled.
		
        This is dynamic and writeable.

		@capability PowerMgmt needed to Set this attribute
		*/
		EPenState,
	
		
		/**
		On input: aInOut contains the mode number.
        On output: aInOut contains: 0 = display is colour;
                                    1 = display is black & white.
        
        aInOut is the 3rd parameter passed to accessor functions
        for derived attributes.
		*/
		EDisplayIsMono,
	
		
		/**
		On input: aInOut contains the mode number;
        On output, aInOut contains: 0 = display is not palettised;
                                    1 = display is palettised.
        
        aInOut is the 3rd parameter passed to accessor functions
        for derived attributes.
  		*/
		EDisplayIsPalettized,
	
		
		/**
		The display bits per pixel.
		
        On input, aInOut contains the mode number.
        On output, aInOut contains the bits per pixel for that mode.

        aInOut is the 3rd parameter passed to accessor functions
        for derived attributes.
        
        It is read only data.
		*/
		EDisplayBitsPerPixel,
	
		
		/**
		The number of display modes available.
		*/
		EDisplayNumModes,
	
		
		/**
		The address of the display memory.
		*/
		EDisplayMemoryAddress,
	
		
		/**
		The offset, in bytes, to the pixel area of the screen from the start of screen memory.
		
		This is used to account for the fact that the palette is sometimes at
		the beginning of the display memory.
		
        On input, aInOut contains the mode number.
        On output, aInOut contains the offset to the first pixel for that mode.

        aInOut is the 3rd parameter passed to accessor functions
        for derived attributes.
		*/
		EDisplayOffsetToFirstPixel,
	
		
		/**
		The separation, in bytes, of successive lines of display in memory.
        
        On input, aInOut contains the mode number.
        On output, aInOut contains the display offset between lines.

        aInOut is the 3rd parameter passed to accessor functions
        for derived attributes.
		*/
		EDisplayOffsetBetweenLines,
	
		
		/**
		@capability MultimediaDD needed to Set this attribute
		*/
		EDisplayPaletteEntry,
	
		
		/**
		It has the values:
		1 = order of pixels in display is RGB;
		0 = otherwise.
		*/
		EDisplayIsPixelOrderRGB,
	
		
		/**
		It has the values:
		1 = pixel order is landscape;
		0 = pixel order is portrait.
		*/
		EDisplayIsPixelOrderLandscape,
	
		
		/**
		This indicates or sets the current display mode where
		EDisplayNumModes-1 is the maximum value for the display mode.
		The properties of a particular display mode are entirely defined by
		the base port software associated with the hardware upon which the OS
		is running.

		@capability MultimediaDD needed to Set this attribute
		*/
		EDisplayMode,
	
		
		/**
		If the target hardware upon which Symbian OS is running has switches
		which can be read by the base port software, this interface allows
		the current status of those switches to be read. 
		*/
		ESwitches,
	
		
		/**
		The port number of the debug port.
		*/
		EDebugPort,
	
		
		/**
		The language code of the Locale which was loaded at device boot time.

        This is dynamic and writeable.

		@see ELanguageIndex

		@capability WriteSystemData needed to Set this attribute
		*/
		ELocaleLoaded,
	
		
		/**
		The drive number to use for storage of Clipboard data.
		0 = Drive A, 1 = Drive B, etc...
		*/
		EClipboardDrive,
	
		/**
		Custom restart
		@capability PowerMgmt
		*/
		ECustomRestart,

		/**
		Custom restart reason
		*/
		ECustomRestartReason,

		/**
		The number of screens.
		*/
		EDisplayNumberOfScreens,

		/**
		The time between nanokernel ticks, in microseconds.
		*/
		ENanoTickPeriod,

		/**
		The frequency of the fast counter.
		*/
		EFastCounterFrequency,

		/**
		Indicates the whether the fast counter counts up or down.
		*/
		EFastCounterCountsUp,

		/**		
		Indicates whether a 3 dimensional pointing device is available for input and Z coordinate
		is provided in appropriate pointer-related TRawEvents generated by the driver.

		It has the values:
		0 = a 3D pointer is not available for input and Z coordinate is not provided in TRawEvents;
		1 = a 3D pointer is present and Z coordinate is provided in TRawEvents.
		*/
		EPointer3D,

		/**		
		The furthest detectable 3D pointing device's proximity above the screen.
		As proximity values above the screen are negative, this will be a negative value.

		This is dynamic and writeable.
		*/
		EPointer3DMaxProximity,

		/**		
		Indicates whether a 3 dimensional pointing device supports Theta polar angle reading.

		It has the values:
		0 = a 3D pointer does not support Theta polar angle reading;
		1 = a 3D pointer supports Theta polar angle reading.
		*/
		EPointer3DThetaSupported,

		/**
		Indicates whether a 3 dimensional pointing device supports Phi polar angle reading.

		It has the values:
		0 = a 3D pointer does not support Phi polar angle reading;
		1 = a 3D pointer supports Phi polar angle reading.
		*/
		EPointer3DPhiSupported,

		/**
		Indicates whether a 3 dimensional pointing device supports rotation angle along its main axis reading.

		It has the values:
		0 = a 3D pointer does not support alpha (rotation) reading;
		1 = a 3D pointer supports alpha (rotation) reading.
		*/
		EPointer3DRotationSupported,

		/**
		Indicates whether a 3 dimensional pointing device supports readings of pressure applied on screen.

		It has the values:
		0 = a 3D pointer does not support pressure reading;
		1 = a 3D pointer supports pressure reading.
		*/
		EPointer3DPressureSupported,

		/**
		Indicates whether hardware floating point is available, and what type.
		
		If no hardware floating point is available, reading this attribute will return KErrNotSupported.
		If hardware floating point is available, reading this attribute will return KErrNone and the type
		available. These types are specified in TFloatingPointType.
		*/
		EHardwareFloatingPoint,

		/**
		The offset between secure and nonsecure clocks. If this attribute is undefined no secure clock
		will be available.
		*/
		ETimeNonSecureOffset,

		/**
		Persist startup mode.

		If no variant specific implementation exists, the startup mode will be stored in platform
		specific values.hda file.
		*/
		EPersistStartupModeKernel,

		/**
		Maximum restart reasons.

		Returns the maximum number of values that can be used to store the restart reason required for a custom restart.
		*/
		EMaximumCustomRestartReasons,

		/**
		Maximum startup modes.
		
		Returns the maximum number of values that can be used to store the startup mode requires for a system restart.
		*/
		EMaximumRestartStartupModes,
		
		/**
		Defines the custom resource drive.
		
		This drive attribute should be set if an additional drive is required for use in the search 
		algorithm for language files.  
        
        @see TDriveNumber
		@see BaflUtils::NearestLanguageFile for how this attribute is used
		@capability WriteDeviceData needed to Set this attribute
		*/
		ECustomResourceDrive,

		/**
		Step size of Z distance data.

		Returns the minimum size of the step between two resolvable z positions
		*/
		EPointer3DProximityStep,

		/**
		Maximum Number of Pointers supported by hardware/driver

		Returns the maximum number of pointers for a multi-touch configuration (or KErrNotSupported or 0 or 1 for single-touch legacy configuration)
		*/
		EPointerMaxPointers,

		/**
		Maximum Number of Pointers
		
		Sets and reads back the number of pointers as requested by the UI (<=EPointerMaxPointers)
		*/
		EPointerNumberOfPointers,

		/**
		Maximum Pressure Value

		Returns the maximum pressure value
		*/
		EPointer3DMaxPressure,

		/**
		Step size of pressure data.

		Returns the minimum size of the step between two resolvable pressure readings			
		*/
		EPointer3DPressureStep,

		/**
		The threshold on pointer's Z coordinate above which EEnterHighPressure pointer event is sent to WSERV's clients.
		This value is intended to be preconfigured in build time and modified by Window Server only

		@prototype 9.5
		*/
		EPointer3DEnterHighPressureThreshold,
	
		/**
		The threshold on pointer's Z coordinate below which EExitHighPressure pointer event is sent to WSERV's clients.
		This value is intended to be preconfigured in build time and modified by Window Server only

		@prototype 9.5
		*/
		EPointer3DExitHighPressureThreshold,

		/**
		The threshold on pointer's Z coordinate above which EEnterCloseProximity pointer event is sent to WSERV's clients.
		This value is intended to be preconfigured in build time and modified by Window Server only

		@prototype 9.5
		*/
		EPointer3DEnterCloseProximityThreshold,

		
		/**
		The threshold on pointer's Z coordinate below which EExitCloseProximity pointer event is sent to WSERV's clients.
		This value is intended to be preconfigured in build time and modified by Window Server only

		@prototype 9.5
		*/
		EPointer3DExitCloseProximityThreshold,
		
		/**
		A Handle to the display memory.

		@prototype 9.5
		*/
		EDisplayMemoryHandle,

		/**
		Serial number of this board
		*/
		ESerialNumber,
		
		
		/**
		Interrupt used by sampling profiler - applicable for SMP only. Each CPU_i is interrupted by interrupt number ECpuProfilingInterrupt + i
		*/
		ECpuProfilingDefaultInterruptBase,


		/**
		Number of processors present on the device. Returns 1 on unicore. This number is constant and does not take account of power management
		*/
		ENumCpus,


		/*
		 * NOTE:
		 * When updating this list, please also update hal/rom/hal.hby and hal/tsrc/t_newhal.cpp.
		 */

		/**
		
		The number of HAL attributes per screen.
		
		It is simply defined by its position in the enumeration.
		*/
		ENumHalAttributes		

		};



    /**
    Defines properties for the hardware attributes.
    
    @see HALData::TAttribute
    */
	enum TAttributeProperty
		{
		/**
		When set, means that an attribute is meaningful on this device.
				
		@see HAL::Get()
		@see HAL::Set()
		*/
		EValid=0x1,
		
		
		/**
		When set, means that an attribute is modifiable.
		A call to HAL::Set() for an attribute that does not have this property,
		returns KErrNotSupported.
		
		@see HAL::Get()
		@see HAL::Set()
		*/
		ESettable=0x2,
		};



    /**
    UIDs for a defined set of device manufacturers.
    
    Note that any manufacturer not represented in this list must obtain
    a value from the Symbian registry.
    
    @see HALData::TAttribute
    */
    enum TManufacturer // UID for manufacturer
		{
		EManufacturer_Ericsson=0x00000000,
		EManufacturer_Motorola=0x00000001,
		EManufacturer_Nokia=0x00000002,
		EManufacturer_Panasonic=0x00000003,
		EManufacturer_Psion=0x00000004,
		EManufacturer_Intel=0x00000005,
		EManufacturer_Cogent=0x00000006,
		EManufacturer_Cirrus=0x00000007,
		EManufacturer_Linkup=0x00000008,
		EManufacturer_TexasInstruments=0x00000009,
		// New manufacturers must obtain an official UID to identify themselves
		};



    /**
	Defines the Symbian OS device families.
	
    @see HALData::TAttribute
    */
	enum TDeviceFamily
		{
		EDeviceFamily_Crystal,
		EDeviceFamily_Pearl,
		EDeviceFamily_Quartz,
		};



    /**
    Defines the set of CPU architectures.
    
    @see HALData::TAttribute
    */
	enum TCPU
		{
		ECPU_ARM,
		ECPU_MCORE,
		ECPU_X86,
		};



    /**
    Defines the set of ABIs used by the CPU for user applications.
    
    @see HALData::TAttribute    
    */
	enum TCPUABI
		{
		ECPUABI_ARM4,
		ECPUABI_ARMI,
		ECPUABI_THUMB,
		ECPUABI_MCORE,
		ECPUABI_MSVC,
		ECPUABI_ARM5T,
		ECPUABI_X86,
		};



    /**
    Defines the set of reasons for a system boot.
    
    @see HALData::TAttribute
    */
	enum TSystemStartupReason
		{
		ESystemStartupReason_Cold,
		ESystemStartupReason_Warm,
		ESystemStartupReason_Fault,
		};



    /**
    Defines the set of available keyboard types.

    @see HALData::TAttribute
    */
	bitmask TKeyboard
		{
		EKeyboard_Keypad=0x1,
		EKeyboard_Full=0x2,
		};



    /**
    Defines the mouse cursor visibility.
    
    @see HALData::TAttribute
    */
	enum TMouseState
		{
		EMouseState_Invisible=0,
		EMouseState_Visible=1,
		};



    /**
    Defines a set of UIDs for specific devices, reference boards etc
    
    @see HALData::TAttribute
    */
	enum TMachineUid
		{
		EMachineUid_Series5mx=0x1000118a,
		EMachineUid_Brutus=0x10005f60,
		EMachineUid_Cogent=0x10005f61,
		EMachineUid_Win32Emulator=0x10005f62,
		EMachineUid_WinC=0x10005f63,
		EMachineUid_CL7211_Eval=0x1000604f,
		EMachineUid_LinkUp=0x00000000,
		EMachineUid_Assabet=0x100093f3,
		EMachineUid_Zylonite=0x101f7f27,
		EMachineUid_IQ80310=0x1000a681,
		EMachineUid_Lubbock=0x101f7f26,
		EMachineUid_Integrator=0x1000AAEA,
		EMachineUid_Helen=0x101F3EE3,
		EMachineUid_X86PC=0x100000ad,
		EMachineUid_OmapH2=0x1020601C,
		EMachineUid_OmapH4=0x102734E3,
		EMachineUid_NE1_TB=0x102864F7,
		EMachineUid_EmuBoard=0x1200afed,
		EMachineUid_OmapH6=0x10286564,
		EMachineUid_OmapZoom=0x10286565,
		EMachineUid_STE8500=0x101FF810,
		};



    /**
    Defines power levels for the system (or 'Main') battery.
    
    @see HALData::TAttribute
    */
	enum TPowerBatteryStatus
		{
		EPowerBatteryStatus_Zero,
		EPowerBatteryStatus_Replace,
		EPowerBatteryStatus_Low,
		EPowerBatteryStatus_Good,
		};



    /**
    Defines power levels for the backup power.

    @see HALData::TAttribute
    */
	enum TPowerBackupStatus
		{
		EPowerBackupStatus_Zero,
		EPowerBackupStatus_Replace,
		EPowerBackupStatus_Low,
		EPowerBackupStatus_Good,
		};
	
	};

#endif
