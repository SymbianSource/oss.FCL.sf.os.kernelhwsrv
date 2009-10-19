// Copyright (c) 2006-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// e32test\device\t_usblib.h
// 
//

#ifndef __T_USBLIB_H__
#define __T_USBLIB_H__


// --- Little USB Test Helpers ---

//
// Returns ETrue if the testing platform is not in the list of platforms which
// are known NOT to have any USB support. In other words, assume that licensee
// platforms which aren't in this list WILL support USB.
//
inline TBool SupportsUsb()
	{
	TInt muid = 0;
	const TInt r = HAL::Get(HAL::EMachineUid, muid);
	if (r != KErrNone) return EFalse;;
	return ((muid != HAL::EMachineUid_Series5mx) &&
			(muid != HAL::EMachineUid_Cogent) &&
			(muid != HAL::EMachineUid_Win32Emulator) &&
			(muid != HAL::EMachineUid_WinC) &&
			(muid != HAL::EMachineUid_CL7211_Eval) &&
			(muid != HAL::EMachineUid_LinkUp) &&
			(muid != HAL::EMachineUid_IQ80310) &&
			(muid != HAL::EMachineUid_Integrator) &&
			(muid != HAL::EMachineUid_Helen) &&
			(muid != HAL::EMachineUid_NE1_TB) &&
			(muid != HAL::EMachineUid_X86PC));
	}


//
// Returns ETrue if the testing platform is not in the list of platforms which
// are known NOT to support alternate interface settings.
//
inline TBool SupportsAlternateInterfaces()
	{
	TInt muid = 0;
	const TInt r = HAL::Get(HAL::EMachineUid, muid);
	if (r != KErrNone) return EFalse;;
	return ((muid != HAL::EMachineUid_Brutus) &&
			(muid != HAL::EMachineUid_Assabet) &&
			(muid != HAL::EMachineUid_Lubbock));
	}


//
// Returns ETrue if the testing platform is not in the list of platforms which
// are known NOT to support stalling of endpoints in an unconfigured state.
// (Some UDCs don't permit endpoint stall operations until a SET_CONFIGURATION
// request has been received.)
//
inline TBool SupportsEndpointStall()
	{
	TInt muid = 0;
	const TInt r = HAL::Get(HAL::EMachineUid, muid);
	if (r != KErrNone) return EFalse;;
	return ((muid != HAL::EMachineUid_OmapH2) &&
			(muid != HAL::EMachineUid_OmapH4));
	}


//
// Calculates a 16-bit value out of two single bytes.
//
inline TUint16 EpSize(TUint8 aLowByte, TUint8 aHighByte)
		{
		TUint16 size = aLowByte;
		size |= (aHighByte << 8);
		return size;
		}


#endif	// __T_USBLIB_H__
