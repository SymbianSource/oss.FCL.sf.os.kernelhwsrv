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
// LDD for testing SDIO functions
// 
//

#include <kernel/kernel.h>
#include "regifc.h"
#include "cisreader.h"
#include "d_sdioif.h"

/**
Define the name of the LDD.

@internal
@test
*/
_LIT(KLddName,"D_SDIOIF");

/**
Define the version of the LDD.

@internal
@test
*/
const TInt KMajorVersionNumber=1;
const TInt KMinorVersionNumber=0;
const TInt KBuildVersionNumber=1;

/**
Define the default socket number.

@internal
@test
*/
#ifdef __WINS__
	const TInt KSocketNumber = 0;
#else
	const TInt KSocketNumber = 0;
//	const TInt KSocketNumber = 1;	// 1 for Integrator!!
#endif

/**
Define the default stack number.

@internal
@test
*/
const TInt KStackNumber  = 0;

/**
Define the default card number.

@internal
@test
*/
const TInt KCardNumber   = 0;

/**
Define an invalid function number outside the normal 0-7 range.

@internal
@test
*/
const TUint8 KInvalidFuncNum = 8;

/**
Define the global Dfc Que.

@internal
@test
*/
TDynamicDfcQue* gDfcQ;

class DTestFactory : public DLogicalDevice
/**
Class to act as a factory for the test LDD

@internal
@test
*/
	{
public:
	DTestFactory();
	~DTestFactory();
	virtual TInt Install(); 					//overriding pure virtual
	virtual void GetCaps(TDes8& aDes) const;	//overriding pure virtual
	virtual TInt Create(DLogicalChannelBase*& aChannel); 	//overriding pure virtual
	};


class DTest : public DLogicalChannel
/**
Class containing the logical device driver to drive the SDIO classes.

@internal
@test
*/
	{
public:
	DTest();
	virtual ~DTest();
protected:
	virtual TInt DoCreate(TInt aUnit, const TDesC8* anInfo, const TVersion& aVer);
	virtual void HandleMsg(class TMessageBase *);
	virtual TInt SendMsg(TMessageBase* aMsg);
private:
	TInt SendRequest(TMessageBase* aMsg);
	TInt DoRequest(TInt aReqNo, TRequestStatus* aStatus, TAny* a1, TAny* a2);
	TInt SendControl(TMessageBase* aMsg);
	TInt DoControl(TInt aFunction, TAny* a1, TAny* a2);
	TInt CheckForChangeOfCis(TInt aFunc);
	TInt DoCancel(TUint aMask);
	static void EventCallBack(TAny* aPtr, TInt aReason, TAny* a1, TAny* a2);

private:
	// The SDIO objects
	DMMCSocket* iSocketP;
	DMMCStack*  iStackP;
	TSDIOCard*  iCardP;
    TInt iFunc;
	TCisReader iCisRd;

	DThread* iClient;
	TPBusCallBack iBusEventCallback;

	// Client requests used for creating local copies of user requests, WDP safe
	TClientRequest* 								iPowerUpRequest;
	TClientRequest* 								iResetCisRequest;
	TClientDataRequest<TSDIOCardConfig>*			iCardCommonReadRequest;
	TClientDataRequest<TSDIOFunctionCaps>*			iFunctionCapsRequest;
	TClientDataRequest<TUint>*						iReadDirectRequest; // Use TUint rather than TUint8 for alignment purposes
	};

DECLARE_STANDARD_LDD()
/**
The standard entry point for logical device drivers.

@internal
@test
*/
	{
	return new DTestFactory;
	}

DTestFactory::DTestFactory()
/**
Constructor.

@internal
@test
*/
	{
    iParseMask=KDeviceAllowUnit;
	iUnitsMask=0xffffffff;
	iVersion=TVersion(KMajorVersionNumber,KMinorVersionNumber,KBuildVersionNumber);
	}

TInt DTestFactory::Create(DLogicalChannelBase*& aChannel)
/**
Create a new DTest on this logical device.

@return One of the system wide error codes.

@internal
@test
*/
	{
	aChannel=new DTest;
	return aChannel?KErrNone:KErrNoMemory;
	}

const TInt KDSdioIfThreadPriority = 27;
 _LIT(KDSdioIfThread,"DSdioIfThread");

TInt DTestFactory::Install()
/**
Install the LDD - overriding pure virtual.

@return One of the system wide error codes.

@internal
@test
*/
	{
	// Allocate a kernel thread to run the DFC 
	TInt r = Kern::DynamicDfcQCreate(gDfcQ, KDSdioIfThreadPriority, KDSdioIfThread);

	if (r != KErrNone)
		return r; 	

	return SetName(&KLddName);
	}

void DTestFactory::GetCaps(TDes8& aDes) const
/**
Return the capabilities of the LDD.

@return A packaged TCapsTestV01.

@internal
@test
*/
	{
	TCapsTestV01 b;
	b.iVersion=TVersion(KMajorVersionNumber,KMinorVersionNumber,KBuildVersionNumber);
    Kern::InfoCopy(aDes,(TUint8*)&b,sizeof(b));
	}

DTestFactory::~DTestFactory()
/**
Destructor

@internal
@test
*/
	{
	if (gDfcQ)
		gDfcQ->Destroy();
	}

TInt DTest::DoCreate(TInt aUnit, const TDesC8* /*aInfo*/, const TVersion& aVer)
/**
Create a logical channel.

@param aUnit The socket number.
@param aInfo Not used.
@param aVer The version requested.

@return KErrNone if the channel was created. KErrNotSupported if the version is not supported
        otherwise one of the system wide error codes.

@internal
@test
*/
	{

	if (!Kern::QueryVersionSupported(TVersion(KMajorVersionNumber,KMinorVersionNumber,KBuildVersionNumber),aVer))
		return KErrNotSupported;

	// Create the asynchronous callback client request
	TInt ret = Kern::CreateClientRequest(iPowerUpRequest);
	if (ret != KErrNone)
		return ret;

	ret = Kern::CreateClientRequest(iResetCisRequest);
	if (ret != KErrNone)
		return ret;
	
	ret = Kern::CreateClientDataRequest(iReadDirectRequest);
	if (ret != KErrNone)
		return ret;

	ret = Kern::CreateClientDataRequest(iCardCommonReadRequest);
	if (ret != KErrNone)
		return ret;

	ret = Kern::CreateClientDataRequest(iFunctionCapsRequest);
	if (ret != KErrNone)
		return ret;
	
	//
	// Obtain the appropriate card from the socket/stack
	//
	iSocketP = static_cast<DMMCSocket*>(DPBusSocket::SocketFromId(KSocketNumber));
	if(iSocketP == NULL)
		return KErrNoMemory;

	iStackP = static_cast<DSDIOStack*>(iSocketP->Stack(KStackNumber));
	if(iStackP == NULL)
		return KErrNoMemory;

	iCardP = static_cast<TSDIOCard*>(iStackP->CardP(KCardNumber));
	if(iCardP == NULL)
		return KErrNoMemory;

	iFunc=KInvalidFuncNum; // Indicates Cis reader isn't selected

	SetDfcQ(gDfcQ);
	iMsgQ.Receive();

	iBusEventCallback.SetSocket(aUnit);
	iBusEventCallback.Add();

	return KErrNone;
	}

DTest::DTest()
/**
Constructor.

@internal
@test
*/
	: iBusEventCallback(DTest::EventCallBack, this)
	{
	iClient=&Kern::CurrentThread();
	((DObject*)iClient)->Open();	// can't fail since thread is running
	}


DTest::~DTest()
/**
Destructor.

@internal
@test
*/
	{
	iBusEventCallback.Remove();
	
	// Destroy the client requests 
	Kern::DestroyClientRequest(iPowerUpRequest);
	Kern::DestroyClientRequest(iResetCisRequest);
	Kern::DestroyClientRequest(iReadDirectRequest);	
	Kern::DestroyClientRequest(iCardCommonReadRequest);
	Kern::DestroyClientRequest(iFunctionCapsRequest);
	
	Kern::SafeClose((DObject*&)iClient,NULL);
	}

/**
Pre-process the received message to prepare the client's request.

@param aMsg A pointer to a message (request) from the user side.
@return One of the system wide error codes.

@internal
@test
*/
TInt DTest::SendMsg(TMessageBase* aMsg)
	{
	TThreadMessage& m = *(TThreadMessage*)aMsg;
    TInt id = m.iValue;

	// we only support one client
	if (id != (TInt)ECloseMsg && m.Client() != iClient)
		return KErrAccessDenied;
	
	TInt r = KErrNone;
	if (id != (TInt)ECloseMsg && id != KMaxTInt)
		{
		if (id<0)
			{
			// It's a request
			TRequestStatus* pS = (TRequestStatus*)m.Ptr0();
			
			// Pre-process the request
			r = SendRequest(aMsg);
			if (r != KErrNone)
				Kern::RequestComplete(pS,r);
			}
		else
			{
			// Pre-process the control 
			r = SendControl(aMsg);
			}
		}
	else
		r = DLogicalChannel::SendMsg(aMsg);
	
	return r;
	}

/**
Handle a request message from the user side RSdioCardCntrlIf.

@param aMsg A pointer to a message (request) from the user side.

@internal
@test
*/
void DTest::HandleMsg(TMessageBase* aMsg)
    {
    TThreadMessage& m=*(TThreadMessage*)aMsg;
    TInt id=m.iValue;
    
	if (id==(TInt)ECloseMsg)
		{
	    // Check for a close message
		m.Complete(KErrNone, EFalse);
		return;
		}
    else if (id==KMaxTInt)
		{
		// DoCancel
		DoCancel(m.Int0());
		m.Complete(KErrNone, ETrue);
		return;
		}

    if (id<0)
		{
		// DoRequest
		TRequestStatus* pS=(TRequestStatus*)m.Ptr0();
		TInt r=DoRequest(~id, pS, m.Ptr1(), m.Ptr2());
		if (r!=KErrNone)
	    	Kern::RequestComplete(iClient, pS, r);
		m.Complete(KErrNone,ETrue);
		}
    else
		{
		// DoControl
		TInt r=DoControl(id,m.Ptr0(),m.Ptr1());
		m.Complete(r,ETrue);
		}
	}

/**
Handle a pre-process for a request message from the user side RSdioCardCntrlIf.
This will set-up the client requests.

@param aMsg A pointer to a message (request) from the user side.
@return One of the system wide error codes.

@internal
@test
*/
TInt DTest::SendRequest(TMessageBase* aMsg)
	{
	TThreadMessage& m = *(TThreadMessage*)aMsg;
    TInt function = ~m.iValue;
	TRequestStatus* pS = (TRequestStatus*)m.Ptr0();
	TAny* a2 = m.Ptr2();
		
	TInt r = KErrNotSupported;
	switch (function)
		{
		// A request to power up the SDIO card & stack
		case RSdioCardCntrlIf::EReqPwrUp:
			r = iPowerUpRequest->SetStatus(pS);
	       	if (r != KErrNone)
        		return r;
 			break;
		// A request to read generic data from the SDIO card 
        case RSdioCardCntrlIf::ERequestReadDirect:
           	{
            r = iReadDirectRequest->SetStatus(pS);
            if (r != KErrNone)
            	return r;
            iReadDirectRequest->SetDestPtr(a2);            
            }
    		break;			
    	// A request to reset the CIS pointer 
        case RSdioCardCntrlIf::ERequestResetCis:
			r = iResetCisRequest->SetStatus(pS);
			if (r != KErrNone)
				return r;
			break;
			
       	// A request to read the Card Common Config  
        case RSdioCardCntrlIf::ERequestGetCommonConfig:
        	{
        	r = iCardCommonReadRequest->SetStatus(pS);
        	if (r != KErrNone)
        		return r;
        	iCardCommonReadRequest->SetDestPtr(a2);
        	}
			break;
			
        // A request to read the function data (FBR)  
        case RSdioCardCntrlIf::ERequestGetFunctionConfig:
        	{
        	r = iFunctionCapsRequest->SetStatus(pS);
        	if (r != KErrNone)
        		return r;
        	iFunctionCapsRequest->SetDestPtr(a2);
        	}
			break;
		}

	if (r == KErrNone)
		r = DLogicalChannel::SendMsg(aMsg);
	return r;
	}

/**
Process any asynchronous requests from the user side.

@param aFunction The asynchronous function to invoke. 
@param aStatus On completion, the success code for the function.
@param a1 Context sensitive data.
@param a2 Context sensitive data.

@return One of the system wide error codes.

@internal
@test
*/
TInt DTest::DoRequest(TInt aFunction, TRequestStatus* aStatus, TAny* a1, TAny* a2)
	{
	TInt r=KErrNone;
    TInt func = (TInt)a1;
	switch (aFunction)
		{
		// A request to power up the SDIO card & stack
        case RSdioCardCntrlIf::EReqPwrUp:
			{
			if(!iSocketP->CardIsPresent())
				{
				// An SDIO card is not present
				Kern::QueueRequestComplete(iClient, iPowerUpRequest, KErrNotReady);
				}
			else if(iSocketP->State() == EPBusOn)
				{
				// The card is already powered up
				Kern::QueueRequestComplete(iClient, iPowerUpRequest, KErrNone);
				}
			else
				{
				// Power up the card
				iSocketP->PowerUp();
				}
			break;
			}

		// A request to read generic data from the SDIO card 
        case RSdioCardCntrlIf::ERequestReadDirect:
            {
            TInt addr = (TInt)a1;

    		TUint8 val = 0;
			r = iCardP->CommonRegisterInterface()->Read8(addr, &val);
			if(r == KErrNone)
				{
		   		iReadDirectRequest->Data() = (TUint)val;
				}
				
			Kern::QueueRequestComplete(iClient, iReadDirectRequest, r);
			break;
            }

    	// A request to reset the CIS pointer 
        case RSdioCardCntrlIf::ERequestResetCis:
            {
	        if ((r=CheckForChangeOfCis(func))==KErrNone)
	            {
			    iCisRd.Restart();
	            }
	            
			Kern::QueueRequestComplete(iClient, iResetCisRequest, r);
			break;
            }

       	// A request to read the card common config  
        case RSdioCardCntrlIf::ERequestGetCommonConfig:
            {
	        if ((r=CheckForChangeOfCis(func))==KErrNone)
                {
				memset(&iCardCommonReadRequest->Data(), 0, sizeof(TSDIOCardConfig));
				
		        r = iCisRd.FindReadCommonConfig(iCardCommonReadRequest->Data());
				Kern::QueueRequestComplete(iClient, iCardCommonReadRequest, r);
                }
			break;
            }

        // A request to read the function data (FBR)  
        case RSdioCardCntrlIf::ERequestGetFunctionConfig:
            {		        
	        if ((r=CheckForChangeOfCis(func))==KErrNone)
                {
				memset(&iFunctionCapsRequest->Data(), 0, sizeof(TSDIOFunctionCaps));
				
		        r=iCisRd.FindReadFunctionConfig(iFunctionCapsRequest->Data());	
				Kern::QueueRequestComplete(iClient, iFunctionCapsRequest, r);
                }            
			break;
            }

		default:
			r=KErrNotSupported;
			break;
		}
	return r;
	}

/**
Surround the DoControl command, creating a kernel copy of the user side data, then copying back afterwards

@param aMsg The message 
@return One of the system wide error codes.

@internal
@test
*/
TInt DTest::SendControl(TMessageBase* aMsg)
	{
	TThreadMessage& m = *(TThreadMessage*)aMsg;
    TInt id = m.iValue;

    TSdioCardInfo kernelCardInfo;
    TAny* userCardInfoPtr = m.Ptr0();
    
	// thread-local copy of configuration data	
	switch (id)
		{
		case RSdioCardCntrlIf::ESvCardInfo:
			// copy config from client to local buffer in context of client thread
			umemget32(&kernelCardInfo, userCardInfoPtr, sizeof(TSdioCardInfo));
			// update message to point to kernel-side buffer
			m.iArg[0] = &kernelCardInfo;
			break;
		}

	TInt r = DLogicalChannel::SendMsg(aMsg);
	if (r != KErrNone)
		return r;

	switch (id)
		{
		case RSdioCardCntrlIf::ESvCardInfo:
			// copy config from local bufferto client in context of client thread
			umemput32(userCardInfoPtr, &kernelCardInfo, sizeof(TSdioCardInfo));
			break;
		}

	return r;
	}

/**
Process any synchronous requests from the user side.

@param aFunction The synchronous function to invoke. 
@param a1 Context sensitive data.
@param a2 Context sensitive data.

@return One of the system wide error codes.

@internal
@test
*/
TInt DTest::DoControl(TInt aFunction, TAny* a1, TAny* /*a2*/)
	{
	TInt r=KErrNone;
	switch (aFunction)
		{
		// Read the card information data
		case RSdioCardCntrlIf::ESvCardInfo:
            {
			if(iCardP)
				{
				iCardP->CheckCIS();
				TSdioCardInfo* cardInfoPtr = (TSdioCardInfo*)a1;
				TSdioCardInfo& info = *cardInfoPtr;

				//
				// Extract the card information
				//
				info.isComboCard=iCardP->IsComboCard();				
	            info.iIsReady=iCardP->IsPresent();
	            info.iIsLocked=iCardP->IsLocked();
	            info.iCardSpeed=iCardP->MaxTranSpeedInKilohertz();
				TCID* cid=(TCID*)&(iCardP->CID());
				TInt i;
				for (i=0;i<16;i++)
					info.iCID[i]=cid->At(i);
				const TCSD& csd = iCardP->CSD();
				for (i=0;i<16;i++)
					info.iCSD[i]=csd.At(i);
	            info.iRCA=TUint16(iCardP->RCA());
	            info.iMediaType=TMmcMediaType(iCardP->MediaType());

				//
				// Extract the function information
				//
				info.iFuncCount = iCardP->FunctionCount();

				TSDIOFunctionCaps functionCaps;
				TSDIOFunction* functionP = NULL;

				for(TUint8 func=0; func<=info.iFuncCount; func++)
					{
					functionP = iCardP->IoFunction((TUint8) (func));
					if(functionP)
						{
						functionCaps = functionP->Capabilities();
						info.iFunction[func].iType = (TSdioFunctionType)(functionCaps.iType);
						}
					}
				}
			else
			    {
                r = KErrGeneral;
			    }

			break;
            }
		default:
			r=KErrNotSupported;
			break;
		}
	return r;
	}


/**
Check if diferent function selected, select new CIS if necessary.

@param aFunc The SDIO function. 

@return One of the system wide error codes.

@internal
@test
*/
TInt DTest::CheckForChangeOfCis(TInt aFunc)
	{

	if (iFunc!=aFunc||iFunc==KInvalidFuncNum)
		{
		TInt err;
		if ((err=iCisRd.SelectCis(KSocketNumber,0,0,(TUint8) aFunc))==KErrNone)
			iFunc=aFunc;
		return(err);
		}
	return(KErrNone);
	}

/**
Cancel an asynchronous request

@param aMask Mask of requests to cancel
@return One of the system wide error codes.

@internal
@test
*/
TInt DTest::DoCancel(TUint /*aMask*/)
	{	
	if (iPowerUpRequest->IsReady())
		{
		Kern::QueueRequestComplete(iClient, iPowerUpRequest, KErrCancel);
		}
	
	if (iResetCisRequest->IsReady())
		{
		Kern::QueueRequestComplete(iClient, iPowerUpRequest, KErrCancel);
		}

	if (iCardCommonReadRequest->IsReady())
		{
		Kern::QueueRequestComplete(iClient, iPowerUpRequest, KErrCancel);
		}

	if (iFunctionCapsRequest->IsReady())
		{
		Kern::QueueRequestComplete(iClient, iPowerUpRequest, KErrCancel);
		}

	if (iReadDirectRequest->IsReady())
		{
		Kern::QueueRequestComplete(iClient, iPowerUpRequest, KErrCancel);
		}
	
	return KErrNone;
	}

/**
Asynchronous call backs from the SDIO stack

@param aPtr Data passed in when the callback was registered.
@param aReason The reason for the callback, one of TPBusCallBack::EPBusStateChange
               or TPBusCallBack::EPBusCustomNotification.                  
@param a1 Context sensitive data.
@param a2 Context sensitive data.

@return One of the system wide error codes.

@internal
@test
*/
void DTest::EventCallBack(TAny* aPtr, TInt aReason, TAny* a1, TAny* a2)
	{
	DTest &mci = *(DTest*)aPtr;

	if (mci.iPowerUpRequest->IsReady())
		{
		// There is an TRequestStatus pending
		TInt retCode = KErrCompletion;

		switch(aReason)
			{
			// There has been a state change
			case TPBusCallBack::EPBusStateChange:
				{
				TPBusState newState = (TPBusState)(TInt)a1;
				TInt errorCode = (TInt)a2;

				switch(newState)
					{
					case EPBusCardAbsent:	retCode = KErrNotFound;		break;			
					case EPBusOff:			retCode = errorCode;		break;
					case EPBusPsuFault:		retCode = KErrBadPower;		break;
					case EPBusOn:			retCode = KErrNone;			break;
					case EPBusPowerUpPending:
					case EPBusPoweringUp:
					default:	
						break;
					}

				break;
				}
			}

		if(retCode != KErrCompletion)
			{
   			Kern::QueueRequestComplete(mci.iClient, mci.iPowerUpRequest, retCode);
			}
		}
	}



