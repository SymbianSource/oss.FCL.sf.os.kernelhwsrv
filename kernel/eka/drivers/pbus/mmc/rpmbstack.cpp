// Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
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
 @internalComponent
 @prototype
*/

#include <drivers/mmc.h>
#include <drivers/rpmbpacket.h>

#include "OstTraceDefinitions.h"
#ifdef OST_TRACE_COMPILER_IN_USE
#include "../../../include/drivers/locmedia_ost.h"
#ifdef __VC32__
#pragma warning(disable: 4127) // disabling warning "conditional expression is constant"
#endif
#include "rpmbstackTraces.h"
#endif

TMMCErr DMMCStack::CIMRpmbAccessSM()
    {
enum TStates
        {
        EStBegin,
        EStEnd
        };

    DMMCSession& s=Session();

    //Extract the RPMB request type from the command. JEDEC specify this to be situated
    //at offset 0 in the packet. When accessing the hardware, this is reversed and is 
    //at offset 511
	TUint8 requestType = * (s.Command().iDataMemoryP + KRpmbRequestLsbOffset);
	
	//There are 5 possible rpmb request types that can be passed through the state machines:
	//
	//Authentication Key programming request:     KRpmbRequestWriteKey = 0x001
	//Reading of the write counter value request: KRpmbRequestReadWriteCounter = 0x0002;
	//Authenticated data write request:           KRpmbRequestWriteData = 0x0003;
	//Authenticated data read request:            KRpmbRequestReadData = 0x0004;
	//Result read request:                        KRpmbRequestReadResultRegister = 0x0005;
	//

	SMF_BEGIN
	
    switch (requestType)
        {
        case KRpmbRequestWriteKey:
			SMF_INVOKES(CIMRpmbWriteAuthenticationKeySMST, EStEnd);
        case KRpmbRequestReadWriteCounter:
			SMF_INVOKES(CIMRpmbReadWrCounterSMST, EStEnd);
        case KRpmbRequestWriteData:
			SMF_INVOKES(CIMRpmbWriteSMST, EStEnd);
        case KRpmbRequestReadData:
 			SMF_INVOKES(CIMRpmbReadSMST, EStEnd);
        default:
            break;   
        }

    SMF_END
}


TMMCErr DMMCStack::CIMRpmbWriteAuthenticationKeySM()
    {
    enum TStates
        {
        EStBegin,   // Write key request
        EStSetupReadRequest,
        EStReadResult,
        EStError,
        EStEnd
        };

    DMMCSession& s=Session();

	SMF_BEGIN

        //
        // Setup write request
        // CMD23 Reliable Write = 1 Block Count = 1
        // CMD25
        // 
        s.SetupRpmbSendRequest(ETrue);
		//
        // KMMCErrByPass is routinely thrown in the ReadWriteBlocks state machine. 
        // It is thrown to indicate that the current command in the state machine is not CMD42(lock/unlock)
        // We need to trap this error here to stop the RPMB state machines from unravelling and passing
        // the error upwards. 
        //	
		m.SetTraps(KMMCErrBypass);
		
		OstTrace0(TRACE_FLOW, DMMCSTACK_CIMRPMBWRITEAUTHENTICATIONKEYSM1, "RPMB: Write Key State Machine - send write key request packet");
        __KTRACE_OPT(KPBUS1, Kern::Printf("RPMB: Write Key State Machine - send write key request packet"));
        
        SMF_INVOKES(CIMReadWriteBlocksSMST, EStSetupReadRequest);

    SMF_STATE(EStSetupReadRequest)

        OstTrace0(TRACE_FLOW, DMMCSTACK_CIMRPMBWRITEAUTHENTICATIONKEYSM2, "RPMB: Write Key State Machine - sent write key request packet");
        __KTRACE_OPT(KPBUS1, Kern::Printf("RPMB: Write Key State Machine - sent write key request packet"));
    
		if (err != KMMCErrNone)
			{
		    OstTrace1(TRACE_FLOW, DMMCSTACK_CIMRPMBWRITEAUTHENTICATIONKEYSM3, "RPMB: Write Key State Machine - sent write key request packet error = %d", err);
		    __KTRACE_OPT(KPBUS1, Kern::Printf("RPMB: Write Key State Machine - sent write key request packet error = %d", err));
			SMF_GOTOS(EStError);
			}
		// 
        // Setup read result register request
        // CMD23 Reliable Write = 0 Block Count = 1
        // CMD25
        //
		s.SetupRpmbSendReadResultRegisterRequest();
		OstTrace0(TRACE_FLOW, DMMCSTACK_CIMRPMBWRITEAUTHENTICATIONKEYSM4, "RPMB: Write Key State Machine - send read result register request packet");
		__KTRACE_OPT(KPBUS1, Kern::Printf("RPMB: Write Key State Machine - send read result register request packet"));
        SMF_INVOKES(CIMReadWriteBlocksSMST, EStReadResult)

    SMF_STATE(EStReadResult)

        OstTrace0(TRACE_FLOW, DMMCSTACK_CIMRPMBWRITEAUTHENTICATIONKEYSM5, "RPMB: Write Key State Machine - sent read result register request packet");
        __KTRACE_OPT(KPBUS1, Kern::Printf("RPMB: Write Key State Machine - sent read result register request packet"));

        if (err != KMMCErrNone)
            {
            OstTrace1(TRACE_FLOW, DMMCSTACK_CIMRPMBWRITEAUTHENTICATIONKEYSM6, "RPMB: Write Key State Machine - sent read result register request packet error = %d", err);
            __KTRACE_OPT(KPBUS1, Kern::Printf("RPMB: Write Key State Machine - sent read result register request packet error = %d", err));
            SMF_GOTOS(EStError);
            }
        //
        // Setup read response
        // CMD23 Reliable Write = 0 Block Count = 1
        // CMD18
        //        
        s.SetupRpmbReceiveResponse();
        OstTrace0(TRACE_FLOW, DMMCSTACK_CIMRPMBWRITEAUTHENTICATIONKEYSM7, "RPMB: Write Key State Machine - get read packet");
        __KTRACE_OPT(KPBUS1, Kern::Printf("RPMB: Write Key State Machine - get read packet"));
        SMF_INVOKES(CIMReadWriteBlocksSMST, EStEnd);

    SMF_STATE(EStError)
        SMF_RETURN(err);

    SMF_END
    }


TMMCErr DMMCStack::CIMRpmbReadWrCounterSM()
    {
    enum TStates
        {
        EStBegin,   // Read counter request
        EStReadResult,
        EStError,
        EStEnd
        };

    DMMCSession& s=Session();

	SMF_BEGIN
	
        //
        // Setup write request
        // CMD23 Reliable Write = 0 Block Count = 1
        // CMD25
        // 
        s.SetupRpmbSendRequest(EFalse);
        m.SetTraps(KMMCErrBypass);
        
        OstTrace0(TRACE_FLOW, DMMCSTACK_CIMRPMBREADWRCOUNTERSM1, "RPMB: Read Write Counter State Machine - send read counter request packet");
        __KTRACE_OPT(KPBUS1, Kern::Printf("RPMB: Read Write Counter State Machine - send read counter request packet"));
 
        SMF_INVOKES(CIMReadWriteBlocksSMST, EStReadResult)

    SMF_STATE(EStReadResult)

        OstTrace0(TRACE_FLOW, DMMCSTACK_CIMRPMBREADWRCOUNTERSM2, "RPMB: Read Write Counter State Machine - sent read counter request packet");
        __KTRACE_OPT(KPBUS1, Kern::Printf("RPMB: Read Write Counter State Machine - sent read counter request packet"));

        if (err != KMMCErrNone)
            {
            OstTrace1(TRACE_FLOW, DMMCSTACK_CIMRPMBREADWRCOUNTERSM3, "RPMB: Read Write Counter State Machine - sent read counter request packet error = %d", err);
            __KTRACE_OPT(KPBUS1, Kern::Printf("RPMB: Read Write Counter State Machine - sent read counter request packet error = %d", err));
            SMF_GOTOS(EStError);
            }
        //
        // Setup read response
        // CMD23 Reliable Write = 0 Block Count = 1
        // CMD18
        //        
        s.SetupRpmbReceiveResponse();
        OstTrace0(TRACE_FLOW, DMMCSTACK_CIMRPMBREADWRCOUNTERSM4, "RPMB: Read Write Counter State Machine - get read packet");
        __KTRACE_OPT(KPBUS1, Kern::Printf("RPMB: Read Write Counter State Machine - get read packet"));
       SMF_INVOKES(CIMReadWriteBlocksSMST, EStEnd);

    SMF_STATE(EStError)
        SMF_RETURN(err);

    SMF_END
    }


TMMCErr DMMCStack::CIMRpmbWriteSM()
    {
    enum TStates
        {
        EStBegin,   // Write data request
        EStSetupReadRequest,
        EStReadResult,
        EStError,
        EStEnd
        };

    DMMCSession& s=Session();

	SMF_BEGIN

        //
        // Setup write request
        // CMD23 Reliable Write = 1 Block Count = 1
        // CMD25
        // 
        s.SetupRpmbSendRequest(ETrue);
	    m.SetTraps(KMMCErrBypass);
	    
	    OstTrace0(TRACE_FLOW, DMMCSTACK_CIMRPMBWRITESM1, "RPMB: Write State Machine - send write request packet");
	    __KTRACE_OPT(KPBUS1, Kern::Printf("RPMB: Write State Machine - send write request packet"));

	    SMF_INVOKES(CIMReadWriteBlocksSMST, EStSetupReadRequest);

    SMF_STATE(EStSetupReadRequest)
	    
	    OstTrace0(TRACE_FLOW, DMMCSTACK_CIMRPMBWRITESM2, "RPMB: Write State Machine - sent write request packet");
	    __KTRACE_OPT(KPBUS1, Kern::Printf("RPMB: Write State Machine - sent write request packet"));

		if (err != KMMCErrNone)
			{
		    OstTrace1(TRACE_FLOW, DMMCSTACK_CIMRPMBWRITESM3, "RPMB: Write State Machine - sent write request packet error = %d", err);
		    __KTRACE_OPT(KPBUS1, Kern::Printf("RPMB: Write State Machine - sent write request packet error = %d", err));
			SMF_GOTOS(EStError);
			}	
        // 
        // Setup read result register request
        // CMD23 Reliable Write = 0 Block Count = 1
        // CMD25
        //
        s.SetupRpmbSendReadResultRegisterRequest();
        OstTrace0(TRACE_FLOW, DMMCSTACK_CIMRPMBWRITESM4, "RPMB: Write State Machine - send read result register request packet");
        __KTRACE_OPT(KPBUS1, Kern::Printf("RPMB: Write State Machine - send read result register request packet"));
        SMF_INVOKES(CIMReadWriteBlocksSMST, EStReadResult)

    SMF_STATE(EStReadResult)
        
        OstTrace0(TRACE_FLOW, DMMCSTACK_CIMRPMBWRITESM5, "RPMB: Write State Machine - sent read result register request packet");
        __KTRACE_OPT(KPBUS1, Kern::Printf("RPMB: Write State Machine - sent read result register request packet"));

        if (err != KMMCErrNone)
            {
            OstTrace1(TRACE_FLOW, DMMCSTACK_CIMRPMBWRITESM6, "RPMB: Write State Machine - sent read result register request packet error = %d", err);
            __KTRACE_OPT(KPBUS1, Kern::Printf("RPMB: Write State Machine - sent read result register request packet error = %d", err));
            SMF_GOTOS(EStError);
            }
        //
        // Setup read response
        // CMD23 Reliable Write = 0 Block Count = 1
        // CMD18
        //        
        s.SetupRpmbReceiveResponse();
        OstTrace0(TRACE_FLOW, DMMCSTACK_CIMRPMBWRITESM7, "RPMB: Write State Machine - get read packet");
        __KTRACE_OPT(KPBUS1, Kern::Printf("RPMB: Write State Machine - get read packet"));
        SMF_INVOKES(CIMReadWriteBlocksSMST, EStEnd);

    SMF_STATE(EStError)
       SMF_RETURN(err);

    SMF_END
    }


TMMCErr DMMCStack::CIMRpmbReadSM()
    {
    enum TStates
        {
        EStBegin,   // Read data request
        EStReadResult,
        EStError,
        EStEnd
        };

    DMMCSession& s=Session();

    SMF_BEGIN
         
        //
        // Setup write request
        // CMD23 Reliable Write = 0 Block Count = 1
        // CMD25
        // 
        s.SetupRpmbSendRequest(EFalse);
        m.SetTraps(KMMCErrBypass);
        
        OstTrace0(TRACE_FLOW, DMMCSTACK_CIMRPMBREADSM1, "RPMB: Read State Machine - send read request packet");
        __KTRACE_OPT(KPBUS1, Kern::Printf("RPMB: Read State Machine - send read request packet"));

		SMF_INVOKES(CIMReadWriteBlocksSMST, EStReadResult)

    SMF_STATE(EStReadResult)
        
        OstTrace0(TRACE_FLOW, DMMCSTACK_CIMRPMBREADSM2, "RPMB: Read State Machine - sent read request packet");
		__KTRACE_OPT(KPBUS1, Kern::Printf("RPMB: Read State Machine - sent read request packet"));
		
		if (err != KMMCErrNone)
           {
           OstTrace1(TRACE_FLOW, DMMCSTACK_CIMRPMBREADSM3, "RPMB: Read State Machine - sent read request packet error = %d", err);
           __KTRACE_OPT(KPBUS1, Kern::Printf("RPMB: Read State Machine - sent read request packet error = %d", err));
           SMF_GOTOS(EStError);
           }
        //
        // Setup read response
        // CMD23 Reliable Write = 0 Block Count = 1
        // CMD18
        //        
		s.SetupRpmbReceiveResponse();
		OstTrace0(TRACE_FLOW, DMMCSTACK_CIMRPMBREADSM4, "RPMB: Read State Machine - get read packet");
		__KTRACE_OPT(KPBUS1, Kern::Printf("RPMB: Read State Machine - get read packet"));
		SMF_INVOKES(CIMReadWriteBlocksSMST, EStEnd);

    SMF_STATE(EStError)
        SMF_RETURN(err);

    SMF_END
    }

TMMCErr DMMCStack::CIMRpmbAccessSMST(TAny* aStackP)
	{return(static_cast<DMMCStack*>(aStackP)->CIMRpmbAccessSM());}
TMMCErr DMMCStack::CIMRpmbWriteAuthenticationKeySMST(TAny* aStackP)
	{return(static_cast<DMMCStack*>(aStackP)->CIMRpmbWriteAuthenticationKeySM());}
TMMCErr DMMCStack::CIMRpmbReadWrCounterSMST(TAny* aStackP)
	{return(static_cast<DMMCStack*>(aStackP)->CIMRpmbReadWrCounterSM());}
TMMCErr DMMCStack::CIMRpmbWriteSMST(TAny* aStackP)
	{return(static_cast<DMMCStack*>(aStackP)->CIMRpmbWriteSM());}
TMMCErr DMMCStack::CIMRpmbReadSMST(TAny* aStackP)
	{return(static_cast<DMMCStack*>(aStackP)->CIMRpmbReadSM());}
