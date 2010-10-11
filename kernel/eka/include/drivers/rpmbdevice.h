// Copyright (c) 1994-2010 Nokia Corporation and/or its subsidiary(-ies).
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
// eka\include\drivers\rpmb\rpmbdevice.h

/**
 @file
 @internalTechnology
*/


#include <drivers/mmc.h>
#include <e32cmn.h>

const TUint KBusNumber = 0;
const TUint KDeviceAddress = 0; //Address on card is not applicable to RPMB. This is handled on media device.
const TUint8 KIndexNotAssigned = 0xFF;

class DRpmbDevice : public DBase
    {
   public:
    
    IMPORT_C DRpmbDevice();
    IMPORT_C ~DRpmbDevice();
    IMPORT_C TInt Open(TUint aDeviceIndex);
    IMPORT_C TInt SendAccessRequest(TDes8 &aRpmbRequest, TDes8 &aRpmbResponse);
    IMPORT_C void Close();
        
   private:
   
    static void SessionEndCallBack(TAny* aSelf);
    static void BusCallBack(TAny* aPtr, TInt aReason, TAny* a1, TAny* a2);
    void DoSessionEndCallBack();
    TInt PowerUpStack();
	void SetSynchronisationParms(TUint8 aDeviceIndex);
	void ClearSynchronisationParms();
    
    DSemaphore* iPowerUpSemaphore;
	DSemaphore* iRequestSemaphore;
	NFastMutex iSynchronisationParmsMutex;

	DMMCSession* iSession;
    TMMCard* iCard;
    DMMCSocket* iSocket;

	TPBusCallBack iBusCallBack;     
	TMMCCallBack iSessionEndCallBack;

	TUint8* iIntBuf;	

	TUint8 iDeviceIndex;
	static DRpmbDevice* DRpmbDevicePtrs[KMaxPBusSockets*4];
	
	TUint8 iSpare[10]; //for future use
    
    };
