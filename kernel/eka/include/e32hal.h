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
// e32\include\e32hal.h
// 
//

#ifndef __E32HAL_H__
#define __E32HAL_H__
#include <e32cmn.h>
#include <e32cmn_private.h>

/**
@publishedPartner
@deprecated No replacement.
*/ 
const TInt KMaxRomDevices=8;




/**
@publishedPartner
@released

Defines the calibration settings that can be restored by 
the digitiser's implementation of DDigitiser::RestoreXYInputCalibration.

See the digitiser template port in:
@code
...\template\template_variant\specific\xyin.cpp
@endcode

@see DDigitiser::RestoreXYInputCalibration()
*/ 
enum TDigitizerCalibrationType
    {
    EFactory, /**< Restore to factory settings. */
    ESaved    /**< Restore to saved settings.   */
    };




/**
@publishedPartner
@released

Defines the screen coordinates of the point touched during calibration.

An object of this type is passed, via a TPckgBuf, to the HAL handler
that deals with the HAL group function-id pair:
- EHalGroupDigitiser, EDigitiserHalSetXYInputCalibration
- EHalGroupDigitiser, EDigitiserHalCalibrationPoints
- EHalGroupDigitiser, EDigitiserHalRestoreXYInputCalibration

@see EDigitiserHalSetXYInputCalibration
@see EDigitiserHalCalibrationPoints
@see EDigitiserHalRestoreXYInputCalibration
@see TDigitiserHalFunction
@see EHalGroupDigitiser
@see TPckgBuf
*/ 
class TDigitizerCalibration
    {
public:
    TPoint iTl;  /**< Top left point.*/
    TPoint iBl;  /**< Bottom left point. */
    TPoint iTr;  /**< Top right point.*/
    TPoint iBr;  /**< Bottom right point.*/
    };




/**
@publishedPartner
@released

An enum that identifies the full set of keyboard types.

A variable of this type is defined and used in the TKeyboardInfoV01 struct. 

@see TKeyboardInfoV01::iKeyboardType.
*/ 
enum TKeyboard
	{
	EKeyboard_Keypad=1, /**< Keypad type*/
	EKeyboard_Full=2,   /**< Full keyboard type*/
	};




/**
@publishedPartner
@released

Encapsulates information about the keyboard.

NOTE that the information represented here is not used by Symbian OS, 
and exists to maintain binary compatibilty with older versions.

However, keyboard drivers that implement the HAL handler for
the EHalGroupKeyboard group still need to return a default object
of this type.
See the template port.

An object of this type is passed, via a TPckgBuf, to the HAL handler
that deals with the HAL group function-id pair:
- EHalGroupKeyboard, EKeyboardHalKeyboardInfo;

@see EKeyboardHalKeyboardInfo
@see TKeyboardHalFunction
@see EHalGroupKeyboard
@see TPckgBuf
*/ 
class TKeyboardInfoV01
	{
public:
    /**
    The number of device keys.
    
    NOTE that the information represented by this variable is not
    used by Symbian OS, and exists to maintain binary compatibility
    with older versions. 
    */
	TInt iDeviceKeys;
	
	
	/**
	The number of application keys.
	
	NOTE that the information represented by this variable is not
    used by Symbian OS, and exists to maintain binary compatibility
    with older versions. 
	*/
    TInt iAppsKeys;
    
    
    /**
    Defines the type of keyboard available as enumerated by
    the TKeyboard enum.
        
    NOTE that the information represented by this variable is not
    used by Symbian OS, and exists to maintain binary compatibility
    with older versions. 
            
    @see TKeyboard
    */
	TKeyboard iKeyboardType;
    };
    
    
    
    
/**
@publishedPartner
@released

Package buffer for a TKeyboardInfoV01 object.

@see TKeyboardInfoV01
*/ 
typedef TPckgBuf<TKeyboardInfoV01> TKeyboardInfoV01Buf;




/**
@publishedPartner
@released

Encapsulates information about the digitiser.

An object of this type is passed, via a TPckgBuf, to the HAL handler
that deals with the HAL group function-id pair:
- EHalGroupDigitiser, EDigitiserHalXYInfo;

@see EDigitiserHalXYInfo
@see TDigitiserHalFunction
@see EHalGroupDigitiser
@see TPckgBuf
*/ 
class TDigitiserInfoV01
	{
public:
	TPoint iOffsetToDisplay;/**< The offset in pixels from the digitiser usable area to the display area. */
	TSize iDigitiserSize;   /**< The width/height of the display in pixels as used by digitiser.*/
	};
	
	
/**
@publishedPartner
@released
Encapsulates extra information required by 3 dimensional pointing devices.
*/ 
class TDigitiserInfoV02 : public TDigitiserInfoV01
	{	
public :
	TInt iZRange;			/**< The maximum distance to screen a pointing device will be detected (settable).*/
	TUint8 iThetaSupported;	/**< A Boolean value that indicates if Theta polar angle detection (tilt) is supported.*/
	TUint8 iPhiSupported;	/**< A Boolean value that indicates if Phi polar angle detection (tilt) is supported.*/
	TUint8 iAlphaSupported;	/**< A Boolean value that indicates if rotation of the pointing device along its main axis is supported.*/
	TUint8 iPressureSupported; /**< A Boolean value that indicates if pressure applied on screen is supported.*/
	TInt iProximityStep;    /**<proximity resolution, e.g. proximity readings change in steps of 1, 5, 10, ... */
	TInt iMaxPressure;      /**< maximum pressure reading*/
	TInt iPressureStep;     /**< pressure resolution, */	
	TUint8 iMaxPointers;    /**< max number of multipletouch pointers supported by hardware/driver.*/ 
	TUint8 iNumberOfPointers;/**< Number of pointers it supports (settable).*/
	};


/**
@publishedPartner
@released

Package buffer for a TDigitiserInfoV01 object.

@see TDigitiserInfoV01
*/ 
typedef TPckgBuf<TDigitiserInfoV01> TDigitiserInfoV01Buf;

/**
@publishedPartner
@prototype

Package buffer for a TDigitiserInfoV02 object.

@see TDigitiserInfoV02
*/ 
typedef TPckgBuf<TDigitiserInfoV02> TDigitiserInfoV02Buf;

/**
@publishedPartner
@released

Encapsulates information about the mouse display characteristics.

An object of this type is passed, via a TPckgBuf, to the HAL handler
that deals with the HAL group function-id pair:
- EHalGroupMouse, EMouseHalMouseInfo;

@see EMouseHalMouseInfo
@see TMouseHalFunction
@see EHalGroupMouse
@see TPckgBuf
*/ 
class TMouseInfoV01
	{
public:
	TInt iMouseButtons;      /**< The number of mouse buttons.*/
	TPoint iOffsetToDisplay; /**< The offset in pixels from the mouse usable area to the display area.*/
	TSize iMouseAreaSize;    /**< The width/height of the display in pixels as used by the mouse.     */
	};
	
	
	
	
/**
@publishedPartner
@released

Package buffer for a TMouseInfoV01 object.

@see TMouseInfoV01
*/ 
typedef TPckgBuf<TMouseInfoV01> TMouseInfoV01Buf;



/**
@publishedPartner
@released

Encapsulates Variant specific information.

An object of this type is passed, via a TPckgBuf, to the HAL handler
that deals with the HAL group function-id pair:
- EHalGroupVariant, EVariantHalVariantInfo;

@see EVariantHalVariantInfo
@see TVariantHalFunction
@see EHalGroupVariant
@see TPckgBuf
*/ 
class TVariantInfoV01
	{
public:
    /**
    The ROM version.
    */
	TVersion iRomVersion;
	
	/**
	The Id that uniquely identifies the device.
	*/
	SInt64 iMachineUniqueId;
	
	/**
	The bits that represent the LED capabilities.
	
	NB capabilities in this context does not mean security capabilities.
	*/
    TUint iLedCapabilities;
    
    /**
    The processor clock speed.
    */
    TInt iProcessorClockInKHz;
    
    /**
    The speed factor. 
    */
    TInt iSpeedFactor;
    };
    
    
    
    
/**
@publishedPartner
@released

Package buffer for a TVariantInfoV01 object.

@see TVariantInfoV01
*/ 
typedef TPckgBuf<TVariantInfoV01> TVariantInfoV01Buf;




/**
@publishedPartner
@deprecated Use HAL::Get() from the HAL library instead with attributes ESystemStartupReason.

The reason that the device last reset

Note that not all reasons can occur on all devices.
*/
enum TMachineStartupType
	{
	EStartupCold,EStartupColdReset,EStartupNewOs,
	EStartupPowerFail,EStartupWarmReset,EStartupKernelFault,
	EStartupSafeReset
	};




/**
@publishedPartner
@deprecated Use HAL::Get() from the HAL library instead with attributes  EPen or EMouse.

The XY input method supported
*/
enum TXYInputType
	{
	EXYInputNone,
	EXYInputPointer,
	EXYInputMouse,
	EXYInputDeltaMouse
	};



/**
@publishedPartner
@deprecated Use HAL::Get() from the HAL library instead.

Miscellaneous machine info.
*/
class TMachineInfoV1
	{
public:
	TVersion iRomVersion;
	TXYInputType iXYInputType;
	TBool iKeyboardPresent;
	TBool iBacklightPresent;
    TSize iDisplaySizeInPixels;
    TSize iXYInputSizeInPixels;
    TSize iPhysicalScreenSize;
	TPoint iOffsetToDisplayInPixels;
	TInt iKeyboardId;
	TInt iDisplayId;
	SInt64 iMachineUniqueId;
    TUint iLedCapabilities;
    TInt iProcessorClockInKHz;
    TInt iSpeedFactor;
    TInt iMaximumDisplayColors;
	};
/**
@publishedPartner
@deprecated
*/
typedef TPckgBuf<TMachineInfoV1> TMachineInfoV1Buf;




/**
@publishedPartner
@deprecated Use HAL::Get() from the HAL library instead with attributes ELanguageIndex or EKeyboardIndex.

Miscellaneous locale info.
*/
class TMachineInfoV2 : public TMachineInfoV1
	{
public:
	TInt iLanguageIndex;
    TInt iKeyboardIndex;
    };
/**
@publishedPartner
@deprecated
*/
typedef TPckgBuf<TMachineInfoV2> TMachineInfoV2Buf;




/**
@publishedPartner
@deprecated Use HAL::Get() from the HAL library instead with attributes EMemoryRAM, EMemoryRAMFree or EMemoryROM.

Miscellaneous memory info.
*/ 
class TMemoryInfoV1
    {
public:
    TInt iTotalRamInBytes;
    TInt iTotalRomInBytes;
    TInt iMaxFreeRamInBytes;
    TInt iFreeRamInBytes;
    TInt iInternalDiskRamInBytes;
    TBool iRomIsReprogrammable;
    };
/**
@publishedPartner
@deprecated
*/
typedef TPckgBuf<TMemoryInfoV1> TMemoryInfoV1Buf;




/**
@publishedPartner
@deprecated No replacement.

Miscellaneous ROM info.
*/ 
class TRomInfoEntryV1
	{
public:
    enum TRomTypeV1
    	{
    	ERomTypeRom=0,
    	ERomTypeFlash=1
    	};
	TInt iSize;		    // size of ROM in bytes, 0=no ROM present
	TInt iWidth;		// bus width in bits
	TInt iSpeed;		// number of wait states
	TRomTypeV1 iType;	// 0=ROM, 1=FLASH
	};




/**
@publishedPartner
@deprecated No replacement.

Miscellaneous ROM info.
*/ 
class TRomInfoV1
	{
public:
	TRomInfoEntryV1	iEntry[KMaxRomDevices];
	};
/**
@publishedPartner
@deprecated
*/
typedef TPckgBuf<TRomInfoV1> TRomInfoV1Buf;

/**
@publishedPartner
@released
*/
const TUint KRuggedFileSystem=0x01;

/**
@publishedPartner
@released
*/
class TDriveInfoV1
    {
public:
	TInt iTotalSupportedDrives;
	TInfoName iDriveName[KMaxLocalDrives];
	TInt iTotalSockets;
	TInfoName iSocketName[KMaxPBusSockets];
	TInt iRuggedFileSystem;
	TUint iRegisteredDriveBitmask;
	};
/**
@publishedPartner
@released
*/
typedef TPckgBuf<TDriveInfoV1> TDriveInfoV1Buf;

#if defined(_UNICODE) && !defined(__KERNEL_MODE__)
/**
@publishedPartner
@released
*/
class TDriveInfoV18
    {
public:
	TInt iTotalSupportedDrives;
	TBuf8<KMaxInfoName> iDriveName[KMaxLocalDrives]; //TInfoName
	TInt iTotalSockets;
	TBuf8<KMaxInfoName> iSocketName[KMaxPBusSockets]; //TInfoName
	TInt iRuggedFileSystem;
	TUint iRegisteredDriveBitmask;
	};
/**
@publishedPartner
@released
*/
typedef TPckgBuf<TDriveInfoV18> TDriveInfoV1Buf8;
#else
typedef TDriveInfoV1 TDriveInfoV18;
typedef TDriveInfoV1Buf TDriveInfoV1Buf8;
#endif

/**
@publishedPartner
@released
*/
class TExcInfo
	{
public:
	TAny *iCodeAddress;
	TAny *iDataAddress;
	TInt iExtraData;
	};

#ifndef __KERNEL_MODE__
#include <e32std.h>




/**
@publishedPartner
@released

A set of user side utility functions for acessing hardware related
information.

Four of these functions are DEPRECATED and should NOT be used; some of
these functions have replacements, but others do not; see the description of
the individual functions.

The following functions are not deprecated, but are only used by Symbian
OS as part of its internal implementation:
-  UserHal::FaultReason()
-  UserHal::ExceptionId()
-  UserHal::ExceptionInfo()
-  UserHal::PageSizeInBytes()
-  UserHal::TickPeriod()
-  UserHal::DriveInfo()
-  UserHal::SwitchOff()
-  UserHal::SetXYInputCalibration()
-  UserHal::CalibrationPoints()
-  UserHal::SaveXYInputCalibration()
-  UserHal::RestoreXYInputCalibration()
*/
class UserHal
	{
public:
	// kernel group
	IMPORT_C static TInt MemoryInfo(TDes8& anInfo);
	IMPORT_C static TInt RomInfo(TDes8& anInfo);
	IMPORT_C static TInt StartupReason(TMachineStartupType& aReason);
	IMPORT_C static TInt FaultReason(TInt &aReason);
	IMPORT_C static TInt ExceptionId(TInt &anId);
	IMPORT_C static TInt ExceptionInfo(TExcInfo &aInfo);
	IMPORT_C static TInt PageSizeInBytes(TInt& aSize);

	// variant group
	IMPORT_C static TInt MachineInfo(TDes8& anInfo);
	IMPORT_C static TInt TickPeriod(TTimeIntervalMicroSeconds32& aPeriod);

	// media group
	IMPORT_C static TInt DriveInfo(TDes8& anInfo);

	// power group
   	IMPORT_C static TInt SwitchOff();

	// digitiser group
	IMPORT_C static TInt SetXYInputCalibration(const TDigitizerCalibration& aCalibration);
	IMPORT_C static TInt CalibrationPoints(TDigitizerCalibration& aCalibration);
	IMPORT_C static TInt SaveXYInputCalibration();
	IMPORT_C static TInt RestoreXYInputCalibration(TDigitizerCalibrationType aType);
	};
#endif
#endif

