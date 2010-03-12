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
// e32test\entropysources\d_entropysources.h
//
// Test driver which notifies userspace when an RNG reseed has taken place
// 
//

/**
 @file
 @internalComponent
 @test
*/

#if !defined(__D_ENTROPYSOURCES_H__)
#define __D_ENTROPYSOURCES_H__

#include <e32cmn.h>
#ifndef __KERNEL_MODE__
#include <e32std.h>
#endif

_LIT(KEntropySourcesName,"D_ENTROPYSOURCES");

class TCapsEntropySources
    {
public:
    TVersion iVersion;
    };

class REntropySources : public RBusLogicalChannel
    {
public:
  enum TRequest
        {
        EReseedTest,        
        };
    
#ifndef __KERNEL_MODE__
public:
    inline TInt Open()
        { 
        return DoCreate(KEntropySourcesName,TVersion(0, 1, 1),KNullUnit,NULL,NULL,EOwnerThread);    
        }
    inline void ReseedTest(TRequestStatus& aRequestStatus)
        {
        DoRequest(EReseedTest, aRequestStatus);
        }
#endif
    };

#endif // __D_ENTROPYSOURCES_H__
