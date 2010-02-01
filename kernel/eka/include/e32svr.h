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
// e32\include\e32svr.h
// 
// WARNING: This file contains some APIs which are internal and are subject
//          to change without notice. Such APIs should therefore not be used
//          outside the Kernel and Hardware Services package.
//

#ifndef __E32SVR_H__
#define __E32SVR_H__
#include <e32hal.h>
#include <d32locd.h>
#include <e32notif.h>
#include <e32ldr.h>
#include <e32ldr_private.h>
#include <e32event.h>
#include <e32event_private.h>
#include <e32ktran.h>
#include <e32debug.h>
#include <e32lmsg.h>

/**
@internalComponent
@removed
*/
enum TBlockType {EBlocked,EUnBlocked,ERelease};

/**
A collection of static functions that are intended for internal use only, 
except for AddEvent.
@see UserSvr::AddEvent()
@publishedPartner
@released
*/
class UserSvr
	{
public:
	IMPORT_C static void CaptureEventHook();	/**< @internalAll */
	IMPORT_C static void ReleaseEventHook();	/**< @internalAll */
	IMPORT_C static void RequestEvent(TRawEventBuf &anEvent,TRequestStatus &aStatus);	/**< @internalAll */
	IMPORT_C static void RequestEventCancel();	/**< @internalAll */
	IMPORT_C static TInt AddEvent(const TRawEvent& anEvent);
	IMPORT_C static void ScreenInfo(TDes8& anInfo);	/**< @internalAll */
	IMPORT_C static TInt DllSetTls(TInt aHandle, TAny *aPtr);	/**<@internalAll */
	IMPORT_C static TInt DllSetTls(TInt aHandle, TInt aDllUid, TAny *aPtr);	/**< @internalComponent */
	IMPORT_C static TAny *DllTls(TInt aHandle);	/**< @internalAll */
	IMPORT_C static TAny *DllTls(TInt aHandle, TInt aDllUid);	/**< @internalComponent */
	IMPORT_C static void DllFreeTls(TInt aHandle);	/**< @internalAll */
	IMPORT_C static void DllFileName(TInt aHandle, TDes &aFileName);	/**< @internalAll */
	IMPORT_C static void FsRegisterThread();	/**< @internalAll */
	IMPORT_C static void RegisterTrustedChunk(TInt aHandle);/**< @internalComponent */
	IMPORT_C static void WsRegisterThread();	/**< @internalAll */
    IMPORT_C static TBool TestBootSequence();	/**< @internalAll */
    IMPORT_C static void WsRegisterSwitchOnScreenHandling(TBool aState);	/**< @internalAll */
    IMPORT_C static void WsSwitchOnScreen();	/**< @internalAll */
    IMPORT_C static TInt ChangeLocale(const TDesC& aLocaleDllName);	/**< @internalAll */
	IMPORT_C static TInt ResetMachine(TMachineStartupType aType);	/**< @internalAll */
	IMPORT_C static void UnlockRamDrive();	/**< @internalAll */
	IMPORT_C static void LockRamDrive();	/**< @internalAll */
	IMPORT_C static TUint32 RomRootDirectoryAddress();	/**< @internalAll */
	IMPORT_C static TInt ExecuteInSupervisorMode(TSupervisorFunction aFunction, TAny* aParameter);	/**< @internalAll */
	IMPORT_C static TUint32 RomHeaderAddress();	/**< @internalAll */
	IMPORT_C static TUint32 DebugMask();	/**< @internalAll */ //Return the kernel debug mask 0
	IMPORT_C static TUint32 DebugMask(TUint aIndex);	/**<@internalAll */	// Return the kernel debug mask
	IMPORT_C static TInt HalFunction(TInt aGroup, TInt aFunction, TAny* a1, TAny* a2);	/**<@internalAll */
	IMPORT_C static TInt HalFunction(TInt aGroup, TInt aFunction, TAny* a1, TAny* a2, TInt aDeviceNumber);	/**<@internalAll */
	IMPORT_C static TInt HalGet(TInt,TAny*);	/**<@internalAll */ // redundant - only here for BC
	IMPORT_C static TInt HalSet(TInt,TAny*);	/**<@internalAll */ // redundant - only here for BC
	IMPORT_C static TInt SetMemoryThresholds(TInt aLowThreshold, TInt aGoodThreshold); /**<@internalAll */
	IMPORT_C static TBool IpcV1Available();	/**<@internalAll */
	IMPORT_C static TLinAddr ExceptionDescriptor(TLinAddr aCodeAddress); /**< @internalTechnology */
	IMPORT_C static TInt LocalePropertiesSetDefaults();	/**< @internalTechnology */
	};

#endif

