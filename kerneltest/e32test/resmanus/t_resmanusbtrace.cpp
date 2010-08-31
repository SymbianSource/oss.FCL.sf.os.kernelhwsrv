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
// e32test\resmanus\t_resmanusbtrace.cpp
// 
//

#include <e32test.h>
#include <e32hal.h>
#include <d32btrace.h>
#include <e32btrace.h>
#include <e32svr.h>
#include <hal.h>
#include <u32hal.h>
#include "d_resmanusbtraceconst.h"
#include "d_resmanusbtrace.h"

_LIT(KLddFileName, "D_RESMANUSBTRACE.LDD");

RLddTest1 ldd;
GLDEF_D RTest test(_L("T_RESMANUSBTRACE"));

TInt BTraceHeaderSize = 0;

RBTrace Trace;
TUint8* BufferPtr;
TInt Count;
TLogInfo LogInfo;

CConsoleBase* console;

class CTestTraceBase
    {
public:    
    CTestTraceBase(TUint8** aBufferPtrAddr, TInt aHeaderSize) : 
            iBufferPtrAddr(aBufferPtrAddr), iHeaderSize(aHeaderSize)
        {
        iBuffer.Zero();
        }
    void SkipHeader()
        {
        *iBufferPtrAddr += iHeaderSize;
        }
    TInt Compare()
        {
	TInt i = 0;
        TPtrC8 ptr2(*iBufferPtrAddr, iBuffer.Length());
        TUint length = iBuffer.Length();
        length = (length + 0x3) & (~0x3);

        for(i = 0; i < ptr2.Length(); i++)
            {
            test.Printf(_L("%02x "), ptr2.Ptr()[i]);
            }
        test.Printf(_L("\n"));
        for(i = 0; i < iBuffer.Length(); i++)
            {
            test.Printf(_L("%02x "), iBuffer.Ptr()[i]);
            }
        test.Printf(_L("\n"));

        *iBufferPtrAddr += length;
        return (iBuffer.Compare(ptr2)==0)?KErrNone:KErrCorrupt;
        }
    void AppendUInt(TUint aVal)
        {
        iBuffer.Append((TUint8*)&aVal, sizeof(TUint));
        }
    void AppendInt(TInt aVal)
        {
        iBuffer.Append((TUint8*)&aVal, sizeof(TInt));
        }    
    void AppendDes(const TDesC8& aDesPtr)
        {
        iBuffer.Append(aDesPtr);
        }
    TUint8** iBufferPtrAddr;
    TBuf8<80> iBuffer;
    TInt iHeaderSize;
    };

class Test_PRM_US_OPEN_CHANNEL_START_TRACE : public CTestTraceBase
    {
public:
    Test_PRM_US_OPEN_CHANNEL_START_TRACE() : CTestTraceBase(&BufferPtr, BTraceHeaderSize)
        {
        AppendUInt(KClient);
        AppendUInt(KCLIENTNAME.iTypeLength);
        AppendDes(KCLIENTNAME);
        }
    };

class Test_PRM_US_OPEN_CHANNEL_END_TRACE : public CTestTraceBase
    {
public:
    Test_PRM_US_OPEN_CHANNEL_END_TRACE() : CTestTraceBase(&BufferPtr, BTraceHeaderSize)
        {
        AppendUInt((TUint)KClientHandle);
        AppendUInt(KCLIENTNAME.iTypeLength);
        AppendDes(KCLIENTNAME);
        }
    };

class Test_PRM_US_REGISTER_CLIENT_START_TRACE : public CTestTraceBase
    {
public:
    Test_PRM_US_REGISTER_CLIENT_START_TRACE() : CTestTraceBase(&BufferPtr, BTraceHeaderSize)
        {
        AppendUInt(KClientHandle);
        TUint32 stateRes32 = ((KStatsRes1&0xFF) << 16) | ((KStatsRes2&0xFF) << 8) | ((KStatsRes3&0xFF));
        AppendUInt(stateRes32);
        AppendUInt(KCLIENTNAME.iTypeLength);
        AppendDes(KCLIENTNAME);
        }
    };

class Test_PRM_US_REGISTER_CLIENT_END_TRACE : public CTestTraceBase
    {
public:
    Test_PRM_US_REGISTER_CLIENT_END_TRACE() : CTestTraceBase(&BufferPtr, BTraceHeaderSize)
        {
        AppendUInt(KClientHandle);
        AppendInt(KRetVal);
        }
    };

class Test_PRM_US_DEREGISTER_CLIENT_START_TRACE : public CTestTraceBase
    {
public:
    Test_PRM_US_DEREGISTER_CLIENT_START_TRACE() : CTestTraceBase(&BufferPtr, BTraceHeaderSize)
        {
        AppendUInt(KClientHandle);
        AppendUInt(KCLIENTNAME.iTypeLength);
        AppendDes(KCLIENTNAME);
        }
    };

class Test_PRM_US_DEREGISTER_CLIENT_END_TRACE   : public CTestTraceBase
    {
public:
    Test_PRM_US_DEREGISTER_CLIENT_END_TRACE()  : CTestTraceBase(&BufferPtr, BTraceHeaderSize)
        {
        AppendUInt(KClientHandle);
        }
    };
    
class Test_PRM_US_GET_RESOURCE_STATE_START_TRACE : public CTestTraceBase
    {
public:
    Test_PRM_US_GET_RESOURCE_STATE_START_TRACE() : CTestTraceBase(&BufferPtr, BTraceHeaderSize)
        {
        AppendUInt(KResourceId);
        AppendUInt(KClientHandle);
        AppendUInt(KCLIENTNAME.iTypeLength);
        AppendDes(KCLIENTNAME);
        }
    };

class Test_PRM_US_GET_RESOURCE_STATE_END_TRACE : public CTestTraceBase
    {
public:
    Test_PRM_US_GET_RESOURCE_STATE_END_TRACE() : CTestTraceBase(&BufferPtr, BTraceHeaderSize)
        {
        AppendUInt(KResourceId);
        AppendUInt(KLevel);
        AppendUInt(KClient);
        AppendUInt(KResult);
        }
    };

class Test_PRM_US_SET_RESOURCE_STATE_START_TRACE : public CTestTraceBase
    {
public:
    Test_PRM_US_SET_RESOURCE_STATE_START_TRACE() : CTestTraceBase(&BufferPtr, BTraceHeaderSize)
        {
        AppendUInt(KResourceId);
        AppendUInt(KLevel);
        AppendUInt(KClientHandle);
        AppendUInt(KCLIENTNAME.iTypeLength);
        AppendDes(KCLIENTNAME);
        }
    };

class Test_PRM_US_SET_RESOURCE_STATE_END_TRACE : public CTestTraceBase
    {
public:
    Test_PRM_US_SET_RESOURCE_STATE_END_TRACE() : CTestTraceBase(&BufferPtr, BTraceHeaderSize)
        {
        AppendUInt(KResourceId);
        AppendUInt(KLevel);
        AppendUInt(KClient);
        AppendUInt(KResult);
        }
    };

class Test_PRM_US_CANCEL_GET_RESOURCE_STATE_START_TRACE : public CTestTraceBase
    {
public:
    Test_PRM_US_CANCEL_GET_RESOURCE_STATE_START_TRACE() : CTestTraceBase(&BufferPtr, BTraceHeaderSize)
        {
        AppendUInt(KResourceId);
        AppendUInt(KClientHandle);
        AppendUInt(KCLIENTNAME.iTypeLength);
        AppendDes(KCLIENTNAME);
        }
    };

class Test_PRM_US_CANCEL_GET_RESOURCE_STATE_END_TRACE : public CTestTraceBase
    {
public:
    Test_PRM_US_CANCEL_GET_RESOURCE_STATE_END_TRACE() : CTestTraceBase(&BufferPtr, BTraceHeaderSize)
        {
        AppendUInt(KResourceId);
        AppendUInt(KClientHandle);
        AppendUInt(KCLIENTNAME.iTypeLength);
        AppendDes(KCLIENTNAME);
        }
    };

class Test_PRM_US_CANCEL_SET_RESOURCE_STATE_START_TRACE : public CTestTraceBase
    {
public:
    Test_PRM_US_CANCEL_SET_RESOURCE_STATE_START_TRACE() : CTestTraceBase(&BufferPtr, BTraceHeaderSize)
        {
        AppendUInt(KResourceId);
        AppendUInt(KClientHandle);
        AppendUInt(KCLIENTNAME.iTypeLength);
        AppendDes(KCLIENTNAME);
        }
    };

class Test_PRM_US_CANCEL_SET_RESOURCE_STATE_END_TRACE : public CTestTraceBase
    {
public:
    Test_PRM_US_CANCEL_SET_RESOURCE_STATE_END_TRACE() : CTestTraceBase(&BufferPtr, BTraceHeaderSize)
        {
        AppendUInt(KResourceId);
        AppendUInt(KClientHandle);
        AppendUInt(KCLIENTNAME.iTypeLength);
        AppendDes(KCLIENTNAME);
        }
    };

class Test_PRM_REGISTER_RESOURCE_TRACE : public CTestTraceBase
    {
public:
    Test_PRM_REGISTER_RESOURCE_TRACE() : CTestTraceBase(&BufferPtr, BTraceHeaderSize)
        {
        AppendUInt(KResCount+1);
        AppendUInt((TUint)(LogInfo.iPR));
        AppendInt(KMinLevel);
        AppendUInt(KMaxLevel);
        AppendUInt(KDefaultLevel);
        AppendUInt(KRESOURCENAME.iTypeLength);
        AppendDes(KRESOURCENAME);
        }
    };

class Test_PRM_CLIENT_REGISTER_TRACE : public CTestTraceBase
    {
public:
    Test_PRM_CLIENT_REGISTER_TRACE() : CTestTraceBase(&BufferPtr, BTraceHeaderSize)
        {
        AppendUInt(KClientId);
        AppendUInt((TUint)LogInfo.iPC);
        AppendUInt(KCLIENTNAME.iTypeLength);
        AppendDes(KCLIENTNAME);
        }
    };
class Test_PRM_CLIENT_DEREGISTER_TRACE : public CTestTraceBase
    {
public:
    Test_PRM_CLIENT_DEREGISTER_TRACE() : CTestTraceBase(&BufferPtr, BTraceHeaderSize)
        {
        AppendUInt(KClientId);
        AppendUInt((TUint)LogInfo.iPC);
        AppendUInt(KCLIENTNAME.iTypeLength);
        AppendDes(KCLIENTNAME);
        }
    };

class Test_PRM_CLIENT_CHANGE_STATE_START_TRACE : public CTestTraceBase
    {
public:
    Test_PRM_CLIENT_CHANGE_STATE_START_TRACE() : CTestTraceBase(&BufferPtr, BTraceHeaderSize)
        {
        AppendUInt(KClientId);
        AppendUInt(KResourceId);
        AppendUInt(KLevel);
        AppendUInt(KCLIENTNAME.iTypeLength);
        AppendDes(KCLIENTNAME);
        AppendUInt(KRESOURCENAME.iTypeLength);
        AppendDes(KRESOURCENAME);        
        }
    };

class Test_PRM_CLIENT_CHANGE_STATE_END_TRACE : public CTestTraceBase
    {
public:
    Test_PRM_CLIENT_CHANGE_STATE_END_TRACE() : CTestTraceBase(&BufferPtr, BTraceHeaderSize)
        {
        AppendUInt(KClientId);
        AppendUInt(KResourceId);
        AppendInt(KRetVal);
        AppendUInt(KLevel);
        AppendUInt(KCLIENTNAME.iTypeLength);
        AppendDes(KCLIENTNAME);
        AppendUInt(KRESOURCENAME.iTypeLength);
        AppendDes(KRESOURCENAME);         
        }
    };

class Test_PRM_POSTNOTIFICATION_REGISTER_TRACE : public CTestTraceBase
    {
public:
    Test_PRM_POSTNOTIFICATION_REGISTER_TRACE() : CTestTraceBase(&BufferPtr, BTraceHeaderSize)
        {
        AppendUInt(KClientId);
        AppendUInt(KResourceId);
        AppendUInt((TUint)(LogInfo.iPCallback));
        AppendInt(KRetVal);
        }
    };

class Test_PRM_POSTNOTIFICATION_DEREGISTER_TRACE : public CTestTraceBase
    {
public:
    Test_PRM_POSTNOTIFICATION_DEREGISTER_TRACE() : CTestTraceBase(&BufferPtr, BTraceHeaderSize)
        {
        AppendUInt(KClientId);
        AppendUInt(KResourceId);
        AppendUInt((TUint)(LogInfo.iPCallback));
        AppendInt(KRetVal);
        }
    };

class Test_PRM_POSTNOTIFICATION_SENT_TRACE : public CTestTraceBase
    {
public:
    Test_PRM_POSTNOTIFICATION_SENT_TRACE() : CTestTraceBase(&BufferPtr, BTraceHeaderSize)
        {
        AppendUInt(KClientId);
        AppendUInt(KResourceId);
        }
    };

class Test_PRM_CALLBACK_COMPLETION_TRACE : public CTestTraceBase
    {
public:
    Test_PRM_CALLBACK_COMPLETION_TRACE() : CTestTraceBase(&BufferPtr, BTraceHeaderSize)
        {
        AppendUInt(KClientId);
        AppendUInt(KResourceId);
        }
    };

class Test_PRM_MEMORY_USAGE_TRACE : public CTestTraceBase
    {
public:
    Test_PRM_MEMORY_USAGE_TRACE() : CTestTraceBase(&BufferPtr, BTraceHeaderSize)
        {
        AppendUInt(KSize);
        }
    };

class Test_PRM_PSL_RESOURCE_GET_STATE_START_TRACE : public CTestTraceBase
    {
public:
    Test_PRM_PSL_RESOURCE_GET_STATE_START_TRACE() : CTestTraceBase(&BufferPtr, BTraceHeaderSize)
        {
        AppendUInt(KClientId);
        AppendUInt(KResourceId);
        AppendUInt(KCLIENTNAME.iTypeLength);
        AppendDes(KCLIENTNAME);
        }
    };

class Test_PRM_RESOURCE_GET_STATE_START_TRACE : public CTestTraceBase
    {
public:
    Test_PRM_RESOURCE_GET_STATE_START_TRACE() : CTestTraceBase(&BufferPtr, BTraceHeaderSize)
        {
        AppendUInt(KClientId);
        AppendUInt(KResourceId);
        AppendUInt(KCLIENTNAME.iTypeLength);
        AppendDes(KCLIENTNAME);
        AppendUInt(KRESOURCENAME.iTypeLength);
        AppendDes(KRESOURCENAME);
        }
    };

class Test_PRM_PSL_RESOURCE_GET_STATE_END_TRACE : public CTestTraceBase
    {
public:
    Test_PRM_PSL_RESOURCE_GET_STATE_END_TRACE() : CTestTraceBase(&BufferPtr, BTraceHeaderSize)
        {
        AppendUInt(KClientId);
        AppendUInt(KResourceId);
        AppendUInt(KLevel);
        AppendInt(KRetVal);
        AppendUInt(KCLIENTNAME.iTypeLength);
        AppendDes(KCLIENTNAME);
        }
    };

class Test_PRM_RESOURCE_GET_STATE_END_TRACE : public CTestTraceBase
    {
public:
    Test_PRM_RESOURCE_GET_STATE_END_TRACE() : CTestTraceBase(&BufferPtr, BTraceHeaderSize)
        {
        AppendUInt(KClientId);
        AppendUInt(KResourceId);
        AppendUInt(KLevel);
        AppendInt(KRetVal);
        AppendUInt(KCLIENTNAME.iTypeLength);
        AppendDes(KCLIENTNAME);
        AppendUInt(KRESOURCENAME.iTypeLength);
        AppendDes(KRESOURCENAME);        
        }
    };

class Test_PRM_RESOURCE_CANCEL_LONGLATENCY_OPERATION_TRACE : public CTestTraceBase
    {
public:
    Test_PRM_RESOURCE_CANCEL_LONGLATENCY_OPERATION_TRACE() : CTestTraceBase(&BufferPtr, BTraceHeaderSize)
        {
        AppendUInt(KClientId);
        AppendUInt(KResourceId);
        AppendInt(KRetVal);
        AppendUInt(KCLIENTNAME.iTypeLength);
        AppendDes(KCLIENTNAME);
        AppendUInt(KRESOURCENAME.iTypeLength);
        AppendDes(KRESOURCENAME);     
        }
    };

class Test_PRM_PSL_RESOURCE_CHANGE_STATE_START_TRACE : public CTestTraceBase
    {
public:
    Test_PRM_PSL_RESOURCE_CHANGE_STATE_START_TRACE() : CTestTraceBase(&BufferPtr, BTraceHeaderSize)
        {
        AppendUInt(KClientId);
        AppendUInt(KResourceId);
        AppendUInt(KLevel);
        AppendUInt(KLevel);
        AppendUInt(KCLIENTNAME.iTypeLength);
        AppendDes(KCLIENTNAME);
        }
    };

class Test_PRM_PSL_RESOURCE_CHANGE_STATE_END_TRACE : public CTestTraceBase
    {
public:
    Test_PRM_PSL_RESOURCE_CHANGE_STATE_END_TRACE() : CTestTraceBase(&BufferPtr, BTraceHeaderSize)
        {
        AppendUInt(KClientId);
        AppendUInt(KResourceId);
        AppendUInt(KLevel);
        AppendUInt(KLevel);
        AppendInt(KRetVal);
        AppendUInt(KCLIENTNAME.iTypeLength);
        AppendDes(KCLIENTNAME);    
        }
    };

class Test_PRM_PSL_RESOURCE_CREATE_TRACE : public CTestTraceBase
    {
public:
    Test_PRM_PSL_RESOURCE_CREATE_TRACE() : CTestTraceBase(&BufferPtr, BTraceHeaderSize)
        {
        AppendInt(KMinLevel);
        AppendInt(KMaxLevel);
        AppendInt(KDefaultLevel);
        AppendUInt(KFlags);
        AppendUInt(KCLIENTNAME.iTypeLength);
        AppendDes(KCLIENTNAME);
        }
    };

class Test_PRM_BOOTING_TRACE : public CTestTraceBase
    {
public:
    Test_PRM_BOOTING_TRACE() : CTestTraceBase(&BufferPtr, BTraceHeaderSize)
        {
        AppendInt(KErrNoMemory);
        }
    };

class Test_PRM_REGISTER_STATIC_RESOURCE_WITH_DEPENDENCY_TRACE : public CTestTraceBase
    {
public:
    Test_PRM_REGISTER_STATIC_RESOURCE_WITH_DEPENDENCY_TRACE() : CTestTraceBase(&BufferPtr, BTraceHeaderSize)
        {
        AppendUInt(KResourceId);
        AppendUInt((TUint)(LogInfo.iPR));
        AppendInt(KMinLevel);
        AppendInt(KMaxLevel);
        AppendInt(KDefaultLevel);
        AppendUInt(KRESOURCENAME.iTypeLength);
        AppendDes(KRESOURCENAME);        
        }
    };

class Test_PRM_REGISTER_DYNAMIC_RESOURCE_TRACE : public CTestTraceBase
    {
public:
    Test_PRM_REGISTER_DYNAMIC_RESOURCE_TRACE() : CTestTraceBase(&BufferPtr, BTraceHeaderSize)
        {
        AppendUInt(KClientId);
        AppendUInt(KResourceId);
        AppendUInt((TUint)(LogInfo.iPR));
        AppendUInt(KCLIENTNAME.iTypeLength);
        AppendDes(KCLIENTNAME);
        AppendUInt(KRESOURCENAME.iTypeLength);
        AppendDes(KRESOURCENAME);
        }
    };

class Test_PRM_DEREGISTER_DYNAMIC_RESOURCE_TRACE : public CTestTraceBase
    {
public:
    Test_PRM_DEREGISTER_DYNAMIC_RESOURCE_TRACE() : CTestTraceBase(&BufferPtr, BTraceHeaderSize)
        {
        AppendUInt(KClientId);
        AppendUInt(KResourceId);
        AppendUInt((TUint)(LogInfo.iPR));
        AppendUInt(KLevel);
        AppendUInt(KCLIENTNAME.iTypeLength);
        AppendDes(KCLIENTNAME);
        AppendUInt(KRESOURCENAME.iTypeLength);
        AppendDes(KRESOURCENAME);
        }
    };

class Test_PRM_REGISTER_RESOURCE_DEPENDENCY_TRACE : public CTestTraceBase
    {
public:
    Test_PRM_REGISTER_RESOURCE_DEPENDENCY_TRACE() : CTestTraceBase(&BufferPtr, BTraceHeaderSize)
        {
        AppendUInt(KClientId);
        AppendUInt(KResourceId);
        AppendUInt(KResourceId);
        AppendUInt((TUint)(LogInfo.iPR));
        AppendUInt((TUint)(LogInfo.iPR));
        AppendUInt(KCLIENTNAME.iTypeLength);
        AppendDes(KCLIENTNAME);
        AppendUInt(KRESOURCENAME.iTypeLength);
        AppendDes(KRESOURCENAME);        
        AppendUInt(KRESOURCENAME.iTypeLength);
        AppendDes(KRESOURCENAME);  
        }
    };

class Test_PRM_DEREGISTER_RESOURCE_DEPENDENCY_TRACE : public CTestTraceBase
    {
public:
    Test_PRM_DEREGISTER_RESOURCE_DEPENDENCY_TRACE() : CTestTraceBase(&BufferPtr, BTraceHeaderSize)
        {
        AppendUInt(KClientId);
        AppendUInt(KResourceId);
        AppendUInt(KResourceId);
        AppendUInt((TUint)(LogInfo.iPR));
        AppendUInt((TUint)(LogInfo.iPR));
        AppendUInt(KCLIENTNAME.iTypeLength);
        AppendDes(KCLIENTNAME);
        AppendUInt(KRESOURCENAME.iTypeLength);
        AppendDes(KRESOURCENAME);        
        AppendUInt(KRESOURCENAME.iTypeLength);
        AppendDes(KRESOURCENAME);      
        }
    };

void TestMacro()
    {
    const TInt numCpus = UserSvr::HalFunction(EHalGroupKernel, EKernelHalNumLogicalCpus, 0, 0);
    if(numCpus>1)
    	{
	BTraceHeaderSize = 20;
	}
    else
        {
	BTraceHeaderSize = 12;
	}

    Test_PRM_US_OPEN_CHANNEL_START_TRACE testPRM_US_OPEN_CHANNEL_START_TRACE; //0
    Test_PRM_US_OPEN_CHANNEL_END_TRACE testPRM_US_OPEN_CHANNEL_END_TRACE; //1
    Test_PRM_US_REGISTER_CLIENT_START_TRACE testPRM_US_REGISTER_CLIENT_START_TRACE; //2
    Test_PRM_US_REGISTER_CLIENT_END_TRACE testPRM_US_REGISTER_CLIENT_END_TRACE; //3
    Test_PRM_US_DEREGISTER_CLIENT_START_TRACE testPRM_US_DEREGISTER_CLIENT_START_TRACE; //4
    Test_PRM_US_DEREGISTER_CLIENT_END_TRACE   testPRM_US_DEREGISTER_CLIENT_END_TRACE; //5
    Test_PRM_US_GET_RESOURCE_STATE_START_TRACE testPRM_US_GET_RESOURCE_STATE_START_TRACE; //6
    Test_PRM_US_GET_RESOURCE_STATE_END_TRACE testPRM_US_GET_RESOURCE_STATE_END_TRACE; //7
    Test_PRM_US_SET_RESOURCE_STATE_START_TRACE testPRM_US_SET_RESOURCE_STATE_START_TRACE; //8
    Test_PRM_US_SET_RESOURCE_STATE_END_TRACE testPRM_US_SET_RESOURCE_STATE_END_TRACE; //9
    Test_PRM_US_CANCEL_GET_RESOURCE_STATE_START_TRACE testPRM_US_CANCEL_GET_RESOURCE_STATE_START_TRACE; //10
    Test_PRM_US_CANCEL_GET_RESOURCE_STATE_END_TRACE testPRM_US_CANCEL_GET_RESOURCE_STATE_END_TRACE; //11
    Test_PRM_US_CANCEL_SET_RESOURCE_STATE_START_TRACE testPRM_US_CANCEL_SET_RESOURCE_STATE_START_TRACE; //12
    Test_PRM_US_CANCEL_SET_RESOURCE_STATE_END_TRACE testPRM_US_CANCEL_SET_RESOURCE_STATE_END_TRACE; //13
    Test_PRM_REGISTER_RESOURCE_TRACE testPRM_REGISTER_RESOURCE_TRACE; //14
    Test_PRM_CLIENT_REGISTER_TRACE testPRM_CLIENT_REGISTER_TRACE; //15
    Test_PRM_CLIENT_DEREGISTER_TRACE testPRM_CLIENT_DEREGISTER_TRACE; //16
    Test_PRM_CLIENT_CHANGE_STATE_START_TRACE testPRM_CLIENT_CHANGE_STATE_START_TRACE; //17
    Test_PRM_CLIENT_CHANGE_STATE_END_TRACE testPRM_CLIENT_CHANGE_STATE_END_TRACE; //18
    Test_PRM_POSTNOTIFICATION_REGISTER_TRACE testPRM_POSTNOTIFICATION_REGISTER_TRACE; //19
    Test_PRM_POSTNOTIFICATION_DEREGISTER_TRACE testPRM_POSTNOTIFICATION_DEREGISTER_TRACE; //20
    Test_PRM_POSTNOTIFICATION_SENT_TRACE testPRM_POSTNOTIFICATION_SENT_TRACE; //21
    Test_PRM_CALLBACK_COMPLETION_TRACE testPRM_CALLBACK_COMPLETION_TRACE; //22
    Test_PRM_MEMORY_USAGE_TRACE testPRM_MEMORY_USAGE_TRACE; //23
    Test_PRM_PSL_RESOURCE_GET_STATE_START_TRACE testPRM_PSL_RESOURCE_GET_STATE_START_TRACE; //24
    Test_PRM_RESOURCE_GET_STATE_START_TRACE testPRM_RESOURCE_GET_STATE_START_TRACE; //25
    Test_PRM_PSL_RESOURCE_GET_STATE_END_TRACE testPRM_PSL_RESOURCE_GET_STATE_END_TRACE; //26
    Test_PRM_RESOURCE_GET_STATE_END_TRACE testPRM_RESOURCE_GET_STATE_END_TRACE; //27
    Test_PRM_RESOURCE_CANCEL_LONGLATENCY_OPERATION_TRACE testPRM_RESOURCE_CANCEL_LONGLATENCY_OPERATION_TRACE; //28
    Test_PRM_PSL_RESOURCE_CHANGE_STATE_START_TRACE testPRM_PSL_RESOURCE_CHANGE_STATE_START_TRACE; //29
    Test_PRM_PSL_RESOURCE_CHANGE_STATE_END_TRACE testPRM_PSL_RESOURCE_CHANGE_STATE_END_TRACE; //30
    Test_PRM_PSL_RESOURCE_CREATE_TRACE testPRM_PSL_RESOURCE_CREATE_TRACE; //31
    Test_PRM_BOOTING_TRACE testPRM_BOOTING_TRACE; //32
    Test_PRM_REGISTER_STATIC_RESOURCE_WITH_DEPENDENCY_TRACE testPRM_REGISTER_STATIC_RESOURCE_WITH_DEPENDENCY_TRACE; //33
    Test_PRM_REGISTER_DYNAMIC_RESOURCE_TRACE testPRM_REGISTER_DYNAMIC_RESOURCE_TRACE; //34
    Test_PRM_DEREGISTER_DYNAMIC_RESOURCE_TRACE testPRM_DEREGISTER_DYNAMIC_RESOURCE_TRACE; //35
    Test_PRM_REGISTER_RESOURCE_DEPENDENCY_TRACE testPRM_REGISTER_RESOURCE_DEPENDENCY_TRACE; //36
    Test_PRM_DEREGISTER_RESOURCE_DEPENDENCY_TRACE testPRM_DEREGISTER_RESOURCE_DEPENDENCY_TRACE; //37
    
//    const TInt KNumTest = 38;
    CTestTraceBase* TestArray[] = 
            {
            &testPRM_US_OPEN_CHANNEL_START_TRACE,
            &testPRM_US_OPEN_CHANNEL_END_TRACE,
            &testPRM_US_REGISTER_CLIENT_START_TRACE,
            &testPRM_US_REGISTER_CLIENT_END_TRACE,
            &testPRM_US_DEREGISTER_CLIENT_START_TRACE,
            &testPRM_US_DEREGISTER_CLIENT_END_TRACE,
            &testPRM_US_GET_RESOURCE_STATE_START_TRACE,
            &testPRM_US_GET_RESOURCE_STATE_END_TRACE,
            &testPRM_US_SET_RESOURCE_STATE_START_TRACE,
            &testPRM_US_SET_RESOURCE_STATE_END_TRACE,
            &testPRM_US_CANCEL_GET_RESOURCE_STATE_START_TRACE,
            &testPRM_US_CANCEL_GET_RESOURCE_STATE_END_TRACE,
            &testPRM_US_CANCEL_SET_RESOURCE_STATE_START_TRACE,
            &testPRM_US_CANCEL_SET_RESOURCE_STATE_END_TRACE,
            &testPRM_REGISTER_RESOURCE_TRACE,
            &testPRM_CLIENT_REGISTER_TRACE,
            &testPRM_CLIENT_DEREGISTER_TRACE,
            &testPRM_CLIENT_CHANGE_STATE_START_TRACE,
            &testPRM_CLIENT_CHANGE_STATE_END_TRACE,
            &testPRM_POSTNOTIFICATION_REGISTER_TRACE,
            &testPRM_POSTNOTIFICATION_DEREGISTER_TRACE,
            &testPRM_POSTNOTIFICATION_SENT_TRACE,
            &testPRM_CALLBACK_COMPLETION_TRACE,
            &testPRM_MEMORY_USAGE_TRACE,
            &testPRM_PSL_RESOURCE_GET_STATE_START_TRACE,
            &testPRM_RESOURCE_GET_STATE_START_TRACE,
            &testPRM_PSL_RESOURCE_GET_STATE_END_TRACE,
            &testPRM_RESOURCE_GET_STATE_END_TRACE,
            &testPRM_RESOURCE_CANCEL_LONGLATENCY_OPERATION_TRACE,
            &testPRM_PSL_RESOURCE_CHANGE_STATE_START_TRACE,
            &testPRM_PSL_RESOURCE_CHANGE_STATE_END_TRACE,
            &testPRM_PSL_RESOURCE_CREATE_TRACE,
            &testPRM_BOOTING_TRACE,
            &testPRM_REGISTER_STATIC_RESOURCE_WITH_DEPENDENCY_TRACE,
            &testPRM_REGISTER_DYNAMIC_RESOURCE_TRACE,
            &testPRM_DEREGISTER_DYNAMIC_RESOURCE_TRACE,
            &testPRM_REGISTER_RESOURCE_DEPENDENCY_TRACE,
            &testPRM_DEREGISTER_RESOURCE_DEPENDENCY_TRACE
            };
    
    TInt r = KErrNone;
    
    for(TUint i = 0; i < sizeof(TestArray)/sizeof(CTestTraceBase*); i++)
        {
        TestArray[i]->SkipHeader();
        test.Printf(_L("\n\nTest number %d\n\n"), i);
        r = TestArray[i]->Compare();
        test(r==KErrNone);
        }
    
    }

void DoTests()
	{
	TInt r = KErrNone;

	test.Printf(_L("Loading logical device \n"));
	r=User::LoadLogicalDevice(KLddFileName);
	test(r == KErrNone);

	test.Printf(_L("Opening of logical device\n"));
	r = ldd.Open();
	test(r == KErrNone);

	Trace.Open();
	TInt OrgBufSize = Trace.BufferSize();
	if(OrgBufSize<1024)
	    Trace.ResizeBuffer(1024);
	
    Trace.Empty();
    Trace.SetMode(RBTrace::EEnable);
	
	Trace.SetFilter(BTrace::EResourceManagerUs, ETrue);
	Trace.SetFilter(BTrace::EResourceManager, ETrue);	

    test.Printf(_L("Test Cat 19 is enabled"));
	test(Trace.Filter(19));
    test.Printf(_L("Test Cat 20 is enabled"));
	test(Trace.Filter(20));
    
	test.Printf(_L("Send log\n"));
	r = ldd.SendLog(&LogInfo);
	test(r == KErrNone);

	Trace.GetData(BufferPtr);
    
	TestMacro();
	
	Trace.DataUsed();
	
	Trace.SetFilter(BTrace::EResourceManagerUs, EFalse);
	Trace.SetFilter(BTrace::EResourceManager, EFalse);	
	
	if(OrgBufSize<1024)
	    Trace.ResizeBuffer(1024);
	
	Trace.Close();
	
	test.Printf(_L("\nClosing the channel\n"));
	ldd.Close();

	test.Printf(_L("Freeing logical device\n"));
	r = User::FreeLogicalDevice(KLddFileName);;
	test(r==KErrNone);


	}

GLDEF_C TInt E32Main()
    {
	test.Start(_L("T_RESMANUSBTRACE"));
	console = test.Console();
	DoTests();
	test.End();
	test.Close();
		
 	return(KErrNone);
    }
