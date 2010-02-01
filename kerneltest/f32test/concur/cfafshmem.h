// Copyright (c) 1997-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// /os/kernelhwsrv/kerneltest/f32test/concur/cfafshmem.h
// 
//

#ifndef __CFAFSHMEM_H__
#define __CFAFSHMEM_H__

#include <f32file.h>
#include <d32locd.h>
#include <e32test.h>
#include "t_server.h"

const TInt KNoMessage = 0;
const TInt KNoOffset = 0;

class THMem
/**
 * Memory base class.
 */
    {
    private:
        /// Copy constructor and assignment operator shall not be implemented.
        THMem(const THMem&);
        THMem& operator=(const THMem&);

    public:
        THMem(void * aNewMemPtr);
        THMem(const void * aNewMemPtr, const RMessagePtr2 &aMessage, int aNewOffset=0);
        TInt Read(void *aBuf, TUint32 aLength, TUint aOffset = 0) const;
        TInt Write(const void *aBuf, TUint32 aLength, TUint32 aOffset = 0);
        inline operator TUint8*()  const { return (TUint8*)(iMemPtr); }
        inline operator TUint16*() const { return (TUint16*)(iMemPtr); }
        inline operator TUint32*() const { return (TUint32*)(iMemPtr); }
    private:
        void*         iMemPtr;
        RMessagePtr2* iMessage;
        TInt          iBaseOffset;
    };

#endif
