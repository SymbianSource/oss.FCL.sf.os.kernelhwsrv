/*
* Copyright (c) 2003 Nokia Corporation and/or its subsidiary(-ies).
* All rights reserved.
* This component and the accompanying materials are made available
* under the terms of the License "Eclipse Public License v1.0"
* which accompanies this distribution, and is available
* at the URL "http://www.eclipse.org/legal/epl-v10.html".
*
* Initial Contributors:
* Nokia Corporation - initial contribution.
*
* Contributors:
*
* Description:
*
*/


#include <kernel/kernel.h>

#include <drivers/sdio/callback.h>
#include <drivers/sdio/sdio.h>
#include "utraceepbussdio.h"

EXPORT_C void TSDIOFunctionCallback::Register(DMMCSocket* aSocketP)
/**
@publishedPartner
@released
 
Provides support for passing notification events to clients of the SDIO controller.

@param aSocketP A pointer to the socket from which to recieve notification events
*/
	{
	TRACE2(TTraceContext(EBorder), UTraceModuleEPBusSDIO::ESDIOFunctionCallbackRegistered, reinterpret_cast<TUint32>(this), reinterpret_cast<TUint32>(aSocketP)); // @SymTraceDataPublishedTvk

	iBusState = (TPBusState)(aSocketP->State());

	iCallBack.iFunction = TSDIOFunctionCallback::NotificationCallBack;
	iCallBack.iPtr=this;
	iCallBack.SetSocket(aSocketP->iSocketNumber);
	iCallBack.Add();	

	TRACE1(TTraceContext(EBorder), UTraceModuleEPBusSDIO::ESDIOFunctionCallbackRegisteredReturning, reinterpret_cast<TUint32>(this)); // @SymTraceDataPublishedTvk	
	}

void TSDIOFunctionCallback::NotificationCallBack(TAny* aPtr, TInt aReason, TAny* a1, TAny* a2)
/**
Provides support for passing notification events to clients of the cards functions.
*/
	{
	SYMBIAN_TRACE_SDIO_VERBOSE_ONLY(Printf(TTraceContext(EInternals), "TSDIOFunctionCallback::NotificationCallBack(%d, 0x%08x, 0x%08x)", aReason, a1, a2)); // @SymTraceDataInternalTechnology
	
	TSDIOFunctionCallback& self = *static_cast<TSDIOFunctionCallback*>(aPtr);

	switch (aReason)
		{
		case TPBusCallBack::EPBusStateChange:
			{
			// a1 == TPBusState
			// a2 == error code
			self.DoStateChange((TPBusState)(TInt)a1, (TInt)a2);
			break;
			}

		case TPBusCallBack::EPBusCustomNotification:
			{
			TInt customReason = (TInt)a1;
			switch(customReason)
				{
				case ESdioNotifyPowerSleep:
					{
					self.CallBack(ESdioNotifyPowerSleep);
					break;
					}
					
				case ESdioNotifyPowerDownPending:
					{
					self.CallBack(ESdioNotifyPowerDownPending);
					break;
					}
					
				default:
					{
					SYMBIAN_TRACE_SDIO_VERBOSE_ONLY(Printf(TTraceContext(EWarning), "Unknown Custom Callback Reason (%d)", customReason)); // @SymTraceDataInternalTechnology
					break;
					}
				}
			break;
			}

		default:
			{
			SYMBIAN_TRACE_SDIO_VERBOSE_ONLY(Printf(TTraceContext(EWarning), "Unknown Callback Reason (%d, 0x%08x, 0x%08x)", aReason, a1, a2)); // @SymTraceDataInternalTechnology
			break;
			}
		}
	}


/**
Provides support for passing notification events to clients of the cards functions.
*/
void TSDIOFunctionCallback::DoStateChange(TPBusState aNewState, TInt aReasonCode)
	{
	if (iBusState != aNewState)
		{
		iBusState = aNewState;
		SYMBIAN_TRACE_SDIO_VERBOSE_ONLY(Printf(TTraceContext(EInternals), "Calling Back to client. iBusState = %d", iBusState)); // @SymTraceDataInternalTechnology
		switch(iBusState)
			{
			case EPBusCardAbsent:
				SYMBIAN_TRACE_SDIO_VERBOSE_ONLY(Printf(TTraceContext(EState), "EPBusCardAbsent (%d)", aReasonCode)); // @SymTraceDataInternalTechnology
				CallBack(ESdioNotifyCardRemoved);
				break;

			case EPBusOff:
				SYMBIAN_TRACE_SDIO_VERBOSE_ONLY(Printf(TTraceContext(EState), "EPBusOff (%d)", aReasonCode)); // @SymTraceDataInternalTechnology

				switch (aReasonCode)
					{
					case KErrNone:
					case KErrTimedOut:
						CallBack(ESdioNotifyPowerDown);
						break;
					case KErrNotReady:
						// card detected following door close
						CallBack(ESdioNotifyCardInserted);
						break;
					case KErrAbort:
						CallBack(ESdioNotifyEmergencyPowerDown);
						break;
					default:
						CallBack(ESdioNotifyPowerUp);
						break;
					}
				break;

			case EPBusPoweringUp:
				SYMBIAN_TRACE_SDIO_VERBOSE_ONLY(Printf(TTraceContext(EState), "EPBusPoweringUp (%d)", aReasonCode)); // @SymTraceDataInternalTechnology
				break;

			case EPBusOn:
				SYMBIAN_TRACE_SDIO_VERBOSE_ONLY(Printf(TTraceContext(EState), "EPBusOn (%d)", aReasonCode)); // @SymTraceDataInternalTechnology
				CallBack(ESdioNotifyPowerUp);
				break;

			case EPBusPsuFault:
				SYMBIAN_TRACE_SDIO_VERBOSE_ONLY(Printf(TTraceContext(EState), "EPBusPsuFault (%d)", aReasonCode)); // @SymTraceDataInternalTechnology
				CallBack(ESdioNotifyPsuFault);
				break;

			case EPBusPowerUpPending:
				SYMBIAN_TRACE_SDIO_VERBOSE_ONLY(Printf(TTraceContext(EState), "EPBusPowerUpPending (%d)", aReasonCode)); // @SymTraceDataInternalTechnology
				break;
			
			default:	
				SYMBIAN_TRACE_SDIO_VERBOSE_ONLY(Printf(TTraceContext(EWarning), "Unknown State Change (%d)", aReasonCode)); // @SymTraceDataInternalTechnology
				break;
			}
		}
	}
