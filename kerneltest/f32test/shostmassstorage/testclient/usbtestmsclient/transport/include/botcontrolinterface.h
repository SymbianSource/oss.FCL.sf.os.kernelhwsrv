// Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
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
// Class declaration for CBulkOnlyTransport.
// 
//



/**
 @file
 @internalTechnology
*/

#ifndef BOTCONTROLINTERFACE_H
#define BOTCONTROLINTERFACE_H


        // for control endpoint
        static const TUint KRequestHdrSize = 8;

class CBulkOnlyTransport;


class TTestConfig
    {
public:
    enum TTestMode
        {
        ENone,
        EWrongTag
        };

    TTestConfig();

private:

    TTestMode iTestMode;

    };



/**
Represent session with control endpoint (Ep0).
handles the control interface, and responds to the class specific commands (RESET and GET_MAX_LUN).
*/
class CBotControlInterface : public CActive
	{
public:
	enum TControlState
		{
		ENone,
		EReadEp0Data,
		ESendMaxLun
		};

public:

	static CBotControlInterface* NewL(CBulkOnlyTransport& aParent);

	~CBotControlInterface();
	TInt Start();
	void Stop();
	virtual void RunL();
	virtual void DoCancel();


private:
	CBotControlInterface(CBulkOnlyTransport& aParent);
	void ConstructL();
	void ReadEp0Data();
	void DecodeEp0Data();

private:
	/** Buffer for request data*/
	TBuf8<TBotFunctionReqCb::KRequestHdrSize> iData;
	TBotServerFunctionReq iRequestHeader;
	/** reference to the  CBulkOnlyTransport*/
	CBulkOnlyTransport& iParent;
	/** represent carrent state for state mashine */
	TControlState iCurrentState;
	};


class CBotControlStateMachine : public CActive
	{
public:

    };

#endif // BOTCONTROLINTERFACE_H
