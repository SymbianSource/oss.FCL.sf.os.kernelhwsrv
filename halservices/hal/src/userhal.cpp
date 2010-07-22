// Copyright (c) 1999-2010 Nokia Corporation and/or its subsidiary(-ies).
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
// hal\src\userhal.cpp
// 
//

#include <kernel/hal_int.h>
#include "u32std.h"
#include <videodriver.h>



_LIT(KLitHalUserHal,"HAL-UserHal");

enum THalUserHalPanic
	{
	EInvalidStartupType=0,
	EInvalidAttributeDP=1,
	EInvalidAttributeDNM=2,
	EInvalidAttributeDM=3,
	EInvalidAttributeDS=4,
	EInvalidAttributeCMI=5,
	EInvalidAttributeSMI=6,
	EInvalidAttributeDC=7,
	EInvalidAttributeMB=8,
	EInvalidAttributeMC=9,
	EInvalidAttributeSMI2=10,
	EInvalidAttributeSD=11,
	EInvalidAttributeSDMA=12,
	EInvalidAttribKeybd=13,
	EInvalidAttribPen=14,
	EInvalidAttribMouse=15,
	EInvalidAttrib3DPointer=16,
	EInvalidAttribDigitiserOrientation=17
	};

void Panic(THalUserHalPanic aPanic)
	{
	User::Panic(KLitHalUserHal,aPanic);		
	}

// ECPUSpeed
TInt GetCPUSpeed(TInt /*aDeviceNumber*/, TInt /*aAttrib*/, TBool /*aSet*/, TAny* aInOut)
	{

      TVariantInfoV01 info;
	TPckg<TVariantInfoV01> infoPckg(info);
	TInt r=UserSvr::HalFunction(EHalGroupVariant, EVariantHalVariantInfo, (TAny*)&infoPckg, NULL);
	if (r==KErrNone)
		{
		*(TInt*)aInOut=info.iProcessorClockInKHz;
		}
	return r;
	}
	
// ECpuProfilingDefaultInterruptBase
TInt GetCPUProfilerInterrupt(TInt /*aDeviceNumber*/, TInt /*aAttrib*/, TBool /*aSet*/, TAny* aInOut)
	{

	return UserSvr::HalFunction(EHalGroupVariant, EVariantHalProfilingDefaultInterruptBase, aInOut, NULL);
	}
	

// ESystemStartupReason
TInt GetSystemStartupReason(TInt /*aDeviceNumber*/, TInt /*aAttrib*/, TBool /*aSet*/, TAny* aInOut)
	{
	TInt reason;
	TInt r=UserSvr::HalFunction(EHalGroupKernel, EKernelHalStartupReason, (TAny*)&reason, NULL);
	if (r==KErrNone)
		{
		switch (reason)
			{
			case EStartupCold:
			case EStartupColdReset:
			case EStartupNewOs:
				*(TInt*)aInOut=HAL::ESystemStartupReason_Cold;
				break;
			case EStartupPowerFail:
			case EStartupWarmReset:
			case EStartupSafeReset:
				*(TInt*)aInOut=HAL::ESystemStartupReason_Warm;
				break;
			case EStartupKernelFault:
				*(TInt*)aInOut=HAL::ESystemStartupReason_Fault;
				break;
			default:
				Panic(EInvalidStartupType);
				break;
			}
		}
	return r;
	}

// ESystemException
TInt GetSystemException(TInt /*aDeviceNumber*/, TInt /*aAttrib*/, TBool /*aSet*/, TAny* aInOut)
	{
	return UserSvr::HalFunction(EHalGroupKernel, EKernelHalExceptionId, aInOut, NULL);
	}

// EMemoryRAM
TInt GetRAMSize(TInt /*aDeviceNumber*/, TInt /*aAttrib*/, TBool /*aSet*/, TAny* aInOut)
	{
	TMemoryInfoV1 info;
	TPckg<TMemoryInfoV1> infoPckg(info);
	TInt r=UserSvr::HalFunction(EHalGroupKernel, EKernelHalMemoryInfo, (TAny*)&infoPckg, NULL);
	if (r==KErrNone)
		{
		*(TInt*)aInOut=info.iTotalRamInBytes;
		}
	return r;
	}

// EMemoryRAMFree
TInt GetFreeRAM(TInt /*aDeviceNumber*/, TInt /*aAttrib*/, TBool /*aSet*/, TAny* aInOut)
	{
	TMemoryInfoV1 info;
	TPckg<TMemoryInfoV1> infoPckg(info);
	TInt r=UserSvr::HalFunction(EHalGroupKernel, EKernelHalMemoryInfo, (TAny*)&infoPckg, NULL);
	if (r==KErrNone)
		{
		*(TInt*)aInOut=info.iFreeRamInBytes;
		}
	return r;
	}

// EMemoryROM
TInt GetROMSize(TInt /*aDeviceNumber*/, TInt /*aAttrib*/, TBool /*aSet*/, TAny* aInOut)
	{
	TMemoryInfoV1 info;
	TPckg<TMemoryInfoV1> infoPckg(info);
	TInt r=UserSvr::HalFunction(EHalGroupKernel, EKernelHalMemoryInfo, (TAny*)&infoPckg, NULL);
	if (r==KErrNone)
		{
		*(TInt*)aInOut=info.iTotalRomInBytes;
		}
	return r;
	}

// EPowerGood
TInt GetPowerGoodState(TInt /*aDeviceNumber*/, TInt /*aAttrib*/, TBool /*aSet*/, TAny* aInOut)
	{
	TSupplyInfoV1 info;
	TPckg<TSupplyInfoV1> infoPckg(info);
	TInt r=UserSvr::HalFunction(EHalGroupPower, EPowerHalSupplyInfo, (TAny*)&infoPckg, NULL);
	if (r==KErrNone)
		{
		TBool state=info.iExternalPowerPresent||(info.iMainBatteryStatus>=ELow);
		*(TBool*)aInOut=state;
		}
	return r;
	}

// EPowerBatteryStatus
TInt GetBatteryStatus(TInt /*aDeviceNumber*/, TInt /*aAttrib*/, TBool /*aSet*/, TAny* aInOut)
	{
	TSupplyInfoV1 info;
	TPckg<TSupplyInfoV1> infoPckg(info);
	TInt r=UserSvr::HalFunction(EHalGroupPower, EPowerHalSupplyInfo, (TAny*)&infoPckg, NULL);
	if (r==KErrNone)
		{
		*(TInt*)aInOut=(TInt)info.iMainBatteryStatus;
		}
	return r;
	}

// EAccessoryPower
TInt GetAccessoryPowerPresent(TInt /*aDeviceNumber*/, TInt /*aAttrib*/, TBool /*aSet*/, TAny* aInOut)
	{
	return UserSvr::HalFunction(EHalGroupPower, EPowerHalAcessoryPowerPresent, aInOut, NULL);
	}

// EPowerBackup
TInt GetBackupPresent(TInt /*aDeviceNumber*/, TInt /*aAttrib*/, TBool /*aSet*/, TAny* aInOut)
	{
	return UserSvr::HalFunction(EHalGroupPower, EPowerHalBackupPresent, aInOut, NULL);
	}

// EPowerBackupStatus
TInt GetBackupStatus(TInt /*aDeviceNumber*/, TInt /*aAttrib*/, TBool /*aSet*/, TAny* aInOut)
	{
	TSupplyInfoV1 info;
	TPckg<TSupplyInfoV1> infoPckg(info);
	TInt r=UserSvr::HalFunction(EHalGroupPower, EPowerHalSupplyInfo, (TAny*)&infoPckg, NULL);
	if (r==KErrNone)
		{
		*(TInt*)aInOut=(TInt)info.iBackupBatteryStatus;
		}
	return r;
	}

// EPowerExternal
TInt GetPowerExternalState(TInt /*aDeviceNumber*/, TInt /*aAttrib*/, TBool /*aSet*/, TAny* aInOut)
	{
	TSupplyInfoV1 info;
	TPckg<TSupplyInfoV1> infoPckg(info);
	TInt r=UserSvr::HalFunction(EHalGroupPower, EPowerHalSupplyInfo, (TAny*)&infoPckg, NULL);
	if (r==KErrNone)
		{
		*(TInt*)aInOut=(TInt)info.iExternalPowerPresent;
		}
	return r;
	}

// EKeyboardState
TInt ProcessKeyboardState(TInt /*aDeviceNumber*/, TInt /*aAttrib*/, TBool aSet, TAny* aInOut)
	{
	if (aSet)
		return UserSvr::HalFunction(EHalGroupKeyboard, EKeyboardHalSetKeyboardState, aInOut, NULL);
	return UserSvr::HalFunction(EHalGroupKeyboard, EKeyboardHalKeyboardState, aInOut, NULL);
	}

// EKeyboard, EKeyboardDeviceKeys and EKeyboardAppKeys
TInt ProcessKeyboardInfo(TInt /*aDeviceNumber*/, TInt aAttrib, TBool /*aSet*/, TAny* aInOut)
	{
	TKeyboardInfoV01 info;	
	TPckg<TKeyboardInfoV01> infoPckg(info);
	TInt r=UserSvr::HalFunction(EHalGroupKeyboard, EKeyboardHalKeyboardInfo, (TAny*)&infoPckg, NULL);
	if (KErrNone == r)
		{
		switch (aAttrib)
			{
			case HAL::EKeyboard:
				*(TInt*)aInOut=(TInt)info.iKeyboardType;
				break;
			case HAL::EKeyboardDeviceKeys:
				*(TInt*)aInOut = info.iDeviceKeys;
				break;
			case HAL::EKeyboardAppKeys:
				*(TInt*)aInOut = info.iAppsKeys;
				break;
			default:
				__ASSERT_DEBUG(EFalse, Panic(EInvalidAttribKeybd));
				break;
			}
		}
	return r;
	}

// EKeyboardClick
TInt GetKeyboardClickPresent(TInt /*aDeviceNumber*/, TInt /*aAttrib*/, TBool /*aSet*/, TAny* aInOut)
	{
	TInt state=0;
	TInt r=UserSvr::HalFunction(EHalGroupSound, ESoundHalKeyClickEnabled, (TAny*)state, NULL);
	*(TInt*)aInOut=(r==KErrNone)?1:0;  // if there is a clicking device we should be able to get its state
	return KErrNone;
	}

// EKeyboardClickVolumeMax
TInt GetKeyboardClickVolumeMax(TInt /*aDeviceNumber*/, TInt /*aAttrib*/, TBool /*aSet*/, TAny* aInOut)
	{
	return UserSvr::HalFunction(EHalGroupSound, ESoundHalKeyClickVolumeMax, aInOut, NULL);
	}

// EKeyboardClickState
TInt ProcessKeyboardClickState(TInt /*aDeviceNumber*/, TInt /*aAttrib*/, TBool aSet, TAny* aInOut)
	{
	if (aSet)
		return UserSvr::HalFunction(EHalGroupSound, ESoundHalSetKeyClickEnabled, aInOut, NULL);
	return UserSvr::HalFunction(EHalGroupSound, ESoundHalKeyClickEnabled, aInOut, NULL);
	}

// EKeyboardClickVolume
TInt ProcessKeyboardClickVolume(TInt /*aDeviceNumber*/, TInt /*aAttrib*/, TBool aSet, TAny* aInOut)
	{
	TInt r;
	if (aSet)
		{
		TInt vol=(TInt)aInOut;
		TInt volMax=0;
		r=UserSvr::HalFunction(EHalGroupSound, ESoundHalKeyClickVolumeMax, (TAny*)volMax, NULL);
		if (r!=KErrNone)
			return r;
		if (vol<0 || vol>volMax)
			return KErrArgument;
		return UserSvr::HalFunction(EHalGroupSound, ESoundHalSetKeyClickLoud, (TAny*)vol, NULL);
		}
	TBool state;
	r=UserSvr::HalFunction(EHalGroupSound, ESoundHalKeyClickLoud, (TAny*)&state, NULL);
	if (r==KErrNone)
		{
		*(TInt*)aInOut=state?1:0;
		}
	return r;
	}

// EDisplayContrast
TInt ProcessDisplayContrast(TInt aDeviceNumber, TInt /*aAttrib*/, TBool aSet, TAny* aInOut)
	{
	if (aSet)
		{
		TInt x=(TInt)aInOut;
		return UserSvr::HalFunction(EHalGroupDisplay, EDisplayHalSetDisplayContrast, (TAny*)x, NULL, aDeviceNumber);
		}
	return UserSvr::HalFunction(EHalGroupDisplay, EDisplayHalDisplayContrast, aInOut, NULL, aDeviceNumber);
	}

// EDisplayBrightness
TInt ProcessDisplayBrightness(TInt aDeviceNumber, TInt /*aAttrib*/, TBool aSet, TAny* aInOut)
	{
	if (aSet)
		{
		TInt x=(TInt)aInOut;
		return UserSvr::HalFunction(EHalGroupDisplay, EDisplayHalSetDisplayBrightness, (TAny*)x, NULL, aDeviceNumber);
		}
	return UserSvr::HalFunction(EHalGroupDisplay, EDisplayHalDisplayBrightness, aInOut, NULL, aDeviceNumber);
	}


// EDisplayPaletteEntry
TInt ProcessDisplayPaletteEntry(TInt aDeviceNumber, TInt __DEBUG_ONLY(aAttrib), TBool aSet, TAny* aInOut)
	{

	__ASSERT_DEBUG(HAL::EDisplayPaletteEntry == aAttrib, Panic(EInvalidAttributeDP));

	if (aSet)
		{
		TInt entry =  ((TInt)aInOut >> 24) & 0xFF;
		TInt color = ((TInt)aInOut) & 0xFFFFFF;
		return UserSvr::HalFunction(EHalGroupDisplay, EDisplayHalSetPaletteEntry, (TAny*)entry, (TAny*)color, aDeviceNumber);
		}
	TInt e = (*(TInt*)aInOut) & 0xFF;
	return UserSvr::HalFunction(EHalGroupDisplay, EDisplayHalPaletteEntry, &e, aInOut, aDeviceNumber);
	}

// EDisplayNumModes
TInt ProcessDisplayNumModes(TInt aDeviceNumber, TInt __DEBUG_ONLY(aAttrib), TBool /*aSet*/, TAny* aInOut)
	{
	__ASSERT_DEBUG(HAL::EDisplayNumModes == aAttrib, Panic(EInvalidAttributeDNM));
	return UserSvr::HalFunction(EHalGroupDisplay, EDisplayHalModeCount, aInOut, NULL, aDeviceNumber);
	}

// EDisplayMode
TInt ProcessDisplayMode(TInt aDeviceNumber, TInt __DEBUG_ONLY(aAttrib), TBool aSet, TAny* aInOut)
	{
	__ASSERT_DEBUG(HAL::EDisplayMode == aAttrib, Panic(EInvalidAttributeDM));
	return UserSvr::HalFunction(EHalGroupDisplay, aSet?EDisplayHalSetMode:EDisplayHalMode, aInOut, NULL, aDeviceNumber);
	}

// EDisplayState
TInt ProcessDisplayState(TInt aDeviceNumber, TInt __DEBUG_ONLY(aAttrib), TBool aSet, TAny* aInOut)
	{
	__ASSERT_DEBUG(HAL::EDisplayState == aAttrib, Panic(EInvalidAttributeDS));

	if (aSet)
		{
		TBool on = (TBool)aInOut;
		return UserSvr::HalFunction(EHalGroupDisplay, EDisplayHalSetState, (TAny*)on, NULL, aDeviceNumber);
		}
	return UserSvr::HalFunction(EHalGroupDisplay, EDisplayHalState, aInOut, NULL, aDeviceNumber);
	
	}

// EDisplayColors
TInt ProcessDisplayColors(TInt aDeviceNumber, TInt __DEBUG_ONLY(aAttrib), TBool /*aSet*/, TAny* aInOut)
	{
	__ASSERT_DEBUG(HAL::EDisplayColors == aAttrib, Panic(EInvalidAttributeDC));
	return UserSvr::HalFunction(EHalGroupDisplay, EDisplayHalColors, aInOut, NULL, aDeviceNumber);
	}

// EDisplayBrightnessMax
TInt ProcessDisplayMaxBrightness(TInt aDeviceNumber, TInt __DEBUG_ONLY(aAttrib), TBool /*aSet*/, TAny* aInOut)
	{
	__ASSERT_DEBUG(HAL::EDisplayBrightnessMax == aAttrib, Panic(EInvalidAttributeMB));
	return UserSvr::HalFunction(EHalGroupDisplay, EDisplayHalMaxDisplayBrightness, aInOut, NULL, aDeviceNumber);
	}

// EDisplayContrastMax
TInt ProcessDisplayMaxContrast(TInt aDeviceNumber, TInt __DEBUG_ONLY(aAttrib), TBool /*aSet*/, TAny* aInOut)
	{
	__ASSERT_DEBUG(HAL::EDisplayContrastMax == aAttrib, Panic(EInvalidAttributeMC));
	return UserSvr::HalFunction(EHalGroupDisplay, EDisplayHalMaxDisplayContrast, aInOut, NULL, aDeviceNumber);
	}

/*
TInt ProcessSecureDisplayCurrentModeInfo(TInt aDeviceNumber, TInt __DEBUG_ONLY(aAttrib), TBool aSet, TAny* aInOut)
	{
	//only info obtainable about secure display is the address
	__ASSERT_DEBUG(HAL::ESecureDisplayMemoryAddress == aAttrib, Panic(EInvalidAttributeSDMA));

	TVideoInfoV01 info;
	TPckg<TVideoInfoV01> infoPckg(info);
	TInt r=UserSvr::HalFunction(EHalGroupDisplay, EDisplayHalCurrentModeInfo, (TAny*)&infoPckg, (TAny*)ETrue, aDeviceNumber);
	if (KErrNone == r)
		*(TInt*)aInOut = info.iVideoAddress;

	return r;
	}
*/

TInt ProcessDisplayCurrentModeInfo(TInt aDeviceNumber, TInt aAttrib, TBool /*aSet*/, TAny* aInOut)
	{
	TVideoInfoV01 info;
	TPckg<TVideoInfoV01> infoPckg(info);
	TInt r=UserSvr::HalFunction(EHalGroupDisplay, EDisplayHalCurrentModeInfo, (TAny*)&infoPckg, (TAny*)EFalse, aDeviceNumber);
	if (KErrNone == r)
		{
		switch (aAttrib)
			{
			case HAL::EDisplayXPixels:
				*(TInt*)aInOut = info.iSizeInPixels.iWidth;
				break;

			case HAL::EDisplayYPixels:
				*(TInt*)aInOut = info.iSizeInPixels.iHeight;
				break;
			
			case HAL::EDisplayXTwips:
				*(TInt*)aInOut = info.iSizeInTwips.iWidth;
				break;
			
			case HAL::EDisplayYTwips:
				*(TInt*)aInOut = info.iSizeInTwips.iHeight;
				break;
			
			case HAL::EDisplayMemoryAddress:
				if (info.iVideoAddress == 0)	//if this is true, the actual address is returned by EDisplayHalGetDisplayMemoryAddress
					{
					r = UserSvr::HalFunction(EHalGroupDisplay, EDisplayHalGetDisplayMemoryAddress, aInOut, (TAny*)EFalse, aDeviceNumber);
					}
				else
					{
					*(TInt*)aInOut = info.iVideoAddress;
					}
				break;

			case HAL::EDisplayIsPixelOrderRGB:
				*(TInt*)aInOut = info.iIsPixelOrderRGB;
				break;

			case HAL::EDisplayIsPixelOrderLandscape:
				*(TInt*)aInOut = info.iIsPixelOrderLandscape;
				break;

			default:
				__ASSERT_DEBUG(EFalse, Panic(EInvalidAttributeCMI));
				break;

			}
		}
	return r;
	}


TInt ProcessDisplaySpecifiedModeInfo(TInt aDeviceNumber, TInt aAttrib, TBool __DEBUG_ONLY(aSet), TAny* aInOut)
	{

	__ASSERT_DEBUG(!aSet, Panic(EInvalidAttributeSMI2));


	TVideoInfoV01 info;
	TPckg<TVideoInfoV01> infoPckg(info);
	TInt r=UserSvr::HalFunction(EHalGroupDisplay, EDisplayHalSpecifiedModeInfo, aInOut, (TAny*)&infoPckg, aDeviceNumber);
	if (KErrNone == r)
		{
		switch (aAttrib)
			{
			case HAL::EDisplayIsMono:
				*(TInt*)aInOut = info.iIsMono;
				break;
			
			case HAL::EDisplayIsPalettized:
				*(TInt*)aInOut = info.iIsPalettized;
				break;

			case HAL::EDisplayBitsPerPixel:
				*(TInt*)aInOut = info.iBitsPerPixel;
				break;

			case HAL::EDisplayOffsetToFirstPixel:
				*(TInt*)aInOut = info.iOffsetToFirstPixel;
				break;

			case HAL::EDisplayOffsetBetweenLines:
				*(TInt*)aInOut = info.iOffsetBetweenLines;
				break;

			default:
				__ASSERT_DEBUG(EFalse, Panic(EInvalidAttributeSMI));
				break;
			}
		}
	return r;
	}

//ESecureDisplay
/*
TInt ProcessSecureDisplay(TInt aDeviceNumber, TInt __DEBUG_ONLY(aAttrib), TBool aSet, TAny* aInOut)
	{
	__ASSERT_DEBUG(HAL::ESecureDisplay == aAttrib, Panic(EInvalidAttributeSD));

	if (aSet)
		return UserSvr::HalFunction(EHalGroupDisplay, EDisplayHalSetSecure, aInOut, NULL, aDeviceNumber);
	return UserSvr::HalFunction(EHalGroupDisplay, EDisplayHalSecure, aInOut, NULL, aDeviceNumber);
	}
*/

// EBacklight
TInt GetBacklightPresent(TInt aDeviceNumber, TInt /*aAttrib*/, TBool /*aSet*/, TAny* aInOut)
	{
	TInt state=0;
	TInt r=UserSvr::HalFunction(EHalGroupDisplay, EDisplayHalBacklightOn, (TAny*)&state, NULL, aDeviceNumber);
	*(TInt*)aInOut=(r==KErrNone)?1:0;  // if there is a backlight we should be able to get its state
	return KErrNone;
	}

// EBacklightState
TInt ProcessBacklightState(TInt aDeviceNumber, TInt /*aAttrib*/, TBool aSet, TAny* aInOut)
	{
	if (aSet)
		return UserSvr::HalFunction(EHalGroupDisplay, EDisplayHalSetBacklightOn, aInOut, NULL, aDeviceNumber);
	return UserSvr::HalFunction(EHalGroupDisplay, EDisplayHalBacklightOn, aInOut, NULL, aDeviceNumber);
	}

// EPen
TInt GetPenPresent(TInt /*aDeviceNumber*/, TInt /*aAttrib*/, TBool /*aSet*/, TAny* aInOut)
	{
	TDigitiserInfoV01Buf buf;
	TInt r=UserSvr::HalFunction(EHalGroupDigitiser, EDigitiserHalXYInfo, (TAny*)&buf, NULL);
	*(TInt*)aInOut=(r==KErrNone)?1:0;  // if there is a pen device we should be able to get info out of it
	return KErrNone;
	}

// EPenX and EPenY
TInt ProcessPenInfo(TInt /*aDeviceNumber*/, TInt aAttrib, TBool /*aSet*/, TAny* aInOut)
	{
	TDigitiserInfoV01 info;
	TPckg<TDigitiserInfoV01> infoPckg(info);
	TInt r=UserSvr::HalFunction(EHalGroupDigitiser, EDigitiserHalXYInfo, (TAny*)&infoPckg, NULL);
	if (KErrNone == r)
		{
		switch (aAttrib)
			{
			case HAL::EPenX:
				*(TInt*)aInOut=(TInt)info.iDigitiserSize.iWidth;
				break;
			case HAL::EPenY:
				*(TInt*)aInOut = info.iDigitiserSize.iHeight;
				break;
			default:
				__ASSERT_DEBUG(EFalse, Panic(EInvalidAttribPen));
				break;
			}
		}
	return r;
	}

// EPenState
TInt ProcessPenState(TInt /*aDeviceNumber*/, TInt /*aAttrib*/, TBool aSet, TAny* aInOut)
	{
	if (aSet)
		return UserSvr::HalFunction(EHalGroupDigitiser, EDigitiserHalSetXYState, aInOut, NULL);
	return UserSvr::HalFunction(EHalGroupDigitiser, EDigitiserHalXYState, aInOut, NULL);
	}

// EPenDisplayOn
TInt ProcessPenDisplayOnState(TInt /*aDeviceNumber*/, TInt /*aAttrib*/, TBool aSet, TAny* aInOut)
	{
	if (aSet)
		return UserSvr::HalFunction(EHalGroupPower, EPowerHalSetPointerSwitchesOn, aInOut, NULL);
	return UserSvr::HalFunction(EHalGroupPower, EPowerHalPointerSwitchesOn, aInOut, NULL);
	}

// EPenClick
TInt GetPenClickPresent(TInt /*aDeviceNumber*/, TInt /*aAttrib*/, TBool /*aSet*/, TAny* aInOut)
	{
	TInt state=0;
	TInt r=UserSvr::HalFunction(EHalGroupSound, ESoundHalPointerClickEnabled, (TAny*)state, NULL);
	*(TInt*)aInOut=(r==KErrNone)?1:0;  // if there is a clicking device we should be able to get its state
	return KErrNone;
	}

// EPenClickVolumeMax
TInt GetPenClickVolumeMax(TInt /*aDeviceNumber*/, TInt /*aAttrib*/, TBool /*aSet*/, TAny* aInOut)
	{
	return UserSvr::HalFunction(EHalGroupSound, ESoundHalPointerClickVolumeMax, aInOut, NULL);
	}

// EPenClickState
TInt ProcessPenClickState(TInt /*aDeviceNumber*/, TInt /*aAttrib*/, TBool aSet, TAny* aInOut)
	{
	if (aSet)
		return UserSvr::HalFunction(EHalGroupSound, ESoundHalSetPointerClickEnabled, aInOut, NULL);
	return UserSvr::HalFunction(EHalGroupSound, ESoundHalPointerClickEnabled, aInOut, NULL);
	}

// EPenClickVolume
TInt ProcessPenClickVolume(TInt /*aDeviceNumber*/, TInt /*aAttrib*/, TBool aSet, TAny* aInOut)
	{
	TInt r;
	if (aSet)
		{
		TInt vol=(TInt)aInOut;
		TInt volMax=0;
		r=UserSvr::HalFunction(EHalGroupSound, ESoundHalPointerClickVolumeMax, (TAny*)volMax, NULL);
		if (r!=KErrNone)
			return r;
		if (vol<0 || vol>volMax)	
			return KErrArgument;
		return UserSvr::HalFunction(EHalGroupSound, ESoundHalSetPointerClickLoud, (TAny*)vol, NULL);
		}
	TBool state;
	r=UserSvr::HalFunction(EHalGroupSound, ESoundHalPointerClickLoud, (TAny*)&state, NULL);
	if (r==KErrNone)
		{
		*(TInt*)aInOut=state?1:0;
		}
	return r;
	}

// ECaseSwitchDisplayOn
TInt ProcessCaseSwitchDisplayOnState(TInt /*aDeviceNumber*/, TInt /*aAttrib*/, TBool aSet, TAny* aInOut)
	{
	if (aSet)
		return UserSvr::HalFunction(EHalGroupPower, EPowerHalSetCaseOpenSwitchesOn, aInOut, NULL);
	return UserSvr::HalFunction(EHalGroupPower, EPowerHalCaseOpenSwitchesOn, aInOut, NULL);
	}

// ECaseSwitchDisplayOff
TInt ProcessCaseSwitchDisplayOffState(TInt /*aDeviceNumber*/, TInt /*aAttrib*/, TBool aSet, TAny* aInOut)
	{
	if (aSet)
		return UserSvr::HalFunction(EHalGroupPower, EPowerHalSetCaseCloseSwitchesOff, aInOut, NULL);
	return UserSvr::HalFunction(EHalGroupPower, EPowerHalCaseCloseSwitchesOff, aInOut, NULL);
	}

// ELEDs
TInt GetLedCaps(TInt /*aDeviceNumber*/, TInt /*aAttrib*/, TBool /*aSet*/, TAny* aInOut)
	{
	TVariantInfoV01 info;
	TPckg<TVariantInfoV01> infoPckg(info);
	TInt r=UserSvr::HalFunction(EHalGroupVariant, EVariantHalVariantInfo, (TAny*)&infoPckg, NULL);
	if (r==KErrNone)
		{
		*(TInt*)aInOut=(info.iLedCapabilities)>>16;		// upper half for number of Leds, lower half for colour capabilities
		}
	return r;
	}

// ELEDmask
TInt ProcessLEDMask(TInt /*aDeviceNumber*/, TInt /*aAttrib*/, TBool aSet, TAny* aInOut)
	{
	if (aSet)
		{
		return UserSvr::HalFunction(EHalGroupVariant, EVariantHalLedMaskSet, aInOut, NULL);
		}
	return UserSvr::HalFunction(EHalGroupVariant, EVariantHalLedMaskGet, aInOut, NULL);
	}

// ESwitches
TInt GetSwitches(TInt /*aDeviceNumber*/, TInt /*aAttrib*/, TBool /*aSet*/, TAny* aInOut)
	{
	return UserSvr::HalFunction(EHalGroupVariant, EVariantHalSwitches, aInOut, NULL);
	}

// EMouse
TInt GetMousePresent(TInt /*aDeviceNumber*/, TInt /*aAttrib*/, TBool /*aSet*/, TAny* aInOut)
	{
	TMouseInfoV01Buf buf;
	TInt r=UserSvr::HalFunction(EHalGroupMouse, EMouseHalMouseInfo, (TAny*)&buf, NULL);
	*(TInt*)aInOut=(r==KErrNone)?1:0;  // if there is a mouse device we should be able to get info out of it
	return KErrNone;
	}

// EMouseX, EMouseY and EMouseButtons
TInt ProcessMouseInfo(TInt /*aDeviceNumber*/, TInt aAttrib, TBool /*aSet*/, TAny* aInOut)
	{
	TMouseInfoV01 info;
	TPckg<TMouseInfoV01> infoPckg(info);
	TInt r=UserSvr::HalFunction(EHalGroupMouse, EMouseHalMouseInfo, (TAny*)&infoPckg, NULL);
	if (KErrNone == r)
		{
		switch (aAttrib)
			{
			case HAL::EMouseX:
				*(TInt*)aInOut=(TInt)info.iMouseAreaSize.iWidth;
				break;
			case HAL::EMouseY:
				*(TInt*)aInOut = info.iMouseAreaSize.iHeight;
				break;
			case HAL::EMouseButtons:
				*(TInt*)aInOut=(TInt)info.iMouseButtons;
				break;
			default:
				__ASSERT_DEBUG(EFalse, Panic(EInvalidAttribMouse));
				break;
			}
		}
	return r;
	}

// EMouseState
TInt ProcessMouseState(TInt /*aDeviceNumber*/, TInt /*aAttrib*/, TBool aSet, TAny* aInOut)
	{
	if (aSet)
		return UserSvr::HalFunction(EHalGroupMouse, EMouseHalSetMouseState, aInOut, NULL);
	return UserSvr::HalFunction(EHalGroupMouse, EMouseHalMouseState, aInOut, NULL);
	}

// EMouseSpeed
TInt ProcessMouseSpeed(TInt /*aDeviceNumber*/, TInt /*aAttrib*/, TBool aSet, TAny* aInOut)
	{
	if (aSet)
		{
		TInt speed=(TInt)aInOut;
		if (speed<0 || speed>255)
			return KErrArgument;
		return UserSvr::HalFunction(EHalGroupMouse, EMouseHalSetMouseSpeed, (TAny*)speed, NULL);
		}
	return UserSvr::HalFunction(EHalGroupMouse, EMouseHalMouseSpeed, aInOut, NULL);
	}

// EMouseAcceleration
TInt ProcessMouseAcceleration(TInt /*aDeviceNumber*/, TInt /*aAttrib*/, TBool aSet, TAny* aInOut)
	{
	if (aSet)
		{
		TInt acc=(TInt)aInOut;
		if (acc<0 || acc>255)
			return KErrArgument;
		return UserSvr::HalFunction(EHalGroupMouse, EMouseHalSetMouseAcceleration, (TAny*)acc, NULL);
		}
	return UserSvr::HalFunction(EHalGroupMouse, EMouseHalMouseAcceleration, aInOut, NULL);
	}

// EMouseButtonState
TInt GetMouseButtonState(TInt /*aDeviceNumber*/, TInt /*aAttrib*/, TBool /*aSet*/, TAny* /*aInOut*/)
	{
	return KErrNotSupported;
	}

// EDebugPort
TInt ProcessDebugPort(TInt /*aDeviceNumber*/, TInt /*aAttrib*/, TBool aSet, TAny* aInOut)
	{
	return UserSvr::HalFunction(EHalGroupVariant, aSet ? EVariantHalDebugPortSet : EVariantHalDebugPortGet, aInOut, NULL);
	}

// ECustomRestart
TInt ProcessCustomRestart(TInt /*aDeviceNumber*/, TInt /*aAttrib*/, TBool aSet, TAny* aInOut)
	{
	if (!aSet)
		return KErrNotSupported;
	return UserSvr::HalFunction(EHalGroupVariant, EVariantHalCustomRestart, aInOut, NULL);
	}

// ECustomRestartReason
TInt ProcessCustomRestartReason(TInt /*aDeviceNumber*/, TInt /*aAttrib*/, TBool /*aSet*/, TAny* aInOut)
	{
	return UserSvr::HalFunction(EHalGroupVariant, EVariantHalCustomRestartReason, aInOut, NULL);
	}

// EHardwareFloatingPoint
TInt GetHardwareFloatingPoint(TInt /*aDeviceNumber*/, TInt /*aAttrib*/, TBool /*aSet*/, TAny* aInOut)
	{
	return UserSvr::HalFunction(EHalGroupKernel, EKernelHalHardwareFloatingPoint, aInOut, NULL);
	}

// ETimeNonSecureOffset
TInt NonsecureClockOffset(TInt /*aDeviceNumber*/, TInt /*aAttrib*/, TBool aSet, TAny* aInOut)
	{
	TInt kernelHalFnId = aSet ? EKernelHalSetNonsecureClockOffset : EKernelHalGetNonsecureClockOffset;
	return UserSvr::HalFunction(EHalGroupKernel, kernelHalFnId, aInOut, NULL);
	}

// ECaseState
TInt GetCaseState(TInt /*aDeviceNumber*/, TInt /*aAttrib*/, TBool /*aSet*/, TAny* aInOut)
	{
	return UserSvr::HalFunction(EHalGroupVariant, EVariantHalCaseState, aInOut, NULL);
	}

// EPersistStartupModeKernel
TInt ProcessPersistStartupMode(TInt /*aDeviceNumber*/, TInt /*aAttrib*/, TBool aSet, TAny* aInOut)
	{
	return UserSvr::HalFunction(EHalGroupVariant, aSet ? EVariantHalPersistStartupMode : EVariantHalGetPersistedStartupMode, aInOut, NULL);
	}

// EAvailableCustomRestartReasons
TInt GetMaximumCustomRestartReasons(TInt /*aDeviceNumber*/, TInt /*aAttrib*/, TBool aSet, TAny* aInOut)
	{
	if (aSet)
		return KErrNotSupported;
	return UserSvr::HalFunction(EHalGroupVariant, EVariantHalGetMaximumCustomRestartReasons, aInOut, NULL);
	}

// EMaximumRestartStartupModes
TInt GetMaximumRestartStartupModes(TInt /*aDeviceNumber*/, TInt /*aAttrib*/, TBool aSet, TAny* aInOut)
	{
	if (aSet)
		return KErrNotSupported;
	return UserSvr::HalFunction(EHalGroupVariant, EVariantHalGetMaximumRestartStartupModes, aInOut, NULL);
	}


// EPointer3DThetaSupported, EPointer3PhiSupported, EPointer3DRotationSupported, EPointer3DPressureSupported,
//EPointer3DProximityStep,EPointerMaxPointers,EPointer3DMaxPressure,EPointer3DPressureStep

TInt ProcessAdvancedPointer(TInt aDeviceNumber, TInt aAttrib, TBool /*aSet*/, TAny* aInOut)
	{
	TDigitiserInfoV02 info;	
	TPckg<TDigitiserInfoV02> infoPckg(info);

	TInt r=UserSvr::HalFunction(EHalGroupDigitiser, EDigitiserHal3DInfo,(TAny*)&infoPckg, NULL,aDeviceNumber);
	if (KErrNone == r)
		{
		switch (aAttrib)
			{
			case HAL::EPointer3DThetaSupported:
				*(TInt*)aInOut=(TInt)info.iThetaSupported;
				break;

			case HAL::EPointer3DPhiSupported:
				*(TInt*)aInOut = info.iPhiSupported;
				break;

			case HAL::EPointer3DRotationSupported:
				*(TInt*)aInOut = info.iAlphaSupported;
				break;

			case HAL::EPointer3DPressureSupported:
				*(TInt*)aInOut = info.iPressureSupported;
				break;

			case HAL::EPointer3DProximityStep:
				*(TInt*)aInOut = info.iProximityStep;
				break;
				
			case HAL::EPointerMaxPointers:
				*(TInt*)aInOut = info.iMaxPointers;
				break;
				
			case HAL::EPointer3DMaxPressure:
				*(TInt*)aInOut = info.iMaxPressure;
				break;

			case HAL::EPointer3DPressureStep:
				*(TInt*)aInOut = info.iPressureStep;
				break;

			default:
				__ASSERT_DEBUG(EFalse, Panic(EInvalidAttrib3DPointer));
				break;
			}
		}
	return r;
	}

// EPointer3D
TInt Get3DPointerPresent(TInt aDeviceNumber, TInt /*aAttrib*/, TBool /*aSet*/, TAny* aInOut)
	{
	return UserSvr::HalFunction(EHalGroupDigitiser, EDigitiserHal3DPointer, aInOut, NULL, aDeviceNumber);
	}

//EPointer3DMaxProximity
TInt Process3DRange(TInt aDeviceNumber, TInt /*aAttrib*/, TBool aSet, TAny* aInOut)
	{
	if (aSet)
		return UserSvr::HalFunction(EHalGroupDigitiser, EDigitiserHalSetZRange, aInOut, NULL, aDeviceNumber);
	else
		{
		TDigitiserInfoV02 info;
		
		TPckg<TDigitiserInfoV02> infoPckg(info);
		TInt r =UserSvr::HalFunction(EHalGroupDigitiser, EDigitiserHal3DInfo,(TAny*)&infoPckg, NULL, aDeviceNumber);
		if(r==KErrNone)
			{
			*(TInt*)aInOut=info.iZRange;
			}
		return r;
		}
	}

// EPointerNumberOfPointers
TInt ProcessNumberOfPointers(TInt aDeviceNumber, TInt /*aAttrib*/, TBool aSet, TAny* aInOut)
	{
	if (aSet)
		return UserSvr::HalFunction(EHalGroupDigitiser, EDigitiserHalSetNumberOfPointers, aInOut, NULL, aDeviceNumber);
	else
		{
		TDigitiserInfoV02 info;
		TPckg<TDigitiserInfoV02> infoPckg(info);
		TInt r =UserSvr::HalFunction(EHalGroupDigitiser, EDigitiserHal3DInfo, (TAny*)&infoPckg, NULL, aDeviceNumber);
		if(r==KErrNone)
			{
			*(TInt*)aInOut=info.iNumberOfPointers;		
			}
		return r;
		}
	}

// ENanoTickPeriod
TInt ProcessNanoTickPeriod(TInt /*aDeviceNumber*/, TInt /*aAttrib*/, TBool /*aSet*/, TAny* aInOut)
	{
	return UserSvr::HalFunction(EHalGroupKernel,EKernelHalNTickPeriod, aInOut, NULL);
	}

// EFastCounterFrequency
TInt ProcessFastCounterFrequency(TInt /*aDeviceNumber*/, TInt /*aAttrib*/, TBool /*aSet*/, TAny* aInOut)
	{
	return UserSvr::HalFunction(EHalGroupKernel,EKernelHalFastCounterFrequency, aInOut, NULL);
	}

//EDisplayMemoryHandle
TInt GetDisplayMemoryHandle(TInt aDeviceNumber, TInt /*aAttrib*/, TBool aSet, TAny* aInOut)
	{
	if (aSet)
		{
		return KErrNotSupported;
		}
	return UserSvr::HalFunction(EHalGroupDisplay, EDisplayHalGetDisplayMemoryHandle, aInOut, NULL, aDeviceNumber);
	}

//ENumCpus
TInt GetNumCpus(TInt /*aDeviceNumber*/, TInt /*aAttrib*/, TBool aSet, TAny* aInOut)
	{
	if (aSet)
		{
		return KErrNotSupported;
		}
	*(TInt*)aInOut=UserSvr::HalFunction(EHalGroupKernel, EKernelHalNumLogicalCpus, NULL, NULL);
	return KErrNone;
	}

// EDigitiserOrientation
#if defined(_DEBUG)
TInt DigitiserOrientation(TInt aDeviceNumber, TInt aAttrib, TBool aSet, TAny* aInOut)
#else
TInt DigitiserOrientation(TInt aDeviceNumber, TInt /*aAttrib*/, TBool aSet, TAny* aInOut)
#endif
	{
	__ASSERT_DEBUG(aAttrib == HALData::EDigitiserOrientation, Panic(EInvalidAttribDigitiserOrientation));
	__ASSERT_DEBUG(aDeviceNumber >= 0, Panic(EInvalidAttribDigitiserOrientation));	
	
	if (aSet)
		{
		//Set
		if ( ((TInt)aInOut) < 0 || ((TInt)aInOut) > HALData::EDigitiserOrientation_270) 
			return KErrArgument;
		return UserSvr::HalFunction(EHalGroupDigitiser, EDigitiserOrientation, aInOut, (TAny*)ETrue, aDeviceNumber);
		}
		
	//Get
	__ASSERT_DEBUG(aInOut != 0, Panic(EInvalidAttribDigitiserOrientation));
	return UserSvr::HalFunction(EHalGroupDigitiser, EDigitiserOrientation, aInOut, (TAny*)EFalse, aDeviceNumber);
	}


