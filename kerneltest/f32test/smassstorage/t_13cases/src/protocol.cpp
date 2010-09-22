// Copyright (c) 2004-2010 Nokia Corporation and/or its subsidiary(-ies).
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


#include "t_13cases_protocol.h"

/**
 Stub SCSI protocol class for testing the bulk only transport classes.
*/

CScsiProtocol::CScsiProtocol() :
	iReadData(NULL),
	iWriteData(NULL)
	{
	}
		
CScsiProtocol* CScsiProtocol::NewL()
	{
	return new (ELeave) CScsiProtocol();
	}
	
void CScsiProtocol::RegisterTransport(MTransportBase* aTransport)
	{
	iTransport = aTransport;
	}
	
TBool CScsiProtocol::DecodePacket(TPtrC8& aData, TUint /*aLun*/)
	{
	ASSERT(aData[0] == 2);
	TUint8 direction = aData[1];
	TInt length = aData[2];
	if (length != 0)
		{
		ASSERT(direction == 0 || direction == 1);
		if (direction == 0) //In
			{
			//Write data
			delete iWriteData;
			iWriteData = HBufC8::NewMax(length);
			ASSERT(iWriteData != NULL);
			TPtrC8 ptr(iWriteData->Des().Ptr(), length);
			iTransport->SetupWriteData(ptr);
			}
		else if (direction == 1) //Out
			{
			//Read data
			ASSERT(iReadData == NULL);
			iReadData = HBufC8::NewMax(length);
			ASSERT(iReadData != NULL);
			iReadingData = ETrue;
			iTransport->SetupReadData(length);
			}
		}
	else
	{
		// initialise receive buffer to be zero length
		if (direction == 1)
		{
			ASSERT(iReadData == NULL);
			iReadData = HBufC8::NewMax(0);
			ASSERT(iReadData != NULL);
			iReadingData = ETrue;
			iTransport->SetupReadData(0);
		}
	}
		
	return ETrue;
	}
	
TInt CScsiProtocol::ReadComplete(TInt aError)
	{
	ASSERT(iReadingData);
	ASSERT(iReadData != NULL);
	iReadingData = EFalse;
	if (aError != KErrNone)
		{
		return KErrAbort;
		}
	delete iReadData;
	iReadData = NULL;
	return KErrNone;
	}
	
TInt CScsiProtocol::Cancel()
	{
	return KErrNone;
	}
	
CScsiProtocol::~CScsiProtocol()
	{
	delete iReadData;
	delete iWriteData;
	}
	
