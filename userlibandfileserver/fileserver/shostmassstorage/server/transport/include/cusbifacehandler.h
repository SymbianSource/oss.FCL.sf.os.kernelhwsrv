// Copyright (c) 2008-2010 Nokia Corporation and/or its subsidiary(-ies).
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

/**
 @file
 @internalTechnology
*/

#ifndef CUSBIFACEHANDLER_H
#define CUSBIFACEHANDLER_H

class CUsbInterfaceHandler : public CActive
	{
public:
    static CUsbInterfaceHandler* NewL(RUsbInterface &aInterface, RUsbPipe& aBulkPipeIn);
	CUsbInterfaceHandler(RUsbInterface &aInterface, RUsbPipe& aBulkPipeIn);
	~CUsbInterfaceHandler();

	void GetMaxLun(TLun* aReceiveData, const RMessage2& aMessage);
	
private:
	void RunL();
	void DoCancel();

    enum TState
        {
        ENone,
        EReset,
        EGetMaxLun
        };

private:
	TState iState;
	RMessage2 iBotGetMaxLun;
	TLun* ipGetMaxLun;

	RUsbInterface& iInterface;
    RUsbPipe& iBulkPipeIn;
	TBuf8<1> iBuffer;
	};

#endif // CUSBIFACEHANDLER_H
