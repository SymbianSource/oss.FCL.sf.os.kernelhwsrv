// Copyright (c) 2008-2009 Nokia Corporation and/or its subsidiary(-ies).
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
	CUsbInterfaceHandler(RUsbInterface &aInterface);
	~CUsbInterfaceHandler();
	void GetMaxLun(TLun* aReceiveData, const RMessage2& aMessage);
	static CUsbInterfaceHandler* NewL(RUsbInterface &aInterface);

private:
	void RunL();
	void Reset();
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
	TBuf8<1> iBuffer;
	};

#endif // CUSBIFACEHANDLER_H
