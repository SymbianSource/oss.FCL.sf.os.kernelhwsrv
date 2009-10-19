// Copyright (c) 2003-2009 Nokia Corporation and/or its subsidiary(-ies).
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
//

#include <drivers/sdio/sdioinit.h>
#include <drivers/pbusmedia.h>
#include "utraceepbussdio.h"

EXPORT_C TInt TSDIOCardControllerInterface::RegisterMediaDevices(TInt aSocket)
/**
Registers the media devices for the specified socket.
By default, All MMC derivatives register at least an MMC driver,
and all SDIO compatable variants register a CSA driver.
@internalTechnology
*/
	{
	TRACE2(TTraceContext(EBorder), UTraceModuleEPBusSDIO::ESDIOTSDIOCardControllerInterfaceRegisterMediaDevice, reinterpret_cast<TUint32>(this), static_cast<TUint32>(aSocket)); // @SymTraceDataPublishedTvk
	
	// Register the generic drivers first:
	TInt err = TMMCardControllerInterface::RegisterMediaDevices(aSocket);
	if(err == KErrNone)
		{
		// ...and now install a CSA media driver:
		SMediaDeviceInfo mdi;
		if (IsSDIOSocket(aSocket, mdi))
			{		
			DPBusSocket* pS=TheSockets[aSocket];
			if(pS == NULL)
				err = KErrNoMemory;
			else
				{
				DPBusPrimaryMedia* pMedia = new DPBusPrimaryMedia(pS);
				if (pMedia == NULL)
					err = KErrNoMemory;
				else
					err = LocDrv::RegisterMediaDevice(mdi.iDevice,
												  mdi.iDriveCount,
												  mdi.iDriveList,
												  pMedia,
												  mdi.
												  iNumMedia,
												  *mdi.iDeviceName);
				}
			}
		}

	TRACE2(TTraceContext(EBorder), UTraceModuleEPBusSDIO::ESDIOTSDIOCardControllerInterfaceRegisterMediaDeviceReturning, reinterpret_cast<TUint32>(this), err); // @SymTraceDataPublishedTvk
	
	return(err);
	}
