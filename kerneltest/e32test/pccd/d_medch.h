// Copyright (c) 1997-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// e32test\pccd\d_medch.h
// This header provides the interface to MEDCH.LDD so that user mode 
// code such as test programs can use it.
// 
//

#if !defined(__D_MEDCH_H__)
#define __D_MEDCH_H__

#include <e32cmn.h>

#ifndef __KERNEL_MODE__
#include <e32std.h>
#endif

class TCapsMediaChangeV01
	{
public:
	TVersion version;
	};

class RMedCh : public RBusLogicalChannel
	{
public:
	enum
		{
		EMajorVersionNumber=1,
		EMinorVersionNumber=0,
		EBuildVersionNumber=1
		};

	enum TControl
        {
		EDoorNormal,
		EDoorOpen,
		EDoorClose,
		EDoubleDoorOpen
		};

	enum TRequest
		{
		EDelayedDoorOpen,
		EDelayedDoorClose
		};
	
public:
	inline void Cancel();
	
	inline TInt Open(TInt aSocket, const TVersion& aVer)
		{
		return(DoCreate(_L("MedCh"), aVer, aSocket, NULL, NULL));
		}
	
	inline TVersion VersionRequired() const
		{
		return(TVersion(EMajorVersionNumber,EMinorVersionNumber,EBuildVersionNumber));
		}
	
	inline TInt DoorOpen()
		{
		return(DoControl(EDoorOpen));
		}

	inline TInt DoorClose(TBool aMediaPresent)
		{
		return(DoControl(EDoorClose, (TAny*)aMediaPresent));
		}
	
	inline TInt DoorNormal()
		{
		return(DoControl(EDoorNormal));
		}

	inline void DelayedDoorOpen(TRequestStatus &aReqStat, TInt aMsDelay)
		{
		DoRequest(EDelayedDoorOpen, aReqStat, (TAny*)aMsDelay);
		}

	inline void DelayedDoorClose(TRequestStatus &aReqStat, TInt aMsDelay, TBool aMediaPresent)
		{
		DoRequest(EDelayedDoorClose, aReqStat, (TAny*)aMsDelay, (TAny*)aMediaPresent);
		}

	inline TInt DoubleDoorOpen()
		{
		return(DoControl(EDoubleDoorOpen));
		}

	};

#endif
